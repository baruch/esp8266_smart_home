#ifndef NODE_SOIL_MOISTURE_H
#define NODE_SOIL_MOISTURE_H

#include "common.h"
#include "libraries/ADS1115/ADS1115.h"

class NodeSoilMoisture : public Node {
public:
  void setup(void);
  unsigned loop(void);
private:
  uint16_t read_value(enum ads1115_mux mux);
  ADS1115 m_ads1115;
};

#endif
