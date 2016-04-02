#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"
#include "node_relaywbutton.h"
#include "DebugSerial.h"

static Node *node;

void node_setup(void)
{
  node = NULL;
  switch (node_type) {
    case 1: node = new NodeHTU21D(); break;
    case 2: node = new NodeRelayWithButton(); break;
  }

  if (node)
    node->setup();
}

void node_loop(void)
{
  if (node)
    node->loop();
}

void node_mqtt_connected(void)
{
  if (node)
    node->mqtt_connected_event();
}

