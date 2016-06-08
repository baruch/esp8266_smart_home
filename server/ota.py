import threading
from bottle import route, run, template, HTTPResponse, request, get, response, static_file

paste_available = False
try:
    import paste
    paste_available = True
except ImportError:
    print 'Paste is not available, server will be single threaded'

HOST = '0.0.0.0'
PORT = 24320

node_list = None
otafile = None

node_types = {
        1: 'Temperature/Humidity',
        2: 'Relay with Button',
        3: 'Soil Moisture',
        5: 'Sewage Pump',
}

def node_type_str(node_type):
    s = node_types.get(node_type, None)
    if s is None:
        return 'Unknown (%d)' % node_type
    else:
        return s

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

@get('/static/<path:path>')
def static(path):
    return static_file(path, root='./static/')

@get('/console/<node_id>')
def console(node_id):
    node = node_list.get_node_by_id(node_id)
    node_ip = '-'
    if node:
        node_ip = node['ip']
    return template('console.tpl', node_id=node_id, node=node, node_ip=node_ip)

@get('/upgrade/<node_id>')
def upgrade(node_id):
    node = node_list.get_node_by_id(node_id)
    ret = node_list.upgrade(node_id, otafile)
    return template('upgrade.tpl', node_id=node_id, node=node, success=ret)

@get('/')
def index():
    return template('index.tpl', nodes=node_list.get_node_list(), node_type_str=node_type_str)

def start(_node_list, _otafile):
    global node_list, otafile
    node_list = _node_list
    otafile = _otafile

    args = dict(host=HOST, port=PORT)
    if paste_available:
        args['server'] = 'paste'
    server_thread = threading.Thread(target=run, kwargs=args)
    server_thread.daemon = True
    server_thread.start()
