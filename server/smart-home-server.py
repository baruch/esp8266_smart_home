#!/usr/bin/python

import discovery
import ota
import time
import threading
import time
import fletcher
import sys
import os

def choose_ip_addr(iface_addrs):
    for iface_addr in iface_addrs:
        if iface_addr[1] == 'eth0': return iface_addr[0]
    return iface_addrs[0][1]

try:
    import netifaces1
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
    import fcntl
    import struct

    def get_ip_address(ifname):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915,  # SIOCGIFADDR
            struct.pack('256s', ifname[:15])
        )[20:24])

    def get_server_ip():
        return get_ip_address('eth0')

class NodeList:
    def __init__(self):
        self._list = {}
        self.lock = threading.Lock()

    def get_node_by_ip(self, ip):
        with self.lock:
            val = self._list.get(ip, None)
        return val

    def update_node(self, node_ip, node_id, node_type, node_desc):
        with self.lock:
            print 'Updating node ip=%s id=%s type=%s desc=%s' % (node_ip, node_id, node_type, node_desc)
            self._list[node_ip] = {'node_id': node_id, 'node_type': node_type, 'node_desc': node_desc, 'last_seen_cpu': time.clock(), 'last_seen_wall': time.time()}

class FileList:
    def __init__(self):
        self._files = {}
        self.lock = threading.Lock()

    def update_file(self, filename, content):
        with self.lock:
            self._files[filename] = (fletcher.fletcher16(content), content)

    def get_files_for_node(self, node):
        # never mind the node, we have one list for now
        l = []
        with self.lock:
            for fname in self._files.keys():
                checksum, content = self._files[fname]
                i = (fname, checksum, content)
                if fname == 'init.lua':
                    # init.lua is better updated at the end
                    l.append(i)
                else:
                    l.insert(0, i)
        return l

def update_file_list(file_list):
    dirname = sys.argv[1]
    for f in os.listdir(dirname):
        if f.endswith('.lua') or f.endswith('.html'):
            filename = os.path.join(dirname, f)
            print 'Adding file %s' % filename
            file_list.update_file(os.path.basename(filename), file(filename).read())

def main():
    node_list = NodeList()
    file_list = FileList()
    update_file_list(file_list)

    server_ip = get_server_ip()
    print 'Server IP is', server_ip
    discovery.start(server_ip, node_list)
    ota.start(node_list, file_list)
    while 1:
        time.sleep(1)

if __name__ == '__main__':
    main()
