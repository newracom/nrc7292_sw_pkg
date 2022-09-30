#!/bin/bash 
sudo killall -9 hostapd
sudo rmmod nrc
sleep 2
sudo cp /home/pi/nrc_pkg/sw/firmware/nrc7292_cspi.bin /lib/firmware/uni_s1g.bin
sudo insmod /home/pi/nrc_pkg/sw/driver/nrc.ko power_save=0 fw_name=uni_s1g.bin
sleep 5
sudo ifconfig wlan0 0.0.0.0
sudo ifconfig eth0 0.0.0.0
sleep 5
/home/pi/nrc_pkg/script/cli_app set txpwr 17
sudo hostapd ap_sdk_map.conf -ddddd | tee hostap.log
