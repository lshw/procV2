#ifndef __PROC_H__
#define __PROC_H__

#include "global.h"
#define STACK_PROTECTOR  512 // bytes
#define MAX_SRV_CLIENTS 4
WiFiServer tcpServer(23);
WiFiClient tcpClients[MAX_SRV_CLIENTS];

void proc_setup() {
  tcpServer.begin();
  tcpServer.setNoDelay(true);
}
#define SBUF_SIZE 128
uint8_t sbuf[SBUF_SIZE];
void proc_loop() {
  if (tcpServer.hasClient()) { //有新的连接进来
    //寻找一个空闲或者断开的位置
    int i;
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
      if (!tcpClients[i]) { // equivalent to !tcpClients[i].connected()
        tcpClients[i] = tcpServer.available();
        break;
      }
    //没有位置， busy ,stop();
    if (i == MAX_SRV_CLIENTS) {
      tcpServer.available().println("busy");
    }
  }
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    while (tcpClients[i].available() && Serial.availableForWrite() > 0) {
      sbuf[0] = tcpClients[i].read();
      Serial.write(sbuf[0]);
      for (int i1 = 0; i1 < MAX_SRV_CLIENTS; i1++)
        if (i1 != i && tcpClients[i1]) tcpClients[i1].write(sbuf[0]);
    }
  size_t len = Serial.available();
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    if (tcpClients[i]) {
      size_t afw = tcpClients[i].availableForWrite();
      if (len > afw ) len = afw;
    }
  if (len > SBUF_SIZE ) len = SBUF_SIZE;
  if (len) {
    size_t serial_got = Serial.readBytes(sbuf, len);
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
      if (tcpClients[i].availableForWrite() >= serial_got) {
        size_t tcp_sent = tcpClients[i].write(sbuf, serial_got);
      }
  }
}
#endif //__PROC_H__
