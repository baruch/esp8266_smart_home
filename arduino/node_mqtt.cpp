#include "node_mqtt.h"
#include "libraries/PubSubClient/PubSubClient.h"
#include "WiFiClient.h"
#include <ESP8266WiFi.h>
#include "DebugSerial.h"
#include "globals.h"
#include "common.h"

static WiFiClient mqtt_client;
static PubSubClient mqtt(mqtt_client);
static char mqtt_upgrade_topic[40];

static void mqtt_callback(char* topic, byte* payload, int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, mqtt_upgrade_topic) == 0) {
    if (strncmp((const char*)payload, VERSION, len) != 0) {
      check_upgrade();
    } else {
      Serial.println("Asking to upgrade to our version, not doing anything");
    }
  }
}

void mqtt_topic(char *buf, int buf_len, const char *name)
{
  snprintf(buf, buf_len, "shm/%s/%s", node_name, name);
}

/* This is useful for a one-off publish, it uses only one ram location for any number of one-off publishes */
const char * mqtt_tmp_topic(const char *name)
{
  static char buf[40];

  mqtt_topic(buf, sizeof(buf), name);
  return buf;
}

void mqtt_setup() {
  mqtt_topic(mqtt_upgrade_topic, sizeof(mqtt_upgrade_topic), "upgrade");
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqtt_callback);
}

void mqtt_disconnect(void)
{
  mqtt.disconnect();
}

bool mqtt_connected() {
  return mqtt.connected();
}


void mqtt_loop(void)
{
  if (!WiFi.isConnected())
    return;

  if (mqtt.connected()) {
    mqtt.loop();
    return;
  }

  long now = millis();
  static long lastReconnectAttempt = -2000;
  if (now - lastReconnectAttempt > 2000) {
    lastReconnectAttempt = now;
    const char *will_topic = mqtt_tmp_topic("online");
    if (mqtt.connect(node_name, will_topic, 0, 1, "offline")) {
      Serial.print("MQTT connected in ");
      Serial.print(millis() - lastReconnectAttempt);
      Serial.println(" msec");

      // Once connected, publish an announcement...
      mqtt.publish(will_topic, "online", 1);
      // ... and resubscribe
      mqtt.subscribe(mqtt_upgrade_topic);
      node_mqtt_connected();
    } else if (now - lastReconnectAttempt > 15000) {
      Serial.println("Failed to connect to MQTT");
      if (node_is_powered()) {
        restart();
      } else {
        // Node not powered, need to save energy
        deep_sleep(DEFAULT_DEEP_SLEEP_TIME);
      }
    }
  }
}

void mqtt_publish_str(const char *name, const char *val)
{
  mqtt.publish(mqtt_tmp_topic(name), val, 1);
}

void mqtt_publish_float(const char *name, float val)
{
  char msg[20];
  dtostrf(val, 0, 4, msg);
  mqtt_publish_str(name, msg);
}

void mqtt_publish_bool(const char *name, bool val)
{
  mqtt_publish_str(name, val ? "1" : "0");
}

