#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "config.h"
#include "ht16c21.h"
#include "Ticker.h"
uint16_t timer1 = 0; //秒 定时测温
uint16_t timer2 = 0; //秒
char disp_buf[22];
bool telnet_auth = false;
bool run_zmd = true;
uint8_t update_time;
int16_t update_timeok = 0; //0-马上wget ，-1 关闭，>0  xx分钟后wget

char www_username[100] = "root";
char www_password[100] = "admin";
Ticker _myTicker, pcResetTicker, pcPowerTicker, ota_test;
extern char ram_buf[10];
extern char disp_buf[22];
extern String mylink;
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
//0,1-正常 2-OTA
#define OTA_MODE 2
uint8_t _24v_out;
bool is_dhcp = true;
IPAddress local_ip = {192, 168, 1, 2};
IPAddress netmask = {255, 255, 255, 0};
IPAddress gateway = {192, 168, 1, 1};
IPAddress dns = {8, 8, 8, 8};
String ntp_server;
uint32_t rate = 115200; //串口速率
uint8_t comset;
String comset_str[] = {
  "8N1",
  "7N1",
  "6N1",
  "5N1",
  "8N2",
  "7N2",
  "6N2",
  "5N2",
  "8E1",
  "7E1",
  "6E1",
  "5E1",
  "8E2",
  "7E2",
  "6E2",
  "5E2",
  "8O1",
  "7O1",
  "6O1",
  "5O1",
  "8O2",
  "7O2",
  "6O2",
  "5O2"
};
SerialConfig comsets[] {
  SERIAL_8N1,
  SERIAL_7N1,
  SERIAL_6N1,
  SERIAL_5N1,
  SERIAL_8N2,
  SERIAL_7N2,
  SERIAL_6N2,
  SERIAL_5N2,
  SERIAL_8E1,
  SERIAL_6E1,
  SERIAL_7E1,
  SERIAL_5E1,
  SERIAL_8E2,
  SERIAL_7E2,
  SERIAL_6E2,
  SERIAL_5E2,
  SERIAL_8O1,
  SERIAL_7O1,
  SERIAL_6O1,
  SERIAL_5O1,
  SERIAL_8O2,
  SERIAL_7O2,
  SERIAL_6O2,
  SERIAL_5O2
};


