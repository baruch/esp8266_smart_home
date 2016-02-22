--local callback=(...)

dofile('serialize.lc')

print('ota create connection')
local conn = net.createConnection(net.TCP, 0)

local function fletcher(filename)
    if file.open(filename, "r") == nil then
        return nil, nil
    end

    local sum1 = 0
    local sum2 = 0

    while true do
        local data = file.read(1024)
        if data == nil then
            -- EOF
            break
        end

        local i
        for i = 1, #data do
            local c = string.byte(data, i)
            sum1 = (sum1 + c) % 255
            sum2 = (sum2 + sum1) % 255
        end
    end
    file.close()

    return sum1, sum2
end

local function download_file_data(filename)
    conn:on('receive', function(conn, data)
        print('Received data')
        coroutine.resume(upgradeThread, data)
    end)

    print('send request')
    conn:send("file "..filename.."\n")
    print('wait for data')
    local data = coroutine.yield()
    print('data received')
    local newline = string.find(data, '\n')
    print(newline)
    local line = string.sub(data, 1, newline-1)
    print(line)
    data = string.sub(data, newline+1)
    local total_len = tonumber(line)
    print(total_len)
    print("Receiving data "..total_len.." bytes")

    local rcvd_len = 0
    while rcvd_len < total_len do
        local data_len = string.len(data)
        if data_len == 0 then
            data = coroutine.yield()
        else
            file.write(data)
            rcvd_len = rcvd_len + data_len
            data = ""
        end
        collectgarbage()
    end
end

local function download_file(filename, fl1, fl2)
    print('Upgrading file', filename)
    print('removing file')
    file.remove('download.tmp')
    print('create file')
    file.open('download.tmp', 'w')

    print('downloading')
    download_file_data(filename)

    print('download done')
    file.close('download.tmp')
    local sum1 = nil
    local sum2 = nil
    sum1, sum2 = fletcher('download.tmp')
    if sum1 ~= fl1 or sum2 ~= fl2 then
        print('Failed to download file', sum1, sum2, fl1, fl2)
        file.remove('download.tmp')
        return false
    else
        print('renaming')
        file.rename('download.tmp', filename)
        if string.find(filename, '.lua') ~= nil and filename ~= "init.lua" then
            print('Compiling')
            node.compile(filename)
            print('Removing source file')
            file.remove(filename)
        end
    end
    print('Done download')
    return true
end

local function handle_line(line, filelist, todolist)
    local filename = nil
    local fl1 = nil
    local fl2 = nil
    _, _, filename, fl1, fl2 = string.find(line,  "([^,]+),(%d+),(%d+)\n")
    if filename == nil or fl1 == nil or fl2 == nil then
        print("Parsing of line failed")
    else
        print("Parsed filename: "..filename)
        fl1 = tonumber(fl1)
        fl2 = tonumber(fl2)

        local vals = filelist[filename]
        local sum1 = nil
        local sum2 = nil
        if vals == nil then
            sum1, sum2 = nil, nil
        else
            sum1 = vals[1]
            sum2 = vals[2]
        end

        if sum1 ~= fl1 or sum2 ~= fl2 then
            print("File is different, will download", fl1, fl2, sum1, sum2)
            todolist[filename] = {fl1, fl2}
        else
            print("File is equal, skipping")
        end
    end
end

local function list_files(filelist)
    conn:send('list\n')
    local todolist = {}

    -- Now parse the list response until an empty line
    local buf = ''
    while buf ~= nil do
        local data = coroutine.yield()
        buf = buf..data

        local start = 0
        local newline = 0
        while true do
            collectgarbage()
            newline = string.find(buf, "\n", newline + 1)
            if newline == nil then
                -- no new line, wait for more data
                buf = string.sub(buf, start)
                break
            else
                -- chop another line
                local line = string.sub(buf, start, newline)
                if line == "\n" then
                    -- empty line, we're done
                    buf = nil
                    line = nil
                    break
                end
                start = newline+1

                handle_line(line, filelist, todolist)
                line = nil
            end
        end

        collectgarbage()
    end

    print("Done checking for updates")
    return todolist
end

upgradeThread = coroutine.create(function (_, server_ip)
    print('connecting to ', server_ip)
    conn:connect(24320, server_ip)
    coroutine.yield()

    local filelist = deserialize_file("file_list.lc")
    if display_table ~= nil then
        print('Installed files')
        display_table(filelist)
        print('List end')
    end
    local todolist = list_files(filelist)

    local filename
    local fl
    for filename, fl in pairs(todolist) do
        if download_file(filename, fl[1], fl[2]) then
            filelist[filename] = fl
        end
        collectgarbage()
    end

    serialize_file("file_list.lua", filelist)

    conn:close()
    conn = nil
    upgradeThread = nil
    collectgarbage()
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
