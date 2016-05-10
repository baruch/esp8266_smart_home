/*
 PubSubClient.h - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#include <Arduino.h>
#include "IPAddress.h"
#include "../ESPAsyncTCP/ESPAsyncTCP.h"
#include "../CooperativeThread/CooperativeThread.h"

#define MQTT_VERSION_3_1      3
#define MQTT_VERSION_3_1_1    4

// MQTT_VERSION : Pick the version
//#define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION MQTT_VERSION_3_1_1
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 256
#endif

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 15
#endif

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 15
#endif

// Possible values for client.state()
typedef enum mqtt_state {
  MQTT_CONNECTION_TIMEOUT,
  MQTT_CONNECTION_LOST,
  MQTT_CONNECT_FAILED,
  MQTT_DISCONNECTED,
  MQTT_CONNECTING,
  MQTT_CONNECTED,
  MQTT_CONNECT_BAD_PROTOCOL,
  MQTT_CONNECT_BAD_CLIENT_ID,
  MQTT_CONNECT_UNAVAILABLE,
  MQTT_CONNECT_BAD_CREDENTIALS,
  MQTT_CONNECT_UNAUTHORIZED,
} mqtt_state_e;

#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved

#define MQTTQOS0        (0 << 1)
#define MQTTQOS1        (1 << 1)
#define MQTTQOS2        (2 << 1)
#define MQTTMASK        (3 << 1)

#include <functional>
#define MQTT_CALLBACK_SIGNATURE std::function<void(char*, uint8_t*, unsigned int)> callback

class MQTTPacketBuffer {
private:
        uint8_t m_buffer[1460];
        uint16_t start_pos;
        uint16_t len;
        uint8_t header_len;
public:
        MQTTPacketBuffer() : start_pos(0), len(0) {}
        
        bool startPacket(uint8_t cmd, uint16_t data_len = MQTT_MAX_PACKET_SIZE);
        void writeHeader(void);
        bool opcodePacket(uint8_t cmd) { if (!startPacket(cmd, 0)) { return false; } else { writeHeader(); return true; }}
        
        void writeString(const char* string);
        void writeByte(uint8_t data) { m_buffer[len++] = data; }

        uint8_t* buffer(void) { return m_buffer; }
        uint16_t length(void) { return len; }
        void clear(void) { len = start_pos = 0; }
};

class PubSubClient : public CoopThread {
private:
   AsyncClient* _client;
   uint8_t buffer_in[1460];
   unsigned buffer_in_len;

   MQTTPacketBuffer buffer_out;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;
   MQTT_CALLBACK_SIGNATURE;
   uint16_t readPacket(uint8_t*);
   boolean readByte(uint8_t * result);
   boolean readByte(uint8_t * result, uint16_t * index);
   bool sendConnectMessage(void);
   IPAddress ip;
   uint16_t port;
   String willTopic;
   bool willRetain;
   String willMessage;
   String id;
   String user;
   String pass;
   int _state;
   uint16_t nextMsgId;

   void onConnect(void *arg, AsyncClient *client);
   void onDisconnect(void *arg, AsyncClient *client);
   void onAck(void *arg, AsyncClient *client, size_t len, uint32_t time);
   void onError(void *, AsyncClient *client, int8_t error);
   void onData(void *, AsyncClient *client, void *data, size_t len);
   void onTimeout(void *, AsyncClient *client, uint32_t time);

   bool sendData(void);
   void sendPing(void);
   void tryToReceivePacket(void);
   bool waitForConnectReply(void);

   bool available(void) { return buffer_in_len > 0; }
   void consume(uint16_t len);

public:
   PubSubClient(AsyncClient* client);
   void user_thread(void);

   PubSubClient& setServer(IPAddress ip, uint16_t port);
   PubSubClient& setCallback(MQTT_CALLBACK_SIGNATURE);
   PubSubClient& setUser(const char *user, const char *pass);
   PubSubClient& setWill(const char *willTopic, bool willRetain, const char *willMessage);
   PubSubClient& setId(const char *id);

   boolean connect(void);
   void disconnect();
   boolean connected();
   int state();

   boolean publish(const char* topic, const char* payload, boolean retained = false) { publish(topic, (const uint8_t*)payload, strlen(payload), retained); }
   boolean publish(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained = false);
   boolean subscribe(const char* topic);
};


#endif