#define ZMD_BUF_SIZE 100
char zmd_disp[ZMD_BUF_SIZE];
uint8_t zmd_offset = 0, zmd_size = 0;
uint16_t http_get(uint8_t);
void send_ram();
void test();
void zmd();
uint8_t test_t = 0;
float get_batt();
float v;
uint8_t year, month = 1, day = 1, hour = 0, minute = 0, sec = 0;
uint8_t timer3 = 10;
DNSServer dnsServer;
bool wifi_connected_is_ok();
void update_disp() {
  if (wifi_connected_is_ok()) {
    if (proc == OTA_MODE) {
      snprintf(zmd_disp, sizeof(zmd_disp), " OTA %s -%s-  ", WiFi.localIP().toString().c_str(), VER);
    } else {
      if (year != 0)
        snprintf(zmd_disp, sizeof(zmd_disp), " 20%02d-%02d-%02d %02d-%02d  %s  ", year, month, day, hour, minute, WiFi.localIP().toString().c_str());
      else
        snprintf(zmd_disp, sizeof(zmd_disp), " %s ", WiFi.localIP().toString().c_str());
    }
  } else {
    if (proc == OTA_MODE)
      snprintf(zmd_disp, sizeof(zmd_disp), " AP -%s- ", VER);
    else
      snprintf(zmd_disp, sizeof(zmd_disp), " %3.2f -%s-  ", v, VER);
  }
}
void timer1s() {
  char disp_buf[20];
  if (timer3 > 0) {
    if (timer3 == 1) {
      ram_buf[0] = 0;
      send_ram();
    }
    timer3--;
  }
  if (timer1 > 0) timer1--;//定时器1 测温
  if (timer2 > 0) timer2--;//定时器2
  sec++;
  if (sec >= 60) {
    sec = 0;
    minute++;
    if (update_timeok > 0) update_timeok--;//定时器2 链接远程服务器
    if (minute >= 60) {
      test_t = 0;
      ota_test.attach(0.5, test);
      minute = 0;
      hour++;
      if (hour >= 24) {
        hour = 0;
        day++;
        if (day >= 28) {
          disp_buf[0] = 31;
          switch (month) {
            case 4:
            case 6:
            case 9:
            case 11:
              disp_buf[0] = 30;
              break;
            case 2:
              if (year % 4 != 0) disp_buf[0] = 28;
              else if (year % 100 == 0 && year % 400 != 0) disp_buf[0] = 28;
              else disp_buf[0] = 29;
              break;
          }
          if (day > disp_buf[0]) {
            month++;
            day = 1;
            if (month > 12) {
              year++;
              month = 1;
            }
          }
        }
      }
    }
    get_batt();
    update_disp();
  }
  run_zmd = true;
}
void wget() {
  uint16_t httpCode = http_get((ram_buf[7] >> 1) & 1); //先试试上次成功的url
  if (httpCode < 200  || httpCode >= 400) {
    httpCode = http_get((~ram_buf[7] >> 1) & 1); //再试试另一个的url
  }
}
void test() {
  if (test_t > 22)  {
    pinMode(PWM, OUTPUT);
    digitalWrite(PWM, LOW);
    ota_test.detach();
    return;
  }
  if (test_t == 20) {
    test_t++;
    digitalWrite(PC_POWER, LOW);
    digitalWrite(_24V_OUT, LOW);
    digitalWrite(PC_RESET, LOW);
    analogWrite(PWM, 20);
    return;
  }
  if (test_t == 0) {
    analogWriteFreq(400);
    analogWrite(PWM, 512);
    digitalWrite(PC_POWER, LOW);
    digitalWrite(_24V_OUT, HIGH);
    digitalWrite(PC_RESET, LOW);
  }
  test_t++;
  bool s = digitalRead(_24V_OUT);
  digitalWrite(_24V_OUT, digitalRead(PC_POWER));
  digitalWrite(PC_POWER, digitalRead(PC_RESET));
  digitalWrite(PC_RESET, s);
  if (test_t <= 20) {
    if (test_t % 2) analogWrite(PWM, 512 + (20 - test_t) * 10);
    else analogWrite(PWM, 512 - (20 - test_t) * 10);
  }
}
void poweroff(uint32_t sec) {
  delay(sec * 1000);
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
  if (!SPIFFS.begin()) return ret;
  if (no == 0 || no == '0')
    fp = SPIFFS.open("/url.txt", "r");
  else
    fp = SPIFFS.open("/url1.txt", "r");
  if (fp) {
    ret = fp.readStringUntil('\n');
    ret.trim();
    fp.close();
  }
  return ret;
}
String get_ssid() {
  File fp;
  String ssid = "test:cfido.com";
  if (!SPIFFS.begin()) return ssid;
  fp = SPIFFS.open("/ssid.txt", "r");
  if (fp) {
    ssid = fp.readString();
    fp.close();
  }
  return ssid;
}

void get_lcd_ram_7() {
  File fp;
  if (!SPIFFS.begin()) return;
  fp = SPIFFS.open("/lcd_ram_7.txt", "r");
  if (fp) {
    ram_buf[7] = fp.read();
    fp.close();
    send_ram();
  }
}

