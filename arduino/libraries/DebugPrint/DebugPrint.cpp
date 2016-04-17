#include "DebugPrint.h"
#include <Arduino.h>

#define LOG_PORT 24321

DebugPrint debug;

void DebugPrint::begin(void)
{
        buf_start = 0;
        buf_end = 0;
        server_name[0] = 0;
}

void DebugPrint::set_log_server(const char *server)
{
        // If we set the same value, do nothing
        if (strcmp(server_name, server) == 0)
                return;

        if (client.connected())
                client.stop();

        strncpy(server_name, server, sizeof(server_name));
        server_name[sizeof(server_name)-1] = 0;

        reconnect();
}

bool DebugPrint::reconnect(void)
{
        bool connected = client.connected();

        if (!connected && server_name[0]) {
                connected = client.connect(server_name, LOG_PORT);
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
