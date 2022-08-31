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

#ifndef _NRC_H_
#define _NRC_H_

#include "nrc-build-config.h"
#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>
#include <linux/firmware.h>
#include <linux/sched.h>
#include <linux/circ_buf.h>
#include <linux/completion.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>


#define DEFAULT_INTERFACE_NAME			"nrc"
struct nrc_hif_device;
#define WIM_SKB_MAX         (10)
#define WIM_RESP_TIMEOUT    (msecs_to_jiffies(100))
#define NR_NRC_VIF			(2)
#define NR_NRC_VIF_HW_QUEUE	(4)
#define NR_NRC_MAX_TXQ		(130)
#ifdef CONFIG_CHECK_DATA_SIZE
#define NR_NRC_MAX_TXQ_SIZE	(1500*100) /* 150K */
#endif
/* VIF0 AC0~3,BCN, GP, VIF1 AC0~3,BCN */
#define NRC_QUEUE_MAX					(NR_NRC_VIF_HW_QUEUE*NR_NRC_VIF + 3)

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#else
#define IEEE80211_NUM_ACS				(4)
#define IEEE80211_P2P_NOA_DESC_MAX		(4)
#endif
#define NRC_MAX_TID						(8)

enum NRC_SCAN_MODE {
	NRC_SCAN_MODE_IDLE = 0,
	NRC_SCAN_MODE_SCANNING,
	NRC_SCAN_MODE_ABORTING,
};

#define NRC_FW_ACTIVE					(0)
#define NRC_FW_LOADING					(1)
#define	NRC_FW_PREPARE_SLEEP			(2)
#define	NRC_FW_SLEEP					(3)

enum NRC_DRV_STATE {
	NRC_DRV_REBOOT = -2,
	NRC_DRV_BOOT = -1,
	NRC_DRV_INIT = 0,
	NRC_DRV_STOP,
	NRC_DRV_CLOSING,
	NRC_DRV_CLOSED,
	NRC_DRV_START,
	NRC_DRV_RUNNING,
	NRC_DRV_PS,
};

enum NRC_PS_MODE {
	NRC_PS_NONE,
	NRC_PS_MODEMSLEEP,
	NRC_PS_DEEPSLEEP_TIM,
	NRC_PS_DEEPSLEEP_NONTIM
};

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#else
enum ieee80211_sta_state {
	/* NOTE: These need to be ordered correctly! */
	IEEE80211_STA_NOTEXIST,
	IEEE80211_STA_NONE,
	IEEE80211_STA_AUTH,
	IEEE80211_STA_ASSOC,
	IEEE80211_STA_AUTHORIZED,
};
#endif

enum ieee80211_tx_ba_state {
	IEEE80211_BA_NONE,
	IEEE80211_BA_REQUEST,
	IEEE80211_BA_ACCEPT,
	IEEE80211_BA_REJECT,
	IEEE80211_BA_CLOSE,
	IEEE80211_BA_DISABLE,
};

struct fwinfo_t {
	uint32_t ready;
	uint32_t version;
	uint32_t tx_head_size;
	uint32_t rx_head_size;
	uint32_t payload_align;
	uint32_t buffer_size;
	uint16_t hw_version;
};

struct vif_capabilities {
	uint64_t cap_mask;
};

struct nrc_capabilities {
	uint64_t cap_mask;
	uint16_t listen_interval;
	uint16_t bss_max_idle;
	uint8_t bss_max_idle_options;
	uint8_t max_vif;
	struct vif_capabilities vif_caps[NR_NRC_VIF];
};

#define BSS_MAX_ILDE_DEAUTH_LIMIT_COUNT 3 /* Keep Alive Timeout Limit Count on AP */
struct nrc_max_idle {
	bool enable;
	u16 period;
	u16 scale_factor;
	u8 options;
	u16 timeout_cnt;
	struct timer_list keep_alive_timer;

	unsigned long idle_period; /* jiffies */
	struct timer_list timer;
};

