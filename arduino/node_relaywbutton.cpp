#include "common.h"
#include "node_mqtt.h"
#include "node_relaywbutton.h"
#include <Arduino.h>

#define BUTTON_PIN 12 // GPIO12
#define BUTTON_RELAY 13 // GPIO13

#define DEBOUNCE_COUNT_MAX 15

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
      set_state(0);
      break;

    case '1':
      debug.log("Got one for relay state");
      set_state(1);
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
  pinMode(BUTTON_RELAY, OUTPUT);
  set_state(1); // Default to output turned on

  debounce_count = DEBOUNCE_COUNT_MAX;
  last_sample_millis = millis();
  mqtt_subscribe("relay_state", std::bind(&NodeRelayWithButton::mqtt_relay_state, this, std::placeholders::_1));
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

  return 0;
}

void NodeRelayWithButton::button_pressed(void)
{
  toggle_state();
}

void NodeRelayWithButton::mqtt_connected_event(void)
{
  state_update();
}

void NodeRelayWithButton::state_update(void)
{
  mqtt_publish_bool("relay_state", relay_state);
}

bool NodeRelayWithButton::set_state_no_update(int state)
{
  if (state == relay_state) {
    debug.log("Request to change to the current state, ignoring");
    return false;
  }
  digitalWrite(BUTTON_RELAY, state);
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
