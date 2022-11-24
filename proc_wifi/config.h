#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VER "1.65"
#define VERA ""
#define CRC_MAGIC 4
//#define HAVE_AUTO_UPDATE

#define _24V_OUT 13
#define PC_RESET 15
#define PC_POWER 14
#define PWM 12
#define DS 0

#ifdef HAVE_AUTO_UPDATE
#define DEFAULT_URL0 "http://temp.cfido.com:808/proc_wifi.php"
#define DEFAULT_URL1 "http://temp2.wf163.com:808/proc_wifi.php"
#endif

#endif //_CONFIG_H_
