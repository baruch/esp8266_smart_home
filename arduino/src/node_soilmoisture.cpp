#include "common.h"
#include "node_mqtt.h"
#include "node_soilmoisture.h"
#include <Arduino.h>
#include "WiFiClient.h"

void NodeSoilMoisture::setup(void)
{
  m_ads1115.begin();
  m_ads1115.set_comp_queue(ADS1115_COMP_QUEUE_DISABLE);
  m_ads1115.set_mode(ADS1115_MODE_SINGLE_SHOT);
  m_ads1115.set_pga(ADS1115_PGA_ONE);
  m_deep_sleep = 0;
}

float NodeSoilMoisture::sample_read(void)
{
  while (m_ads1115.is_sample_in_progress()) {
    thread_sleep(3);
  }

  return m_ads1115.read_sample_float();
}

unsigned NodeSoilMoisture::loop(void)
{
  if (m_deep_sleep)
    return m_deep_sleep;

  thread_run();
  return m_deep_sleep;
}

void NodeSoilMoisture::i2c_error(uint8_t request)
{
  debug.log("Failed to trigger request ", request);
  mqtt_publish_int("i2c_state", request);
  m_deep_sleep = DEFAULT_DEEP_SLEEP_TIME*3;
}

void NodeSoilMoisture::user_thread(void)
{
  uint8_t request;

  // Start reading the battery voltage
  m_ads1115.set_pga(ADS1115_PGA_TWO);
  m_ads1115.set_mux(ADS1115_MUX_GND_AIN0);
  request = m_ads1115.trigger_sample();
  if (request != 0) {
    i2c_error(request);
    return;
  }

  m_bat = sample_read();
  m_bat *= 2.0;
  battery_check(m_bat);

  // Battery voltage read, initiate the moisture reading
  m_ads1115.set_pga(ADS1115_PGA_ONE);
  m_ads1115.set_mux(ADS1115_MUX_GND_AIN2);
  request = m_ads1115.trigger_sample();
  if (request != 0) {
    i2c_error(request);
    return;
  }

  m_moisture = sample_read();

  // Moisture voltage read, initiate the trigger reading
  m_ads1115.set_mux(ADS1115_MUX_GND_AIN3);
  request = m_ads1115.trigger_sample();
  if (request != 0) {
    i2c_error(request);
    return;
  }

  m_trigger = sample_read();

  debug.log("Battery: ", m_bat, " Moisture: ", m_moisture, " Digital: ", m_trigger);

  while (!mqtt_connected())
    thread_sleep(5);

  mqtt_publish_float("battery", m_bat);
  mqtt_publish_float("moisture", m_moisture);
  mqtt_publish_float("trigger", m_trigger);
  mqtt_publish_int("i2c_state", 0);

  m_deep_sleep = DEFAULT_DEEP_SLEEP_TIME*3;
}
