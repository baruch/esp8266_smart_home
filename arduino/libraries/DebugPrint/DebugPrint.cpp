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
}

void DebugPrint::set_log_server(IPAddress const &server)
{
        // If we set the same value, do nothing
        if (server == server_ip || server == INADDR_NONE)
                return;

        if (client.connected())
                client.stop();

        server_ip = server;

        reconnect();
}

bool DebugPrint::reconnect(void)
{
        if (!WiFi.isConnected())
          return false;

        bool connected = client.connected();

        if (!connected && server_ip) {
                connected = client.connect(server_ip, LOG_PORT);
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
                // Discard any data sent to us, we have no use for it
                client.flush();

                // End of line, try to send to server
                if (!client.connected() && !reconnect())
                        return ret;

                // We are connected
                if (buf_end < buf_start) {
                        client.write(buf+buf_start, sizeof(buf) - buf_start);
                        buf_start = 0;
                }
                if (buf_start != buf_end) {
                        client.write(buf+buf_start, buf_end-buf_start);
                        buf_start = buf_end;
                }
        }
        return ret;
}
