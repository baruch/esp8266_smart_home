import SocketServer
import threading
import socket

HOST = '0.0.0.0'
PORT = 24321

class LogHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        client_address = self.client_address[0]
        self.server.node_list.log(client_address, "Server: Node connect to log server")
        while 1:
            try:
                data = self.request.recv(4096)
            except IOError:
                break
            if not data: break
            self.server.node_list.log(client_address, data)
        self.server.node_list.log(client_address, "Server: Node disconnected from log server")
        try:
            self.request.shutdown(socket.SHUT_RDWR)
        except IOError:
            pass

class LogServer(SocketServer.ThreadingTCPServer):
    def __init__(self, address, handler, node_list):
        self.allow_reuse_address = True
        SocketServer.ThreadingTCPServer.__init__(self, address, handler)
        self.node_list = node_list

def start(node_list):
    print 'Starting log server on port ', PORT
    server = LogServer((HOST, PORT), LogHandler, node_list)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
