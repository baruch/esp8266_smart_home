#define NO_REDEF_SERIAL

#include "globals.h"
#include "DebugSerial.h"
#include "libraries/arduinoWebSockets/WebSocketsServer.h"
#include "HardwareSerial.h"

static WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %s url: %s\n", num, ip.toString().c_str(), payload);

        // send message to client
        webSocket.sendTXT(num, "Connected to " + String(node_name) + " at " + WiFi.localIP().toString() + "\n");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      // send message to client
      // webSocket.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocket.broadcastTXT("message here");
      break;
    case WStype_BIN:
      //Serial.printf("[%u] get binary lenght: %u\n", num, lenght);
      //hexdump(payload, lenght);

      // send message to client
      // webSocket.sendBIN(num, payload, lenght);
      break;
  }

}

DebugSerial::DebugSerial()
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void DebugSerial::begin(int speed)
{
  Serial.begin(speed);
}

bool DebugSerial::available(void)
{
  return Serial.available();
}

char DebugSerial::read(void)
{
  return Serial.read();
}

size_t DebugSerial::write(uint8_t ch)
{
  webSocket.broadcastTXT(&ch, 1);
  Serial.write(ch);
  return 1;
}

size_t DebugSerial::write(const uint8_t *buffer, size_t size)
{
  webSocket.broadcastTXT(buffer, size);
  Serial.write(buffer, size);
  return size;
}

DebugSerial iDebugSerial;
