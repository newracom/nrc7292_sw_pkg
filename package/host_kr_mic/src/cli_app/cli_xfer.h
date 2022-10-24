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

#ifndef _CLI_XFER_H_
#define _CLI_XFER_H_

#include <stdint.h>

typedef struct _xfer_umac_apinfo{
	uint8_t	    bssid[6];
	uint8_t 	ssid[32];			// key
	uint8_t	    ssid_len;			// key_length
	uint8_t	    security;
	uint16_t	beacon_interval;
	uint16_t	short_beacon_interval;
	uint8_t 	assoc_s1g_channel;
	uint32_t 	comp_ssid;
	uint32_t	change_seq_num;
	// Peer Info
	uint16_t	s1g_long_support           : 1;
	uint16_t	pv1_frame_support          : 1;
	uint16_t	nontim_support             : 1;
	uint16_t	twtoption_activated        : 1;
	uint16_t	ampdu_support              : 1;
	uint16_t	ndp_pspoll_support         : 1;
	uint16_t	traveling_pilot_support    : 1;

	uint16_t	shortgi_1mhz_support       : 1;
	uint16_t	shortgi_2mhz_support       : 1;
	uint16_t	shortgi_4mhz_support       : 1;
	uint16_t	maximum_mpdu_length        : 1; //is this really a boolean?
	uint8_t	    maximum_ampdu_length_exp   : 3;
	uint8_t	    minimum_mpdu_start_spacing : 5;
	uint8_t	    rx_s1gmcs_map;
	uint8_t	    color;
} xfer_umac_apinfo;

typedef struct _xfer_umac_stainfo {
	uint8_t	    maddr[6];
	uint16_t    aid;			// key
	uint32_t	listen_interval;
	// Peer Info
	uint16_t	s1g_long_support           : 1;
	uint16_t	pv1_frame_support          : 1;
	uint16_t	nontim_support             : 1;
	uint16_t	twtoption_activated        : 1;
	uint16_t	ampdu_support              : 1;
	uint16_t	ndp_pspoll_support         : 1;
	uint16_t	traveling_pilot_support    : 1;
	uint16_t	shortgi_1mhz_support       : 1;
	uint16_t	shortgi_2mhz_support       : 1;
	uint16_t	shortgi_4mhz_support       : 1;
	uint8_t	    maximum_mpdu_length;
	uint8_t	    maximum_ampdu_length_exp   : 3;
	uint8_t	    minimum_mpdu_start_spacing : 5;
	uint8_t 	rx_s1gmcs_map;
} xfer_umac_stainfo;

typedef struct _xfer_umac_stainfo_mini {
	uint8_t	    maddr[6];
	uint16_t    aid;			// key
	uint8_t	    state;
} xfer_umac_stainfo_mini;

typedef enum {
	AC_BK = 0,
	AC_BE,
	AC_VI,
	AC_VO,
	AC_MAX
} AC_TYPE;

typedef struct xfer_maxagg_info {
	uint8_t 	is_ap : 1;
	uint8_t 	state : 1;
	uint8_t 	ba_session : 1;
	uint8_t 	ac : 2;
	uint8_t 	reserved : 3;
	uint8_t 	max_agg_num;
	uint16_t	agg_num_size;
	uint16_t	aid;
} xfer_maxagg_info;

#endif /* _CLI_XFER__ */
