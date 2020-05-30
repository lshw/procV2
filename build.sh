#!/bin/bash
cd `dirname $0`/lib
if ! [ -x ./uncrc32 ] ; then
gcc -o uncrc32 uncrc32.c
fi
cd ..
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:7}
echo $ver
export COMMIT=$ver
#echo "#define GIT_COMMIT_ID \"$ver\"" > proc_wifi/commit.h

arduino=/opt/arduino-1.8.12
arduinoset=~/.arduino15
sketchbook=~/sketchbook
mkdir -p /tmp/build_proc_wifi /tmp/cache_proc_wifi
chown liushiwei /tmp/build_proc_wifi /tmp/cache_proc_wifi
$arduino/arduino-builder -dump-prefs -logger=machine \
-hardware $arduino/hardware \
-hardware $arduinoset/packages \
-tools $arduino/tools-builder \
-tools $arduino/hardware/tools/avr \
-tools $arduinoset/packages \
-built-in-libraries $arduino/libraries \
-libraries $sketchbook/libraries \
-fqbn=esp8266com:esp8266:espduino:ResetMethod=v2,xtal=80,vt=flash,exception=legacy,eesz=4M3M,ip=hb2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200 \
-ide-version=10812 \
-build-path /tmp/build_proc_wifi \
-warnings=none \
-build-cache /tmp/cache_proc_wifi \
-prefs=build.warn_data_percentage=75 \
-verbose \
./proc_wifi/proc_wifi.ino

$arduino/arduino-builder \
-compile \
-logger=machine \
-hardware $arduino/hardware \
-hardware $arduinoset/packages \
-tools $arduino/tools-builder \
-tools $arduino/hardware/tools/avr \
-tools $arduinoset/packages \
-built-in-libraries $arduino/libraries \
-libraries $sketchbook/libraries \
-fqbn=esp8266com:esp8266:espduino:ResetMethod=v2,xtal=80,vt=flash,exception=legacy,eesz=4M3M,ip=hb2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200 \
-ide-version=10812 \
-build-path /tmp/build_proc_wifi \
-warnings=none \
-build-cache /tmp/cache_proc_wifi \
-prefs=build.warn_data_percentage=75 \
-verbose \
./proc_wifi/proc_wifi.ino \
> /tmp/info_proc_wifi.log
if [ $? = 0 ] ; then
#/opt/arduino-1.8.12/hardware/esp8266com/esp8266/tools/xtensa-lx106-elf/bin/xtensa-lx106-elf-size /tmp/build_proc_wifi/proc_wifi.ino.elf |head -n 5
 grep "Global vari" /tmp/info_proc_wifi.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "内存：使用"$1"字节,"$3"%,剩余:"$4"字节"}'
 grep "Sketch uses" /tmp/info_proc_wifi.log |awk -F[ '{printf $2}'|tr -d ']'|awk -F' ' '{print "ROM：使用"$1"字节,"$3"%"}'

 cp -a /tmp/build_proc_wifi/proc_wifi.ino.bin lib/proc_wifi.bin
 lib/uncrc32 lib/proc_wifi.bin 0
 if [ "a$1" != "a"  ] ;then
  $arduino/hardware/esp8266com/esp8266/tools/espota.py -p 8266 -i $1 -f lib/proc_wifi.bin
 fi
fi
