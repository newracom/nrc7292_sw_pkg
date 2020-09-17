#!/usr/bin/python

import sys
import os
import time
import commands
 
def change_channel():
	cmdstr = "sudo iw dev wlan0 set channel " + str(sys.argv[1])
	os.system(cmdstr)

total = len(sys.argv)
cmdargs = str(sys.argv)

if total < 2:
	print ("NewraPeek Channel Change Usage: change_ch.py channelnumber")
else:
	print ("NewraPeek channel number: %s " % str(sys.argv[1]))
	change_channel()