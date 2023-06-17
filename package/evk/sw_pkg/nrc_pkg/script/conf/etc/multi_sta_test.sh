#!/bin/bash

if [ $# -ne 2 ];
then
 echo "Usage: ./multi_sta_test.sh STA_NUM INTERVAL (eg. ./test_multi_sta.sh 100 3 : connect 100 STA with interval 3s)"
 exit 1
fi

if [ $2 -lt 2 ];
then
 echo "Interval should be greater than 1 second"
 exit 1
fi

echo "Start Multi-STA connection Test. Connect STA($1) with interval($2)"
sudo systemctl stop dhcpcd

for i in $(seq 1 $1);
do
 addr=`awk -v i=$i 'BEGIN{printf("84:25:3f:%02x:%02x:%02x\n", (i/65536)%256, (i/256)%256,i%256)}'`
 echo "STA($i) is now trying to connect to AP with MAC:$addr"
 sudo ifconfig wlan0 down
 sudo ifconfig wlan0 hw ether ${addr}
 sudo ifconfig wlan0 up
 sudo ifconfig wlan0 192.168.200.10
 sleep $2
#ping 192.168.200.1 -c 1
done

sudo systemctl start dhcpcd
echo "Multi-STA Test is done"
