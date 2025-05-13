#!/bin/bash

project=$( basename $( dirname $( realpath $0 )))
echo $project

cd $( dirname $0 )

me=$( whoami )
if [ "$me" == "root" ] ; then
  home=/home/liushiwei
else
  home=~
fi

arduino_cli=$( which arduino-cli )
if [ $? != 0 ] ; then
arduino_cli=$( find /opt/arduino-ide_* -name "arduino-cli" |sort |tail -n 1 )
if ! [ -x $arduino_cli ] ; then
echo not find arduino_cli
exit
fi
fi
echo find $arduino_cli
which astyle
if [ $? = 0 ] ; then
astyle  --options=../lib/formatter.conf *.h *.ino
fi
rm -f *.orig
if ! [ -x ../lib/uncrc32 ] ; then
gcc -o ../lib/uncrc32 ../lib/uncrc32.c
fi

cd ..
CRC_MAGIC=$( grep CRC_MAGIC ${project}/config.h | awk '{printf $3}' )
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:7}
echo $ver
export COMMIT=$ver

build=/tmp/${me}_${project}_build
mkdir -p $build

#debug输出设置
debug=",dbg=Disabled,lvl=None____"
#debug=",dbg=Serial,lvl=WIFI"
#debug=",dbg=Serial,lvl=SSLTLS_MEMHTTP_CLIENTHTTP_SERVERCOREWIFIHTTP_UPDATEUPDATEROTAOOMMDNSHWDT"
fqbn="esp8266:esp8266:generic:xtal=160,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,mmu=4816,non32xfer=fast,CrystalFreq=26,FlashFreq=80,FlashMode=qio,eesz=4M3M,led=2,sdk=nonosdk305,ip=hb2f$debug,wipe=none,baud=921600"

#传递宏定义 GIT_VER 到源码中，源码git版本
CXXFLAGS="-DGIT_VER=\"$ver\" -DBUILD_SET=\"$fqbn\""

rm -f /tmp/${me}_${project}_build/${project}.ino.bin
$arduino_cli compile \
--fqbn $fqbn \
--verbose \
--build-property compiler.cpp.extra_flags="$CXXFLAGS" \
--build-property compiler.c.extra_flags="$CXXFLAGS" \
--build-path $build \
$project 2>&1 |tee /tmp/${me}_info.log 

if [ $? != 0 ] ; then
  exit
fi

if [ -e /tmp/${me}_${project}_build/${project}.ino.bin ] ; then
#8266
  tail -n 100 /tmp/${me}_info.log |sed -n "s/^. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used \([0-9]*\) .* (\([0-9]*\)%).*$/RAM:使用\1字节(\2%)/p"
  tail -n 100 /tmp/${me}_info.log |sed -n "s/^. Code in flash (default, ICACHE_FLASH_ATTR), used \([0-9]*\) .* (\(39\)%)$/ROM:使用\1字节(\2%)/p"

#esp32-c3
  tail -n 100 /tmp/${me}_info.log |sed -n "s/^Global variables use \([0-9]*\) bytes (\([0-9]*\)%) of dynamic memory, leaving \([0-9]*\) bytes for local variables. .*$/RAM:全局变量使用\1字节(\2%),剩余用于局部变量:\3字节/p"
  tail -n 100 /tmp/${me}_info.log |sed -n "s/^Sketch uses \([0-9]*\) bytes (\([0-9]*\)%) of program storage space. Maximum is.*$/ROM:使用\1字节(\2%)/p"

  cp -a /tmp/${me}_${project}_build/${project}.ino.bin lib/${project}.bin
lib/uncrc32 lib/${project}.bin $CRC_MAGIC
fi
echo $ver
