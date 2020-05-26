#ifndef __OTA_H__
#define __OTA_H__
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include "ap_web.h"
char ip_buf[30];
uint8_t ip_offset, ip_len;
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
    ram_buf[0] = 0;
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
  wifi_set_sleep_type(LIGHT_SLEEP_T);
}
void zmd() {
  uint8_t i, i0, i1;
  i = ip_offset;
  i0 = 0;
  i1 = 0;
  while (i1 < 5) {
    i = i % ip_len;
    disp_buf[i0] = ip_buf[i];
    disp_buf[i0 + 1] = 0;
    i++;
    i0++;
    if (ip_buf[i] != '.') i1++;
  }
  i = strlen(disp_buf) - 1;
  if (disp_buf[i] == '.') disp_buf[i] = 0; //最后一个数字不能带小数点 //显示屏的最后一个数字无小数点
  ip_offset = (ip_offset + 1) % ip_len;
  while (ip_buf[ip_offset] == '.' || ip_buf[ip_offset + 1] == '.')
    ip_offset = (ip_offset + 1) % ip_len; //第一个字符是点，跳过
}
uint16_t sec0, sec1;
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
  if (ip_buf[0] == 0)
  {
    snprintf(ip_buf, sizeof(ip_buf), "OTA %s     ", WiFi.localIP().toString().c_str());
    ip_len = strlen(ip_buf);
    ip_offset = 0;
  }
  if (millis() < 600000) {
    sec0 = millis() / 1000;
    if (sec0 != sec1) {
      get_batt();
      zmd(); //"OTA 192.168.12.126  " 走马灯填充disp_buf
      sec1 = sec0;
      if (sec0 > 5)
        ram_buf[0] = 0;
      disp(disp_buf);
      system_soft_wdt_feed ();
    }
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
