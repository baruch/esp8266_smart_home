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
#define LED_PIN 4
#define ON_TIME 15  // minutes
#define OFF_TIME 30  // minutes

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

static inline bool is_led_on(void)
{
  return digitalRead(LED_PIN) == LOW;
}

static void turn_led_on(void)
{
  // Turn on led
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

static void turn_led_off(void)
{
  // Turn off led
  digitalWrite(LED_PIN, HIGH);
  pinMode(LED_PIN, INPUT_PULLUP);
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

  m_pump_on_trigger_time = ON_TIME * 60 * 1000; // minutes => millis
  m_pump_forced_off_max_time = OFF_TIME * 60 * 1000; // minutes => millis
  m_pump_on_min_current = 500; // 500mA

  m_pump_forced_off_time = 0;
  m_pump_on_time = 0;
  m_pump_on = false;
  m_pump_switched_off = false;
  m_alert_distance = false;
  m_alert_power = false;
  m_alert_pump = false;
  m_pump_current = 0;
  m_distance_filter.reset();
  m_distance = 0;
  m_last_i2c_state = -1;
  m_last_measure_time = millis();

  // TODO: Add threshold init for ADS to react on pump (higher probability than power off)
  // (or maybe better to set threshold for power detection)

  mqtt_subscribe("pump_on_trigger_time", std::bind(&NodeSewagePump::mqtt_pump_on_trigger_time, this, std::placeholders::_1));
  mqtt_subscribe("pump_off_time", std::bind(&NodeSewagePump::mqtt_pump_off_time, this, std::placeholders::_1));
  mqtt_subscribe("pump_on_min_current", std::bind(&NodeSewagePump::mqtt_pump_on_min_current, this, std::placeholders::_1));
}

unsigned NodeSewagePump::loop(void)
{
  static unsigned long m_last_sample_time = 0;
  unsigned long now = millis();

  // Wait 5 sec. Run loop once in 5 sec.
  if (now - m_last_sample_time < 5000)
    return 0;
  m_last_sample_time = now;

  bool update = false;
  update |= measure_current();
  update |= measure_distance();
  update |= measure_input_power();
  update |= control_pump(now);
  if (update) {
    // data updated, send an mqtt update on all variables
    mqtt_connected_event();
  }

  // TODO: Add long button press detection and action here. Or make it common.

  if (m_alert_pump) {
    if (is_led_on())
      turn_led_off();
    else
      turn_led_on();
  }

  return 0;
}

void NodeSewagePump::mqtt_connected_event(void)
{
  mqtt_publish_int("i2c_state", 0);
  mqtt_publish_bool("pump_on", m_pump_on);
  mqtt_publish_bool("pump_switched_off", m_pump_switched_off);
  mqtt_publish_bool("alert_power", m_alert_power);
  mqtt_publish_bool("alert_distance", m_alert_distance);
  mqtt_publish_bool("alert_pump", m_alert_pump);
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
  if (str2int(data, m_pump_forced_off_max_time))
    debug.log("Pump OFF time is now ", m_pump_forced_off_max_time);
}

void NodeSewagePump::mqtt_pump_on_min_current(char *data)
{
  if (str2int(data, m_pump_on_min_current))
    debug.log("Pump ON minimum current is now ", m_pump_on_min_current);
}

bool NodeSewagePump::measure_current(void)
{
  // Set ADS to read current from coil
  m_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1);
  m_adc.set_pga(ADS1115_PGA_TWO);
  m_adc.set_data_rate(ADS1115_DATA_RATE_860_SPS);
  m_adc.set_mode(ADS1115_MODE_CONTINUOUS);

  uint8_t i2c_state = m_adc.trigger_sample();
  if (i2c_state != m_last_i2c_state) {
    mqtt_publish_int("i2c_state", i2c_state);
    m_last_i2c_state = i2c_state;
  }
  if (i2c_state != 0)
    return false;

  float sum = 0.0;
  delay(1);
  #define NUM_CURRENT_SAMPLES 100
  for (int i = 0; i < NUM_CURRENT_SAMPLES; i++) {
    float val = m_adc.read_sample_float();
	  sum += val * val;
    delay(1);
  }
  float current_rms = sqrt(sum / NUM_CURRENT_SAMPLES) * 5.0;

  // Turn off the continuous sampling
  m_adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  m_adc.trigger_sample();

  unsigned prev_pump_current = m_pump_current;
  m_pump_current = int(current_rms*1000);  // mA

  // Prepare report about pump state
  bool prev_pump_on = m_pump_on;
  m_pump_on = m_pump_current > m_pump_on_min_current;

  // Update if pump switched on or off and if pump current changed by more than 100ma
  return (abs(m_pump_current - prev_pump_current) > 100) || m_pump_on != prev_pump_on;
}

