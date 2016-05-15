#include "common.h"
#include <Arduino.h>

static int sleep_locked = 0;
static unsigned long sleep_timeout;
static unsigned long sleep_boot_time;

void sleep_init(void)
{
  sleep_boot_time = millis() + 20*1000;
  sleep_timeout = sleep_boot_time;
}

void sleep_lock(void)
{
  sleep_locked++;
}

void sleep_postpone(unsigned long msec)
{
  unsigned long target = millis() + msec;
  if (sleep_timeout < target) // Only allow reducing the overall time
    return;
  sleep_timeout = target;
}

sleep_res_t should_sleep(uint32_t sleep_usec)
{
  if (sleep_locked)
    return SLEEP_RES_NO;

  unsigned long now = millis();

  // We were asked to postpone the timeout (to wait for input after mqtt connection)
  if (sleep_timeout && now <= sleep_timeout)
    return SLEEP_RES_NO;

  // No postponement and node completed its work
  if (sleep_usec)
    return SLEEP_RES_YES;

  // Timeout without completing the action
  if (sleep_boot_time > now)
    return SLEEP_RES_TIMEOUT;

  return SLEEP_RES_NO;
}
