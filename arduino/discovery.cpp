#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include "common.h"
#include "globals.h"
#include "node_mqtt.h"

#define DISCOVERY_CYCLES (60*60*1000) // an hour in msecs
static long unsigned next_discovery = 0;
static WiFiUDP udp;
static unsigned long next_reply = 0;
static bool first_successful_discovery = true;

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

static int extract_ip(const char *buf, int buf_len, int start, IPAddress &ip_out)
{
  if (buf_len < start)
    return -1;

  if (buf[start] != 4) // It's an IP, it is always four characters
    return -1;

  if (buf_len < start + 5 - 1) // Ensure we have all data
    return -1;

  ip_out = IPAddress(buf[start+1], buf[start+2], buf[start+3], buf[start+4]);
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

static bool discover_send_pkt()
{
  char buf[64];
  int res;
  int pktlen;

  debug.log("Sending discovery packet");
  udp.begin(DISCOVER_PORT);

  IPAddress broadcast_ip = ~WiFi.subnetMask() | WiFi.gatewayIP();
  res = udp.beginPacket(broadcast_ip, DISCOVER_PORT);
  if (res != 1) {
    debug.log("Failed to prepare udp packet for send");
    return false;
  }

  buf[0] = 'S';
  pktlen = 1;

  pktlen = discover_set_str(buf, pktlen, node_name);
  pktlen = discover_set_int(buf, pktlen, node_type);
  pktlen = discover_set_str(buf, pktlen, node_desc);
  pktlen = discover_set_str(buf, pktlen, VERSION);

  udp.write(buf, pktlen);

  res = udp.endPacket();
  if (res != 1) {
    debug.log("Failed to send udp discover packet");
    return false;
  }

  return true;
}

static bool parse_packet(const char *reply, int reply_len)
{
  char new_desc[32];
  IPAddress new_ip;
  IPAddress new_gw;
  IPAddress new_nm;
  IPAddress new_dns;
  char new_node_type[10];
  IPAddress new_mqtt_server;
  IPAddress new_log_server_ip;
  int new_mqtt_port;

  if (reply[0] != 'R') {
    debug.log("Invalid reply header, got ", (int)reply[0]);
    return false;
  }
  int cur_pos = 1;

  int ret = extract_ip(reply, reply_len, 1, new_mqtt_server);
  if (ret < 0) {
    debug.log("Invalid MQTT IP");
    return false;
  }
  debug.log("MQTT IP: '", new_mqtt_server, '\'');
  cur_pos += ret;

  if (reply[cur_pos] != 2) {
    debug.log("Invalid port length ", (int)reply[cur_pos], " pos ", cur_pos);
    return false;
  }

  new_mqtt_port = reply[7] | (reply[8] << 8);
  debug.log("MQTT Port: ", new_mqtt_port);
  cur_pos += 3;

  if (reply_len <= cur_pos)
    return false;

  ret = extract_str(reply, reply_len, cur_pos, new_desc, sizeof(new_desc));
  if (ret < 0) {
    debug.log("Bad desc");
    return false;
  }
  cur_pos += ret;

  ret = extract_ip(reply, reply_len, cur_pos, new_ip);
  if (ret < 0) {
    debug.log("Bad IP");
    return false;
  }
  cur_pos += ret;

  ret  = extract_ip(reply, reply_len, cur_pos, new_gw);
  if (ret < 0) {
    debug.log("Bad GW");
    return false;
  }
  cur_pos += ret;

  ret  = extract_ip(reply, reply_len, cur_pos, new_nm);
  if (ret < 0) {
    debug.log("Bad NM");
    return false;
  }
  cur_pos += ret;

  ret  = extract_ip(reply, reply_len, cur_pos, new_dns);
  if (ret < 0) {
    debug.log("Bad DNS");
    return false;
  }
  cur_pos += ret;

  ret = extract_str(reply, reply_len, cur_pos, new_node_type, sizeof(new_node_type));
  if (ret < 0) {
    debug.log("Bad Node type");
    return false;
  }
  cur_pos += ret;

  ret = extract_ip(reply, reply_len, cur_pos, new_log_server_ip);
  if (ret < 0) {
    debug.log("No log server IP");
  }
  cur_pos += ret;

  debug.log("New config done");

  if (new_log_server_ip) {
    debug.log("Log server IP ", new_log_server_ip);
    debug.set_log_server(new_log_server_ip);
  }

  mqtt_update_server(new_mqtt_server, new_mqtt_port);

  debug.log("desc '", node_desc, "' '", new_desc, '\'');
  if (/*strcmp(new_desc, node_desc) != 0 ||*/
      new_ip != static_ip ||
      new_gw != static_gw ||
      new_nm != static_nm ||
      new_dns != dns)
  {
    debug.log("Reconfigure");
    strcpy(node_desc, new_desc);
    static_ip = new_ip;
    static_gw = new_gw;
    static_nm = new_nm;
    dns = new_dns;

    config_save();
  }

  if (node_type == 0) {
    node_type = atoi(new_node_type);
    if (node_type > 0)
      node_type_save();
  }

  return true;
}

static bool check_reply() {
  int res;
  char reply[128];

  res = udp.parsePacket();

  if (res > 0) {
    res = udp.read(reply, sizeof(reply));
    debug.log("Packet:");
    print_hexdump(reply, res);

    if (res >= 9 && parse_packet(reply, res)) {
      debug.log("Discovery done");
      return true;
    } else {
      debug.log("Failed to parse packet");
    }
  }

  return false;
}

void discover_poll(void)
{
  if (!WiFi.isConnected()) {
    if (next_reply) {
      udp.stop();
      next_reply = 0;
    }
  } else {
    if (next_reply) {
      // We sent a packet and wait for a reply
      if (check_reply()) {
        // Reply received and parsed ok
        next_reply = 0;
        next_discovery = millis() + DISCOVERY_CYCLES;
        udp.stop();
        if (first_successful_discovery) {
          first_successful_discovery = false;
        }
      } else if (TIME_PASSED(next_reply)) {
        // Reply timed out
        debug.log("Packet receipt timed out");
        next_reply = 0;
        next_discovery = millis() + 10 * 1000; // If we fail, try again in 10 seconds
        udp.stop();
      }
    } else if (TIME_PASSED(next_discovery)) {
      if (discover_send_pkt()) {
        debug.log("Packet sent");
        next_reply = millis() + 250;
        if (next_reply == 0)
          next_reply = 1;
      }
    }
  }
}

void discovery_now(void)
{
  next_discovery = 0;
}
