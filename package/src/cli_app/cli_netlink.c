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

#ifndef _CLI_NETLINK_
#define _CLI_NETLINK_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/genetlink.h>

#include "cli_cmd.h"
#include "cli_util.h"
#include "cli_netlink.h"

#define NL_MSG_BUFFER_SIZE	1024
#define NL_TID_BYTE_SIZE	1
#define NL_MAC_ADDRESS_BYTE_SIZE	6

#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char*)(na) + NLA_HDRLEN))

typedef struct nl_msg{ //memory for netlink request and response messages - headers are included
	struct nlmsghdr n;
	struct genlmsghdr g;
	char buf[NL_MSG_BUFFER_SIZE];
} nl_msg_t;

int netlink_send_data_with_retry(char cmd_type, char* param, char* response, int retry_count)
{
	int infinite = (retry_count == -1);
	do
	{
		if(netlink_send_data(cmd_type, param, response)==0)
			return 0;
		cli_delay_ms(COMMAND_DELAY_MS);
	} while(infinite || retry_count-- > 0);
	return 1;
}

static char netlink_set_attribute_type(char cmd_type)
{
	char ret = 0;
	switch(cmd_type){
		case NL_SHELL_RUN:
		case NL_CLI_APP_GET_INFO:
			ret = NL_SHELL_RUN_CMD;
			break;
		case NL_SHELL_RUN_RAW:
			ret = NL_SHELL_RUN_CMD_RAW;
			break;
		case NL_CLI_APP_DRIVER:
			ret = NL_CLI_APP_DRIVER_CMD;
			break;
		default:
			ret = 0;
	}
	return ret;
}

