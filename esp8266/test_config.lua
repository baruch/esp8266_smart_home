dofile('serialize.lua')

function net_config(ssid, pw)
    serialize_file("netconfig.lua", {
        wifi_ssid = ssid,
        wifi_password = pw,
        wifi_ip = '',
        wifi_nm = '',
        wifi_gw = '',
        wifi_dns = '',
        wifi_desc = 'test config',
    })
end
