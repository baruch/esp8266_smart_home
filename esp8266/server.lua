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
	tmr.deregister(0)
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
        tmr.alarm(0,1000,0,function() -- wait to complete file flushing
                    node.restart()
        end)
    end

    if url == "favicon.ico" then
        conn:send("HTTP/1.1 404 file not found")
        responseBytes = -1
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
	
    if responseBytes>=0 and method=="GET" then
        if file.open(url, "r") then            
            file.seek("set", responseBytes)
            local line=file.read(512)
            file.close()
            if line then
                conn:send(line)
                responseBytes = responseBytes + 512    

                if (string.len(line)==512) then
                    return
                end
            end
        end        
    end

    conn:close() 
  end)
end)
print("HTTP Server: Started")


function valid_ip(instr, is_mask)
   local min, max = 1, 254
   if is_mask then
       min, max = 0, 255
   end
   if instr == nil or instr == "" then return false end
   _, _, ip1s, ip2s, ip3s, ip4s = string.find(instr, "(%d+)%.(%d+)%.(%d+)%.(%d+)")
   if ip1s/1 >= min and ip1s/1 <= max and ip2s/1 >= min and ip2s/1 <= max and 
      ip3s/1 >= min and ip3s/1 <= max and ip4s/1 >= min and ip4s/1 <= max then return true
   end
   return false
end

function urldecode(instr)
  -- Transform encoded characters into readable symbols
  instr = instr:gsub('+', ' ') -- replace + with space
       :gsub('%%(%x%x)', function(h) -- replace non-alphabets with symbols
                           return string.char(tonumber(h, 16))
                         end)
  return instr
end

function parse_wifi_credentials(vars)
    if vars == nil or vars == "" then
        return false
    end

    local _, _, wifi_ssid = string.find(vars, "wifi_ssid\=([^&]+)")
    local _, _, wifi_password = string.find(vars, "wifi_password\=([^&]+)")
    local _, _, wifi_ip = string.find(vars, "wifi_ip\=(%d+%.%d+%.%d+%.%d+)")
    local _, _, wifi_nm = string.find(vars, "wifi_nm\=(%d+%.%d+%.%d+%.%d+)")
    local _, _, wifi_gw = string.find(vars, "wifi_gw\=(%d+%.%d+%.%d+%.%d+)")
    local _, _, wifi_dns = string.find(vars, "wifi_dns\=(%d+%.%d+%.%d+%.%d+)")
    local _, _, wifi_repo = string.find(vars, "wifi_repo\=(%d+%.%d+%.%d+%.%d+)")
    local _, _, wifi_desc = string.find(vars, "wifi_desc\=([^&]+)")

    if wifi_ssid == nil or wifi_ssid == "" or wifi_password == nil then
        return false
    end

    pwd_len = string.len(wifi_password)
    if pwd_len ~= 0 and (pwd_len < 8 or pwd_len > 64) then
        print("Password length should be between 8 and 64 characters")
        return false
    end

    -- Go DHCP if no valid network details provided
    print(valid_ip(wifi_ip, false))
    print(valid_ip(wifi_nm, true))
    print(valid_ip(wifi_gw, false))
    print(valid_ip(wifi_dns, false))
    if not valid_ip(wifi_ip, false) and not valid_ip(wifi_nm, true) and not valid_ip(wifi_gw, false) then
        wifi_ip = ""
        wifi_nm = ""
        wifi_gw = ""
		wifi_dns = ""
    -- Request all network details again if one or more of them are not valid
    elseif not valid_ip(wifi_ip, false) or not valid_ip(wifi_nm, true) or not valid_ip(wifi_gw, false) or not valid_ip(wifi_dns, false) then return false
    end
	
	-- Clear the description
    if wifi_desc == nil or wifi_desc == "" then wifi_desc = ""  
	-- Replace pluses with spaces and hexa with symbols
	else wifi_desc = urldecode(wifi_desc)
	end
    
    print("New WiFi credentials received")
    print("-----------------------------")
    print("wifi_ssid     : " .. wifi_ssid)
    print("wifi_password : " .. wifi_password)
    print("wifi_ip : " .. wifi_ip)
    print("wifi_nm : " .. wifi_nm)
    print("wifi_gw : " .. wifi_gw)
    print("wifi_dns : " .. wifi_dns)
    print("wifi_repo : " .. wifi_repo)
    print("wifi_desc : " .. wifi_desc)

    file.remove("netconfig.lc")
    file.open("netconfig.lua", "w+")
    file.writeline("wifi_ssid='"..wifi_ssid.."'")
    file.writeline("wifi_password='"..wifi_password.."'")
    file.writeline("wifi_ip='"..wifi_ip.."'")
    file.writeline("wifi_nm='"..wifi_nm.."'")
    file.writeline("wifi_gw='"..wifi_gw.."'")
    file.writeline("wifi_dns='"..wifi_dns.."'")
    file.writeline("wifi_repo='"..wifi_repo.."'")
    file.writeline("wifi_desc='"..wifi_desc.."'")
    file.flush()
    file.close()
    node.compile("netconfig.lua")
    file.remove("netconfig.lua")
    collectgarbage()
    return true
end

