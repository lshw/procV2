#ifndef _CONFIG_H_
#define _CONFIG_H_

#define DEFAULT_URL0 "http://temp.cfido.com:808/proc_wifi.php"
#define DEFAULT_URL1 "http://temp2.wf163.com:808/proc_wifi.php"

#include "commit.h"  //版本号
#ifndef GIT_COMMIT_ID
#define GIT_COMMIT_ID "test"
#endif
#define _24V_OUT 13
#define PC_RESET 15
#define PC_POWER 14
#define PWM 12
#define DS 0

#endif //_CONFIG_H_
