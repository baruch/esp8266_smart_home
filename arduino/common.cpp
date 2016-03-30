#include "common.h"
#include "DebugSerial.h"

void print_str(const char *name, const char *val)
{
  Serial.print(name);
  Serial.print(": \"");
  Serial.print(val);
  Serial.println('"');
}

static void print_hexdump_line(const char *buf, size_t buf_len)
{
  size_t i;

  for (i = 0; i < buf_len && i < 16; i++) {
    Serial.print(buf[i], HEX);
    Serial.print(' ');
  }
  Serial.print("   ");
  for (i = 0; i < buf_len && i < 16; i++) {
    char ch = buf[i];
    if (!isprint(ch))
      ch = '.';
    Serial.print(ch);
  }
  Serial.print('\n');
}

void print_hexdump(const char *buf, size_t buf_len)
{
  size_t i;

  for (i = 0; i < buf_len; i += 16)
    print_hexdump_line(buf+i, buf_len - i);
}

char nibbleToChar(uint32_t val)
{
  val &= 0xF;
  if (val < 10)
    return '0' + val;
  else
    return 'A' + val - 10;
}

