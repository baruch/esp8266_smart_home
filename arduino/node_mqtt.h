#ifndef NODE_MQTT_H
#define NODE_MQTT_H

void mqtt_topic(char *buf, int buf_len, const char *name);
const char * mqtt_tmp_topic(const char *name);
void mqtt_setup(void);
bool mqtt_connected(void);
void mqtt_loop(void);
void mqtt_publish_float(const char *name, float val);
void mqtt_publish_str(const char *name, const char *val);

#endif
