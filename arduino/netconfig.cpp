#include "common.h"
#include "globals.h"
#include "config.h"

#include "libraries/WiFiAsyncManager/WiFiAsyncManager.h"
#include <WiFiUdp.h>

void config_load() {
  Config cfg(CONFIG_FILE);

  cfg.readFile();
  debug.println("Config loaded");
  strcpy(static_ip, cfg.getValueStr("ip"));
  strcpy(static_gw, cfg.getValueStr("gw"));
  strcpy(static_nm, cfg.getValueStr("nm"));
  strcpy(dns, cfg.getValueStr("dns"));
  strcpy(node_desc, cfg.getValueStr("desc"));

  print_str("IP", static_ip);
  print_str("GW", static_gw);
  print_str("NM", static_nm);
  print_str("DNS", dns);
  print_str("Node Desc", node_desc);
}

void config_save(void)
{
  debug.println("saving config");
  Config cfg(CONFIG_FILE);
  cfg.setValueStr("ip", static_ip);
  cfg.setValueStr("gw", static_gw);
  cfg.setValueStr("nm", static_nm);
  cfg.setValueStr("dns", dns);
  cfg.setValueStr("desc", node_desc);
  cfg.writeFile();
  debug.println("save done");

  delay(1000);
  restart();
}

void net_config_setup() {
  IPAddress ip, gw, nm, dns_ip;
  ip.fromString(static_ip);
  gw.fromString(static_gw);
  nm.fromString(static_nm);
  dns_ip.fromString(dns);

  wifi.begin(ip, gw, nm, dns_ip);
  discovery_now();
}

void ip_to_str(IPAddress &ip, char *buf, size_t buf_len)
{
  if (static_cast<uint32_t>(ip) != 0) {
    ip.toString().toCharArray(buf, buf_len, 0);
  } else {
    buf[0] = 0;
  }
}

void net_config_loop() {
  wifi.loop();

  if (wifi.is_config_changed()) {
    //save the custom parameters to FS
    IPAddress _ip, _gw, _sn, _dns;

    wifi.get_static_ip(_ip, _gw, _sn, _dns);

    ip_to_str(_ip, static_ip, sizeof(static_ip));
    ip_to_str(_gw, static_gw, sizeof(static_gw));
    ip_to_str(_sn, static_nm, sizeof(static_nm));
    ip_to_str(_dns, dns, sizeof(dns));

    wifi.get_desc(node_desc, sizeof(node_desc));

    config_save();
    // config save will restart
  }
}

