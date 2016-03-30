#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>

void config_load(void);
void net_config(void);
void discover_server(void);
void conditional_discover(void);
char nibbleToChar(uint32_t val);
void print_hexdump(const char *buf, size_t buf_len);
void print_str(const char *name, const char *val);
void check_upgrade(void);
void node_setup(void);
void node_loop(void);

#endif
