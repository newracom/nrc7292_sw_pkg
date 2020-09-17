#!/usr/bin/python

import os
import time
import commands
import sys

arg1 = int(sys.argv[1])

os.system("python /home/pi/nrc_pkg/etc/python/shell.py run --cmd=" + "\"phy rxgain %d\""%(arg1))
