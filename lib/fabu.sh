#!/bin/bash
cd `dirname $0`
cd ..

branch=`git status |head -n 1 |awk '{print $2}'`
a=`git log --date=short -1 |head -n 1 |awk '{print $2}'`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:8}
echo $ver
git tag $ver
export debug=",dbg=Serial,lvl=WIFI"
lib/build.sh
mv lib/proc_wifi.bin lib/proc_wifi_dbg.bin
export debug=",dbg=Disabled,lvl=None____"
lib/build.sh
git log >changelog.txt
tar cvfz wifi_disp_$ver.tar.gz lib/proc_wifi.bin lib/proc_wifi_dbg.bin changelog.txt lib/update.sh lib/*.py
