#ifndef __NTP_H__
#define __NTP_H__
#include <WiFiUdp.h>
WiFiUDP ntp;
Ticker ntpTicker;
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
void ntpclient();
char* ntpServerName = "2.debian.pool.ntp.org";
uint8_t ntp_count = 0;
void ntp_recover() {
  int cb = ntp.parsePacket();
  if (!cb) {
    if (ntp_count > 3) {
      ntp.stop();
      ntpTicker.attach(601, ntpclient);
      ntp_count = 0;
      ntpServerName[0]++;
      if (ntpServerName[0] > '9') ntpServerName[0] = '0';
      return;
    }
    ntp_count++;
    return;
  }
  ntp_count = 0;
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
  ntpTicker.attach(3600 * 24, ntpclient);
}
void ntpclient() {
  IPAddress timeServerIP; // time.nist.gov NTP server address
  ntp.begin(123);
  WiFi.hostByName(ntpServerName, timeServerIP);
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
  ntpTicker.attach_ms(500, ntp_recover);
}


#endif //__NTP_H__
