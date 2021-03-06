#ifndef CACHED_VARS_H
#define CACHED_VARS_H

#include <IPAddress.h>

class CachedVars {
public:
  void load(void);
  void save(void);

  const IPAddress &get_mqtt_server(void) const { return m_mqtt_server; }
  int get_mqtt_port(void) const { return m_mqtt_port; }
  const IPAddress &get_log_server(void) const { return m_log_server; }
  const IPAddress &get_sntp_server(void) const { return m_sntp_server; }

  bool set_log_server(IPAddress const &server);
  bool set_mqtt_server(IPAddress const &server);
  bool set_mqtt_port(int port);
  bool set_sntp_server(IPAddress const &server);

private:
  bool set_server(IPAddress const &server, IPAddress &this_server);

  IPAddress m_log_server;
  IPAddress m_mqtt_server;
  IPAddress m_sntp_server;
  int m_mqtt_port;
  bool m_modified;
};

extern CachedVars cache;

#endif
