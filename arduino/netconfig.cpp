#include "common.h"
#include "globals.h"
#include "config.h"

#include "libraries/WiFiAsyncManager/WiFiAsyncManager.h"
#include <WiFiUdp.h>

void config_load() {
  Config cfg(CONFIG_FILE);

  cfg.readFile();
  debug.log("Config loaded");
  static_ip.fromString(cfg.getValueStr("ip"));
  static_gw.fromString(cfg.getValueStr("gw"));
  static_nm.fromString(cfg.getValueStr("nm"));
  dns.fromString(cfg.getValueStr("dns"));
  strcpy(node_desc, cfg.getValueStr("desc"));

  debug.log("SSID: ", WiFi.SSID(), " pw: ", WiFi.psk());
  debug.log("IP: ", static_ip, " GW: ", static_gw, " NM: ", static_nm, " DNS: ", dns);
  debug.log("Node Desc: ", node_desc);
}

void config_save(void)
{
  debug.log("saving config");
  Config cfg(CONFIG_FILE);
  cfg.setValueStr("ip", static_ip.toString().c_str());
  cfg.setValueStr("gw", static_gw.toString().c_str());
  cfg.setValueStr("nm", static_nm.toString().c_str());
  cfg.setValueStr("dns", dns.toString().c_str());
  cfg.setValueStr("desc", node_desc);
  cfg.writeFile();
  debug.log("save done");

  delay(1000);
  restart();
}

void net_config_setup() {
  wifi.begin(static_ip, static_gw, static_nm, dns);
  discovery_now();

  if( WiFi.SSID().length() == 0) {
    sleep_lock.lock_take();
    // We have no network configuration, do not go to sleep, this lock will release on reboot
  }
}

void net_config_loop() {
  wifi.loop();

  if (wifi.is_config_changed()) {
    //save the custom parameters to FS
    wifi.get_static_ip(static_ip, static_gw, static_nm, dns);
    wifi.get_desc(node_desc, sizeof(node_desc));
    config_save();
    // config save will restart
  }
}
