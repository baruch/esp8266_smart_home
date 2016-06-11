#ifndef UDP_TRACE_H
#define UDP_TRACE_H

#include <stdint.h>
#include <WiFiUdp.h>

class UdpTrace {
public:
  void begin(IPAddress ip, uint16_t port);
  void sample(unsigned long msec, uint16_t val);
private:
  void start_packet(void);
  
  unsigned long m_last_msec;
  unsigned m_pkt_len;
  WiFiUDP m_udp;
  IPAddress m_ip;
  uint16_t m_port;
  uint16_t m_id;
};

#endif
