#!/bin/bash
cd `dirname $0`

branch=`git status |head -n 1 |awk '{print $2}'`
a=`git log --date=short -1 |head -n 1 |awk '{print $2}'`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
ver=$date-${a:0:8}
echo $ver
git tag $ver
export debug=",dbg=Serial,lvl=WIFI"
./build.sh
mv proc_wifi.bin proc_wifi_dbg.bin
export debug=",dbg=Disabled,lvl=None____"
./build.sh
git log >changelog.txt
tar cvfz ../wifi_disp_$ver.tar.gz proc_wifi.bin proc_wifi_dbg.bin changelog.txt update.sh *.py
