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
os.system("sudo killall -9 wpa_supplicant")
os.system("sudo killall -9 hostapd")
os.system("sudo killall -9 wireshark-gtk")
stopNAT()
stopMeshNAT()
time.sleep(1)

print("[1] Remove module")
os.system("sudo rmmod nrc")
os.system("sudo rm "+script_path+"conf/temp_self_config.conf")
time.sleep(2)

print("[2] Done")