uint8_t get_update_time() {
  File fp;
  uint8_t ret = 12;
  if (!SPIFFS.begin()) return ret;
  fp = SPIFFS.open("/update_time.txt", "r");
  if (fp) {
    ret = fp.readString().toInt();
    fp.close();
  }
  return ret;
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
void get_comset() {
  File fp;
  comset = 0;
  if (!SPIFFS.begin()) return;
  fp = SPIFFS.open("/comset.txt", "r");
  if (fp) {
    rate = fp_gets(fp).toInt();
    comset = fp_gets(fp).toInt();
    fp.close();
  }
  Serial.begin(rate, comsets[comset]);
}

void get_network() {
  File fp;
  comset = 0;
  if (!SPIFFS.begin()) return;
  fp = SPIFFS.open("/network.txt", "r");
  if (fp) {
    if (fp_gets(fp).toInt() == 0) is_dhcp = false;
    else is_dhcp = true;
    local_ip.fromString(fp_gets(fp));
    netmask.fromString(fp_gets(fp));
    gateway.fromString(fp_gets(fp));
    dns.fromString(fp_gets(fp));
    fp.close();
  }
}

void get_http_auth() {
  File fp;
  if (!SPIFFS.begin()) return;
  fp = SPIFFS.open("/http_auth.txt", "r");
  if (fp) {
    memset(www_username, 0, sizeof(www_username));
    memset(www_password, 0, sizeof(www_password));
    strncpy(www_username, fp_gets(fp).c_str(), sizeof(www_username));
    strncpy(www_password, fp_gets(fp).c_str(), sizeof(www_username));
    fp.close();
  }
  return;
}

void get_mylink() {
  File fp;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/mylink.txt", "r");
    if (fp) {
      mylink = fp.readString();
      fp.close();
    }
  }
  if (mylink == "") mylink = "在线文档:\r<a href=https://www.bjlx.org.cn/node/929>https://www.bjlx.org.cn/node/929</a>";
}

void AP() {
  // Go into software AP mode.
  struct softap_config cfgESP;

  while (!wifi_softap_get_config(&cfgESP)) {
    system_soft_wdt_feed();
  }
  cfgESP.authmode = AUTH_OPEN;//无密码模式
  wifi_softap_set_config(&cfgESP);
  delay(10);
  WiFi.softAP("proc", "");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
}

void pcPowerUp() {
  digitalWrite(PC_POWER, LOW);
  pcPowerTicker.detach();
}
void pcResetUp() {
  digitalWrite(PC_RESET, LOW);
  pcResetTicker.detach();
}

bool no_dot() {//判断显示是否有效,并填充disp_buf
  uint8_t i = 0, i0 = 0, dots[8];
  memset(disp_buf, 0, sizeof(disp_buf));
  //  memset(dots,0,sizeof(dots));
  while (i < 5) {
    if (zmd_disp[zmd_offset + i0] == '.') dots[i0] = '.';
    else {
      dots[i0] = '0';
      i++;
    }
    disp_buf[i0] = zmd_disp[(zmd_offset + i0) % zmd_size];
    i0++;
    if (i0 == zmd_size) return true; //循环了
  }// disp[]='.123.45.' -> dots[]='.00.0.'
  if (dots[0] == '.' || dots[1] == '.' || dots[5] == '.' || dots[6] == '.' )
    return false;
  else
    return true;
}

void zmd() {  //1s 一次Ticker
  if (!wifi_connected_is_ok()) return;
  zmd_size = strlen(zmd_disp);
  if (zmd_size == 0) return;
  if (zmd_size < zmd_offset) zmd_offset = 0;
  while (!no_dot())
    zmd_offset = (zmd_offset + 1) % zmd_size;
  disp(disp_buf);
  zmd_offset = (zmd_offset + 1) % zmd_size;
}
uint8_t set_change = 0;
#define COM_CHANGE 1
#define NET_CHANGE 2
void set_save() {
  File fp;
  if (!SPIFFS.begin()) return;
  if (set_change & COM_CHANGE) {
    fp = SPIFFS.open("/comset.txt", "w");
    fp.println(rate);
    fp.println(comset);
    fp.println(comset_str[comset]);
    fp.close();
    Serial.begin(rate, comsets[comset]);
    void update_head_footer();
  }
  if (set_change & NET_CHANGE) {
    fp = SPIFFS.open("/network.txt", "w");
    fp.println(is_dhcp);
    fp.println(local_ip);
    fp.println(netmask);
    fp.println(gateway);
    fp.println(dns);
    fp.println(ntp_server);
    fp.close();
    if (!is_dhcp)
      WiFi.config(local_ip, dns, gateway, netmask);
  }
  set_change = 0;
}
#endif
