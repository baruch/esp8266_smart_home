#ifndef NODE_POWERBUTTON_H
#define NODE_POWERBUTTON_H

#include "common.h"

/*
 * This class represents a module with a button and a relay, when the button is pressed it toggles the relay state.
 * It also listens to mqtt to toggle the state remotely.
 */

class NodeRelayWithButton : public Node {
public:
  void setup(void);
  void loop(void);
  void set_state(int state);
  void toggle_state(void);
private:
  int relay_state;
  int last_button_state;
};

#endif
