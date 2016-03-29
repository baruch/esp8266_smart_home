#include "common.h"
#include "globals.h"
#include "config.h"

#include <ESP8266WiFi.h>
#include <Esp.h>
#include <ESP8266httpUpdate.h>

#include "DebugSerial.h"

#include "libraries/WiFiManager/WiFiManager.h"

#define DISCOVERY_CYCLES (60*60*1000) // an hour in msecs

static bool shouldSaveConfig;
static unsigned last_discovery;

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
    last_discovery = -DISCOVERY_CYCLES;
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
    last_discovery = DISCOVERY_CYCLES;
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
  last_discovery = millis();
}

void conditional_discover(void)
{
  if (millis() - last_discovery > DISCOVERY_CYCLES)
    discover_server();
}

