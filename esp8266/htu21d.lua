htu21d = {
	address = 64,

	init = function (self, sda, scl)
		self.bus = 0
		i2c.setup(self.bus, sda, scl, i2c.SLOW)
	end,

        sendcmd = function (self, cmd)
                i2c.start(self.bus)
                i2c.address(self.bus, self.address, i2c.TRANSMITTER)
                i2c.write(self.bus, cmd)
                i2c.stop(self.bus)
        end,

        readreply = function (self, len)
                i2c.start(self.bus)
                i2c.address(self.bus, self.address, i2c.RECEIVER)
                local c = i2c.read(self.bus, len)
                i2c.stop(self.bus)
                return c
        end,

        crc8 = function (data)
            local crc = 0x00
            local crc8_poly = 0x31 -- 0x31=0b00110001 <=> x^8+x^5+x^4+x^0
            local i
            for i = 1, #data do
                crc = bit.bxor( crc, data:byte(i) )
                local b
                for b = 1, 8 do
                    local t = bit.lshift(crc, 1)
                    if bit.band(crc, 0x80) == 0x80 then
                        crc = bit.bxor(t, crc8_poly)
                    else
                        crc = t
                    end
                end
            end
            return bit.band(crc,0xff)
        end,

        dataToFloat = function (self, data)
		local msb = string.byte(data, 1)
                local lsb = string.byte(data, 2)
                local cksum = string.byte(data, 3)
		local raw = bit.bor(bit.lshift(msb,8), lsb)
		raw = bit.band(raw, 0xFFFC)
		return raw/65535
        end,

	readTempRaw = function (self)
                self:sendcmd(0xF3)
		tmr.delay(50000)
		return self:readreply(3)
	end,

	temp = function (self, data)
                local data
                if data == nil then
                        data = self:readTempRaw()
                end
                if self:crc8(data) ~= 0 then
                        return -1000
                end
                local temp = self:dataToFloat(data)
                local real = -46.85+(172.72*temp)
		return real
	end,

        readHumidRaw = function(self)
                self:sendcmd(0xF5)
                tmr.delay(50000)
                return self:readreply(3)
        end,

        humidity = function(self)
                local data
                if data == nil then
                        data = self:readHumidRaw()
                end
                if self:crc8(data) ~= 0 then
                        return -1000
                end
                local humid = self:dataToFloat(data)
                local real = 125 * humid - 6
		return real
        end,
}
