import nrcnetlink 
import subprocess
import argparse
import unittest
import sys


nl = nrcnetlink.NrcNetlink()

if len(sys.argv) == 3:
    print "config bss-max-idle (" + sys.argv[1] + "," + sys.argv[2] + ")"
    nl.wfa_capi_set_bss_max_idle(int(sys.argv[1]), int(sys.argv[2]), 0)
elif len(sys.argv) == 4:
    print "config bss-max-idle (" + sys.argv[1] + "," + sys.argv[2] +"," + sys.argv[3] + ")"
    nl.wfa_capi_set_bss_max_idle(int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]))
else:
    print "Usage:"
    # set bss_max_idle (10) : "test_bss_max_idle.py 0 10" or "test_bss_max_idle.py 0 16385 0"
    print "         test_bss_max_idle.py <vif_id> <value: in unit of 1000TU> <(optional)no_usf_convert: 1 (0:default)"
    exit(1)
