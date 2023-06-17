import nrcnetlink 
import subprocess
import argparse
import unittest
import sys

if len(sys.argv) < 2:
	print "Usage:"
	print "halow_color.py <color value>"

nl = nrcnetlink.NrcNetlink()

resp = nl.wfa_halow_set("Color_Indication", sys.argv[1])

print resp 
