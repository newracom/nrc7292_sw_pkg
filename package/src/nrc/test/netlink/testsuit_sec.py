import nrcnetlink 
import subprocess
import argparse
import unittest

class TestSecurity(unittest.TestCase):
	def test_trigger_mmic_failure(self):
		nl = nrcnetlink.NrcNetlink()
		ret = nl.sec_trigger_mmic_failure()
		self.assertEqual(ret, "NONE")
	
if __name__ == '__main__':
	unittest.main()
