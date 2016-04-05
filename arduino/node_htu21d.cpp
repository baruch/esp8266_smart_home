#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"
#include "DebugSerial.h"

void NodeHTU21D::setup(void)
{
  m_loop_only_if_connected = true;
  htu21d.begin();
}

unsigned NodeHTU21D::loop(void)
{
  float humd = htu21d.readHumidity();
  mqtt_publish_float("humidity", humd);
  
  float temp = htu21d.readTemperature();
  mqtt_publish_float("temperature", temp);

  Serial.print("Time:");
  Serial.print(millis());
  Serial.print(" Temperature:");
  Serial.print(temp, 1);
  Serial.print("C");
  Serial.print(" Humidity:");
  Serial.print(humd, 1);
  Serial.println("%");

  return 60;
}
