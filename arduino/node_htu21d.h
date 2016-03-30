#ifndef NODE_HTU21D_H
#define NODE_HTU21D_H

#include "common.h"
#include "libraries/HTU21D/SparkFunHTU21D.h"

class NodeHTU21D : public Node {
public:
  void setup(void);
  void loop(void);
private:
  HTU21D htu21d;
};

#endif
