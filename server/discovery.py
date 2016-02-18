import SocketServer
import threading
import socket

HOST = '0.0.0.0'
PORT = 24320
server_ip = ''

def set_server_ip(_server_ip):
    global server_ip
    parts = _server_ip.split('.')
    parts_int = tuple(map(lambda x: int(x), parts))
    server_ip = '\004%c%c%c%c' % parts_int

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

class DiscoveryUDPServer(SocketServer.BaseRequestHandler):
    def handle(self):
        data = self.request[0]
        socket = self.request[1]

        # parse data
        node_id, node_type, node_desc = parse_message(data)
        if node_id is None: return

        # send response
        response = 'R' + server_ip
        
        socket.sendto(response, self.client_address)

def start(server_ip):
    set_server_ip(server_ip)
    server = SocketServer.UDPServer((HOST, PORT), DiscoveryUDPServer)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
