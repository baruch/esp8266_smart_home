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
  m_state = STATE_INIT;
  m_next_poll = 0;
}

bool NodeSoilMoisture::sample_read(float &pval)
{
  if (m_ads1115.is_sample_in_progress())
    return false;

  pval = m_ads1115.read_sample_float();
  return true;
}

unsigned NodeSoilMoisture::loop(void)
{
  if (!mqtt_connected())
    return 0;

  if (!TIME_PASSED(m_next_poll))
    return 0;

  m_next_poll = millis() + 3;

  uint8_t request;
  switch (m_state) {
    case STATE_INIT:
      // Start reading the battery voltage
      m_ads1115.set_pga(ADS1115_PGA_TWO);
      m_ads1115.set_mux(ADS1115_MUX_GND_AIN0);
      request = m_ads1115.trigger_sample();
      if (request != 0) {
        m_state = STATE_DONE;
        debug.log("Failed to trigger request ", request);
        return 0;
      }
      m_state = STATE_READ_BAT;
      break;

    case STATE_READ_BAT:
      if (sample_read(m_bat)) {
        // Battery voltage read, initiate the moisture reading
        m_ads1115.set_pga(ADS1115_PGA_ONE);
        m_ads1115.set_mux(ADS1115_MUX_GND_AIN2);
        request = m_ads1115.trigger_sample();
        if (request != 0) {
          m_state = STATE_DONE;
          debug.log("Failed to trigger request ", request);
          return 0;
        }

        // Send the battery voltage
        m_bat *= 2.0;
        mqtt_publish_float("battery", m_bat);
        m_state = STATE_READ_MOISTURE;
      }
      break;

    case STATE_READ_MOISTURE:
      if (sample_read(m_moisture)) {
        // Moisture voltage read, initiate the trigger reading
        m_ads1115.set_mux(ADS1115_MUX_GND_AIN3);
        request = m_ads1115.trigger_sample();
        if (request != 0) {
          m_state = STATE_DONE;
          debug.log("Failed to trigger request ", request);
          return 0;
        }

        // Send the moisture reading
        mqtt_publish_float("moisture", m_moisture);
        m_state = STATE_READ_TRIGGER;
      }
      break;

    case STATE_READ_TRIGGER:
      if (sample_read(m_trigger)) {
        //  Trigger reading done, send it
        mqtt_publish_float("trigger", m_trigger);
        m_state = STATE_DONE;
        debug.log("Battery: ", m_bat, " Moisture: ", m_moisture, " Digital: ", m_trigger);
        return DEFAULT_DEEP_SLEEP_TIME*3;
      }
      break;
  }

  return 0;
}