/* Private txq driver data structure */
struct nrc_txq {
	u16 hw_queue; /* 0: AC_BK, 1: AC_BE, 2: AC_VI, 3: AC_VO */
	struct list_head list;
	struct sk_buff_head queue; /* own queue */
#ifdef CONFIG_CHECK_DATA_SIZE
	unsigned int data_size;
#endif
	unsigned long nr_fw_queueud;
	unsigned long nr_push_allowed;
	struct ieee80211_vif vif;
	struct ieee80211_sta *sta;
};

struct nrc_delayed_deauth {
	atomic_t delayed_deauth;
	s8 vif_index;
	u16 aid;
	bool removed;
	struct sk_buff *deauth_frm;
	struct sk_buff *ch_skb;
	struct ieee80211_vif v;
	struct ieee80211_sta s;
	struct ieee80211_key_conf p;
	struct ieee80211_key_conf g;
	struct ieee80211_bss_conf b;
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	struct cfg80211_chan_def c;
	struct ieee80211_channel ch;
#else
	struct ieee80211_conf c;
#endif
	struct ieee80211_tx_queue_params tqp[NRC_QUEUE_MAX];
};

struct nrc {
	struct platform_device *pdev;
	struct nrc_hif_device *hif;

	struct ieee80211_hw *hw;
	struct ieee80211_vif *vif[NR_NRC_VIF];
	int nr_active_vif;
	bool enable_vif[NR_NRC_VIF];
	spinlock_t vif_lock;
	bool promisc;
#ifdef CONFIG_USE_NEW_BAND_ENUM
	struct ieee80211_supported_band bands[NUM_NL80211_BANDS];
#else
	struct ieee80211_supported_band bands[IEEE80211_NUM_BANDS];
#endif
	struct mac_address mac_addr[NR_NRC_VIF];

	int drv_state;
	atomic_t fw_state;
	atomic_t fw_tx;
	atomic_t fw_rx;
	u64 tsf_offset;
	u32 sleep_ms;
	struct nrc_txq ntxq[NRC_QUEUE_MAX];

	struct mutex target_mtx;
	struct mutex state_mtx;
	enum NRC_SCAN_MODE scan_mode;
	struct workqueue_struct *workqueue;
	struct delayed_work check_start;

	char alpha2[2];

	/* Move to vif or sta driver data */
	u8 frame_seqno;
	u8 wim_seqno;
	u8 band;
	u16 center_freq;
	u16 aid;
	u32 cipher_pairwise;
	u32 cipher_group;

	struct fwinfo_t fwinfo;
	struct nrc_capabilities cap;

	/* power management */
	enum ps_mode {
		PS_DISABLED,
		PS_ENABLED,
		PS_AUTO_POLL,
		PS_MANUAL_POLL
	} ps;
	bool ps_poll_pending;
	bool ps_enabled;
	bool ps_drv_state;
	bool invoke_beacon_loss;
	struct timer_list dynamic_ps_timer;
	struct workqueue_struct *ps_wq;

	/* sync power management operation between
	 * mac80211 config and host interface
	 */
	struct completion hif_tx_stopped;
	struct completion hif_rx_stopped;
	struct completion hif_irq_stopped;
	uint32_t vendor_arg;

	struct dentry *debugfs;
	bool loopback;
	bool ampdu_supported;
	int lb_count;
	bool amsdu_supported;
	bool block_frame;
	bool ampdu_reject;
	bool bd_valid;

	/* tx */
	spinlock_t txq_lock;
	struct list_head txq;
	/* 0: AC_BK, 1: AC_BE, 2: AC_VI, 3: AC_VO */
	atomic_t tx_credit[IEEE80211_NUM_ACS*3];
	atomic_t tx_pend[IEEE80211_NUM_ACS*3];

	struct completion wim_responded;
	struct sk_buff *last_wim_responded;

	/* firmware */
	struct nrc_fw_priv *fw_priv;
	struct firmware *fw;
	bool   has_macaddr[NR_NRC_VIF];

    /* p2p remain on channel */
	struct delayed_work roc_finish;

