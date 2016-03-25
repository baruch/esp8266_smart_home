#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <Esp.h>
#include "WiFiManager/WiFiManager.h"
#include "config.h"
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <GDBStub.h>

#define CONFIG_FILE "/config.ini"
#define UPGRADE_PORT 24320
#define UPGRADE_PATH "/node_v1.bin"
#define VERSION "SHMVER-0.0.1"

const int DISCOVER_PORT = 24320;

char node_name[20];
char node_type[32];
char node_desc[32];
char mqtt_server[40];
int mqtt_port;
char static_ip[16] = "";
char static_nm[16] = "";
char static_gw[16] = "";
char dns[40] = "";
bool shouldSaveConfig;

WiFiClient mqtt_client;
PubSubClient mqtt(mqtt_client);
char mqtt_upgrade_topic[32];

void check_upgrade();

void print_str(const char *name, const char *val)
{
  Serial.print(name);
  Serial.print(": \"");
  Serial.print(val);
  Serial.println('"');
}

char nibbleToChar(uint32_t val)
{
  val &= 0xF;
  if (val < 10)
    return '0' + val;
  else
    return 'A' + val - 10;
}

void spiffs_mount() {
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFs not initialized, reinitializing");
    if (!SPIFFS.format()) {
      Serial.println("SPIFFs format failed.");
      while (1);
    }
  }
}

void config_load() {
  Serial.println("SPIFFS initialized");
  Config cfg(CONFIG_FILE);

  Serial.println("Loading config");
  cfg.readFile();
  Serial.println("Config loaded");
  strcpy(static_ip, cfg.getValueStr("ip"));
  strcpy(static_gw, cfg.getValueStr("gw"));
  strcpy(static_nm, cfg.getValueStr("nm"));
  strcpy(static_nm, cfg.getValueStr("dns"));
  strcpy(node_desc, cfg.getValueStr("desc"));

  print_str("IP", static_ip);
  print_str("GW", static_gw);
  print_str("NM", static_nm);
  print_str("DNS", static_nm);
  print_str("Node Desc", node_desc);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void net_config() {
  WiFiManager wifiManager;

  WiFiManagerParameter node_desc_param("desc", "Description", node_desc, 32);
  WiFiManagerParameter custom_ip("ip", "Static IP", static_ip, 40);
  WiFiManagerParameter custom_gw("gw", "Static Gateway", static_gw, 5);
  WiFiManagerParameter custom_nm("nm", "Static Netmask", static_nm, 5);
  WiFiManagerParameter custom_dns("dns", "Domain Name Server", dns, 40);

  if (strlen(static_ip) > 0) {
    IPAddress _ip, _gw, _nm;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _nm.fromString(static_nm);

    wifiManager.setSTAStaticIPConfig(_ip, _gw, _nm);
  }

  wifiManager.addParameter(&node_desc_param);
  wifiManager.addParameter(&custom_ip);
  wifiManager.addParameter(&custom_nm);
  wifiManager.addParameter(&custom_gw);
  wifiManager.addParameter(&custom_dns);

  //wifiManager.setTimeout(600);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  if (!wifiManager.autoConnect()) {
    // Not connected
    Serial.println("Failed to connect to AP");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Connected");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());


  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    Config cfg(CONFIG_FILE);
    cfg.setValueStr("desc", node_desc_param.getValue());
    cfg.setValueStr("ip", custom_ip.getValue());
    cfg.setValueStr("gw", custom_gw.getValue());
    cfg.setValueStr("nm", custom_nm.getValue());
    cfg.setValueStr("dns", custom_dns.getValue());
    cfg.writeFile();

    delay(100);
    ESP.restart();
    delay(1000);
  }
}

int discover_set_str(char *buf, int start, const char *src)
{
  const int len = strlen(src);
  buf[start++] = len;
  memcpy(buf + start, src, len);
  return start + len;
}

