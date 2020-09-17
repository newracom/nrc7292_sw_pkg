# This class provides the testability for WFACAPI 

import argparse
import socket
import time
import netlink
import binascii

NL_WFA_CAPI_STA_GET_INFO	= 0
NL_WFA_CAPI_STA_SET_11N		= 1 
NL_WFA_CAPI_STA_SEND_ADDBA	= 2 
NL_WFA_CAPI_STA_SEND_DELBA	= 3
NL_TEST_MMIC_FAILURE		= 4
NL_SHELL_RUN			= 5
NL_MGMT_FRAME_INJECTION		= 6
NL_CMD_LOG_EVENT		= 7
NL_CMD_SHELL_RUN_SIMPLE		= 8
NL_HALOW_SET_DUT		= 9

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
NL_LOG_MSG = 15
NL_CMD_LOG_TYPE = 16
NL_HALOW_PARAM_NAME = 17
NL_HALOW_PARAM_STR_VAL = 18
NL_HALOW_RESPONSE = 19

class NrcNetlink(object):
    def __init__(self):
        self._conn = netlink.Connection(netlink.NETLINK_GENERIC)
        self._fid = netlink.genl_controller.get_family_id(b'NRC-NL-FAM')

    def wfa_capi_sta_get_info(self):
        attrs = []
        attrs.append(netlink.NulStrAttr(NL_WFA_CAPI_INTF_ID, b'test'))
        req = netlink.GenlMessage(self._fid, NL_WFA_CAPI_STA_GET_INFO,
			flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
			attrs = attrs)

	resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs

    def wfa_capi_sta_set_11n(self, cmd, value):
        attrs = []
        attrs.append(netlink.NulStrAttr(NL_WFA_CAPI_INTF_ID, b'test'))
        attrs.append(netlink.NulStrAttr(NL_WFA_CAPI_PARAM_NAME, cmd))
        attrs.append(netlink.NulStrAttr(NL_WFA_CAPI_PARAM_STR_VAL, value))
        req = netlink.GenlMessage(self._fid, NL_WFA_CAPI_STA_SET_11N,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs[NL_WFA_CAPI_PARAM_RESP].nulstr()

    def wfa_capi_sta_send_addba_tid(self, tid):
        attrs = []
        attrs.append(netlink.U8Attr(NL_WFA_CAPI_PARAM_TID, tid))

        req = netlink.GenlMessage(self._fid, NL_WFA_CAPI_STA_SEND_ADDBA,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs[NL_WFA_CAPI_PARAM_RESP].nulstr()

    def wfa_capi_sta_send_addba(self, macaddr, tid):
        attrs = []
        attrs.append(netlink.AddrAttr(NL_WFA_CAPI_PARAM_DESTADDR, macaddr))
        attrs.append(netlink.U8Attr(NL_WFA_CAPI_PARAM_TID, tid))

        req = netlink.GenlMessage(self._fid, NL_WFA_CAPI_STA_SEND_ADDBA,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs[NL_WFA_CAPI_PARAM_RESP].nulstr()

    def wfa_capi_sta_send_delba_tid(self, tid):
        attrs = []
        attrs.append(netlink.U8Attr(NL_WFA_CAPI_PARAM_TID, tid))

        req = netlink.GenlMessage(self._fid, NL_WFA_CAPI_STA_SEND_DELBA,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs[NL_WFA_CAPI_PARAM_RESP].nulstr()

    def wfa_capi_sta_send_delba(self, macaddr, tid):
        attrs = []
        attrs.append(netlink.AddrAttr(NL_WFA_CAPI_PARAM_DESTADDR, macaddr))
        attrs.append(netlink.U8Attr(NL_WFA_CAPI_PARAM_TID, tid))

        req = netlink.GenlMessage(self._fid, NL_WFA_CAPI_STA_SEND_DELBA,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs[NL_WFA_CAPI_PARAM_RESP].nulstr()

    def sec_trigger_mmic_failure(self):
        attrs = []
	# use default TID and TA
        req = netlink.GenlMessage(self._fid, NL_TEST_MMIC_FAILURE,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs[NL_WFA_CAPI_PARAM_RESP].nulstr()

    def nrc_shell_cmd(self, cmd):
        attrs = []
        attrs.append(netlink.AddrAttr(NL_SHELL_RUN_CMD, cmd))
        req = netlink.GenlMessage(self._fid, NL_SHELL_RUN,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs[NL_SHELL_RUN_CMD_RESP].str()

    def nrc_inject_mgmt_frame(self, stype):
        attrs = []
        attrs.append(netlink.U8Attr(NL_MGMT_FRAME_INJECTION_STYPE, stype))
        req = netlink.GenlMessage(self._fid, NL_MGMT_FRAME_INJECTION,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)

    def wfa_halow_set(self, cmd, value):
        attrs = []
        attrs.append(netlink.NulStrAttr(NL_HALOW_PARAM_NAME, cmd))
        attrs.append(netlink.NulStrAttr(NL_HALOW_PARAM_STR_VAL, value))
        req = netlink.GenlMessage(self._fid, NL_HALOW_SET_DUT,
                                  flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
                                  attrs = attrs)
        resp = req.send_and_recv(self._conn)
	resp_attrs = netlink.parse_attributes(resp.payload[4:])
	return resp_attrs[NL_HALOW_RESPONSE].nulstr()
