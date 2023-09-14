import nrcnetlink 
import subprocess
import argparse
import unittest

nl = nrcnetlink.NrcNetlink()

res = nl.wfa_halow_set("Block_Frame", "Enable")
#res = nl.wfa_halow_set("Block_Frame", "Disable")

print res
