#ifndef __PROC_H__
#define __PROC_H__

#include "global.h"
#define STACK_PROTECTOR  512 // bytes
#define MAX_SRV_CLIENTS 4
WiFiServer tcpServer(23);
WiFiClient tcpClients[MAX_SRV_CLIENTS];
uint8_t client_enable[MAX_SRV_CLIENTS];
uint16_t client_read[MAX_SRV_CLIENTS];
uint32_t client_ms[MAX_SRV_CLIENTS];
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
        client_read[i] = 0;
        client_ms[i] = millis();
        if (proc == OTA_MODE)
          client_enable[i] = 1;
        else {
          client_enable[i] = 0; //等待web认证
          tcpClients[i].println("to web, enable this link.#" + String(i + 1));
        }
        break;
      }

    //没有位置， busy ,stop();
    if (i == MAX_SRV_CLIENTS) {
      tcpServer.available().println("busy");
    }
  }
  yield();
  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (client_enable[i] && tcpClients[i].availableForWrite()) { //tcp可以写
      if (client_enable[i] == 2) {
        tcpClients[i].println("--ok! enabled. #" + String(i + 1)+"--");
        client_enable[i] = 1;
      }
      while (tcpClients[i].available() && Serial.availableForWrite() > 0) { //tcp可以读， 串口可以写
        sbuf[0] = tcpClients[i].read();
        Serial.write(sbuf[0]);
        for (int i1 = 0; i1 < MAX_SRV_CLIENTS; i1++)
          if (i1 != i && tcpClients[i] == 1) tcpClients[i1].write(sbuf[0]);
        client_read[i]++;
      }
    }
  }
  yield();
  size_t len = Serial.available();
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    if (tcpClients[i]) {
      size_t afw = tcpClients[i].availableForWrite(); //tcp可以写
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
void net_log(String msg) {
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
      if (client_enable[i] && tcpClients[i].availableForWrite())
          tcpClients[i].println("--"+msg+"--");
}
#endif //__PROC_H__
