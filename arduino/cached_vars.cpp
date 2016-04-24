#include "cached_vars.h"
#include "config.h"
#include "libraries/DebugPrint/DebugPrint.h"

#define CACHE_FILENAME "cache.cfg"

CachedVars cache;

bool CachedVars::set_mqtt_server(IPAddress const &server)
{
  if (server == m_mqtt_server)
    return false;

  m_mqtt_server = server;
  m_modified = true;
  return true;
}

bool CachedVars::set_mqtt_port(int port)
{
  if (port == m_mqtt_port)
    return false;

  m_mqtt_port = port;
  m_modified = true;
  return true;
}

bool CachedVars::set_log_server(IPAddress const &server)
{
    if (server == m_log_server)
      return false;

    m_log_server = server;
    m_modified = true;
    return true;
}

void CachedVars::load(void)
{
  Config cfg(CACHE_FILENAME);
  cfg.readFile();
  m_mqtt_server = cfg.getValueIP("mqtt_server");
  m_mqtt_port = cfg.getValueInt("mqtt_port");
  m_log_server = cfg.getValueIP("log_server");

  debug.log("Cached mqtt server: ", m_mqtt_server.toString());
  debug.log("Cached mqtt port: ", m_mqtt_port);
  debug.log("Cached log server: ", m_log_server.toString());

  m_modified = false;
}

void CachedVars::save(void)
{
  if (!m_modified)
    return;

  Config cfg(CACHE_FILENAME);
  cfg.setValueIP("mqtt_server", m_mqtt_server);
  cfg.setValueInt("mqtt_port", m_mqtt_port);
  cfg.setValueIP("log_server", m_log_server);
  cfg.writeFile();
  m_modified = false;
}
