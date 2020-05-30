#ifndef __AP_WEB_H__
#define __AP_WEB_H__
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "wifi_client.h"
#include "global.h"
#include "proc.h"
extern void disp(char *);
extern char ram_buf[10];
extern String hostname;
void send_ram();
float get_batt();
void ht16c21_cmd(uint8_t cmd, uint8_t dat);

ESP8266WebServer httpd(80);
String head, footer;
String mylink;
void update_head_footer() {
  File fp;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/mylink.txt", "r");
    if (fp) {
      mylink = fp.readString();
      fp.close();
    }
  }
  if (mylink == "") mylink = "在线文档:\r<a href=https://www.bjlx.org.cn/mode/929>https://www.bjlx.org.cn/node/929</a>";

  head =
    "<html>"
    "<head>"
    "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
    "<script>"
    "function ajax_get(url) {"
    " xhr = new XMLHttpRequest();"
    " xhr.open('GET', url, true);"
    " xhr.setRequestHeader('Content-Type', 'text/html; charset=UTF-8');"
    " xhr.send();"
    "}"
    "function goto_if(url,msg) {"
    "if(confirm(msg))"
    " location.replace(url);"
    "}"
    "function ajax_if(url,msg) {"
    "if(confirm(msg))"
    " ajax_get(url);"
    "}"
    "function modi(url,text,defaultext) {"
    " var data=prompt(text,defaultext);"
    "if( data != defaultext)"
    "location.replace(url+data);"
    "return false;"
    "}"
    "</script>"
    "</head>"
    "<body>"
    "SN:<mark>" + hostname + "</mark> &nbsp; "
    "版本:<mark>" VER "</mark><br>"
    "<button onclick=ajax_if('/switch.php?b=RESET&t=300','复位电脑?')>短按复位</button>"
    "<button onclick=ajax_if('/switch.php?b=RESET&t=4000','复位电脑?')>长按复位</button>"
    "<button onclick=ajax_if('/switch.php?b=POWER&t=300','按下电源键?')>短按电源</button>"
    "<button onclick=ajax_if('/switch.php?b=POWER&t=4000','按下电源键?')>长按电源</button>";
  footer =
    "<hr><table width=100%><tr>"
    "<td align=left>" + mylink + "</td>"
    "<td><td align=right>程序编译时间: <mark>" __DATE__ " " __TIME__ "</mark></td></tr></table></body></html>";
}
uint32_t ap_on_time = 120000;
void handleRoot() {
  String exit_button;
  String telnets;
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  if (proc != OTA_MODE) exit_button = "<a href=http://logout@" + WiFi.localIP().toString() + "><button>退出</button></a>";
  for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++)
    if (tcpClients[i]) { // equivalent to !tcpClients[i].connected()
      telnets += "<tr><td><input type=checkbox";
      if (client_enable[i]) telnets += " checked";
      telnets += " onclick=ajax_get('/telnet_client.php?id=" + String(i) + "&checked='+this.checked); >#" + String(i + 1) + "<td>" + tcpClients[i].remoteIP().toString() + ":" + String(tcpClients[i].remotePort()) + "</td><td>" + String(client_read[i]) + "</td></tr>";
    }
  if (telnets != "") telnets = "<hr><table border=1><tr><td>允许</td><td>IP:PORT</td><td>收到字节</td></tr>" + telnets + "</table>";

  httpd.send(200, "text/html",
             head +
             "<a href=/set.php><button>网络设置及升级</button></a>"
             + exit_button + telnets + footer
            );
  httpd.client().stop();
}
void telnet_client_php() {
  int8_t id = -1, checked = -1;
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
  } else if (pin == "_24V_OUT") {
    digitalWrite(_24V_OUT, LOW);
    pc24vOutTicker.detach();
    pc24vOutTicker.attach_ms(t, pc24vOn);
  }
  httpd.send(200, "text/html", pin + " " + String(t) + "ms");
  httpd.client().stop();
}
void set_php() {
  String wifi_stat, wifi_scan;
  String ssid;
  String update_auth;
  if (proc != OTA_MODE && !httpd.authenticate(www_username, www_password))
    return httpd.requestAuthentication();
  if (proc == OTA_MODE) {
    update_auth = "登陆名:<input type=text value="
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

  httpd.send(200,
             "text/html",
             head +
             "<a href=/><button>返回首页</button></a>"
             "<hr>"
             + wifi_stat + "<hr>" + wifi_scan +
             "<hr><form action=/save.php method=post>"
             "输入ssid:passwd(可以多行多个)<br>"
             "<textarea  style='width:500px;height:80px;' name=data>" + get_ssid() + "</textarea><br>"
             "可以设置自己的升级服务器地址(清空恢复原始设置)<br>"
             "url0:<input maxlength=100  size=30 type=text value='" + get_url(0) + "' name=url><br>"
             "url1:<input maxlength=100  size=30 type=text value='" + get_url(1) + "' name=url1><br>"
             "间隔时间:<input maxlength=100  size=10 type=text value='" + update_time + "' name=update_time>小时,0为关闭<br>"
             + update_auth +
             "自定义链接(html格式,清空恢复原始设置):<br>"
             "<textarea style='width:500px;height:80px;' name=mylink>" + mylink + "</textarea><br>"
             "<input type=submit name=submit value=保存>"
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
        fp = SPIFFS.open("/ssid.txt", "r");
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
  SPIFFS.end();
  wifi_setup();
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/set.php');</script></body></html>");
  httpd.client().stop();
}
void httpd_listen() {
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
    ram_buf[0] = 0;
    send_ram();
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
      Update.end(true);
    }
    yield();
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

    if ( millis() > ap_on_time) {
      if (millis() < 1800000 ) ap_on_time = millis() + 200000; //有外接电源的情况下，最长半小时
      else {
        ram_buf[0] = 0;
        disp("00000");
        ht16c21_cmd(0x84, 0);
        httpd.close();
        ESP.restart();
      }
    }
  }
}
#endif //__AP_WEB_H__
