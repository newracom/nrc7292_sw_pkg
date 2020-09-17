#!/usr/bin/env python
import fire
import nrcnetlink 

class nrc_shell(object):
	def run(self, cmd):
		nl = nrcnetlink.NrcNetlink()
		ret = nl.nrc_shell_cmd(cmd)
		print(ret)

def main():
	fire.Fire(nrc_shell)

if __name__ == '__main__':
	main()
