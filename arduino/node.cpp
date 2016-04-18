#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"
#include "node_relaywbutton.h"
#include "node_soilmoisture.h"

static Node *node;

void node_setup(void)
{
  node = NULL;
  switch (node_type) {
    case 1: node = new NodeHTU21D(); break;
    case 2: node = new NodeRelayWithButton(); break;
    case 3: node = new NodeSoilMoisture(); break;
    default: debug.log("Unknown node type ", node_type); break;
  }

  if (node)
    node->setup();
}

unsigned node_loop(void)
{
  if (node) {
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
