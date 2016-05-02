#include "common.h"
#include "node_mqtt.h"
#include "node_soilmoisture.h"
#include <Arduino.h>
#include "WiFiClient.h"

#define DEEP_SLEEP_TIME (DEFAULT_DEEP_SLEEP_TIME*3)

void NodeSoilMoisture::setup(void)
{
  m_ads1115.begin();
  m_ads1115.set_comp_queue(ADS1115_COMP_QUEUE_DISABLE);
  m_ads1115.set_mode(ADS1115_MODE_SINGLE_SHOT);
  m_ads1115.set_pga(ADS1115_PGA_ONE);
  m_deep_sleep = 0;
}

void NodeSoilMoisture::wait_for_read(float &pval)
{
  while (m_ads1115.is_sample_in_progress())
    thread_sleep(3);

  pval = m_ads1115.read_sample_float();
}

unsigned NodeSoilMoisture::loop(void)
{
  if (mqtt_connected() && m_deep_sleep == 0) {
    thread_run();
  }
  return m_deep_sleep;
}

void NodeSoilMoisture::error(uint8_t i2c_state)
{
  debug.log("Failed to i2c comm ", i2c_state);
  mqtt_publish_int("i2c_state", i2c_state);
  m_deep_sleep = DEFAULT_DEEP_SLEEP_TIME;
}

void NodeSoilMoisture::user_thread(void)
{
  uint8_t i2c_state;

  // Start reading the battery voltage
  m_ads1115.set_pga(ADS1115_PGA_TWO);
  m_ads1115.set_mux(ADS1115_MUX_GND_AIN0);
  i2c_state = m_ads1115.trigger_sample();
  if (i2c_state != 0) {
    error(i2c_state);
    return;
  }

  wait_for_read(m_bat);

  // Battery voltage read, initiate the moisture reading
  m_ads1115.set_pga(ADS1115_PGA_ONE);
  m_ads1115.set_mux(ADS1115_MUX_GND_AIN2);
  i2c_state = m_ads1115.trigger_sample();
  if (i2c_state != 0) {
    error(i2c_state);
    return;
  }

  m_bat *= 2.0;
  mqtt_publish_float("battery", m_bat);

  wait_for_read(m_moisture);

  // Moisture voltage read, initiate the trigger reading
  m_ads1115.set_mux(ADS1115_MUX_GND_AIN3);
  i2c_state = m_ads1115.trigger_sample();
  if (i2c_state != 0) {
    error(i2c_state);
    return;
  }

  mqtt_publish_float("moisture", m_moisture);

  wait_for_read(m_trigger);

  mqtt_publish_float("trigger", m_trigger);
  mqtt_publish_int("i2c_state", 0);
  debug.log("Battery: ", m_bat, " Moisture: ", m_moisture, " Digital: ", m_trigger);

  m_deep_sleep = DEFAULT_DEEP_SLEEP_TIME*3;
}
