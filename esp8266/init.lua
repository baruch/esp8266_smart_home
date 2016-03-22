function deserialize_file(name)
    local nc = loadfile(name)
    if nc == nil then
        print(name .. ": Config file is missing, using defaults")
        return {}
    else
        local success, result = pcall(nc)
        if success ~= true then
            print('failed to load', exception)
        end
        return result
    end
end

function dofile_callback(name, callback)
    print('Loading file', name)
    local func = loadfile(name)
    if func ~= nil then
        local success, result = pcall(func, callback)
        if success ~= true then
            print('Failed to run', exception)
        end
    else
        print('loadfile for '..name..' failed')
    end
    func = nil
    collectgarbage()
end

function check_upgrade()
    if upgradeThread ~= nil then
        print('Upgrade already in progress')
        return
    end

    dofile_callback('ota_upgrade.lua', function (reboot_needed)
        print('firmware upgrade finished, reboot needed:', reboot_needed)
        if reboot_needed == true then
            -- In the future it might be worth to coordinate the restart with other actions but for now this is good enough
            print('Rebooting')
            node.restart()
        end
    end)
end

function wifi_connected()
    dofile_callback('discovery.lua', function (server_ip)
        _G.server_ip = server_ip
        print('server discovery completed, server: ', server_ip)

        check_upgrade()
        dofile_callback('mqtt_client.lua', nil)
    end)
end

_G.server_ip = nil
_G.wifi_desc = ''
dofile('bootreason.lua')
if former_run_failed() then
    print('Not running rest of code due to error in previous run')
else
    deserialize_file('node_params.lua')
    print("Node type", _G.node_type)
    dofile_callback('wifi.lua', wifi_connected)
    dofile_callback("button_setup.lua", nil)  -- uses timer #5
end
