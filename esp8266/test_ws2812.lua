wspin=5

function ws_cycle()
    local r,g,b
    for r = 0,255 do
        for g = 0,255 do
            for b = 0,255 do
                local leds = string.char(r,g,b)
                print(r, g, b)
                ws2812.write(wspin, leds)
                tmr.wdclr()
                tmr.delay(10*1000)
            end
        end
    end
end
