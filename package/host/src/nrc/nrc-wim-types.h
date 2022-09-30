/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _NRC_WIM_TYPES_H_
#define _NRC_WIM_TYPES_H_

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef NR_NRC_VIF
#define NR_NRC_VIF 2
#endif

#define MAX_FRAME_PKT_TLV_SIZE      (256)
#define MAX_WIM_PKT_TLV_SIZE        (1500)
#define WIM_HEAD_SIZE               (sizeof(struct hif_hdr) \
					+ sizeof(struct wim_hdr))
#define WIM_MAX_DATA_SIZE           (512)
#define WIM_MAX_TLV_SIZE            (1024)
#define WIM_MAX_SIZE                (WIM_HEAD_SIZE \
					+ WIM_MAX_DATA_SIZE + WIM_MAX_TLV_SIZE)
#define WIM_MAX_TLV_SCAN_IE         (512)

#ifndef __packed
#define  __packed __attribute__((__packed__))
#endif

enum HIF_TYPE {
	HIF_TYPE_FRAME  = 0,
	HIF_TYPE_WIM,
	HIF_TYPE_NANOPB,
	HIF_TYPE_UART_CMD,
	HIF_TYPE_SSP_READYRX,
	HIF_TYPE_SSP_PING,
	HIF_TYPE_SSP_PONG,
	HIF_TYPE_SSP_SKIP,
	HIF_TYPE_LOG,
	HIF_TYPE_LOOPBACK,
	HIF_TYPE_DUMP,
	HIF_TYPE_MAX
};

enum HIF_SUBTYPE {
	HIF_FRAME_SUB_DATA_BE = 0,
	HIF_FRAME_SUB_DATA_BK,
	HIF_FRAME_SUB_DATA_VO,
	HIF_FRAME_SUB_DATA_VI,
	HIF_FRAME_SUB_MGMT,
	HIF_FRAME_SUB_BEACON,
	HIF_FRAME_SUB_PROBE_RESP,
	HIF_FRAME_SUB_802_3,
	HIF_FRAME_SUB_CTRL,
	HIF_FRAME_SUB_RESPONSE = 31,
	HIF_WIM_SUB_REQUEST = 0,
	HIF_WIM_SUB_RESPONSE,
	HIF_WIM_SUB_EVENT,
	HIF_WIM_SUB_MAX,
	HIF_NANOPB_SUB_REQUEST = 0,
	HIF_NANOPB_SUB_RESPONSE,
	HIF_NANOPB_SUB_EVENT,
	HIF_NANOPB_SUB_EAPOL,
	HIF_NANOPB_SUB_L2PACKET,
	HIF_NANOPB_SUB_ACTION,
	HIF_NANOPB_SUB_DATA,
	HIF_NANOPB_SUB_MAX,
	HIF_LOG_SUB_VB		= 0,	/*TL_VB*/
	HIF_LOG_SUB_INFO	= 1,	/*TL_INFO*/
	HIF_LOG_SUB_ERROR	= 2,	/*TL_ERR*/
	HIF_LOG_SUB_ASSERT	= 3,	/*TL_ASSERT*/
	HIF_LOG_SUB_NONE	= 4,	/*TL_NONE*/
	HIF_LOG_SUB_MAX				/*TL_MAX*/

};

enum WIM_CMD_GRP {
	WIM_CMD_GRP_SYSTEM,
	WIM_CMD_GRP_MGMT,
	WIM_CMD_GRP_SCAN,
	WIM_CMD_GRP_PS,
	WIM_CMD_GRP_P2P,
	WIM_CMD_GRP_S1G,
	WIM_CMD_GRP_SEC,
	WIM_CMD_GRP_STA,
	WIM_CMD_GRP_MAX = 8
};

enum WIM_CMD_ID {
	WIM_CMD_INIT,
	WIM_CMD_START,
	WIM_CMD_STOP,
	WIM_CMD_SCAN_START,
	WIM_CMD_SCAN_STOP,
	WIM_CMD_SET_KEY,
	WIM_CMD_DISABLE_KEY,
	WIM_CMD_STA_CMD,
	WIM_CMD_SET,
	/* DON'T ADD A NEW CMD BEFORE WIM_CMD_REQ_FW !! */
	WIM_CMD_REQ_FW,
	WIM_CMD_FW_RELOAD,
	WIM_CMD_AMPDU_ACTION,
	WIM_CMD_SHELL,
	WIM_CMD_SLEEP,
	WIM_CMD_MIC_SCAN,
	WIM_CMD_KEEP_ALIVE,
	WIM_CMD_SET_IE,
	WIM_CMD_SET_SAE,
	WIM_CMD_PM,
	WIM_CMD_SHELL_RAW,
	WIM_CMD_MAX,
};

