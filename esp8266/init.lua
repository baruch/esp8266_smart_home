function deserialize_file(name)
    local nc = loadfile(name)
    if nc == nil then
        print(name .. ": Config file is missing, using defaults")
        return {}
    else
        return nc()
    end
end

function dofile_callback(name, callback)
    print('Loading file', name)
    local func = loadfile(name)
    if func ~= nil then
        func(callback)
    else
        print('loadfile for '..name..' failed')
    end
    func = nil
    collectgarbage()
end

function wifi_connected()
    dofile_callback('discovery.lc', function (server_ip)
        _G.server_ip = server_ip
        print('server discovery completed, server: ', server_ip)
        dofile_callback('ota_upgrade.lc', function (reboot_needed)
            print('firmware upgrade finished, reboot needed:', reboot_needed)
            if reboot_needed == true then
                -- In the future it might be worth to coordinate the restart with other actions but for now this is good enough
                print('Rebooting')
                node.restart()
            end
        end)
    end)
end

_G.server_ip = nil
_G.wifi_desc = ''
deserialize_file('node_params.lc')
print("Node type", _G.node_type)
dofile_callback('wifi.lc', wifi_connected)
