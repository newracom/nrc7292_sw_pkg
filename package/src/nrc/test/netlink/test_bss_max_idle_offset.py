import nrcnetlink 
import subprocess
import argparse
import unittest
import sys

print "config bss-max-idle-offset (" + sys.argv[1] + ")"

nl = nrcnetlink.NrcNetlink()

if len(sys.argv) == 2:
	nl.wfa_capi_set_bss_max_idle_offset(int(sys.argv[1]))
else:
	print "Usage:"
	print "         test_bss_max_idle_offset .py <ms>"
	exit(1)
