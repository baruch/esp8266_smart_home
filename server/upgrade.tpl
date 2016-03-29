%rebase('base.tpl', title='Upgrade for ' + node_id)

<div class="row">
% if node is None:
        Unknown node {{node_id}}
% else:
        Upgrading: {{success}}
</div>
