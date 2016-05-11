#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"

void NodeHTU21D::setup(void)
{
  htu21d.begin();
  m_deep_sleep = 0;
}

unsigned NodeHTU21D::loop(void)
{
  if (m_deep_sleep == 0) {
    thread_run();
  }
  return m_deep_sleep;
}

bool NodeHTU21D::wait_for_result(uint16_t &value)
{
  uint8_t cksum;
  int counter;

  debug.log("Waiting for result");
  for (counter = 0; counter < 100; counter++) {
    if (htu21d.try_read_value(value, cksum)) {
      if (htu21d.check_crc(value, cksum) == false) {
        debug.log("Checksum error");
        error(101);
        return false;
      }
      debug.log("waiting completed");
      return true;
    }
    thread_sleep(5);
  }

  error(100);
  debug.log("Timed out waiting for data");
  return false;
}

void NodeHTU21D::error(uint8_t i2c_state)
{
  mqtt_publish_int("i2c_state", i2c_state);
  m_deep_sleep = DEFAULT_DEEP_SLEEP_TIME;
}

void NodeHTU21D::user_thread(void)
{
  uint16_t value;
  uint8_t i2c_state;

  debug.log("Read humidity");
  i2c_state = htu21d.trigger_read_humidity();
  if (i2c_state) {
    error(i2c_state);
    debug.log("Failed to trigger humidity read from i2c");
    return;
  }
  if (!wait_for_result(value))
    return;
  float humd = htu21d.translate_humidity(value);

  debug.log("Read temperature");
  i2c_state = htu21d.trigger_read_temp();
  if (i2c_state) {
    error(i2c_state);
    debug.log("Failed to trigger temperature read from i2c");
    return;
  }
  if (!wait_for_result(value))
    return;
  float temp = htu21d.translate_temp(value);

  float bat = analogRead(0) * 4.2 / 1024.0;
  debug.log("Temperature:", temp, " Humidity: ", humd, "%% bat ", bat);

  while (!mqtt_connected())
    thread_yield();

  mqtt_publish_float("temperature", temp);
  mqtt_publish_float("humidity", humd);
  mqtt_publish_float("battery", bat);

  m_deep_sleep = DEFAULT_DEEP_SLEEP_TIME;
}
