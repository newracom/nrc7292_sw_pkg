import nrcnetlink 
import subprocess
import argparse
import unittest
import sys

print "Input TID is (" + sys.argv[1] + ")"


nl = nrcnetlink.NrcNetlink()

if len(sys.argv) == 2:
	nl.wfa_capi_sta_send_addba_tid(int(sys.argv[1]))
elif len(sys.argv) == 3:
	mac = sys.argv[2]
	nl.wfa_capi_sta_send_addba(mac.replace(':','').decode('hex'), int(sys.argv[1]))
else:
	print "Usage:"
	print "         test_send_addba.py <tid> <mac address>"
	print "                         <mac address> can be ommitted, if STA is non-AP STA"
	exit(1)
