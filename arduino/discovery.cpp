#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include "common.h"
#include "globals.h"

#define DISCOVERY_CYCLES (60*60*1000) // an hour in msecs
static long unsigned next_discovery;


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

  sprintf(ip_str, "%d.%d.%d.%d", buf[start + 1], buf[start + 2], buf[start + 3], buf[start + 4]);
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

  memcpy(out, buf + start + 1, str_len);
  out[str_len] = 0;
  return str_len + 1;
}


void discover_server() {
  WiFiUDP udp;
  int res;
  char buf[64];
  char reply[128];
  char new_desc[32];
  char new_ip[16];
  char new_gw[16];
  char new_nm[16];
  char new_dns[16];
  char new_node_type[10];
  int new_config = 0;

  next_discovery = millis() + 10 * 1000; // If we fail, try again in 10 seconds
  udp.begin(DISCOVER_PORT);

  IPAddress broadcast_ip = ~WiFi.subnetMask() | WiFi.gatewayIP();
  res = udp.beginPacket(broadcast_ip, DISCOVER_PORT);
  if (res != 1) {
    Serial.println("Failed to prepare udp packet for send");
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
    return;
  }

  for (i = 0, res = -1; i < 5; i ++) {
    Serial.println("Packet sent");
    int wait;
    for (wait = 0; res <= 0 && wait < 250; wait++) {
      delay(1);
      res = udp.parsePacket();
    }

    if (res <= 0) {
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
    next_discovery = millis() + DISCOVERY_CYCLES;
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
  if (!WiFi.isConnected())
    return;

  if (millis() >= next_discovery)
    discover_server();
}

void discovery_now(void)
{
  next_discovery = millis();
}

