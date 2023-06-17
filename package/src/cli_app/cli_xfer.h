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
	uint16_t	s1g_1mctrlresppreamble_support       : 1;
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
	uint16_t	s1g_1mctrlresppreamble_support       : 1;
	uint8_t	    maximum_mpdu_length;
	uint8_t	    maximum_ampdu_length_exp   : 3;
	uint8_t	    minimum_mpdu_start_spacing : 5;
	uint8_t 	rx_s1gmcs_map;
} xfer_umac_stainfo;

typedef struct _xfer_umac_stainfo_mini {
	uint8_t	    maddr[6];
	uint16_t    aid;			// key
	uint8_t	    state;
	uint8_t	    sgi:4;
	uint8_t	    bw:4;
	uint8_t	    tx_mcs:4;
	uint8_t	    rx_mcs:4;
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

typedef enum {
	UNKNOWN = 0,
	SYSCONFIG_FORMAT_1,
	SYSCONFIG_FORMAT_2,
} XFER_SYSCONFIG_FORMAT;

typedef struct xfer_header {
	uint8_t 	more : 1;  // Indicates whether more data is available (0 for no more data, 1 for more data)
	uint8_t 	sysconfig_format : 3;    // Specifies the device model for sysconfig: SYSCONFIG_TYPE1(1) or SYSCONFIG_TYPE2(2)
	uint8_t 	reserved1 : 4;  // Reserved for future use
	uint8_t 	reserved2;  // Reserved for future use
	uint16_t	dataSize;  // Specifies the size(in bytes) of the data being transferred
} xfer_header_t;

#define XFER_SYSCONFIG_SECTOR_SIZE               4096
#define XFER_SYSCONFIG_PRE_USER_FACTORY_SIZE      256
#define XFER_SYSCONFIG_USER_FACTORY_SIZE          512
#define XFER_SHELL_RESPONSE_MAX_SIZE 452
#define XFER_HEADER_SIZE sizeof(xfer_header_t)
#define XFER_MAX_DATA_SIZE XFER_SHELL_RESPONSE_MAX_SIZE - XFER_HEADER_SIZE

//1 Byte
typedef struct {
	uint32_t control;
	uint32_t value;
}xfer_sys_config_pllldo_t;

//1 Byte
typedef struct {
	uint8_t cfo_cal   : 1; //1: pass, 0: fail
	uint8_t da_cal    : 1;
	uint8_t txpwr_cal : 1;
	uint8_t rssi_cal  : 1;
	uint8_t reserved  : 2;
	uint8_t tx_test   : 1;
	uint8_t rx_test   : 1;
} xfer_sys_config_trx_pass_fail_t;

//4 Bytes
typedef struct {
	uint32_t type     : 16;
	uint32_t reserved : 16;
} xfer_sys_config_chip_type_t;

//4 Bytes
typedef struct {
	uint32_t type     : 16;
	uint32_t reserved : 16;
} xfer_sys_config_module_type_t;

// 4 Bytes
typedef struct {
	union {
		struct {
			uint32_t txpwr_boosting_valid :  1;
			uint32_t fem_polarity_valid   :  1;
			uint32_t external_pa_exists   :  1;
			uint32_t max_txgain_valid     :  1;
			uint32_t max_txpwr_valid      :  1;
			uint32_t reserved             : 27;
		};
		uint32_t word;
	};
} xfer_sys_config_module_feature_t;

// 1 Byte
typedef struct {
	uint8_t tmx_gmrc : 2;
	uint8_t reserved : 6;
} xfer_sys_config_txpwr_boosting_t;

// 4 Bytes
typedef struct {
	union {
		struct {
			uint32_t pa_en_valid         : 1;
			uint32_t pa_en_always_on     : 1;
			uint32_t pa_en_pin           : 6;

			uint32_t ant_sel_valid       : 1; //TRX Switch
			uint32_t ant_sel_reserved    : 1;
			uint32_t ant_sel_pin         : 6; //TRX Switch

			uint32_t power_down_valid    : 1;
			uint32_t power_down_data     : 1;
			uint32_t power_down_pin      : 6;

			uint32_t reserved         : 8;
		};
		uint32_t word;
	};
} xfer_sys_config_gpio_index_map_t;

//768 Bytes (256 + 512)
typedef struct {
	uint32_t                        version; /* sys_config structure version*/
	char                            mac_addr0[6]; /*mac address for interface 0*/
	char                            mac_addr1[6]; /*mac address for interface 1*/
	uint8_t                         cal_use; /*enable/disable the usage of calibration data*/
	xfer_sys_config_trx_pass_fail_t   trx_pass_fail;
	uint16_t                        hw_version; /* HW Version */
	uint32_t                        reserved1[2]; /* Previously rf_pllldo12_tr struct */
	xfer_sys_config_chip_type_t       chip_type; /* NRC7393/7394 module name - TBD */
	xfer_sys_config_module_type_t     module_type;
	xfer_sys_config_module_feature_t  module_feature;
	xfer_sys_config_txpwr_boosting_t  txpwr_boosting;
	uint8_t                         max_txgain; /* NRC7393/7394 max txgain */
	uint8_t                         max_txpwr; /* NRC7393/7394 max txpwr */
	uint8_t                         fem_polarity;
	xfer_sys_config_gpio_index_map_t  gpio_index_map;
	char                            serial_number[32];
	uint8_t                         reserved2[176];
	char                            user_factory[512];
} xfer_sys_config_t;

#endif /* _CLI_XFER__ */
