import SocketServer
import threading
import socket

HOST = '0.0.0.0'
PORT = 24320

def parse_message_part(data):
    if len(data) < 1: return (None, '')
    part_len = ord(data[0])
    if len(data) < part_len + 1: return (None, '')
    return (data[1:part_len+1], data[part_len+1:])

def translate_node_type(node_type):
    try:
        return int(node_type)
    except ValueError:
        return 0

def translate_ip(raw):
    if raw is None or len(raw) != 4:
        return '0.0.0.0'

    return '%d.%d.%d.%d' % (ord(raw[0]), ord(raw[1]), ord(raw[2]), ord(raw[3]))

def parse_message(data):
    print 'message', data
    if data[0] != 'S': return (None, None, None, None, None, None, None, None)
    data = data[1:] # Remove the leading S

    node_id, data = parse_message_part(data)
    node_type, data = parse_message_part(data)
    node_desc, data = parse_message_part(data)
    version, data = parse_message_part(data)
    static_ip, data = parse_message_part(data)
    static_gw, data = parse_message_part(data)
    static_nm, data = parse_message_part(data)
    dns, data = parse_message_part(data)

    if node_id is None or node_type is None: return (None, None, None, None, None, None, None, None)

    node_type = translate_node_type(node_type)
    static_ip = translate_ip(static_ip)
    static_gw = translate_ip(static_gw)
    static_nm = translate_ip(static_nm)
    dns = translate_ip(dns)
    return (node_id, node_type, node_desc, version, static_ip, static_gw, static_nm, dns)

def encode_str(s):
    l = '%c' % len(s)
    return l+s

def encode_ip(ip):
    parts = ip.split('.')
    parts_int = tuple(map(lambda x: int(x), parts))
    return '\004%c%c%c%c' % parts_int

def encode_port(port):
    port_net = socket.htons(port)
    port_high = port_net >> 8
    port_low = port_net & 0xFF
    return '\002%c%c' % (port_high, port_low)

class DiscoveryUDPHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        data = self.request[0]
        socket = self.request[1]

        # parse data
        node_id, node_type, node_desc, version, static_ip, static_gw, static_nm, dns = parse_message(data)
        if node_id is None: return

        node_ip = self.client_address[0]
        node_desc, static_ip, static_gw, static_nm, dns = self.server.node_list.update_node(node_ip, node_id, node_type, node_desc, version, static_ip, static_gw, static_nm, dns)

        # send response
        response = 'R' + self.server.server_ip + self.server.mqtt_port + encode_str(node_desc) + encode_ip(static_ip) + encode_ip(static_gw) + encode_ip(static_nm) + encode_ip(dns)
        
        socket.sendto(response, self.client_address)

class DiscoveryServer(SocketServer.UDPServer):
    def set_server_ip(self, _server_ip, _port):
        self.server_ip = encode_ip(_server_ip)
        self.mqtt_port = encode_port(_port)

    def set_node_list(self, node_list):
        self.node_list = node_list


def start(server_ip, port, node_list):
    server = DiscoveryServer((HOST, PORT), DiscoveryUDPHandler)
    server.set_server_ip(server_ip, port)
    server.set_node_list(node_list)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
