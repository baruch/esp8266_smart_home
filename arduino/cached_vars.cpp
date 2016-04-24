#include "cached_vars.h"

CachedVars cache;

bool CachedVars::set_mqtt_server(IPAddress const &server)
{
  if (server == m_mqtt_server)
    return false;

  m_mqtt_server = server;
  return true;
}

bool CachedVars::set_mqtt_port(int port)
{
  if (port == m_mqtt_port)
    return false;

  m_mqtt_port = port;
  return true;
}
