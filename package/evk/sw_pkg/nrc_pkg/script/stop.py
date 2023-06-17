#!/usr/bin/python

import os
import time
from mesh import *

script_path = "/home/pi/nrc_pkg/script/"

def stopNAT():
    os.system('sudo sh -c "echo 0 > /proc/sys/net/ipv4/ip_forward"')
    os.system("sudo iptables -t nat --flush")
    os.system("sudo iptables --flush")

print("STOP")

print("[0] Stop applications")
removeBridgeMeshAP("wlan0", "wlan1")
os.system("sudo hostapd_cli disable 2>/dev/null")
os.system("sudo wpa_cli disable wlan0 2>/dev/null")
os.system("sudo killall -9 wpa_supplicant 2>/dev/null")
os.system("sudo killall -9 hostapd 2>/dev/null")
os.system("sudo killall -9 wireshark-gtk 2>/dev/null")
stopNAT()
stopMeshNAT()
time.sleep(1)

print("[1] Remove module")
os.system("sudo batctl if del wlan0 > /dev/null 2>&1")
os.system("sudo batctl if del mesh0 > /dev/null 2>&1")
os.system("sudo ifconfig bat0 down > /dev/null 2>&1")
os.system("sudo rmmod nrc 2>/dev/null")
os.system("sudo rm "+script_path+"conf/temp_self_config.conf 2>/dev/null")
os.system("sudo rm "+script_path+"conf/temp_hostapd_config.conf 2>/dev/null")
time.sleep(2)

print("[2] Done")