int netlink_send_data(char cmd_type, char* param, char* response)
{
	char *pos;
	int nl_fd;  //netlink socket's file descriptor
	struct sockaddr_nl nl_address; //netlink socket address
	int nl_family_id = 0; //The family ID resolved by the netlink controller for this userspace program
	int nl_rxtx_length; //Number of bytes sent or received via send() or recv()
	struct nlattr *nl_na; //pointer to netlink attributes structure within the payload
	nl_msg_t nl_request_msg, nl_response_msg;
	int parse_data_length=0;
	char *argv[10] = { NULL, };
	int argc = 0;
	int len = 0;
	char tid;
	char string[16];

	if ((pos = strchr(param, '\n')) != NULL){
		*pos = '\0';
	}

	//Step 0: reset response data
	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);

	//Step 1: Open the socket. Note that protocol = NETLINK_GENERIC
	nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if (nl_fd < 0) {
		printf("Error socket() :%d\n",  nl_rxtx_length);
		return -1;
	}

	//Step 2: Bind the socket.
	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;
	nl_address.nl_groups = 0;

	if (bind(nl_fd, (struct sockaddr *) &nl_address, sizeof(nl_address)) < 0) {
		printf("Error bind()\n");
		close(nl_fd);
		return -1;
	}

	//Step 3. Resolve the family ID corresponding to the string "NRC-NL-FAM"
	//Populate the netlink header
	nl_request_msg.n.nlmsg_type = GENL_ID_CTRL;
	nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
	nl_request_msg.n.nlmsg_seq = 0;
	nl_request_msg.n.nlmsg_pid = getpid();
	nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	//Populate the payload's "family header" : which in our case is genlmsghdr
	nl_request_msg.g.cmd = CTRL_CMD_GETFAMILY;
	nl_request_msg.g.version = 0x1;
	//Populate the payload's "netlink attributes"
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_request_msg);
	nl_na->nla_type = CTRL_ATTR_FAMILY_NAME;
	nl_na->nla_len = strlen("NRC-NL-FAM") + 1 + NLA_HDRLEN;
	strcpy(NLA_DATA(nl_na), "NRC-NL-FAM"); //Family name length can be upto 16 chars including \0

	nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;

	//Send the family ID request message to the netlink controller
	nl_rxtx_length = sendto(nl_fd, (char *)&nl_request_msg, nl_request_msg.n.nlmsg_len,
						0, (struct sockaddr *) &nl_address, sizeof(nl_address));

	if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
		printf("Error sendto() :%d\n",  nl_rxtx_length);
		close(nl_fd);
		return -1;
	}

	//Wait for the response message
	nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);

	if (nl_rxtx_length < 0) {
		printf("Error recv() :%d\n",  nl_rxtx_length);
		return -1;
	}

	//Validate response message
	if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
		fprintf(stderr, "nl_rxtx_length=%d\n", nl_rxtx_length);
		fprintf(stderr, "family ID request : invalid message\n");
		return -1;
	}

	if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //error
		fprintf(stderr, "family ID request : receive error\n");
		return -1;
	}

	//Extract family ID
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);
	nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
	if (nl_na->nla_type == CTRL_ATTR_FAMILY_ID) {
		nl_family_id = *(__u16 *) NLA_DATA(nl_na);
	}

	//Step 4. Send own custom message
	memset(&nl_request_msg, 0, sizeof(nl_request_msg));
	memset(&nl_response_msg, 0, sizeof(nl_response_msg));

	nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	nl_request_msg.n.nlmsg_type = nl_family_id;
	nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
	nl_request_msg.n.nlmsg_seq = 60;
	nl_request_msg.n.nlmsg_pid = getpid();
	nl_request_msg.g.cmd = cmd_type;

	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_request_msg);

	if(cmd_type == NL_SHELL_RUN  || cmd_type == NL_SHELL_RUN_RAW
		|| cmd_type == NL_CLI_APP_GET_INFO || cmd_type == NL_CLI_APP_DRIVER){
		nl_na->nla_type = netlink_set_attribute_type(cmd_type);
		len = strlen(param)+ 1; // including '\0'
		nl_na->nla_len =  len + NLA_HDRLEN; //Message length
		memcpy(NLA_DATA(nl_na), param, len);
		nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);
	}
	else if(cmd_type == NL_WFA_CAPI_SEND_ADDBA || cmd_type == NL_WFA_CAPI_SEND_DELBA){
		argc = util_cmd_parse_line(param, argv);
		memset(string, 0x0, sizeof(string));
		nl_na->nla_type = NL_WFA_CAPI_PARAM_TID;
		memcpy(string, argv[0], 1);
		tid = hex_to_int(string[0]);
		nl_na->nla_len =  NL_TID_BYTE_SIZE  +  NLA_HDRLEN;
		memcpy(NLA_DATA(nl_na), &tid,  NL_TID_BYTE_SIZE);
		nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);
		if(argc == 2){
			nl_na = (struct nlattr *)((char*)nl_na + NLMSG_ALIGN(nl_na->nla_len));
			nl_na->nla_type = NL_WFA_CAPI_PARAM_DESTADDR;
			memset(string, 0x0, sizeof(string));
			macaddr_to_ascii(argv[1], string);
			nl_na->nla_len =  NL_MAC_ADDRESS_BYTE_SIZE + NLA_HDRLEN;
			memcpy(NLA_DATA(nl_na), string, NL_MAC_ADDRESS_BYTE_SIZE);
			nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);
		}
	}
	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;

	//Send the custom message
	nl_rxtx_length = sendto(nl_fd, (char *) &nl_request_msg, nl_request_msg.n.nlmsg_len,
						0, (struct sockaddr *) &nl_address, sizeof(nl_address));

	if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
		printf("Error sendto() :%d\n",  nl_rxtx_length);
		close(nl_fd);
		return -1;
	}

	if(cmd_type == NL_SHELL_RUN  || cmd_type == NL_SHELL_RUN_RAW
		|| cmd_type == NL_CLI_APP_GET_INFO || cmd_type == NL_CLI_APP_DRIVER){
		//Receive reply from kernel
		nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);

		if (nl_rxtx_length < 0) {
			printf("Error recv() :%d\n",  nl_rxtx_length);
			return -1;
		}

		//Validate response message
		if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //Error
			printf("Error while receiving reply from kernel: NACK Received\n");
			close(nl_fd);
			return -1;
		}
		if (nl_rxtx_length < 0) {
			printf("Error while receiving reply from kernel\n");
			close(nl_fd);
			return -1;
		}
		if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
			printf("Error while receiving reply from kernel: Invalid Message\n");
			close(nl_fd);
			return -1;
		}

		//Parse the reply message
		nl_rxtx_length = GENLMSG_PAYLOAD(&nl_response_msg.n);
		nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);
		int str_len = 0;
		int copy_pos = 0;

		if(cmd_type == NL_SHELL_RUN_RAW)
		{
			if ((nl_na->nla_len - 4) > 0)
				memcpy(response, NLA_DATA(nl_na), nl_na->nla_len - 4);
		}
		else
		{
			while(1)
			{
				//printf("Kernel replied: %s %d\n",(char *)NLA_DATA(nl_na), strlen((char *)NLA_DATA(nl_na)));
				str_len =  strlen((char *)NLA_DATA(nl_na));
				strncpy((response+copy_pos),(char *)NLA_DATA(nl_na), str_len);
				parse_data_length += NLA_ALIGN(nl_na->nla_len);

				copy_pos += (str_len );
				if(parse_data_length < nl_rxtx_length){
					*(response+copy_pos)=',';
					copy_pos++;
					nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
				}
				else{
					break;
				}
			}
		}

	}

	//Step 5. Close the socket and quit
	close(nl_fd);
	return 0;
}

#endif /* _CLI_NETLINK_ */
