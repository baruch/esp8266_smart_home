#include <pgmspace.h>

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "DebugSerial.h"
#include <ESP8266WiFi.h>
#include <Esp.h>
#include "libraries/WiFiManager/WiFiManager.h"
#include "config.h"
#include "libraries/PubSubClient/PubSubClient.h"
#include <ESP8266httpUpdate.h>
#include "libraries/HTU21D/SparkFunHTU21D.h"
#include "libraries/ADS1X15/Adafruit_ADS1015.h"
#include "libraries/BME280/Adafruit_BME280.h"
#include "libraries/BMP180/BMP180.h"
#include "libraries/TSL2561/TSL2561.h"
#include "libraries/BH1750/BH1750.h"
#include <GDBStub.h>

#define CONFIG_FILE "/config.ini"
#define UPGRADE_PORT 24320
#define DISCOVER_PORT 24320
#define UPGRADE_PATH "/node_v1.bin"
#define VERSION "SHMVER-0.0.4"

bool shouldSaveConfig;
static unsigned lastDiscovery;

WiFiClient mqtt_client;
PubSubClient mqtt(mqtt_client);
char mqtt_upgrade_topic[40];

HTU21D htu21d;
Adafruit_BME280 bme280;
TSL2561 tsl2561;
BH1750 bh1750;
BMP180 bmp180;
Adafruit_ADS1015 ads1115;

void check_upgrade();

void print_str(const char *name, const char *val)
{
  Serial.print(name);
  Serial.print(": \"");
  Serial.print(val);
  Serial.println('"');
}

void print_hexdump_line(const char *buf, size_t buf_len)
{
  size_t i;

  for (i = 0; i < buf_len && i < 16; i++) {
    Serial.print(buf[i], HEX);
    Serial.print(' ');
  }
  Serial.print("   ");
  for (i = 0; i < buf_len && i < 16; i++) {
    char ch = buf[i];
    if (!isprint(ch))
      ch = '.';
    Serial.print(ch);
  }
  Serial.print('\n');
}

void print_hexdump(const char *buf, size_t buf_len)
{
  size_t i;

  for (i = 0; i < buf_len; i += 16)
    print_hexdump_line(buf+i, buf_len - i);
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
  strcpy(dns, cfg.getValueStr("dns"));
  strcpy(node_desc, cfg.getValueStr("desc"));

  print_str("IP", static_ip);
  print_str("GW", static_gw);
  print_str("NM", static_nm);
  print_str("DNS", dns);
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
  WiFiManagerParameter custom_dns("dns", "Domain Name Server", dns, 40);

  if (strlen(static_ip) > 0) {
    IPAddress _ip, _gw, _nm;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _nm.fromString(static_nm);

    wifiManager.setSTAStaticIPConfig(_ip, _gw, _nm);
  }

  wifiManager.setForceStaticQuestion(true);
  wifiManager.addParameter(&node_desc_param);
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
    IPAddress _ip, _gw, _sn;
    wifiManager.getSTAStaticIPConfig(_ip, _gw, _sn);
    _ip.toString().toCharArray(static_ip, sizeof(static_ip), 0);
    _gw.toString().toCharArray(static_gw, sizeof(static_gw), 0);
    _sn.toString().toCharArray(static_nm, sizeof(static_nm), 0);
        
    Serial.println("saving config");
    Config cfg(CONFIG_FILE);
    cfg.setValueStr("ip", static_ip);
    cfg.setValueStr("gw", static_gw);
    cfg.setValueStr("nm", static_nm);
    cfg.setValueStr("dns", custom_dns.getValue());
    cfg.setValueStr("desc", node_desc_param.getValue());
    cfg.writeFile();
    Serial.println("save done");

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

  int pktlen;
  int i;

  buf[0] = 'S';
  pktlen = 1;

  pktlen = discover_set_str(buf, pktlen, node_name);
  pktlen = discover_set_str(buf, pktlen, node_type);
  pktlen = discover_set_str(buf, pktlen, node_desc);
  pktlen = discover_set_str(buf, pktlen, VERSION);

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
  lastDiscovery = millis();
}

void conditional_discover(void)
{
  if (millis() - lastDiscovery > 60*60*1000)
    discover_server();
}

void mqtt_callback(char* topic, byte* payload, int len) {
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

#if defined(HTTP_UPDATE_START_CALLBACK)
void upgrade_starting(void) {
  Serial.println("Upgrade starting");
  mqtt.publish(mqtt_tmp_topic("online"), "upgrading", 1);
}
#endif

void check_upgrade() {
#if defined(HTTP_UPDATE_START_CALLBACK)
  ESPhttpUpdate.onStart(upgrade_starting);
#endif
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
      Serial.printf("Unknown code %d\n", ret);
      break;
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
      size_t len = f.read((uint8_t*)buf, sizeof(buf));
      f.close();
      Serial.println("FILE START");
      print_hexdump(buf, len);
      Serial.println("FILE END");
    }

  }
}

void loop() {
  read_serial_commands();
  conditional_discover();

  if (mqtt_connected()) {
    mqtt.loop();

    static long lastMsg = -60000; // Ensure the first time we are around we do the first report
    long now = millis();
    if (now - lastMsg > 60000) {
      char msg[20];
      lastMsg = now;

      {
        float humd = htu21d.readHumidity();
        float temp = htu21d.readTemperature();

        dtostrf(humd, 0, 1, msg);
        mqtt.publish(mqtt_tmp_topic("humidity"), msg);

        dtostrf(temp, 0, 1, msg);
        mqtt.publish(mqtt_tmp_topic("temperature"), msg);

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
