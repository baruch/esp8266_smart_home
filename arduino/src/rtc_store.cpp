#include "rtc_store.h"
#include "DebugPrint.h"
#include <string.h>
#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

#define RTC_STORE_MAGIC 0x90F2
#define RTC_STORE_OFFSET 128
#define RTC_STORE_VERSION 0

rtc_store_t rtc_store;

void rtc_store_save(rtc_store_t *data)
{
  system_rtc_mem_write(RTC_STORE_OFFSET, data, sizeof(*data));
}

bool rtc_store_load(rtc_store_t *data)
{
  system_rtc_mem_read(RTC_STORE_OFFSET, data, sizeof(*data));
  if (data->magic == RTC_STORE_MAGIC) {
    if (data->size < sizeof(*data)) {
      // Data size increased since last time, clear the unread data
      memset(data + data->size, 0, sizeof(*data) - data->size);
    }
    // TODO: When needed use the data->version for upgrade/downgrade handling

    debug.log("RTC loaded");
    if (rtc_store.need_high_power) {
      // If we failed to connect last time, increase the output power
      debug.log("Using high TX power");
      WiFi.setOutputPower(20);
      rtc_store.need_high_power--; // Filter out in case it was a transient event
    }

    return true;
  }

  memset(data, 0, sizeof(*data));
  data->magic = RTC_STORE_MAGIC;
  data->size = sizeof(*data);
  data->version = RTC_STORE_VERSION;
  debug.log("RTC not present");
  return false;
}

void rtc_store_event_connected(void)
{
  rtc_store.num_connect_fails = 0;
  rtc_store.last_rssi = 0;
  if (WiFi.RSSI() < -80)
    rtc_store.need_high_power = 24;
}

void rtc_store_event_connection_failed(void)
{
  rtc_store.last_rssi = WiFi.RSSI();
  rtc_store.num_connect_fails++;
  rtc_store.need_high_power = 24;
}