enum WIM_EVENT_ID {
	WIM_EVENT_SCAN_COMPLETED,
	WIM_EVENT_READY,
	WIM_EVENT_CREDIT_REPORT,
	WIM_EVENT_PS_READY,
	WIM_EVENT_PS_WAKEUP,
	WIM_EVENT_KEEP_ALIVE,
	WIM_EVENT_REQ_DEAUTH,
	WIM_EVENT_CSA,
	WIM_EVENT_CH_SWITCH,
	WIM_EVENT_MAX,
};

enum WIM_TLV_ID {
	WIM_TLV_SSID = 0,
	WIM_TLV_BSSID = 1,
	WIM_TLV_MACADDR = 2,
	WIM_TLV_AID = 3,
	WIM_TLV_STA_TYPE = 4,
	WIM_TLV_SCAN_PARAM = 5,
	WIM_TLV_KEY_PARAM = 6,
	WIM_TLV_STA_PARAM = 7,
	WIM_TLV_CHANNEL = 8,
	WIM_TLV_HT_MODE = 9,
	WIM_TLV_BCN_INTV = 10,
	WIM_TLV_SCAN_BAND_IE = 11,
	WIM_TLV_SCAN_PROBE_REQ_IE = 12,
	WIM_TLV_SCAN_COMMON_IE = 13,
	WIM_TLV_BEACON = 14,
	WIM_TLV_BEACON_ENABLE = 15,
	WIM_TLV_PROBE_RESP = 16,
	WIM_TLV_FIRMWARE = 17,
	WIM_TLV_PS_ENABLE = 18,
	WIM_TLV_READY = 19,
	WIM_TLV_TXQ_PARAM = 20,
	WIM_TLV_AC_CREDIT_REPORT = 21,
	WIM_TLV_MCS = 22,
	WIM_TLV_CHANNEL_WIDTH = 23,
	WIM_TLV_BASIC_RATE = 24,
	WIM_TLV_CH_BW_MCS = 25,
	WIM_TLV_CH_BW = 26,
	WIM_TLV_MCS_NSS = 27,
	WIM_TLV_BD = 28,
	WIM_TLV_TID,
	WIM_TLV_AMPDU_MODE, //30
	WIM_TLV_EXTRA_TX_INFO,
	WIM_TLV_SHORT_BCN_INTV,
	/* CB4861 */
	WIM_TLV_CAP,
	WIM_TLV_SUPPORTED_RATES,
	WIM_TLV_HT_CAP,
	WIM_TLV_PHY_MODE,
	WIM_TLV_ERP_PARAM,
	WIM_TLV_TIM_PARAM,
	WIM_TLV_SHELL_CMD,
	WIM_TLV_SHELL_RESP, //40
	WIM_TLV_MACADDR_PARAM,
	WIM_TLV_DTIM_PERIOD,
	WIM_TLV_P2P_OPPPS,
	WIM_TLV_P2P_NOA,
	WIM_TLV_SGI,
	WIM_TLV_S1G_STA_TYPE,
	WIM_TLV_AMPDU_SUPPORT,
	WIM_TLV_AMSDU_SUPPORT,
	WIM_TLV_S1G_PV1,
	WIM_TLV_COUNTRY_CODE, //50
	WIM_TLV_CH_TABLE,
	WIM_TLV_1MHZ_CTRL_RSP,
	WIM_TLV_COLOR_IND,
	WIM_TLV_S1G_TIM_MODE,
	WIM_TLV_AP_ONLY,
	WIM_TLV_SLEEP_DURATION,
	WIM_TLV_CCA_1M,
	WIM_TLV_S1G_CHANNEL,
	WIM_TLV_RTS_THREASHOLD,
	WIM_TLV_FRAME_INJECTION, //60
	WIM_TLV_IE_PARAM,
	WIM_TLV_NDP_PREQ,
	WIM_TLV_SAE_PARAM,
	WIM_TLV_DRV_INFO,
	WIM_TLV_NDP_ACK_1M, //65
	WIM_TLV_SET_TXPOWER,
	WIM_TLV_LEGACY_ACK,
	WIM_TLV_MAX,
};

