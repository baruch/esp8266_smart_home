#include "common.h"
#include "node_mqtt.h"
#include "node_relaywbutton.h"
#include <Arduino.h>

#define BUTTON_PIN 12 // GPIO12
#define BUTTON_RELAY 13 // GPIO13

#define DEBOUNCE_COUNT_MAX 15

void NodeRelayWithButton::setup(void)
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RELAY, OUTPUT);
  set_state(1); // Default to output turned on

  debounce_count = DEBOUNCE_COUNT_MAX;
  last_sample_millis = millis();
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

void NodeRelayWithButton::set_state(int state)
{
  digitalWrite(BUTTON_RELAY, state);
  relay_state = state;
  state_update();
  debug.log("state change ", state);
}

void NodeRelayWithButton::toggle_state(void)
{
  debug.log("Toggle state");
  set_state(1 - relay_state);
}

