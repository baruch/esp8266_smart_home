import threading
from bottle import route, run, template, HTTPResponse, request, get, response

HOST = '0.0.0.0'
PORT = 24320

node_list = None
otafile = None

@get('/node_v1.bin')
def node_v1_download():
    node = node_list.get_node_by_ip(request.remote_addr)
    if node is None:
        return HTTPResponse(status='403 Unknown Node')
    
    _version = request.get_header('x-ESP8266-version')
    if _version is None:
        return HTTPResponse(status='403 Unknown version')

    _md5, content = otafile.check_update(_version)
    if content is None:
        return HTTPResponse(status='304 Not Modified')

    response.set_header('x-MD5', _md5)
    return content

@get('/')
def index():
    return 'Hello World!'

def start(_node_list, _otafile):
    global node_list, otafile
    node_list = _node_list
    otafile = _otafile

    server_thread = threading.Thread(target=run, kwargs=dict(host=HOST, port=PORT))
    server_thread.daemon = True
    server_thread.start()
