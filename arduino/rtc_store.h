#ifndef RTC_STORE_H
#define RTC_STORE_H

#include <stdint.h>

typedef struct rtc_store {
  uint16_t magic;
  uint8_t size; // Previous struct size, to allow upgrades
  uint8_t version;
  uint8_t num_connect_fails;
  int8_t last_rssi;
  uint8_t need_high_power;
} rtc_store_t;

extern rtc_store_t rtc_store;

void rtc_store_save(rtc_store_t *data);
bool rtc_store_load(rtc_store_t *data);

void rtc_store_event_connected(void);
void rtc_store_event_connection_failed(void);

#endif
