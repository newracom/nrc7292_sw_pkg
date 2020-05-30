#!/usr/bin/python

import os
import time
import commands

print "STOP"

print "[0] Stop applications"
os.system("sudo killall -9 wpa_supplicant")
os.system("sudo killall -9 hostapd")
os.system("sudo killall -9 wireshark-gtk")
time.sleep(2)

print "[1] Remove module"
os.system("sudo rmmod nrc")
time.sleep(2)

print "[2] Done"
