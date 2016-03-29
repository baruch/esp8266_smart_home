%rebase('base.tpl', title='Console for ' + node['node_id'])

<div class="row">
Node: {{node_id]}}<br/>
IP: {{node['ip']}}<br/>
</div>
