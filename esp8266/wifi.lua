local callback=...

local function run_setup()
    wifi.setmode(wifi.SOFTAP)
    local cfg={}
    -- Set your own AP prefix. SHM = Smart Home Module.
    cfg.ssid="SHM"..node.chipid()
    wifi.ap.config(cfg)

    print("Opening WiFi credentials portal")
    dofile ("dns-liar.lc")
    dofile ("server.lc")
end

local function non_nil(val)
    if val == nil then
        return ""
    end
end

local function read_wifi_credentials()
    local c = deserialize_file("netconfig.lc")
    c.wifi_desc = non_nil(c.wifi_desc)
    c.wifi_ip = non_nil(c.wifi_ip)
    c.wifi_ssid = non_nil(c.wifi_ssid)
    c.wifi_password = non_nil(c.wifi_password)
    c.wifi_nm = non_nil(c.wifi_nm)
    c.wifi_gw = non_nil(c.wifi_gw)
    c.wifi_dns = non_nil(c.wifi_dns)

    return c
end

local function try_connecting(conf)
    print('Config loaded, trying to connect')
    wifi.setmode(wifi.STATION)
    wifi.sta.config(conf.wifi_ssid, conf.wifi_password)
    wifi.sta.connect()
    wifi.sta.autoconnect(1)

    -- Set IP if no DHCP required
    if conf.wifi_ip ~= "" then
        wifi.sta.setip({ip=conf.wifi_ip, netmask=conf.wifi_nm, gateway=conf.wifi_gw})
    end

    tmr.alarm(0, 2000, 1, function()
        if wifi.sta.status() ~= 5 then
          print("Connecting to AP...")
        else
          tmr.stop(1)
          tmr.stop(0)
          print("Connected as: " .. wifi.sta.getip())
          tmr.unregister(0)
          tmr.unregister(1)
          collectgarbage()
          callback()
        end
    end)

    tmr.alarm(1, 20000, 0, function()
        -- Sleep if sensor (save power until WiFi gets back), 
        -- else run configuration mode
        if wifi.sta.status() ~= 5 then
            tmr.stop(0)
            tmr.unregister(0)
            print("Failed to connect to", wifi_ssid, ".")
            if uclass == nil or uclass == "sensor" then 
                print("Sleep 5 min + retry...")
                print("Press the button 5 seconds on the next boot to enter WiFi configuration captive mode.")
                -- No sense to run setup if the settings present. Sleep and retry.
                node.dsleep(5 * 60 * 1000 * 1000, 0)   -- 5 min sleep            
            else
                run_setup()
            end
        end
    end)
end


-------------------------
------  MAIN  -----------
-------------------------
print('start')
-- REVIEW: how to merge unit type and node params. Brainstorm together.
if file.open("ut.lc", "r") then dofile("ut.lc")
else print "Unit type is not set." end

dofile("button_setup.lc")  -- uses timer #5
wifi.sta.disconnect()
print('disconnected')
local conf = read_wifi_credentials()
print('config read')
-- TODO: Add your functionality here to do BEFORE connection established.
--

if conf.wifi_ssid ~= nil and conf.wifi_password ~= nil then
    print("Retrieved stored WiFi credentials")
    print("---------------------------------")
    print("wifi_ssid     : ", conf.wifi_ssid)
    print("wifi_password : ", conf.wifi_password)
    print("wifi_ip : ", conf.wifi_ip)
    print("wifi_nm : ", conf.wifi_nm)
    print("wifi_gw : ", conf.wifi_gw)
    print("wifi_dns : ", conf.wifi_dns)
    print("wifi_desc : ", conf.wifi_desc)
    _G.wifi_desc = conf.wifi_desc

    -- set DNS to second slot if configured.
    if conf.wifi_dns ~= nil and conf.wifi_dns ~= '' then
        net.dns.setdnsserver(wifi_dns, 1)
    end

    try_connecting(conf)
    conf = nil
    collectgarbage()
else
    run_setup()
end
