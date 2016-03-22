dofile('htu21d.lua')

print('Initializing HTU21D')
htu21d:init(5,6)

function htu_poll()
        print(""..htu21d:temp().." "..htu21d:humidity())
        collectgarbage()
end

function htu_stop()
        print("Stopping HTU21D polling")
        tmr.stop(1)
end

function htu_start()
    print('Start HTU21D polling')
    tmr.alarm(1, 30*1000, tmr.ALARM_AUTO, htu_poll)
    htu_poll()
end

function htu_async_read(reg, parser, timer_id, callback)
    local counter = 1
    htu21d:sendcmd(reg)

    tmr.alarm(timer_id, 1, tmr.ALARM_AUTO, function ()
        local reply, ack = htu21d:readreply(3)
        print(counter, reply, ack)
        counter = counter + 1
        if counter == 50 or ack == true then
            tmr.unregister(timer_id)
            print('Done')
            if ack == true then
                callback(ack, parser(reply))
            else
                callback(ack, "")
            end
        end
    end)

end

function htu_async_read_temp(timer_id, callback)
    htu_async_read(htu21d.temp_reg, htu21d.temp, timer_id,callback)
end

function htu_async_read_humid(timer_id, callback)
    htu_async_read(htu21d.humid_reg, htu21d.humid, timer_id, callback)
end

function htu_async()
    local timer_id = 2
    htu_async_read_temp(timer_id, function(ack, data)
        print('ack', ack)
        print('temp', data)
        htu_async_read_humid(timer_id, function(ack, data)
            print('ack', ack)
            print('humid', data)
        end)
    end)
end

function htu_async_single(co, timer_id, reg)
    local ack = htu21d:sendcmd(reg)
    if ack == false then
        print('single err 1')
        return nil
    end

    local counter = 1

    tmr.alarm(timer_id, 1, tmr.ALARM_AUTO, function ()
        local reply, ack = htu21d:readreply(3)
        counter = counter + 1
        if ack == true or counter == 30 then
            tmr.stop(timer_id)
            coroutine.resume(co, reply, ack)
        end
    end)

    local _, data, ack = coroutine.yield()
    print(data, ack)
    if ack == false or #data ~= 3 then
        print('single err 2')
        return nil
    end

    print('single done')
    return data
end

function htu_async_inner(co, timer_id)
    print('Reading temp')
    local data = htu_async_single(co, timer_id, htu21d.temp_reg)
    if data == nil then
        print ('err1')
        return 0, 0, false
    end
    print(data)
    local temp = htu21d:temp(data)

    print ('Reading humid')
    data = htu_async_single(co, timer_id, htu21d.humid_reg)
    if data == nil then
        print('err2')
        return 0, 0, false
    end
    local humid = htu21d:humid(data)
    
    return temp, humid, true
end

function htu_async_coroutine(co, timer_id)
    local temp, humid, succcess = htu_async_inner(co, timer_id)
    print(string.format("HTU21D temp=%.2f humid=%.2f success=%s", temp, humid, tostring(success)))
end

function htu_async_read_co()
    local c = coroutine.create(htu_async_coroutine)
    coroutine.resume(c, c, 3)
end
