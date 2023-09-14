import nrcnetlink 
import subprocess
import argparse
import unittest

nl = nrcnetlink.NrcNetlink()

resp = nl.wfa_halow_set("AMPDU", "Enable")
#resp = nl.wfa_halow_set("AMPDU", "Disable")

print resp 
