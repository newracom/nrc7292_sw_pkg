/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef _CLI_NETLINK_H_
#define _CLI_NETLINK_H_

struct nrc;

/**
 * README: match the numeric definition with nrcnetlink.py
 * Must match nl cmd, attr as defined by the driver.
 */
enum nrc_nl_op_cmds {
	NL_WFA_CAPI_STA_GET_INFO			= 0,
	NL_WFA_CAPI_STA_SET_11N				= 1,
	NL_WFA_CAPI_SEND_ADDBA				= 2,
	NL_WFA_CAPI_SEND_DELBA				= 3,
	NL_TEST_MMIC_FAILURE				= 4,
	NL_SHELL_RUN						= 5,
	NL_MGMT_FRAME_INJECTION				= 6,
	NL_CMD_LOG_EVENT					= 7,
	NL_SHELL_RUN_SIMPLE					= 8,
	NL_HALOW_SET_DUT					= 9,
	NL_CLI_APP_GET_INFO					= 10,
	NL_WFA_CAPI_LISTEN_INTERVAL			= 11,
	NL_WFA_CAPI_BSS_MAX_IDLE			= 12,
	NL_WFA_CAPI_BSS_MAX_IDLE_OFFSET		= 13,
	NL_MIC_SCAN							= 14,
	NL_CMD_RECOVERY						= 15,
	NL_FRAME_INJECTION					= 16,
	NL_SET_IE							= 17,
	NL_SET_SAE							= 18,
	NL_SHELL_RUN_RAW                    = 19,
	NL_AUTO_BA_TOGGLE                   = 20,
	NL_CLI_APP_DRIVER               = 21,
};

enum nrc_nl_op_attrs {
	NL_WFA_CAPI_INTF_ID					= 0,
	NL_WFA_CAPI_PARAM_NAME				= 1,
	NL_WFA_CAPI_PARAM_STR_VAL			= 2,
	NL_WFA_CAPI_PARAM_DESTADDR			= 3,
	NL_WFA_CAPI_PARAM_MCS				= 4,
	NL_WFA_CAPI_PARAM_TID				= 5,
	NL_WFA_CAPI_PARAM_SMPS				= 6,
	NL_WFA_CAPI_PARAM_STBC				= 7,
	NL_WFA_CAPI_PARAM_VENDOR1			= 8,
	NL_WFA_CAPI_PARAM_VENDOR2			= 9,
	NL_WFA_CAPI_PARAM_VENDOR3			= 10,
	NL_WFA_CAPI_PARAM_RESPONSE			= 11,
	NL_SHELL_RUN_CMD					= 12,
	NL_SHELL_RUN_CMD_RESP				= 13,
	NL_MGMT_FRAME_INJECTION_STYPE		= 14,
	NL_CMD_LOG_MSG						= 15,
	NL_CMD_LOG_TYPE						= 16,
	NL_HALOW_PARAM_NAME					= 17,
	NL_HALOW_PARAM_STR_VAL				= 18,
	NL_HALOW_RESPONSE					= 19,
	NL_WFA_CAPI_PARAM_VIF_ID			= 20,
	NL_WFA_CAPI_PARAM_BSS_MAX_IDLE		= 21,
	NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET	= 22,
	NL_WFA_CAPI_PARAM_LISTEN_INTERVAL	= 23,
	NL_MIC_SCAN_CHANNEL_START			= 24,
	NL_MIC_SCAN_CHANNEL_END				= 25,
	NL_MIC_SCAN_CHANNEL_BITMAP			= 26,
	NL_CMD_RECOVERY_MSG					= 27,
	NL_FRAME_INJECTION_BUFFER			= 28,
	NL_SET_IE_EID						= 29,
	NL_SET_IE_LENGTH					= 30,
	NL_SET_IE_DATA						= 31,
	NL_SET_SAE_EID						= 32,
	NL_SET_SAE_LENGTH					= 33,
	NL_SET_SAE_DATA						= 34,
	NL_SHELL_RUN_CMD_RAW				= 35,
	NL_SHELL_RUN_CMD_RESP_RAW			= 36,
	NL_AUTO_BA_ON						= 37,
	NL_CLI_APP_DRIVER_CMD				= 38,
	NL_CLI_APP_DRIVER_CMD_RESP			= 39,
	NL_WFA_CAPI_ATTR_LAST,
	MAX_NL_WFA_CAPI_ATTR = NL_WFA_CAPI_ATTR_LAST-1,
};

#define NL_WFA_CAPI_RESP_OK   ("COMPLETE")
#define NL_WFA_CAPI_RESP_ERR  ("ERROR")
#define NL_WFA_CAPI_RESP_NONE ("NONE")

#define NL_HALOW_RESP_OK   ("OK")
#define NL_HALOW_RESP_ERR  ("ERROR")
#define NL_HALOW_RESP_NOT_SUPP  ("Not supported")

#define NL_MSG_MAX_RESPONSE_SIZE	1024

enum nrc_nl_multicast_grp {
	NL_MCGRP_WFA_CAPI_RESPONSE,
	NL_MCGRP_NRC_LOG,
	NL_MCGRP_LAST,
};

int netlink_send_data_with_retry(char cmd_type, char* param, char* response, int retry_count);
int netlink_send_data(char cmd_type, char* param1, char* response);

#endif /* _CLI_NETLINK_H_ */
