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

enum nrc_nl_op_cmds {
	NL_WFA_CAPI_STA_GET_INFO,
	NL_WFA_CAPI_STA_SET_11N,
	NL_WFA_CAPI_SEND_ADDBA,
	NL_WFA_CAPI_SEND_DELBA,
	NL_TEST_MMIC_FAILURE,
	NL_SHELL_RUN,
	NL_MGMT_FRAME_INJECTION,
	NL_CMD_LOG_EVENT,
	NL_SHELL_RUN_SIMPLE,
	NL_HALOW_SET_DUT,
	NL_CLI_APP_GET_INFO,
};

enum nrc_nl_op_attrs {
	NL_WFA_CAPI_INTF_ID,
	NL_WFA_CAPI_PARAM_NAME,
	NL_WFA_CAPI_PARAM_STR_VAL,
	NL_WFA_CAPI_PARAM_DESTADDR,
	NL_WFA_CAPI_PARAM_MCS,
	NL_WFA_CAPI_PARAM_TID,
	NL_WFA_CAPI_PARAM_SMPS,
	NL_WFA_CAPI_PARAM_STBC,
	NL_WFA_CAPI_PARAM_VENDOR1,
	NL_WFA_CAPI_PARAM_VENDOR2,
	NL_WFA_CAPI_PARAM_VENDOR3,
	NL_WFA_CAPI_PARAM_RESPONSE,
	NL_SHELL_RUN_CMD,
	NL_SHELL_RUN_CMD_RESP,
	NL_MGMT_FRAME_INJECTION_STYPE,
	NL_CMD_LOG_MSG,
	NL_CMD_LOG_TYPE,
	NL_HALOW_PARAM_NAME,
	NL_HALOW_PARAM_STR_VAL,
	NL_HALOW_RESPONSE,
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

int netlink_send_data(char cmd_type, char* param1, char* response);

#endif /* _CLI_NETLINK_H_ */
