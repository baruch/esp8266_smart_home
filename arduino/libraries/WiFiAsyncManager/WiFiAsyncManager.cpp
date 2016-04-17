#include "WiFiAsyncManager.h"
#include "../DebugPrint/DebugPrint.h"

#include <Esp.h>

extern "C" {
#include "user_interface.h"
}

#define RECONNECT_TIME (30*1000)

WiFiAsyncManager wifi;

static bool is_wifi_configured(void)
{
    struct station_config conf;
    wifi_station_get_config(&conf);
    return conf.ssid[0];
}

void WiFiAsyncManager::begin(const IPAddress& ip, const IPAddress& gw, const IPAddress& nm, const IPAddress& dns)
{
        m_is_wifi_configured = is_wifi_configured();
        m_last_status = WL_DISCONNECTED;
        m_config_test_end_time = 0;
        m_is_config_changed = false;

        config_wifi(NULL, NULL, ip, gw, nm, dns);
}

void WiFiAsyncManager::config_wifi(const char *ssid, const char *pass, const IPAddress &ip, const IPAddress &gw, const IPAddress &nm, const IPAddress &dns)
{
        Serial.println("configuring wifi");
        m_last_reconnect_time = 0;

        if (ip != INADDR_NONE) {
                WiFi.config(ip, gw, nm, dns);
                m_is_static_ip = true;
        } else {
                m_is_static_ip = false;
        }

        //debug.log("Wifi configured=", m_is_wifi_configured, " static=", m_is_static_ip);

        WiFi.begin(ssid, pass);
        debug.log("wifi started");
}

void WiFiAsyncManager::loop(void)
{
        wl_status_t new_status = WiFi.status();
        if (new_status != m_last_status) {
                debug.log("Wifi status changed from ", m_last_status, " to ", new_status);

                if (m_last_status == WL_CONNECTED) {
                        // This will restart reconnection immediately
                        m_last_reconnect_time = 0;
                }

                if (new_status == WL_CONNECTED) {
                        debug.log("IP ", WiFi.localIP(), " Netmask ", WiFi.subnetMask(), " Gateway ", WiFi.gatewayIP());
                }
                m_last_status = new_status;
        }

        if (m_is_wifi_configured) {
                // If AP configured and we are disconnected, reconnect after a timeout
                if (WiFi.isConnected())
                        return;

                unsigned long now = millis();
                if (now - m_last_reconnect_time > RECONNECT_TIME) {
                        WiFi.reconnect();
                        debug.log("Attempting to reconnect to wifi");
                        m_last_reconnect_time = now;
                }
        } else {
                // If no AP configured, start AP and wait for configuration
                if (m_portal) {
                        m_portal->loop();
                        if (m_config_test_end_time == 0) {
                                if (m_portal->is_done()) {
                                        // We actually just got the configuration, now we need to test it and retry if unsuccessful
                                        debug.log(F("Portal configuration done, now testing it"));
                                        WiFi.persistent(false);
                                        debug.log(F("network "), m_portal->m_ip, '/', m_portal->m_nm, '/', m_portal->m_gw, " dns ", m_portal->m_dns_ip);
                                        debug.log(F("wifi "), m_portal->m_ssid.c_str(), ' ', m_portal->m_pass.c_str());
                                        //delay(2000);
                                        //m_portal->stop();
                                        //WiFiClient::stopAll();
                                        delay(100);
                                        WiFi.enableAP(false);
                                        WiFi.enableSTA(true);
                                        delay(100);
                                        config_wifi(m_portal->m_ssid.c_str(), m_portal->m_pass.c_str(), m_portal->m_ip, m_portal->m_gw, m_portal->m_nm, m_portal->m_dns_ip);
                                        debug.log("configured wifi");
                                        debug.log("portal reset");
                                        m_config_test_end_time = millis() + 30*1000;
                                        WiFi.persistent(true);
                                        debug.log("all done");
                                }
                        } else {
                                if (new_status == WL_CONNECTED) {
                                        debug.log(F("WiFi configured and connected"));
                                        // We configured everything and are running now
                                        WiFi.persistent(true);
                                        WiFi.disconnect();
                                        config_wifi(m_portal->m_ssid.c_str(), m_portal->m_pass.c_str(), m_portal->m_ip, m_portal->m_gw, m_portal->m_nm, m_portal->m_dns_ip);

                                        // Close out the config portal
                                        // TODO: (fix the crash and remove the memory) delete m_portal;
                                        m_portal = NULL;
                                        m_is_wifi_configured = true;
                                        m_is_config_changed = true;
                                        m_last_reconnect_time = 0;
                                } else if (millis() > m_config_test_end_time) {
                                        debug.log("WiFi configuration timed out");
                                        WiFi.enableSTA(false);
                                        WiFi.enableAP(true);
                                        m_config_test_end_time = 0;
                                        m_portal->reset();
                                        // TODO: fix the crash and remove the memory) m_portal->begin();
                                }
                        }
                } else {
                        WiFi.enableSTA(false);
                        m_portal = new ConfigPortal();
                        m_portal->begin();
                        m_portal->loop();
                }

        }

        // Possibly also if AP configured and not connected for a while, bring AP in parallel to attempting STA to allow reconfigure
}

void WiFiAsyncManager::get_static_ip(IPAddress &ip, IPAddress &gw, IPAddress &nm, IPAddress &dns)
{
        if (m_is_static_ip) {
                ip = WiFi.localIP();
                gw = WiFi.gatewayIP();
                nm = WiFi.subnetMask();
                dns = WiFi.dnsIP();
        } else {
                ip = gw = nm = dns = INADDR_NONE;
        }
}
