#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VER "1.70"
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
extern uint8_t pcb_ver;
uint8_t get_pcb_ver();
float get_R18() {
  if (pcb_ver == 0) return get_pcb_ver();
  if (pcb_ver == 2) return 47.000;
  switch (ESP.getChipId()) {
    case 0x9488F0:
    case 0x9463F5:
    case 0x0DE19A:
    case 0x949CB5:
      return 43.200;
  }
  return 100.000;
}

#endif //_CONFIG_H_
