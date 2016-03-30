#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

#define VERSION "SHMVER-0.0.4d"
#define CONFIG_FILE "/config.ini"
#define NODE_TYPE_FILENAME "/nodetype.bin"
#define UPGRADE_PORT 24320
#define DISCOVER_PORT 24320
#define UPGRADE_PATH "/node_v1.bin"

extern char node_name[20];
extern int node_type;
extern char node_desc[32];
extern char mqtt_server[40];
extern int mqtt_port;
extern char static_ip[16];
extern char static_nm[16];
extern char static_gw[16];
extern char dns[40];

#endif
