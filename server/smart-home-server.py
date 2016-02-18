#!/usr/bin/python

import discovery
import time
import netifaces

def choose_ip_addr(iface_addrs):
    for iface_addr in iface_addrs:
        if iface_addr[1] == 'eth0': return iface_addr[0]
    return iface_addrs[0][1]

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

def main():
    discovery.start(get_server_ip())
    while 1:
        time.sleep(1)

if __name__ == '__main__':
    main()
