#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"
#include "DebugSerial.h"

static Node *node;

void node_setup(void)
{
  node = NULL;
  switch (node_type) {
    case 1: node = new NodeHTU21D(); break;
  }

  if (node)
    node->setup();
}

void node_loop(void)
{
  if (node)
    node->loop();
}

