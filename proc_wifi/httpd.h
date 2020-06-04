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
String mylink;
void update_head_footer() {
  head =
    "<html>"
    "<head>"
    "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
    "<style type='text/css'>"
    "hide {display:none;}"
    "unhide {display:inline;}"
    "</style>"
    "<script>"
    " pwm=" + String(nvram.pwm) + ";"
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
    " function ajax_if(url,msg) {"
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
    "}"
    "</script>"
    "</head>"
    "<body>"
    "SN:<mark>" + hostname + "</mark> &nbsp; "
    "版本:<mark>" VER "</mark>&nbsp;&nbsp;串口:" + String(rate) + "," + comset_str[comset] + "<br>"
    "<button onclick=ajax_if('/switch.php?b=RESET&t=300','复位电脑?')>短按复位</button>"
    "<button onclick=ajax_if('/switch.php?b=RESET&t=5000','复位电脑?')>长按复位</button>"
    "<button onclick=ajax_if('/switch.php?b=POWER&t=300','按下电源键?')>短按电源</button>"
    "<button onclick=ajax_if('/switch.php?b=POWER&t=5000','按下电源键?')>长按电源</button>"
    "<span style='white-space: nowrap;'><span id='a1' style='display:none'>"
    "<button onclick=setpwm(pwm-50);><<</button>"
    "<button onclick=setpwm(pwm-10);><</button></span>"
    "<button onclick=unhide('a1');unhide('a2');setpwm(Number(prompt('输入pwm值(0-1023)',pwm)));><span id=pwm>pwm:"
    + nvram.pwm + "</span></button>"
    "<span id=a2 style='display:none'><button onclick=setpwm(pwm+10);>></button>"
    "<button onclick=setpwm(pwm+50);>>></button></span>"
    "</span>";
  get_batt();
  if (digitalRead(_24V_OUT) == LOW)
    head += "<button onclick=\"if(ajax_if('/switch.php?b=_24V_OUT&t=1','开启电源输出?')) setTimeout(function(){window.location.reload();},1000);\">电源输出" + String(get_batt()) + "V已关闭</button>";
  else
    head += "<button onclick=\"if(ajax_if('/switch.php?b=_24V_OUT&t=0','关闭电源输出?')) setTimeout(function(){window.location.reload();},1000);\">电源输出" + String(get_batt()) + "V已开启</button>";
  head +=   "<button onclick=\"if(ajax_if('/switch.php?b=reboot','重启proc?')) setTimeout(function(){window.location.reload();},15000);\">重启proc</button>";


  footer =
    "<hr><table width=100%><tr>"
    "<td align=left>" + mylink + "</td>"
    "<td><td align=right valign=bottom>程序编译时间: <mark>" __DATE__ " " __TIME__ "</mark></td></tr></table></body></html>";
}
uint32_t ap_on_time = 120000;
void handleRoot() {
  String exit_button;
  String telnets;
  yield();
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  if (proc != OTA_MODE) exit_button = "<a href=http://logout@" + WiFi.localIP().toString() + "><button>退出</button></a>";
  for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++)
    if (tcpClients[i]) { // equivalent to !tcpClients[i].connected()
      telnets += "<tr align=center><td><input type=checkbox";
      if (client_enable[i]) telnets += " checked";
      telnets += " onclick=ajax_get('/telnet_client.php?id=" + String(i) + "&checked='+this.checked); >#" + String(i + 1) + "<td>" + tcpClients[i].remoteIP().toString() + ":" + String(tcpClients[i].remotePort()) + "</td><td>" + String((millis() - client_ms[i]) / 1000) + "</td><td>" + String(client_read[i]) + "</td></tr>";
    }
  if (telnets != "") telnets = "<hr><table border=1><tr align=center><td>允许</td><td>IP:PORT</td><td>时长(秒)</td><td>接收字节</td></tr>" + telnets + "</table>";

  httpd.send(200, "text/html",
             head +
             "<a href=/set.php><button>设置</button></a>"
             + exit_button + telnets + footer
            );
  httpd.client().stop();
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
  httpd.send(200, "text/html", "");
  httpd.client().stop();
}
void switch_php() {
  String pin;
  uint16_t t;
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
    update_auth = "<hr>登陆名:<input type=text value="
                  + String((char *)www_username) +
                  " name=username size=10 maxsize=100>&nbsp;密码:<input type=text value="
                  + String((char *) www_password) +
                  " name=password size=10 maxsize=100><br>";
  }
  int n = WiFi.scanNetworks();
  if (n > 0) {
    wifi_scan = "自动扫描到如下WiFi热点,点击添加:<br>";
    for (int i = 0; i < n; ++i) {
      ssid = String(WiFi.SSID(i));
      if (WiFi.encryptionType(i) != ENC_TYPE_NONE)
        wifi_scan += "&nbsp;<button onclick=modi('/add_ssid.php?data="
                     + ssid +
                     ":','输入无线密码','')>*";
      else
        wifi_scan += "&nbsp;<button onclick=gotoif('/add_ssid.php?data=" + ssid + "')>";
      wifi_scan += String(WiFi.SSID(i)) + "(" + String(WiFi.RSSI(i)) + "dbm)";
      wifi_scan += "</button>";
      delay(10);
    }
    wifi_scan += "<br>";
  }
  if (wifi_connected_is_ok()) {
    wifi_stat = "wifi已连接 ssid:<mark>" + String(WiFi.SSID()) + "</mark> &nbsp; "
                + "ap:<mark>" + WiFi.BSSIDstr() + "</mark> &nbsp; "
                + "信号:<mark>" + String(WiFi.RSSI()) + "</mark>dbm &nbsp; "
                + "ip:<mark>" + WiFi.localIP().toString() + "</mark> &nbsp; "
                + "电压:<mark>" + String(v) + "</mark>V<br>";
  }
  String comset_option;
  for (uint8_t i = 0; i < sizeof(comsets) / sizeof(SerialConfig); i++) comset_option += "<option value=" + String(i) + ">" + comset_str[i] + "</option>";
  String select_dhcp = "", select_ip = "";
  if (is_dhcp) select_dhcp = "checked";
  else select_ip = "checked";
  httpd.send(200,
             "text/html",
             head +
             "<a href=/><button>返回首页</button></a>"
             "<hr>"
             + wifi_stat + "<hr>" + wifi_scan +
             "<form action=/save.php method=post>"
             "输入ssid:passwd(可以多行多个)<br>"
             "<textarea  style='width:500px;height:80px;' name=data>" + get_ssid() + "</textarea><br>"
             "<input type=radio name=is_dhcp value=1 " + select_dhcp + ">dhcp&nbsp;<input type=radio name=is_dhcp value=0 " + select_ip + ">"
             "ip:<input name=local_ip size=8 value='" + local_ip.toString() + "'>"
             "子网掩码:<input name=netmask size=8 value='" + netmask.toString() + "'>"
             "网关:<input name=gateway size=8 value='" + gateway.toString() + "'>"
             "dns:<input name=dns size=8 value='" + dns.toString() + "'><br>"
             "ntp:<input name=ntp size=20 value=" + ntpServerName[0] + "><br>"
             "<hr>可以设置自己的升级服务器地址(清空恢复原始设置)<br>"
             "url0:<input maxlength=100  size=50 type=text value='" + get_url(0) + "' name=url><br>"
             "url1:<input maxlength=100  size=50 type=text value='" + get_url(1) + "' name=url1><br>"
             "间隔时间:<input maxlength=3  size=3 type=text value='" + update_time + "' name=update_time>小时,0为关闭<br>"
             + update_auth +
             "<hr>自定义html块(页面左下角,清空恢复原始设置):<br>"
             "<textarea style='width:500px;height:80px;' name=mylink>" + mylink + "</textarea><br>"
             "<hr>串口设置:<select name=rate><option value=" + rate + ">" + rate + "</option>"
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
             "<select name=comset><option value=" + comset + ">" + comset_str[comset] + "</option>"
             + comset_option +
             "</select>"
             "<hr><input type=submit name=submit value=保存>"
             "</form>"
             "<hr>"
             "<form method='POST' action='/update.php' enctype='multipart/form-data'>上传更新固件firmware:<br>"
             "<input type='file' name='update'onchange=\"var size=this.files[0].size;document.getElementById('size_disp').textContent=size;document.getElementById('size').value=this.files[0].size;\"><span id=size_disp></span><input type=hidden name=size id=size><br>"
             "<input type='submit' value='上传'></form>"
             + footer
            );
  httpd.client().stop();
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
  message = "File Not Found\n\n";
  "URI: " + httpd.uri() +
  "\nArguments: " + httpd.args() + "\n";

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
  httpd.send(200, "text/html", "<html><head></head><body>"
             "<script>"
             "location.replace('/set.php');"
             "</script>"
             "</body></html>");
  httpd.client().stop();
}
void save_php() {
  File fp;
  String url, data;
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  SPIFFS.begin();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
      if (data.length() > 8) {
        fp = SPIFFS.open("/ssid.txt", "w");
        fp.println(data);
        fp.close();
      }
    } else if (httpd.argName(i).compareTo("url") == 0) {
      url = httpd.arg(i);
      url.trim();
      if (url.length() == 0) {
        SPIFFS.remove("/url.txt");
      } else {
        fp = SPIFFS.open("/url.txt", "w");
        fp.println(url);
        fp.close();
      }
    } else if (httpd.argName(i).compareTo("url1") == 0) {
      url = httpd.arg(i);
      url.trim();
      if (url.length() == 0) {
        SPIFFS.remove("/url1.txt");
      } else {
        fp = SPIFFS.open("/url1.txt", "w");
        fp.println(url);
        fp.close();
      }
    } else if (httpd.argName(i).compareTo("update_time") == 0) {
      update_time = httpd.arg(i).toInt();
      if (update_time == 0) {
        update_timeok = -1;
      } else {
        update_timeok = update_time * 60;
      }
      fp = SPIFFS.open("/update_time.txt", "w");
      fp.print(update_time);
      fp.close();
    } else if (httpd.argName(i).compareTo("mylink") == 0) {
      if (mylink != httpd.arg(i)) {
        mylink = httpd.arg(i);
        fp = SPIFFS.open("/mylink.txt", "w");
        fp.println(mylink);
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
      if (local_ip.toString() != httpd.arg(i)) {
        set_change |= NET_CHANGE;
        local_ip.fromString(httpd.arg(i));
      }
    } else if (httpd.argName(i).compareTo("netmask") == 0) {
      if (netmask.toString() != httpd.arg(i)) {
        set_change |= NET_CHANGE;
        netmask.fromString(httpd.arg(i));
      }
    } else if (httpd.argName(i).compareTo("gateway") == 0) {
      if (gateway.toString() != httpd.arg(i)) {
        set_change |= NET_CHANGE;
        gateway.fromString(httpd.arg(i));
      }
    } else if (httpd.argName(i).compareTo("dns") == 0) {
      if (dns.toString() != httpd.arg(i)) {
        set_change |= NET_CHANGE;
        dns.fromString(httpd.arg(i));
      }
    } else if (httpd.argName(i).compareTo("ntp") == 0) {
      if (ntpServerName[0] != httpd.arg(i)) {
        set_change |= NET_CHANGE;
        ntpServerName[0] = httpd.arg(i);
      }
    } else if (httpd.argName(i).compareTo("rate") == 0) {
      if (rate != httpd.arg(i).toInt()) {
        set_change |= COM_CHANGE;
        rate = httpd.arg(i).toInt();
      }
    } else if (httpd.argName(i).compareTo("comset") == 0) {
      if (comset != httpd.arg(i).toInt()) {
        set_change |= COM_CHANGE;
        comset = httpd.arg(i).toInt();
      }
    } else if (httpd.argName(i).compareTo("username") == 0) {
      strncpy(www_username, httpd.arg(i).c_str(), sizeof(www_username));
      fp = SPIFFS.open("/http_auth.txt", "w");
      fp.println((char *)www_username);
      fp.print((char *)www_password);
      fp.close();
    } else if (httpd.argName(i).compareTo("password") == 0) {
      strncpy(www_password, httpd.arg(i).c_str(), sizeof(www_password));
      fp = SPIFFS.open("/http_auth.txt", "w");
      fp.println((char *)www_username);
      fp.print((char *)www_password);
      fp.close();
    }
  }
  url = "";
  wifi_setup();
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/set.php');</script></body></html>");
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
      httpd.send(200, "text/html", "<html>"
                 "<head>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "</head>"
                 "<body>"
                 "升级失败 <a href=/>返回</a>"
                 "</body>"
                 "</html>"
                );
    } else {
      httpd.send(200, "text/html", "<html>"
                 "<head>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "</head>"
                 "<body>"
                 "<script>setTimeout(function(){ alert('升级成功!');location.replace('/set.php'); }, 15000); </script>"
                 "</body>"
                 "</html>"
                );
      ht16c21_cmd(0x88, 1); //闪烁
      delay(5);
      ESP.restart();
    }
  }, []() {
    HTTPUpload& upload = httpd.upload();
    if (upload.status == UPLOAD_FILE_START) {
      WiFiUDP::stopAll();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      Update.begin(maxSketchSpace); //start with max available size
    } else if (upload.status == UPLOAD_FILE_WRITE) {
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
  if (ms0 < millis()) {
    get_batt();
    system_soft_wdt_feed ();
    ms0 = (ap_on_time - millis()) / 1000;
    if (ms0 < 10) sprintf(disp_buf, "AP  %d", ms0);
    else if (ms0 < 100) sprintf(disp_buf, "AP %d", ms0);
    else sprintf(disp_buf, "AP%d", ms0);
    ms0 = millis() + 1000;
    disp(disp_buf);

    yield();
    if ( millis() > ap_on_time) {
      if (millis() < 1800000 ) ap_on_time = millis() + 200000; //有外接电源的情况下，最长半小时
      else {
        if (nvram.proc != 0) {
          nvram.proc = 0;
          nvram.change = 1;
        }
        disp("00000");
        ht16c21_cmd(0x84, 0);
        httpd.close();
        ESP.restart();
      }
    }
  }
}
#endif //__AP_WEB_H__
