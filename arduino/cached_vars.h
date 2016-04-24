#ifndef CACHED_VARS_H
#define CACHED_VARS_H

#include <IPAddress.h>

class CachedVars {
public:
  const IPAddress &get_mqtt_server(void) const { return m_mqtt_server; }
  int get_mqtt_port(void) const { return m_mqtt_port; }

  bool set_mqtt_server(IPAddress const &server);
  bool set_mqtt_port(int port);

private:
  IPAddress m_mqtt_server;
  int m_mqtt_port;
};

extern CachedVars cache;

#endif
