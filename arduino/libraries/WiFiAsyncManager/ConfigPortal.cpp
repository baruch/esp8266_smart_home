#include "ConfigPortal.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "../DebugPrint/DebugPrint.h"

#undef min
#undef max
#include <algorithm>

static const int DNS_PORT = 53;

static const char HTTP_HEAD[] PROGMEM            = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>";
static const char HTTP_HEAD_END[] PROGMEM        = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>"
                                                   "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>"
                                                   "</head><body><div style='text-align:left;display:inline-block;min-width:260px;'>";
static const char HTTP_ITEM[] PROGMEM            = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
static const char HTTP_FORM[] PROGMEM      = "<form method='get' action='wifisave'>"
                                             "<input id='s' name='s' length=32 placeholder='SSID'><br/>"
                                             "<input id='p' name='p' length=64 type='password' placeholder='password'><br/>"
                                             "<input id='d' name='d' length=32 type='text' placeholder='Desscription'><br/>"
                                             "<input id='ip' name='ip' length=15 type='text' placeholder='Static IP'><br/>"
                                             "<input id='nm' name='nm' length=15 type='text' placeholder='Netmask'><br/>"
                                             "<input id='gw' name='gw' length=15 type='text' placeholder='Gateway IP'><br/>"
                                             "<input id='dns' name='dns' length=15 type='text' placeholder='DNS IP'><br/>"
                                             "<br/>"
                                             "<button type='submit'>save</button>"
                                             "</form>"
                                             "<br/>"
                                             "<div class='c'><a href='/wifi'>Scan</a></div>";
static const char HTTP_SAVED[] PROGMEM           = "<div>Credentials Saved<br />Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>";
static const char HTTP_VALIDATION_FAILED[] PROGMEM           = "<div>WiFi params validation failed<br /><a href='/'>Go back</a></div>";
static const char HTTP_END[] PROGMEM             = "</div></body></html>";

static int RSSI_to_quality(int RSSI)
{
  int quality = 0;

  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}


ConfigPortal::~ConfigPortal()
{
        // Disconnect the server apps
        stop();

        // Wait a bit to send any packets needed for closure
        delay(50);

        // Shutdown the Access Point
        WiFi.enableAP(false);
}

void ConfigPortal::stop(void)
{
        m_dns->stop();
        m_web->stop();
        delay(100);

        delete m_dns;
        m_dns = NULL;

        delete m_web;
        m_web = NULL;
}

void ConfigPortal::begin(void)
{
  debug.println(F("Configuring access point... "));

  WiFi.softAP(WiFi.hostname().c_str());

  delay(500); // Without delay I've seen the IP address blank

  m_dns = new DNSServer();
  m_web = new ESP8266WebServer(80);
  m_is_done = false;

  debug.println(F("AP IP address: "), WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  m_dns->setErrorReplyCode(DNSReplyCode::NoError);
  m_dns->start(DNS_PORT, "*", WiFi.softAPIP());

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  m_web->on("/", std::bind(&ConfigPortal::handle_root, this));
  m_web->on("/wifisave", std::bind(&ConfigPortal::handle_wifi_save, this));
  m_web->onNotFound (std::bind(&ConfigPortal::handle_not_found, this));
  m_web->begin(); // Web server start

  debug.println(F("HTTP server started"));
}

void ConfigPortal::loop(void)
{
    m_dns->processNextRequest();
    m_web->handleClient();
}

void ConfigPortal::handle_root(void)
{
        if (handle_captive_portal())
                return;

        String page = FPSTR(HTTP_HEAD);
        page.replace("{v}", "Config ESP");
        page += FPSTR(HTTP_HEAD_END);

        int n = WiFi.scanNetworks();
        if (n == 0) {
                page += F("No networks found. Refresh to scan again.");
        } else {
                //sort networks
                int indices[n];
                for (int i = 0; i < n; i++) {
                        indices[i] = i;
                }

                // RSSI SORT
                std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
                                {
                                return WiFi.RSSI(a) > WiFi.RSSI(b);
                                });
                yield();

                // remove duplicates ( must be RSSI sorted )
                {
                        String cssid;
                        for (int i = 0; i < n; i++) {
                                if(indices[i] == -1) continue;
                                cssid = WiFi.SSID(indices[i]);
                                for (int j = i + 1; j < n; j++) {
                                        if(cssid == WiFi.SSID(indices[j])){
                                                debug.println("DUP AP: " + WiFi.SSID(indices[j]));
                                                indices[j] = -1; // set dup aps to index -1
                                        }
                                }
                                yield();
                        }
                }

                //display networks in page
                for (int i = 0; i < n; i++) {
                        if(indices[i] == -1) continue; // skip dups
                        int quality = RSSI_to_quality(WiFi.RSSI(indices[i]));

                        String item = FPSTR(HTTP_ITEM);
                        String rssiQ(quality);
                        item.replace("{v}", WiFi.SSID(indices[i]));
                        item.replace("{r}", rssiQ);
                        if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
                                item.replace("{i}", "protected");
                        } else {
                                item.replace("{i}", "");
                        }
                        page += item;
                        yield();
                }
                page += "<br/>";
        }

        WiFi.scanDelete(); // Free memory used in the scan

        page += FPSTR(HTTP_FORM);
        page += FPSTR(HTTP_END);

        m_web->send(200, "text/html", page);
}

