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
    dofile_callback('discovery.lc', function (serverip)
        print('callback called, server: ', serverip)
    end)
end

dofile_callback('wifi.lc', wifi_connected)
