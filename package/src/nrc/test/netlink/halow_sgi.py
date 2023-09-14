import nrcnetlink 
import subprocess
import argparse
import unittest

nl = nrcnetlink.NrcNetlink()

resp = nl.wfa_halow_set("Enable_SGI", "All")
#resp = nl.wfa_halow_set("Disable_SGI", "All")

print resp 
