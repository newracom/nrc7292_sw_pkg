import os
import sys
import time
import ctypes
import threading
import libnl.handlers
from libnl.error import errmsg
from libnl.nl import nl_recvmsgs
from libnl.attr import nla_parse
from libnl.nl80211 import nl80211
from libnl.msg import nlmsg_data, nlmsg_hdr
from libnl.linux_private.genetlink import genlmsghdr
from libnl.genl.ctrl import genl_ctrl_resolve, genl_ctrl_resolve_grp
from libnl.genl.genl import genl_connect, genlmsg_attrdata, genlmsg_attrlen
from libnl.socket_ import nl_socket_add_membership, nl_socket_alloc, nl_socket_drop_membership

NL_CMD_RECOVERY_MSG = 27

PATH_TO_DRIVER = "/home/pi/nrc_pkg/sw/driver/nrc.ko"
PATH_TO_CONFIG = "/home/pi/nrc_pkg/script/conf/{}/"
FIRMWARE = "uni_s1g.bin"

class Recovery:
	def __init__(self):
		self.recovery = False
		self.recovery_count = 0
		self.keyboard = False
		self.sk = None
		self.driver_id = None
		self.station_type = 0
		self.config = -1
		self.country = "US"

	def trigger_recovery(self):
		self.recovery_count += 1
		self.recovery = False

		#startup routine
		self.startup()

		print("Recovery Count: {}".format(self.recovery_count))
		self.connect()

	def startup(self): #example
		app = "wpa_supplicant" if self.is_sta() else "hostapd"

		os.system("sudo killall -9 {} > /dev/null 2>&1".format(app))
		os.system("sudo rmmod nrc > /dev/null 2>&1")

		os.system("sudo insmod {} fw_name={} > /dev/null".format(PATH_TO_DRIVER, FIRMWARE))
		time.sleep(5)
		os.system("sudo {} {} {}/{} -B > /dev/null".format(app, "-iwlan0 -c" if self.is_sta() else "", PATH_TO_CONFIG.format(self.country), self.get_config()))
		time.sleep(7)

	def parse_arg(self):
		if (len(sys.argv) < 4):
			sys.exit("Usage: python run_recovery.py <station-type> <security-type> <country-code>")

		self.station_type = int(sys.argv[1])
		self.config = int(sys.argv[2])
		self.country = sys.argv[3]

	def is_sta(self):
		return self.station_type == 0

	def get_config(self):
		station = "sta" if self.is_sta() else "ap"
		if self.config == 0:
			return "{}_halow_open.conf".format(station)
		elif self.config == 1:
			return "{}_halow_wpa2.conf".format(station)
		elif self.config == 2:
			return "{}_halow_owe.conf".format(station)
		elif self.config == 3:
			return "{}_halow_sae.conf".format(station)
		else:
			return "{}_halow_open.conf".format(station)

	def connect(self):
		self.sk = nl_socket_alloc()
		recovery.ok(0, genl_connect, self.sk)
		self.driver_id = recovery.ok(0, genl_ctrl_resolve, self.sk, b'NRC-NL-FAM')

	def ok(self, no_exit, func, *args, **kwargs):
		ret = func(*args, **kwargs)
		if no_exit or ret >= 0:
			return ret
		reason = errmsg[abs(ret)]
		print('{0}() returned {1} ({2})'.format(func.__name__, ret, reason))

	def callback_trigger(self, msg, arg):
		gnlh = genlmsghdr(nlmsg_data(nlmsg_hdr(msg)))

		tb = dict((i, None) for i in range(10 + 1))
		nla_parse(tb, nl80211.NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), None)
		if tb.get(NL_CMD_RECOVERY_MSG):
			self.recovery = True

		return libnl.handlers.NL_STOP

	def mcast_handler(self):
		mcid = self.ok(0, genl_ctrl_resolve_grp, self.sk, b'NRC-NL-FAM', b'nrc-log')
		ret = nl_socket_add_membership(self.sk, mcid)
		err = ctypes.c_int(1)
		results = ctypes.c_int(-1)
		cb = libnl.handlers.nl_cb_alloc(libnl.handlers.NL_CB_DEFAULT)
		libnl.handlers.nl_cb_set(cb, libnl.handlers.NL_CB_VALID, libnl.handlers.NL_CB_CUSTOM, self.callback_trigger, results)
		libnl.handlers.nl_cb_set(cb, libnl.handlers.NL_CB_SEQ_CHECK, libnl.handlers.NL_CB_CUSTOM, lambda *_: libnl.handlers.NL_OK, None)

		try:
			while True:
				ret = nl_recvmsgs(self.sk, cb)
				if self.recovery: break
		except KeyboardInterrupt:
			self.keyboard = True
		finally:
			nl_socket_drop_membership(self.sk, mcid)

if __name__ == "__main__":
	recovery = Recovery()
	recovery.parse_arg()
	recovery.connect()

	while True:
		print("Waiting for recovery trigger...")
		recovery.mcast_handler()
		if recovery.keyboard:
			break
		if recovery.recovery:
			print("Recovery Triggered...")
			recovery.trigger_recovery()
