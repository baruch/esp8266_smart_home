#include "globals.h"

#define VERSION_STR "SHMVER-0.0.13.8"

char VERSION[] = VERSION_STR;
char node_name[20];
int node_type;
char node_desc[32];
IPAddress static_ip;
IPAddress static_nm;
IPAddress static_gw;
IPAddress dns;
float battery;