void discover_server() {
  WiFiUDP udp;
  int res;
  const IPAddress INADDR_BCAST(255, 255, 255, 255);
  char buf[64];

  udp.begin(DISCOVER_PORT);
  res = udp.beginPacket(INADDR_BCAST, DISCOVER_PORT);
  if (res != 1) {
    Serial.println("Failed to prepare udp packet for send");
    return;
  }

  uint32_t id;
  int pktlen;
  int i;

  buf[0] = 'S';
  pktlen = 1;

  pktlen = discover_set_str(buf, pktlen, node_name);
  pktlen = discover_set_str(buf, pktlen, node_type);
  pktlen = discover_set_str(buf, pktlen, node_desc);

  udp.write(buf, pktlen);

  res = udp.endPacket();
  if (res != 1) {
    Serial.println("Failed to send udp discover packet");
    return;
  }

  for (i = 0; i < 5; i ++) {
    Serial.println("Packet sent");
    int wait;
    for (wait = 0, res = -1; res == -1 && wait < 250; wait++) {
      delay(1);
      res = udp.parsePacket();
    }

    if (res == -1) {
      Serial.println("Packet reply timed out, retrying");
      continue;
    }

    Serial.print("Pkt reply took ");
    Serial.print(wait);
    Serial.println(" msec");

    char reply[32];
    res = udp.read(reply, sizeof(reply));
    if (res < 9) {
      Serial.printf("Not enough data in the reply (len=%d)\n", res);
      continue;
    }
    Serial.print("Packet:");
    for (i = 0; i < res; i++) {
      Serial.print(' ');
      Serial.print(reply[i], HEX);
    }
    Serial.print('\n');

    if (reply[0] != 'R') {
      Serial.println("Invalid reply header");
      continue;
    }

    if (reply[1] != 4) {
      Serial.println("Invalid ip length");
      continue;
    }

    sprintf(mqtt_server, "%d.%d.%d.%d", reply[2], reply[3], reply[4], reply[5]);
    print_str("MQTT IP", mqtt_server);

    if (reply[6] != 2) {
      Serial.println("Invalid port length");
      continue;
    }

    mqtt_port = reply[7] | (reply[8] << 8);
    Serial.print("MQTT Port: ");
    Serial.println(mqtt_port);
  }

  udp.stop();
  Serial.println("Discovery done");
}

void mqtt_callback(char* topic, byte* payload, unsigned int len) {
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
  static char buf[32];

  mqtt_topic(buf, sizeof(buf), name);
  return buf;
}

void mqtt_setup() {
  mqtt_topic(mqtt_upgrade_topic, sizeof(mqtt_upgrade_topic), "upgrade");
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqtt_callback);
}

bool mqtt_connected() {
  if (mqtt.connected())
    return true;

  long now = millis();
  static long lastReconnectAttempt = 0;
  if (now - lastReconnectAttempt > 5000) {
    lastReconnectAttempt = now;
    Serial.println("Attempting MQTT connection...");
    const char *will_topic = mqtt_tmp_topic("online");
    if (mqtt.connect(node_name, will_topic, 0, 1, "offline")) {
      Serial.println("MQTT connected");
      // Once connected, publish an announcement...
      mqtt.publish(will_topic, "online", 1);
      mqtt.publish(mqtt_tmp_topic("desc"), node_desc, 1);
      mqtt.publish(mqtt_tmp_topic("version"), VERSION, 1);
      mqtt.publish("outTopic", "hello world");
      // ... and resubscribe
      mqtt.subscribe(mqtt_upgrade_topic);
      return true;
    }
  }

  return false;
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

void check_upgrade() {
  Serial.println("Checking for upgrade");
  t_httpUpdate_return ret = ESPhttpUpdate.update(mqtt_server, UPGRADE_PORT, UPGRADE_PATH, VERSION);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;

    default:
      Serial.printf("Unknown code %d\n");
  }
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
    }
  }
}

void loop() {
  read_serial_commands();

  if (mqtt_connected()) {
    mqtt.loop();

    static long lastMsg = 0;
    static long value = 0;
    long now = millis();
    if (now - lastMsg > 15000) {
      char msg[20];
      lastMsg = now;
      ++value;
      snprintf (msg, 75, "hello world #%ld", value);
      Serial.print("Publish message: ");
      Serial.println(msg);
      mqtt.publish("outTopic", msg);
    }
  }
}
