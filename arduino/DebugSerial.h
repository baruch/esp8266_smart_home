#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

#include <Print.h>
#include "globals.h"

class DebugSerial : public Print {
public:
  DebugSerial();
  void begin(int speed);
  bool available(void);
  char read(void);
  virtual size_t write(uint8_t ch);
  virtual size_t write(const uint8_t *buffer, size_t size);
  void flush(void);
  
private:
  void add_to_buf(const uint8_t *buffer, size_t size);

  char buf[1024];
  int buf_len;
};

extern DebugSerial iDebugSerial;

#ifndef NO_REDEF_SERIAL
#define Serial iDebugSerial
#endif

#endif