enum WIM_STA_TYPE {
	WIM_STA_TYPE_NONE = 0,
	WIM_STA_TYPE_STA,
	WIM_STA_TYPE_AP,
	WIM_STA_TYPE_P2P_GO,
	WIM_STA_TYPE_P2P_GC,
	WIM_STA_TYPE_P2P_DEVICE,
	WIM_STA_TYPE_MONITOR,
	WIM_STA_TYPE_MESH_POINT
};

enum WIM_S1G_STA_TYPE {
	WIM_S1G_STA_TYPE_BOTH,
	WIM_S1G_STA_TYPE_SENSOR,
	WIM_S1G_STA_TYPE_NON_SENSOR,
};

enum WIM_AMPDU_ACTION {
	WIM_AMPDU_TX_START = 0,
	WIM_AMPDU_TX_STOP,
	WIM_AMPDU_RX_START,
	WIM_AMPDU_RX_STOP,
	WIM_AMPDU_TX_OPERATIONAL,
};

enum WIM_S1G_TIM_MODE {
	WIM_TIM_OLB = 0,
	WIM_TIM_SINGLE_AID,
	WIM_TIM_BLOCK_BMP
};

struct wim_cmd_map {
	uint16_t grp;
	uint16_t len;
};

struct wim_tlv_map {
	uint16_t type;
	uint16_t len;
};

struct hif_hdr {
	uint8_t type;
	uint8_t subtype;
	uint8_t flags;
	int8_t vifindex;
	uint16_t len;
	uint16_t tlv_len;
} __packed;

struct hif_lb_hdr {
	uint8_t type;
	uint8_t subtype;
	uint16_t count;
	uint16_t len;
	uint16_t index;
} __packed;

#define NRC_FRAME_CIPHER_NONE (0)
#define NRC_FRAME_CIPHER_CCMP (1)

struct frame_hdr {
	union {
		struct rx_flags {
			uint8_t error_mic:1;
			uint8_t iv_stripped:1;
			uint8_t snr:6;
			int8_t rssi;
		} rx;
		struct tx_flags {
			uint8_t ac;
			uint8_t reserved;
		} tx;
	} flags;
	union {
		struct rx_info {
			uint16_t frequency;
		} rx;

		struct tx_info {
			uint8_t cipher;
			uint8_t tlv_len;
		} tx;
	} info;
} __packed;

enum {
	WIM_BW_1M = 0,
	WIM_BW_2M,
	WIM_BW_4M,
	WIM_BW_MAX,

	WIM_BW_8M = 3,
	WIM_BW_16M,
};

enum {
	WIM_S1G = 0,
	WIM_S1G_DUP_1M,
	WIM_S1G_DUP_2M
};

struct sigS1g {
	union {
		struct s1g1M {
			/* sig bit pattern LSB -> MSB */
			uint32_t nsts			: 2;
			/* 0=1 space time stream, 1=2 space time stream, etc. */
			uint32_t short_gi		: 1;
			/* 0 for no use of short guard interval, otherwise 1 */
			uint32_t coding			: 2;
			/* x0=BCC, x1=LDPC */
			uint32_t stbc			: 1;
			/* 0=no spatial streams has STBC,
			 * 1=all spatial streams has STBC
			 */
			uint32_t reserved2		: 1;
			uint32_t mcs			: 4;
			uint32_t aggregation	: 1;
			/* 1 for aggregation */
			uint32_t length			: 9;
			/* PPDU length in symbols in case of aggregation */
			/* otherwise, byte length */
			uint32_t response_ind	: 2;
			/* 0=no rsp, 1=NDP rsp, 2=normal rsp, 3=long rsp */
			uint32_t smoothing		: 1;
			/* 1 for recommending channel smoothing */
			uint32_t doppler		: 1;
			/* 0=regular pilot tone, 1=traveling pilots */
			uint32_t ndp_ind		: 1;
			/* 1 for NDP */
			uint32_t crc			: 4;
			/* for TX use (CRC calulation for S1G SIGA fields) */
			uint32_t reserved1		: 2;

