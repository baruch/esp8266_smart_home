#include <Arduino.h>
#include <Wire.h>
#include "ADS1115.h"

#define SAMPLE_BIT (0x8000)

enum ads1115_register {
	ADS1115_REGISTER_CONVERSION = 0,
	ADS1115_REGISTER_CONFIG = 1,
	ADS1115_REGISTER_LOW_THRESH = 2,
	ADS1115_REGISTER_HIGH_THRESH = 3,
};

ADS1115::ADS1115(uint8_t address)
{
        m_address = address;
        m_config = ADS1115_COMP_QUEUE_AFTER_ONE |
                   ADS1115_COMP_LATCH_NO |
                   ADS1115_COMP_POLARITY_ACTIVE_LOW |
                   ADS1115_COMP_MODE_WINDOW |
                   ADS1115_DATA_RATE_128_SPS |
                   ADS1115_MODE_SINGLE_SHOT |
                   ADS1115_MUX_GND_AIN0;
}

uint8_t ADS1115::write_register(uint8_t reg, uint16_t val)
{
        Wire.beginTransmission(m_address);
        Wire.write(reg);
        Wire.write(val>>8);
        Wire.write(val & 0xFF);
        return Wire.endTransmission();
}

uint16_t ADS1115::read_register(uint8_t reg)
{
        Wire.beginTransmission(m_address);
        Wire.write(reg);
        Wire.endTransmission();

        uint8_t result = Wire.requestFrom(m_address, 2, true);
        if (result != 2) {
                Wire.flush();
                Serial.println("Not enough data read");
                return 0;
        }

        uint16_t val;

        val = Wire.read() << 8;
        val |= Wire.read();
        return val;
}

void ADS1115::begin()
{
        Wire.begin();
}

uint8_t ADS1115::trigger_sample()
{
        return write_register(ADS1115_REGISTER_CONFIG, m_config | SAMPLE_BIT);
}

bool ADS1115::is_sample_in_progress()
{
        return (read_register(ADS1115_REGISTER_CONFIG) & SAMPLE_BIT) != 0;
}

uint16_t ADS1115::read_sample()
{
        return read_register(ADS1115_REGISTER_CONVERSION);
}