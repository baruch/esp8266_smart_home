#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include "globals.h"
#include "DebugPrint.h"

#define TIME_PASSED(next) (next == 0 || ( ((long)(millis()-next)) >= 0 ))

void config_load(void);
void node_type_save(void);
void net_config_setup(void);
void net_config_loop(void);
void config_save(void);
void discover_poll(void);
void discovery_now(void);
char nibbleToChar(uint32_t val);
void print_hexdump(const char *buf, size_t buf_len);
void check_upgrade(void);
void node_setup(void);
unsigned node_loop(void);
void node_mqtt_connected(void);
bool node_is_powered(void);
void restart();
void deep_sleep(unsigned seconds);
void battery_check(float bat);
void config_time(void);

/* Sleeping logic */
typedef enum {
  SLEEP_RES_NO,
  SLEEP_RES_YES,
  SLEEP_RES_TIMEOUT,
} sleep_res_t;

sleep_res_t should_sleep(uint32_t sleep_usec);
void sleep_postpone(unsigned long msec);
void sleep_lock(void);
void sleep_init(void);

class Node {
  public:
    virtual void setup(void) {}

    // This loop is for the type (Sensor or Actuator)
    virtual void loop_for_type(void) {}

    // This loop is for the specific node
    virtual unsigned loop(void) {
      return 0;
    }
    virtual void mqtt_connected_event(void) {}
    virtual bool is_battery_powered() { return false; }
};

class NodeSensor : public Node {
  bool is_battery_powered() { return true; }
};

class NodeActuator : public Node {
  public:
    void loop_for_type(void);

  private:
    int32_t m_last_rssi;
    unsigned long m_next_rssi_poll;
};

#endif
