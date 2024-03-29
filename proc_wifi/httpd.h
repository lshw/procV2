#ifndef __AP_WEB_H__
#define __AP_WEB_H__
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "wifi_client.h"
#include "global.h"
#include "proc.h"
#include "nvram.h"
extern void disp(char *);
extern String hostname;
float get_batt();
void ht16c21_cmd(uint8_t cmd, uint8_t dat);

ESP8266WebServer httpd(80);
String head, footer;
String body;
char day_cron[6];
int8_t day_cron_hour = -1, day_cron_minute = -1;

void httpd_send_200(String javascript) {
  ota_timeout = millis() + 3600000; //有web就延迟1小时
  httpd.sendHeader( "charset", "utf-8" );
  httpd.send(200, "text/html", F("<html>"
                                 "<head>"
                                 "<title>")
             + hostname + " " + GIT_VER
             + F("</title>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "<style type='text/css'>"
                 "hide {display:none;}"
                 "unhide {display:inline;}"
                 "</style>"
                 "<script>"
                 "function modi(url,text,Defaulttext) {"
                 "var data=prompt(text,Defaulttext);"
                 "if (data==null) {return false;}"
                 "location.replace(url+data);"
                 "}"
                 " pwm=")
             + String(nvram.pwm)
             + F(";"
                 " function ajax_get(url) {"
                 " xhr = new XMLHttpRequest();"
                 " xhr.open('GET', url, true);"
                 " xhr.setRequestHeader('Content-Type', 'text/html; charset=UTF-8');"
                 " xhr.send();"
                 " }"
                 " function set_val(id,val) {"
                 "document.getElementById(id).textContent=val;"
                 " }"
                 " function goto_if(url,msg) {"
                 " if(confirm(msg))"
                 " location.replace(url);"
                 " else return false;"
                 " return true;"
                 " }"
                 " function hide(id){"
                 " document.getElementById(id).style.display='none';"
                 " }"
                 " function unhide(id){"
                 " document.getElementById(id).style.display='inline';"
                 " }"
                 " function ajax_if(url, msg) {"
                 " if(confirm(msg))"
                 " ajax_get(url);"
                 " else return false;"
                 " return true;"
                 " }"
                 " function modi(url,text,defaultext) {"
                 " var data=prompt(text,defaultext);"
                 " if( data == defaultext)"
                 " return false;"
                 " location.replace(url+data);"
                 " return true;"
                 "}"
                 " function setpwm(val) {"
                 " if(val<1) val=1;"
                 " if(val>1019) val=1019;"
                 " pwm=val;"
                 " set_val('pwm','pwm:'+val);"
                 " ajax_get('/switch.php?b=PWM&t='+val);"
                 "}"
                 " function ajax_modi(url,text,defaultext) {"
                 " var data=prompt(text,defaultext);"
                 " if( data == defaultext)"
                 " return false;"
                 " ajax_get(url+data);"
                 " return true;"
                 "}")
             + javascript
             + F("</script>"
                 "</head>"
                 "<body>")
             + head
             + body
             + footer
             + F("</body>"
                 "</html>"));
  httpd.client().stop();
}

