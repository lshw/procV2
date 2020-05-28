#ifndef __OTA_H__
#define __OTA_H__
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include "httpd.h"
extern void disp(char *);
extern float get_batt();
extern uint32_t ap_on_time;
extern char ram_buf[10];
extern DNSServer dnsServer;
void send_ram();
void ota_setup() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    send_ram();
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    type = "";
  });
  ArduinoOTA.onEnd([]() {
    ht16c21_cmd(0x88, 1); //闪烁
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    sprintf(disp_buf, "OTA.%02d", progress * 99 / total );
    disp(disp_buf);
  });
  ArduinoOTA.begin();
}
void ota_loop() {
  if ( millis() > ap_on_time) {
    if (millis() < 1800000 ) ap_on_time = millis() + 200000; //有外接电源的情况下，最长半小时
    else {
      ram_buf[0] = 0;
      send_ram();
      delay(2000);
      ESP.restart();
      return;
    }
  }
  if (millis() < 600000) {
    dnsServer.processNextRequest();
    ArduinoOTA.handle();
    httpd_loop();
  } else {
    ht16c21_cmd(0x88, 0); //不闪烁
    ESP.restart();
  }
  return;
}

#endif //__OTA_H__
