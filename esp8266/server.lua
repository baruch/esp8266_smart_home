----
--Modified by Andy Reischle, igrowing
--
--Based on 
--XChip's NodeMCU IDE
----

--  Run timer for timeout in case no one was connected
--  This is mostly for when we are configured but failed to connect, this will retry the connection to the AP once in a while
--  but still allow us to get reconfigured if the entire network collapsed.
function run_timeout()
    tmr.alarm(0, 15 * 60 * 1000, 0, function()
        node.restart()
    end)
end
run_timeout()

--  Run the server
srv=net.createServer(net.TCP) 
srv:listen(80, function(conn) 

   local rnrn=0
   local Status = 0
   local responseBytes = 0
   local method=""
   local url=""
   local vars=""

   conn:on("receive",function(conn, payload)
  
    if Status==0 then
        _, _, method, url, vars = string.find(payload, "([A-Z]+) /([^?]*)%??(.*) HTTP")
        if method == nil then
            print("Received garbage via HTTP.")
            return
        end
        -- Get the vars from POST: data is not passed in address.
        if string.lower(method) == "post" then
            _, _, vars = string.find(payload, "(wifi_ssid.*)")
        end
        -- print(method, url, vars)                          
    end
    
--    print("Heap   : " .. node.heap())
--    print("Payload: " .. payload)
--    print("Method : " .. method)
--    print("URL    : " .. url)
--    if vars then print("Vars   : " .. vars  .. "\n\n\n") end

    -- Check if wifi-credentials have been supplied
    if vars~=nil and parse_wifi_credentials(vars) then
        print('Rebooting shortly')
        -- wait to complete file flushing
        tmr.alarm(2, 1000, 0,
            function() 
                node.restart()
            end
        )
    end

    if url == "favicon.ico" then
        conn:send("HTTP/1.1 404 file not found")
        responseBytes = -1
        collectgarbage()
        return
    end    

    -- Only support one sending one file
    url="index.html"
    responseBytes = 0
    
    conn:send("HTTP/1.1 200 OK\r\n\r\n")
    collectgarbage()
  end)
  
  conn:on("sent",function(conn) 
    -- Restart the timout timer, let user to enter the data.
    run_timeout()
    -- Pad the unit identifier with spaces to fixed length.
    holder = "generic_and_very_long_place_holder_keep_even_more"
    ident = (uclass or "not_set").."-"..(utype or "not_set").."-"..node.chipid()..string.rep(" ", 50)
    ident=(string.sub(ident, 1, string.len(holder)))

    if responseBytes>=0 and method=="GET" then
        if file.open(url, "r") then            
            file.seek("set", responseBytes)
            local line=file.read(512)
            file.close()
            if line then
                -- Update the data to send with unit identifier
                line=(string.gsub(line, holder, ident))
                conn:send(line)
                responseBytes = responseBytes + 512    

                if (string.len(line)==512) then
                    return
                end
            end
        end        
    end
    ident, holder = nil, nil -- clean memory
    
    conn:close() 
  end)
end)
print("HTTP Server: Started")


function valid_ip(instr, is_mask)
   local min, max = 1, 254
   if is_mask then
       min, max = 0, 255
   end
   if instr == nil or instr == "" then
       return 0
   end
   _, _, ip1s, ip2s, ip3s, ip4s = string.find(instr, "(%d+)%.(%d+)%.(%d+)%.(%d+)")
   if ip1s/1 >= min and ip1s/1 <= max and ip2s/1 >= min and ip2s/1 <= max and 
      ip3s/1 >= min and ip3s/1 <= max and ip4s/1 >= min and ip4s/1 <= max then
      return 1
   end
   return 0
end

function urldecode(instr)
  -- Transform encoded characters into readable symbols
  instr = instr:gsub('+', ' ') -- replace + with space
       :gsub('%%(%x%x)', function(h) -- replace non-alphabets with symbols
                           return string.char(tonumber(h, 16))
                         end)
  return instr
end

function extract_arg(vars, name)
    -- Don't differ between text and IP at this stage. IP validation comes later.
    local _, _, value = string.find(vars, name .. "\=([^&]+)")
    if value == nil then
        return ""
    end

    return urldecode(value)  -- REVIEW: urldecode does nothing...
end

function parse_wifi_credentials(vars)
    if vars == nil or vars == "" then
        return false
    end

    local wifi_ssid = extract_arg(vars, "wifi_ssid")
    local wifi_password = extract_arg(vars, "wifi_password")
    local wifi_ip = extract_arg(vars, "wifi_ip")
    local wifi_nm = extract_arg(vars, "wifi_nm")
    local wifi_gw = extract_arg(vars, "wifi_gw")
    local wifi_dns = extract_arg(vars, "wifi_dns")
    -- REVIEW: keep wifi_repo meanwhile as a field. May be used for other purpose later.
    local wifi_repo = extract_arg(vars, "wifi_repo")
    local wifi_desc = extract_arg(vars, "wifi_desc")

    if wifi_ssid == "" or wifi_password == "" then
        return false
    end

    --print(wifi_ssid, wifi_password, wifi_ip, wifi_nm, wifi_gw, wifi_dns, wifi_repo, wifi_desc)
    local pwd_len = string.len(wifi_password)
    if pwd_len < 8 or pwd_len > 64 then
        -- TODO: Print the message to browser. User should see this.
        print("Error: Password length should be between 8 and 64 characters")
        return false
    end

    local valid_ips = valid_ip(wifi_ip, false) + valid_ip(wifi_nm, true) + valid_ip(wifi_gw, false)
    if valid_ips == 0 then
        -- Go DHCP if no valid network details provided
    elseif valid_ips ~= 3 then
        -- REVIEW: no need to check DNS here: it's optional. --or valid_ip(wifi_dns, false) == 0
        -- TODO: Print the message to browser. User should see this.
        print("Error: Request all network details again if one or more of them are not valid")
        return false
    elseif wifi_dns ~= "" and valid_ip(wifi_dns, false) == 0 then
        -- TODO: Print the message to browser. User should see this.
        print("Error: DNS should be an IP address or empty.")
        return false
    end
    
   
    print("New WiFi credentials received")
    print("-----------------------------")
    print("wifi_ssid     : ", wifi_ssid)
    print("wifi_password : ", wifi_password)
    print("wifi_ip : ", wifi_ip)
    print("wifi_nm : ", wifi_nm)
    print("wifi_gw : ", wifi_gw)
    print("wifi_dns : ", wifi_dns)
    print("wifi_repo : " .. wifi_repo)
    print("wifi_desc : ", wifi_desc)

    dofile("serialize.lc")
    serialize_file("netconfig.lua", {
        wifi_ssid = wifi_ssid,
        wifi_password = wifi_password,
        wifi_ip = wifi_ip,
        wifi_nm = wifi_nm,
        wifi_gw = wifi_gw,
        wifi_dns = wifi_dns,
        wifi_desc = wifi_desc,
    })
    -- Compiled files harder to read (weak theft protection).
    collectgarbage()
    return true
end

