#include "common.h"
#include "node_mqtt.h"
#include "node_relaywbutton.h"
#include <Arduino.h>
#include <Wire.h>
#include <time.h>

#define ADS_ALERT_PIN 5
#define BUTTON_PIN 12 // GPIO12
#define RELAY_PIN 13 // GPIO13

#define DEBOUNCE_COUNT_MAX 15

//#define TRACE_IP 192,168,2,226
#ifdef TRACE_IP
#include "UdpTrace.h"
static UdpTrace m_trace;
#endif

void NodeRelayWithButton::mqtt_relay_config(char *payload)
{
  char ch;

  if (strlen(payload) != 1) {
    debug.log("Requiring a single byte payload, got ", strlen(payload));
    return;
  }

  ch = payload[0];

  switch (ch) {
    case '0':
      debug.log("Got zero for relay config");
      set_relay_config(0);
      break;

    case '1':
      debug.log("Got one for relay config");
      set_relay_config(1);
      break;

    default:
      debug.log("Got unknown value for relay config: ", payload);
      return;
  }
}

void NodeRelayWithButton::setup(void)
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  relay_config = relay_state = 1;

  Wire.begin(2, 0);
  m_adc.begin();
  m_adc.set_data_rate(ADS1115_DATA_RATE_860_SPS);
  m_adc.set_mode(ADS1115_MODE_CONTINUOUS);
  m_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1);
  m_adc.set_pga(ADS1115_PGA_TWO);
  if (m_adc.trigger_sample() != 0) {
    debug.log("ADS1115 unreachable on I2C");
  }

  m_current = 0;
  m_current_sample_time = 0;
  m_current_samples = 0;
  m_current_sum = 0;
  m_last_update = 0;
  debounce_count = DEBOUNCE_COUNT_MAX;
  last_sample_millis = millis();
  mqtt_subscribe("relay_config", std::bind(&NodeRelayWithButton::mqtt_relay_config, this, std::placeholders::_1));

  config_time(); // We will need wall time to do the logic

#ifdef TRACE_IP
  m_trace.begin(IPAddress(TRACE_IP), 9999);
#endif
}

void NodeRelayWithButton::check_button(unsigned long now)
{
  // Read button state
  if (now == last_sample_millis)
    return;

  last_sample_millis = now;
  int state = digitalRead(BUTTON_PIN);
  if (state) {
    if (debounce_count < DEBOUNCE_COUNT_MAX) {
      debounce_count++;
    }
  } else {
    if (debounce_count > 0) {
      debounce_count--;
      if (debounce_count == 0) {
        button_pressed();
      }
    }
  }
}

void NodeRelayWithButton::check_current(unsigned long now)
{
  // Read current if we are on
  if (relay_state != 1) {
    if (m_current > 0.00001) {
      debug.log("Relay turned off, report zero current");
      m_current = 0;
      m_current_sum = 0;
      m_current_samples = 0;
      m_last_update = 0;
    }
    return;
  }

  // Read from i2c only once every msec, adc samples at 870sps
  if (now == m_current_sample_time)
    return;
  m_current_sample_time = now;

  uint16_t rval = m_adc.read_sample();
#ifdef TRACE_IP
  m_trace.sample(now, rval);
#endif
  float val = m_adc.sample_to_float(rval);
  m_current_sum += val*val;
  m_current_samples++;

  if (m_current_samples == 1000) { // Approx. 1 second
    m_current = 5.0*sqrt(m_current_sum / m_current_samples);
    if (m_current < 0.005) // ADC noise, round down to 0 current
      m_current = 0.0;
    m_current_sum = 0;
    m_current_samples = 0;
  }

  float diff = abs(m_last_reported_current - m_current);
  // Change current update frequency based on level of change and time passed
  if (diff > 0.01 || (now - m_last_update >= 60000 && diff > 0.001)) {
    m_last_update = 0;
    debug.log("Current ", m_current);
  }
}

unsigned NodeRelayWithButton::loop(void)
{
  unsigned long now = millis();

  check_button(now);
  check_current(now);
  check_relay_state();
  // Update if requested or once per 15 minutes
  if (m_last_update == 0 || now - m_last_update > 15*60*1000) {
    state_update();

    time_t t = time(NULL);
    char* result = ctime(&t);
    debug.log("Time ", t, " is ", result);
  }
  return 0;
}

void NodeRelayWithButton::button_pressed(void)
{
  debug.log("Button pressed");
  toggle_config();
}

void NodeRelayWithButton::mqtt_connected_event(void)
{
  m_last_update = 0;
}

void NodeRelayWithButton::state_update(void)
{
  mqtt_publish_bool("relay_state", relay_state);
  mqtt_publish_float("current", m_current);
  m_last_reported_current = m_current;
  m_last_update = millis();
}

void NodeRelayWithButton::check_relay_state(void)
{
  if (relay_state == relay_config)
    return;

  digitalWrite(RELAY_PIN, relay_config);
  relay_state = relay_config;
  debug.log("state change ", relay_state);
  m_last_update = 0;
}

void NodeRelayWithButton::toggle_config(void)
{
  debug.log("Toggle relay config");
  relay_config ^= 1;
  mqtt_publish_bool("relay_config", relay_config);
}

void NodeRelayWithButton::set_relay_config(int state)
{
  debug.log("Relay config set to ", state);
  relay_config = state;
}