	/* beacon interval (AP) */
	u16 beacon_int;
	/* vendor specific element (AP) */
	struct sk_buff *vendor_skb;
	/* work for removing vendor specific ie for wowlan pattern (AP) */
	struct delayed_work rm_vendor_ie_wowlan_pattern;
	/* this must be assigned via nrc_mac_set_wakeup() */
	bool wowlan_enabled;

	/* work for fake frames */
	struct delayed_work fake_bcn;
	struct delayed_work fake_prb_resp;

	/* cloned beacon for cqm in deepsleep */
	struct sk_buff *c_bcn;

	/* cloned probe response for cqm in deepsleep */
	struct sk_buff *c_prb_resp;

	/* for processing deauth when deepsleep */
	struct nrc_delayed_deauth d_deauth;

	/* firmware recovery */
	struct nrc_recovery_wdt *recovery_wdt;
};

/* vif driver data structure */
struct nrc_vif {
	struct nrc *nw;
	int index;
	struct net_device *dev;

	/* scan */
	struct delayed_work scan_timeout;

	/* power save */
	bool ps_polling;

	/* MLME */
	spinlock_t preassoc_sta_lock;
	struct list_head preassoc_sta_list;

	/* inactivity */
	u16 max_idle_period;

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	/* P2p client NoA */
	struct ieee80211_noa_data noa;
#endif
};

#define to_ieee80211_vif(v) \
	container_of((void *)v, struct ieee80211_vif, drv_priv)

#define to_i_vif(v) ((struct nrc_vif *) (v)->drv_priv)

static inline int hw_vifindex(struct ieee80211_vif *vif)
{
	struct nrc_vif *i_vif;

	if (vif == NULL)
		return 0;

	i_vif = to_i_vif(vif);
	return i_vif->index;
}

/* sta driver data structure */
struct nrc_sta {
	struct nrc *nw;
	struct ieee80211_vif *vif;
	/*struct ieee80211_sta *sta;*/

	enum ieee80211_sta_state state;
	struct list_head list;

	/* keys */
	struct ieee80211_key_conf *ptk;
	struct ieee80211_key_conf *gtk;

	/* BSS max idle period */
	struct nrc_capabilities cap;
	struct nrc_max_idle max_idle;

	/* Block Ack Session per TID */
	enum ieee80211_tx_ba_state tx_ba_session[NRC_MAX_TID];
	uint32_t ba_req_last_jiffies[NRC_MAX_TID];
};

#define to_ieee80211_sta(s) \
	container_of((void *)s, struct ieee80211_sta, drv_priv)

#define to_i_sta(s) ((struct nrc_sta *) (s)->drv_priv)

struct nrc_sta_handler {
	int (*sta_state)(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			 struct ieee80211_sta *sta,
			 enum ieee80211_sta_state old_state,
			 enum ieee80211_sta_state new_state);

};

#define STAH(fn)					\
	static struct nrc_sta_handler __stah_ ## fn	\
	__attribute((__used__))				\
	__attribute((__section__("nrc.sta"))) = {	\
		.sta_state = fn,			\
	}

extern struct nrc_sta_handler __sta_h_start, __sta_h_end;

/* trx */

struct nrc_trx_data {
	struct nrc *nw;
	struct ieee80211_vif *vif;
	struct ieee80211_sta *sta;
	struct sk_buff *skb;
	int result;
};

struct nrc_trx_handler {
	struct list_head list;
	int (*handler)(struct nrc_trx_data *data);
	bool need_sta;
	u32 vif_types;
};

#define TXH(fn, mask)					\
	static struct nrc_trx_handler __txh_ ## fn	\
	__attribute((__used__))				\
	__attribute((__section__("nrc.txh"))) = {	\
		.handler = fn,				\
		.vif_types = mask,			\
	}

#define RXH(fn, mask)					\
	static struct nrc_trx_handler __rxh_ ## fn	\
	__attribute((__used__))				\
	__attribute((__section__("nrc.rxh"))) = {	\
		.handler = fn,				\
		.vif_types = mask,			\
	}

