#include <FS.h>
#define VER "1.55"
#define HOSTNAME "proc_"
extern "C" {
#include "user_interface.h"
}
#include "config.h"
#include "global.h"

bool lcd_flash = false;
extern char ip_buf[30];
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
char disp_buf[22];
uint32_t next_disp = 1800; //下次开机
String hostname = HOSTNAME;
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
//0,1-正常 2-OTA
#define OTA_MODE 2

#include "ota.h"
#include "wifi_client.h"
#include "ap_web.h"
#include "ht16c21.h"
#include "proc.h"
void setup()
{
  uint8_t i;
  ip_buf[0] = 0;
  pinMode(_24V_OUT, OUTPUT);
  digitalWrite(_24V_OUT, HIGH); //默认24V开启输出
  pinMode(PC_RESET, OUTPUT);
  digitalWrite(PC_RESET, LOW);
  pinMode(PC_POWER, OUTPUT);
  digitalWrite(PC_POWER, LOW);
  _myTicker.attach(1,timer1s);
  _myTicker.attach(0.5,test);
  Serial.begin(115200);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  wifi_setup();
  ht16c21_setup();
  ht16c21_cmd(0x88, 1); //闪烁
  get_batt();
  proc = ram_buf[0];
  if (millis() > 10000) proc = 0; //程序升级后第一次启动
  switch (proc) {
    case OTA_MODE:
      wdt_disable();
      ram_buf[0] = 0;//ota以后，
      disp(" OTA ");
      ota_setup();
      return;
      break;
    default:
      proc_setup();
      ram_buf[0] = OTA_MODE;
      sprintf(disp_buf, " %3.2f ", v);
      disp(disp_buf);
      break;
  }
  send_ram();
  wget();
  wifi_set_sleep_type(LIGHT_SLEEP_T);
}

void loop()
{
  switch (proc) {
    case OTA_MODE:
      if (WiFiMulti.run() == WL_CONNECTED)
        ota_loop();
      else
        ap_loop();
      break;
  }
}
