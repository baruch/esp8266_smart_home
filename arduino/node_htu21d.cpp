#include "common.h"
#include "node_mqtt.h"
#include "node_htu21d.h"

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

  debug.println("Temperature:", temp, " Humidity: ", humd, '%');

  return DEFAULT_DEEP_SLEEP_TIME;
}
