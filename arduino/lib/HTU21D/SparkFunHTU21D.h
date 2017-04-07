/* 
 HTU21D Humidity Sensor Library
 By: Nathan Seidle
 SparkFun Electronics
 Date: September 22nd, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 Get humidity and temperature from the HTU21D sensor.
 
 */
 

#if defined(ARDUINO) && ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>

#define HTDU21D_ADDRESS 0x40  //Unshifted 7-bit I2C address for the sensor

#define TRIGGER_TEMP_MEASURE_HOLD  0xE3
#define TRIGGER_HUMD_MEASURE_HOLD  0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD  0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD  0xF5
#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE

#define USER_REGISTER_RESOLUTION_MASK 0x81
#define USER_REGISTER_RESOLUTION_RH12_TEMP14 0x00
#define USER_REGISTER_RESOLUTION_RH8_TEMP12 0x01
#define USER_REGISTER_RESOLUTION_RH10_TEMP13 0x80
#define USER_REGISTER_RESOLUTION_RH11_TEMP11 0x81

#define USER_REGISTER_END_OF_BATTERY 0x40
#define USER_REGISTER_HEATER_ENABLED 0x04
#define USER_REGISTER_DISABLE_OTP_RELOAD 0x02

class HTU21D {

public:
  HTU21D();

  //Public Functions
  static void begin();

  // Non-blocking functions
  static uint8_t trigger_read_temp(void);
  static uint8_t trigger_read_humidity(void);
  static bool try_read_value(uint16_t &value, uint8_t &cksum);
  static bool check_crc(uint16_t value, uint8_t cksum);
  static float translate_humidity(uint16_t value);
  static float translate_temp(uint16_t value);

  // Blocking functions
  static float readHumidity(void);
  static float readTemperature(void);
  static void setResolution(byte resBits);

  static byte readUserRegister(void);
  static void writeUserRegister(byte val);

private:
  //Private Functions

  static float read_value(void);

  //Private Variables

};
