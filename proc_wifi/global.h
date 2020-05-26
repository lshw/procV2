#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include "config.h"
#include "ht16c21.h"
#include "Ticker.h"
uint16_t timer1 = 0; //秒 定时测温
uint16_t timer2 = 0; //秒
Ticker _myTicker,pcResetTicker,pcPowerTicker,ota_test;
extern char ram_buf[10];
uint16_t http_get(uint8_t);
void send_ram();
float get_batt();
float v;
uint8_t hour=0,minute=0,sec=56;
void timer1s() {
  char disp_buf[6];
  if (timer1 > 0) timer1--;//定时器1 测温
  if (timer2 > 0) timer2--;//定时器2 链接远程服务器
  sec++;
  if(sec >= 60) {
    sec = 0;
    minute++;
    if (minute >= 60) {
      minute = 0;
      hour++;
      if(hour >= 24)
        hour = 0;
    }
    sprintf(disp_buf,"%02d-%02d",hour,minute);
    disp(disp_buf);
  }
}

void pcPowerUp() {
digitalWrite(PC_POWER,LOW);
}

void pcResetUp() {
digitalWrite(PC_RESET,LOW);
}

void wget() {
  uint16_t httpCode = http_get((ram_buf[7] >> 1) & 1); //先试试上次成功的url
  if (httpCode < 200  || httpCode >= 400) {
    httpCode = http_get((~ram_buf[7] >> 1) & 1); //再试试另一个的url
  }
}
uint8_t test_t=0;
void test(){
  if(test_t>20) return;
  if(test_t == 20) {
    test_t++;
    digitalWrite(PC_POWER,LOW);
    digitalWrite(_24V_OUT,LOW);
    digitalWrite(PC_RESET,LOW);
    return;
  }
  if(test_t == 0) {
    analogWriteFreq(400);
    analogWrite(PWM,512);
    digitalWrite(PC_POWER,LOW);
    digitalWrite(_24V_OUT,HIGH);
    digitalWrite(PC_RESET,LOW);
  }
  test_t++;
  bool s=digitalRead(_24V_OUT);
  digitalWrite(_24V_OUT,digitalRead(PC_POWER));
  digitalWrite(PC_POWER,digitalRead(PC_RESET));
  digitalWrite(PC_RESET,s);
  if(test_t % 2) analogWrite(PWM,512+(20-test_t)*20);
  else analogWrite(PWM,512-(20-test_t)*20);
}
void poweroff(uint32_t sec) {
  delay(sec*1000);
  ESP.restart();
}
float get_batt() {//电压
  uint32_t dat = analogRead(A0);
  dat = analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0);

  v = (float) dat / 8 * (1000.0 + 100.0) / 100.0 / 1023 ;
  return v;
}

String get_url(uint8_t no) {
  File fp;
  char fn[20];
  String ret;
  if (no == 0 || no == '0') ret = String(DEFAULT_URL0);
  else ret = String(DEFAULT_URL1);
  if (SPIFFS.begin()) {
    if (no == 0 || no == '0')
      fp = SPIFFS.open("/url.txt", "r");
    else
      fp = SPIFFS.open("/url1.txt", "r");
    if (fp) {
      ret = fp.readStringUntil('\n');
      ret.trim();
      fp.close();
    }
  }
  SPIFFS.end();
  return ret;
}
String get_ssid() {
  File fp;
  String ssid;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    if (fp) {
      ssid = fp.readString();
      fp.close();
    } else {
      fp = SPIFFS.open("/ssid.txt", "w");
      ssid = "test:cfido.com";
      fp.println(ssid);
      fp.close();
    }
  }
  SPIFFS.end();
  return ssid;
}

String fp_gets(File fp) {
  int ch = 0;
  String ret = "";
  while (1) {
    ch = fp.read();
    if (ch == -1) return ret;
    if (ch != 0xd && ch != 0xa) break;
  }
  while (ch != -1 && ch != 0xd && ch != 0xa) {
    ret += (char)ch;
    ch = fp.read();
  }
  ret.trim();
  return ret;
}
#endif
