#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>

void config_load(void);
void node_type_save(void);
void net_config(void);
void discover_server(void);
void conditional_discover(void);
char nibbleToChar(uint32_t val);
void print_hexdump(const char *buf, size_t buf_len);
void print_str(const char *name, const char *val);
void check_upgrade(void);
void node_setup(void);
void node_loop(void);
void node_mqtt_connected(void);
void restart();

class Node {
public:
  virtual void setup(void) {};
  virtual void loop(void) {};
  virtual void mqtt_connected_event(void) {};
};
#endif
