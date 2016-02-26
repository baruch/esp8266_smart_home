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

function check_upgrade()
    dofile_callback('ota_upgrade.lc', function (reboot_needed)
        print('firmware upgrade finished, reboot needed:', reboot_needed)
        if reboot_needed == true then
            -- In the future it might be worth to coordinate the restart with other actions but for now this is good enough
            print('Rebooting')
            node.restart()
        end
    end)
end

function topic_sensor(name)
    return mqtt_prefix.."/sensor/"..name
end

function topic_status(name)
    return mqtt_prefix.."/status"
end

function do_sensor()
    if mq_online then
        mq:publish(topic_sensor("vdd"), adc.readvdd33(), 0, 1)
    end
    collectgarbage()
end

function wifi_connected()
    dofile_callback('discovery.lc', function (server_ip)
        _G.server_ip = server_ip
        print('server discovery completed, server: ', server_ip)

        check_upgrade()

        mqtt_prefix = "/shm/"..node.chipid()
        mq = mqtt.Client("SHM"..node.chipid(), 120, "", "")
        -- If we die, report we are offline
        mq:lwt(topic_status(), "offline", 0, 1)
        mq_online = false
        mq:on('connect', function(client)
            print('mqtt connected')
            mq_online = true
            -- Report we are online after connection
            mq:publish(topic_status(), "online", 0, 1)
            -- Get notifications of upgrades
            mq:subscribe("/shm/upgrade_now", 0)
            do_sensor()
        end)
        mq:on('offline', function(client)
            print('mqtt offline')
            mq_online = false
        end)
        mq:on('message', function(conn, topic, data)
            if topic == "/shm/upgrade_now" then
                print("Check upgrade")
                check_upgrade()
            end
        end)
        mq:connect(_G.server_ip, 1883, 0, 1)
        tmr.alarm(1, 30*1000, tmr.ALARM_AUTO, do_sensor)
    end)
end

_G.server_ip = nil
_G.wifi_desc = ''
deserialize_file('node_params.lc')
print("Node type", _G.node_type)

dofile_callback('wifi.lc', wifi_connected)
