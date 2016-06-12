#include "common.h"
#include "node_mqtt.h"
#include "node_relaywbutton.h"
#include <Arduino.h>
#include <Wire.h>

#define ADS_ALERT_PIN 5
#define BUTTON_PIN 12 // GPIO12
#define RELAY_PIN 13 // GPIO13

#define DEBOUNCE_COUNT_MAX 15

// Current sensor is 1 for coil and 2 for ACS712
#define CURRENT_SENSOR 1

//#define TRACE_IP 192,168,2,226
#ifdef TRACE_IP
#include "libraries/UdpTrace/UdpTrace.h"
static UdpTrace m_trace;
#endif

void NodeRelayWithButton::mqtt_relay_state(char *payload)
{
  char ch;
  int state;

  if (strlen(payload) != 1) {
    debug.log("Requiring a single byte payload, got ", strlen(payload));
    return;
  }

  ch = payload[0];

  switch (ch) {
    case '0':
      debug.log("Got zero for relay state");
      state = 0;
      break;

    case '1':
      debug.log("Got one for relay state");
      state = 1;
      break;

    default:
      debug.log("Got unknown value for relay state: ", payload);
      return;
  }

  set_state_no_update(state);
}

void NodeRelayWithButton::setup(void)
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  relay_state = 1;

  Wire.begin(2, 0);
  m_adc.begin();
  m_adc.set_data_rate(ADS1115_DATA_RATE_860_SPS);
  m_adc.set_mode(ADS1115_MODE_CONTINUOUS);
#if CURRENT_SENSOR == 1
  m_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1);
#else
  m_adc.set_mux(ADS1115_MUX_GND_AIN3);
#endif
  m_adc.set_pga(ADS1115_PGA_ONE);
  if (m_adc.trigger_sample() != 0) {
    debug.log("ADS1115 unreachable on I2C");
  }

  debounce_count = DEBOUNCE_COUNT_MAX;
  last_sample_millis = millis();
  mqtt_subscribe("relay_state", std::bind(&NodeRelayWithButton::mqtt_relay_state, this, std::placeholders::_1));

#ifdef TRACE_IP
  m_trace.begin(IPAddress(TRACE_IP), 9999);
#endif
}

unsigned NodeRelayWithButton::loop(void)
{
  unsigned long now = millis();
  if (now != last_sample_millis) {
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

  if (relay_state == HIGH) {

    if (now != m_current_sample_time) {
      uint16_t rval = m_adc.read_sample();
#ifdef TRACE_IP
      m_trace.sample(now, rval);
#endif
      float val = m_adc.sample_to_float(rval);
      #if CURRENT_SENSOR == 2
      val -= 1.65;
      #endif
      m_current_sum += val*val;
      m_current_samples++;
      m_current_sample_time = now;

      if (m_current_samples == 1000) { // Approx. 1 second
        m_current = m_current_sum / m_current_samples;
        #if CURRENT_SENSOR == 2
        // 5A ACS712 has sensitivity value of 185mV/A
        m_current /= 0.185;
        #endif
        m_current = sqrt(m_current);
        m_current_sum = 0;
        m_current_samples = 0;
      }
    }

  } else {
    m_current = 0;
    m_current_sum = 0;
    m_current_samples = 0;
  }

  if (abs(m_last_reported_current - m_current) > 0.01) {
    state_update();
    debug.log("Current ", m_current);
  }

  return 0;
}

void NodeRelayWithButton::button_pressed(void)
{
  debug.log("Button pressed");
  toggle_state();
}

void NodeRelayWithButton::mqtt_connected_event(void)
{
  state_update();
}

void NodeRelayWithButton::state_update(void)
{
  mqtt_publish_bool("relay_state", relay_state);
  mqtt_publish_float("current", m_current);
  m_last_reported_current = m_current;
}

bool NodeRelayWithButton::set_state_no_update(int state)
{
  if (state == relay_state) {
    debug.log("Request to change to the current state, ignoring");
    return false;
  }
  digitalWrite(RELAY_PIN, state);
  relay_state = state;
  debug.log("state change ", state);
  return true;
}

void NodeRelayWithButton::set_state(int state)
{
  if (set_state_no_update(state))
    state_update();
}

void NodeRelayWithButton::toggle_state(void)
{
  debug.log("Toggle state");
  set_state(1 - relay_state);
}
