#include <FS.h>
#define VER "1.62"
#define HOSTNAME "proc_"
extern "C" {
#include "user_interface.h"
}
#include "config.h"
#include "global.h"
#include "ntp.h"
String hostname = HOSTNAME;

#include "ota.h"
#include "wifi_client.h"
#include "httpd.h"
#include "ht16c21.h"
#include "proc.h"
void setup()
{
  pinMode(_24V_OUT, OUTPUT);
  digitalWrite(_24V_OUT, HIGH); //默认24V开启输出
  pinMode(PC_RESET, OUTPUT);
  digitalWrite(PC_RESET, LOW);
  pinMode(PC_POWER, OUTPUT);
  digitalWrite(PC_POWER, LOW);
  _myTicker.attach(1, timer1s);
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
      ota_test.attach(0.5, test);
      ota_setup();
      httpd_listen();
      break;
    default:
      proc_setup();
      ram_buf[0] = OTA_MODE;
      sprintf(disp_buf, " %3.2f ", v);
      disp(disp_buf);
      break;
  }
  send_ram();
  update_disp();
  zmd();
  update_time = get_update_time();//检查升级的间隔小时
  if (update_time == 0)
    update_timeok = -1; //如果是0,关闭自动更新
  else
    update_timeok = 0; //开机运行一次
  get_http_auth();
  delay(1000);
}

bool httpd_up = false;
void loop()
{
  switch (proc) {
    case OTA_MODE:
      httpd_loop();
      if (wifi_connected_is_ok()) {
        if (!httpd_up) {
          ntpclient();
          wget();
          update_disp();
          zmd();
          httpd_up = true;
        }
        ota_loop();
      } else
        ap_loop();
      break;
    default:
      if (wifi_connected_is_ok()) {
        proc_loop();
        if (!httpd_up) {
          update_disp();
          ntpclient();
          wget();
          httpd_up = true;
          httpd_listen();
        }
        httpd_loop();
      }
  }
  yield();//先看看其它任务， 有啥需要忙的
  if (run_zmd) {
    run_zmd = false;
    zmd();
  }
  if (update_timeok == 0) {
    if (update_time > 0)
      update_timeok = update_time * 60;
    else
      update_timeok = -1; //停止;
    wget();
  }
  system_soft_wdt_feed ();
}
