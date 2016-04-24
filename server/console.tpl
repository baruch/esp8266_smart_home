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
Console is inoperational for now, we need to show the log lines from we received instead of the websockets we used to use.
</div>
