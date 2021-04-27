#!/bin/bash

#get PM status from iwconfig
ps=$(iwconfig wlan0| grep 'Power Management:' | awk '{print $2}' | cut -d':' -f2)
if [ $ps = "on" ]
then
    echo "Power Management is already on"
    exit
fi

#print current status
#echo "status"
#echo "======================="
#ifconfig wlan0| grep -w 'ether'| awk '{print "MAC :" $2}'
#ifconfig wlan0| grep -w 'inet'| awk '{print "IP :" $2}'
#iwconfig wlan0| grep 'Power Management:' | awk '{print "Power " $2}'
#echo "======================="

#resume PM
echo
if [ $# -eq 0 ]
then
    echo "resuming Power Save after 3 seconds..."
    sudo iwconfig wlan0 power timeout 3
else
    echo "resuming Power Save after $1 seconds..."
    sudo iwconfig wlan0 power timeout $1
fi
sleep 1

#print changed status
echo
echo "======================="
iwconfig wlan0| grep 'Power Management:' | awk '{print "Power " $2}'
echo "======================="

echo "Done"
