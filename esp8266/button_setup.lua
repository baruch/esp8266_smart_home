-- LED-button GPIO index
LBGI=LBGI or 5  -- GPIO 14
INIT_DELAY = 5*1000*1000

-- Process pressed button
gpio.mode(LBGI, gpio.INPUT, gpio.PULLUP)  -- Set IO to Input pullup to off the LED.
if gpio.read(LBGI) == 0 then -- button pressed
	t1 = tmr.now()
	print('Button pressed')
	tmr.alarm(5, 100, 1, function()
		if gpio.read(LBGI) == 1 and (tmr.now() - t1 < INIT_DELAY) then -- Abort wait
		    print("Button released.")
			tmr.stop(5)
			tmr.unregister(5)
		elseif gpio.read(LBGI) == 0 and (tmr.now() - t1 >= INIT_DELAY) then -- Initialize the gadget
			tmr.stop(5)
			tmr.unregister(5)
			print("Removing old settings and running setup.")
			file.remove("netconfig.lc")
			node.restart()
		end
	end)
end
