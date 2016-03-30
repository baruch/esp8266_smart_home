#include <ESP8266httpUpdate.h>
#include "DebugSerial.h"
#include "common.h"
#include "node_mqtt.h"
#include "globals.h"

#if defined(HTTP_UPDATE_START_CALLBACK)
void upgrade_starting(void) {
  Serial.println("Upgrade starting");
  mqtt_publish_str("online", "upgrading");
}
#endif

void check_upgrade() {
#if defined(HTTP_UPDATE_START_CALLBACK)
  ESPhttpUpdate.onStart(upgrade_starting);
#endif
  Serial.println("Checking for upgrade");
  t_httpUpdate_return ret = ESPhttpUpdate.update(mqtt_server, UPGRADE_PORT, UPGRADE_PATH, VERSION);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;

    default:
      Serial.printf("Unknown code %d\n", ret);
      break;
  }
}

