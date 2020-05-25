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

#include "fs.h"
#include "ota.h"
#include "wifi_client.h"
#include "ap_web.h"
#include "ht16c21.h"
#include "proc.h"
void setup()
{
  uint8_t i;
  ip_buf[0] = 0;
  _myTicker.attach(1,timer1s);
  Serial.begin(115200);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  ht16c21_setup();
  get_batt();
  proc = ram_buf[0];
  proc_setup();
  if (millis() > 10000) proc = 0; //程序升级后第一次启动
  switch (proc) {
    case OTA_MODE:
      wdt_disable();
      ram_buf[7] |= 1; //充电
      ram_buf[0] = 0;//ota以后，
      disp(" OTA ");
      break;
    default:
      ram_buf[0] = OTA_MODE;
      sprintf(disp_buf, " %3.2f ", v);
      disp(disp_buf);
      test();
      break;
  }
  send_ram();
  //更新时闪烁
  ht16c21_cmd(0x88, 1); //闪烁
  if (wifi_connect()) {
  ht16c21_cmd(0x88, 0); //停止闪烁
  }

  if (proc == OTA_MODE) {
    ota_setup();
    return;
  }
  wget();
  wifi_set_sleep_type(LIGHT_SLEEP_T);
}

void loop()
{
//  proc_loop();
  switch (proc) {
    case OTA_MODE:
      if (WiFiMulti.run() == WL_CONNECTED)
        ota_loop();
      else
        ap_loop();
      break;
  }
}
