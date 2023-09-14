# This class provides the testability for WFACAPI

import argparse
import socket
import time
import netlink
import binascii

#########################################################################################
# README: match the numeric definition with nrc-netlink.h - sw.ki - 2019-0409 - CB#NONE
#  It must exactly match nl cmd, attr as defined by the driver.
#########################################################################################
NL_WFA_CAPI_STA_GET_INFO				= 0
NL_WFA_CAPI_STA_SET_11N					= 1
NL_WFA_CAPI_STA_SEND_ADDBA				= 2
NL_WFA_CAPI_STA_SEND_DELBA				= 3
NL_TEST_MMIC_FAILURE					= 4
NL_SHELL_RUN							= 5
NL_MGMT_FRAME_INJECTION					= 6
NL_CMD_LOG_EVENT						= 7
NL_CMD_SHELL_RUN_SIMPLE					= 8
NL_HALOW_SET_DUT						= 9
NL_CLI_APP_GET_INFO             		= 10
NL_WFA_CAPI_LISTEN_INTERVAL 			= 11
NL_WFA_CAPI_BSS_MAX_IDLE 				= 12
NL_WFA_CAPI_BSS_MAX_IDLE_OFFSET			= 13
NL_MIC_SCAN                     		= 14
NL_FRAME_INJECTION						= 16
NL_SET_IE								= 17
NL_SET_SAE								= 18
NL_SHELL_RUN_RAW                  		= 19
NL_AUTO_BA_TOGGLE						= 20

NL_WFA_CAPI_INTF_ID                     = 0
NL_WFA_CAPI_PARAM_NAME                  = 1
NL_WFA_CAPI_PARAM_STR_VAL               = 2
NL_WFA_CAPI_PARAM_DESTADDR              = 3
NL_WFA_CAPI_PARAM_MCS                   = 4
NL_WFA_CAPI_PARAM_TID                   = 5
NL_WFA_CAPI_PARAM_SMPS                  = 6
NL_WFA_CAPI_PARAM_STBC                  = 7
NL_WFA_CAPI_PARAM_VENDOR1               = 8
NL_WFA_CAPI_PARAM_VENDOR2               = 9
NL_WFA_CAPI_PARAM_VENDOR3               = 10
NL_WFA_CAPI_PARAM_RESP                  = 11
NL_SHELL_RUN_CMD                        = 12
NL_SHELL_RUN_CMD_RESP                   = 13
NL_MGMT_FRAME_INJECTION_STYPE           = 14
NL_LOG_MSG                              = 15
NL_CMD_LOG_TYPE                         = 16
NL_HALOW_PARAM_NAME                     = 17
NL_HALOW_PARAM_STR_VAL                  = 18
NL_HALOW_RESPONSE                       = 19
NL_WFA_CAPI_PARAM_VIF_ID                = 20
NL_WFA_CAPI_PARAM_BSS_MAX_IDLE          = 21
NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET   = 22
NL_WFA_CAPI_PARAM_LISTEN_INTERVAL       = 23
NL_MIC_SCAN_CHANNEL_START               = 24
NL_MIC_SCAN_CHANNEL_END                 = 25
NL_MIC_SCAN_CHANNEL_BITMAP              = 26
NL_FRAME_INJECTION_BUFFER				= 28
NL_SET_IE_EID							= 29
NL_SET_IE_LENGTH						= 30
NL_SET_IE_DATA							= 31
NL_SET_SAE_EID							= 32
NL_SET_SAE_LENGTH						= 33
NL_SET_SAE_DATA							= 34
NL_SHELL_RUN_CMD_RAW					= 35
NL_SHELL_RUN_CMD_RESP_RAW				= 36
NL_AUTO_BA_ON							= 37

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

	def wfa_capi_set_bss_max_idle_offset(self, value):
		attrs = []
		attrs.append(netlink.S32Attr(NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET, value))

		req = netlink.GenlMessage(self._fid, NL_WFA_CAPI_BSS_MAX_IDLE_OFFSET,
				flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
				attrs = attrs)
		resp = req.send_and_recv(self._conn)
		resp_attrs = netlink.parse_attributes(resp.payload[4:])
		return resp_attrs[NL_WFA_CAPI_PARAM_RESP].nulstr()

	def wfa_capi_set_bss_max_idle(self, vif_id, value, autotsf):
		attrs = []
		attrs.append(netlink.S32Attr(NL_WFA_CAPI_PARAM_VIF_ID, vif_id))
		attrs.append(netlink.S32Attr(NL_WFA_CAPI_PARAM_BSS_MAX_IDLE, value))
		attrs.append(netlink.S32Attr(NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET, autotsf))

		req = netlink.GenlMessage(self._fid, NL_WFA_CAPI_BSS_MAX_IDLE,
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

	def nrc_mic_scan(self, start, end):
		attrs = []
		attrs.append(netlink.S32Attr(NL_MIC_SCAN_CHANNEL_START, start))
		attrs.append(netlink.S32Attr(NL_MIC_SCAN_CHANNEL_END, end))
		req = netlink.GenlMessage(self._fid, NL_MIC_SCAN,
				flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
				attrs = attrs)
		resp = req.send_and_recv(self._conn)
		resp_attrs = netlink.parse_attributes(resp.payload[4:])
		return resp_attrs[NL_MIC_SCAN_CHANNEL_BITMAP].u32()

	def nrc_frame_injection(self, buffer):
		attrs = []
		attrs.append(netlink.NulStrAttr(NL_FRAME_INJECTION_BUFFER, buffer))
		req = netlink.GenlMessage(self._fid, NL_FRAME_INJECTION,
				flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
				attrs = attrs)
		req.send(self._conn)
		return

	def nrc_set_ie(self, eid, length, data):
		attrs = []
		attrs.append(netlink.U16Attr(NL_SET_IE_EID, eid))
		attrs.append(netlink.U8Attr(NL_SET_IE_LENGTH, length))
		attrs.append(netlink.NulStrAttr(NL_SET_IE_DATA, data))
		req = netlink.GenlMessage(self._fid, NL_SET_IE,
				flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
				attrs = attrs)
		req.send(self._conn)
		return

	def nrc_set_sae(self, eid, length, data):
		print(self)
		attrs = []
		attrs.append(netlink.U16Attr(NL_SET_SAE_EID, eid))
		print("user area nrcnetlink log : eid appended")
		attrs.append(netlink.U16Attr(NL_SET_SAE_LENGTH, length))
		print("user area nrcnetlink log : length appended")
		attrs.append(netlink.NulStrAttr(NL_SET_SAE_DATA, data))
		print("user area nrcnetlink log : data appended")
		req = netlink.GenlMessage(self._fid, NL_SET_SAE_DATA,
				flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
				attrs = attrs)
		print("user area nrcnetlink log : message created")
		req.send(self._conn)
		print("user area nrcnetlink log : message sent to driver")
		return

	def nrc_auto_ba_toggle(self, on):
		attrs = []
		attrs.append(netlink.U8Attr(NL_AUTO_BA_ON, on))
		req = netlink.GenlMessage(self._fid, NL_AUTO_BA_TOGGLE,
				flags = netlink.NLM_F_REQUEST | netlink.NLM_F_ACK,
				attrs = attrs)
		req.send(self._conn)
		return
