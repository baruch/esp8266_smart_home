%rebase('base.tpl', title='ESP8266 Smart Home Console')
<div class="row">
<table>
<tr><th>ID</th><th>Description</th><th>Version</th><th>IP</th><th> </th></tr>
%for node in nodes:
        <tr>
        <td>{{node['node_id']}}</td>
        <td>{{node['node_desc']}}</td>
        <td>{{node['version']}}</td>
        <td>{{node['ip']}}</td>
        <td><a href="/console/{{node['node_id']}}">Console</a> | <a href="/upgrade/{{node['node_id']}}">Upgrade</a></td>
        </tr>
%end
</table>
</div>
