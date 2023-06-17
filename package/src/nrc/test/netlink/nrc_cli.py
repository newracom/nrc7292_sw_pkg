#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import ctypes
import fcntl
import logging
import math
import os
import signal
import socket
import struct
import sys
import time
import threading, requests, time
import argparse

#import nrcnetlink

from termcolor import colored
from docopt import docopt
from terminaltables import AsciiTable
import libnl.handlers
from libnl.attr import nla_parse, nla_parse_nested, nla_put, nla_put_nested, nla_put_u32, nla_get_u8, nla_get_string, nla_put_string
from libnl.error import errmsg
from libnl.genl.ctrl import genl_ctrl_resolve, genl_ctrl_resolve_grp
from libnl.genl.genl import genl_connect, genlmsg_attrdata, genlmsg_attrlen, genlmsg_put
from libnl.linux_private.genetlink import genlmsghdr
from libnl.linux_private.netlink import NLM_F_DUMP
from libnl.msg import nlmsg_alloc, nlmsg_data, nlmsg_hdr
from libnl.nl import nl_recvmsgs, nl_send_auto
from libnl.nl80211 import nl80211
from libnl.nl80211.helpers import parse_bss
from libnl.nl80211.iw_scan import bss_policy
from libnl.socket_ import nl_socket_add_membership, nl_socket_alloc, nl_socket_drop_membership
from libnl.misc import get_string

prompt = 'nrc_cli>'

NL_WFA_CAPI_STA_GET_INFO    = 0
NL_WFA_CAPI_STA_SET_11N     = 1 
NL_WFA_CAPI_STA_SEND_ADDBA  = 2 
NL_WFA_CAPI_STA_SEND_DELBA  = 3 
NL_TEST_MMIC_FAILURE        = 4
NL_SHELL_RUN                = 5
NL_MGMT_FRAME_INJECTION     = 6
NL_CMD_LOG_EVENT            = 7
NL_SHELL_RUN_SIMPLE         = 8

NL_WFA_CAPI_INTF_ID = 0
NL_WFA_CAPI_PARAM_NAME = 1
NL_WFA_CAPI_PARAM_STR_VAL = 2
NL_WFA_CAPI_PARAM_DESTADDR = 3
NL_WFA_CAPI_PARAM_MCS = 4
NL_WFA_CAPI_PARAM_TID = 5
NL_WFA_CAPI_PARAM_SMPS = 6
NL_WFA_CAPI_PARAM_STBC = 7
NL_WFA_CAPI_PARAM_VENDOR1 = 8
NL_WFA_CAPI_PARAM_VENDOR2 = 9
NL_WFA_CAPI_PARAM_VENDOR3 = 10
NL_WFA_CAPI_PARAM_RESP = 11
NL_SHELL_RUN_CMD = 12
NL_SHELL_RUN_CMD_RESP = 13
NL_MGMT_FRAME_INJECTION_STYPE = 14
NL_CMD_LOG_MSG = 15
NL_CMD_LOG_TYPE = 16


parser = argparse.ArgumentParser(description='Newracom Host CLI')
parser.add_argument("-C", "--config", help="config for firmware")
args = parser.parse_args()

def user_input(prompt):
	user = raw_input(prompt).lower()
	return user

def error(message, code=1):
	if message:
		print('ERROR: {0}'.format(message), file=sys.stderr)
	else:
		print(file=sys.stderr)
	sys.exit(code)


def ok(no_exit, func, *args, **kwargs):
	ret = func(*args, **kwargs)
	if no_exit or ret >= 0:
		return ret
	reason = errmsg[abs(ret)]
	error('{0}() returned {1} ({2})'.format(func.__name__, ret, reason))


def error_handler(_, err, arg):
	arg.value = err.error
	return libnl.handlers.NL_STOP


def ack_handler(_, arg):
	arg.value = 0
	return libnl.handlers.NL_STOP

def level_print(level, strmsg):
	lvstr = {
			0:colored(("%s"    %strmsg),"white"),
			1:colored(("%s"  %strmsg),"green"),
			2:colored(("%s" %strmsg),"magenta"),
			3:colored(("%s"%strmsg),"red"),
			4:colored(("%s"  %strmsg),"green")
			}

	print (lvstr.get(level, "Invalid"), end='')

def callback_trigger(msg, arg):
	gnlh = genlmsghdr(nlmsg_data(nlmsg_hdr(msg)))

	tb = dict((i, None) for i in range(10 + 1))
	nla_parse(tb, nl80211.NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), None)

	if not tb[NL_CMD_LOG_MSG]:
		print('not exist')
		return libnl.handlers.NL_SKIP

	if not tb[NL_CMD_LOG_TYPE]:
		print('not exist')
		return libnl.handlers.NL_SKIP

	level_print(nla_get_u8(tb[NL_CMD_LOG_TYPE]), nla_get_string(tb[NL_CMD_LOG_MSG]))

	return libnl.handlers.NL_SKIP


def mcast_handler(sk, driver_id):

	mcid = ok(0, genl_ctrl_resolve_grp, sk, b'NRC-NL-FAM', b'nrc-log')
	ret = nl_socket_add_membership(sk, mcid)

	err = ctypes.c_int(1)  
	results = ctypes.c_int(-1)  
	cb = libnl.handlers.nl_cb_alloc(libnl.handlers.NL_CB_DEFAULT)
	libnl.handlers.nl_cb_set(cb, libnl.handlers.NL_CB_VALID, libnl.handlers.NL_CB_CUSTOM, callback_trigger, results)
	libnl.handlers.nl_cb_err(cb, libnl.handlers.NL_CB_CUSTOM, error_handler, err)
	libnl.handlers.nl_cb_set(cb, libnl.handlers.NL_CB_ACK, libnl.handlers.NL_CB_CUSTOM, ack_handler, err)
	libnl.handlers.nl_cb_set(cb, libnl.handlers.NL_CB_SEQ_CHECK, libnl.handlers.NL_CB_CUSTOM, lambda *_: libnl.handlers.NL_OK, None)  # Ignore sequence checking.

	try:
		while True:
			ret = nl_recvmsgs(sk, cb)
			if ret < 0:
				return;
	except KeyboardInterrupt:
		pass
	finally:
		nl_socket_drop_membership(sk, mcid) 

def nrc_shell_cmd(sk, driver_id, cmd):
	msg = nlmsg_alloc()
	genlmsg_put(msg, 0, 0, driver_id, 0, 0, NL_SHELL_RUN_SIMPLE, 0)
	nla_put_string(msg, NL_SHELL_RUN_CMD, cmd)
	ret = nl_send_auto(sk, msg)

def main():
	sk = nl_socket_alloc()
	ok(0, genl_connect, sk)
	driver_id = ok(0, genl_ctrl_resolve, sk, b'NRC-NL-FAM')
	
	if args.config:
		nrc_shell_cmd(sk, driver_id, args.config)
		return 0;
	mcast_thread = threading.Thread(target=mcast_handler, args=(sk,driver_id))
	mcast_thread.daemon = True
	mcast_thread.start()

	while True:
		user = user_input(prompt)
		print(user)
		nrc_shell_cmd(sk, driver_id, user)

if __name__ == '__main__':
	main()
