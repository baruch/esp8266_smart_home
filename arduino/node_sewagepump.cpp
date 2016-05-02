#include "common.h"
#include "node_mqtt.h"
#include "node_sewagepump.h"
#include <Arduino.h>

static bool str2int(const char *data, int& value)
{
  char *endptr = NULL;
  int tmp = strtol(data, &endptr, 10);
  if (*data != '\0' && *endptr == '\0') {
    value = tmp;
    return true;
  }
  debug.log("Invalid value provided (not an integer)");
  return false;
}

void NodeSewagePump::setup(void)
{
  m_pump_on_trigger_time = 30; // 30 minutes
  m_pump_off_time = 30; // 30 minutes
  m_pump_on_min_current = 500; // 500mA

  m_pump_on = false;
  m_pump_switched_off = false;
  m_input_power = false;
  m_pump_current = 0;
  m_distance = 0;

  mqtt_subscribe("pump_on_trigger_time", std::bind(&NodeSewagePump::mqtt_pump_on_trigger_time, this, std::placeholders::_1));
  mqtt_subscribe("pump_off_time", std::bind(&NodeSewagePump::mqtt_pump_off_time, this, std::placeholders::_1));
  mqtt_subscribe("pump_on_min_current", std::bind(&NodeSewagePump::mqtt_pump_on_min_current, this, std::placeholders::_1));
}

unsigned NodeSewagePump::loop(void)
{
  if (measure_current() || measure_distance() || measure_input_power()) {
    // data updated, send an mqtt update on all variables
    mqtt_connected_event();
  }
  return 0;
}

void NodeSewagePump::mqtt_connected_event(void)
{
  mqtt_publish_bool("pump_on", m_pump_on);
  mqtt_publish_bool("pump_switched_off", m_pump_switched_off);
  mqtt_publish_bool("input_power", m_input_power);
  mqtt_publish_int("pump_current", m_pump_current);
  mqtt_publish_int("distance", m_distance);
}

void NodeSewagePump::mqtt_pump_on_trigger_time(char *data)
{
  if (str2int(data, m_pump_on_trigger_time))
    debug.log("Pump ON trigger time is now ", m_pump_on_trigger_time);
}

void NodeSewagePump::mqtt_pump_off_time(char *data)
{
  if (str2int(data, m_pump_off_time))
    debug.log("Pump OFF time is now ", m_pump_off_time);
}

void NodeSewagePump::mqtt_pump_on_min_current(char *data)
{
  if (str2int(data, m_pump_on_min_current))
    debug.log("Pump ON minimum current is now ", m_pump_on_min_current);
}

bool NodeSewagePump::measure_current(void)
{
  return false;
}

bool NodeSewagePump::measure_input_power(void)
{
  return false;
}

bool NodeSewagePump::measure_distance(void)
{
  return false;
}