bool NodeSewagePump::control_pump(unsigned long now)
{
  // Save pump alert status
  bool old_status = m_alert_pump;

  // Update pump function/block time
  if (m_pump_on) {
	  m_pump_on_time += (now - m_last_measure_time);
    if (m_pump_on_time >= m_pump_on_trigger_time) {
      // Pump on for too long, turn it off
      m_alert_pump = true;
      digitalWrite(RELAY_PIN, LOW);
    }
  } else if (m_alert_pump) {
	  // Pump is off due to alert
	  m_pump_forced_off_time += (now - m_last_measure_time);
    if (m_pump_forced_off_time >= m_pump_forced_off_max_time) {
      m_alert_pump = false;
      digitalWrite(RELAY_PIN, HIGH);
      turn_led_off();
    }
  } else {
    // Reset all timers
    m_pump_on_time = 0;
    m_pump_forced_off_time = 0;
  }

  m_last_measure_time = now;

  // Update data if pump alert state changed
  return old_status != m_alert_pump;
}

bool NodeSewagePump::measure_input_power(void)
{
  m_adc.set_mux(ADS1115_MUX_GND_AIN2);
  m_adc.set_pga(ADS1115_PGA_ONE);
  m_adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  m_adc.set_data_rate(ADS1115_DATA_RATE_860_SPS);

  // Trigger the measure
  uint8_t i2c_state = m_adc.trigger_sample();
  if (i2c_state != m_last_i2c_state) {
    mqtt_publish_int("i2c_state", i2c_state);
    m_last_i2c_state = i2c_state;
  }
  if (i2c_state != 0)
    return false;

  while (m_adc.is_sample_in_progress())
    delay(1);

  float input_power_raw = m_adc.read_sample_float();
  bool input_power = input_power_raw > 2.0;

  if (input_power == m_alert_power) {
    debug.log("Input power changed from ", m_alert_power, " to ", input_power, " raw ", input_power_raw);
    m_alert_power = !input_power;
    return true;
  }

  return false;
}

bool NodeSewagePump::measure_distance(void)
{
  // Initiate trigger pulse for 10msec
  digitalWrite(DISTANCE_TRIGGER_PIN, HIGH);
  delay(10);

  long duration;
  {
    InterruptLock lock;

    // Finishes trigger pulse, start to measure time from now
    digitalWrite(DISTANCE_TRIGGER_PIN, LOW);

    // Blocks for at most 10msec with no interrupts
    duration = pulseIn(DISTANCE_DATA_PIN, HIGH, 10000);
  }

  float distance = duration / 2.0 / 29.1;
  m_distance_filter.input((int)distance);

  if (abs(m_distance - m_distance_filter.output()) >= 2) {
    debug.log("Distance changed from ", m_distance, " to ", m_distance_filter.output());
    m_distance = m_distance_filter.output();
    // alert when water closer than 25 cm. Hysteresis of 2cm to remove the alert
    m_alert_distance = (m_alert_distance) ? m_distance > 27 : m_distance < 25;
    return true;
  }
  return false;
}
