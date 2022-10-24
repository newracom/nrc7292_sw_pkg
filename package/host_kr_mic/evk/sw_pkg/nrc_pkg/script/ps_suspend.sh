#!/bin/bash

#get PM status from iwconfig
ps=$(iwconfig wlan0| grep 'Power Management:' | awk '{print $2}' | cut -d':' -f2)
if [ $ps = "off" ]
then
    echo "Power Management is already off"
    exit
fi

#print current status
#echo "status"
#echo "======================="
#ifconfig wlan0| grep -w 'ether'| awk '{print "MAC :" $2}'
#ifconfig wlan0| grep -w 'inet'| awk '{print "IP :" $2}'
#iwconfig wlan0| grep 'Power Management:' | awk '{print "Power " $2}'
#echo "======================="

#suspend PM and wakeup target
echo
echo "suspending power save and wake target..."
sudo iwconfig wlan0 power off
sleep 1
ping 192.168.200.1 -c 1
sleep 1

#print changed status
echo
echo "======================="
iwconfig wlan0| grep 'Power Management:' | awk '{print "Power " $2}'
echo "======================="

echo "Done"
