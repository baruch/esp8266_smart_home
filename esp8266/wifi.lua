local callback=...

function run_setup()
    wifi.setmode(wifi.SOFTAP)
    local cfg={}
    -- Set your own AP prefix. SHM = Smart Home Module.
    cfg.ssid="SHM"..node.chipid()
    wifi.ap.config(cfg)

    print("Opening WiFi credentials portal")
    dofile ("dns-liar.lc")
    dofile ("server.lc")
end

function read_wifi_credentials()
    local nc = loadfile("netconfig.lc")
    if nc == nil then
        return {}
    else
        return nc()
    end
end

function try_connecting(conf)
    print('Config loaded, trying to connect')
    wifi.setmode(wifi.STATION)
    wifi.sta.config(conf.wifi_ssid, conf.wifi_password)
    wifi.sta.connect()
    wifi.sta.autoconnect(1)

    -- Set IP if no DHCP required
    if conf.wifi_ip ~= "" then
        wifi.sta.setip({ip=conf.wifi_ip, netmask=conf.wifi_nm, gateway=conf.wifi_gw})
    end

    tmr.alarm(0, 1000, 1, function()
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

    tmr.alarm(1, 10000, 0, function()
        if wifi.sta.status() ~= 5 then
            tmr.stop(0)
            print("Failed to connect to \"" .. conf.wifi_ssid .. "\"")
            run_setup()
            tmr.unregister(0)
        end
    end)
end


-------------------------
------  MAIN  -----------
-------------------------
print('start')
wifi.sta.disconnect()
print('disconnected')
local conf = read_wifi_credentials()
print('config read')
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
    if conf.wifi_dns ~= nil and conf.wifi_dns ~= '' then
        net.dns.setdnsserver(wifi_dns, 1)
    end
    try_connecting(conf)
    conf = nil
    collectgarbage()
else
    run_setup()
end
