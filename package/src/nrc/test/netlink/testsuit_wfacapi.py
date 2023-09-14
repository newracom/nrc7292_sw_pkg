import nrcnetlink 
import subprocess
import argparse
import unittest

class TestCAPIs(unittest.TestCase):
	def test_wfa_capi_sta_get_info(self):
		nl = nrcnetlink.NrcNetlink()
		attrs = nl.wfa_capi_sta_get_info()
		self.assertEqual(attrs[nrcnetlink.NL_WFA_CAPI_PARAM_VENDOR1].nulstr(), 'vendor_test1')
		self.assertEqual(attrs[nrcnetlink.NL_WFA_CAPI_PARAM_VENDOR2].nulstr(), 'vendor_test2')
		self.assertEqual(attrs[nrcnetlink.NL_WFA_CAPI_PARAM_VENDOR3].nulstr(), 'vendor_test3')

	def test_neg_wfa_capi_set_11n(self):
		nl = nrcnetlink.NrcNetlink()
		ret = nl.wfa_capi_sta_set_11n("GARBAGE", "Enable")
		self.assertEqual(ret, "ERROR")

	def test_pos_wfa_capi_set_11n(self):
		nl = nrcnetlink.NrcNetlink()
		ret = nl.wfa_capi_sta_set_11n("MCS_FixedRate", "3")
		self.assertEqual(ret, "COMPLETE")

#	def test_pos_wfa_sta_send_addba(self):
#		addr = b'\x14\xdd\xa9\xd7\xaf\x80'
#		nl = nrcnetlink.NrcNetlink()
#		ret = nl.wfa_capi_sta_send_addba(addr, 1)
#		self.assertEqual(ret, "NONE")

	def test_pos_wfa_sta_send_addba_wo_addr(self):
		nl = nrcnetlink.NrcNetlink()
		ret = nl.wfa_capi_sta_send_addba_tid(2)
		self.assertEqual(ret, "NONE")

	
if __name__ == '__main__':
	unittest.main()
