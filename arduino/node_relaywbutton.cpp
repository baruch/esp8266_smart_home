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
  set_state(0);
  attachInterrupt(BUTTON_PIN, pin_interrupt, RISING);
}

void NodeRelayWithButton::loop(void)
{
  if (button_state_changed) {
    Serial.println("button state changed");
    toggle_state();
    button_state_changed = 0;
  }

  int state = digitalRead(BUTTON_PIN);
  static int last_state = 1;
  if (state != last_state) {
    Serial.println("poll changed");
    Serial.println(state);
    Serial.print(last_state);
    last_state = state;
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

