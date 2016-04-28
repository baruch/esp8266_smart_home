#ifndef WIFI_ASYNC_MANAGER_H
#define WIFI_ASYNC_MANAGER_H

#include <ESP8266WiFi.h>
#include "ConfigPortal.h"

class WiFiAsyncManager {
	public:

		void begin(const IPAddress& ip, const IPAddress& gw, const IPAddress& nm, const IPAddress& dns);
		void loop(void);

		void get_static_ip(IPAddress &ip, IPAddress &gw, IPAddress &nm, IPAddress &dns);
		void get_desc(char *desc, size_t desc_size);
		
		bool is_config_changed(void) { return m_is_config_changed; }
		void config_saved(void) { m_is_config_changed = false; }

		// Disconnect from the network and forget all credentials
		static void disconnect(void) { WiFi.disconnect(); }

	private:
		void config_wifi(const char *ssid, const char *pass, const IPAddress& ip, const IPAddress& gw, const IPAddress& nm, const IPAddress& dns);

		bool m_is_config_changed;
		bool m_is_static_ip;
		bool m_is_wifi_configured;
		unsigned long m_config_test_end_time;
		unsigned long m_last_reconnect_time;
		wl_status_t m_last_status;
		ConfigPortal *m_portal;
};

extern WiFiAsyncManager wifi;

#endif
