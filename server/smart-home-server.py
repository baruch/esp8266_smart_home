#!/usr/bin/python

import discovery
import ota
import time
import threading
import time
import sys
import os
import md5
import paho.mqtt.client as mqtt

def choose_ip_addr(iface_addrs):
    for iface_addr in iface_addrs:
        if iface_addr[1] == 'eth0': return iface_addr[0]
    return iface_addrs[0][1]

try:
    import netifaces
    def get_server_ip():
        PROTO = netifaces.AF_INET
        ifaces = netifaces.interfaces()
        if_addrs = [(netifaces.ifaddresses(iface), iface) for iface in ifaces]
        if_inet_addrs = [(tup[0][PROTO], tup[1]) for tup in if_addrs if PROTO in tup[0]]
        iface_addrs = [(s['addr'], tup[1]) for tup in if_inet_addrs for s in tup[0] if 'addr' in s]
        print 'interface ips', iface_addrs
        ip = choose_ip_addr(iface_addrs)
        print 'ip', ip
        return ip
except ImportError:
    import socket

    def get_server_ip():
        import socket
        return socket.gethostbyname(socket.gethostname())

def mqtt_connected(client, userdata, flags, rc):
    print('MQTT connected')

def mqtt_message(client, userdata, msg):
    print('MQTT message received')

def mqtt_published(client, userdata, msg):
    print('MQTT message published')

client = mqtt.Client()
client.on_connect = mqtt_connected
client.on_message = mqtt_message
client.on_publish = mqtt_published

def mqtt_topic(node_id, name):
    return "shm/%s/%s" % (node_id, name)

def mqtt_publish(node_id, name, value):
    print client.publish(mqtt_topic(node_id, name), value, 1, True)

class NodeList:
    def __init__(self):
        self._list = {}
        self.lock = threading.Lock()

    def get_node_by_ip(self, ip):
        if ip == '127.0.0.1': return {'node_id': 'local', 'node_type': 'test', 'node_desc': 'test'}
        with self.lock:
            val = self._list.get(ip, None)
        return val

    def update_node(self, node_ip, node_id, node_type, node_desc, version):
        with self.lock:
            print 'Updating node ip=%s id=%s type=%s desc=%s version=%s' % (node_ip, node_id, node_type, node_desc, version)
            self._list[node_ip] = {'node_id': node_id, 'node_type': node_type, 'node_desc': node_desc, 'version': version, 'last_seen_cpu': time.clock(), 'last_seen_wall': time.time()}
        mqtt_publish(node_id, "status", "discovery")
        mqtt_publish(node_id, "version", version)
        mqtt_publish(node_id, "desc", node_desc)

class OTAFile:
    def __init__(self, otaname):
        self._otaname = otaname
        self._content, self._md5, self._version = self._load_file()
        self.lock = threading.Lock()

    def _load_file(self):
        content = file(self._otaname, 'r').read()
        content_md5 = md5.new(content).hexdigest()

        # Find the version in the image
        idx = content.find('SHMVER-')
        version = content[idx:]
        idx = version.find('\000')
        version = version[:idx]

        print('Loaded image, md5=%s version=%s len=%d' % (content_md5, version, len(content)))
        return content, content_md5, version

    def update_file(self):
        with self.lock:
            content, content_md5, version = self._load_file()
            if content_md5 != self._md5:
                print('File updated, new md5 is %s' % content_md5)
                self._md5 = content_md5
                self._content = content
                self._version = version

    def check_update(self, version):
        self.update_file()
        if version != self._version:
            return self._md5, self._content
        return None, None

def usage():
    print("USAGE: %s file_repository_folder" % sys.argv[0])
    exit(1)

def main():
    if len(sys.argv) != 2:
        usage()

    otaname = sys.argv[1]
    if not os.path.isfile(otaname):
        print 'ERROR: Path "%s" doesn\'t exist or is not a file\n' % otaname
        usage()

    print client.connect('127.0.0.1', 1883, 60)
    node_list = NodeList()
    otafile = OTAFile(otaname)

    server_ip = get_server_ip()
    print 'Server IP is', server_ip
    discovery.start(server_ip, 1883, node_list)
    ota.start(node_list, otafile)
    while 1:
        client.loop()

if __name__ == '__main__':
    main()
