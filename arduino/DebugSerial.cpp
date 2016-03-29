#define NO_REDEF_SERIAL

#include "globals.h"
#include "DebugSerial.h"
#include "libraries/arduinoWebSockets/WebSocketsServer.h"
#include "HardwareSerial.h"

static WebSocketsServer webSocket = WebSocketsServer(81);
static int connected;
static char recv_buf[64];
static int recv_buf_start;
static int recv_buf_end;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      connected--;
      break;
    case WStype_CONNECTED:
      {
        connected++;
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %s url: %s\n", num, ip.toString().c_str(), payload);

        // send message to client
        webSocket.sendTXT(num, "Connected to " + String(node_name) + " at " + WiFi.localIP().toString() + "\n");
        iDebugSerial.flush();
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      {
        size_t i;
        for (i = 0; i < lenght; i++) {
          recv_buf[recv_buf_end] = payload[i];
          if (++recv_buf_end == sizeof(recv_buf))
            recv_buf_end = 0;
        }
      }
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
  buf_len = 0;
  connected = 0;
  recv_buf_start = recv_buf_end = 0;

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void DebugSerial::begin(int speed)
{
  Serial.begin(speed);
}

int DebugSerial::available(void)
{
  if (recv_buf_end != recv_buf_start)
    return 1;
  return Serial.available();
}

int DebugSerial::read(void)
{
  if (recv_buf_end != recv_buf_start) {
    char ch = recv_buf[recv_buf_start];
    if (++recv_buf_start == sizeof(recv_buf))
      recv_buf_start = 0;
    return ch;
  }
  return Serial.read();
}

int DebugSerial::peek(void)
{
  if (recv_buf_end != recv_buf_start)
    return recv_buf[recv_buf_start];
  else
    return Serial.peek();
}

void DebugSerial::flush(void)
{
  Serial.flush();
  if (connected && buf_len > 0) {
    webSocket.broadcastTXT(buf, buf_len);
    buf_len = 0;
  }
}

void DebugSerial::add_to_buf(const uint8_t *buffer, size_t size)
{
  if (buf_len + size > sizeof(buf)) {
    if (connected) {
      webSocket.broadcastTXT(buf, buf_len);
      buf_len = 0;

#define OVERFLOW_MSG "\n\nBUFFER OVERFLOWED\n\n"
      webSocket.broadcastTXT(OVERFLOW_MSG, strlen(OVERFLOW_MSG));
    } else {
      return;
    }
  }

  memcpy(buf+buf_len, buffer, size);
  buf_len += size;
  if (connected && buf[buf_len-1] == '\n') {
    webSocket.broadcastTXT(buf, buf_len);
    buf_len = 0;
  }
}

size_t DebugSerial::write(uint8_t ch)
{
  Serial.write(ch);

  add_to_buf(&ch, 1);
  return 1;
}

size_t DebugSerial::write(const uint8_t *buffer, size_t size)
{
  add_to_buf(buffer, size);
  Serial.write(buffer, size);
  return size;
}

DebugSerial iDebugSerial;
