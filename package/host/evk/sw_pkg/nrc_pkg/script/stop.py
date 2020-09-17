#!/usr/bin/python

import os
import time
import commands

def stopNAT():
    os.system('sudo sh -c "echo 0 > /proc/sys/net/ipv4/ip_forward"')
    os.system("sudo iptables -t nat --flush")
    os.system("sudo iptables --flush")

print "STOP"

print "[0] Stop applications"
os.system("sudo killall -9 wpa_supplicant")
os.system("sudo killall -9 hostapd")
os.system("sudo killall -9 wireshark-gtk")
stopNAT()
time.sleep(1)

print "[1] Remove module"
os.system("sudo rmmod nrc")
time.sleep(2)

print "[2] Done"
