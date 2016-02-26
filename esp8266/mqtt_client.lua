mqtt_prefix = "/shm/"..node.chipid()

mq = mqtt.Client("SHM"..node.chipid(), 120, "", "")

-- If we die, report we are offline
mq:lwt(topic_status(), "offline", 0, 1)

-- Take note if our mqtt client is connected or not
mq_connected = false

mq:on('connect', function(client)
        print('mqtt connected')
        mq_connected = true
        -- Report we are online after connection
        mq:publish(topic_status(), "online", 0, 1)
        -- Get notifications of upgrades
        mq:subscribe("/shm/upgrade_now", 0)
        do_sensor()
end)

mq:on('offline', function(client)
        print('mqtt offline')
        mq_connected = false
end)

mq:on('message', function(conn, topic, data)
        if topic == "/shm/upgrade_now" then
                print("Check upgrade")
                check_upgrade()
        end
end)

mq:connect(_G.server_ip, 1883, 0, 1)
