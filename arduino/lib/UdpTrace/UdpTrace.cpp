#include "UdpTrace.h"

void UdpTrace::begin(IPAddress ip, uint16_t port)
{
  m_ip = ip;
  m_port = port;
  m_id = 0;
  m_last_msec = 0;
  m_pkt_len = 0;

  start_packet();
}

void UdpTrace::start_packet()
{
  m_udp.beginPacket(m_ip, m_port);
  m_udp.write((uint8_t*)&m_id, sizeof(m_id));
  m_pkt_len += sizeof(m_id);
  m_id++;
}

void UdpTrace::sample(unsigned long msec, uint16_t val)
{
  unsigned long msec_diff_full = msec - m_last_msec;
  uint8_t msec_diff;

  if (msec_diff_full > 255)
    msec_diff = 0xFF;
  else {
    msec_diff = msec_diff_full;
  }
  m_last_msec = msec;

  m_udp.write(&msec_diff, 1);
  m_udp.write((uint8_t *)&val, sizeof(val));
  m_pkt_len += 1 + sizeof(val);

  if (m_pkt_len > 1400) {
    m_udp.endPacket();
    m_pkt_len = 0;
    start_packet();
  }
}
