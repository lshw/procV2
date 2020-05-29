#!/bin/bash
#update.sh 192.168.1.2
cd `dirname $0`/..
arduino=/opt/arduino-1.8.12
if [ "a$1" != "a" ] ; then
lib/espota.py -p 8266 -f lib/proc_wifi.bin -i $1
else
if [ "`ps |grep min[i]com`" ] ; then
killall minicom 2>/dev/null
sleep 3
fi
if [ -x /dev/ttyUSB0 ] ; then
$arduino/hardware/esp8266com/esp8266/tools/esptool/esptool -vv -cd nodemcu -cb 115200 -cp /dev/ttyUSB0 -ca 0x00000 -cf lib/proc_wifi.bin
else
$arduino/hardware/esp8266com/esp8266/tools/esptool/esptool -vv -cd nodemcu -cb 115200 -cp /dev/ttyS0 -ca 0x00000 -cf lib/proc_wifi.bin
fi
fi
