#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "WiFiManager/WiFiManager.h"
#include <ArduinoJson.h>
#include "config.h"
#include <GDBStub.h>

#define CONFIG_FILE "/config.ini"

char mqtt_server[40];
char mqtt_port[6] = "9090";
char static_ip[16] = "";
char static_nm[16] = "";
char static_gw[16] = "";
char dns[40] = "";
bool shouldSaveConfig;

void print_str(const char *name, const char *val)
{
  Serial.print(name);
  Serial.print(": \"");
  Serial.print(val);
  Serial.println('"');
}

void spiffs_mount() {
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFs not initialized, reinitializing");
    if (!SPIFFS.format()) {
      Serial.println("SPIFFs format failed.");
      while (1);
    }
  }
}

void config_load() {
  Serial.println("SPIFFS initialized");
  Config cfg(CONFIG_FILE);

  Serial.println("Loading config");
  cfg.readFile();
  Serial.println("Config loaded");
  strcpy(static_ip, cfg.getValueStr("ip"));
  strcpy(static_gw, cfg.getValueStr("gw"));
  strcpy(static_nm, cfg.getValueStr("nm"));
  strcpy(static_nm, cfg.getValueStr("dns"));

  print_str("IP", static_ip);
  print_str("GW", static_gw);
  print_str("NM", static_nm);
  print_str("DNS", static_nm);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void net_config() {
  WiFiManager wifiManager;

  WiFiManagerParameter custom_ip("ip", "Static IP", static_ip, 40);
  WiFiManagerParameter custom_gw("gw", "Static Gateway", static_gw, 5);
  WiFiManagerParameter custom_nm("nm", "Static Netmask", static_nm, 5);
  WiFiManagerParameter custom_dns("dns", "Domain Name Server", dns, 40);
  
  if (strlen(static_ip) > 0) {
    IPAddress _ip, _gw, _nm;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _nm.fromString(static_nm);
    
    wifiManager.setSTAStaticIPConfig(_ip, _gw, _nm);
    
  }

  wifiManager.addParameter(&custom_ip);
  wifiManager.addParameter(&custom_nm);
  wifiManager.addParameter(&custom_gw);
  wifiManager.addParameter(&custom_dns);

  //wifiManager.setTimeout(600);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  if (!wifiManager.autoConnect()) {
    // Not connected
    Serial.println("Failed to connect to AP");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Connected");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());


  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    Config cfg(CONFIG_FILE);
    cfg.setValueStr("ip", custom_ip.getValue());
    cfg.setValueStr("gw", custom_gw.getValue());
    cfg.setValueStr("nm", custom_nm.getValue());
    cfg.setValueStr("dns", custom_dns.getValue());
    cfg.writeFile();
    
    delay(100);
    ESP.restart();
    delay(1000);
 }
}

void setup() {
  Serial.begin(115200);
#ifdef DEBUG
  delay(10000); // Wait for port to open, debug only
#endif

  Serial.println(ESP.getResetInfo());
  Serial.println(ESP.getResetReason());

  uint32_t t1 = ESP.getCycleCount();
  spiffs_mount();
  config_load();
  net_config();
  uint32_t t2 = ESP.getCycleCount();

  Serial.print("setup time ");
  Serial.print(t2-t1);
  Serial.print(" in micros ");
  Serial.println(clockCyclesToMicroseconds(t2-t1));

  // Load node params
  // Load wifi config
  // Button reset check
  Serial.println("Setup done");
}

void loop() {
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'f') {
      Serial.println("Clearing Config");
      WiFi.disconnect(true);
      SPIFFS.remove(CONFIG_FILE);
      Serial.println("Clearing done");
      delay(100);
      ESP.restart();
    }
  }
}
