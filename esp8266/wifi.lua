local callback=(...)

function run_setup()
    wifi.setmode(wifi.SOFTAP)
    cfg={}
    -- Set your own AP prefix. SHM = Smart Home Module.
    cfg.ssid="SHM"..node.chipid()
    wifi.ap.config(cfg)

    print("Opening WiFi credentials portal")
    dofile ("dns-liar.lc")
    dofile ("server.lc")
end

function read_wifi_credentials()
    wifi_dns = nil
    wifi_ssid = nil
    wifi_password = nil
    wifi_ip = nil
    wifi_nm = nil
    wifi_gw = nil
    wifi_desc = nil

    if file.open("netconfig.lc", "r") then
        file.close()
        dofile('netconfig.lc')
    end

    -- set DNS to second slot if configured.
    if wifi_dns ~= nil and wifi_dns ~= '' then net.dns.setdnsserver(wifi_dns, 1) end
	
    return wifi_ssid, wifi_password, wifi_ip, wifi_nm, wifi_gw, wifi_desc
end

function try_connecting(wifi_ssid, wifi_password, wifi_ip, wifi_nm, wifi_gw)
    wifi.setmode(wifi.STATION)
    wifi.sta.config(wifi_ssid, wifi_password)
    wifi.sta.connect()
    wifi.sta.autoconnect(1)

    -- Set IP if no DHCP required
    if wifi_ip ~= "" then wifi.sta.setip({ip=wifi_ip, netmask=wifi_nm, gateway=wifi_gw}) end

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
            print("Failed to connect to \"" .. wifi_ssid .. "\"")
            run_setup()
            tmr.unregister(0)
        end
    end)
end


-------------------------
------  MAIN  -----------
-------------------------
wifi.sta.disconnect()
wifi_ssid, wifi_password, wifi_ip, wifi_nm, wifi_gw, wifi_desc = read_wifi_credentials()
if wifi_ssid ~= nil and wifi_password ~= nil then
    print("Retrieved stored WiFi credentials")
    print("---------------------------------")
    print("wifi_ssid     : " .. wifi_ssid)
    print("wifi_password : " .. wifi_password)
    print("wifi_ip : " .. wifi_ip)
    print("wifi_nm : " .. wifi_nm)
    print("wifi_gw : " .. wifi_gw)
    print("wifi_dns : " .. (wifi_dns or ""))
    print("wifi_repo : " .. (wifi_repo or ""))
    print("wifi_desc : " .. (wifi_desc or ""))
    _G.wifi_desc = (wifi_desc or "")
    try_connecting(wifi_ssid, wifi_password, wifi_ip, wifi_nm, wifi_gw)
else
    run_setup()
end
