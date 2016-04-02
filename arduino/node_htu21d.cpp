#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"
#include "DebugSerial.h"

void NodeHTU21D::setup(void)
{
  htu21d.begin();
}

void NodeHTU21D::loop(void)
{
  static long lastMsg = -60000; // Ensure the first time we are around we do the first report
  
  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;

    {
      float humd = htu21d.readHumidity();
      float temp = htu21d.readTemperature();

      if (mqtt_connected()) {
        mqtt_publish_float("humidity", humd);
        mqtt_publish_float("temperature", temp);
      }

      Serial.print("Time:");
      Serial.print(millis());
      Serial.print(" Temperature:");
      Serial.print(temp, 1);
      Serial.print("C");
      Serial.print(" Humidity:");
      Serial.print(humd, 1);
      Serial.println("%");
    }
  }
}
