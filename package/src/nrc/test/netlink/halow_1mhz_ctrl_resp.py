import nrcnetlink 
import subprocess
import argparse
import unittest

nl = nrcnetlink.NrcNetlink()

resp = nl.wfa_halow_set("1mhz_ctrl_resp", "Enable")
#resp = nl.wfa_halow_set("1mhz_ctrl_resp", "Disable")

print resp 
