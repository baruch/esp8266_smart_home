#include "common.h"
#include "node_mqtt.h"
#include "node_relaywbutton.h"
#include <Arduino.h>
#include "DebugSerial.h"

#define BUTTON_PIN 12 // GPIO12
#define BUTTON_RELAY 13 // GPIO13

static int button_state_changed;

void pin_interrupt(void)
{
  button_state_changed = 1;
}

void NodeRelayWithButton::setup(void)
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RELAY, OUTPUT);
  button_state_changed = 0;
  last_button_millis = 0;
  set_state(0);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), pin_interrupt, FALLING);
}

void NodeRelayWithButton::loop(void)
{
  if (button_state_changed) {
    unsigned long now = millis();

    if (now - last_button_millis > 100) {
      Serial.print("time ");
      Serial.print(millis());
      Serial.println(" button state changed");
      toggle_state();
      last_button_millis = now;
    }

    button_state_changed = 0;
  }
}

void NodeRelayWithButton::set_state(int state)
{
  digitalWrite(BUTTON_RELAY, state);
  mqtt_publish_bool("relay_state", state);
  relay_state = state;
  Serial.print("state change ");
  Serial.println(state);
}

void NodeRelayWithButton::toggle_state(void)
{
  Serial.println("Toggle state");
  set_state(1-relay_state);
}