void update_head_footer() {
  char ymd[12];
  snprintf_P(ymd, sizeof(ymd), PSTR("%04d-%02d-%02d"), __YEAR__, __MONTH__, __DAY__);
  head =
    F("SN:<mark>") + hostname + F("</mark> &nbsp; "
                                  "软件版本:<mark>" VER "-" GIT_VER "</mark>&nbsp;&nbsp;硬件版本:<mark>2.") + String(pcb_ver) + F("</mark>&nbsp;&nbsp;串口:") + String(rate) + "," + comset_str[comset]
    + F("<br>"
        "<button onclick=ajax_if('/switch.php?b=RESET&t=300','复位电脑?')>短按复位</button>"
        "<button onclick=ajax_if('/switch.php?b=RESET&t=5000','复位电脑?')>长按复位</button>"
        "<button onclick=ajax_if('/switch.php?b=POWER&t=300','按下电源键?')>短按电源</button>"
        "<button onclick=ajax_if('/switch.php?b=POWER&t=5000','按下电源键?')>长按电源</button>"
        "<span style='white-space: nowrap;'><span id='a1' style='display:none'>"
        "<button onclick=setpwm(pwm-50);><<</button>"
        "<button onclick=setpwm(pwm-10);><</button></span>"
        "<button onclick=unhide('a1');unhide('a2');setpwm(Number(prompt('输入pwm值(0-1023)',pwm)));><span id=pwm>pwm:")
    + nvram.pwm + F("</span></button>"
                    "<span id=a2 style='display:none'><button onclick=setpwm(pwm+10);>></button>"
                    "<button onclick=setpwm(pwm+50);>>></button></span>"
                    "</span>");
  get_batt();
  if (digitalRead(_24V_OUT) == LOW)
    head += F("<button onclick=\"if(ajax_if('/switch.php?b=_24V_OUT&t=1','开启电源输出?')) setTimeout(function(){window.location.reload();},1000);\">电源输出") + String(get_batt()) + F("V已关闭</button>");
  else
    head += F("<button onclick=\"if(ajax_if('/switch.php?b=_24V_OUT&t=0','关闭电源输出?')) setTimeout(function(){window.location.reload();},1000);\">电源输出") + String(get_batt()) + F("V已开启</button>");
  head +=   F("<button onclick=\"if(ajax_if('/switch.php?b=reboot','重启proc?')) setTimeout(function(){window.location.reload();},15000);\">重启proc</button>");

  footer = F("<hr>")
           + mylink()
           + F("<hr>程序编译时间: <mark>") + String(ymd)
           + F(" " __TIME__ "</mark><br>"
               "编译参数:<br>"
               "<mark>" BUILD_SET "</mark>"
               "</body></html>");
}
uint32_t ap_on_time = 120000;
void handleRoot() {
  String exit_button;
  String telnets;
  yield();
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  body = F("<a href=/set.php><button>设置</button></a>");
  if (proc != OTA_MODE) body += F("<a href=http://logout@") + WiFi.localIP().toString() + F("><button>退出</button></a>");
  telnets = "";
  for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++)
    if (tcpClients[i]) { // equivalent to !tcpClients[i].connected()
      telnets += F("<tr align=center><td><input type=checkbox");
      if (client_enable[i]) telnets += F(" checked");
      telnets += F(" onclick=ajax_get('/telnet_client.php?id=") + String(i) + F("&checked='+this.checked); >#") + String(i + 1) + F("<td>") + tcpClients[i].remoteIP().toString() + ":" + String(tcpClients[i].remotePort()) + F("</td><td>") + String((millis() - client_ms[i]) / 1000) + F("</td><td>") + String(client_read[i]) + F("</td></tr>");
    }
  if (telnets != "") body += F("<hr><table border=1><tr align=center><td>允许</td><td>IP:PORT</td><td>时长(秒)</td><td>接收字节</td></tr>") + telnets + F("</table>");

  httpd_send_200("");
}
void telnet_client_php() {
  int8_t id = -1, checked = -1;
  yield();
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("id") == 0) {
      id = httpd.arg(i).toInt();
    } else if (httpd.argName(i).compareTo("checked") == 0) {
      if (httpd.arg(i) == "true")
        checked = 2; //2 是要等proc进程给远端发信息
      else if (httpd.arg(i) == "false")
        checked = 0;
    }
    if (checked != -1 && id != -1) {
      client_enable[id] = checked;
      break;
    }
  }
  httpd_send_200("");
}
void switch_php() {
  String pin;
  uint16_t t;
  File fp;
  yield();
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("b") == 0) {
      pin = httpd.arg(i);
      pin.trim();
    } else if (httpd.argName(i).compareTo("t") == 0) {
      t = httpd.arg(i).toInt();
    }
  }
  if (pin == "POWER") {
    digitalWrite(PC_POWER, HIGH);
    pcPowerTicker.detach();
    pcPowerTicker.attach_ms(t, pcPowerUp);
  } else if (pin == "RESET") {
    digitalWrite(PC_RESET, HIGH);
    pcResetTicker.detach();
    pcResetTicker.attach_ms(t, pcResetUp);
  } else if (pin == "PWM") {
    if (t > 1019) t = 1019;
    if (t < 1) t = 1;
    if (nvram.pwm != t) {
      nvram.pwm = t;
      analogWrite(PWM, t);
      nvram.change = 1;
    }
  } else if (pin == "_24V_OUT") {
    if (t != _24v_out) {
      if (t == 0) {
        if (nvram.nvram7 != nvram.nvram7 & ~ NVRAM7_24V) {
          nvram.nvram7 &= ~ NVRAM7_24V;
          nvram.change = 1;
        }
        digitalWrite(_24V_OUT, LOW);
      } else {
        if (nvram.nvram7 != nvram.nvram7 | NVRAM7_24V) {
          nvram.nvram7 |= NVRAM7_24V;
          nvram.change = 1;
        }
        digitalWrite(_24V_OUT, HIGH);
      }
      _24v_out = t;
      save_nvram();
      update_head_footer();
      yield();
    }
  }
  httpd.send(200, "text/html", pin + " " + String(t) + "ms");
  httpd.client().stop();
  if (pin == "reboot") {
    ht16c21_cmd(0x88, 1); //闪烁
    yield();
    ESP.restart();
  }
}
void set_php() {
  String wifi_stat, wifi_scan;
  String ssid;
  String update_auth;
  yield();
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  if (proc == OTA_MODE) {
    update_auth = F("<hr>登陆名:<input type=text value=")
                  + String((char *)www_username) +
                  F(" name=username size=10 maxsize=100>&nbsp;密码:<input type=text value=")
                  + String((char *) www_password) +
                  F(" name=password size=10 maxsize=100><br>");
  }
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("wifi_scan") == 0) {
      int n = WiFi.scanNetworks();
      if (n > 0) {
        wifi_scan = "自动扫描到如下WiFi,点击连接:<br>";
        for (int i = 0; i < n; ++i) {
          ssid = String(WiFi.SSID(i));
          if (WiFi.encryptionType(i) != ENC_TYPE_NONE)
            wifi_scan += "&nbsp;<button onclick=get_passwd('" + ssid + "')>*";
          else
            wifi_scan += "&nbsp;<button onclick=select_ssid('" + ssid + "')>";
          wifi_scan += String(WiFi.SSID(i)) + "(" + String(WiFi.RSSI(i)) + "dbm)";
          wifi_scan += "</button>";
          delay(10);
        }
        wifi_scan += "<br>";
      }
    }
  }
  if (wifi_scan == "") {
    wifi_scan = "<a href=/set.php?wifi_scan=1><buttom>扫描WiFi</buttom></a>";
  }

  if (connected_is_ok) {
    wifi_stat = F("wifi已连接 ssid:<mark>") + String(WiFi.SSID()) + F("</mark> &nbsp; ")
                + F("ap:<mark>") + WiFi.BSSIDstr() + F("</mark> &nbsp; ")
                + F("信号:<mark>") + String(WiFi.RSSI()) + F("</mark>dbm &nbsp; ")
                + F("ip:<mark>") + WiFi.localIP().toString() + F("</mark> &nbsp; ")
                + F("电压:<mark>") + String(v) + F("</mark>V &nbsp;剩余内存:<mark>") + String(ESP.getFreeHeap()) + F("</mark><br>");
  }
  String comset_option;
  for (uint8_t i = 0; i < sizeof(comsets) / sizeof(SerialConfig); i++) comset_option += F("<option value=") + String(i) + ">" + comset_str[i] + F("</option>");
  String select_dhcp = "", select_ip = "";
  if (is_dhcp) select_dhcp = F("checked");
  else select_ip = F("checked");
  body = F("<a href=/><button>返回首页</button></a>"
           "<hr>")
         + wifi_stat + F("<hr>") + wifi_scan
         + F("<form action=/save.php method=post>"
             "输入ssid:passwd(可以多行多个)<br>"
             "<textarea  style='width:500px;height:80px;' name=data>") + get_ssid()
         + F("</textarea><br>"
             "<input type=radio name=is_dhcp value=1 ") + select_dhcp
         + F(">dhcp&nbsp;<input type=radio name=is_dhcp value=0 ") + select_ip
         + F(">"
             "ip:<input name=local_ip size=8 value='") + local_ip.toString()
         + F("'>"
             "子网掩码:<input name=netmask size=8 value='") + netmask.toString()
         + F("'>"
             "网关:<input name=gateway size=8 value='") + gateway.toString()
         + F("'>"
             "dns:<input name=dns size=8 value='") + dns.toString()
         + F("'><br>"
             "ntp:<input name=ntp size=20 value=") + ntpServerName[0]
         + F("><br>")