			uint32_t magic_code_3	: 8;
			uint32_t magic_code_2	: 8;
			uint32_t magic_code_1	: 8;
			uint32_t magic_code_0	: 8;
		} s1g1M;
		struct s1gShort {
			/* sig bit pattern LSB -> MSB */
			uint32_t reserved2		: 1;
			uint32_t stbc			: 1;
			uint32_t uplink_ind		: 1;
			/* set to the value of TXVECTOR
			 * parameter UPLINK_INDICATION
			 */
			uint32_t bandwidth		: 2;
			/* 0:2M, 1:4M, 2:8M, 3:16M */
			uint32_t nsts			: 2;
			uint32_t id				: 9;
			uint32_t short_gi		: 1;
			uint32_t coding			: 2;
			uint32_t mcs			: 4;
			uint32_t smoothing		: 1;
			uint32_t reserved1		: 8;
			uint32_t aggregation	: 1;
			uint32_t length			: 9;
			uint32_t response_ind	: 2;
			uint32_t doppler		: 1;
			uint32_t ndp_ind		: 1;
			uint32_t crc			: 4;
			/* for TX use */
			uint32_t reserved3		: 14;
		} s1gShort;
		struct s1gLong {
			/* sig bit pattern LSB -> MSB */
			uint32_t mu_su			: 1;
			uint32_t stbc			: 1;
			uint32_t uplink_ind		: 1;
			uint32_t bandwidth		: 2;
			/* 0:2M, 1:4M, 2:8M, 3:16M */
			uint32_t nsts			: 2;
			uint32_t id				: 9;
			uint32_t short_gi		: 1;
			uint32_t coding			: 2;
			uint32_t mcs			: 4;
			uint32_t smoothing		: 1;
			uint32_t reserved1		: 8;
			uint32_t aggregation	: 1;
			uint32_t length			: 9;
			uint32_t response_ind	: 2;
			uint32_t reserved3		: 1;
			uint32_t doppler		: 1;
			uint32_t crc			: 4;
			/* for TX use */
			uint32_t reserved2		: 14;
		} s1gLong;
	} flags;
} __packed;

struct rxInfo {
	uint32_t mpdu_length			: 14;
	uint32_t center_freq				: 18;
	uint32_t format					: 2;
	uint32_t preamble_type			: 1;
	uint32_t bandwidth				: 2;
	uint32_t scrambler_or_crc		: 7;
	uint32_t rcpi					: 8;
	uint32_t obss_frame				: 1;
	uint32_t ndp_ind				: 1;
	uint32_t long_2m_ind			: 1;
	uint32_t aggregation			: 1;
	uint32_t error_mic				: 1;
	uint32_t error_key				: 1;
	uint32_t protection				: 1;
	uint32_t error_length			: 1;
	uint32_t error_match			: 1;
	uint32_t error_crc				: 1;
	uint32_t okay					: 1;
	uint32_t valid					: 1;
	uint32_t timestamp				: 32;
	uint32_t fcs					: 32;
} __packed;

struct wim_hdr {
	union {
		uint16_t cmd;
		uint16_t resp;
		uint16_t event;
	};
	uint8_t seq_no;
	uint8_t n_tlvs;
} __packed;

#define NRC_TLV_HDR_LEN (4)
struct nrc_tlv_hdr {
	uint16_t type;
	uint16_t len;
};

struct nrc_tlv_item {
	uint16_t type;
	uint16_t len;
	uint8_t value[0];
};

struct nrc_tlv {
	struct nrc_tlv_hdr tlv_hdr;
	union {
		struct wim_scan_param *scan_param;
		struct wim_channel_1m_param *chan_1m_param;
		struct wim_tx_queue_param *queue_param;
		struct wim_key_param *key_param;
		struct wim_channel_param *channel_param;
		struct wim_sta_param *sta_param;
		struct wim_ready_param *ready_param;
		struct wim_credit_report_param *credit_report_param;
		struct wim_channel_width_param *channel_width_param;
		struct wim_basic_rate_param *basic_rate_param;
		struct wim_ch_bw_mcs_param *ch_bw_mcs_param;
		struct wim_ch_bw_param *ch_bw_param;
		struct wim_mcs_nss_param *mcs_nss_param;
		struct frame_tx_info_param *tx_info_param;
		struct wim_tim_param *tim_param;
		struct wim_sleep_duration_param *sleep_duration_param;
		struct wim_pm_param *pm_param;
		struct wim_s1g_channel_param *s1g_chan_param;
		struct wim_bd_param *bd_param;
		uint8_t *bytes_param;
		bool *bool_param;
		uint32_t *u32_param;
		uint16_t *u16_param;
		uint8_t *u8_param;
		void *pvoid;
		uint8_t param[0];
	};
} __packed;

