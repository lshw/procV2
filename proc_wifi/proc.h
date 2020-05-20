#ifndef __PROC_H__
#define __PROC_H__

#include <algorithm> // std::min
#define STACK_PROTECTOR  512 // bytes
#define MAX_SRV_CLIENTS 2
WiFiServer tcpServer(23);
WiFiClient tcpClients[MAX_SRV_CLIENTS];

void proc_setup() {
  pinMode(_24V_OUT, OUTPUT);
  digitalWrite(_24V_OUT, HIGH); //默认24V开启输出
  pinMode(PC_RESET, OUTPUT);
  digitalWrite(PC_RESET, LOW);
  pinMode(PC_POWER, OUTPUT);
  digitalWrite(PC_POWER, LOW);
  Serial.begin(115200);
//  digitalWrite(_24V_OUT, eeprom_read(VOUT_SET));
  tcpServer.begin();
  tcpServer.setNoDelay(true);
}

void proc_loop() {
  uint8_t ch;
  if (tcpServer.hasClient()) { //有新的连接进来
    //寻找一个空闲或者断开的位置
    int i;
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
      if (!tcpClients[i]) { // equivalent to !tcpClients[i].connected()
        tcpClients[i] = tcpServer.available();
        break;
      }
    //没有位置， busy
    if (i == MAX_SRV_CLIENTS) {
      tcpServer.available().println("busy");
      // hints: tcpServer.available() is a WiFiClient with short-term scope
      // when out of scope, a WiFiClient will
      // - flush() - all data will be sent
      // - stop() - automatically too
    }
  }
  //check TCP clients for data
  // Incredibly, this code is faster than the bufferred one below - #4620 is needed
  // loopback/3000000baud average 348KB/s
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    while (tcpClients[i].available() && Serial.availableForWrite() > 0) {
      // working char by char is not very efficient
      ch=tcpClients[i].read();
      Serial.write(ch);
      for(int i1=0;i1<MAX_SRV_CLIENTS; i1++)
        if(i1 != i && tcpClients[i1]) tcpClients[i1].write(ch);
    }

  // determine maximum output size "fair TCP use"
  // client.availableForWrite() returns 0 when !client.connected()
  size_t maxToTcp = 0;
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    if (tcpClients[i]) {
      size_t afw = tcpClients[i].availableForWrite();
      if (afw) {
        if (!maxToTcp) {
          maxToTcp = afw;
        } else {
          maxToTcp = std::min(maxToTcp, afw);
        }
      }
    }

  //check UART for data
  size_t len = std::min((size_t)Serial.available(), maxToTcp);
  len = std::min(len, (size_t)STACK_PROTECTOR);
  if (len) {
    uint8_t sbuf[len];
    size_t serial_got = Serial.readBytes(sbuf, len);
    // push UART data to all connected telnet clients
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
      // if client.availableForWrite() was 0 (congested)
      // and increased since then,
      // ensure write space is sufficient:
      if (tcpClients[i].availableForWrite() >= serial_got) {
        size_t tcp_sent = tcpClients[i].write(sbuf, serial_got);
      }
  }
}

#endif //__PROC_H__
