#include "DebugPrint.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>

#define LOG_PORT 24321

DebugPrint debug;

void DebugPrint::begin(void)
{
        buf_start = 0;
        buf_end = 0;
        server_ip = INADDR_NONE;
        last_connect_time = 0;
}

void DebugPrint::stop(void)
{
  debug.log("Disconnecting");
  delay(0);
  client.abort();
}

void DebugPrint::set_log_server(IPAddress const &server)
{
        // If we set the same value, do nothing
        if (server == server_ip || server == INADDR_NONE)
                return;

        debug.log("Setting a new log server");
        if (client.connected() || client.connecting()) {
          debug.log("disconnecting from old log server");
          client.stop();
          last_connect_time = 0;
        }

        server_ip = server;

        reconnect();
}

bool DebugPrint::reconnect(void)
{
        if (!WiFi.isConnected())
          return false;

        bool connected = client.connected();

        if (!connected && server_ip) {
          unsigned long now = millis();
          if (last_connect_time == 0 || now - last_connect_time > 1000) {
            client.connect(server_ip, LOG_PORT);
            last_connect_time = now;
          }
        }
        return connected;
}

size_t DebugPrint::write(uint8_t ch)
{
        buf[buf_end++] = ch;
        if (buf_end >= sizeof(buf))
                buf_end = 0;

        size_t ret = Serial.write(ch);
        if (ch == '\n') {
                // End of line, try to send to server
                if (!reconnect())
                        return ret;

                if (!client.canSend())
                  return ret;

                // We are connected
                if (buf_end < buf_start) {
                        client.write((const char *)buf+buf_start, sizeof(buf) - buf_start);
                        buf_start = 0;
                }
                if (buf_start != buf_end) {
                        client.write((const char *)buf+buf_start, buf_end-buf_start);
                        buf_start = buf_end;
                }
        }
        return ret;
}
