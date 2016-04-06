#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"
#include "node_relaywbutton.h"
#include "node_soilmoisture.h"
#include "DebugSerial.h"

static Node *node;

void node_setup(void)
{
  node = NULL;
  switch (node_type) {
    case 1: node = new NodeHTU21D(); break;
    case 2: node = new NodeRelayWithButton(); break;
    case 3: node = new NodeSoilMoisture(); break;
    default: Serial.print("Unknown node type "); Serial.println(node_type); break;
  }

  if (node)
    node->setup();
}

unsigned node_loop(void)
{
  unsigned ret = 0;
  if (node && (node->loop_only_if_connected() == false || mqtt_connected())) {
    ret = node->loop();
  }
  return ret;
}

void node_mqtt_connected(void)
{
  if (node)
    node->mqtt_connected_event();
}

bool node_is_powered(void)
{
  // For now shortcut with using loop_only_if_connected() since they are the same for all current nodes
  if (node && node->loop_only_if_connected() == false)
    return true;

  return false;
}

