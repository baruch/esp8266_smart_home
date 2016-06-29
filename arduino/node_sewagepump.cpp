#include "common.h"
#include "node_mqtt.h"
#include "node_sewagepump.h"
#include <Arduino.h>
#include <Wire.h>
#include <interrupts.h>

#include <ESP8266WiFi.h>

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
  WiFi.setOutputPower(20);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relay is Normally Closed and active low (low means we close the relay)
  pinMode(DISTANCE_TRIGGER_PIN, OUTPUT);
  digitalWrite(DISTANCE_TRIGGER_PIN, LOW);
  pinMode(DISTANCE_DATA_PIN, INPUT);
  Wire.begin(12, 0); // Configure I2C IOs. SDA, SCL.

  m_adc.begin();

  m_pump_on_trigger_time = 30; // 30 minutes
  m_pump_off_time = 30; // 30 minutes
  m_pump_on_min_current = 500; // 500mA

  m_pump_on = false;
  m_pump_switched_off = false;
  m_input_power = false;
  m_pump_current = 0;
  m_distance_filter.reset();
  m_distance = 0;
  m_last_i2c_state = -1;

  mqtt_subscribe("pump_on_trigger_time", std::bind(&NodeSewagePump::mqtt_pump_on_trigger_time, this, std::placeholders::_1));
  mqtt_subscribe("pump_off_time", std::bind(&NodeSewagePump::mqtt_pump_off_time, this, std::placeholders::_1));
  mqtt_subscribe("pump_on_min_current", std::bind(&NodeSewagePump::mqtt_pump_on_min_current, this, std::placeholders::_1));
}

unsigned NodeSewagePump::loop(void)
{
  static unsigned long m_last_sample_time = 0;
  unsigned long now = millis();

  if (now - m_last_sample_time < 5000)
    return 0;
  m_last_sample_time = now;

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
  /*
  m_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1);
  m_adc.set_pga(ADS1115_PGA_EIGHT);
  m_adc.set_data_rate(ADS1115_DATA_RATE_250_SPS);
  m_adc.set_mode(ADS1115_MODE_CONTINUOUS);

  uint8_t i2c_state = m_adc.trigger_sample();
  if (i2c_state != m_last_i2c_state) {
    mqtt_publish_int("i2c_state", i2c_state);
    m_last_i2c_state = i2c_state;
  }
  if (i2c_state != 0)
    return false;

  float sum = 0.0;
  delay(5);
  #define NUM_CURRENT_SAMPLES 50
  for (int i = 0; i < NUM_CURRENT_SAMPLES; i++) {
    float val = m_adc.read_sample_float();
    sum += val * val;
  }
  float current_rms = sqrt(sum / NUM_CURRENT_SAMPLES);
  debug.log("current ", current_rms);

  // Turn off the continuous sampling
  m_adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  m_adc.trigger_sample();
  */
  return false;
}

bool NodeSewagePump::measure_input_power(void)
{
  m_adc.set_mux(ADS1115_MUX_GND_AIN2);
  m_adc.set_pga(ADS1115_PGA_ONE);
  m_adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  m_adc.set_data_rate(ADS1115_DATA_RATE_250_SPS);

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
  bool input_power = input_power_raw > 2.0;
  if (input_power != m_input_power) {
    debug.log("Input power changed from ", m_input_power, " to ", input_power, " raw ", input_power_raw);
    m_input_power = input_power;
    return true;
  }

  return false;
}

bool NodeSewagePump::measure_distance(void)
{
  digitalWrite(DISTANCE_TRIGGER_PIN, HIGH);
  delay(10);

  long duration;
  {
    InterruptLock lock;
    digitalWrite(DISTANCE_TRIGGER_PIN, LOW);

    duration = pulseIn(DISTANCE_DATA_PIN, HIGH, 10000);
  }

  float distance = duration / 2.0  / 29.1;
  m_distance_filter.input((int)distance);

  if (abs(m_distance - m_distance_filter.output()) >= 2) {
    debug.log("Distance changed from ", m_distance, " to ", m_distance_filter.output());
    m_distance = m_distance_filter.output();
    return true;
  }
  return false;
}
