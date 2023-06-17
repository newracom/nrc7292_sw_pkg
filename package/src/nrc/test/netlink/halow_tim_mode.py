import nrcnetlink 
import subprocess
import argparse
import unittest
import sys

nl = nrcnetlink.NrcNetlink()

if len(sys.argv) < 2:
	print "Usage:"
	print "halow_color.py <color value>"

mode = sys.argv[1]

if mode == 'o':
	print "OLB Mode"
	resp = nl.wfa_halow_set("Tim_Mode", "OLB")
elif mode == 's':
	print "Single Mode"
	resp = nl.wfa_halow_set("Tim_Mode", "Single_AID")
elif mode == 'b':
	print "Block Bitmap Mode"
	resp = nl.wfa_halow_set("Tim_Mode", "BlockBitmap")
else:
	print "Usage:"
	print "halow_type_mode.py [o|s|b]"
	print "o : OBL Mode"
	print "s : Single Mode"
	print "b :Block Bitmap Mode"

print resp 