enum WIM_SCAN_FLAG {
	WIM_SCAN_FLAG_NOCCK = BIT(1),
};

enum WIM_AID_ADDR_FLAG {
	AID_ADDR_FLAG_HAS_MACADDR = BIT(0),
	AID_ADDR_FLAG_HAS_AID     = BIT(1),
};

enum WIM_CHANNEL_PARAM_TYPE {
	CHAN_NO_HT,
	CHAN_HT20,
	CHAN_HT40MINUS,
	CHAN_HT40PLUS
};

enum WIM_CHANNEL_PARAM_WIDTH {
	CH_WIDTH_20_NOHT,
	CH_WIDTH_20,
	CH_WIDTH_40,
	CH_WIDTH_80,
	CH_WIDTH_80P80,
	CH_WIDTH_160,
	CH_WIDTH_5,
	CH_WIDTH_10,
	CH_WIDTH_1,
	CH_WIDTH_2,
	CH_WIDTH_4,
	CH_WIDTH_8,
	CH_WIDTH_16,
};

struct wim_channel_param {
	uint16_t channel;
	uint8_t type;
	uint8_t width;
} __packed;


#define WIM_MAX_BD_DATA_LEN		(540)
struct wim_bd_param {
	uint16_t type;
	uint16_t length;
	uint16_t checksum;
	uint16_t hw_version;
	uint8_t value[WIM_MAX_BD_DATA_LEN];
} __packed;

#define WIM_MAX_SCAN_SSID       (2)
#define WIM_MAX_SCAN_BSSID      (2)
#define WIM_MAX_SCAN_CHANNEL    (55)
#ifndef IEEE80211_MAX_SSID_LEN
#define IEEE80211_MAX_SSID_LEN  (32)
#endif

struct wim_scan_ssid {
	uint8_t ssid[IEEE80211_MAX_SSID_LEN];
	uint8_t ssid_len;
}  __packed;

struct wim_scan_bssid {
	uint8_t bssid[6];
} __packed;

#define WIM_DECLARE(x) \
	struct x {\
		struct nrc_tlv_hdr h;\
		struct x##_param v;\
	} __packed

#define WIM_SYSTEM_VER_11N      (1)
#define WIM_SYSTEM_VER_11AH_1ST (2)
#define WIM_SYSTEM_VER_11AH_2ND (3)
#define WIM_SYSTEM_VER_11AH_3RD (4)

enum wim_system_cap {
	WIM_SYSTEM_CAP_USF = BIT(0),
	WIM_SYSTEM_CAP_HWSEC = BIT(1),
	WIM_SYSTEM_CAP_HWSEC_OFFL = BIT(2),
	WIM_SYSTEM_CAP_CHANNEL_2G = BIT(3),
	WIM_SYSTEM_CAP_CHANNEL_5G = BIT(4),
	WIM_SYSTEM_CAP_MULTI_VIF = BIT(5),
};

struct wim_vif_cap_param {
	uint64_t cap;
} __packed;

struct wim_cap_param {
	uint64_t cap;
	uint16_t listen_interval;
	uint16_t bss_max_idle;
	uint16_t max_vif;
	struct wim_vif_cap_param vif_caps[];
} __packed;

struct wim_ready_param {
	uint32_t version;
	uint32_t tx_head_size;
	uint32_t rx_head_size;
	uint32_t payload_align;
	uint32_t buffer_size;
	uint8_t macaddr[NR_NRC_VIF][ETH_ALEN];
	bool has_macaddr[NR_NRC_VIF];
	uint16_t hw_version;
	struct wim_cap_param cap;
} __packed;
WIM_DECLARE(wim_ready);

struct wim_credit_report_param {
	uint8_t change_index;
	uint8_t reserved0;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t ac[12];
} __packed;
WIM_DECLARE(wim_credit_report);

struct wim_channel_width_param {
	uint8_t chan_width;
	uint8_t prim_loc;
	uint8_t reserved[2];
} __packed;
WIM_DECLARE(wim_channel_width);

struct wim_basic_rate_param {
	uint32_t rate;
} __packed;
WIM_DECLARE(wim_basic_rate);

