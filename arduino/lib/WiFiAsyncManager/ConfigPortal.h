#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <DNSServer.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <IPAddress.h>

class ConfigPortal {
	public:
		~ConfigPortal();

		void begin(void);
		void stop(void);
		void loop(void);
		bool is_done(void) { return m_is_done; }
		void reset(void) { m_is_done = false; m_ssid = m_pass = ""; m_ip = m_nm = m_gw = m_dns_ip = INADDR_NONE; }

		String m_ssid;
		String m_pass;
		String m_desc;
		IPAddress m_ip;
		IPAddress m_nm;
		IPAddress m_gw;
		IPAddress m_dns_ip;

	private:
		void handle_root(void);
		void handle_wifi_save(void);
		void handle_not_found(void);
		bool handle_captive_portal(void);
		bool valid_wifi_params(void);

		DNSServer *m_dns;
		ESP8266WebServer *m_web;
		bool m_is_done;
};

#endif
