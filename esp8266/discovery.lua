local callback=(...)

print('create connection')
conn = net.createConnection(net.UDP, 0)

print ('set callback')
conn:on("receive", function(conn, data)
    print("received", data)
    ip = nil
    if string.len(data) >= 6 and string.byte(data, 1) == 82 and string.byte(data,2) == 4 then
        ip = string.format('%d.%d.%d.%d', string.byte(data,3), string.byte(data, 4), string.byte(data, 5), string.byte(data, 6))
    end
    conn:close()
    callback(ip)
    collectgarbage()
end)

print('connecting to udp')
conn:connect(24320, '255.255.255.255')

function string_encode(s)
    if s == nil then
        s = ''
    end
    l = string.len(s)
    e = string.char(l) .. s
    return e
end

local buf = 'S' .. string_encode(node.chipid()) .. string_encode(_G.wifi_type) .. string_encode(_G.wifi_desc)
print('send packet: ' .. buf)
conn:send(buf)
buf = nil
collectgarbage()