struct wim_ch_bw_mcs_param {
	uint8_t prim_ch_num;
	uint8_t second_ch_offset;
	uint8_t sta_channel_width;
	uint8_t reserved;
	uint32_t mcs[4];
} __packed;
WIM_DECLARE(wim_ch_bw_mcs);

struct wim_ch_bw_param {
	uint8_t prim_ch_width;
	uint8_t oper_ch_width;
	uint8_t loc_1m_prim_ch;
	uint8_t prim_ch_num;
	uint8_t center_freq;
	uint8_t mcs_10_permit;
	uint8_t mcs_min;
	uint8_t mcs_max;
	uint8_t nss;
	uint8_t reserved[3];
} __packed;
WIM_DECLARE(wim_ch_bw);

struct wim_mcs_nss_param {
	uint8_t mcs_min;
	uint8_t mcs_max;
	uint8_t nss;
	uint8_t reserved;
} __packed;
WIM_DECLARE(wim_mcs_nss);

struct frame_tx_info_param {
	uint16_t aid;
	uint8_t use_rts:1;
	uint8_t use_11b_protection:1;
	uint8_t short_preamble:1;
	uint8_t ampdu:1;
	uint8_t after_dtim:1;
	uint8_t no_ack:1;
	uint8_t eosp:1;
	uint8_t inject:1;
	uint16_t reserved0:8;
	uint32_t reserved1;
} __packed;
WIM_DECLARE(frame_tx_info);

struct wim_sleep_duration_param {
	uint32_t sleep_ms;
} __packed;
WIM_DECLARE(wim_sleep_duration);

/**
 * struct wowlan_pattern - packet pattern
 * @offset: packet offset (in bytes)
 * @mask_len: length of mask (in bytes)
 * @pattern_len: length of pattern (in bytes)
 * @mask: bitmask where to match pattern and where to ignore bytes,
 *  one bit per byte, in same format as nl80211
 * @pattern: bytes to match where bitmask is 1
 */
struct wowlan_pattern {
	uint16_t offset:6;
	uint16_t mask_len:4;
	uint16_t pattern_len:6;
	uint8_t mask[7];
	uint8_t pattern[56];
} __packed;

struct wim_pm_param {
	uint8_t ps_mode;
	uint8_t ps_enable;
	uint16_t ps_wakeup_pin;
	uint64_t ps_duration;
	uint8_t wowlan_wakeup_host_pin;
	uint8_t wowlan_enable_any;
	uint8_t wowlan_enable_magicpacket;
	uint8_t wowlan_enable_disconnect;
	uint8_t wowlan_n_patterns;
	struct wowlan_pattern wp[2];
} __packed;
WIM_DECLARE(wim_pm);

struct wim_drv_info_param {
	uint32_t boot_mode			:1;
	uint32_t cqm_off			:1;
	uint32_t bitmap_encoding	:1;
	uint32_t reverse_scrambler	:1;
	uint32_t reserved			:28;
} __packed;
WIM_DECLARE(wim_drv_info);

#define WIM_SCAN_PARAM_FLAG_NO_CCK      (BIT(0))
#define WIM_SCAN_PARAM_FLAG_HT			(BIT(1))
#define WIM_SCAN_PARAM_FLAG_WMM			(BIT(2))
#define WIM_SCAN_PARAM_FLAG_DS			(BIT(3))

struct wim_channel_1m_param {
	int channel_start;
	int channel_end;
	uint32_t cca_bitmap;
} __packed;
WIM_DECLARE(wim_channel_1m);

struct wim_scan_channel {
	uint16_t freq;
	struct wim_channel_width_param width;
};

struct wim_scan_param {
	uint8_t mac_addr[6];
	uint8_t mac_addr_mask[6];
	uint32_t rate;
	uint32_t scan_flag;
	uint8_t n_ssids;
	uint8_t n_bssids;
	uint8_t n_channels;
	struct wim_scan_ssid    ssid[WIM_MAX_SCAN_SSID];
	struct wim_scan_bssid   bssid[WIM_MAX_SCAN_BSSID];
	uint16_t                channel[WIM_MAX_SCAN_CHANNEL];
	struct wim_scan_channel s1g_channel[WIM_MAX_SCAN_CHANNEL];
} __packed;

