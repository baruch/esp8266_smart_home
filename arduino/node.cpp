#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"
#include "node_relaywbutton.h"
#include "node_soilmoisture.h"
#include "node_sewagepump.h"
#include <ESP8266WiFi.h>

#define RSSI_DIFF 4
#define RSSI_POLL_INTERVAL 60000

static Node *node;

void node_setup(void)
{
  node = NULL;
  switch (node_type) {
    case 1: node = new NodeHTU21D(); break;
    case 2: node = new NodeRelayWithButton(); break;
    case 3: node = new NodeSoilMoisture(); break;
    case 5: node = new NodeSewagePump(); break;
    default: debug.log("Unknown node type ", node_type); break;
  }

  if (node) {
    node->setup();
    if (!node->is_battery_powered()) {
      sleep_lock();
    }
  }
}

unsigned node_loop(void)
{
  if (node) {
    node->loop_for_type();
    return node->loop();
  } else {
    return 0;
  }
}

void node_mqtt_connected(void)
{
  if (node)
    node->mqtt_connected_event();
}

bool node_is_powered(void)
{
  return node && node->is_battery_powered();
}

void NodeActuator::loop_for_type(void)
{
  if (TIME_PASSED(m_next_rssi_poll)) {
    m_next_rssi_poll = millis() + RSSI_POLL_INTERVAL;

    int32_t rssi = WiFi.RSSI();
    if (m_last_rssi > rssi + RSSI_DIFF || m_last_rssi < rssi - RSSI_DIFF) {
      mqtt_publish_int("rssi", rssi);
      m_last_rssi = rssi;
    }
  }
}
