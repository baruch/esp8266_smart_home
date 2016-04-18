import SocketServer
import threading
import socket

HOST = '0.0.0.0'
PORT = 24321

class LogHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        client_address = self.client_address[0]
        while 1:
            try:
                data = self.request.recv(4096)
            except IOError:
                break
            if not data: break
            self.server.node_list.log(client_address, data)

class LogServer(SocketServer.ThreadingTCPServer):
    def set_node_list(self, node_list):
        self.node_list = node_list

def start(node_list):
    print 'Starting log server on port ', PORT
    server = LogServer((HOST, PORT), LogHandler)
    server.set_node_list(node_list)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
