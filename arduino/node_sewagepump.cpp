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

  mqtt_subscribe("pump_on_trigger_time", std::bind(&NodeSewagePump::mqtt_pump_on_trigger_time, this, std::placeholders::_1));
  mqtt_subscribe("pump_off_time", std::bind(&NodeSewagePump::mqtt_pump_off_time, this, std::placeholders::_1));
  mqtt_subscribe("pump_on_min_current", std::bind(&NodeSewagePump::mqtt_pump_on_min_current, this, std::placeholders::_1));
}

unsigned NodeSewagePump::loop(void)
{
  return 0;
}

void NodeSewagePump::mqtt_connected_event(void)
{
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
