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

void config_save(void)
{
    Serial.println("saving config");
    Config cfg(CONFIG_FILE);
    cfg.setValueStr("ip", static_ip);
    cfg.setValueStr("gw", static_gw);
    cfg.setValueStr("nm", static_nm);
    cfg.setValueStr("dns", dns);
    cfg.setValueStr("desc", node_desc);
    cfg.writeFile();
    Serial.println("save done");

    delay(100);
    ESP.restart();
    delay(1000);
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

    strcpy(dns, custom_dns.getValue());
    strcpy(node_desc, node_desc_param.getValue());
    config_save();
    // config save will restart
  }
}

int discover_set_buf(char *buf, int start, const uint8_t *src, int src_len)
{
  buf[start++] = src_len;
  memcpy(buf + start, src, src_len);
  return start + src_len;
}

int discover_set_str(char *buf, int start, const char *src)
{
  const int len = strlen(src);
  return discover_set_buf(buf, start, (const uint8_t *)src, len);
}

int discover_set_int(char *buf, int start, int val)
{
  char out[16];

  itoa(val, out, 10);
  return discover_set_str(buf, start, out);
}

static int extract_ip(const char *buf, int buf_len, int start, char *ip_str)
{
  if (buf_len < start)
    return -1;

  if (buf[start] != 4) // It's an IP, it is always four characters
    return -1;

  if (buf_len < start + 5 - 1) // Ensure we have all data
    return -1;

  sprintf(ip_str, "%d.%d.%d.%d", buf[start+1], buf[start+2], buf[start+3], buf[start+4]);
  return 5;
}

static int extract_str(const char *buf, int buf_len, int start, char *out, int max_len)
{
  int str_len;
  if (buf_len < start)
    return -1;

  str_len = buf[start];
  if (str_len >= max_len) // Ensure we have space to store it
    return -1;

  if (buf_len < start + str_len) // Ensure we have all data
    return -1;

  memcpy(out, buf+start+1, str_len);
  out[str_len] = 0;
  return str_len+1;
}

void discover_server() {
  WiFiUDP udp;
  int res;
  const IPAddress INADDR_BCAST(255, 255, 255, 255);
  char buf[64];
  char reply[128];
  char new_desc[32];
  char new_ip[16];
  char new_gw[16];
  char new_nm[16];
  char new_dns[16];
  char new_node_type[10];
  int new_config = 0;

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
  pktlen = discover_set_int(buf, pktlen, node_type);
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

    res = udp.read(reply, sizeof(reply));
    if (res < 9) {
      Serial.printf("Not enough data in the reply (len=%d)\n", res);
      continue;
    }
    Serial.println("Packet:");
    print_hexdump(reply, res);

    if (reply[0] != 'R') {
      Serial.println("Invalid reply header");
      continue;
    }
    int cur_pos = 1;

    int ret = extract_ip(reply, res, 1, mqtt_server);
    if (ret < 0) {
      Serial.println("Invalid MQTT IP");
      continue;
    }
    print_str("MQTT IP", mqtt_server);
    cur_pos += ret;

    if (reply[cur_pos] != 2) {
      Serial.println("Invalid port length");
      continue;
    }

    mqtt_port = reply[7] | (reply[8] << 8);
    Serial.print("MQTT Port: ");
    Serial.println(mqtt_port);
    cur_pos += 3;

    if (res <= cur_pos)
      break;

    ret = extract_str(reply, res, cur_pos, new_desc, sizeof(new_desc));
    if (ret < 0) {
      Serial.println("Bad desc");
      continue;
    }
    cur_pos += ret;

    ret = extract_ip(reply, res, cur_pos, new_ip);
    if (ret < 0) {
      Serial.println("Bad IP");
      continue;
    }
    cur_pos += ret;
    
    ret  = extract_ip(reply, res, cur_pos, new_gw);
    if (ret < 0) {
      Serial.println("Bad GW");
      continue;
    }
    cur_pos += ret;
    
    ret  = extract_ip(reply, res, cur_pos, new_nm);
    if (ret < 0) {
      Serial.println("Bad NM");
      continue;
    }
    cur_pos += ret;

    ret  = extract_ip(reply, res, cur_pos, new_dns);
    if (ret < 0) {
      Serial.println("Bad DNS");
      continue;
    }
    cur_pos += ret;

    ret = extract_str(reply, res, cur_pos, new_node_type, sizeof(new_node_type));
    if (ret < 0) {
      Serial.println("Bad Node type");
      continue;
    }
    Serial.println("New config done");
    new_config = 1;
    break;
  }

  udp.stop();
  Serial.println("Discovery done");
  if (i < 5) {
    last_discovery = millis();
    Serial.print("New config ");
    Serial.println(new_config);

    if (new_config) {
      if (strcmp(new_desc, node_desc) != 0 ||
         strcmp(new_ip, static_ip) != 0 ||
         strcmp(new_gw, static_gw) != 0 ||
         strcmp(new_nm, static_nm) != 0 ||
         strcmp(new_dns, dns) != 0)
      {
        Serial.println("Reconfigure");
        strcpy(node_desc, new_desc);
        strcpy(static_ip, new_ip);
        strcpy(static_gw, new_gw);
        strcpy(static_nm, new_nm);
        strcpy(dns, new_dns);

        config_save();
      }

      if (node_type == 0) {
        node_type = atoi(new_node_type);
        if (node_type > 0)
          node_type_save();
      }
    }

  } else {
    Serial.println("Discovery problem");
  }
}

void conditional_discover(void)
{
  if (millis() - last_discovery > DISCOVERY_CYCLES)
    discover_server();
}

