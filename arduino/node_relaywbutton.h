#ifndef NODE_POWERBUTTON_H
#define NODE_POWERBUTTON_H

#include "common.h"
#include "libraries/ADS1115/ADS1115.h"

/*
   This class represents a module with a button and a relay, when the button is pressed it toggles the relay state.
   It also listens to mqtt to toggle the state remotely.

   GPIO	Purpose

   0	   SCL
   2	   SDA
   5	   Alert ADS
   12	   Button
   13	   Relay active low

   ADS1115: (For testing we have two current measurement devices)
   Pins 1,2: Coil differential measurement
   Pin  4: ACS712
*/

class NodeRelayWithButton : public NodeActuator {
  public:
    void setup(void);
    unsigned loop(void);
    void mqtt_connected_event(void);
    void state_update(void);
    void set_state(int state);
    bool set_state_no_update(int state);
    void toggle_state(void);
  private:
    void button_pressed(void);
    void mqtt_relay_state(char *data);

    int relay_state;
    unsigned long debounce_count;
    unsigned long last_sample_millis;
    ADS1115 m_adc;

    float m_current;
    float m_last_reported_current;
    float m_current_sum;
    unsigned m_current_samples;
    unsigned long m_current_sample_time;
};

#endif
