%rebase('base.tpl', title='Console for ' + node_id)

<div class="row">
Node: {{node_id}}<br/>
% if node:
    IP: {{node['ip']}}<br/>
% else:
    Unknown node<br/>
% end
</div>

<div class="row">
<form onsubmit="return false;">
    <h3>Serial Monitor</h3>
    <input type="button" value="Connect" onclick="connect(host.value)" />
    <br>
	<label>Connection Status:</label>
	<input type="text" id="connectionLabel" readonly="true" value="Idle"/>
    <input type="hidden" id="host" value="{{node_ip}}"/>
	<br />

    <br>
    <textarea id="output" style="width:100%;height:500px;"></textarea>
    <br>
    <input type="button" value="Clear" onclick="clearText()">
</form>

<script type="text/javascript">
    const PING_INTERVAL_MILLIS = 5000;

    var output = document.getElementById('output');
    var connectionLabel = document.getElementById('connectionLabel');
    var socket;
    var isRunning = false;

    function connect(host) {
        console.log('connect', host);
        if (window.WebSocket) {
            connectionLabel.value = "Connecting";
            if(socket) {
                socket.close();
                socket = null;
            }

            socket = new WebSocket(host);

            socket.onmessage = function(event) {
                output.value += event.data; // + "\r\n";
                var textarea = document.getElementById('output');
                textarea.scrollTop = textarea.scrollHeight;
            };
            socket.onopen = function(event) {
                isRunning = true;
                connectionLabel.value = "Connected";
            };
            socket.onclose = function(event) {
                isRunning = false;
                connectionLabel.value = "Disconnected";
                //	            socket.removeAllListeners();
                //	            socket = null;
            };
            socket.onerror = function(event) {
                connectionLabel.value = "Error";
                //	            socket.removeAllListeners();
                //	            socket = null;
            };
        } else {
            alert("Your browser does not support Web Socket.");
        }
    }

/*
window.setInterval(function() {
  send("ping");
}, PING_INTERVAL_MILLIS);
*/

function send(data) {
    if (!window.WebSocket) {
        return;
    }

    if (socket.readyState == WebSocket.OPEN) {
        var message = data;
        output.value += 'sending : ' + data + '\r\n';
        socket.send(message);
    } else {
        alert("The socket is not open.");
    }
}

function clearText() {
    output.value="";
}

% if node:
    function autoconnect() {
        if (!isRunning) {
            connect('ws://{{node_ip}}:81/');
        }
    }
    window.setInterval(function() { autoconnect(); }, 1000);
    autoconnect();
% end
</script>
</div>
