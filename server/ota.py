import threading
from bottle import route, run, template, HTTPResponse, request, get, response

HOST = '0.0.0.0'
PORT = 24320

node_list = None
otafile = None
l = file('bottle.log', 'w')

def log(msg):
    l.write(msg.__str__())
    l.write('\n')
    l.flush()

@get('/node_v1.bin')
def node_v1_download():
    node = node_list.get_node_by_ip(request.remote_addr)
    log(request.remote_addr)
    log(node)
    if node is None:
        log('403')
        return HTTPResponse(status='403 Unknown Node')
    
    _version = request.get_header('x-ESP8266-version')
    log(_version)
    if _version is None:
        log('version bad')
        return HTTPResponse(status='403 Unknown version')

    _md5, content = otafile.check_update(_version)
    log(_md5)
    if content is None:
        log('no content')
        return HTTPResponse(status='304 Not Modified')

    log('normal')
    log('content len %d' % len(content))
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
