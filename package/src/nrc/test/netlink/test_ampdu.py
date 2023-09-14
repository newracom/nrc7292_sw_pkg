import nrcnetlink 
import subprocess
import argparse
import unittest

nl = nrcnetlink.NrcNetlink()
nl.wfa_capi_sta_set_11n("ADDBA_Reject", "disable")
