function dofile_callback(name, callback)
    local func = loadfile(name)
    if func ~= nil then
        func(callback)
    else
        print('loadfile for '..name..' failed')
    end
    collectgarbage()
end

function wifi_connected()
    dofile_callback('discovery.lc', function (server_ip)
        _G.server_ip = server_ip
        print('server discovery completed, server: ', server_ip)
--        dofile_callback('ota_upgrade.lc', function (reboot_needed)
--            print('firmware upgrade finished, reboot needed:', reboot_needed)
--        end)
    end)
end

_G.server_ip = nil
_G.wifi_type = ''
_G.wifi_desc = ''
dofile_callback('node_params.lc', nil)
dofile_callback('wifi.lc', wifi_connected)
