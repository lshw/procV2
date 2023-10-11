#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "CRC32.h"
CRC32 crc;
#include "config.h"
#include "nvram.h"
#include "ht16c21.h"
#include "Ticker.h"
uint8_t pcb_ver = 0;
char disp_buf[22];
bool telnet_auth = false;
bool run_zmd = true;
uint8_t update_time;
int16_t update_timeok = 0; //0-马上wget ，-1 关闭，>0  xx分钟后wget
char www_username[100] = "root";
char www_password[100] = "admin";
Ticker _myTicker, pcResetTicker, pcPowerTicker, ota_test;
extern String mylink;
extern char day_cron[6];
extern bool connected_is_ok;
extern int8_t day_cron_hour, day_cron_minute;
extern uint32_t day_cron_delay; //每天执行完day_cron_delay后， 要延迟一下
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
//0,1-正常 2-OTA
#define OTA_MODE 2
#define SMART_CONFIG_MODE 3
uint8_t _24v_out;
String ntpServerName[5] = {
  "",
  "3.debian.pool.ntp.org",
  "1.openwrt.pool.ntp.org"
  "1.debian.pool.ntp.org",
  "2.debian.pool.ntp.org",
};
bool is_dhcp = true;
IPAddress master_ip = {0, 0, 0, 0};
IPAddress local_ip = {192, 168, 1, 2};
IPAddress netmask = {255, 255, 255, 0};
IPAddress gateway = {192, 168, 1, 1};
IPAddress dns = {8, 8, 8, 8};
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
void test();
uint8_t test_t = 0;
float get_batt();
float v;
uint8_t year, month = 1, day = 1, hour = 0, minute = 0, sec = 0;
DNSServer dnsServer;
void update_disp() {
  uint8_t zmdsize = strlen(zmd_disp);
  if (connected_is_ok) {
    if (proc == OTA_MODE) {
      snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" OTA %s -" VER "-  "), WiFi.localIP().toString().c_str());
    } else {
      if (year != 0)
        snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" %s   20%02d-%02d-%02d %02d-%02d  "), year, month, day, hour, minute, WiFi.localIP().toString().c_str());
      else
        snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" %s "), WiFi.localIP().toString().c_str());
    }
  } else {
    if (proc == OTA_MODE)
      snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" AP -%s- "), VER);
    else
      snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" %3.2f -%s-  "), v, VER);
  }
  if (zmdsize != strlen(zmd_disp)) zmd_offset = 0; //长度有变化， 就从头开始显示
}
void timer1s() {
  char mdays;
  sec++;
  if (sec >= 60) {
    sec = 0;
    minute++;
    if (update_timeok > 0) update_timeok--;//定时器2 链接远程服务器
    if (minute >= 60) {
      minute = 0;
      hour++;
      if (hour >= 24) {
        hour = 0;
        day++;
        if (day >= 28) {
          mdays = 31;
          switch (month) {
            case 4:
            case 6:
            case 9:
            case 11:
              mdays = 30;
              break;
            case 2:
              if (year % 4 != 0) mdays = 28;
              else if (year % 100 == 0 && year % 400 != 0) mdays = 28;
              else mdays = 29;
              break;
          }
          if (day > mdays) {
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
uint8_t get_pcb_ver() {
  if (pcb_ver == 0) {
    Serial.begin(115200);
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
    if (digitalRead(2) == HIGH)
      pcb_ver = 1;
    else
      pcb_ver = 2;
  }
  pinMode(2, INPUT);
  return pcb_ver;
}
#ifdef HAVE_AUTO_UPDATE
void wget() {
  uint16_t httpCode = http_get( nvram.nvram7 & NVRAM7_URL); //先试试上次成功的url
  if (httpCode < 200  || httpCode >= 400) {
    nvram.nvram7 = (nvram.nvram7 & ~ NVRAM7_URL) | (~ nvram.nvram7 & NVRAM7_URL);
    nvram.change = 1;
    httpCode = http_get(nvram.nvram7 & NVRAM7_URL); //再试试另一个的url
  }
}
#endif
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
  float r18 = get_R18();
  v = (float) dat / 8 * (1000.0 + r18) / r18 / 1023 ;
  return v;
}
#ifdef HAVE_AUTO_UPDATE
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
#endif
String get_ssid() {
  File fp;
  String ssid = F("test:cfido.com");
  if (!SPIFFS.begin()) return ssid;
  fp = SPIFFS.open("/ssid.txt", "r");
  if (fp) {
    ssid = fp.readString();
    fp.close();
  }
  return ssid;
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
void get_otherset() {
  File fp;
  if (!SPIFFS.begin()) return;
  fp = SPIFFS.open("/other.txt", "r");
  if (fp) {
    master_ip.fromString(fp_gets(fp));
    fp.close();
  }
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
    ntpServerName[0] = fp_gets(fp);
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
  mylink.trim();
  if (mylink == "") mylink = F("在线文档:\r<a href=https://www.bjlx.org.cn/node/929>https://www.bjlx.org.cn/node/929</a>");
}

void get_day_cron() {
  File fp;
  if (!SPIFFS.begin()) return;
  fp = SPIFFS.open("/day_cron", "r");
  if (fp) {
    strncpy(day_cron, fp_gets(fp).c_str(), sizeof(day_cron)); //hh:mm
    day_cron_hour = atoi(day_cron);
    day_cron_minute = atoi(&day_cron[3]);
    fp.close();
  } else {
    day_cron_hour = -1;
    day_cron_minute = -1;
  }
  return;
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

void zmd() {  //1s 一次Ticker
  uint8_t i = 0, i0 = 0;
  if (!connected_is_ok) return;
  zmd_size = strlen(zmd_disp);
  if (zmd_size == 0) return;
  if (zmd_size < zmd_offset) zmd_offset = 0;
  for (i = 0; i < 10; i++) { //跳过第一个点
    if (zmd_disp[zmd_offset] != '.') break;
    zmd_offset = (zmd_offset + 1) % zmd_size;
  }

  memset(disp_buf, 0, sizeof(disp_buf));
  for (i = 0; i < sizeof(disp_buf); i++) {
    disp_buf[i] = zmd_disp[(zmd_offset + i) % zmd_size];
    if (disp_buf[i] != '.') i0++;
    if (i0 >= 5) break;
  }
  if (disp_buf[1] == '.') { //第一个字母后面是点，就把第一个字母，显示为空格
    for (i = 0; i < sizeof(disp_buf) - 1; i++)
      disp_buf[i] = disp_buf[i + 1];
    disp_buf[0] = ' ';
  }
  disp(disp_buf);
  zmd_offset = (zmd_offset + 1) % zmd_size;
}

uint8_t set_change = 0;
#define COM_CHANGE 1
#define NET_CHANGE 2
#define OTHER_CHANGE 4
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
    fp.println(ntpServerName[0]);
    fp.close();
    ESP.restart();
  }
  if (set_change & OTHER_CHANGE) {
    fp = SPIFFS.open("/other.txt", "w");
    fp.println(master_ip);
    fp.close();
  }
  set_change = 0;
}

void wifi_set_clean() {
  if (SPIFFS.begin()) {
    SPIFFS.remove("/ssid.txt");
    SPIFFS.end();
  }
}
void  wifi_set_add(const char * wps_ssid, const char * wps_password) {
  File fp;
  int8_t mh_offset;
  String wifi_sets, line;
  if (wps_ssid[0] == 0) return;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    wifi_sets = String(wps_ssid) + ":" + String(wps_password) + "\r\n";
    if (fp) {
      while (fp.available()) {
        line = fp.readStringUntil('\n');
        line.trim();
        if (line == "")
          continue;
        if (line.length() > 110)
          line = line.substring(0, 110);
        mh_offset = line.indexOf(':');
        if (mh_offset < 2) continue;
        if (line.substring(0, mh_offset) == wps_ssid)
          continue;
        else
          wifi_sets += line + "\r\n";
      }
      fp.close();
    }
    fp = SPIFFS.open("/ssid.txt", "w");
    if (fp) {
      fp.print(wifi_sets);
      fp.close();
    }
    SPIFFS.end();
  }
}
#define __YEAR__ ((((__DATE__[7]-'0')*10+(__DATE__[8]-'0'))*10 \
                   +(__DATE__[9]-'0'))*10+(__DATE__[10]-'0'))

#define __MONTH__ (__DATE__[2]=='n'?(__DATE__[1]=='a'?1:6)   /*Jan:Jun*/  \
                   :__DATE__[2]=='b'?2 \
                   :__DATE__[2]=='r'?(__DATE__[0]=='M'?3:4) \
                   :__DATE__[2]=='y'?5 \
                   :__DATE__[2]=='l'?7 \
                   :__DATE__[2]=='g'?8 \
                   :__DATE__[2]=='p'?9 \
                   :__DATE__[2]=='t'?10 \
                   :__DATE__[2]=='v'?11:12)

#define __DAY__ ((__DATE__[4]==' '?0:__DATE__[4]-'0')*10 \
                 +(__DATE__[5]-'0'))


#endif
