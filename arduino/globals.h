#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <IPAddress.h>
#include "libraries/Semaphore/Semaphore.h"

#define VERSION "SHMVER-0.0.11"
#define CONFIG_FILE "/config.ini"
#define NODE_TYPE_FILENAME "/nodetype.bin"
#define UPGRADE_PORT 24320
#define DISCOVER_PORT 24320
#define UPGRADE_PATH "/node_v1.bin"
#define DEFAULT_DEEP_SLEEP_TIME (15*60)

extern char node_name[20];
extern int node_type;
extern char node_desc[32];
extern IPAddress static_ip;
extern IPAddress static_nm;
extern IPAddress static_gw;
extern IPAddress dns;
extern Semaphore sleep_lock;

#endif
