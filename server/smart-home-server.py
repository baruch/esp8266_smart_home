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

class NodeList:
    def __init__(self):
        self._list = {}
        self.lock = threading.Lock()

    def get_node_by_ip(self, ip):
        if ip == '127.0.0.1': return {'node_id': 'local', 'node_type': 'test', 'node_desc': 'test'}
        with self.lock:
            val = self._list.get(ip, None)
        return val

    def update_node(self, node_ip, node_id, node_type, node_desc):
        with self.lock:
            print 'Updating node ip=%s id=%s type=%s desc=%s' % (node_ip, node_id, node_type, node_desc)
            self._list[node_ip] = {'node_id': node_id, 'node_type': node_type, 'node_desc': node_desc, 'last_seen_cpu': time.clock(), 'last_seen_wall': time.time()}

class FileList:
    def __init__(self, dirname):
        self._files = {}
        self._dirname = dirname
        self.lock = threading.Lock()

    def update_list(self):
        for f in os.listdir(self._dirname):
            if f.endswith('.lua') or f.endswith('.html'):
                filename = os.path.join(self._dirname, f)
                print 'Adding file %s' % filename
                self.update_file(os.path.basename(filename), file(filename).read())



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

def usage():
    print("USAGE: %s file_repository_folder" % sys.argv[0])
    exit(1)

def main():
    if len(sys.argv) != 2:
        usage()

    dirname = sys.argv[1]
    if not os.path.isdir(dirname):
        print 'ERROR: Path "%s" doesn\'t exist or is not a directory\n' % dirname
        usage()

    node_list = NodeList()
    file_list = FileList(dirname)

    server_ip = get_server_ip()
    print 'Server IP is', server_ip
    discovery.start(server_ip, node_list)
    ota.start(node_list, file_list)
    while 1:
        time.sleep(1)

if __name__ == '__main__':
    main()
