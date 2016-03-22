mqtt_prefix = "/shm/"..node.chipid()

mq = mqtt.Client("SHM"..node.chipid(), 120, "", "")

local topic_status = "/shm/upgrade_now"
local sensors = {}

-- If we die, report we are offline
mq:lwt(topic_status, "offline", 0, 1)

-- Take note if our mqtt client is connected or not
mq_connected = false

local function _mqtt_sensor(name, value)
	mq:publish(mqtt_prefix.."/sensor/"..name, value, 1, 1)
end

function mqtt_sensor(name, value)
	if mq_connected == true then
		_mqtt_sensor(name, value)
		return true
	else
		sensors[name] = value
		return false
	end
end

local function report_sensors()
	local name, value
	for name, value in pairs(sensors) do
		_mqtt_sensor(name, value)
	end
	sensors = {}
end

mq:on('connect', function(client)
        print('mqtt connected')
        mq_connected = true
        -- Report we are online after connection
        mq:publish(topic_status, "online", 0, 1)
        -- Get notifications of upgrades
        mq:subscribe("/shm/upgrade_now", 0)

		report_sensors()
		collectgarbage()
end)

mq:on('offline', function(client)
        print('mqtt offline')
        mq_connected = false
		collectgarbage()
end)

mq:on('message', function(conn, topic, data)
        if topic == topic_status then
                print("Check upgrade")
                check_upgrade()
        end
		collectgarbage()
end)

mq:connect(_G.server_ip, 1883, 0, 1)
