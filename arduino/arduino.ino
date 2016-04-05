#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "DebugSerial.h"
#include <ESP8266WiFi.h>
#include <Esp.h>
#include "config.h"
#include "common.h"
#include "node_mqtt.h"
//#include <GDBStub.h>


void spiffs_mount() {
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFs not initialized, reinitializing");
    if (!SPIFFS.format()) {
      Serial.println("SPIFFs format failed.");
      while (1);
    }
  }
}

void node_type_load(void) {
  node_type = 0;
  File f = SPIFFS.open(NODE_TYPE_FILENAME, "r");
  size_t size = f.size();
  if (size > sizeof(node_type)) {
    Serial.println("Trimming node type file");
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
  uint32_t t1 = ESP.getCycleCount();
  node_type = 0;
  node_desc[0] = 0;
  build_name();

  Serial.begin(115200);
#ifdef DEBUG
  delay(10000); // Wait for port to open, debug only
#endif

  Serial.println('*');
  Serial.println(VERSION);
  Serial.println(ESP.getResetInfo());
  Serial.println(ESP.getResetReason());

  spiffs_mount();
  node_type_load();
  node_setup();

  config_load();
  net_config();
  discover_server();
  check_upgrade();
  mqtt_setup();
  uint32_t t2 = ESP.getCycleCount();

  Serial.print("Setup done in ");
  Serial.print(clockCyclesToMicroseconds(t2 - t1));
  Serial.println(" micros");
}

void read_configure_type(void)
{
  Serial.println("Configure node type");
  int new_node_type = Serial.parseInt();
  Serial.print("Node type set to ");
  Serial.println(new_node_type);

  if (node_type != new_node_type) {
    Serial.println("Node type changed, saving it");
    node_type = new_node_type;
    node_type_save();
  }
}

void read_serial_commands() {
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 't') {
      read_configure_type();
    } else if (ch == 'T') {
      Serial.print("Node type config: ");
      Serial.println(node_type);
    } else if (ch == 'f') {
      Serial.println("Clearing Config");
      WiFi.disconnect(true);
      SPIFFS.remove(CONFIG_FILE);
      Serial.println("Clearing done");
      restart();
    } else if (ch == 'r') {
      Serial.println("Reset");
      restart();
    } else if (ch == 'u') {
      check_upgrade();
    } else if (ch == 'p') {
      char buf[1024];
      File f = SPIFFS.open(CONFIG_FILE, "r");
      Serial.print("File size is ");
      Serial.println(f.size());
      if (f.size()) {
        size_t len = f.read((uint8_t*)buf, sizeof(buf));
        f.close();
        Serial.println("FILE START");
        print_hexdump(buf, len);
        Serial.println("FILE END");
      } else {
        Serial.println("Empty or non-existent file");
      }
    }

  }
}

void loop() {
  read_serial_commands();
  conditional_discover();
  mqtt_loop();

  unsigned sleep_interval = node_loop();
  if (sleep_interval) {
    Serial.print("Going to sleep for ");
    Serial.print(sleep_interval);
    Serial.println(" seconds");
    delay(1);
    mqtt_loop();
    delay(1);
    WiFiClient::stopAll();
    ESP.deepSleep(sleep_interval * 1000 * 1000);
  }
}
