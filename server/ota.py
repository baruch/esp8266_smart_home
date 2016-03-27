import threading
import BaseHTTPServer

HOST = '0.0.0.0'
PORT = 24320

class OTAHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    def do_GET(self):
        node = self.server.node_list.get_node_by_ip(self.client_address[0])
        if node is None:
            self.send_error(403, 'Unknown Node')
            return

        print self.headers

        _version = self.headers.get('x-ESP8266-version')
        _md5, content = self.server.otafile.check_update(_version)
        if content is None:
            self.send_error(304, 'Not Modified')
            return

        self.send_response(200)
        self.send_header('Content-Length', len(content))
        self.send_header('x-MD5', _md5)
        self.end_headers()
        self.wfile.write(content)
        return


class OTAServer(BaseHTTPServer.HTTPServer):
    allow_reuse_address = True
    
    def setup(self, node_list, otafile):
        self.node_list = node_list
        self.otafile = otafile

def start(node_list, otafile):
    server = OTAServer(('', PORT), OTAHandler)
    server.setup(node_list, otafile)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
