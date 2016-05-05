/*
  PubSubClient.cpp - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include "Arduino.h"

#define SHORT_HEADER_LEN 2
#define LONG_HEADER_LEN 3

bool MQTTPacketBuffer::startPacket(uint8_t cmd, uint16_t data_len)
{
  if (2 + data_len + len > sizeof(m_buffer))
    return false;

  header_len = data_len < 128 ? SHORT_HEADER_LEN : LONG_HEADER_LEN;
  start_pos = len;
  len += header_len;
  m_buffer[start_pos] = cmd;
  return true;
}

void MQTTPacketBuffer::writeHeader(void)
{
  uint16_t data_len = len - start_pos - header_len;

  if (header_len == SHORT_HEADER_LEN) {
    m_buffer[start_pos+1] = data_len;
  } else {
    m_buffer[start_pos+1] = 0x80 | (data_len % 128);
    m_buffer[start_pos+2] = data_len / 128;
  }
}

PubSubClient::PubSubClient(AsyncClient* client) {
    this->_state = MQTT_DISCONNECTED;
    this->_client = client;
    client->onConnect(std::bind(&PubSubClient::onConnect, this, std::placeholders::_1, std::placeholders::_2), NULL);
    nextMsgId = 1;
}

boolean PubSubClient::connect(void) {
    if (connected())
      return true;

    if (!_client->connect(this->ip, this->port))
      return false;

    this->_state = MQTT_CONNECTING;
    return true;
}

bool PubSubClient::sendConnectMessage(void)
{
  if (!buffer_out.startPacket(MQTTCONNECT))
    return false;

#if MQTT_VERSION == MQTT_VERSION_3_1
  buffer_out.writeString("MQIsdp");
#elif MQTT_VERSION == MQTT_VERSION_3_1_1
  buffer_out.writeString("MQTT");
#else
#error Unknown MQTT Version
#endif

  buffer_out.writeByte(MQTT_VERSION);

  uint8_t v = 0x02;
  if (willTopic) {
    v |= 0x04;
    if (willRetain)
      v |= 0x20;
  }
  if(user != NULL) {
    v = v|0x80;

    if(pass != NULL) {
      v = v|0x40;
    }
  }

  buffer_out.writeByte(v);
  buffer_out.writeByte(MQTT_KEEPALIVE >> 8);
  buffer_out.writeByte(MQTT_KEEPALIVE & 0xFF);
  buffer_out.writeString(id.c_str());

  if (willTopic) {
    buffer_out.writeString(willTopic.c_str());
    buffer_out.writeString(willMessage.c_str());
  }

  if (user) {
    buffer_out.writeString(user.c_str());
    if (pass) {
      buffer_out.writeString(pass.c_str());
    }
  }

  sendData();
}

bool PubSubClient::waitForConnectReply(void)
{
  while (!_client->available()) {
    unsigned long t = millis();
    if (t-lastInActivity >= ((int32_t) MQTT_SOCKET_TIMEOUT*1000UL)) {
      _state = MQTT_CONNECTION_TIMEOUT;
      _client->stop();
      return false;
    }

    thread_yield();
  }
  uint8_t llen;
  uint16_t len = readPacket(&llen);

  if (len == 4) {
    if (buffer[3] == 0) {
      lastInActivity = millis();
      pingOutstanding = false;
      _state = MQTT_CONNECTED;
      return true;
    } else {
      _state = buffer[3]; // Better translate this, it doesnt match the new enum
    }
  }
  _client->stop();
  _state = MQTT_CONNECT_FAILED;
  return false;
}

// reads a byte into result
boolean PubSubClient::readByte(uint8_t * result) {
   uint32_t previousMillis = millis();
   while(!_client->available()) {
     uint32_t currentMillis = millis();
     if(currentMillis - previousMillis >= ((int32_t) MQTT_SOCKET_TIMEOUT * 1000)){
       return false;
     }
   }
   *result = _client->read();
   return true;
}

// reads a byte into result[*index] and increments index
boolean PubSubClient::readByte(uint8_t * result, uint16_t * index){
  uint16_t current_index = *index;
  uint8_t * write_address = &(result[current_index]);
  if(readByte(write_address)){
    *index = current_index + 1;
    return true;
  }
  return false;
}

uint16_t PubSubClient::readPacket(uint8_t* lengthLength) {
    uint16_t len = 0;
    if (!readByte(buffer, &len))
      return 0;
    bool isPublish = (buffer[0]&0xF0) == MQTTPUBLISH;
    uint32_t multiplier = 1;
    uint16_t length = 0;
    uint8_t digit = 0;
    uint8_t start = 0;

    do {
        if(!readByte(&digit)) return 0;
        buffer[len++] = digit;
        length += (digit & 127) * multiplier;
        multiplier *= 128;
    } while ((digit & 128) != 0);
    *lengthLength = len-1;

    if (isPublish) {
        // Read in topic length to calculate bytes to skip over for Stream writing
        if(!readByte(buffer, &len)) return 0;
        if(!readByte(buffer, &len)) return 0;
        start = 2;
    }

    for (uint16_t i = start;i<length;i++) {
        if(!readByte(&digit)) return 0;
        if (len < MQTT_MAX_PACKET_SIZE) {
            buffer[len] = digit;
        }
        len++;
    }

    if (len > MQTT_MAX_PACKET_SIZE) {
        len = 0; // This will cause the packet to be ignored.
    }

    return len;
}

void PubSubClient::user_thread() {
  while (1) {
    // We will be resumed onConnect
    thread_suspend();

    // The TCP connection is established, send conenction message
    sendConnectMessage();
    while (waitForConnectReply()) {
      thread_suspend();
      thread_yield();
    }
    if (_state != MQTT_CONNECTED) {
      // We failed to connect, go back to the start
      continue;
    }

    while (_client->connected()) {
      // We are connected, wait for data and ping from time to time
      tryToReceivePacket();
      sendPing();
      sendData();
      thread_yield();
    }

    // We got disconnected
    _client->stop();
    if (_state == MQTT_CONNECTED)
      _state = MQTT_CONNECTION_LOST;
  }
}

bool PubSubClient::sendData(void)
{
  if (buffer_out.length() == 0)
    return true;
  
  if (!_client->canSend())
    return false;

  _client->write((const char *)buffer_out.buffer(), buffer_out.length());
  buffer_out.clear();
  lastOutActivity = millis();
  return true;
}

void PubSubClient::sendPing(void)
{
  unsigned long t = millis();
  if ((t - lastInActivity < MQTT_KEEPALIVE*1000UL) && (t - lastOutActivity < MQTT_KEEPALIVE*1000UL))
    return;

  if (pingOutstanding) {
    // We sent ping and got no reply, game over
    this->_state = MQTT_CONNECTION_TIMEOUT;
    _client->stop();
  } else {
    // Send a request
    buffer_out.opcodePacket(MQTTPINGREQ);
    pingOutstanding = true;
  }
}

void PubSubClient::tryToReceivePacket(void)
{
  if (!_client->available())
    return;

  uint8_t llen;
  uint16_t len = readPacket(&llen);
  uint8_t *payload;
  uint16_t payload_len;

  if (len == 0)
    return;

  lastInActivity = millis();
  uint8_t type = buffer_in[0]&0xF0;

  switch (type) {
    case MQTTPINGREQ:
      buffer_out.opcodePacket(MQTTPINGRESP);
      return;
    case MQTTPINGRESP:
      pingOutstanding = false;
      return;
    case MQTTPUBLISH:
      // We'll deal with this after the switch
      break;
    default:
      // Not going to do anything with any other message
      return;
  }

  // We are now dealing with a published message
  if (!callback)
    return;

  uint16_t tl = (buffer_in[llen+1]<<8)+buffer_in[llen+2];
  char topic[tl+1];
  for (uint16_t i=0;i<tl;i++) {
    topic[i] = buffer_in[llen+3+i];
  }
  topic[tl] = 0;
  payload = buffer_in+llen+3+tl;
  payload_len = len - llen -3 - tl;

  // msgId only present for QOS>0
  if ((buffer_in[0]&MQTTMASK) == MQTTQOS1) {
    payload += 2;
    payload_len -= 2;

    // Send an ack for a QoS 1 message
    buffer_out.startPacket(MQTTPUBACK, 2);
    buffer_out.writeByte(buffer_in[llen+3+tl]);
    buffer_out.writeByte(buffer_in[llen+3+tl+1]);
    buffer_out.writeHeader();
  }

  callback(topic, payload, payload_len);
}

boolean PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained) {
  if (!connected())
    return false;

  uint8_t cmd = MQTTPUBLISH;
  if (retained)
    cmd |= 1;

  if (!buffer_out.startPacket(cmd, strlen(topic) + plength + 2))
    return false; // TODO: need to flush the buffer and try to write again

  buffer_out.writeString(topic);

  uint16_t i;
  for (i=0;i<plength;i++) {
    buffer_out.writeByte(payload[i]);
  }

  buffer_out.writeHeader();

  return true;
}

boolean PubSubClient::subscribe(const char* topic) {
    if (MQTT_MAX_PACKET_SIZE < 9 + strlen(topic)) {
        // Too long
        return false;
    }
    if (!connected())
      return false;

    nextMsgId++;
    if (nextMsgId == 0) {
      nextMsgId = 1;
    }

    buffer_out.startPacket(MQTTSUBSCRIBE|MQTTQOS1, 5 + strlen(topic));
    buffer_out.writeByte(nextMsgId >> 8);
    buffer_out.writeByte(nextMsgId & 0xFF);
    buffer_out.writeString(topic);
    buffer_out.writeByte(0); // QoS of zero
    buffer_out.writeHeader();
    return true;
}

void PubSubClient::disconnect() {
    buffer_out.opcodePacket(MQTTDISCONNECT);
    sendData();
    _state = MQTT_DISCONNECTED;
    _client->stop();
}

boolean PubSubClient::connected() {
  return this->_state == MQTT_CONNECTED;
}

PubSubClient& PubSubClient::setServer(IPAddress ip, uint16_t port) {
    this->ip = ip;
    this->port = port;
    return *this;
}

PubSubClient& PubSubClient::setCallback(MQTT_CALLBACK_SIGNATURE) {
    this->callback = callback;
    return *this;
}

PubSubClient& PubSubClient::setUser(const char *user, const char *pass)
{
  this->user = user;
  this->pass = pass;
  return *this;
}

PubSubClient& PubSubClient::setWill(const char *willTopic, bool willRetain, const char *willMessage)
{
  this->willTopic = willTopic;
  this->willRetain = willRetain;
  this->willMessage = willMessage;
  return *this;
}

PubSubClient& PubSubClient::setId(const char *id)
{
  this->id = id;
  return *this;
}

int PubSubClient::state() {
    return this->_state;
}


void PubSubClient::onConnect(void *arg, AsyncClient *client)
{
    thread_resume();
}

void PubSubClient::onDisconnect(void *arg, AsyncClient *client)
{
}

void PubSubClient::onAck(void *arg, AsyncClient *client, size_t len, uint32_t time)
{
}

void PubSubClient::onError(void *, AsyncClient *client, int8_t error)
{
}

void PubSubClient::onData(void *, AsyncClient *client, void *data, size_t len)
{
}

void PubSubClient::onTimeout(void *, AsyncClient *client, uint32_t time)
{
}
