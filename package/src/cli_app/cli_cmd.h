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

#ifndef _CLI_CMD_H_
#define _CLI_CMD_H_

#define NRC_MAX_ARGV			20
#define NRC_MAX_CMDLINE_SIZE		100

#define MAX_RESPONSE_SIZE		1024

#define SHOW_RX_DISPLAY_DEFAULT_INTERVAL	1 // sec

#define CMD_RET_SUCCESS		0
#define CMD_RET_FAILURE		1
#define CMD_RET_EXIT_PROGRAM	2
#define CMD_RET_RESPONSE_TIMEOUT 3
#define CMD_RET_EMPTY_INPUT	-1

typedef struct nrc_cmd {
	const char *name;
	int(*handler)(struct nrc_cmd *t, int argc, char *argv[]);
	const char *desc;
	const char *usage;
	const char *key_list;
	int flag;
} cmd_tbl_t;

enum cmd_list_type {
	MAIN_CMD,
	SYS_CMD,
	SHOW_SUB_CMD,
	SHOW_STATS_SUB_CMD,
	SHOW_MAC_SUB_CMD,
	SHOW_MAC_TX_SUB_CMD,
	SHOW_MAC_RX_SUB_CMD,
	NRF_SUB_CMD,
	SET_SUB_CMD,
	TEST_SUB_CMD,
	GPIO_SUB_CMD,
 	MAX_CMD_LIST
};

enum umac_info_type {
	AP_UMAC_INFO,
	STA_UMAC_INFO,
	STA_UMAC_INFO_MINI
};

cmd_tbl_t * get_cmd_list(enum cmd_list_type type, int *list_size, int *list_depth);
int run_shell_cmd(cmd_tbl_t *t, int argc, char *argv[], const char *param_str, char *resp_buf, int resp_buf_len );
int run_driver_cmd(cmd_tbl_t *t, int argc, char *argv[], const char *param_str, char *resp_buf, int resp_buf_len );

#endif /* _CLI_CMD_H_ */
