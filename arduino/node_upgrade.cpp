#include <ESP8266httpUpdate.h>
#include "common.h"
#include "node_mqtt.h"
#include "globals.h"

#if defined(HTTP_UPDATE_START_CALLBACK)
void upgrade_starting(void) {
  debug.log("Upgrade starting");
  mqtt_publish_str("online", "upgrading");
}
#endif

void check_upgrade() {
#if defined(HTTP_UPDATE_START_CALLBACK)
  ESPhttpUpdate.onStart(upgrade_starting);
#endif
  debug.log("Checking for upgrade");
  t_httpUpdate_return ret = ESPhttpUpdate.update(mqtt_server.toString(), UPGRADE_PORT, UPGRADE_PATH, VERSION);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      debug.log("HTTP_UPDATE_FAILD Error (", ESPhttpUpdate.getLastError(), "): ", ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      debug.log("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      debug.log("HTTP_UPDATE_OK");
      break;

    default:
      debug.log("Unknown code ", ret);
      break;
  }
}

