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

def parse_message(data):
    print 'message', data
    if data[0] != 'S': return (None, None, None)

    node_id, data = parse_message_part(data[1:])
    node_type, data = parse_message_part(data)
    node_desc, data = parse_message_part(data)

    if node_id is None or node_type is None: return (None, None, None)
    return (node_id, node_type, node_desc)

class DiscoveryUDPHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        data = self.request[0]
        socket = self.request[1]

        # parse data
        node_id, node_type, node_desc = parse_message(data)
        if node_id is None: return

        node_ip = self.client_address[0]
        self.server.node_list.update_node(node_ip, node_id, node_type, node_desc)

        # send response
        response = 'R' + self.server.server_ip + self.server.mqtt_port
        
        socket.sendto(response, self.client_address)

class DiscoveryServer(SocketServer.UDPServer):
    def set_server_ip(self, _server_ip, _port):
        parts = _server_ip.split('.')
        parts_int = tuple(map(lambda x: int(x), parts))
        self.server_ip = '\004%c%c%c%c' % parts_int
        port_net = socket.htons(_port)
        port_high = port_net >> 8
        port_low = port_net & 0xFF
        self.mqtt_port = '\002%c%c' % (port_high, port_low)

    def set_node_list(self, node_list):
        self.node_list = node_list


def start(server_ip, port, node_list):
    server = DiscoveryServer((HOST, PORT), DiscoveryUDPHandler)
    server.set_server_ip(server_ip, port)
    server.set_node_list(node_list)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
