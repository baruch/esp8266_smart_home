import SocketServer
import threading
import fletcher

HOST = '0.0.0.0'
PORT = 24320

def get_file_data(files, fname):
    for f in files:
        if f[0] == fname:
            return f[2]
    return ''

class OTAHandler(SocketServer.StreamRequestHandler):

    def handle(self):
        node = self.server.node_list.get_node_by_ip(self.client_address[0])
        if node is None:
            print 'Unknown node'
            return

        self.server.file_list.update_list()
        files = self.server.file_list.get_files_for_node(node)
        if files is None:
            print 'No files for node'
            return

        while 1:
            req = self.rfile.readline()
            req = req.strip()
            print req
            if req == '': break
            parts = req.split()
            if len(parts) == 0: return
            print parts

            reply = ''
            if parts[0] == 'list':
                for f in files:
                    reply += '%s,%d,%d\n' % (f[0], f[1][0], f[1][1])
                print 'reply', reply
                self.wfile.write(reply + '\n')
            elif parts[0] == 'file':
                if len(parts) < 2: return
                fname = parts[1]
                content = get_file_data(files, fname)
                header = '%d\n' % len(content)
                self.wfile.write(header + content)
            else:
                return

class OTAServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    allow_reuse_address = True
    
    def setup(self, node_list, file_list):
        self.node_list = node_list
        self.file_list = file_list

def start(node_list, file_list):
    server = OTAServer((HOST, PORT), OTAHandler)
    server.setup(node_list, file_list)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
