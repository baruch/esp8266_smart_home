#include "common.h"
#include "node_mqtt.h"
#include "node_sewagepump.h"
#include <Arduino.h>
#include <Wire.h>

#define DISTANCE_TRIGGER_PIN 14
#define DISTANCE_DATA_PIN 13
#define BUTTON_PIN 4
#define RELAY_PIN 5

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
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, 0); // Relay is Normally Closed so this should keep the pump online by default
  pinMode(DISTANCE_TRIGGER_PIN, OUTPUT);
  pinMode(DISTANCE_DATA_PIN, INPUT);
  Wire.begin(0, 2); // Configure I2C on alternate port
  m_adc.begin();

  m_pump_on_trigger_time = 30; // 30 minutes
  m_pump_off_time = 30; // 30 minutes
  m_pump_on_min_current = 500; // 500mA

  m_pump_on = false;
  m_pump_switched_off = false;
  m_input_power = false;
  m_pump_current = 0;
  m_distance = 0;
  m_last_i2c_state = -1;

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
  mqtt_publish_int("i2c_state", 0);
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
  m_adc.set_mux(ADS1115_MUX_GND_AIN2);
  m_adc.set_pga(ADS1115_PGA_ONE);

  uint8_t i2c_state = m_adc.trigger_sample();
  if (i2c_state != m_last_i2c_state) {
    mqtt_publish_int("i2c_state", i2c_state);
    m_last_i2c_state = i2c_state;
  }
  if (i2c_state != 0)
    return false;

  while (!m_adc.is_sample_in_progress())
    delay(1);

  float input_power_raw = m_adc.read_sample_float();
  bool input_power = input_power_raw > 0.5;
  if (input_power != m_input_power) {
    debug.log("Input power changed from ", m_input_power, " to ", input_power);
    m_input_power = input_power;
    return true;
  }

  return false;
}

bool NodeSewagePump::measure_distance(void)
{
  digitalWrite(DISTANCE_TRIGGER_PIN, HIGH);
  delay(10);
  digitalWrite(DISTANCE_TRIGGER_PIN, LOW);

  long duration = pulseIn(DISTANCE_TRIGGER_PIN, HIGH, 10000);
  float distance = (duration / 2 ) / 29.1;
  int idistance = distance;
  if (abs(idistance - m_distance) > 2) {
    debug.log("Distance changed from ", m_distance, " to ", idistance);
    m_distance = idistance;
    return true;
  }
  return false;
}
