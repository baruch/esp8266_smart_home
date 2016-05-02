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
    virtual void user_thread(void);

  private:
    void wait_for_read(float &pval);
    void error(uint8_t i2c_state);

    ADS1115 m_ads1115;
    float m_bat;
    float m_moisture;
    float m_trigger;
    unsigned long m_deep_sleep;
};

#endif
