%rebase('base.tpl', title='ESP8266 Smart Home Console')
<div class="row">
<table>
<tr><th>ID</th><th>Description</th><th>Version</th><th>IP</th></tr>
%for node in nodes:
        <tr>
        <td>{{node['node_id']}}</td>
        <td>{{node['node_desc']}}</td>
        <td>{{node['version']}}</td>
        <td>{{node['ip']}}</td>
        </tr>
%end
</table>
</div>