enum wim_cipher_type {
	WIM_CIPHER_TYPE_INVALID = -1,
	WIM_CIPHER_TYPE_WEP40 = 0,
	WIM_CIPHER_TYPE_WEP104 = 1,
	WIM_CIPHER_TYPE_TKIP = 2,
	WIM_CIPHER_TYPE_CCMP = 3,
	WIM_CIPHER_TYPE_CCMP_256 = 3,
	WIM_CIPHER_TYPE_WAPI = 4,
	WIM_CIPHER_TYPE_NONE = 5,
	WIM_CIPHER_TYPE_BIP = 6,
};

#define WIM_KEY_MAX_LEN         (32)
#define WIM_KEY_MAX_INDEX       (4)

#define WIM_KEY_FLAG_PAIRWISE   BIT(0)
#define WIM_KEY_FLAG_GROUP      BIT(1)
#define WIM_KEY_FLAG_IGROUP     BIT(2)

struct wim_key_param {
	uint8_t cipher_type;
	uint8_t key_index;
	uint8_t mac_addr[6];
	uint16_t aid;
	uint32_t key_flags;
	uint32_t key_len;
	uint8_t key[WIM_KEY_MAX_LEN];
};

enum wim_sta_cmd {
	WIM_STA_CMD_ADD    = 0,
	WIM_STA_CMD_REMOVE,
	WIM_STA_CMD_NOTIFY,
	WIM_STA_CMD_STATE_NOTEXIST,
	WIM_STA_CMD_STATE_NONE,
	WIM_STA_CMD_STATE_AUTH,
	WIM_STA_CMD_STATE_ASSOC,
	WIM_STA_CMD_STATE_AUTHORIZED,
};

struct wim_sta_param {
	uint8_t cmd;
	uint8_t addr[6];
	uint8_t sleep; /*0: awake, 1: sleep*/
	uint16_t aid;
	uint32_t flags;
};

#define INFO_ELEMENT_MAX_LENGTH	255
struct wim_set_ie_param {
	uint16_t eid;
	uint8_t length;
	uint8_t data[INFO_ELEMENT_MAX_LENGTH];
};

#define SET_SAE_MAX_LENGTH 515
struct wim_set_sae_param {
	uint16_t eid;
	uint16_t length;
	uint8_t data[SET_SAE_MAX_LENGTH];
};

struct wim_tx_queue_param {
	uint8_t ac;
	uint16_t txop;
	uint16_t cw_min;
	uint16_t cw_max;
	uint8_t aifsn; /* aifs or aifsn? */
	uint8_t acm;
	uint8_t uapsd;
	uint8_t sta_type; /* 0: both, 1: sensor, 2: non-sensor */
	uint8_t rvd;
} __packed;

struct wim_tim_param {
	uint16_t aid;
	bool set;
};

struct wim_erp_param {
	uint8_t use_11b_protection;
	uint8_t use_short_preamble;
	uint8_t use_short_slot;
} __packed;
WIM_DECLARE(wim_erp);

#define FRAME_PKT_N_TLV_MAX    (1)
struct frame_pkt {
	struct hif_hdr   _hif_hdr;
	struct frame_hdr _frame_hdr;
	uint8_t *frame;
};

#define WIM_PKT_N_TLV_MAX    (10)
struct wim_pkt {
	struct hif_hdr _hif_hdr;
	struct wim_hdr _wim_hdr;
	struct nrc_tlv tlvs[WIM_PKT_N_TLV_MAX];
};

struct wim_addr_param {
	bool enable; /*0: awake, 1: sleep*/
	bool p2p;	/*0: non-p2p, 1: p2p*/
	uint8_t addr[6];
};

struct wim_noa_param {
	uint8_t index;
	uint8_t count;          /*how many absence period be scheduled*/
	uint32_t start_time;    /*Start Time of first absence period*/
	uint32_t interval;      /*time between consecutive absence periods*/
	uint32_t duration;      /*length of each absence period*/
};

enum WIM_S1G_CHANNEL_FLAGS {
	/* Recommended not to use MCS10 */
	WIM_S1G_CHANNEL_FLAG_MCS10_NOT_RECOMMENDED = BIT(2),
};

struct wim_s1g_channel_param {
	uint16_t pr_freq;
	uint16_t op_freq;
	uint16_t width;
	uint16_t flags;
};

enum WIM_TXPWR_TYPE {
	TXPWR_AUTO = 0,
	TXPWR_LIMIT,
	TXPWR_FIXED,
};

#endif /* _NRC_WIM_TYPES_H_ */
