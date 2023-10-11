#ifndef __NTP_H__
#define __NTP_H__
#include <WiFiUdp.h>
WiFiUDP ntp;
#define NTP_PACKET_SIZE  48 // NTP time stamp is in the first 48 bytes of the message
uint8_t now_ntp = 0;
void ntpclient() {
  byte packetBuffer[ NTP_PACKET_SIZE + 2]; //buffer to hold incoming and outgoing packets
  IPAddress timeServerIP; // time.nist.gov NTP server address
  ntp.begin(123);
  for (uint8_t i = 0; i < 5; i++) {
    if (WiFi.hostByName(ntpServerName[now_ntp % 5].c_str(), timeServerIP)) break;
    now_ntp = (now_ntp + 1) % 5; //最多5个ntp
  }
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  ntp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
  ntp.write(packetBuffer, NTP_PACKET_SIZE);
  ntp.endPacket();
  uint8_t i;
  for (i = 0; i < 40; i++) {
    if (ntp.parsePacket()) break;
    delay(100);
    yield();
  }
  if (i >= 9) {
    ntp.stop();
    return;
  }

  ntp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  uint32_t dida = word(packetBuffer[40], packetBuffer[41]);
  dida = dida << 16 | word(packetBuffer[42], packetBuffer[43]);
  dida += 8 * 3600;
  uint32_t dida2000 = dida - (365 * 100 + 24) * 24 * 60 * 60;
  year = dida2000 / (36524 * 36 * 24);
  dida2000 = dida2000 % (36524 * 36 * 24);
  int16_t days = dida2000 / (3600 * 24);
  uint8_t md[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (year % 4 == 0) md[1] = 29;
  for (uint8_t i = 0; i < 12; i++) {
    if (days - md[i] < 0) {
      month = i + 1;
      day = days + 1;
      break;
    }
    days -= md[i];
  }
  dida2000 = dida % (3600 * 24);
  sec = dida2000 % 60;
  dida2000 = dida2000 / 60;
  minute = dida2000 % 60;
  hour = dida2000 / 60;
  ntp.stop();
  update_disp();
}

#endif //__NTP_H__
