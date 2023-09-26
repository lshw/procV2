#!/bin/bash
#update.sh 192.168.1.2
#or by ttyUSB0/ttyS0
#update.sh

cd `dirname $0`
if [ "a$1" != "a" ] ; then
#带参数，是网络ota刷机
 ./espota.py -p 8266 -f ./proc_wifi.bin -i $1
else
#不带参数就试试串口线刷
 if [ "`ps |grep min[i]com`" ] ; then
  killall minicom 2>/dev/null
  sleep 3
 fi

 if [ -e /dev/ttyUSB0 ] ; then
  port=/dev/ttyUSB0
 else
  port=/dev/ttyS0
 fi
#先试试 esptool.py
 ./esptool.py --chip esp8266 --port $port  --baud 460800 write_flash 0 ./proc_wifi.bin
fi

