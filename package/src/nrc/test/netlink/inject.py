#!/usr/bin/env python
import fire
import nrcnetlink 
import time

class nrc_inject(object):
	def assoc(self):
		self.auth()
		time.sleep(1)
		self.assoc_req()

	def assoc_req(self):
		nl = nrcnetlink.NrcNetlink()
		ret = nl.nrc_inject_mgmt_frame(0)

	def auth(self):
		nl = nrcnetlink.NrcNetlink()
		ret = nl.nrc_inject_mgmt_frame(11)

def main():
	fire.Fire(nrc_inject)

if __name__ == '__main__':
	main()