bool ConfigPortal::valid_wifi_params(void)
{
        m_ssid = m_web->arg("s");
        if (m_ssid.length() == 0) {
                debug.println("SSID not provided");
                return false;
        }

        m_pass = m_web->arg("p");
        m_desc = m_web->arg("d");
        bool ip_valid = m_ip.fromString(m_web->arg("ip"));
        bool nm_valid = m_nm.fromString(m_web->arg("nm"));
        bool gw_valid = m_gw.fromString(m_web->arg("gw"));
        m_dns_ip.fromString(m_web->arg("dns"));

        debug.println(F("network "), m_ip, '/', m_nm, '/', m_gw, " dns ", m_dns_ip);
        debug.println(F("wifi "), m_ssid, ' ', m_pass);

        if ((ip_valid || nm_valid || gw_valid) && (!ip_valid || !nm_valid || !gw_valid)) {
                // The entire triplet must be valid IP addresses
                debug.println(F("Static IP address incomplete"));
                m_ip = m_nm = m_gw = INADDR_NONE;
                return false;
        }

        // TODO: If ip/nm/gw provided need to validate that nm is valid and that gw is within ip/nm range
        // TODO: Also need to validate that if the network is locked that a password was provided
        return true;
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void ConfigPortal::handle_wifi_save()
{
        debug.println(F("WiFi save"));
        bool valid = valid_wifi_params();
        String page = FPSTR(HTTP_HEAD);

        if (valid) {
                //SAVE/connect here
                debug.println(F("Credentials Saved"));
                page.replace("{v}", "Credentials Saved");
                page += FPSTR(HTTP_HEAD_END);
                page += FPSTR(HTTP_SAVED);
                m_is_done = true;
        } else {
                debug.println(F("Validation failed"));
                page.replace("{v}", "Validation failed");
                page += FPSTR(HTTP_HEAD_END);
                page += FPSTR(HTTP_VALIDATION_FAILED);
                
                m_is_done = false;
                m_ssid = m_pass = "";
                m_ip = m_nm = m_gw = m_dns_ip = INADDR_NONE;
        }
        page += FPSTR(HTTP_END);

        m_web->send(200, F("text/html"), page);
        delay(10);
        m_web->client().stop(); // Stop is needed because we sent no content length

        debug.println(F("Sent wifi save page"));
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
bool ConfigPortal::handle_captive_portal(void)
{
        IPAddress host_ip;
        bool is_ip = host_ip.fromString(m_web->hostHeader());
        if (is_ip && host_ip == WiFi.softAPIP())
                return false;

        m_web->sendHeader(F("Location"), String("http://") + WiFi.softAPIP().toString(), true);
        m_web->send(302, F("text/plain"), "");
        m_web->client().stop(); // Stop is needed because we sent no content length
        return true;
}

void ConfigPortal::handle_not_found(void)
{
        if (handle_captive_portal()) { // If captive portal redirect instead of displaying the error page.
                return;
        }
        String message = "File Not Found\n\nURI: ";
        message += m_web->uri();
        message += "\nMethod: ";
        message += ( m_web->method() == HTTP_GET ) ? "GET" : "POST";
        message += "\nArguments: ";
        message += m_web->args();
        message += "\n";

        for ( uint8_t i = 0; i < m_web->args(); i++ ) {
                message += " " + m_web->argName ( i ) + ": " + m_web->arg ( i ) + "\n";
        }
        m_web->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        m_web->sendHeader("Pragma", "no-cache");
        m_web->sendHeader("Expires", "-1");
        m_web->send ( 404, F("text/plain"), message );
}
