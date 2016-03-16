function i2c_send(address, data)
    local l = 1
    if type(data) == "string" then
        l = #data
    end
    i2c.start(0)
    local ack = i2c.address(0, address, i2c.TRANSMITTER)
    if ack == true then
        ack = i2c.write(0, data) == l
    end
    i2c.stop(0)
    return ack
end

function i2c_recv(address, len)
    i2c.start(0)
    local ack = i2c.address(0, address, i2c.RECEIVER)
    local data = ""
    if ack == true then
        data = i2c.read(0, len)
    end
    i2c.stop(0)
    return data, ack
end
