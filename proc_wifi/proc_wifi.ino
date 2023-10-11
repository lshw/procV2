#include <FS.h>
#define HOSTNAME "proc_"
extern "C" {
#include "user_interface.h"
}
#include "config.h"
#include "global.h"
#include "ntp.h"
String hostname = HOSTNAME;
uint32_t ota_timeout;

#include "ota.h"
#include "wifi_client.h"
#include "httpd.h"
#include "ht16c21.h"
#include "proc.h"
#include <Pinger.h>
Pinger pinger;
int8_t ping_status = 0;

void setup()
{
  load_nvram();
  if (nvram.nvram7 & NVRAM7_24V) //lcd_ram[7] bit3是24V输出状态
    _24v_out = HIGH;
  else
    _24v_out = LOW;
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
  get_pcb_ver();
  analogWriteFreq(400);
  analogWrite(PWM, nvram.pwm);
  pinMode(_24V_OUT, OUTPUT);
  digitalWrite(_24V_OUT, _24v_out);
  pinMode(PC_RESET, OUTPUT);
  digitalWrite(PC_RESET, LOW);
  pinMode(PC_POWER, OUTPUT);
  digitalWrite(PC_POWER, LOW);
  ht16c21_setup();
  ht16c21_cmd(0x88, 1); //闪烁
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
  get_comset();
  get_otherset();
  Serial.begin(rate, comsets[comset]);
  get_pcb_ver();
  if (nvram.nvram7 & NVRAM7_UPDATE) {
    disp(F("-" VER "-"));
    nvram.nvram7 &= ~ NVRAM7_UPDATE;
    nvram.change = 1;
    delay(1000);
  }
  _myTicker.attach(1, timer1s);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  get_network();
  get_batt();
  get_day_cron();
  proc = nvram.proc;
  if (millis() > 10000) proc = 0; //程序升级后第一次启动
  switch (proc) {
    case OTA_MODE:
      wifi_setup();
      ota_timeout = 3600000; //1小时
      wdt_disable();
      nvram.proc = SMART_CONFIG_MODE;//ota以后，
      nvram.change = 1;
      save_nvram();
      disp(F(" OTA "));
      delay(1000);
      nvram.proc = 0;
      nvram.change = 1;
      save_nvram();
      ota_setup();
      httpd_listen();
      break;
    case SMART_CONFIG_MODE:
      nvram.proc = 0;//ota以后，
      nvram.change = 1;
      save_nvram();
      disp(F("CO  "));
      delay(1000);
      nvram.proc = 0;
      nvram.change = 1;
      save_nvram();
      smart_config();
      ESP.restart();
      break;
    default:
      wifi_setup();
      proc_setup();
      if (nvram.proc != OTA_MODE) {
        nvram.proc = OTA_MODE;
        nvram.change = 1;
        save_nvram();
      }
      sprintf(disp_buf, " %3.2f ", v);
      disp(disp_buf);
      delay(1000);
      nvram.proc = 0;
      nvram.change = 1;
      save_nvram();
      httpd_listen();
      break;
  }
  wait_connected(2000);
  save_nvram();
  update_disp();
  zmd();
  update_time = get_update_time();//检查升级的间隔小时
  if (update_time == 0)
    update_timeok = -1; //如果是0,关闭自动更新
  else
    update_timeok = 0; //开机运行一次
  get_http_auth();
  for (uint8_t i = 0; i < 10; i++) {
    delay(200);
    yield();
  }
  get_mylink(); //定制的web左下角
  pinger.OnEnd([](const PingerResponse & response) {
    if (response.TotalSentRequests == response.TotalReceivedResponses)
      ping_status = 1;
    else
      ping_status = -1;
    return true;
  });
  wait_connected(5000);
}

bool httpd_up = false;
uint32_t day_cron_delay = 0; //每天执行完day_cron_delay后， 要延迟一下
void loop()
{
  switch (proc) {
    case OTA_MODE:
      if (ota_timeout < millis()) {
        nvram.proc = 0;
        proc = 0;
        nvram.change = 1;
        save_nvram();
        ESP.restart();
        return;
      }
      if (WiFi.smartConfigDone()) {
        wifi_set_clean();
        wifi_set_add(WiFi.SSID().c_str(), WiFi.psk().c_str());
        Serial.println(F("Smartconfig OK"));
        disp(F("6.6.6.6.6."));
        proc = 0;
        nvram.change = 1;
        save_nvram();
        ESP.restart();
        return;
      }
      httpd_loop();
      if (WiFi_isConnected()) {
        ota_loop();
      }
      ap_loop();
      break;
    default:
      if (WiFi_isConnected()) {
        httpd_loop();
        proc_loop();
      } else if (millis() > 30000) { //30秒不能登陆网络进smart_config
        smart_config();
        ESP.restart();
      }
  }
  yield();//先看看其它任务， 有啥需要忙的
  if (run_zmd) {
    run_zmd = false;
    zmd();
  }
#ifdef HAVE_AUTO_UPDATE
  if (update_timeok == 0) {
    if (update_time > 0)
      update_timeok = update_time * 60;
    else
      update_timeok = -1; //停止;
    yield();
    wget();
    yield();
  }
#endif
  if (nvram.change != 0) save_nvram();
  if (set_change) set_save();
  system_soft_wdt_feed();
  if (day_cron_delay < millis()) {
    if ((hour == day_cron_hour || day_cron_hour >= 60) && minute == day_cron_minute) {
      day_cron_delay = millis() + 36000000 ; //10小时
      ping_powerup();
    }
  }
  if (millis() > 0xf0000000)
    ESP.restart();  //防止millis() 溢出
}
void ping_powerup() {
  uint32_t ms0;
  ping_status = 0;
  if (master_ip.toString() != String("0.0.0.0") && pinger.Ping(master_ip)) {
    ms0 = millis() + 30000; //等待30秒 ping回应
    while (ms0 > millis()) {
      if (ping_status != 0) break;
      yield();
    }
    if (ping_status == -1) {
      net_log(F("ping ") + master_ip.toString() + F(" fail, power up!"));
      disp(F("11111"));
      digitalWrite(PC_POWER, HIGH);
      pcPowerTicker.detach();
      pcPowerTicker.attach_ms(300, pcPowerUp);
      yield();
    } else
      net_log(F("ping ") + master_ip.toString() + F(" ok, skip power up!"));
  }
}
bool smart_config() {
  //插上电， 等20秒， 如果没有上网成功， 就会进入 CO xx计数， 100秒之内完成下面的操作
  //手机连上2.4G的wifi,然后微信打开网页：http://wx.ai-thinker.com/api/old/wifi/config
  nvram.proc = 0;
  nvram.change = 1;
  if (connected_is_ok) return true;
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  Serial.println(F("SmartConfig start"));
  for (uint8_t i = 0; i < 100; i++) {
    if (WiFi.smartConfigDone()) {
      wifi_set_clean();
      wifi_set_add(WiFi.SSID().c_str(), WiFi.psk().c_str());
      Serial.println(F("OK"));
      return true;
    }
    Serial.write('.');
    delay(1000);
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("CON%02d"), i);
    disp(disp_buf);
  }
  snprintf_P(disp_buf, sizeof(disp_buf), PSTR(" %3.2f "), v);
  disp(disp_buf);
  return false;
}
