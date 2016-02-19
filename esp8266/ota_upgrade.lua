--local callback=(...)

print('ota create connection')
local conn = net.createConnection(net.TCP, 0)

local upgradeThread = coroutine.create(function (conn, server_ip)
    print('connecting to ', server_ip)
    conn:connect(24320, server_ip)
    coroutine.yield()

    conn:send('list\n')

    -- Now parse the list response until an empty line
    local files = {}
    local buf = ''
    while buf ~= nil do
        data = coroutine.yield()
        buf = buf..data

        local start = 0
        local newline = 0
        while true do
            newline = string.find(buf, "\n", newline + 1)
            if newline == nil then
                -- no new line, wait for more data
                buf = string.sub(buf, start)
                break
            else
                -- chop another line
                line = string.sub(buf, start, newline)
                if line == "\n" then
                    -- empty line, we're done
                    buf = nil
                    break
                end
                print("line '"..line.."'")
                _, _, filename, fl1, fl2 = string.find(line,  "(%w+.%w+),(%d+),(%d+)\n")
                if filename == nil or fl1 == nil or fl2 == nil then
                    print("Parsing of line failed")
                else
                    print("Parsed filename: "..filename)
                end
                start = newline+1
            end
        end

        collectgarbage()
    end

    conn:close()
    --callback()
end)

conn:on('connection', function(conn)
    print('connected to ota server')
    coroutine.resume(upgradeThread)
    collectgarbage()
end)

conn:on('receive', function(conn, data)
    print('Received data')
    coroutine.resume(upgradeThread, data)
    collectgarbage()
end)

coroutine.resume(upgradeThread, conn, _G.server_ip)
