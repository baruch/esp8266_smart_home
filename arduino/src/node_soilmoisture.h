#ifndef NODE_SOIL_MOISTURE_H
#define NODE_SOIL_MOISTURE_H

#include "common.h"
#include "libraries/ADS1115/ADS1115.h"
#include "libraries/CooperativeThread/CooperativeThread.h"

class NodeSoilMoisture : public NodeSensor, CoopThread {
  public:
    void setup(void);
    unsigned loop(void);
  protected:
    void user_thread(void);

  private:
    float sample_read(void);
    void i2c_error(uint8_t i2c_state);

    ADS1115 m_ads1115;
    float m_bat;
    float m_moisture;
    float m_trigger;
    unsigned m_deep_sleep;
};

#endif