#ifdef HAVE_AUTO_UPDATE
         + F("<hr>可以设置自己的自动升级服务器地址(清空恢复原始设置)<br>"
             "url0:<input maxlength=100  size=50 type=text value='") + get_url(0)
         + F("' name=url><br>"
             "url1:<input maxlength=100  size=50 type=text value='") + get_url(1)
         + F("' name=url1><br>"
             "间隔时间:<input maxlength=3  size=3 type=text value='") + update_time
         + F("' name=update_time>小时,0为关闭<br>")
#endif
         + update_auth
         + F("<hr>串口设置:<select name=rate><option value=") + rate + ">" + rate
         + F("</option>"
             "<option value='460800'>460800</option>"
             "<option value='230400'>230400</option>"
             "<option value='115200'>115200</option>"
             "<option value='57600'>57600</option>"
             "<option value='38400'>38400</option>"
             "<option value='19200'>19200</option>"
             "<option value='9600'>9600</option>"
             "<option value='4800'>4800</option>"
             "<option value='2400'>2400</option>"
             "<option value='1200'>1200</option></select>"
             "<select name=comset><option value='") + comset + "'>" + comset_str[comset]
         + F("</option>")
         + comset_option
         + F("</select>"
             "<hr>每日定时开机(hh:mm):<input type=text name=day_cron size=6 value='") + String(day_cron)
         + F("'>清空关闭"
             "<hr>主机ip:<input type=text name=master_ip size=20 value='") + master_ip.toString()
         + F("'>"
             "<hr>自定义html块(页面左下角,清空恢复原始设置):<br>"
             "<textarea style='width:500px;height:80px;' name=mylink>") + mylink()
         + F("</textarea><br>"
             "<hr><input type=submit name=submit value=保存>"
             "</form>"
             "<hr>"
             "<form method='POST' action='/update.php' enctype='multipart/form-data'>上传更新固件firmware:<br>"
             "<input type='file' name='update'onchange=\"var size=this.files[0].size;document.getElementById('size_disp').textContent=size;document.getElementById('size').value=this.files[0].size;\"><span id=size_disp></span><input type=hidden name=size id=size><br>"
             "<input type='submit' value='上传'></form>");
  httpd_send_200("");
  ap_on_time = millis() + 200000;
}
void handleNotFound() {
  File fp;
  int ch;
  String message;
  yield();
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  SPIFFS.begin();
  if (SPIFFS.exists(httpd.uri().c_str())) {
    fp = SPIFFS.open(httpd.uri().c_str(), "r");
    if (fp) {
      while (1) {
        ch = fp.read();
        if (ch == -1) break;
        message += (char)ch;
      }
      fp.close();
      httpd.send ( 200, "text/plain", message );
      httpd.client().stop();
      message = "";
      return;
    }
  }
  message = F("File Not Found\n\n"
              "URI: ") + httpd.uri() +
            F("\nArguments: ") + httpd.args() + "\n";

  httpd.send ( 404, "text/plain", message );
  httpd.client().stop();
  message = "";
}
void add_ssid_php() {
  File fp;
  String ssid, data;
  char ch;
  yield();
  SPIFFS.begin();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
    }
  }
  if (data == "") return;
  fp = SPIFFS.open("/ssid.txt", "r");
  if (fp) {
    uint16_t Fsize = fp.size();
    ssid = "";
    for (uint16_t i = 0; i < Fsize; i++) {
      ch = fp.read();
      if (ch == 0xd || ch == 0xa ) {
        if (ssid != "" && ssid != data) { //忽略空行或者重复
          data += "\r\n" + ssid;
        }
        ssid = "";
        continue;
      } else ssid += ch;
    }
    fp.close();
  }
  SPIFFS.remove("/ssid.txt");
  fp = SPIFFS.open("/ssid.txt", "w");
  fp.println(data);
  fp.close();
  SPIFFS.end();
  wifi_setup();
  httpd.send(200, "text/html", F("<html><head></head><body>"
                                 "<script>"
                                 "location.replace('/set.php');"
                                 "</script>"
                                 "</body></html>"));
  httpd.client().stop();
}
void save_php() {
  File fp;
  String url, data;
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  SPIFFS.begin();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    data = httpd.arg(i);
    data.trim();
    data.replace("\xef\xbc\x9a", ":"); //utf8 :
    data.replace("\xa3\xba", ":"); //gbk :
    data.replace("\xa1\x47", ":"); //big5 :
    if (httpd.argName(i).compareTo("data") == 0) {
      if (data.length() > 8) {
        fp = SPIFFS.open("/ssid.txt", "w");
        fp.println(data);
        fp.close();
      }
#ifdef HAVE_AUTO_UPDATE
    } else if (httpd.argName(i).compareTo("url") == 0) {
      url = data;
      if (url.length() == 0) {
        SPIFFS.remove("/url.txt");
      } else {
        fp = SPIFFS.open("/url.txt", "w");
        fp.println(url);
        fp.close();
      }
    } else if (httpd.argName(i).compareTo("url1") == 0) {
      url = data;
      if (url.length() == 0) {
        SPIFFS.remove("/url1.txt");
      } else {
        fp = SPIFFS.open("/url1.txt", "w");
        fp.println(url);
        fp.close();
      }
#endif //HAVE_AUTO_UPDATE
    } else if (httpd.argName(i).compareTo("update_time") == 0) {
      update_time = data.toInt();
      if (update_time == 0) {
        update_timeok = -1;
      } else {
        update_timeok = update_time * 60;
      }
      fp = SPIFFS.open("/update_time.txt", "w");
      fp.print(update_time);
      fp.close();
    } else if (httpd.argName(i).compareTo("mylink") == 0) {
      if (mylink() != httpd.arg(i)) {
        fp = SPIFFS.open("/mylink.txt", "w");
        fp.println(data);
        fp.close();
        update_head_footer();
      }
    } else if (httpd.argName(i).compareTo("is_dhcp") == 0) {
      if (httpd.arg(i) == "1" && !is_dhcp) {
        set_change |= NET_CHANGE;
        is_dhcp = true;
      } else if (httpd.arg(i) == "0" && is_dhcp) {
        set_change |= NET_CHANGE;
        is_dhcp = false;
      }
    } else if (httpd.argName(i).compareTo("local_ip") == 0) {
      if (local_ip.toString() != data) {
        set_change |= NET_CHANGE;
        local_ip.fromString(data);
      }
    } else if (httpd.argName(i).compareTo("netmask") == 0) {
      if (netmask.toString() != data) {
        set_change |= NET_CHANGE;
        netmask.fromString(data);
      }
    } else if (httpd.argName(i).compareTo("gateway") == 0) {
      if (gateway.toString() != data) {
        set_change |= NET_CHANGE;
        gateway.fromString(data);
      }
    } else if (httpd.argName(i).compareTo("dns") == 0) {
      if (dns.toString() != data) {
        set_change |= NET_CHANGE;
        dns.fromString(data);
      }
    } else if (httpd.argName(i).compareTo("ntp") == 0) {
      if (ntpServerName[0] != data) {
        set_change |= NET_CHANGE;
        ntpServerName[0] = data;
      }
    } else if (httpd.argName(i).compareTo("rate") == 0) {
      if (rate != data.toInt()) {
        set_change |= COM_CHANGE;
        rate = data.toInt();
      }
    } else if (httpd.argName(i).compareTo("comset") == 0) {
      if (comset != data.toInt()) {
        set_change |= COM_CHANGE;
        comset = data.toInt();
      }
    } else if (httpd.argName(i).compareTo("username") == 0) {
      strncpy(www_username, data.c_str(), sizeof(www_username));
      fp = SPIFFS.open("/http_auth.txt", "w");
      fp.println((char *)www_username);
      fp.print((char *)www_password);
      fp.close();
    } else if (httpd.argName(i).compareTo("master_ip") == 0) {
      if (data.length() < 5) data = "0.0.0.0"; //空为关闭
      if (master_ip.toString() != data) {
        set_change |= OTHER_CHANGE;
        master_ip.fromString(data);
      }
    } else if (httpd.argName(i).compareTo("password") == 0) {
      strncpy(www_password, data.c_str(), sizeof(www_password));
      fp = SPIFFS.open("/http_auth.txt", "w");
      fp.println((char *)www_username);
      fp.print((char *)www_password);
      fp.close();
    } else if (httpd.argName(i).compareTo("day_cron") == 0) {
      if (data.length() < 5) {
        SPIFFS.remove("/day_cron");
        day_cron_hour = -1;
        day_cron_minute = -1;
        memset(day_cron, 0, sizeof(day_cron));
      } else {
        strncpy(day_cron, data.c_str(), sizeof(day_cron));
        if (day_cron_hour != atoi(day_cron) || day_cron_minute != atoi(&day_cron[3])) {
          day_cron_hour = atoi(day_cron);
          day_cron_delay = 0;
          day_cron_minute = atoi(&day_cron[3]);
          fp = SPIFFS.open("/day_cron", "w");
          fp.print(data);
          fp.close();
        }
      }
    }
  }
  url = "";
  wifi_setup();
  httpd.send(200, "text/html", F("<html><head></head><body><script>location.replace('/set.php');</script></body></html>"));
  httpd.client().stop();
  //  yield();
  SPIFFS.end();
}
void httpd_listen() {
  yield();
  update_head_footer();
  httpd.begin();

  httpd.on("/", handleRoot);
  httpd.on("/set.php", set_php);
  httpd.on("/telnet_client.php", telnet_client_php);
  httpd.on("/switch.php", switch_php);
  httpd.on("/save.php", save_php); //保存设置
  httpd.on("/add_ssid.php", add_ssid_php); //保存设置

  httpd.on("/update.php", HTTP_POST, []() {
    if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
      return httpd.requestAuthentication();
    if (nvram.proc != 0 && nvram.nvram7 != (nvram.nvram7 | NVRAM7_UPDATE)) {
      nvram.proc = 0;
      nvram.nvram7 |= NVRAM7_UPDATE;
      nvram.change = 1;
      save_nvram();
    }
    httpd.sendHeader("Connection", "close");
    if (Update.hasError()) {
      httpd.send(200, "text/html", F("<html>"
                                     "<head>"
                                     "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                                     "</head>"
                                     "<body>"
                                     "升级失败 <a href=/>返回</a>"
                                     "</body>"
                                     "</html>")
                );
    } else if (crc.finalize() == CRC_MAGIC) {
      httpd.send(200, "text/html", F("<html>"
                                     "<head>"
                                     "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                                     "</head>"
                                     "<body>"
                                     "<script>setTimeout(function(){ alert('升级成功!');location.replace('/set.php'); }, 15000); </script>"
                                     "</body>"
                                     "</html>")
                );
      ht16c21_cmd(0x88, 1); //闪烁
      delay(5);
      ESP.restart();
    } else {
      body = F("升级失败 <a href=/><buttom>返回首页</buttom></a>");
      httpd_send_200("");
    }
  }, []() {
    HTTPUpload& upload = httpd.upload();
    if (upload.status == UPLOAD_FILE_START) {
      WiFiUDP::stopAll();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      Update.begin(maxSketchSpace); //start with max available size
      updating = true;
      crc.reset();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      crc.update((uint8_t *)upload.buf, upload.currentSize);
      sprintf(disp_buf, "UP.%d", upload.totalSize / 1000);
      disp(disp_buf);
      Update.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      yield();
      Update.end(true);
    }
  });
  httpd.onNotFound(handleNotFound);
  httpd.begin();

}

#define httpd_loop() httpd.handleClient()

uint32_t ms0;
void ap_loop() {
  dnsServer.processNextRequest();
  httpd_loop();
  ArduinoOTA.handle();
  system_soft_wdt_feed ();
  if (ms0 < millis()) {
    if (!connected_is_ok || !updating) {
      get_batt();
      ms0 = (ap_on_time - millis()) / 1000;
      if (ms0 < 10) sprintf_P(disp_buf, PSTR("AP  %d"), ms0);
      else if (ms0 < 100) sprintf_P(disp_buf, PSTR("AP %d"), ms0);
      else sprintf_P(disp_buf, PSTR("AP%d"), ms0);
      ms0 = millis() + 1000;
      disp(disp_buf);
    }
    yield();
    if ( millis() > ap_on_time) {
      if (millis() < 1800000 ) ap_on_time = millis() + 200000; //有外接电源的情况下，最长半小时
      else {
        if (nvram.proc != 0) {
          nvram.proc = 0;
          nvram.change = 1;
          save_nvram();
        }
        disp(F("00000"));
        ht16c21_cmd(0x84, 0);
        httpd.close();
        ESP.restart();
      }
    }
  }
}
#endif //__AP_WEB_H__
