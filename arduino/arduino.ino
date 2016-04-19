#include <Arduino.h>

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "libraries/WiFiAsyncManager/WiFiAsyncManager.h"
#include <Esp.h>
#include "config.h"
#include "common.h"
#include "globals.h"
#include "node_mqtt.h"
//#include <GDBStub.h>


void spiffs_mount() {
  if (!SPIFFS.begin()) {
    debug.log("SPIFFs not initialized, reinitializing");
    if (!SPIFFS.format()) {
      debug.log("SPIFFs format failed.");
      while (1);
    }
  }
  debug.log("SPIFFS initialized");
}

void node_type_load(void) {
  node_type = 0;
  File f = SPIFFS.open(NODE_TYPE_FILENAME, "r");
  size_t size = f.size();
  if (size > sizeof(node_type)) {
    debug.log("Trimming node type file");
    size = sizeof(node_type);
  }
  if (size > 0)
    f.read((uint8_t*)&node_type, size);
  f.close();
}

void node_type_save(void) {
  File f = SPIFFS.open(NODE_TYPE_FILENAME, "w");
  f.write((uint8_t*)&node_type, sizeof(node_type));
  f.close();

  restart();
}

void build_name() {
  uint32_t id;
  int i;
  int len = 0;

  node_name[len++] = 'S';
  node_name[len++] = 'H';
  node_name[len++] = 'M';

  id = ESP.getChipId();
  for (i = 0; i < 8; i++, id >>= 4) {
    node_name[len++] = nibbleToChar(id);
  }
  id = ESP.getFlashChipId();
  for (i = 0; i < 8; i++, id >>= 4) {
    node_name[len++] = nibbleToChar(id);
  }
  node_name[len] = 0;
}

void setup() {
  unsigned long t1 = millis();
  node_type = 0;
  node_desc[0] = 0;
  build_name();

  debug.begin();
  Serial.begin(115200);

  debug.log('*');
  debug.log(VERSION);
  debug.log(ESP.getResetInfo());
  debug.log(ESP.getResetReason());

  spiffs_mount();
  node_type_load();
  node_setup();

  config_load();
  net_config_setup();
  mqtt_setup();
  unsigned long t2 = millis();

  debug.log("Setup done in ", t2-t1, " millis");
}

void read_configure_type(void)
{
  int new_node_type = Serial.parseInt();
  debug.log("Node type set to ", new_node_type);

  if (node_type != new_node_type) {
    debug.log("Node type changed, saving it");
    node_type = new_node_type;
    node_type_save();
  } else {
    debug.log("Node type not changed");
  }
}

void read_serial_commands() {
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 't') {
      read_configure_type();
    } else if (ch == 'T') {
      debug.log("Node type config: ", node_type);
    } else if (ch == 'f') {
      debug.log("Clearing Config");
      WiFi.persistent(true);
      WiFi.disconnect(true);
      SPIFFS.remove(CONFIG_FILE);
      debug.log("Clearing done");
      restart();
    } else if (ch == 'r') {
      debug.log("Reset");
      restart();
    } else if (ch == 'u') {
      check_upgrade();
    } else if (ch == 'p') {
      char buf[1024];
      File f = SPIFFS.open(CONFIG_FILE, "r");
      debug.log("File size is ", f.size());
      if (f.size()) {
        size_t len = f.read((uint8_t*)buf, sizeof(buf));
        f.close();
        debug.log("FILE START");
        print_hexdump(buf, len);
        debug.log("FILE END");
      } else {
        debug.log("Empty or non-existent file");
      }
    }

  }
}

void loop() {
  read_serial_commands();
  net_config_loop();
  discover_poll();
  mqtt_loop();

  unsigned sleep_interval = node_loop();
  if (sleep_interval) {
    deep_sleep(sleep_interval);
  }

  if (node_is_powered() && millis() > 10*1000) {
    debug.log("Timed out connecting to wifi and we are battery powered, going to sleep");
    deep_sleep(15*60);
  }
}
