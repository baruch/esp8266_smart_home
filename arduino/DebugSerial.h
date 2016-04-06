#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

#include <Stream.h>
#include "globals.h"

class DebugSerial : public Stream {
  public:
    DebugSerial();
    void begin(int speed);
    int available(void);
    int read(void);
    virtual size_t write(uint8_t ch);
    virtual size_t write(const uint8_t *buffer, size_t size);
    void flush(void);
    int peek(void);
    void disconnect(void);

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
