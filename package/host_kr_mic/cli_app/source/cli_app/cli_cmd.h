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

#define UMAC_INFO_SIMPLE_DISPLAY		1

typedef struct nrc_cmd {
	const char *name;
	int(*handler)(struct nrc_cmd *t, int argc, char *argv[]);
	const char *desc;
	const char *usage;
	const char *key_list;
	const char subListExist;
} cmd_tbl_t;

enum cmd_list_type {
	MAIN_CMD,
	SYS_CMD,
	SHOW_SUB_CMD,
	SHOW_STATS_SUB_CMD,
	SHWO_MAC_SUB_CMD,
	SHOW_MAC_TX_SUB_CMD,
	SHOW_MAC_RX_SUB_CMD,
	NRF_SUB_CMD,
	SET_SUB_CMD,
	TEST_SUB_CMD,
	GPIO_SUB_CMD,
	GPRF_SUB_CMD,
	MAX_CMD_LIST
};

enum umac_info_type {
	AP_UMAC_INFO,
	STA_UMAC_INFO
};

typedef struct {
	char	bssid[18];
	char	ssid[32];			// key
	int	ssid_len;			// key_length
	int	security;
	int	beacon_interval;
	int	short_beacon_interval;
	int	assoc_s1g_channel;
	char	comp_ssid[10];
	int	change_seq_num;
	// Peer Info
	char	s1g_long_support;
	char	pv1_frame_support;
	char	nontim_support;
	char	twtoption_activated;
	char	ampdu_support;
	char	ndp_pspoll_support;
	char	traveling_pilot_support;

	char	shortgi_1mhz_support;
	char	shortgi_2mhz_support;
	char	shortgi_4mhz_support;
	char	maximum_mpdu_length;
	char	maximum_ampdu_length_exp;
	char	minimum_mpdu_start_spacing;
	char	rx_s1gmcs_map[6];
	int	color;
}umac_apinfo;

typedef struct _umac_stainfo {
	char	maddr[18];
	int	aid;			// key
	int	listen_interval;
	// Peer Info
	char	s1g_long_support;
	char	pv1_frame_support;
	char	nontim_support;
	char	twtoption_activated;
	char	ampdu_support;
	char	ndp_pspoll_support;
	char	traveling_pilot_support;
	char	shortgi_1mhz_support;
	char	shortgi_2mhz_support;
	char	shortgi_4mhz_support;
	char	maximum_mpdu_length;
	char	maximum_ampdu_length_exp;
	char	minimum_mpdu_start_spacing;
	int	rx_s1gmcs_map[6];
}umac_stainfo;

cmd_tbl_t * get_cmd_list(enum cmd_list_type type, int *list_size, int *list_depth);

#endif /* _CLI_CMD_H_ */
