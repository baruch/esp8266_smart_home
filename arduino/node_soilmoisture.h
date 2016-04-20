#ifndef NODE_SOIL_MOISTURE_H
#define NODE_SOIL_MOISTURE_H

#include "common.h"
#include "libraries/ADS1115/ADS1115.h"

class NodeSoilMoisture : public NodeSensor {
  public:
    void setup(void);
    unsigned loop(void);
  private:
    bool sample_read(float &pval);
    ADS1115 m_ads1115;
    unsigned long m_next_poll;
    float m_bat;
    float m_moisture;
    float m_trigger;
    enum {
      STATE_INIT,
      STATE_READ_BAT,
      STATE_READ_MOISTURE,
      STATE_READ_TRIGGER,
      STATE_DONE,
    } m_state;
};

#endif
