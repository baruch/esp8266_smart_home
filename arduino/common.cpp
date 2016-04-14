#include "common.h"
#include <Arduino.h>
#include "node_mqtt.h"
#include <ESP8266WiFi.h>

void print_str(const char *name, const char *val)
{
  Serial.print(name);
  Serial.print(": \"");
  Serial.print(val);
  Serial.println('"');
}

static void print_hexdump_line(const char *buf, size_t buf_len)
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
    print_hexdump_line(buf + i, buf_len - i);
}

char nibbleToChar(uint32_t val)
{
  val &= 0xF;
  if (val < 10)
    return '0' + val;
  else
    return 'A' + val - 10;
}

void restart(void)
{
  // We want to send any message pending
  delay(50);

  // Next we want to disconnect and let the disconnect propogate to the clients
  mqtt_disconnect();
  delay(50);

  // Now we reset
  ESP.restart();

  // It was shown that after a restart we need to delay for the restart to work reliably
  delay(1000);
}

void deep_sleep(unsigned seconds)
{
  Serial.print("Going to sleep for ");
  Serial.print(seconds);
  Serial.println(" seconds");
  delay(1);
  mqtt_loop();
  delay(1);
  WiFiClient::stopAll();
  delay(1);
  ESP.deepSleep(seconds * 1000 * 1000);
}

