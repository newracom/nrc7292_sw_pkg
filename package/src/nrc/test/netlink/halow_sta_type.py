import nrcnetlink 
import subprocess
import argparse
import unittest

nl = nrcnetlink.NrcNetlink()

res = nl.wfa_halow_set("STA_Type", "Sensor")
#res = nl.wfa_halow_set("STA_Type", "Non_Sensor")
#res = nl.wfa_halow_set("STA_Type", "Both")

print res
