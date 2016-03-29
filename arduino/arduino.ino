#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "DebugSerial.h"
#include <ESP8266WiFi.h>
#include <Esp.h>
#include "config.h"
#include "common.h"
#include "node_mqtt.h"
#include "libraries/HTU21D/SparkFunHTU21D.h"
#include <GDBStub.h>

HTU21D htu21d;


void spiffs_mount() {
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFs not initialized, reinitializing");
    if (!SPIFFS.format()) {
      Serial.println("SPIFFs format failed.");
      while (1);
    }
  }
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
  node_type[0] = 0;
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
  config_load();
  net_config();
  discover_server();
  check_upgrade();
  mqtt_setup();
  uint32_t t2 = ESP.getCycleCount();

  htu21d.begin();

  // Load node params
  // Load wifi config
  // Button reset check
  Serial.print("Setup done in ");
  Serial.print(clockCyclesToMicroseconds(t2 - t1));
  Serial.println(" micros");
}

void read_serial_commands() {
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'f') {
      Serial.println("Clearing Config");
      WiFi.disconnect(true);
      SPIFFS.remove(CONFIG_FILE);
      Serial.println("Clearing done");
      delay(100);
      ESP.restart();
    } else if (ch == 'r') {
      Serial.println("Reset");
      delay(100);
      ESP.restart();
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

  if (mqtt_connected()) {
    mqtt_loop();

    static long lastMsg = -60000; // Ensure the first time we are around we do the first report
    long now = millis();
    if (now - lastMsg > 60000) {
      lastMsg = now;

      {
        float humd = htu21d.readHumidity();
        float temp = htu21d.readTemperature();

        mqtt_publish_float("humidity", humd);
        mqtt_publish_float("temperature", temp);

        Serial.print("Time:");
        Serial.print(millis());
        Serial.print(" Temperature:");
        Serial.print(temp, 1);
        Serial.print("C");
        Serial.print(" Humidity:");
        Serial.print(humd, 1);
        Serial.println("%");
      }
    }
  }
}