extern struct nrc_trx_handler __tx_h_start, __tx_h_end;
extern struct nrc_trx_handler __rx_h_start, __rx_h_end;


#define NL80211_IFTYPE_ALL (BIT(NUM_NL80211_IFTYPES)-1)

/* radio tap */
struct nrc_radiotap_hdr {
	struct ieee80211_radiotap_header hdr;
	uint64_t rt_tsft;		/* IEEE80211_RADIOTAP_TSFT */
	uint8_t  rt_flags;		/* IEEE80211_RADIOTAP_FLAGS */
	uint8_t  rt_pad;		/* pad for IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_frequency;	/* IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_flags;
	uint16_t rt_pad2;		/* pad for IEEE80211_RADIOTAP_TLV:S1G */
	uint16_t rt_tlv_type;	/* IEEE80211_RADIOTAP_TLV */
	uint16_t rt_tlv_length;
	uint16_t rt_s1g_known;
	uint16_t rt_s1g_data1;
	uint16_t rt_s1g_data2;
} __packed;

struct nrc_radiotap_hdr_agg {
	struct ieee80211_radiotap_header hdr;
	uint64_t rt_tsft;		/* IEEE80211_RADIOTAP_TSFT */
	uint8_t  rt_flags;		/* IEEE80211_RADIOTAP_FLAGS */
	uint8_t  rt_pad;		/* pad for IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_frequency; /* IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_flags;
	/* pad for IEEE80211_RADIOTAP_AMPDU_STATUS */
	uint16_t rt_pad2;
	uint32_t rt_ampdu_ref;	/* IEEE80211_RADIOTAP_AMPDU_STATUS */
	uint16_t rt_ampdu_flags;
	uint8_t  rt_ampdu_crc;
	uint8_t  rt_ampdu_reserved;
	uint16_t rt_tlv_type;	/* IEEE80211_RADIOTAP_TLV */
	uint16_t rt_tlv_length;
	uint16_t rt_s1g_known;
	uint16_t rt_s1g_data1;
	uint16_t rt_s1g_data2;
} __packed;

struct nrc_radiotap_hdr_ndp {
	struct ieee80211_radiotap_header hdr;
	uint64_t rt_tsft;		/* IEEE80211_RADIOTAP_TSFT */
	uint8_t  rt_flags;		/* IEEE80211_RADIOTAP_FLAGS */
	uint8_t  rt_pad;		/* pad for IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_frequency;	/* IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_flags;
	uint8_t  rt_zero_length_psdu; /* IEEE80211_RADIOTAP_0_LENGTH_PSDU */
} __packed;

struct nrc_local_radiotap_hdr {
	struct ieee80211_radiotap_header hdr;
	__le64 rt_tsft;
	u8 rt_flags;
	u8 rt_rate;
	__le16 rt_channel;
	__le16 rt_chbitmask;
} __packed;

/* Module parameters */
extern char *hifport;
extern int hifspeed;
extern int spi_bus_num;
extern int spi_cs_num;
extern int spi_gpio_irq;
extern int spi_polling_interval;
extern int spi_gdma_irq;
extern int disable_cqm;
extern int bss_max_idle_offset;
extern int power_save;
extern int sleep_duration[];
extern bool wlantest;
extern bool ndp_preq;
extern bool ndp_ack_1m;
extern bool enable_hspi_init;
extern bool nullfunc_enable;
extern bool auto_ba;
extern bool sw_enc;
extern bool signal_monitor;
extern bool enable_usn;
extern bool debug_level_all;
extern bool enable_short_bi;
extern int credit_ac_be;
extern bool discard_deauth;
extern bool enable_legacy_ack;
#if defined(CONFIG_SUPPORT_BD)
extern char *bd_name;
#endif /* CONFIG_SUPPORT_BD */

void nrc_set_bss_max_idle_offset(int value);
void nrc_set_auto_ba(bool toggle);
#endif
