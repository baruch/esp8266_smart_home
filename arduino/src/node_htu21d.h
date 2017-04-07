#ifndef NODE_HTU21D_H
#define NODE_HTU21D_H

#include "common.h"
#include "SparkFunHTU21D.h"
#include "CooperativeThread.h"

class NodeHTU21D : public NodeSensor, CoopThread {
  public:
    void setup(void);
    unsigned loop(void);

  protected:
    virtual void user_thread(void);

  private:
    bool wait_for_result(uint16_t &value);
    void error(uint8_t i2c_state);

    HTU21D htu21d;
    unsigned m_deep_sleep;
};

#endif
