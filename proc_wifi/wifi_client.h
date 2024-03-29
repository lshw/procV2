#ifndef __WIFI_CLIENT_H__
#define __WIFI_CLIENT_H__
#include "config.h"
#include "nvram.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
void AP();
bool http_update();
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
ESP8266WiFiMulti WiFiMulti;
HTTPClient http;
String ssid, passwd;
uint8_t hex2ch(char dat) {
  dat |= 0x20; //41->61 A->a
  if (dat >= 'a') return dat - 'a' + 10;
  return dat - '0';
}
void wifi_setup() {
  File fp;
  uint32_t i;
  char buf[3];
  char ch;
  boolean is_ssid = true;
  WiFi.setAutoConnect(true);//自动链接上次
  WiFi.setAutoReconnect(true);//断线自动重连
  wifi_station_set_auto_connect(1);
  wifi_station_set_reconnect_policy(true);
  if (proc == OTA_MODE) { //ota时要 ap 和 client
    WiFi.mode(WIFI_AP_STA);
    AP();
  } else  { //测温时， 只用client
    WiFi.mode(WIFI_STA);
  }
  if (!is_dhcp)
    WiFi.config(local_ip, dns, gateway, netmask);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  if (SPIFFS.begin()) {
    if (!SPIFFS.exists("/ssid.txt")) {
      fp = SPIFFS.open("/ssid.txt", "w");
      fp.println("test:cfido.com");
      fp.close();
    }
    fp = SPIFFS.open("/ssid.txt", "r");
    ssid = "";
    passwd = "";
    if (fp) {
      uint16_t Fsize = fp.size();
      for (i = 0; i < Fsize; i++) {
        ch = fp.read();
        switch (ch) {
          case 0xd:
          case 0xa:
            if (ssid != "") {
              WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
            }
            is_ssid = true;
            ssid = "";
            passwd = "";
            break;
          case ' ':
          case ':':
            is_ssid = false;
            break;
          default:
            if (is_ssid)
              ssid += ch;
            else
              passwd += ch;
        }
      }
      if (ssid != "" && passwd != "") {
        WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
      }
    }
    fp.close();
    SPIFFS.end();
  }
}

#ifdef HAVE_AUTO_UPDATE
uint16_t http_get(uint8_t no) {
  char key[17];
  WiFiClient client;
  if (no > 1) no = 1;
  String url0 = get_url(no);
  if (url0.indexOf('?') > 0)
    url0 += '&';
  else
    url0 += '?';
#ifndef VERA
#define VERA
#endif
  url0 += "ver="  VER VERA "&sn=" + hostname ;

  http.begin(client, url0 ); //HTTP提交
  http.setTimeout(4000);
  int httpCode;
  for (uint8_t i = 0; i < 10; i++) {
    httpCode = http.GET();
    if (httpCode < 0) {
      delay(20);
      continue;
    }
    if (httpCode >= 200 && httpCode <= 299) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        payload.toCharArray(disp_buf, 15); //.1.2.3.4.5,1800
        uint8_t    i1 = payload.indexOf(',');
        if (  disp_buf[0] == 'U'
              && disp_buf[1] == 'P'
              && disp_buf[2] == 'D'
              && disp_buf[3] == 'A'
              && disp_buf[4] == 'T'
              && disp_buf[5] == 'E') {
          SPIFFS.begin();
          updating = true;
          if (http_update() == false)
            http_update();
          delay(180);
          ESP.restart();
        }
        disp_buf[i1] = 0;
        disp(disp_buf);
        break;
      }
    } else {

      if (httpCode > 0)
        sprintf(disp_buf, ".E %03d", httpCode);
      disp(disp_buf);
      break;
    }
  }
  url0 = "";
  return httpCode;
}

void update_progress(int cur, int total) {
  char disp_buf[6];
  sprintf(disp_buf, "HUP.%02d", cur * 99 / total);
  disp(disp_buf);
}

bool http_update()
{
  disp(F("H UP. "));
  if (nvram.nvram7 & NVRAM7_UPDATE) {
    nvram.proc = 0;
    nvram.nvram7 |= NVRAM7_UPDATE;
    nvram.change = 1;
  }
  WiFiClient client;
  String update_url = F("http://www.anheng.com.cn/firmware.php?type=procV2&SN=") + hostname + F("&GIT=" GIT_VER "&ver=" VER); //php可以在header里下发X-MD5作为校验
  ESPhttpUpdate.onProgress(update_progress);
  t_httpUpdate_return  ret = ESPhttpUpdate.update(client, update_url);
  update_url = "";

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      ESP.restart();
      break;

    case HTTP_UPDATE_NO_UPDATES:
      break;

    case HTTP_UPDATE_OK:
      ht16c21_cmd(0x88, 1); //闪烁
      return true;
      break;
  }
  delay(1000);
  return false;
}
#endif //HAVE_AUTO_UPDATE

bool connected_is_ok = false;
bool fast_wifi = true;
bool WiFi_isConnected() {
  yield();
  if (connected_is_ok) {
    ht16c21_cmd(0x88, 0); //停止闪烁
    return connected_is_ok;
  }
  if (WiFiMulti.run() == WL_CONNECTED) {
    connected_is_ok = true;
    ntpclient();
#ifdef HAVE_AUTO_UPDATE
    wget();
#endif
    update_disp();
    ht16c21_cmd(0x88, 0); //停止闪烁
    return true;
  }
  ht16c21_cmd(0x88, 1); //开始闪烁
  return false;
}
bool wait_connected(uint16_t ms) {
  while (millis() < ms && !WiFi_isConnected()) {
    yield();
    delay(200);
  }
  return WiFi_isConnected();
}
#endif __WIFI_CLIENT_H__
