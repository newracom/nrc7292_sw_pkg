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

#include "nrc-mac80211.h"
#include "nrc-fw.h"
#include "nrc-netlink.h"
#include "nrc-hif.h"
#include "wim.h"
#include "nrc-debug.h"
#include "nrc-init.h"
#include "nrc-vendor.h"
#include "nrc-stats.h"
#include "nrc-dump.h"
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <net/dst.h>
#include <net/xfrm.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <net/genetlink.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include "compat.h"
#if defined(CONFIG_S1G_CHANNEL)
#include "nrc-s1g.h"
#endif /* defined(CONFIG_S1G_CHANNEL) */
#if defined(CONFIG_SUPPORT_BD)
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "nrc-bd.h"
#else
#define US_BASE_FREQ	2412
#define K1_BASE_FREQ	5180
#define K2_BASE_FREQ	5180
#define JP_BASE_FREQ	5180
#define TW_BASE_FREQ	5180
#define EU_BASE_FREQ	5180
#define CN_BASE_FREQ	5180
#define NZ_BASE_FREQ	5180
#define AU_BASE_FREQ	5180
#endif /* defined(CONFIG_SUPPORT_BD) */

char nrc_cc[2];

#define CHAN2G(freq)			\
{								\
	.band = NL80211_BAND_2GHZ,	\
	.center_freq = (freq),		\
	.hw_value = (freq),			\
	.max_power = 20,			\
}

#define CHAN5G(freq)			\
{								\
	.band = NL80211_BAND_5GHZ,	\
	.center_freq = (freq),		\
	.hw_value = (freq),			\
	.max_power = 20,			\
}

#define NRC_CONFIGURE_FILTERS	\
	(FIF_ALLMULTI				\
	|FIF_PSPOLL					\
	|FIF_BCN_PRBRESP_PROMISC	\
	|FIF_PROBE_REQ				\
	)

#define FREQ_TO_100KHZ(mhz, khz) (mhz * 10 + khz / 100)

#if defined(CONFIG_S1G_CHANNEL)
static const struct ieee80211_sta_s1g_cap nrc_s1g_cap = {
	.s1g = true,
	.cap = {
		S1G_CAP0_SGI_1MHZ | S1G_CAP0_SGI_2MHZ | S1G_CAP0_SGI_4MHZ,
		0,
		0,
		S1G_CAP3_MAX_MPDU_LEN,
		0,
		S1G_CAP5_AMPDU,
		0,
		S1G_CAP7_DUP_1MHZ,
		S1G_CAP8_TWT_RESPOND | S1G_CAP8_TWT_REQUEST,
		0},
	.nss_mcs = { 0xfc | 1, /* MCS 7 for 1 SS */
	/* RX Highest Supported Long GI Data Rate 0:7 */
			0,
	/* RX Highest Supported Long GI Data Rate 0:7 */
	/* TX S1G MCS Map 0:6 */
			0xfa,
	/* TX S1G MCS Map :7 */
	/* TX Highest Supported Long GI Data Rate 0:6 */
			0x80,
	/* TX Highest Supported Long GI Data Rate 7:8 */
	/* Rx Single spatial stream and S1G-MCS Map for 1MHz */
	/* Tx Single spatial stream and S1G-MCS Map for 1MHz */
			0 },
};
static struct ieee80211_channel nrc_channels_s1ghz[MAX_S1G_CHANNEL_NUM];

#else
static struct ieee80211_channel nrc_channels_2ghz[] = {
	CHAN2G(2412), /* Channel 1 */
	CHAN2G(2417), /* Channel 2 */
	CHAN2G(2422), /* Channel 3 */
	CHAN2G(2427), /* Channel 4 */
	CHAN2G(2432), /* Channel 5 */
	CHAN2G(2437), /* Channel 6 */
	CHAN2G(2442), /* Channel 7 */
	CHAN2G(2447), /* Channel 8 */
	CHAN2G(2452), /* Channel 9 */
	CHAN2G(2457), /* Channel 10 */
	CHAN2G(2462), /* Channel 11 */
	CHAN2G(2467), /* Channel 12 */
	CHAN2G(2472), /* Channel 13 */
	CHAN2G(2484), /* Channel 14 */
};

static struct ieee80211_channel nrc_channels_5ghz[] = {
	CHAN5G(5180), /* Channel 36 */
	CHAN5G(5185), /* Channel 37 */
	CHAN5G(5190), /* Channel 38 */
	CHAN5G(5195), /* Channel 39 */
	CHAN5G(5200), /* Channel 40 */
	CHAN5G(5205), /* Channel 41 */
	CHAN5G(5210), /* Channel 42 */
	CHAN5G(5215), /* Channel 43 */
	CHAN5G(5220), /* Channel 44 */
	CHAN5G(5225), /* Channel 45 */
	CHAN5G(5230), /* Channel 46 */
	CHAN5G(5235), /* Channel 47 */
	CHAN5G(5240), /* Channel 48 */
	CHAN5G(5260), /* Channel 52 */
	CHAN5G(5280), /* Channel 56 */
	CHAN5G(5300), /* Channel 60 */
	CHAN5G(5320), /* Channel 64 */
	CHAN5G(5500), /* Channel 100 */
	CHAN5G(5520), /* Channel 104 */
	CHAN5G(5540), /* Channel 108 */
	CHAN5G(5560), /* Channel 112 */
	CHAN5G(5580), /* Channel 116 */
	CHAN5G(5745), /* Channel 149 */
	CHAN5G(5750), /* Channel 150 */
	CHAN5G(5755), /* Channel 151 */
	CHAN5G(5760), /* Channel 152 */
	CHAN5G(5765), /* Channel 153 */
	CHAN5G(5770), /* Channel 154 */
	CHAN5G(5775), /* Channel 155 */
	CHAN5G(5780), /* Channel 156 */
	CHAN5G(5785), /* Channel 157 */
	CHAN5G(5790), /* Channel 158 */
	CHAN5G(5795), /* Channel 159 */
	CHAN5G(5800), /* Channel 160 */
	CHAN5G(5805), /* Channel 161 */
	CHAN5G(5810), /* Channel 162 */
	CHAN5G(5815), /* Channel 163 */
	CHAN5G(5820), /* Channel 164 */
	CHAN5G(5825), /* Channel 165 */
};
#endif /* CONFIG_S1G_CHANNEL */

#if !defined(CONFIG_S1G_CHANNEL)
static struct ieee80211_rate nrc_rates[] = {
	/* 11b rates */
	{ .bitrate = 10 },
	{ .bitrate = 20, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 55, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 110, .flags = IEEE80211_RATE_SHORT_PREAMBLE },

	/* 11g rates */
	{ .bitrate = 60 },
	{ .bitrate = 90 },
	{ .bitrate = 120 },
	{ .bitrate = 180 },
	{ .bitrate = 240 },

	/* README it is removed for 11N Certification 5.2.34 */
	{ .bitrate = 360 },
	{ .bitrate = 480 },
	{ .bitrate = 540 }
};
#else
static struct ieee80211_rate nrc_rates[] = {
	/* 11ah rates */
	{ .bitrate = 10 },
	{ .bitrate = 20, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 40, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
//	{ .bitrate = 80, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
//	{ .bitrate = 160, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
};
#endif


#if defined(CONFIG_S1G_CHANNEL)
static const struct ieee80211_regdomain mac80211_regdom = {
	.n_reg_rules = 1,
	.alpha2 =  "99",
	.reg_rules = {
		REG_RULE(400, 1000, 4, 0, 30, 0),
	},
};
#else
static const struct ieee80211_regdomain mac80211_regdom = {
	.n_reg_rules = 4,
	.alpha2 =  "99",
	.reg_rules = {
		REG_RULE(2412-10, 2484+10, 40, 0, 30, 0),
		REG_RULE(5180-10, 5320+10, 40, 0, 30, 0),
		REG_RULE(5500-10, 5580+10, 40, 0, 30, 0),
		REG_RULE(5745-10, 5825+10, 40, 0, 30, 0),
	},
};
#endif /* CONFIG_S1G_CHANNEL */

static const char nrc_gstrings_stats[][ETH_GSTRING_LEN] = {
	"tx_pkts_nic",
	"tx_bytes_nic",
	"rx_pkts_nic",
	"rx_bytes_nic",
	"d_tx_dropped",
	"d_tx_failed",
	"d_ps_mode",
	"d_group",
	"d_tx_power",
};

static const u32 nrc_cipher_supported[] = {
#if !defined(CONFIG_S1G_CHANNEL)
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
#endif
	WLAN_CIPHER_SUITE_CCMP,
#ifdef CONFIG_SUPPORT_CCMP_256
	WLAN_CIPHER_SUITE_CCMP_256,
#endif
#ifdef CONFIG_SUPPORT_GCMP
	 WLAN_CIPHER_SUITE_GCMP,
	 WLAN_CIPHER_SUITE_GCMP_256,
#endif
#ifdef CONFIG_SUPPORT_GMAC
	 WLAN_CIPHER_SUITE_BIP_GMAC_128,
	 WLAN_CIPHER_SUITE_BIP_GMAC_256,
#endif

	WLAN_CIPHER_SUITE_AES_CMAC,
};

static const struct ieee80211_iface_limit if_limits_multi[] = {
	{
		.max = 2,
		.types = BIT(NL80211_IFTYPE_STATION) |
#ifdef CONFIG_MAC80211_MESH
			 BIT(NL80211_IFTYPE_MESH_POINT) |
#endif
#if defined(CONFIG_WIRELESS_WDS)
			BIT(NL80211_IFTYPE_WDS) |
#endif
			BIT(NL80211_IFTYPE_AP)
	},
};

static const struct ieee80211_iface_combination if_comb_multi[] = {
	{
		.limits = if_limits_multi,
		.n_limits = ARRAY_SIZE(if_limits_multi),
		/* The number of supported VIF (2) + P2P_DEVICE */
		.max_interfaces = 2,
		.num_different_channels = 1,

		.beacon_int_infra_match = true,
#if !defined(CONFIG_S1G_CHANNEL)
		.radar_detect_widths =
			BIT(NL80211_CHAN_WIDTH_20_NOHT) |
			BIT(NL80211_CHAN_WIDTH_20) |
			BIT(NL80211_CHAN_WIDTH_40) |
			BIT(NL80211_CHAN_WIDTH_80) |
			BIT(NL80211_CHAN_WIDTH_160),
#endif
	},
};

#if defined(CONFIG_SUPPORT_IBSS )
u64 current_bssid_beacon_timestamp;
#endif

static void force_sw_enc_mode_by_sta_type (struct nrc *nw, struct ieee80211_vif *vif)
{
	int is_relay = ((nw->vif[0] && nw->vif[1]) && (sw_enc != WIM_ENCDEC_HYBRID));
#ifdef CONFIG_SUPPORT_IBSS
	int is_ibss = (vif->type == NL80211_IFTYPE_ADHOC);
#else
	int is_ibss = 0;
#endif
	int is_mesh = (((nw->vif[0] && nw->vif[0]->type == NL80211_IFTYPE_MESH_POINT) ||
			(nw->vif[1] && nw->vif[1]->type == NL80211_IFTYPE_MESH_POINT)) &&
			(sw_enc != WIM_ENCDEC_HYBRID));

	switch (nw->chip_id) {
		case 0x7292:
			if (is_relay || is_ibss || is_mesh) {
				sw_enc = WIM_ENCDEC_SW;
				nrc_mac_dbg("Force sw enc\n");
			}
			break;
		case 0x7393:
		case 0x7394:
			if (is_ibss || is_mesh) {
				sw_enc = WIM_ENCDEC_SW;
				nrc_mac_dbg("Force sw enc\n");
			}
			break;
		default:
			dev_err(nw->dev, "Unknown Newracom IEEE80211 chipset %04x",
					nw->chip_id);
			BUG();
	}
}

static bool get_intf_addr(const char *intf_name, char *addr)
{
	struct socket *sock	= NULL;
	struct net_device *dev	= NULL;
	struct net *net		= NULL;
	int retval = 0;

	if (!addr || !intf_name)
		return false;

	retval = sock_create(AF_INET, SOCK_STREAM, 0, &sock);
	if (retval < 0)
		return false;

	net = sock_net(sock->sk);

	dev = dev_get_by_name_rcu(net, intf_name);
	if (!dev) {
		sock_release(sock);
		return false;
	}

	memcpy(addr, dev->dev_addr, 6);

	sock_release(sock);

	return true;
}

static void set_mac_address(struct mac_address *macaddr, u8 vif)
{
	eth_zero_addr(macaddr->addr);
	get_intf_addr("eth0", macaddr->addr);

	macaddr->addr[0] = 0x2;
	macaddr->addr[1] = vif;
	macaddr->addr[5]++;
}

#ifdef CONFIG_USE_TXQ
static inline struct ieee80211_txq *to_txq(struct nrc_txq *p)
{
	return container_of((void *)p, struct ieee80211_txq, drv_priv);
}

/**
 * nrc_txq_init - Initialize txq driver data
 */
static void nrc_init_txq(struct ieee80211_txq *txq, struct ieee80211_vif *vif, struct ieee80211_sta *sta)
{
	struct nrc_txq *q;

	if (!txq)
		return;

	q = (void *)txq->drv_priv;
	INIT_LIST_HEAD(&q->list);
	q->hw_queue = vif->hw_queue[txq->ac];
	q->sta = sta;
}

static void nrc_flush_txq(struct nrc *nw)
{
#if KERNEL_VERSION(5, 4, 42) <= NRC_TARGET_KERNEL_VERSION
	struct sk_buff *skb;
	struct ieee80211_txq *txq;
	u32 ac;

	spin_lock_bh(&nw->txq_lock);
	rcu_read_lock();
	for (ac = 0; ac < IEEE80211_NUM_ACS; ac++) {
		ieee80211_txq_schedule_start(nw->hw, ac);
		while ((txq = ieee80211_next_txq(nw->hw, ac))) {
			while ((skb = ieee80211_tx_dequeue_ni(nw->hw, txq)) != NULL) {
				ieee80211_free_txskb(nw->hw, skb);
			}
			ieee80211_return_txq(nw->hw, txq, false);
		}
		ieee80211_txq_schedule_end(nw->hw, ac);
	}
	rcu_read_unlock();
	spin_unlock_bh(&nw->txq_lock);
#endif
}

unsigned int nrc_ac_credit(struct nrc *nw, int ac)
{
	int ret;

	ret = atomic_read(&nw->tx_credit[ac])
		- atomic_read(&nw->tx_pend[ac]);
	if (ret < 0)
		return 0;
	return ret;
}

static __u32 nrc_txq_pending(struct ieee80211_hw *hw)
{
	struct nrc *nw = hw->priv;
	struct nrc_txq *cur, *next;
	int ac;
	int len = 0;
	u64 now = 0, diff = 0;

	if (nw->drv_state != NRC_DRV_RUNNING)
		return 0;

	now = ktime_to_us(ktime_get_real());

	spin_lock_bh(&nw->txq_lock);

	rcu_read_lock();
	list_for_each_entry_safe(cur, next, &nw->txq, list) { /* CKLEE_TODO */
		ac = cur->hw_queue;
		len += atomic_read(&nw->tx_pend[ac]);
		/* Need to consider credit? */
	}
	rcu_read_unlock();

	spin_unlock_bh(&nw->txq_lock);

	diff = ktime_to_us(ktime_get_real()) - now;
	if ((!diff) || (diff > NRC_MAC80211_RCU_LOCK_THRESHOLD))
		nrc_mac_dbg("%s, diff=%lu", __func__, (unsigned long)diff);

	return len;
}


static int nrc_push_txq(struct nrc *nw, struct nrc_txq *ntxq)
{
	struct sk_buff *skb;
	struct ieee80211_txq *txq;
	struct ieee80211_tx_control control;
	int ac, credit;
	int ret = 0;

	txq = to_txq(ntxq);
	ac = ntxq->hw_queue;
	credit = nrc_ac_credit(nw, ac);

#if 0
	{
	unsigned long frame_cnt, byte_cnt;
	ieee80211_txq_get_depth(txq, &frame_cnt, &byte_cnt);
	nrc_mac_dbg("Queue: frame:%u, byte:%u, credit:%d", frame_cnt, byte_cnt, credit);
	}
#endif

	if (credit == 0) {
		//nrc_mac_dbg("Wake up but no chance");
		return 1;
	}

	control.sta = ntxq->sta;

	rcu_read_lock();

	while ((skb = ieee80211_tx_dequeue(nw->hw, txq)) != NULL) {
		nrc_mac_tx(nw->hw, &control, skb);
		credit --;
		if (credit <= 0) {
			ret = 1;
			break;
		}
	}

	rcu_read_unlock();

	/* 0: all skb is pushed, 1: all credit is consumed */
	return ret;
}


/**
 * nrc_wake_tx_queue
 *
 * The function moves skb in @txq to sk_buff_head in its private data
 * structure, and add it to woken up list.
 */
static
void nrc_wake_tx_queue(struct ieee80211_hw *hw, struct ieee80211_txq *txq)
{
	struct nrc *nw = hw->priv;
	struct nrc_txq *ntxq = (void *)txq->drv_priv;

	if (nw->drv_state == NRC_DRV_PS) {
		//ieee80211_stop_queues(nw->hw); /* wake_queues in spi_resume */
		nrc_hif_wake_target(nw->hif);
	}

	spin_lock_bh(&nw->txq_lock);

	if (list_empty(&ntxq->list)) 
		list_add_tail(&ntxq->list, &nw->txq);

	spin_unlock_bh(&nw->txq_lock);

	nrc_kick_txq(nw);
}

#ifdef CONFIG_NEW_TASKLET_API
void nrc_tx_tasklet(struct tasklet_struct *t)
{
	struct nrc *nw = from_tasklet(nw, t, tx_tasklet);
#else
void nrc_tx_tasklet(unsigned long cookie)
{
	struct nrc *nw = (struct nrc *)cookie;
#endif
	struct nrc_txq *ntxq, *tmp;
	int ret;

	spin_lock_bh(&nw->txq_lock);

	list_for_each_entry_safe(ntxq, tmp, &nw->txq, list) {
		ret = nrc_push_txq(nw, ntxq); /* 0: all skb is pushed, 1: all credit is consumed */
		if (ret == 0) {
				list_del_init(&ntxq->list);
		} else { /* If the credit is insufficient, give way to the next txq. */
				list_move_tail(&ntxq->list, &nw->txq);
				break;
		}
	}

	spin_unlock_bh(&nw->txq_lock);
}

/**
 * nrc_kick_txq - push tx frames to hif.
 *
 * This function is called from both wake_tx_queue() callback
 * and the ac queue status event handler.
 *
 * TODO:
 * Decide scheduling algorithm: first-come first-serve or
 * fairness between txq's (currently, it's first-come first served).
 */
void nrc_kick_txq(struct nrc *nw)
{

	if (nw->drv_state != NRC_DRV_RUNNING)
		return;

	tasklet_schedule(&nw->tx_tasklet);
}

/*
 * nrc_cleanup_txq_all - cleanup txq
 *
 * This function is called from nrc_unregister_hw()
 *
 * This function must be called before ieee80211_unregister_hw()
 */
void nrc_cleanup_txq_all (struct nrc *nw)
{
	struct sk_buff *skb;
	struct ieee80211_txq *txq;
	struct nrc_txq *cur, *next;

	spin_lock_bh(&nw->txq_lock);
	rcu_read_lock();
	list_for_each_entry_safe(cur, next, &nw->txq, list) {
		txq = to_txq(cur);
		while ((skb = ieee80211_tx_dequeue(nw->hw, txq)) != NULL) {
			ieee80211_free_txskb(nw->hw, skb);
		}
		list_del_init(&cur->list);
	}
	rcu_read_unlock();
	spin_unlock_bh(&nw->txq_lock);
}

void nrc_cleanup_txq (struct nrc *nw, struct ieee80211_txq *txq)
{
	struct sk_buff *skb;
	struct nrc_txq *ntxq;

	if (!txq)
		return;

	ntxq = (struct nrc_txq *)txq->drv_priv;

	spin_lock_bh(&nw->txq_lock);
	rcu_read_lock();
	while ((skb = ieee80211_tx_dequeue(nw->hw, txq)) != NULL) {
		ieee80211_free_txskb(nw->hw, skb);
	}
	if (!list_empty(&ntxq->list)) {
		list_del_init(&ntxq->list);
	}
	rcu_read_unlock();
	spin_unlock_bh(&nw->txq_lock);
}

void nrc_cleanup_txq_by_macaddr (struct nrc *nw, struct ieee80211_vif *vif,
					uint8_t *macaddr)
{
	struct ieee80211_sta *sta;
	int i;

	sta = ieee80211_find_all_sta(vif, macaddr);

	if (!sta) {
		rcu_read_unlock();
		nrc_mac_dbg("[%s] sta is NULL", __func__);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(sta->txq); i++) {
		nrc_cleanup_txq(nw, sta->txq[i]);
	}
}

#else
#define nrc_init_txq(txq, vif)
#endif

static void nrc_assoc_h_basic(struct ieee80211_hw *hw,
							struct ieee80211_vif *vif,
							struct ieee80211_bss_conf *info,
							struct ieee80211_sta *sta,
							struct sk_buff *skb)
{
	struct nrc *nw = hw->priv;
#ifdef CONFIG_USE_VIF_CFG
	struct ieee80211_vif_cfg *vif_cfg = &vif->cfg;
#endif
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	struct ieee80211_chanctx_conf *conf;
#else
	struct ieee80211_conf	*conf = &hw->conf;
	struct ieee80211_channel *conf_chan = conf->channel;
#endif

#ifdef CONFIG_USE_NEW_BAND_ENUM
	enum nl80211_band band;
#else
	enum ieee80211_band band;
#endif

#ifdef CONFIG_USE_VIF_CFG
	nrc_mac_dbg("%s: aid=%u, bssid=%pM", __func__,
		vif_cfg->aid, info->bssid);
	nw->aid = vif_cfg->aid;
#else
	nrc_mac_dbg("%s: aid=%u, bssid=%pM", __func__,
		info->aid, info->bssid);
	nw->aid = info->aid;
#endif

#ifdef CONFIG_TRX_BACKOFF
	nw->ampdu_supported = 0;
#endif
#ifdef CONFIG_USE_VIF_CFG
	nrc_wim_skb_add_tlv(skb, WIM_TLV_AID, sizeof(vif_cfg->aid), &vif_cfg->aid);
#else
	nrc_wim_skb_add_tlv(skb, WIM_TLV_AID, sizeof(info->aid), &info->aid);
#endif
	nrc_wim_skb_add_tlv(skb, WIM_TLV_BSSID, ETH_ALEN, (void *)info->bssid);

	/* Enable later when rate adaptation is supported in the target */
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
#ifdef CONFIG_USE_BSS_CHAN_CONF
	conf = rcu_dereference(vif->bss_conf.chanctx_conf);
#else
	conf = rcu_dereference(vif->chanctx_conf);
#endif /* ifdef CONFIG_USE_BSS_CHAN_CONF */
	band = conf->def.chan->band;
#else
	band = conf_chan->band;
#endif

#ifdef CONFIG_SUPPORT_LINK_STA
	nrc_wim_skb_add_tlv(skb, WIM_TLV_SUPPORTED_RATES,
						sizeof(sta->deflink.supp_rates[band]),
						&sta->deflink.supp_rates[band]);
#else
	nrc_wim_skb_add_tlv(skb, WIM_TLV_SUPPORTED_RATES,
						sizeof(sta->supp_rates[band]),
						&sta->supp_rates[band]);
#endif /* ifdef CONFIG_SUPPORT_LINK_STA */
	nrc_wim_skb_add_tlv(skb, WIM_TLV_BASIC_RATE,
						sizeof(info->basic_rates), &info->basic_rates);
}


static
void nrc_assoc_h_ht(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
					struct ieee80211_bss_conf *bss_conf,
					struct ieee80211_sta *sta, struct sk_buff *skb)
{
#ifdef CONFIG_SUPPORT_LINK_STA
	struct ieee80211_sta_ht_cap *ht_cap = &sta->deflink.ht_cap;
#else
	struct ieee80211_sta_ht_cap *ht_cap = &sta->ht_cap;
#endif /* ifdef CONFIG_SUPPORT_LINK_STA */

	/* Assumption: ht_cap->ht_supported is false if HT Capabilities
	 * element is not included in the Association Response frame
	 */

	nrc_mac_dbg("%s: %s (ht_cap=%04x)", __func__,
		ht_cap->ht_supported ? "HT" : "non-HT",
		ht_cap->ht_supported ? ht_cap->cap : 0x0);

	if (!ht_cap->ht_supported)
		return;

	nrc_wim_skb_add_tlv(skb, WIM_TLV_HT_CAP, sizeof(u16), &ht_cap->cap);

	/* TODO: MCS */
}

static
void nrc_assoc_h_phymode(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			 struct ieee80211_bss_conf *bss_conf,
			 struct ieee80211_sta *sta, struct sk_buff *skb)
{
	enum { PHY_HT_NONE = 0, PHY_11B = 1, PHY_HT_MF = 3};
	static char * const phymodestr[] = {"HT-none", "11b", "", "HT"};
	u8 phymode;

#ifdef CONFIG_SUPPORT_LINK_STA
	/* HT_MF, non-HT, 11b */
	if (sta->deflink.ht_cap.ht_supported)
		phymode = PHY_HT_MF;
	else if (sta->deflink.supp_rates[NL80211_BAND_2GHZ] >> 4)
		phymode = PHY_HT_NONE;
	else
		phymode = PHY_11B;
#else
	/* HT_MF, non-HT, 11b */
	if (sta->ht_cap.ht_supported)
		phymode = PHY_HT_MF;
	else if (sta->supp_rates[NL80211_BAND_2GHZ] >> 4)
		phymode = PHY_HT_NONE;
	else
		phymode = PHY_11B;
#endif /* ifdef CONFIG_SUPPORT_LINK_STA */

	nrc_mac_dbg("%s: phy mode=%s", __func__, phymodestr[phymode]);

	nrc_wim_skb_add_tlv(skb, WIM_TLV_PHY_MODE, sizeof(u8), &phymode);
}


static
void nrc_bss_assoc(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			  struct ieee80211_bss_conf *bss_conf,
			  struct sk_buff *skb)
{
	struct ieee80211_sta *ap_sta;
	u64 now = 0, diff = 0;

	now = ktime_to_us(ktime_get_real());

	rcu_read_lock();
	ap_sta = ieee80211_find_sta(vif, bss_conf->bssid);
	if (!ap_sta) {
		nrc_mac_dbg("failed to find sta for bss %pM",
			bss_conf->bssid);
		goto out;
	}

	nrc_assoc_h_basic(hw, vif, bss_conf, ap_sta, skb);
	nrc_assoc_h_ht(hw, vif, bss_conf, ap_sta, skb);
	nrc_assoc_h_phymode(hw, vif, bss_conf, ap_sta, skb);

 out:
	rcu_read_unlock();
	diff = ktime_to_us(ktime_get_real()) - now;
	if ((!diff) || (diff > NRC_MAC80211_RCU_LOCK_THRESHOLD))
		nrc_mac_dbg("%s, diff=%lu", __func__, (unsigned long)diff);
}

static int nrc_vendor_update_beacon(struct ieee80211_hw *hw,
			struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb, *b;
	u8 *pos;
	u16 need_headroom, need_tailroom;
#ifdef CONFIG_USE_LINK_ID
	b = ieee80211_beacon_get_template(hw, vif, NULL, vif->bss_conf.link_id);
#else
	b = ieee80211_beacon_get_template(hw, vif, NULL);
#endif /* ifdef CONFIG_USE_LINK_ID */
	if (!b)
		return -EINVAL;

	if (nw->vendor_skb_beacon) {
		need_headroom = skb_headroom(b);
		need_tailroom = nw->vendor_skb_beacon->len;

		if (skb_tailroom(b) < need_tailroom) {
			if (pskb_expand_head(b, need_headroom, need_tailroom,
						GFP_ATOMIC)) {
				nrc_mac_dbg("Fail to expand Beacon for vendor elem (need: %d)",
					need_tailroom);
				dev_kfree_skb_any(b);
				dev_kfree_skb_any(nw->vendor_skb_beacon);
				nw->vendor_skb_beacon = NULL;
				return 0;
			}
		}
		pos = skb_put(b, nw->vendor_skb_beacon->len);
		memcpy(pos, nw->vendor_skb_beacon->data, nw->vendor_skb_beacon->len);
	}

	if (b->len > WIM_MAX_SIZE) {
		nrc_mac_dbg("Fail to alloc skb for wim(b->len:%d, max: %d)",
			b->len, WIM_MAX_SIZE);
		dev_kfree_skb_any(b);
		dev_kfree_skb_any(nw->vendor_skb_beacon);
		nw->vendor_skb_beacon = NULL;
		return 0;
	}

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	pos = nrc_wim_skb_add_tlv(skb, WIM_TLV_BEACON, b->len, b->data);

	dev_kfree_skb_any(b);
	return nrc_xmit_wim_request(nw, skb);
}

static int nrc_vendor_update_probe_req(struct ieee80211_hw *hw,
			struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;

	if (nw->vendor_skb_probe_req->len > WIM_MAX_SIZE) {
		nrc_mac_dbg("Fail to alloc skb for wim(vendor_skb_probe_req->len:%d, max: %d)",
			nw->vendor_skb_probe_req->len, WIM_MAX_SIZE);
		return 0;
	}

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_PROBE_REQ_VENDOR_IE,
				        nw->vendor_skb_probe_req->len,
						nw->vendor_skb_probe_req->data);

	return nrc_xmit_wim_request(nw, skb);
}

static int nrc_vendor_update_probe_rsp(struct ieee80211_hw *hw,
			struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;

	if (nw->vendor_skb_probe_rsp->len > WIM_MAX_SIZE) {
		nrc_mac_dbg("Fail to alloc skb for wim(vendor_skb_probe_rsp->len:%d, max: %d)",
			nw->vendor_skb_probe_rsp->len, WIM_MAX_SIZE);
		return 0;
	}

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_PROBE_RSP_VENDOR_IE,
				        nw->vendor_skb_probe_rsp->len,
						nw->vendor_skb_probe_rsp->data);

	return nrc_xmit_wim_request(nw, skb);
}

static int nrc_vendor_update_assoc_req(struct ieee80211_hw *hw,
			struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;

	if (nw->vendor_skb_assoc_req->len > WIM_MAX_SIZE) {
		nrc_mac_dbg("Fail to alloc skb for wim(vendor_skb_assoc_req->len:%d, max: %d)",
			nw->vendor_skb_assoc_req->len, WIM_MAX_SIZE);
		return 0;
	}

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_ASSOC_REQ_VENDOR_IE,
				        nw->vendor_skb_assoc_req->len,
						nw->vendor_skb_assoc_req->data);

	return nrc_xmit_wim_request(nw, skb);
}

static int nrc_mac_start(struct ieee80211_hw *hw)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
	int alloc_size;

#if defined(CONFIG_SUPPORT_BD)
	/* Case of Invalid board data */
	if(!g_bd_valid) {
		nrc_mac_dbg("@@@@@@@@@ bd(%d) is invalid. Stop MAC START @@@@@@@", g_bd_valid);
		return -1;
	}
#endif

	nw->drv_state = NRC_DRV_RUNNING;
	nw->aid = 0;

	alloc_size = tlv_len(sizeof(u16)) + tlv_len(ETH_ALEN);
	if (nrc_mac_is_s1g(nw)) {
		alloc_size += tlv_len(sizeof(u8));
	}
	skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, alloc_size);

	nrc_wim_set_aid(nw, skb, nw->aid);
	nrc_wim_add_mac_addr(nw, skb, nw->mac_addr[0].addr);

	if (nrc_mac_is_s1g(nw)) {
		nrc_wim_set_ndp_preq(nw, skb, ndp_preq);
#if defined(CONFIG_SUPPORT_LEGACY_ACK)
		if(enable_legacy_ack)
			nrc_wim_set_legacy_ack(nw, skb, enable_legacy_ack);
#endif /* CONFIG_SUPPORT_LEGACY_ACK */
	}

	nrc_xmit_wim_request(nw, skb);

	return 0;
}

void nrc_mac_stop(struct ieee80211_hw *hw)
{
	struct nrc *nw = hw->priv;
	int ret = 0;

	if (nw->drv_state == NRC_DRV_CLOSING ||
		atomic_read(&nw->d_deauth.delayed_deauth))
		return;

	nw->drv_state = NRC_DRV_STOP;
	ret = nrc_hif_check_target(nw->hif, 0x1B);
	if (ret != 1) {
		nrc_xmit_wim_simple_request(nw, WIM_CMD_STOP);
	}
}

/**
 * DOC: VIF handlers
 *
 */
static int nrc_alloc_vif_index(struct nrc *nw, struct ieee80211_vif *vif)
{
	int i;
	const int P2P_INDEX = 1;

	spin_lock_bh(&nw->vif_lock);
	for (i = 0; i < ARRAY_SIZE(nw->vif); i++) {
		/* Assign P2P DEVICE's index to P2P GO or GC */
		if (vif->p2p && i == P2P_INDEX && nw->vif[i])
			nw->vif[i] = NULL;

		if (!nw->vif[i]) {
			struct nrc_vif *i_vif = to_i_vif(vif);

			i_vif->index = i;
			nw->vif[i] = vif;
			nw->enable_vif[i] = true;
			spin_unlock_bh(&nw->vif_lock);
			return 0;
		} else if (!memcmp(vif->addr, nw->vif[i]->addr, ETH_ALEN)) {
			nw->enable_vif[i] = true;
			spin_unlock_bh(&nw->vif_lock);
			return 0;
		}
	}

	spin_unlock_bh(&nw->vif_lock);
	/* All h/w vifs are in use */
	return -EBUSY;
}

void nrc_free_vif_index(struct nrc *nw, struct ieee80211_vif *vif)
{
	struct nrc_vif *i_vif = to_i_vif(vif);

	spin_lock_bh(&nw->vif_lock);
	nw->vif[i_vif->index] = NULL;
	nw->enable_vif[i_vif->index] = false;
	i_vif->index = -1;
	spin_unlock_bh(&nw->vif_lock);
}

bool nrc_access_vif(struct nrc *nw)
{
	int i;

	spin_lock_bh(&nw->vif_lock);
	for (i = 0; i < ARRAY_SIZE(nw->vif); i++) {
		/* FIXME: use vif_lock for fw_state -- sw.ki -- 2019-1022 */
		if (nw->vif[i] /*&& nw->fw_state == NRC_FW_ACTIVE*/) {
			spin_unlock_bh(&nw->vif_lock);
			return true;
		}
	}
	spin_unlock_bh(&nw->vif_lock);
	return false;
}

const char *iftype_string(enum nl80211_iftype iftype)
{
	switch (iftype) {
	case NL80211_IFTYPE_UNSPECIFIED: return "UNSPECIFIED";
	case NL80211_IFTYPE_ADHOC: return "ADHOC";
	case NL80211_IFTYPE_STATION: return "STATION";
	case NL80211_IFTYPE_AP: return "AP";
	case NL80211_IFTYPE_AP_VLAN: return "AP_VLAN";
	case NL80211_IFTYPE_WDS: return "WDS";
	case NL80211_IFTYPE_MONITOR: return "MONITOR";
	case NL80211_IFTYPE_MESH_POINT: return "MESH_POINT";
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	case NL80211_IFTYPE_P2P_CLIENT: return "P2P_CLIENT";
	case NL80211_IFTYPE_P2P_GO: return "P2P_GO";
	case NL80211_IFTYPE_P2P_DEVICE: return "P2P_DEVICE";
#endif
#ifdef CONFIG_SUPPORT_IFTYPE_OCB
	case NL80211_IFTYPE_OCB: return "OCB";
#endif
	default: return "UNKNOWN Type";
	}
}

void nrc_mac_roc_finish(struct work_struct *work)
{
	struct nrc *nw = container_of(work, struct nrc, roc_finish.work);

	nrc_mac_dbg("%s", __func__);
	ieee80211_remain_on_channel_expired(nw->hw);
}

#ifdef CONFIG_USE_SCAN_TIMEOUT
static void nrc_mac_scan_timeout(struct work_struct *work);
#endif

static int nrc_mac_add_interface(struct ieee80211_hw *hw,
				 struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;
	struct nrc_vif *i_vif = to_i_vif(vif);
	u64 now = 0, diff = 0;

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	/* 20190724, jmjang, CB#8781, Gerrit #2200
	 *	This feature is opposite to legacy power save in host mode and
	 *  so this feature is blocked.
	 *	Consideration: There is an WFA test case about WMM-PS but
	 *  it will be related to Wi-Fi P2P
	 */
	/* vif->driver_flags |= IEEE80211_VIF_SUPPORTS_UAPSD; */
#endif
	memset(i_vif, 0, sizeof(*i_vif));

	if (WARN_ON(nrc_alloc_vif_index(nw, vif) < 0))
		return -1;

	if (vif->type == NL80211_IFTYPE_MONITOR) {
		/*
		 * Monitor interfaces are the only active interfaces.
		 * Put the target into promiscuous mode
		 */
		nw->promisc = true;

		nrc_mac_dbg("promiscuous mode on");
		goto out;
	}

	nw->promisc = false;


#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	if (vif->type == NL80211_IFTYPE_AP ||
		vif->type == NL80211_IFTYPE_WDS ||
		vif->type == NL80211_IFTYPE_P2P_GO
#if defined CONFIG_SUPPORT_IBSS
		|| vif->type == NL80211_IFTYPE_ADHOC
#endif
	) {
		if (i_vif->index == 0) { /* VIF0 */
			vif->cab_queue = 4;
		} else { /* VIF1 */
			if (nw->hw_queues == 6) { /* 7292 type, Use GP */
				vif->cab_queue = 4;
			} 
			else if(nw->hw_queues == 11) { /* 7393 type, Use BK1, BE1,... */
				vif->cab_queue = 10;
			} else {
				dev_err(nw->dev, "Invalid Chip ID(0x%x), queues:%d\n", nw->chip_id, nw->hw_queues);
				BUG();
			}
			
		}
	}
	else
		vif->cab_queue = IEEE80211_INVAL_HW_QUEUE;

	/* VIF0 */
	if (i_vif->index == 0) {
		vif->hw_queue[IEEE80211_AC_VO] = 3;
		vif->hw_queue[IEEE80211_AC_VI] = 2;
		vif->hw_queue[IEEE80211_AC_BE] = 1;
		vif->hw_queue[IEEE80211_AC_BK] = 0;
	}
	/* VIF1 */
	if (i_vif->index == 1) { /* VIF1 */
		switch (nw->hw_queues) {
			case 6:  /* 7292 type, Use GP */
				vif->hw_queue[IEEE80211_AC_VO] = 5;
				vif->hw_queue[IEEE80211_AC_VI] = 5;
				vif->hw_queue[IEEE80211_AC_BE] = 5;
				vif->hw_queue[IEEE80211_AC_BK] = 5;
				break;
			case 11: /* 7393 type, Use BK1, BE1,... */
				vif->hw_queue[IEEE80211_AC_VO] = 9;
				vif->hw_queue[IEEE80211_AC_VI] = 8;
				vif->hw_queue[IEEE80211_AC_BE] = 7;
				vif->hw_queue[IEEE80211_AC_BK] = 6;
				break;
			default:
				dev_err(nw->dev, "Invalid Chip ID(0x%x), queues:%d\n", nw->chip_id, nw->hw_queues);
				BUG();
		}
	}
	if (i_vif->index > 1) {
		dev_err(nw->dev, "Invalid Vif Index(%d)\n", i_vif->index);
		BUG();
	}
	nrc_dbg(NRC_DBG_MAC, "%s: VIF%d's hwqueue:%d", __func__, i_vif->index, nw->hw_queues);
#endif

	nrc_init_txq(vif->txq, vif, NULL);

	i_vif->nw = nw;
	i_vif->max_idle_period = nw->cap.bss_max_idle;

	now = ktime_to_us(ktime_get_real());

	rcu_read_lock();
	i_vif->dev = dev_getbyhwaddr_rcu(wiphy_net(hw->wiphy), ARPHRD_ETHER,
					 vif->addr);
	rcu_read_unlock();

	diff = ktime_to_us(ktime_get_real()) - now;
	if ((!diff) || (diff > NRC_MAC80211_RCU_LOCK_THRESHOLD))
		nrc_mac_dbg("%s, diff=%lu", __func__, (unsigned long)diff);

	if (i_vif->dev == NULL)
		return -1;

	spin_lock_init(&i_vif->preassoc_sta_lock);
	INIT_LIST_HEAD(&i_vif->preassoc_sta_list);

#ifdef CONFIG_USE_SCAN_TIMEOUT
	INIT_DELAYED_WORK(&i_vif->scan_timeout, nrc_mac_scan_timeout);
#endif

	if (vif->type == NL80211_IFTYPE_MESH_POINT)
		signal_monitor = true;

	nrc_mac_dbg("%s %s addr:%pM hwindex:%d %s", __func__,
			i_vif->dev ? netdev_name(i_vif->dev):"NULL",
			vif->addr, i_vif->index, iftype_string(vif->type));

 out:

	if (vif->p2p)
		nrc_wim_set_p2p_addr(nw, vif);
	else
		nrc_wim_set_mac_addr(nw, vif);

	force_sw_enc_mode_by_sta_type(nw, vif);
	nrc_wim_set_sta_type(nw, vif);
	nrc_hif_resume_rx_thread(nw->hif);

	return 0;
}

static int nrc_mac_change_interface(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif,
				enum nl80211_iftype newtype,
				bool newp2p)
{
	struct nrc *nw = hw->priv;
	struct nrc_vif *i_vif = to_i_vif(vif);

	nrc_mac_dbg("%s %s addr:%pM hwindex:%d %s->%s", __func__,
			i_vif->dev ? netdev_name(i_vif->dev):"NULL",
			vif->addr, i_vif->index, iftype_string(vif->type),
			iftype_string(newtype));
	/*
	 * interface may change from non-AP to AP in
	 * which case this needs to be set up again
	 */
	vif->type = newtype;
	vif->p2p = newp2p;

#ifdef CONFIG_USE_TXQ
	nrc_cleanup_txq(nw, vif->txq);
#endif

	nrc_wim_set_mac_addr(nw, vif);
	force_sw_enc_mode_by_sta_type(nw, vif);
	nrc_wim_set_sta_type(nw, vif);

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	vif->cab_queue = 0;
#endif

	return 0;
}

static void nrc_mac_remove_interface(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;
	struct nrc_vif *i_vif = to_i_vif(vif);

	if (vif->type == NL80211_IFTYPE_MONITOR || vif->p2p)
		return;

	nrc_mac_dbg("%s %s addr:%pM hwindex:%d %s", __func__,
				i_vif->dev ? netdev_name(i_vif->dev) : "NULL",
				vif->addr, i_vif->index, iftype_string(vif->type));

#ifdef CONFIG_USE_TXQ
	nrc_cleanup_txq(nw, vif->txq);
#endif

#ifdef CONFIG_USE_SCAN_TIMEOUT
	cancel_delayed_work_sync(&i_vif->scan_timeout);
#endif
	if (!atomic_read(&nw->d_deauth.delayed_deauth)) {
		int ret = 0;
		ret = nrc_hif_check_target(nw->hif, 0x1B);
		if (ret != 1) {
			nrc_wim_unset_sta_type(nw, vif);
		}
		nrc_free_vif_index(hw->priv, vif);
		i_vif->dev = NULL;
	} else {
		nw->d_deauth.removed = true;
	}
	nrc_mac_dbg("%s:end", __func__);
}

static enum WIM_CHANNEL_PARAM_WIDTH
get_wim_channel_width(enum nl80211_chan_width width)
{
	switch (width) {
	case NL80211_CHAN_WIDTH_20_NOHT:
		return CH_WIDTH_20_NOHT;
	case NL80211_CHAN_WIDTH_20:
		return CH_WIDTH_20;
	case NL80211_CHAN_WIDTH_40:
		return CH_WIDTH_40;
	case NL80211_CHAN_WIDTH_80:
		return CH_WIDTH_80;
	case NL80211_CHAN_WIDTH_80P80:
		return CH_WIDTH_80P80;
	case NL80211_CHAN_WIDTH_160:
		return CH_WIDTH_160;
	case NL80211_CHAN_WIDTH_5:
		return CH_WIDTH_5;
	case NL80211_CHAN_WIDTH_10:
		return CH_WIDTH_10;
#if defined(CONFIG_S1G_CHANNEL)
	case NL80211_CHAN_WIDTH_1:
		return CH_WIDTH_1;
	case NL80211_CHAN_WIDTH_2:
		return CH_WIDTH_2;
	case NL80211_CHAN_WIDTH_4:
		return CH_WIDTH_4;
	case NL80211_CHAN_WIDTH_8:
		return CH_WIDTH_8;
	case NL80211_CHAN_WIDTH_16:
		return CH_WIDTH_16;
	default:
		return CH_WIDTH_1;
#else
	default:
		return CH_WIDTH_20;
#endif /* CONFIG_S1G_CHANNEL */
	}
}

#if !defined(CONFIG_SUPPORT_BD)
uint16_t get_base_freq(void)
{
	uint16_t ret_freq = 0;

	if (nrc_cc[0] == 'U' && nrc_cc[1] == 'S') {
		ret_freq = US_BASE_FREQ;
	} else if (nrc_cc[0] == 'J' && nrc_cc[1] == 'P') {
		ret_freq = JP_BASE_FREQ;
	} else if (nrc_cc[0] == 'T' && nrc_cc[1] == 'W') {
		ret_freq = TW_BASE_FREQ;
	} else if (nrc_cc[0] == 'A' && nrc_cc[1] == 'U') {
		ret_freq = AU_BASE_FREQ;
	} else if (nrc_cc[0] == 'N' && nrc_cc[1] == 'Z') {
		ret_freq = NZ_BASE_FREQ;
	} else if (nrc_cc[0] == 'E' && nrc_cc[1] == 'U') {
		ret_freq = EU_BASE_FREQ;
	} else if (nrc_cc[0] == 'C' && nrc_cc[1] == 'N') {
		ret_freq = CN_BASE_FREQ;
	} else if (nrc_cc[0] == 'K' && nrc_cc[1] == 'R') {
		ret_freq = (kr_band == 1) ? K1_BASE_FREQ : K2_BASE_FREQ;
	} else if (country_match(eu_countries_cc, nrc_cc)) {
		ret_freq = EU_BASE_FREQ;
	}
	return ret_freq;
}
#endif /* #if !defined(CONFIG_SUPPORT_BD) */

#ifdef CONFIG_S1G_CHANNEL
void init_s1g_channels(struct nrc *nw)
{
	struct ieee80211_channel *channels;
	struct sk_buff *skb;
	int i, freq, w;

	nrc_set_s1g_country(nrc_country_code);

	nrc_dbg(NRC_DBG_MAC, "%s:country table size:%d",
		__func__, sizeof(struct s1g_channel_table)*nrc_get_num_channels_by_current_country());
	skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, WIM_MAX_SIZE);
 	nrc_wim_skb_add_tlv(skb, WIM_TLV_CH_TABLE, sizeof(struct s1g_channel_table)*nrc_get_num_channels_by_current_country(), (struct s1g_channel_table *)nrc_get_current_s1g_cc_table());
	nrc_xmit_wim_request(nw, skb);

	nrc_dbg(NRC_DBG_MAC, "%s: Country: %s", __func__, (char *)nrc_get_current_s1g_country());
	skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, WIM_MAX_SIZE);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_COUNTRY_CODE, sizeof(u16), (char *)nrc_get_current_s1g_country());

	nrc_xmit_wim_request(nw, skb);
	channels = nrc_channels_s1ghz;

	for (i = 0; i < nrc_get_num_channels_by_current_country(); i++) {
		freq = nrc_get_s1g_freq_by_arr_idx(i);
		channels[i].band = NL80211_BAND_S1GHZ;
		channels[i].center_freq = freq / 10;
		channels[i].freq_offset = freq % 10 * 100;

		w = nrc_get_s1g_width_by_arr_idx(i);

		switch(w)
		{
			default:
			case 1:
				channels[i].flags = IEEE80211_CHAN_1MHZ;
				break;
			case 2:
				channels[i].flags = IEEE80211_CHAN_2MHZ;
				break;
			case 4:
				channels[i].flags = IEEE80211_CHAN_4MHZ;
				break;
		}
		channels[i].hw_value = i+1;
	}
}
#endif /* CONFIG_S1G_CHANNEL */

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
void nrc_mac_add_tlv_channel(struct sk_buff *skb,
					struct cfg80211_chan_def *chandef)
{
#if !defined(CONFIG_S1G_CHANNEL)
	enum nl80211_channel_type ch_type;
	struct wim_channel_param ch_param;

	ch_type = cfg80211_get_chandef_type(chandef);

	/*temporarily resolved host system freezing */
	if (WARN_ON(!chandef->chan))
		return;

	ch_param.channel = chandef->chan->center_freq;
	ch_param.type = ch_type;
	ch_param.width = get_wim_channel_width(chandef->width);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_CHANNEL,
				   sizeof(ch_param), &ch_param);
#else
	struct s1g_channel_table param;
	static char * wim_s1g_alpha2;
	wim_s1g_alpha2 = nrc_get_current_s1g_country();

	param.alpha2[0] = wim_s1g_alpha2[0];
	param.alpha2[1] = wim_s1g_alpha2[1];
	param.alpha2[2] = wim_s1g_alpha2[2];
	param.s1g_freq = FREQ_TO_100KHZ(chandef->chan->center_freq,
				chandef->chan->freq_offset);
	param.s1g_freq_index = nrc_get_channel_idx_by_freq(param.s1g_freq);
	param.cca_level_type = nrc_get_cca_by_freq(param.s1g_freq);
	nrc_s1g_set_channel_bw(param.s1g_freq, chandef);
	param.chan_spacing = get_wim_channel_width(chandef->width);
	param.global_oper_class = nrc_get_oper_class_by_freq(param.s1g_freq);
	param.offset = nrc_get_offset_by_freq(param.s1g_freq);
	param.primary_loc = nrc_get_pri_loc_by_freq(param.s1g_freq);

	nrc_wim_skb_add_tlv(skb, WIM_TLV_S1G_CHANNEL, sizeof(param), &param);
#endif /* CONFIG_S1G_CHANNEL */
}
#else
void nrc_mac_add_tlv_channel(struct sk_buff *skb,
			struct ieee80211_conf *chandef)
{
	enum nl80211_channel_type ch_type = chandef->channel_type;
	struct wim_channel_param ch_param;

	ch_param.channel = chandef->channel->center_freq;
	ch_param.type = ch_type;
	if (ch_type >= NL80211_CHAN_HT40MINUS)
		ch_param.width = NL80211_CHAN_HT40MINUS;
	else
		ch_param.width = ch_type;

	nrc_wim_skb_add_tlv(skb, WIM_TLV_CHANNEL,
				sizeof(ch_param), &ch_param);
}
#endif /* CONFIG_SUPPORT_CHANNEL_INFO */

static int nrc_mac_config(struct ieee80211_hw *hw, u32 changed)
{
	struct nrc *nw = hw->priv;
	struct wim_pm_param *p;
	struct sk_buff *skb;
	int ret = 0;
	struct ieee80211_channel ch = {0,};
#if defined(CONFIG_SUPPORT_BD)
	int i;
	bool supp_ch_flag = false;
#endif /* defined(CONFIG_SUPPORT_BD) */
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	struct cfg80211_chan_def chandef = {0,};
	memcpy(&chandef, &hw->conf.chandef, sizeof(struct cfg80211_chan_def));
	memcpy(&ch, hw->conf.chandef.chan, sizeof(struct ieee80211_channel));
	chandef.chan = &ch;
#else
	struct ieee80211_conf chandef = {0,};
	memcpy(&chandef, &hw->conf, sizeof(struct ieee80211_conf));
	memcpy(&ch, hw->conf.channel, sizeof(struct ieee80211_channel));
	chandef.channel = &ch;
#endif

#if defined(CONFIG_SUPPORT_BD)
	if (g_supp_ch_list.num_ch) {
		if (changed & IEEE80211_CONF_CHANGE_CHANNEL) {
			for(i=0; i < g_supp_ch_list.num_ch; i++) {
#ifdef CONFIG_S1G_CHANNEL
				if(g_supp_ch_list.nons1g_ch_freq[i] ==
					(hw->conf.chandef.center_freq1)*10 + (hw->conf.chandef.freq1_offset)/100 ) {
					supp_ch_flag = true;
					break;
				}
#else
				if(g_supp_ch_list.nons1g_ch_freq[i] ==
					hw->conf.chandef.chan->center_freq) {
					supp_ch_flag = true;
					break;
				}
#endif
			}
			if (!supp_ch_flag && nw->alpha2[0] != 'U' && nw->alpha2[1] != 'S' &&
				hw->conf.chandef.chan->center_freq == 2412) {
				supp_ch_flag = true;
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
				chandef.chan->center_freq = g_supp_ch_list.nons1g_ch_freq[0];
#else
				chandef.channel->center_freq = g_supp_ch_list.nons1g_ch_freq[0];
#endif /* CONFIG_SUPPORT_CHANNEL_INFO */
			}
			if(!supp_ch_flag) {
				nrc_mac_dbg("%s: Not supported channel %u",
					__func__, hw->conf.chandef.chan->center_freq);
				return -EINVAL;
			}
		}
	}
#else
	if (nw->alpha2[0] != 'U' && nw->alpha2[1] != 'S' &&
			hw->conf.chandef.chan->center_freq == 2412) {
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
		chandef.chan->center_freq = get_base_freq();
#else
		chandef.channel->center_freq = get_base_freq();
#endif /* CONFIG_SUPPORT_CHANNEL_INFO */
	}
#endif /* CONFIG_SUPPORT_BD */

	if (changed & IEEE80211_CONF_CHANGE_CHANNEL) {
		skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET,
				tlv_len(sizeof(struct wim_channel_param)));

		/* TODO: Remove the following line */
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
		nw->band = hw->conf.chandef.chan->band;
		nw->center_freq = hw->conf.chandef.chan->center_freq;
#else
		nw->band = hw->conf.channel->band;
		nw->center_freq = hw->conf.channel->center_freq;
#endif

		if (!atomic_read(&nw->d_deauth.delayed_deauth)) {
#ifdef CONFIG_S1G_CHANNEL
			init_s1g_channels(nw);
#endif /* #ifdef CONFIG_S1G_CHANNEL */
			nrc_mac_add_tlv_channel(skb, &chandef);
			ret = nrc_xmit_wim_request(nw, skb);
		} else {
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
			memcpy(&nw->d_deauth.c, &hw->conf.chandef, sizeof(struct cfg80211_chan_def));
			memcpy(&nw->d_deauth.ch, hw->conf.chandef.chan, sizeof(struct ieee80211_channel));
			nw->d_deauth.c.chan = &nw->d_deauth.ch;
#else
			memcpy(&nw->d_deauth.c, &hw->conf, sizeof(struct ieee80211_conf));
#endif
		}
		/* TODO: band (2G, 5G, etc) and bandwidth (20MHz, 40MHz, etc) */
	}

	if (changed & IEEE80211_CONF_CHANGE_PS) {
		nw->ps_enabled = (hw->conf.flags & IEEE80211_CONF_PS);
		nw->ps_drv_state = nw->ps_enabled;

		nrc_common_dbg("[%s,L%d] drv_state:%d ps_enable:%d dynamic_ps_timeout:%d\n",
			__func__, __LINE__,
			nw->drv_state, nw->ps_enabled, hw->conf.dynamic_ps_timeout);

		if (nw->drv_state == NRC_DRV_PS || nw->ps_modem_enabled) {
			/**
			 * [NRC_PS_DEEPSLEEP_* ONLY]
			 * if the current state is already NRC_DRV_PS,
			 * there's nothing to do in here even if mac80211 notifies wake-up.
			 * the actual action to wake up for target will be done by
			 * nrc_wake_tx_queue() with changing gpio signal.
			 * (when driver receives a data frame.)
			 */
			if (nw->ps_enabled)
				nrc_ps_dbg("Target is already in deepsleep...\n");
			else
				nrc_ps_dbg("Target will wake-up after frame type check.\n");

			return ret;
		}

		if (ieee80211_hw_check(hw, SUPPORTS_DYNAMIC_PS)) {
			if (hw->conf.dynamic_ps_timeout > 0) {
				if (nw->ps_enabled) {
					mod_timer(&nw->dynamic_ps_timer,
						jiffies + msecs_to_jiffies(hw->conf.dynamic_ps_timeout));
					return ret;
				} else {
					if (power_save >= NRC_PS_DEEPSLEEP_TIM)
						return ret;
				}
			} else {
				return ret;
			}
		} else if (ieee80211_hw_check(hw, SUPPORTS_PS)) {
			if (!nw->ps_enabled) {
				if (power_save >= NRC_PS_DEEPSLEEP_TIM)
					return ret;
			}
		}

		if (nw->ps_enabled && power_save > NRC_PS_NONE) {
			skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET,
				tlv_len(sizeof(struct wim_pm_param)));
			p = nrc_wim_skb_add_tlv(skb, WIM_TLV_PS_ENABLE,
					sizeof(struct wim_pm_param), NULL);
			memset(p, 0, sizeof(struct wim_pm_param));
			p->ps_mode = power_save;
			p->ps_enable = nw->ps_enabled;
			p->ps_timeout = hw->conf.dynamic_ps_timeout;

			if (power_save >= NRC_PS_DEEPSLEEP_TIM) {
				p->ps_wakeup_pin = power_save_gpio[1];
				p->ps_duration = sleep_duration[0] * (sleep_duration[1] ? 1000 : 1);
				ieee80211_stop_queues(nw->hw);
#ifdef CONFIG_USE_TXQ
				nrc_cleanup_txq_all(nw);
#endif
				nrc_ps_dbg("Enter DEEPSLEEP1!!!\n");
				nrc_ps_dbg("sleep_duration: %lld ms\n", p->ps_duration);

			} else {
				nrc_ps_dbg("[%s] %s MODEMSLEEP...\n", __func__,
						(nw->ps_enabled ? "Enter" : "Wakeup"));
			}

			nrc_hif_sleep_target_start(nw->hif, power_save);

			ret = nrc_xmit_wim_request(nw, skb);

			if (nw->ps_enabled) {
				if (power_save == NRC_PS_MODEMSLEEP) {
					nw->ps_modem_enabled = true;
				} else if (power_save >= NRC_PS_DEEPSLEEP_TIM) {
					nrc_hif_suspend(nw->hif);
#if 0 /* Not work to call ieee80211_stop_queues */
					nrc_ps_dbg("sleep before\n");
					usleep_range(200 * 1000, 300 * 1000);
					nrc_ps_dbg("sleep after\n");
#endif
					ieee80211_wake_queues(nw->hw);
				}
			}
			nrc_hif_sleep_target_end(nw->hif, power_save);
		}
	}

	return ret;
}

static void nrc_mac_configure_filter(struct ieee80211_hw *hw,
									unsigned int changed_flags,
					 				unsigned int *total_flags,
									u64 multicast)
{

	*total_flags &= NRC_CONFIGURE_FILTERS;

	/* TODO: talk to target */
}
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static void nrc_mac_update_p2p_ps(struct sk_buff *skb,
				  struct ieee80211_vif *vif)
{
	int i = 0;
	u8 ctwin;
	struct ieee80211_p2p_noa_attr *noa = &vif->bss_conf.p2p_noa_attr;
	struct wim_noa_param *p;

	if (!vif->p2p)
		return;

	if (noa->oppps_ctwindow & IEEE80211_P2P_OPPPS_ENABLE_BIT) {
		ctwin = cpu_to_le32(noa->oppps_ctwindow);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_P2P_OPPPS, sizeof(u8),
				&ctwin);
		nrc_mac_dbg("%s: OppPS, ctwindow(%d)", __func__, ctwin);
	}

	for (i = 0; i < IEEE80211_P2P_NOA_DESC_MAX; i++) {
		const struct ieee80211_p2p_noa_desc *desc = &noa->desc[i];

		if (!desc->count || !desc->duration)
			continue;

		p = nrc_wim_skb_add_tlv(skb, WIM_TLV_P2P_NOA, sizeof(*p),
					NULL);

		p->index = i;
		p->count = desc->count;
		p->start_time = le32_to_cpu(desc->start_time);
		p->interval = le32_to_cpu(desc->interval);
		p->duration = le32_to_cpu(desc->duration);

		nrc_mac_dbg(
			"%s(%d/%d): NoA cnt: %d,st: %d, dur: %d, intv: %d",
			__func__, noa->index, i, p->count, p->start_time,
			p->duration, p->interval);
	}
}
#endif

#define BSS_CHANGED_ERP						\
	(BSS_CHANGED_ERP_CTS_PROT | BSS_CHANGED_ERP_PREAMBLE |	\
	 BSS_CHANGED_ERP_SLOT)

/* S1G Short Beacon Interval (in TU) */
#define DEF_CFG_S1G_SHORT_BEACON_COUNT	10

void nrc_mac_bss_info_changed(struct ieee80211_hw *hw,
							struct ieee80211_vif *vif,
							struct ieee80211_bss_conf *info,
#if KERNEL_VERSION(6,0,0) <= NRC_TARGET_KERNEL_VERSION
							u64 changed)
#else
							u32 changed)
#endif
{
	struct nrc_vif *i_vif = to_i_vif(vif);
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
	int ret;

	//nrc_mac_dbg("%s: changed=0x%x", __func__, changed);

	/*
	 * When sending deauth frame while the target is on deep sleep mode,
	 * mac80211 operates as follows.
	 * After transferring deauth frame through ieee80211_send_deauth_disassoc,
	 * change assoc to false, and notify disassoc through bss_info_changed.
	 *
	 * In this case, since the assoc information is set to true
	 * in the delayed deauth frame, the information of the delayed deauth frame
	 * is also should be updated through the processing below.
	 */
	if (nw->drv_state == NRC_DRV_PS) {
		if (changed == 0x80309f && atomic_read(&nw->d_deauth.delayed_deauth)) {
#ifdef CONFIG_USE_VIF_CFG
			memcpy(&nw->d_deauth.v, vif, sizeof(struct ieee80211_vif));
#endif
			memcpy(&nw->d_deauth.b, info, sizeof(struct ieee80211_bss_conf));
		}

		return;
	}

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);

	if (changed & BSS_CHANGED_ASSOC) {
#ifdef CONFIG_USE_VIF_CFG
		if (vif->cfg.assoc) {
#else
		if (info->assoc) {
#endif
			nrc_bss_assoc(hw, vif, info, skb);

			spin_lock_bh(&nw->vif_lock);
			nw->associated_vif = vif;
			if (!disable_cqm) {
				nrc_mac_dbg("mod_timer in %s:%d", __FUNCTION__, __LINE__);
				mod_timer(&nw->bcn_mon_timer,
					jiffies + msecs_to_jiffies(nw->beacon_timeout));
			}
			spin_unlock_bh(&nw->vif_lock);
		} else {
			spin_lock_bh(&nw->vif_lock);
			if (!disable_cqm) {
				nw->beacon_timeout = 0;
				nrc_mac_dbg("del_timer in %s:%d", __FUNCTION__, __LINE__);
				try_to_del_timer_sync(&nw->bcn_mon_timer);
			}
			nw->associated_vif = NULL;
			spin_unlock_bh(&nw->vif_lock);
		}
		nrc_mac_dbg("[BSS_CHANGED_ASSOC] associated_vif:%d beacon_timeout:%lu",
			nw->associated_vif?i_vif->index:-1, nw->beacon_timeout);
	}
#ifndef CONFIG_S1G_CHANNEL
	if (changed & BSS_CHANGED_BASIC_RATES) {
		nrc_mac_dbg("basic_rate: %08x", info->basic_rates);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_BASIC_RATE,
				sizeof(info->basic_rates), &info->basic_rates);
	}

	if (changed & BSS_CHANGED_HT) {
		nrc_mac_dbg("ht: %08x", info->ht_operation_mode);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_HT_MODE,
							sizeof(info->ht_operation_mode),
							&info->ht_operation_mode);
	}
#endif
	if (changed & BSS_CHANGED_BSSID) {
		nrc_mac_dbg("bssid=%pM", info->bssid);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_BSSID, ETH_ALEN,
							(void *)info->bssid);
	}

	if (changed & BSS_CHANGED_BEACON_INT ||
		changed & BSS_CHANGED_BEACON_ENABLED) {
		/**
		 * BSS_CHANGED_BEACON_ENABLED flag is used only for AP and MESH.
		 */
		u16 bi, short_bi = 0;
		u8 dtim_period = info->dtim_period;

		nrc_mac_dbg("beacon: %s, interval=%u, dtim_period:%u",
				info->enable_beacon ? "enabled" : "disabled",
				info->beacon_int,
				info->dtim_period);

		nw->beacon_int = bi = info->beacon_int;
		if (!disable_cqm) {
			if (vif->type == NL80211_IFTYPE_STATION) {
				nw->beacon_timeout = beacon_loss_count * bi;
				nrc_mac_dbg("[BSS_CHANGED_BEACON_INT] assoc:%d beacon_timeout:%lu",
					nw->associated_vif?i_vif->index:-1, nw->beacon_timeout);
			}
		}
		if ((vif->type == NL80211_IFTYPE_AP || (vif->type == NL80211_IFTYPE_MESH_POINT)
#if defined (CONFIG_SUPPORT_IBSS)
			|| (vif->type == NL80211_IFTYPE_ADHOC)
#endif
		) && nrc_mac_is_s1g(nw) && enable_short_bi) {
			/**
			 * AP/MESH : When 'enable_short_bi' is true(AP will send S1G beacons with both(full/minimum) set),
			 *			the Short Beacon Interval is 'beacon_int' value in hostap/wpa_supplicant conf. file
			 *			and the Beacon Interval will be 'Short Beacon Interval * 10'.
			 *			Otherwise, 'beacon_int' is used for the value of Beacon Interval and
			 *			the Short Beacon Interval is 0.
			 *     STA : Basically, the beacon_int from mac80211 will be treated as the Beacon Interval in STA.
			 *			and the Short Beacon Interval value will be extracted from beacons directly in the target
			 *			after established the Wi-Fi connection.
			 */
			short_bi = info->beacon_int;
			/* beacon interval is 16-bit length */
			if (DEF_CFG_S1G_SHORT_BEACON_COUNT * short_bi <= 65535) {
				/* default ratio is 10 */
				bi = DEF_CFG_S1G_SHORT_BEACON_COUNT * short_bi;
			} else {
				/* adjust ratio of s1g bcn and s1g short bcn */
				bi = (65535 / short_bi) * short_bi;
			}
		}

		nrc_wim_skb_add_tlv(skb, WIM_TLV_BCN_INTV, sizeof(bi), &bi);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_SHORT_BCN_INTV,
					sizeof(short_bi), &short_bi);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_DTIM_PERIOD,
				sizeof(dtim_period), &dtim_period);

#if defined(CONFIG_SUPPORT_BEACON_BYPASS)
		if(enable_beacon_bypass) {
			uint8_t enable = (uint8_t)enable_beacon_bypass;
			nrc_wim_skb_add_tlv(skb, WIM_TLV_BEACON_BYPASS, sizeof(u8), &enable);
		}
#endif /* CONFIG_SUPPORT_BEACON_BYPASS */

		if (changed & BSS_CHANGED_BEACON_ENABLED) {
			nrc_wim_skb_add_tlv(skb, WIM_TLV_BEACON_ENABLE,
						sizeof(info->enable_beacon),
						&info->enable_beacon);
		}
	}

	if (changed & BSS_CHANGED_BEACON)
		nrc_vendor_update_beacon(hw, vif);

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	if (changed & BSS_CHANGED_SSID) {
#ifdef CONFIG_USE_VIF_CFG
		nrc_mac_dbg("ssid=%s", vif->cfg.ssid);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_SSID, vif->cfg.ssid_len,
				vif->cfg.ssid);
#else
		nrc_mac_dbg("ssid=%s", info->ssid);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_SSID, info->ssid_len,
				info->ssid);
#endif
	}
#endif

	if (changed & BSS_CHANGED_ERP) {
		struct wim_erp_param *p;

		p = nrc_wim_skb_add_tlv(skb, WIM_TLV_ERP_PARAM, sizeof(*p),
					NULL);
		p->use_11b_protection = info->use_cts_prot;
		p->use_short_preamble = info->use_short_preamble;
		p->use_short_slot = info->use_short_slot;
	}

	/* TODO: implement! */
	if (changed & BSS_CHANGED_CQM)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_CQM");
	if (changed & BSS_CHANGED_IBSS)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_IBSS");
	if (changed & BSS_CHANGED_ARP_FILTER)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_ARP_FILTER");
#if 0
	if (changed & BSS_CHANGED_QOS)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_QOS");
#endif
	if (changed & BSS_CHANGED_IDLE)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_IDLE");
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	if (changed & BSS_CHANGED_TXPOWER) {
		int txpower = info->txpower;
		uint16_t txpower_type = info->txpower_type;
		nrc_common_dbg("%s(changed:%s[PW=%d TYPE=%s])", __func__,
			"BSS_CHANGED_TXPOWER", txpower,
			txpower_type == TXPWR_LIMIT ? "limit" : txpower_type ? "fixed" : "auto");
#ifdef CONFIG_SUPPORT_IW_IWCONFIG_TXPWR
#if 0 //def CONFIG_SUPPORT_BD
		/**
		 * cli_app cannot be used on openWRT system. so use "iw phy nrc80211 set txpower"
		 * But, txpower could be limited smaller than expected by user in this cse
		 * since the max value of txpower via iw command should comply the regulations.
		 * So, it's necessary to change it forcibly in here when txpower type is limit(1) or auto(0).
		 * Then, the actual txpower will be assigned as the value in bd file finally.
		 */
		if (txpower_type < TXPWR_FIXED && txpower < 24) {
			nrc_common_dbg("%s max txpower was changed (%d -> 24)",__func__, txpower);
			txpower = 24;
		}
#endif /* CONFIG_SUPPORT_BD */
		if (txpower < 1 || txpower > 30) {
			nrc_common_dbg("%s invalid txpowr (%d)", __func__, txpower);
		} else {
			u32 p = txpower_type;
			p = (p << 16) | txpower;
			nrc_wim_skb_add_tlv(skb, WIM_TLV_SET_TXPOWER, sizeof(u32), &p);
		}
#endif /* CONFIG_SUPPORT_IW_IWCONFIG_TXPWR */
	}
	if (changed & BSS_CHANGED_P2P_PS) {
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_P2P_PS");
		nrc_mac_update_p2p_ps(skb, vif);
	}
	if (changed & BSS_CHANGED_BEACON_INFO) {
		/*
		 * dtim_period for STA is transferred by this.
		 * but, the target is getting this info from TIM ie in beacon.
		 */
		nrc_mac_dbg("%s(changed:%s) dtim_period:%u", __func__,
				"BSS_CHANGED_BEACON_INFO", info->dtim_period);
	}
	if (changed & BSS_CHANGED_BANDWIDTH)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_BANDWIDTH");
#ifdef CONFIG_SUPPORT_IFTYPE_OCB
	if (changed & BSS_CHANGED_OCB)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_OCB");
#endif
#endif

	if (skb->len > sizeof(struct wim)) {
		ret = nrc_xmit_wim_request(nw, skb);
		if (ret < 0){
			nrc_mac_dbg("failed to transmit a wim request");
		}
	} else {
		dev_kfree_skb(skb);
	}
}

static
int nrc_mac_sta_add(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
					struct ieee80211_sta *sta)
{
	struct nrc *nw = hw->priv;
#ifdef CONFIG_USE_TXQ
	int i;
#endif

	nrc_stats_add(sta->addr, 16);
	//nrc_stats_print();
	nrc_wim_change_sta(nw, vif, sta, WIM_STA_CMD_ADD, 0);

#ifdef CONFIG_USE_TXQ
	/* Initialize txq driver data.
	 * Assumptions are:
	 * (1) per-sta per-tid txq is not in use up to this point (associated).
	 * (2) EAPOL frames do not resort to txq.
	 */
	for (i = 0; i < ARRAY_SIZE(sta->txq); i++)
		nrc_init_txq(sta->txq[i], vif, sta);

	/* Set higher timeout of mac80211 TX Queue */
	if (nrc_mac_is_s1g(nw)) {
		rate_control_set_rates(hw, sta, NULL);
	}
#endif
	return 0;
}

int nrc_mac_sta_remove(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
						struct ieee80211_sta *sta)
{
	nrc_stats_del(sta->addr);
	//nrc_stats_print();
	nrc_wim_change_sta(hw->priv, vif, sta, WIM_STA_CMD_REMOVE, 0);

	return 0;
}

static void nrc_init_sta_ba_session(struct ieee80211_sta *sta)
{
	struct nrc_sta *i_sta = NULL;
	int i;

	if (sta) {
		i_sta = to_i_sta(sta);
		if (i_sta) {
			for (i=0 ; i < NRC_MAX_TID; i++) {
				i_sta->tx_ba_session[i] = IEEE80211_BA_NONE;
				i_sta->ba_req_last_jiffies[i] = 0;
				i_sta->rx_ba_session[i].started = false;
				i_sta->rx_ba_session[i].sn = 0;
#if KERNEL_VERSION(4, 19, 0) <= NRC_TARGET_KERNEL_VERSION
				i_sta->rx_ba_session[i].buf_size = IEEE80211_MAX_AMPDU_BUF_HT;
#else
				i_sta->rx_ba_session[i].buf_size = IEEE80211_MAX_AMPDU_BUF;
#endif
			}
		}
	}
}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static
int nrc_wim_change_sta_state(struct nrc *nw, struct ieee80211_vif *vif,
		struct ieee80211_sta *sta, int new_state)
{
	int state = WIM_STA_CMD_STATE_NOTEXIST;

	switch (new_state) {
		case IEEE80211_STA_NOTEXIST:
			state = WIM_STA_CMD_STATE_NOTEXIST;
			break;
		case IEEE80211_STA_NONE:
			state = WIM_STA_CMD_STATE_NONE;
			break;
		case IEEE80211_STA_AUTH:
			state = WIM_STA_CMD_STATE_AUTH;
			break;
		case IEEE80211_STA_ASSOC:
			state = WIM_STA_CMD_STATE_ASSOC;
			break;
		case IEEE80211_STA_AUTHORIZED:
			state = WIM_STA_CMD_STATE_AUTHORIZED;

			if (ampdu_mode == NRC_AMPDU_DISABLE) {
				nrc_dbg(NRC_DBG_STATE, "%s: AMPDU is disabled", __func__);
					nw->ampdu_supported = false;
					nw->ampdu_reject = true;
					nw->ampdu_started = false;
			} else {
				nrc_dbg(NRC_DBG_STATE, "%s: AMPDU is ready with peer (%pM)",
					__func__, sta->addr);
				nrc_init_sta_ba_session(sta);
#ifdef CONFIG_S1G_CHANNEL
				sta->ht_cap.ht_supported=true;
#endif /* #ifdef CONFIG_S1G_CHANNEL */
				nw->ampdu_supported = true;
				nw->ampdu_reject = false;
				nw->ampdu_started = true;
			}
			break;
	}

	return nrc_wim_change_sta(nw, vif, sta, state, 0);
}
#endif

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static
int nrc_mac_sta_state(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
					struct ieee80211_sta *sta,
					enum ieee80211_sta_state old_state,
					enum ieee80211_sta_state new_state)
{
	struct nrc_vif *i_vif = to_i_vif(vif);
	struct nrc_sta *i_sta = to_i_sta(sta);
	unsigned long flags;
	struct nrc_sta_handler *h;
	struct nrc *nw = (struct nrc*)hw->priv;

	nrc_dbg(NRC_DBG_STATE, "%s: sta:%pM, %d->%d", __func__, sta->addr,
		old_state, new_state);

	i_sta->state = new_state;

#define state_changed(old, new) \
(old_state == IEEE80211_STA_##old && new_state == IEEE80211_STA_##new)

	if (state_changed(NOTEXIST, NONE)) {

		memset(i_sta, 0, sizeof(*i_sta));
		i_sta->nw = nw;
		i_sta->vif = vif;

		INIT_LIST_HEAD(&i_sta->list);

		spin_lock_irqsave(&i_vif->preassoc_sta_lock, flags);
		list_add_tail(&i_sta->list, &i_vif->preassoc_sta_list);
		spin_unlock_irqrestore(&i_vif->preassoc_sta_lock, flags);


		/* This value is set in rx_h_bss_max_idle_period normally when assoc frames are received 
		   but, While test of ifconfig up /down, some weired transition occured without any frame exchanges.
		   so, values is set to zero. this causes invalid timer setting on STA.
		*/

		/* set default max_idle_period from AP */
		nrc_mac_dbg("%s: set default max_idle_period with AP's period: 0x%x", __FUNCTION__, i_vif->max_idle_period);
		i_sta->max_idle.period = i_vif->max_idle_period;
		i_sta->max_idle.options = 0;

	} else if (state_changed(NONE, NOTEXIST)) {
		spin_lock_irqsave(&i_vif->preassoc_sta_lock, flags);
		list_del_init(&i_sta->list);
		spin_unlock_irqrestore(&i_vif->preassoc_sta_lock, flags);
	} else if (state_changed(NONE, AUTH)) {

	} else if (state_changed(AUTH, ASSOC)) {
		i_sta->vif = vif;
		nrc_mac_sta_add(hw, vif, sta);

	} else if (state_changed(ASSOC, AUTH)) {
#ifdef CONFIG_USE_TXQ
		int i;
		for (i = 0; i < ARRAY_SIZE(sta->txq); i++) {
			nrc_cleanup_txq(nw, sta->txq[i]);
		}
#endif
		if (!atomic_read(&nw->d_deauth.delayed_deauth)) {
			nrc_mac_sta_remove(hw, vif, sta);
                }
	}

	if (!atomic_read(&nw->d_deauth.delayed_deauth))
		nrc_wim_change_sta_state(nw, vif, sta, new_state);

	for (h = &__sta_h_start; h < &__sta_h_end; h++)
		h->sta_state(hw, vif, sta, old_state, new_state);

	return 0;
}
#endif

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static void nrc_mac_sta_pre_rcu_remove(struct ieee80211_hw *hw,
									struct ieee80211_vif *vif,
									struct ieee80211_sta *sta)
{
	struct nrc_vif *i_vif = to_i_vif(vif);
	struct nrc_sta *i_sta = to_i_sta(sta);
	unsigned long flags;

	spin_lock_irqsave(&i_vif->preassoc_sta_lock, flags);
	list_del_init(&i_sta->list);
	spin_unlock_irqrestore(&i_vif->preassoc_sta_lock, flags);
}
#endif


static void nrc_mac_sta_notify(struct ieee80211_hw *hw,
							struct ieee80211_vif *vif,
							enum sta_notify_cmd cmd,
							struct ieee80211_sta *sta)
{
	nrc_wim_change_sta(hw->priv, vif, sta, WIM_STA_CMD_NOTIFY,
			   cmd == STA_NOTIFY_SLEEP);
}


static int nrc_mac_set_tim(struct ieee80211_hw *hw, struct ieee80211_sta *sta,
			   bool set)
{
	struct nrc *nw = hw->priv;
	struct nrc_sta *i_sta = to_i_sta(sta);
	struct sk_buff *skb;
	struct wim_tim_param *tim;

	if (WARN_ON(i_sta->vif == NULL))
		return -EINVAL;

	skb = nrc_wim_alloc_skb_vif(nw, i_sta->vif, WIM_CMD_SET,
					WIM_MAX_SIZE);

	tim = nrc_wim_skb_add_tlv(skb, WIM_TLV_TIM_PARAM, sizeof(*tim), NULL);
	tim->aid = sta->aid;
	tim->set = set;

	if (!nrc_mac_is_s1g(nw)) {
#ifdef CONFIG_USE_LINK_ID
		struct sk_buff *b = ieee80211_beacon_get_template(hw,
				i_sta->vif, NULL, i_sta->vif->bss_conf.link_id);
#else
		struct sk_buff *b = ieee80211_beacon_get_template(hw,
				i_sta->vif, NULL);
#endif
		if (b) {
			nrc_wim_skb_add_tlv(skb, WIM_TLV_BEACON, b->len,
					b->data);
			dev_kfree_skb_any(b);
		}
	}

	return nrc_xmit_wim_request(nw, skb);
}

static u16 mac80211_to_nrc_aci_map[4] = {
	3, 2, 1, 0};

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
int nrc_mac_conf_tx(struct ieee80211_hw *hw,
					struct ieee80211_vif *vif,
#ifdef CONFIG_USE_LINK_ID
					unsigned int link_id,
#endif
					u16 ac,
					const struct ieee80211_tx_queue_params *params)
#else
int nrc_mac_conf_tx(struct ieee80211_hw *hw,
					u16 ac,
					const struct ieee80211_tx_queue_params *params)
#endif
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
	static struct wim_tx_queue_param tqp[NRC_QUEUE_MAX];
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#else
	struct ieee80211_vif *vif = nw->vif[0];
#endif

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#if 0 /* ac is aci, target use aci for setting */ /* but IEEE80211_AC_VO is zero in mac80211, so need to reverse */
	ac = vif->hw_queue[ac];
#else
	BUG_ON(ac < IEEE80211_AC_VO || ac > IEEE80211_AC_BK);
	ac = mac80211_to_nrc_aci_map[ac];
#endif
#endif

	if (atomic_read(&nw->d_deauth.delayed_deauth))
		memcpy(&nw->d_deauth.tqp[ac], params, sizeof(struct ieee80211_tx_queue_params));

	if (nw->drv_state >= NRC_DRV_RUNNING) {
		if (tqp[ac].txop != params->txop || tqp[ac].cw_min != params->cw_min ||
			tqp[ac].cw_max != params->cw_max || tqp[ac].aifsn != params->aifs ||
			tqp[ac].uapsd != params->uapsd || tqp[ac].ac != ac) {
			skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, tlv_len(sizeof(struct wim_tx_queue_param)));
			tqp[ac].ac = ac;
			tqp[ac].txop = params->txop;
			tqp[ac].cw_min = params->cw_min;
			tqp[ac].cw_max = params->cw_max;
			tqp[ac].aifsn = params->aifs;
			tqp[ac].uapsd = params->uapsd;
			nrc_wim_skb_add_tlv(skb, WIM_TLV_TXQ_PARAM, sizeof(struct wim_tx_queue_param), &tqp[ac]);

#if 0
			nrc_mac_dbg("%s: ac=%d txop=%d cw_min=%d cw_max=%d aifs=%d uapsd=%d", __func__,
				ac, params->txop, params->cw_min, params->cw_max, params->aifs, params->uapsd);
#endif

			return nrc_xmit_wim_request(nw, skb);
		}
	}
	return 0;
}

static int nrc_mac_get_survey(struct ieee80211_hw *hw, int idx,
							struct survey_info *survey)
{
	struct stats_channel_noise *channel_noise;

	nrc_mac_dbg("%s (idx=%d)", __func__, idx);

	channel_noise = nrc_stats_channel_noise_report(idx, 0);
	if(!channel_noise)
		return -ENOENT;

	survey->channel = channel_noise->chan;

	/*
	 * Magically conjured noise level
	 * --- this is only ok for simulated hardware.
	 *
	 * A real driver which cannot determine the real channel noise MUST NOT
	 * report any noise, especially not a magically conjured one :-)
	 */
	survey->filled = SURVEY_INFO_NOISE_DBM;
	survey->noise = channel_noise->noise;

	return 0;
}

#ifdef CONFIG_USE_IEEE80211_AMPDU_PARAMS
static int nrc_mac_ampdu_action(struct ieee80211_hw *hw,
								struct ieee80211_vif *vif,
								struct ieee80211_ampdu_params *params)
#else
#ifdef CONFIG_SUPPORT_NEW_AMPDU_ACTION
static int nrc_mac_ampdu_action(struct ieee80211_hw *hw,
								struct ieee80211_vif *vif,
								enum ieee80211_ampdu_mlme_action action,
								struct ieee80211_sta *sta,
								u16 tid,
								u16 *ssn,
								u8 buf_size,
								bool amsdu)
#else
static int nrc_mac_ampdu_action(struct ieee80211_hw *hw,
								struct ieee80211_vif *vif,
								enum ieee80211_ampdu_mlme_action action,
								struct ieee80211_sta *sta,
								u16 tid,
								u16 *ssn,
								u8 buf_size)
#endif
#endif
{
	struct nrc *nw = hw->priv;
	struct nrc_sta *i_sta = NULL;

#ifdef CONFIG_USE_IEEE80211_AMPDU_PARAMS
	enum ieee80211_ampdu_mlme_action action = params->action;
	struct ieee80211_sta *sta = params->sta;
	u16 tid = params->tid;
	u16 *ssn = &params->ssn;
	u16 buf_size = params->buf_size;
#endif

	if (nw->ampdu_supported && !nw->ampdu_started) {
		if (ieee80211_start_tx_ba_session(sta, 0, 0) < 0) {
			nrc_dbg(NRC_DBG_MAC, "%s: can't start ampdu", __func__);
			return -EOPNOTSUPP;
		} else {
			nw->ampdu_started = true;
		}
	}

	if (!sta || tid >= NRC_MAX_TID) {
		nrc_dbg(NRC_DBG_MAC, "%s: sta is NULL", __func__);
		return -EOPNOTSUPP;
	}

	i_sta = to_i_sta(sta);
	nrc_dbg(NRC_DBG_MAC, "%s: peer MAC(%pM) TID(%d)", __func__, sta->addr, tid);

	switch (action) {
	case IEEE80211_AMPDU_TX_START:
		nrc_dbg(NRC_DBG_MAC, "%s: IEEE80211_AMPDU_TX_START", __func__);
#ifdef CONFIG_SUPPORT_LINK_STA
		if (!nw->ampdu_supported || !sta->deflink.ht_cap.ht_supported)
#else
		if (!nw->ampdu_supported || !sta->ht_cap.ht_supported)
#endif
			return -EOPNOTSUPP;

		if (nrc_wim_ampdu_action(nw, vif, WIM_AMPDU_TX_START, sta, tid))
			return -EOPNOTSUPP;

		i_sta->tx_ba_session[tid] = IEEE80211_BA_REQUEST;
		ieee80211_start_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		break;
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	case IEEE80211_AMPDU_TX_STOP_FLUSH:
		nrc_dbg(NRC_DBG_MAC, "%s: IEEE80211_AMPDU_TX_STOP_FLUSH", __func__);
		i_sta->tx_ba_session[tid] = IEEE80211_BA_CLOSE;
		break;
	case IEEE80211_AMPDU_TX_STOP_FLUSH_CONT:
		nrc_dbg(NRC_DBG_MAC, "%s: IEEE80211_AMPDU_TX_STOP_FLUSH_CONT", __func__);
		i_sta->tx_ba_session[tid] = IEEE80211_BA_CLOSE;
		break;
	case IEEE80211_AMPDU_TX_STOP_CONT:
		nrc_dbg(NRC_DBG_MAC, "%s: IEEE80211_AMPDU_TX_STOP_CONT", __func__);
		if (nrc_wim_ampdu_action(nw, vif, WIM_AMPDU_TX_STOP, sta, tid))
			return -EOPNOTSUPP;
		i_sta->tx_ba_session[tid] = IEEE80211_BA_REJECT;
		i_sta->ba_req_last_jiffies[tid] = jiffies;
		ieee80211_stop_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		// nrc_set_auto_ba(false);
		break;
#endif
	case IEEE80211_AMPDU_TX_OPERATIONAL:
		nrc_dbg(NRC_DBG_MAC, "%s: IEEE80211_AMPDU_TX_OPERATIONAL", __func__);
		i_sta->tx_ba_session[tid] = IEEE80211_BA_ACCEPT;
		if (nrc_wim_ampdu_action(nw, vif, WIM_AMPDU_TX_OPERATIONAL, sta, tid))
			return -EOPNOTSUPP;

		// nrc_set_auto_ba(true);
		return 0;
	case IEEE80211_AMPDU_RX_START:
		nrc_dbg(NRC_DBG_MAC, "%s: IEEE80211_AMPDU_RX_START", __func__);
		i_sta->rx_ba_session[tid].sn = *ssn;
		i_sta->rx_ba_session[tid].buf_size = buf_size;
		i_sta->rx_ba_session[tid].started = true;
		if (nw->ampdu_reject) {
			nrc_dbg(NRC_DBG_MAC, "%s: Reject AMPDU", __func__);
			return -EOPNOTSUPP;
		}
		return 0;
	case IEEE80211_AMPDU_RX_STOP:
		nrc_dbg(NRC_DBG_MAC, "%s: IEEE80211_AMPDU_RX_STOP", __func__);
		i_sta->rx_ba_session[tid].started = false;
		if (nw->ampdu_reject) {
			nrc_dbg(NRC_DBG_MAC, "%s: Reject AMPDU", __func__);
			return -EOPNOTSUPP;
		}
		return 0;
	default:
		return -EOPNOTSUPP;
	}

#if defined(CONFIG_USE_IEEE80211_AMPDU_PARAMS)
	params->amsdu = nw->amsdu_supported;
#endif
	return 0;
}

static void change_scan_mode(struct nrc *nw, enum NRC_SCAN_MODE new_mode)
{
	mutex_lock(&nw->state_mtx);
	nw->scan_mode = new_mode;
	mutex_unlock(&nw->state_mtx);
}

void scan_complete(struct ieee80211_hw *hw, bool aborted)
{
#ifdef CONFIG_USE_CFG80211_SCAN_INFO
	struct cfg80211_scan_info info = {
		.aborted = aborted,
	};

	ieee80211_scan_completed(hw, &info);
#else
	ieee80211_scan_completed(hw, aborted);
#endif
}

static void nrc_cancel_hw_scan(struct ieee80211_hw *hw,
							struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
#ifdef CONFIG_USE_SCAN_TIMEOUT
	struct nrc_vif *i_vif = to_i_vif(vif);

	cancel_delayed_work_sync(&i_vif->scan_timeout);
#endif

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SCAN_STOP, 0);
	nrc_xmit_wim_request(nw, skb);

	change_scan_mode(nw, NRC_SCAN_MODE_IDLE);

	scan_complete(hw, false);
}

void nrc_mac_cancel_hw_scan(struct ieee80211_hw *hw,
							struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;

	if (nw->scan_mode == NRC_SCAN_MODE_IDLE)
		return;

	nrc_cancel_hw_scan(hw, vif);
}

#ifdef NRC_BUILD_USE_HWSCAN
static int
__nrc_mac_hw_scan(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		  		struct cfg80211_scan_request *req,
		  		struct ieee80211_scan_ies *ies)
{
	struct nrc *nw = hw->priv;
	int scan_to = 30000; /* msec */
#ifdef CONFIG_USE_SCAN_TIMEOUT
	struct nrc_vif *i_vif = to_i_vif(vif);
#endif

	if (nw->block_frame ||
			(int)atomic_read(&nw->fw_state) != NRC_FW_ACTIVE) {
		nrc_mac_dbg("%s Scan Cancelled", __func__);
		return -EBUSY;
	}

	if(nw->drv_state == NRC_DRV_PS) {
		nrc_mac_dbg("%s Scan prohibited when PS", __func__);
		return -EBUSY;
	}

	if (nw->scan_mode != NRC_SCAN_MODE_IDLE)
		nrc_mac_cancel_hw_scan(hw, vif);

	if(nrc_stats_channel_noise_reset() < 0)
		nrc_mac_dbg("%s Channel noise reset fail", __func__);

	if(nw->associated_vif) {
		if (!disable_cqm) {
			nrc_mac_dbg("%s CQM timer off %u", __func__, nw->beacon_timeout);
			try_to_del_timer_sync(&nw->bcn_mon_timer);
		}

		if (ieee80211_hw_check(nw->hw, SUPPORTS_DYNAMIC_PS)) {
			if (nw->drv_state >= NRC_DRV_RUNNING &&
				nw->hw->conf.dynamic_ps_timeout > 0) {
				nrc_mac_dbg("%s PS timer off %ul", __func__, nw->hw->conf.dynamic_ps_timeout);
				try_to_del_timer_sync(&nw->dynamic_ps_timer);
			}
		}
	}

	scan_to += 120 * req->n_channels;

	nrc_wim_hw_scan(nw, vif, req, ies);

	if (req->n_ssids)
		change_scan_mode(nw, NRC_SCAN_MODE_ACTIVE_SCANNING);
	else
		change_scan_mode(nw, NRC_SCAN_MODE_PASSIVE_SCANNING);

#ifdef CONFIG_USE_SCAN_TIMEOUT
	queue_delayed_work(nw->workqueue, &i_vif->scan_timeout,
			msecs_to_jiffies(scan_to));
#endif

	return 0;
}

#ifdef CONFIG_USE_NEW_SCAN_REQ
static int
nrc_mac_hw_scan(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		struct ieee80211_scan_request *req)
{
	return __nrc_mac_hw_scan(hw, vif, &req->req, &req->ies);
}

#else
static int
nrc_mac_hw_scan(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
				struct cfg80211_scan_request *req)
{
	return __nrc_mac_hw_scan(hw, vif, req, NULL);
}
#endif
#endif /* NRC_BUILD_USE_HWSCAN */
#ifdef CONFIG_USE_SCAN_TIMEOUT
static void nrc_mac_scan_timeout(struct work_struct *work)
{
	struct nrc_vif *i_vif;
	struct sk_buff *skb;

	i_vif = container_of(work, struct nrc_vif, scan_timeout.work);

	skb = nrc_wim_alloc_skb_vif(i_vif->nw, to_ieee80211_vif(i_vif),
								WIM_CMD_SCAN_STOP, 0);

	nrc_xmit_wim_request(i_vif->nw, skb);

	change_scan_mode(i_vif->nw, NRC_SCAN_MODE_IDLE);

	scan_complete(i_vif->nw->hw, true);
}
#endif

static int
nrc_mac_set_bitrate_mask(struct ieee80211_hw *hw,
				   struct ieee80211_vif *vif,
				   const struct cfg80211_bitrate_mask *mask)
{
	int i, ret;
	uint8_t mcs_mask, mcs_level;
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
	enum nl80211_band band = NL80211_BAND_2GHZ;

	nrc_mac_dbg("%s legacy 0x%8X 0x%8X 0x%2X 0x%2X", __func__,
			mask->control[0].legacy,
			mask->control[1].legacy,
			mask->control[0].ht_mcs[0],
			mask->control[1].ht_mcs[0]);
#ifndef CONFIG_S1G_CHANNEL
	if (mask->control[band].ht_mcs[0] == 0xFF)
		band = NL80211_BAND_5GHZ;
	mcs_mask = mask->control[band].ht_mcs[0] & 0xFF;

	if (!mcs_mask) {
		mcs_level = 8;
	} else {
		for (i = 0; i < 8; i++) {
			if (mcs_mask & 0x1) {
				mcs_level = (uint8_t) i;
				nrc_mac_dbg("%s mcs level is %d", __func__,
						   mcs_level);
				break;
			}
			mcs_mask = mcs_mask>>1;
		}
	}
#else
	band = NL80211_BAND_S1GHZ;
	mcs_level = 10;
	mcs_mask = 0xFF;
	i=0;	//for removing mask compiler warning
#endif
	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_MCS, sizeof(mcs_level), &mcs_level);
	ret = nrc_xmit_wim_request(nw, skb);

	return ret;
}

#ifndef NRC_BUILD_USE_HWSCAN
static void nrc_mac_sw_scan(struct ieee80211_hw *hw,
							struct ieee80211_vif *vif,
							const u8 *mac_addr)
{
	struct nrc *nw = hw->priv;

	nrc_mac_dbg("%s", __func__);

	mutex_lock(&nw->state_mtx);

	if (nw->scan_mode == NRC_SCAN_MODE_IDLE) {
		nrc_mac_dbg("two sw_scans detected!");
		goto out;
	}

	nw->scan_mode = NRC_SCAN_MODE_SCANNING;
 out:
	mutex_unlock(&nw->state_mtx);
}

static void nrc_mac_sw_scan_complete(struct ieee80211_hw *hw,
									struct ieee80211_vif *vif)
{
	struct nrc *nw = hw->priv;

	nrc_mac_dbg("%s", __func__);

	mutex_lock(&nw->state_mtx);

	if (nw->scan_mode == NRC_SCAN_MODE_SCANNING) {
		nrc_mac_dbg("Scan has not started!");
		goto out;
	}

	nw->scan_mode = NRC_SCAN_MODE_IDLE;
 out:
	mutex_unlock(&nw->state_mtx);
}
#endif

#ifdef CONFIG_SUPPORT_NEW_FLUSH
static void nrc_mac_flush(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			  u32 queues, bool drop)
#else
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static void nrc_mac_flush(struct ieee80211_hw *hw, u32 queues, bool drop)
#else
static void nrc_mac_flush(struct ieee80211_hw *hw, bool drop)
#endif
#endif
{

}

#ifdef CONFIG_SUPPORT_TX_FRAMES_PENDING
static bool nrc_mac_tx_frames_pending(struct ieee80211_hw *hw)
{

#ifdef CONFIG_USE_TXQ
	return (nrc_txq_pending(hw) > 0);
#else
	return false;
#endif
}
#endif

static inline u64 nrc_mac_get_tsf_raw(void)
{
	return ktime_to_us(ktime_get_real());
}

static __le64 _nrc_mac_get_tsf(struct nrc *nw)
{
	u64 now = nrc_mac_get_tsf_raw();

	return cpu_to_le64(now + nw->tsf_offset);
}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static u64 nrc_mac_get_tsf(struct ieee80211_hw *hw,
			   struct ieee80211_vif *vif)
#else
static u64 nrc_mac_get_tsf(struct ieee80211_hw *hw)
#endif
{
	struct nrc *nw = hw->priv;

#ifdef CONFIG_SUPPORT_IBSS
	if ( vif->type == NL80211_IFTYPE_ADHOC){
		return current_bssid_beacon_timestamp;
	}else{
		return le64_to_cpu(_nrc_mac_get_tsf(nw));
	}
#else
	return le64_to_cpu(_nrc_mac_get_tsf(nw));
#endif /* CONFIG_SUPPORT_IBSS */
}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static void nrc_mac_set_tsf(struct ieee80211_hw *hw,
							struct ieee80211_vif *vif,
							u64 tsf)
#else
static void nrc_mac_set_tsf(struct ieee80211_hw *hw,
							u64 tsf)
#endif
{
}

#ifdef CONFIG_SUPPORT_IBSS
/*
* @tx_last_beacon: Determine whether the last IBSS beacon was sent by us.
*  This is needed only for IBSS mode and the result of this function is
*  used to determine whether to reply to Probe Requests.
*  Returns non-zero if this device sent the last beacon.
*  The callback can sleep.
*/
static int nrc_tx_last_beacon(struct ieee80211_hw *hw)
{
	return 1; // To send probe respose
}
#endif



#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static void nrc_mac_get_et_strings(struct ieee80211_hw *hw,
								struct ieee80211_vif *vif,
								u32 sset, u8 *data)
{
	if (sset == ETH_SS_STATS)
		memcpy(data, *nrc_gstrings_stats, sizeof(nrc_gstrings_stats));
}

static int nrc_mac_get_et_sset_count(struct ieee80211_hw *hw,
									struct ieee80211_vif *vif,
									int sset)
{
	if (sset == ETH_SS_STATS)
		return ARRAY_SIZE(nrc_gstrings_stats);
	return 0;
}

static void nrc_mac_get_et_stats(struct ieee80211_hw *hw,
								struct ieee80211_vif *vif,
								struct ethtool_stats *stats,
								u64 *data)
{

}
#endif

static int nrc_mac_set_rts_threshold(struct ieee80211_hw *hw, u32 value)
{
	struct sk_buff *skb;
	struct nrc *nw = hw->priv;
	struct ieee80211_vif *vif = nw->vif[0];

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	if (!skb)
		return -ENOMEM;

	nrc_mac_dbg("RTS Threshold: %u", value);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_RTS_THRESHOLD, sizeof(u32), &value);
	nrc_xmit_wim_request(nw, skb);
	return 0;
}

static int nrc_mac_set_key(struct ieee80211_hw *hw, enum set_key_cmd cmd,
						struct ieee80211_vif *vif, struct ieee80211_sta *sta,
						struct ieee80211_key_conf *key)
{
	struct nrc *nw = hw->priv;
	struct nrc_vif *i_vif = to_i_vif(vif);
	int vif_id = i_vif->index;
	int ret;

	nrc_mac_dbg("[%s_1] VIF(%d) CAP_MASK:0X%llx CMD:%d",__func__, vif_id,
		nw->cap.vif_caps[vif_id].cap_mask, cmd);
	nrc_mac_dbg("[%s_2] key (flag:%u, cipher:%u)",__func__, key->flags, key->cipher);
	nrc_mac_dbg("[%s_3] sw_enc (%d)",__func__, sw_enc);

	if (atomic_read(&nw->d_deauth.delayed_deauth)) {
		if (key->flags & IEEE80211_KEY_FLAG_PAIRWISE)
			memcpy(&nw->d_deauth.p, key, sizeof(struct ieee80211_key_conf));
		else
			memcpy(&nw->d_deauth.g, key, sizeof(struct ieee80211_key_conf));
		return 0;
	}

	/* if use SW SECURITY, return 1 */
	if (sw_enc == WIM_ENCDEC_SW) {
		if ((cmd == SET_KEY) &&	(key->cipher == WLAN_CIPHER_SUITE_AES_CMAC ||
			key->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_128 ||
			key->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_256))
			nw->cipher_pairwise = key->cipher; //for PMF deauth for keep alive on AP

		nrc_mac_dbg("SW Encryption for vif type=%d", vif->type);
		return 1;
	}

	/* if not use HW SECURITY of VIF , return 1 */
	if (!(nw->cap.vif_caps[vif_id].cap_mask & WIM_SYSTEM_CAP_HWSEC)) {
		dev_err(nw->dev, "failed to set caps");
		return 1;
	}

	if (sta == NULL && vif->type == NL80211_IFTYPE_STATION &&
			!(key->flags & IEEE80211_KEY_FLAG_PAIRWISE)) {
		sta = ieee80211_find_sta(vif, vif->bss_conf.bssid);
	}

	if (cmd == DISABLE_KEY) {
		nrc_mac_dbg("%s delete key (flag:%u, cipher:%u)",
			__func__, key->flags, key->cipher);

		if ((key->flags & IEEE80211_KEY_FLAG_PAIRWISE) && sta == NULL) {
			nrc_mac_dbg("%s delete ptk but sta is null! (key->flag:%u)",
				__func__, key->flags);
			return 1;
		}

		if (key->cipher == WLAN_CIPHER_SUITE_AES_CMAC ||
			key->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_128 ||
			key->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_256) {
			nrc_mac_dbg("%s delete key but not supported key cipher (key->cipher:%u)",
				__func__, key->cipher);
			return 1;
		}

		/* If HYBRID SECURITY of VIF,
			PTK : HW Security remove key, GTK: SW Security (return 1) */
		if (nw->cap.vif_caps[vif_id].cap_mask & WIM_SYSTEM_CAP_HYBRIDSEC) {
			if (!(key->flags & IEEE80211_KEY_FLAG_PAIRWISE)) {
				nrc_mac_dbg("[DEL] VIF(%d) KEY_FLAG(%u) GTK for HYBRID SECURITY. return 1",
					vif_id, key->flags);
				return 1;
			}
		}

	}

	/* Record key information to per-STA driver data structure for RX */
	/* TODO: DISABLE_KEY -> later */
	if (cmd == SET_KEY) {
		nrc_mac_dbg("%s add key (flag:%u, cipher:%u)",
			__func__, key->flags, key->cipher);

		/* HW does NOT Support BIP until now => need to SW-based Crypto => return 1 for this */
		if (key->cipher == WLAN_CIPHER_SUITE_AES_CMAC ||
			key->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_128 ||
			key->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_256) {
			nw->cipher_pairwise = key->cipher; //for PMF deauth for keep alive on AP
			key->flags |= IEEE80211_KEY_FLAG_SW_MGMT_TX;
			nrc_mac_dbg("%s add key but not supported key cipher (key->cipher:%u)",
				__func__, key->cipher);
			return 1;
		}

		/* If HYBRID SECURITY of VIF,
			PTK : HW Security(install key), GTK: SW Security (return 1) */
		if (nw->cap.vif_caps[vif_id].cap_mask & WIM_SYSTEM_CAP_HYBRIDSEC) {
			if (!(key->flags & IEEE80211_KEY_FLAG_PAIRWISE)) {
				nrc_mac_dbg("[ADD] VIF(%d) KEY_FLAG(%u) GTK for HYBRID SECURITY. return 1",
					vif_id, key->flags);
				return 1;
			}
		}

		if (sta) {
			struct nrc_sta *i_sta = to_i_sta(sta);

			if (key->flags & IEEE80211_KEY_FLAG_PAIRWISE)
				i_sta->ptk = key;
			else {
				i_sta->gtk = key;
				if (key->cipher == WLAN_CIPHER_SUITE_WEP40 ||
					key->cipher == WLAN_CIPHER_SUITE_WEP104)
					i_sta->ptk = key;
			}
		} else
			WARN_ON(!(vif->type == NL80211_IFTYPE_AP ||
#if defined(CONFIG_SUPPORT_IBSS)
				  vif->type == NL80211_IFTYPE_ADHOC ||
#endif
				  vif->type == NL80211_IFTYPE_P2P_GO ||
				  vif->type == NL80211_IFTYPE_MESH_POINT));
	}

	ret = nrc_wim_install_key(nw, cmd, vif, sta, key);
	if (ret < 0)
		ret = 1; /* fallback to software crypto */

	key->flags |= IEEE80211_KEY_FLAG_GENERATE_IV;	/* IV by the stack */
#if defined(CONFIG_SUPPORT_KEY_RESERVE_TAILROOM)
	/* Check whether MMIC will be generated by HW */
	if (nw->cap.cap_mask & WIM_SYSTEM_CAP_HWSEC_OFFL)
		key->flags |= IEEE80211_KEY_FLAG_RESERVE_TAILROOM;
#endif
	return ret;
}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static void nrc_mac_set_default_unicast_key(struct ieee80211_hw *hw,
											struct ieee80211_vif *vif,
											int keyidx)
{

}
#endif

static void nrc_mac_channel_policy(void *data, u8 *mac,
		struct ieee80211_vif *vif)
{
	struct sk_buff *skb;
	struct nrc_vif *i_vif = to_i_vif(vif);
	struct nrc *nw = i_vif->nw;
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	struct cfg80211_chan_def *chan_to_follow =
		(struct cfg80211_chan_def *)data;
	struct wireless_dev *wdev = ieee80211_vif_to_wdev(vif);
#ifdef CONFIG_USE_LINK_ID
	struct cfg80211_chan_def *chandef;
#endif
#else
	struct ieee80211_conf *chan_to_follow =
		(struct ieee80211_conf *)data;
	struct wireless_dev *wdev = i_vif->dev->ieee80211_ptr;
#endif

	if (!wdev)
		return;

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
#ifdef CONFIG_USE_LINK_ID
	chandef = wdev_chandef(wdev, vif->bss_conf.link_id);
	if (chandef->chan &&
		chandef->chan->center_freq ==
		chan_to_follow->chan->center_freq)
#else
	if (wdev->chandef.chan &&
		wdev->chandef.chan->center_freq ==
		chan_to_follow->chan->center_freq)
#endif /* ifdef CONFIG_USE_LINK_ID */
#else
	if (wdev->channel &&
		wdev->channel->center_freq ==
		chan_to_follow->channel->center_freq)
#endif /* ifdef CONFIG_SUPPORT_CHANNEL_INFO */
		return;

	if (!(vif->type == NL80211_IFTYPE_STATION ||
		vif->type == NL80211_IFTYPE_MESH_POINT))
		return;
#ifdef CONFIG_USE_VIF_CFG
	if (vif->cfg.assoc)
#else
	if (vif->bss_conf.assoc)
#endif
		return;

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);

	if (!skb)
		return;

	nrc_mac_add_tlv_channel(skb, chan_to_follow);
	nrc_xmit_wim_request(nw, skb);
}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static int nrc_mac_roc(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		struct ieee80211_channel *chan, int duration,
		enum ieee80211_roc_type type)
#else
static int nrc_mac_roc(struct ieee80211_hw *hw, struct ieee80211_channel *chan,
		enum nl80211_channel_type type, int duration)
#endif
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	struct cfg80211_chan_def chandef = {0};
	struct cfg80211_chan_def *cdef;
#else
	struct ieee80211_conf chandef = {0};
	struct ieee80211_conf *cdef;
	struct ieee80211_vif *vif = nw->vif[0];
#endif

	nrc_mac_dbg("%s, ch=%d, dur=%d", __func__,
			chan->center_freq, duration);

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET,
			tlv_len(sizeof(u16))
			+ tlv_len(ETH_ALEN));

	if (!skb)
		return -EINVAL;

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	if (!hw->conf.chandef.chan) {
		chandef.chan = chan;
#ifndef CONFIG_S1G_CHANNEL
		chandef.width = NL80211_CHAN_WIDTH_20;
#else
		chandef.width = NL80211_CHAN_WIDTH_1;
#endif
		cdef = &chandef;
	} else
		cdef = &hw->conf.chandef;

	nrc_mac_add_tlv_channel(skb, cdef);
#else
	if (!hw->conf.channel) {
		chandef.channel = chan;
		cdef = &chandef;
	} else
		cdef = &hw->conf;

	nrc_mac_add_tlv_channel(skb, cdef);
#endif
	nrc_xmit_wim_request(nw, skb);

	ieee80211_ready_on_channel(hw);

	nw->band = chan->band;
	nw->center_freq = chan->center_freq;

	ieee80211_queue_delayed_work(hw, &nw->roc_finish,
			msecs_to_jiffies(duration));

#ifdef CONFIG_SUPPORT_ITERATE_INTERFACE
	ieee80211_iterate_interfaces(nw->hw, IEEE80211_IFACE_ITER_ACTIVE,
			nrc_mac_channel_policy, cdef);
#else
	ieee80211_iterate_active_interfaces(nw->hw,
			nrc_mac_channel_policy, cdef);
#endif

	return 0;
}

#if KERNEL_VERSION(5, 4, 0) <= NRC_TARGET_KERNEL_VERSION
static int nrc_mac_cancel_roc(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
#else
static int nrc_mac_cancel_roc(struct ieee80211_hw *hw)
#endif
{
	struct nrc *nw = hw->priv;

	nrc_mac_dbg("%s", __func__);
	cancel_delayed_work_sync(&nw->roc_finish);

	return 0;
}

#ifdef CONFIG_USE_CHANNEL_CONTEXT
static int nrc_mac_add_chanctx(struct ieee80211_hw *hw,
		struct ieee80211_chanctx_conf *ctx)
{
	nrc_mac_dbg("%s, %d MHz/width: %d/cfreqs:%d/%d MHz\n",
			__func__,
			ctx->def.chan->center_freq, ctx->def.width,
			ctx->def.center_freq1, ctx->def.center_freq2);
	return 0;
}

static void nrc_mac_remove_chanctx(struct ieee80211_hw *hw,
		struct ieee80211_chanctx_conf *ctx)
{
	nrc_mac_dbg("%s, %d MHz/width: %d/cfreqs:%d/%d MHz\n",
			__func__,
			ctx->def.chan->center_freq, ctx->def.width,
			ctx->def.center_freq1, ctx->def.center_freq2);
}

static void nrc_mac_change_chanctx(struct ieee80211_hw *hw,
		struct ieee80211_chanctx_conf *ctx,
		u32 changed)
{
	nrc_mac_dbg("%s, %d MHz/width: %d/cfreqs:%d/%d MHz\n",
			__func__,
			ctx->def.chan->center_freq, ctx->def.width,
			ctx->def.center_freq1, ctx->def.center_freq2);
}

static int nrc_mac_assign_vif_chanctx(struct ieee80211_hw *hw,
		struct ieee80211_vif *vif,
		struct ieee80211_chanctx_conf *ctx)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;

	nrc_mac_dbg("%s, vif[type:%d, addr:%pM] %d MHz/width: %d/cfreqs:%d/%d MHz\n",
			__func__, vif->type, vif->addr,
			ctx->def.chan->center_freq, ctx->def.width,
			ctx->def.center_freq1, ctx->def.center_freq2);

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	if (!skb)
		return -EINVAL;

	nrc_mac_add_tlv_channel(skb, &ctx->def);
	nrc_xmit_wim_request(nw, skb);

	if (vif->type != NL80211_IFTYPE_MONITOR)
#ifdef CONFIG_SUPPORT_ITERATE_INTERFACE
		ieee80211_iterate_interfaces(nw->hw,
				IEEE80211_IFACE_ITER_ACTIVE,
				nrc_mac_channel_policy, &ctx->def);
#else
		ieee80211_iterate_active_interfaces(nw->hw,
				nrc_mac_channel_policy, &ctx->def);
#endif

	return 0;
}

static void nrc_mac_unassign_vif_chanctx(struct ieee80211_hw *hw,
		struct ieee80211_vif *vif,
		struct ieee80211_chanctx_conf *ctx)
{
	nrc_mac_dbg("%s, vif[type:%d, addr:%pM] %d MHz/width: %d/cfreqs:%d/%d MHz\n",
			__func__, vif->type, vif->addr,
			ctx->def.chan->center_freq, ctx->def.width,
			ctx->def.center_freq1, ctx->def.center_freq2);
}

static int nrc_mac_switch_vif_chanctx(struct ieee80211_hw *hw,
		struct ieee80211_vif_chanctx_switch *vifs,
		int n_vifs,
		enum ieee80211_chanctx_switch_mode mode)
{
	struct ieee80211_vif *vif = vifs->vif;
	struct ieee80211_chanctx_conf *old_ctx = vifs->old_ctx;
	struct ieee80211_chanctx_conf *new_ctx = vifs->new_ctx;

	struct nrc *nw = hw->priv;
	struct sk_buff *skb;

	nrc_mac_dbg("%s, vif[type:%d, addr:%pM]\n",
			__func__, vif->type, vif->addr);
	nrc_mac_dbg("%s, old[%d MHz/width: %d/cfreqs:%d/%d MHz]\n",
			__func__,
			old_ctx->def.chan->center_freq, old_ctx->def.width,
			old_ctx->def.center_freq1, old_ctx->def.center_freq2);
	nrc_mac_dbg("%s, new[%d MHz/width: %d/cfreqs:%d/%d MHz]\n",
			__func__,
			new_ctx->def.chan->center_freq, new_ctx->def.width,
			new_ctx->def.center_freq1, new_ctx->def.center_freq2);

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	if (!skb)
		return -EINVAL;
#if !defined(CONFIG_S1G_CHANNEL)
	nrc_wim_skb_add_tlv(skb,
			WIM_TLV_CHANNEL,
			sizeof(new_ctx->def.chan->center_freq),
			&new_ctx->def.chan->center_freq);
#else
	struct s1g_channel_table param;
	static char * wim_s1g_alpha2;
	wim_s1g_alpha2 = nrc_get_current_s1g_country();

	param.alpha2[0] = wim_s1g_alpha2[0];
	param.alpha2[1] = wim_s1g_alpha2[1];
	param.alpha2[2] = wim_s1g_alpha2[2];

	param.s1g_freq = FREQ_TO_100KHZ(new_ctx->def.chan->center_freq,
				new_ctx->def.chan->freq_offset);
	param.s1g_freq_index = nrc_get_channel_idx_by_freq(param.s1g_freq);
	param.cca_level_type = nrc_get_cca_by_freq(param.s1g_freq);
	nrc_s1g_set_channel_bw(param.s1g_freq, new_ctx->def.chan);
	param.chan_spacing = get_wim_channel_width(new_ctx->def.chan->width);
	param.global_oper_class = nrc_get_oper_class_by_freq(param.s1g_freq);
	param.offset = nrc_get_offset_by_freq(param.s1g_freq);
	param.primary_loc = nrc_get_pri_loc_by_freq(param.s1g_freq);

	nrc_wim_skb_add_tlv(skb, WIM_TLV_S1G_CHANNEL, sizeof(param), &param);
#endif /* !defined(CONFIG_S1G_CHANNEL) */
	nrc_xmit_wim_request(nw, skb);

#ifdef CONFIG_SUPPORT_ITERATE_INTERFACE
	ieee80211_iterate_interfaces(nw->hw, IEEE80211_IFACE_ITER_ACTIVE,
			nrc_mac_channel_policy, &new_ctx->def);
#else
	ieee80211_iterate_active_interfaces(nw->hw,
			nrc_mac_channel_policy, &new_ctx->def);
#endif

	return 0;
}
#endif

static void nrc_mac_channel_switch_beacon(struct ieee80211_hw *hw,struct ieee80211_vif *vif, struct cfg80211_chan_def *chandef)
{
	struct sk_buff *b;

#ifdef CONFIG_USE_LINK_ID
	b = ieee80211_beacon_get_template(hw, vif, NULL, vif->bss_conf.link_id);
#else
	b = ieee80211_beacon_get_template(hw, vif, NULL);
#endif

	print_hex_dump(KERN_DEBUG, "new vendor elem: ", DUMP_PREFIX_NONE,
		   16, 1, b->data, b->len, false);

	nrc_vendor_update_beacon(hw, vif);
	nrc_dbg(NRC_DBG_STATE, "[nrc_mac_channel_switch_beacon] Update Beacon for CSA\n");
}

static int nrc_pre_channel_switch(struct ieee80211_hw *hw,struct ieee80211_vif *vif, struct ieee80211_channel_switch *ch_switch)
{
	nrc_dbg(NRC_DBG_STATE, "[%s, %d] Channel switch start\n", __func__, __LINE__);
	return 0;
}

static int nrc_post_channel_switch(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
	nrc_dbg(NRC_DBG_STATE, "[%s, %d] Channel switch complete\n", __func__, __LINE__);
	return 0;
}

static void nrc_channel_switch(struct ieee80211_hw *hw,struct ieee80211_vif *vif, struct ieee80211_channel_switch *ch_switch)
{
	nrc_dbg(NRC_DBG_STATE, "[%s, %d] waiting for count less than 1 ... (CH to %d)\n", __func__, __LINE__, ch_switch->chandef.chan->center_freq);
	// ieee80211_chswitch_done(vif, true);
}

#ifdef CONFIG_PM
static struct wiphy_wowlan_support nrc_wowlan_support = {
	.flags = WIPHY_WOWLAN_ANY,
	/*
	 * We only supports two patterns.
	 */
	.n_patterns = 2,
	.pattern_max_len = WOWLAN_PATTER_SIZE,
	.pattern_min_len = 16,
	.max_pkt_offset = 16,
};

void nrc_mac_set_wakeup(struct ieee80211_hw *hw, bool enabled)
{
	struct nrc *nw = hw->priv;
	nw->wowlan_enabled = enabled;
	nrc_ps_dbg("[%s, L%d] wowlan enabled:%d\n", __func__, __LINE__, nw->wowlan_enabled);
	return;
}

int nrc_mac_resume(struct ieee80211_hw *hw)
{
	struct nrc *nw = hw->priv;

	nrc_ps_dbg("[%s, L%d]\n", __func__, __LINE__);

	nrc_hif_set_enable_irq(nw->hif);

	/* Resume rx queue */
	nrc_hif_resume_rx_thread(nw->hif);
	ieee80211_wake_queues(nw->hw);

	nrc_hif_resume(nw->hif);

	return 0;
}

int nrc_mac_suspend(struct ieee80211_hw *hw, struct cfg80211_wowlan *wowlan)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
	int ret = 0, i;
	struct wim_pm_param *p;

	nrc_ps_dbg("[%s,L%d] any:%d patterns(%p) n_patterns(%d)\n", __func__, __LINE__,
			wowlan->any, wowlan->patterns, wowlan->n_patterns);

	/*
	 * If the target is already in deepsleep state(running uCode),
	 * it's necessary to wake up the target first
	 * and then the wowlan info should be transferred.
	 */
	if (nw->drv_state == NRC_DRV_PS) {
		nrc_hif_wake_target(nw->hif);
		while (nw->drv_state == NRC_DRV_PS) {
			msleep(100);
		}
	}

	skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, WIM_MAX_SIZE);

	p = nrc_wim_skb_add_tlv(skb, WIM_TLV_PS_ENABLE,
				sizeof(struct wim_pm_param) + 64, NULL);
	memset(p, 0, sizeof(struct wim_pm_param));
	p->ps_mode = NRC_PS_DEEPSLEEP_TIM;
	p->ps_enable = true;
	p->ps_wakeup_pin = power_save_gpio[1];
	p->wowlan_wakeup_host_pin = TARGET_GPIO_FOR_WAKEUP_HOST;
	p->wowlan_enable_any = wowlan->any;
	p->wowlan_enable_magicpacket = wowlan->magic_pkt;
	p->wowlan_enable_disconnect = wowlan->disconnect;
	p->wowlan_n_patterns = wowlan->n_patterns;
	for (i = 0; i < wowlan->n_patterns; i++) {
		p->wp[i].offset = (u8)wowlan->patterns[i].pkt_offset;
		p->wp[i].pattern_len = (u8)wowlan->patterns[i].pattern_len;
		p->wp[i].mask_len = DIV_ROUND_UP(p->wp[i].pattern_len, 8);
		memcpy(p->wp[i].pattern, wowlan->patterns[i].pattern, p->wp[i].pattern_len);
		memcpy(p->wp[i].mask, wowlan->patterns[i].mask, p->wp[i].mask_len);
	}

	/* Stop rx queue */
	ieee80211_stop_queues(nw->hw);
#ifdef CONFIG_USE_TXQ
	nrc_cleanup_txq_all(nw);
#endif

	if (!disable_cqm) {
		try_to_del_timer_sync(&nw->bcn_mon_timer);
	}

	nrc_hif_sleep_target_start(nw->hif, NRC_PS_DEEPSLEEP_NONTIM);
	/* TX WIM  */
	ret = nrc_xmit_wim_request(nw, skb);

	//nrc_hif_flush_wq(nw->hif);


	/* SPI suspend for deepsleep  */
	if (!ieee80211_hw_check(hw, SUPPORTS_DYNAMIC_PS) ||
	    !ieee80211_hw_check(hw, SUPPORTS_PS)) {
		nrc_hif_suspend(nw->hif);
	}
	nrc_hif_sleep_target_end(nw->hif, NRC_PS_DEEPSLEEP_NONTIM);
	nrc_ps_dbg("[%s,L%d] Enter DEEPSLEEP(%d)!!!\n", __func__, __LINE__, ret);

	nrc_hif_disable_irq(nw->hif);

	return 0;
}
#endif

#if KERNEL_VERSION(4, 8, 0) <= NRC_TARGET_KERNEL_VERSION
static u32 nrc_get_expected_throughput(struct ieee80211_hw *hw,
		struct ieee80211_sta *sta)
#else
static u32 nrc_get_expected_throughput(struct ieee80211_sta *sta)
#endif
{
	uint32_t tput = 0;

	if (!sta)
		return 0;

	tput = nrc_stats_metric(sta->addr);
	/* prevent overflow in mac80211 */
	if (tput < 200)
		tput = 200;

	return tput;
}

#define MPDU_LEN_THRESHOLD			511 		/* See lmac_11ah.h  */
static int nrc_set_frag_threshold(struct ieee80211_hw *hw, u32 value)
{
	struct nrc *nw = hw->priv;

	nw->frag_threshold = value;
	nrc_mac_dbg("Fragmentation Threshold: %d", nw->frag_threshold);

	if (nw->frag_threshold >= MPDU_LEN_THRESHOLD) {
		dev_err(nw->dev, "Frag threshold value must be smaller than %d\n", MPDU_LEN_THRESHOLD);
		return -EINVAL;
	}

	spin_lock_bh(&nw->txq_lock);
	rcu_read_lock();

	if (nw->frag_threshold == -1) {
		nrc_mac_dbg("Fragmentation Disabled.\n");
	}
	else {
		nrc_mac_dbg("Fragmentation Enabled.\n");
		nrc_cleanup_ba_session_all (nw);

	}
	rcu_read_unlock();
	spin_unlock_bh(&nw->txq_lock);

	return 0;
}

static const struct ieee80211_ops nrc_mac80211_ops = {
	.tx = nrc_mac_tx,
	.start = nrc_mac_start,
	.stop = nrc_mac_stop,
#ifdef CONFIG_PM
	.suspend = nrc_mac_suspend,
	.resume = nrc_mac_resume,
	.set_wakeup = nrc_mac_set_wakeup,
#endif
	.add_interface = nrc_mac_add_interface,
	.change_interface = nrc_mac_change_interface,
	.remove_interface = nrc_mac_remove_interface,
	.config = nrc_mac_config,
	.configure_filter = nrc_mac_configure_filter,
	.bss_info_changed = nrc_mac_bss_info_changed,
#ifdef CONFIG_USE_TXQ
	.wake_tx_queue = nrc_wake_tx_queue,
#endif
#ifdef NRC_BUILD_USE_HWSCAN
	.hw_scan = nrc_mac_hw_scan,
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	.cancel_hw_scan = nrc_mac_cancel_hw_scan,
#endif
#endif
	.set_key = nrc_mac_set_key,
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	.set_default_unicast_key = nrc_mac_set_default_unicast_key,
	.sta_state = nrc_mac_sta_state,
	.sta_pre_rcu_remove = nrc_mac_sta_pre_rcu_remove,
#else
	.sta_add = nrc_mac_sta_add,
	.sta_remove = nrc_mac_sta_remove,
#endif
	.sta_notify = nrc_mac_sta_notify,
	.set_tim = nrc_mac_set_tim,
	.set_rts_threshold = nrc_mac_set_rts_threshold,
	.set_frag_threshold = nrc_set_frag_threshold,
	.conf_tx = nrc_mac_conf_tx,
	.get_survey = nrc_mac_get_survey,
	.ampdu_action = nrc_mac_ampdu_action,
	.set_bitrate_mask = nrc_mac_set_bitrate_mask,
#ifndef NRC_BUILD_USE_HWSCAN
	.sw_scan_start = nrc_mac_sw_scan,
	.sw_scan_complete = nrc_mac_sw_scan_complete,
#endif
	.flush = nrc_mac_flush,
#ifdef CONFIG_SUPPORT_TX_FRAMES_PENDING
	.tx_frames_pending = nrc_mac_tx_frames_pending,
#endif
	.get_tsf = nrc_mac_get_tsf,
	.set_tsf = nrc_mac_set_tsf,
	.remain_on_channel = nrc_mac_roc,
	.cancel_remain_on_channel = nrc_mac_cancel_roc,
#ifdef CONFIG_SUPPORT_IBSS
	.tx_last_beacon = nrc_tx_last_beacon,
#endif
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	.get_et_sset_count = nrc_mac_get_et_sset_count,
	.get_et_stats = nrc_mac_get_et_stats,
	.get_et_strings = nrc_mac_get_et_strings,
#endif
#ifdef CONFIG_USE_CHANNEL_CONTEXT
	.add_chanctx = nrc_mac_add_chanctx,
	.remove_chanctx = nrc_mac_remove_chanctx,
	.change_chanctx = nrc_mac_change_chanctx,
	.assign_vif_chanctx = nrc_mac_assign_vif_chanctx,
	.unassign_vif_chanctx = nrc_mac_unassign_vif_chanctx,
	.switch_vif_chanctx = nrc_mac_switch_vif_chanctx,
#endif
	.channel_switch_beacon = nrc_mac_channel_switch_beacon,
	.pre_channel_switch = nrc_pre_channel_switch,
	.post_channel_switch = nrc_post_channel_switch,
	.channel_switch = nrc_channel_switch,
	.get_expected_throughput = nrc_get_expected_throughput
};


#ifdef CONFIG_NEW_REG_NOTIFIER
void nrc_reg_notifier(struct wiphy *wiphy,
		struct regulatory_request *request)
#else
int nrc_reg_notifier(struct wiphy *wiphy,
		struct regulatory_request *request)
#endif
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
#if defined(CONFIG_SUPPORT_BD)
	int i;
	struct wim_bd_param *bd_param = NULL;
#endif /* CONFIG_SUPPORT_BD */
#ifdef CONFIG_S1G_CHANNEL
	const struct s1g_channel_table *cc_table;
#endif /* CONFIG_S1G_CHANNEL */

	nrc_mac_dbg("info: cfg80211 regulatory domain callback for %c%c",
			request->alpha2[0], request->alpha2[1]);
	nrc_mac_dbg("request->initiator:%d", request->initiator);
	nrc_cc[0] = request->alpha2[0];
	nrc_cc[1] = request->alpha2[1];
	if ((request->alpha2[0] == '0' && request->alpha2[1] == '0') ||
		(request->alpha2[0] == '9' && request->alpha2[1] == '9')) {
		dev_err(nw->dev, "[Error] CC is 00 or 99. Skip loading BD and setting CC");
#ifdef CONFIG_NEW_REG_NOTIFIER
		return;
#else
		return 0;
#endif
	}

	if (nw->drv_state == NRC_DRV_PS) {
		/* HSPI is not ready */
#ifdef CONFIG_NEW_REG_NOTIFIER
		return;
#else
		return 0;
#endif
	}

#if defined(CONFIG_SUPPORT_BD)
	/**
	 * Read board data and save buffer.
	 * nrc_cc can be changed to EU, K1 or K2 in nrc_read_bd_tx_pwr().
	 */
	atomic_set(&nw->bd_down, NRC_BD_LOADING);
	bd_param = nrc_read_bd_tx_pwr(nw, nrc_cc);
	if(bd_param) {
		nrc_dbg(NRC_DBG_BD,"type %04X length %04X checksum %04X target_ver %04X",
				bd_param->type, bd_param->length, bd_param->checksum, bd_param->hw_version);
		for(i=0; i < bd_param->length - 4;) {
			nrc_dbg(NRC_DBG_BD,"%02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d",
				(bd_param->value[i]), (bd_param->value[i+1]), (bd_param->value[i+2]),
				(bd_param->value[i+3]), (bd_param->value[i+4]), (bd_param->value[i+5]),
				(bd_param->value[i+6]), (bd_param->value[i+7]), (bd_param->value[i+8]),
				(bd_param->value[i+9]), (bd_param->value[i+10]), (bd_param->value[i+11])
				);
			i += 12;
		}
	} else {
		/* Default policy is that if board data is invalid, block loading of FW */
		nrc_mac_dbg("BD file is invalid! Stop loading FW");
		g_bd_valid= false;
#ifdef CONFIG_NEW_REG_NOTIFIER
		return;
#else
		return -1;
#endif /* CONFIG_NEW_REG_NOTIFIER */
	}
#endif /* defined(CONFIG_SUPPORT_BD) */

	nw->alpha2[0] = request->alpha2[0];
	nw->alpha2[1] = request->alpha2[1];

	skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, WIM_MAX_SIZE);
#ifdef CONFIG_S1G_CHANNEL
	nrc_set_s1g_country(nrc_cc);
#else
	nrc_wim_skb_add_tlv(skb, WIM_TLV_COUNTRY_CODE, sizeof(u16), nrc_cc);
#endif

#if defined(CONFIG_SUPPORT_BD)
	if(bd_param) {
		nrc_dbg(NRC_DBG_STATE,"succeed in loading board data on target");
		nrc_wim_skb_add_tlv(skb, WIM_TLV_BD, sizeof(*bd_param), bd_param);
		kfree(bd_param);
	} else {
		nrc_dbg(NRC_DBG_STATE,"fail to load board data on target");
	}
#endif /* defined(CONFIG_SUPPORT_BD) */

	nrc_xmit_wim_request(nw, skb);

#ifdef CONFIG_S1G_CHANNEL
	nrc_dbg(NRC_DBG_MAC, "%s: Country: %s", __func__, (char *)nrc_get_current_s1g_country());
	cc_table = (const struct s1g_channel_table *) nrc_get_current_s1g_cc_table();
	skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, WIM_MAX_SIZE);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_CH_TABLE, sizeof(struct s1g_channel_table)*nrc_get_num_channels_by_current_country(), (struct s1g_channel_table *)nrc_get_current_s1g_cc_table());
	nrc_xmit_wim_request(nw, skb);
#endif /* CONFIG_S1G_CHANNEL */

#if defined(CONFIG_SUPPORT_BD)
	atomic_set(&nw->bd_down, NRC_BD_READY);
#endif /* defined(CONFIG_SUPPORT_BD) */

#ifdef CONFIG_NEW_REG_NOTIFIER
	return;
#else
	return 0;
#endif
}

static u8* nrc_vendor_remove(struct nrc *nw, u8 subcmd)
{
	u8 *pos = NULL;
	struct sk_buff **vendor_skb_ptr = NULL;

	if ((subcmd >= NRC_SUBCMD_ANNOUNCE1) &&
	    (subcmd <= NRC_SUBCMD_BCAST_FOTA_4)) {
		vendor_skb_ptr = &nw->vendor_skb_beacon;
	} else if ((subcmd >= NRC_SUBCMD_ANNOUNCE6) &&
		       (subcmd <= NRC_SUBCMD_ANNOUNCE10)) {
		vendor_skb_ptr = &nw->vendor_skb_probe_req;
	} else if ((subcmd >= NRC_SUBCMD_ANNOUNCE11) &&
		       (subcmd <= NRC_SUBCMD_ANNOUNCE15)) {
		vendor_skb_ptr = &nw->vendor_skb_probe_rsp;
	} else if ((subcmd >= NRC_SUBCMD_ANNOUNCE16) &&
		       (subcmd <= NRC_SUBCMD_ANNOUNCE20)) {
		vendor_skb_ptr = &nw->vendor_skb_assoc_req;
	}

	if (!(*vendor_skb_ptr))
		return NULL;

	pos = (u8*)cfg80211_find_vendor_ie(VENDOR_OUI, subcmd,
			(*vendor_skb_ptr)->data, (*vendor_skb_ptr)->len);

	if (pos) {
		u8 len = *(pos + 1);
		memmove(pos, pos + len + 2, (*vendor_skb_ptr)->len -
			(pos - (*vendor_skb_ptr)->data) - (len + 2));
		skb_trim(*vendor_skb_ptr, (*vendor_skb_ptr)->len - (len + 2));
	}

	nrc_mac_dbg("%s: removed vendor IE", __func__);
	return pos;
}

static int nrc_vendor_update(struct nrc *nw, u8 subcmd,
				const u8 *data, int data_len)
{
	const int OUI_LEN = 3;
	const int MAX_DATALEN = 255;
	int new_elem_len = data_len + 2 /* EID + LEN */ + OUI_LEN + 1;
	u8 *pos;
	struct sk_buff **vendor_skb_ptr = NULL;

	if (!data || data_len < 1 || (data_len + OUI_LEN + 1) > MAX_DATALEN)
		return -EINVAL;

	if (subcmd >= NRC_SUBCMD_ANNOUNCE1 && subcmd <= NRC_SUBCMD_BCAST_FOTA_4)
		vendor_skb_ptr = &nw->vendor_skb_beacon;
	else if (subcmd >= NRC_SUBCMD_ANNOUNCE6 && subcmd <= NRC_SUBCMD_ANNOUNCE10)
		vendor_skb_ptr = &nw->vendor_skb_probe_req;
	else if (subcmd >= NRC_SUBCMD_ANNOUNCE11 && subcmd <= NRC_SUBCMD_ANNOUNCE15)
		vendor_skb_ptr = &nw->vendor_skb_probe_rsp;
	else if (subcmd >= NRC_SUBCMD_ANNOUNCE16 && subcmd <= NRC_SUBCMD_ANNOUNCE20)
		vendor_skb_ptr = &nw->vendor_skb_assoc_req;
	else
		WARN_ON(true);

	if (!(*vendor_skb_ptr))
		*vendor_skb_ptr = dev_alloc_skb(IEEE80211_MAX_FRAME_LEN);

	// Remove old data first
	pos = nrc_vendor_remove(nw, subcmd);

	/* Append new data */
	pos = skb_put(*vendor_skb_ptr, new_elem_len);
	*pos++ = WLAN_EID_VENDOR_SPECIFIC;
	*pos++ = data_len + OUI_LEN + 1;
	*pos++ = (VENDOR_OUI & 0xFF0000) >> 16;
	*pos++ = (VENDOR_OUI & 0xFF00) >> 8;
	*pos++ = (VENDOR_OUI & 0xFF);
	*pos++ = subcmd;
	memcpy(pos, data, data_len);

	if (debug_level_all) {
		print_hex_dump(KERN_DEBUG, "new vendor elem: ", DUMP_PREFIX_NONE,
				16, 1, (*vendor_skb_ptr)->data, (*vendor_skb_ptr)->len, false);
	}

	return 0;
}

static int nrc_vendor_cmd_remove(struct wiphy *wiphy,
				struct wireless_dev *wdev, u8 subcmd)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct ieee80211_vif *vif = wdev_to_ieee80211_vif(wdev);
	struct nrc *nw = hw->priv;

	/* Remove the vendor ie */
	if (nrc_vendor_remove(nw, subcmd) == NULL)
		return -EINVAL;

	/* Update beacon */
	if (subcmd >= NRC_SUBCMD_ANNOUNCE1 && subcmd <= NRC_SUBCMD_BCAST_FOTA_4)
		return nrc_vendor_update_beacon(hw, vif);
	else if (subcmd >= NRC_SUBCMD_ANNOUNCE6 && subcmd <= NRC_SUBCMD_ANNOUNCE10)
		return nrc_vendor_update_probe_req(hw, vif);
	else if (subcmd >= NRC_SUBCMD_ANNOUNCE11 && subcmd <= NRC_SUBCMD_ANNOUNCE15)
		return nrc_vendor_update_probe_rsp(hw, vif);
	else if (subcmd >= NRC_SUBCMD_ANNOUNCE16 && subcmd <= NRC_SUBCMD_ANNOUNCE20)
		return nrc_vendor_update_assoc_req(hw, vif);
	else
		WARN_ON(true);

	return -EINVAL;
}

struct timer_list remotecmd_timer;
struct remotecmd_params remotecmd_params;

#if KERNEL_VERSION(4, 15, 0) > NRC_TARGET_KERNEL_VERSION
void remotecmd_callback(unsigned long ptr)
{
	struct remotecmd_params *params = (struct remotecmd_params *)ptr;
	struct wiphy *wiphy = params->wiphy;
	struct wireless_dev *wdev = params->wdev;
	u8 subcmd = params->subcmd;
#else
void remotecmd_callback(struct timer_list *t)
{
	struct wiphy *wiphy = remotecmd_params.wiphy;
	struct wireless_dev *wdev = remotecmd_params.wdev;
	u8 subcmd = remotecmd_params.subcmd;
#endif
	nrc_vendor_cmd_remove(wiphy, wdev, subcmd);
}

void remotecmd_schedule_off(struct wiphy *wiphy, struct wireless_dev *wdev,
				u8 subcmd, const u8 cntdwn, u16 beacon_int)
{
	remotecmd_params.wiphy = wiphy;
	remotecmd_params.wdev = wdev;
	remotecmd_params.subcmd = subcmd;

#if KERNEL_VERSION(4, 15, 0) > NRC_TARGET_KERNEL_VERSION
	init_timer(&remotecmd_timer);
	remotecmd_timer.function = remotecmd_callback;
	remotecmd_timer.data = (unsigned long)&remotecmd_params;
	remotecmd_timer.expires = jiffies +
	usecs_to_jiffies(beacon_int * cntdwn * 1024);
	add_timer(&remotecmd_timer);
#else
	timer_setup(&remotecmd_timer, remotecmd_callback, 0);
	mod_timer(&remotecmd_timer, jiffies +
		usecs_to_jiffies(beacon_int * cntdwn * 1024));
#endif
}

static int nrc_vendor_cmd_append(struct wiphy *wiphy, struct wireless_dev *wdev,
				u8 subcmd, const void *data, int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct ieee80211_vif *vif = wdev_to_ieee80211_vif(wdev);
	struct nrc *nw = hw->priv;

	/* Update local vendor data */
	if (nrc_vendor_update(nw, subcmd, data, data_len) != 0)
		return -EINVAL;
	// Schedule async vendor IE removal if REMOTECMD
	if (subcmd == NRC_SUBCMD_REMOTECMD) {
#ifdef CONFIG_USE_VIF_CFG
		remotecmd_schedule_off(wiphy, wdev, subcmd, *(const u8 *)data, vif->bss_conf.beacon_int);
#else
		remotecmd_schedule_off(wiphy, wdev, subcmd, *(const u8 *)data, wdev->beacon_interval);
#endif
	}

	if (subcmd < NRC_SUBCMD_BCAST_FOTA_1)
		return nrc_vendor_update_beacon(hw, vif);
	else if ((subcmd >= NRC_SUBCMD_BCAST_FOTA_1) && (subcmd < NRC_SUBCMD_ANNOUNCE6))
		return 0;
	else if ((subcmd >= NRC_SUBCMD_ANNOUNCE6) && (subcmd < NRC_SUBCMD_ANNOUNCE11))
		return nrc_vendor_update_probe_req(hw, vif);
	else if ((subcmd >= NRC_SUBCMD_ANNOUNCE11) && (subcmd < NRC_SUBCMD_ANNOUNCE16))
		return nrc_vendor_update_probe_rsp(hw, vif);
	else if ((subcmd >= NRC_SUBCMD_ANNOUNCE16) && (subcmd < NRC_SUBCMD_ANNOUNCE20))
		return nrc_vendor_update_assoc_req(hw, vif);
	else
		return -EINVAL;
}

static int nrc_vendor_cmd_wowlan_pattern(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	struct sk_buff *skb;
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct nrc *nw = hw->priv;
	u8* new_data = (u8*)data;
	u8 count = *new_data;

	nrc_mac_dbg("%s: called", __func__);
	nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_WOWLAN_PATTERN,
				(void*)(new_data + 1), data_len - 1);

	queue_delayed_work(nw->workqueue, &nw->rm_vendor_ie_wowlan_pattern,
							msecs_to_jiffies(count * nw->beacon_int));

	/* Send a response to the command */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, 10);
	if (!skb)
		return -ENOMEM;

	nla_put_u32(skb, NRC_SUBCMD_WOWLAN_PATTERN, 0x11223344);

	return cfg80211_vendor_cmd_reply(skb);
}

static int nrc_vendor_cmd_announce1(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE1,
				 data, data_len);
}

static int nrc_vendor_cmd_announce2(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE2,
				 data, data_len);
}

static int nrc_vendor_cmd_announce3(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE3,
				 data, data_len);
}

static int nrc_vendor_cmd_announce4(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE4,
				 data, data_len);
}

static int nrc_vendor_cmd_announce5(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE5,
				 data, data_len);
}

static int nrc_vendor_cmd_remotecmd(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_REMOTECMD,
				 data, data_len);
}

static int nrc_vendor_cmd_remove_vendor_ie(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	nrc_mac_dbg("%s: called", __func__);
	return nrc_vendor_cmd_remove(wiphy, wdev, *((u8*)data));
}

static int nrc_vendor_cmd_bcast_fota_info(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_BCAST_FOTA_INFO,
				 data, data_len);
}

static int nrc_vendor_cmd_bcast_fota_1(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_BCAST_FOTA_1,
				 data, data_len);
}

static int nrc_vendor_cmd_bcast_fota_2(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_BCAST_FOTA_2,
				 data, data_len);
}

static int nrc_vendor_cmd_bcast_fota_3(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_BCAST_FOTA_3,
				 data, data_len);
}

static int nrc_vendor_cmd_bcast_fota_4(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_BCAST_FOTA_4,
				 data, data_len);
}

static int nrc_vendor_cmd_announce6(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE6,
				 data, data_len);
}

static int nrc_vendor_cmd_announce7(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE7,
				 data, data_len);
}

static int nrc_vendor_cmd_announce8(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE8,
				 data, data_len);
}

static int nrc_vendor_cmd_announce9(struct wiphy *wiphy,
				struct wireless_dev*wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE9,
				 data, data_len);
}

static int nrc_vendor_cmd_announce10(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE10,
				 data, data_len);
}

static int nrc_vendor_cmd_announce11(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE11,
				 data, data_len);
}

static int nrc_vendor_cmd_announce12(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE12,
				 data, data_len);
}

static int nrc_vendor_cmd_announce13(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE13,
				 data, data_len);
}

static int nrc_vendor_cmd_announce14(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE14,
				 data, data_len);
}

static int nrc_vendor_cmd_announce15(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE15,
				 data, data_len);
}

static int nrc_vendor_cmd_announce16(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE16,
				 data, data_len);
}

static int nrc_vendor_cmd_announce17(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE17,
				 data, data_len);
}

static int nrc_vendor_cmd_announce18(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE18,
				 data, data_len);
}

static int nrc_vendor_cmd_announce19(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE19,
				 data, data_len);
}

static int nrc_vendor_cmd_announce20(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_SUBCMD_ANNOUNCE20,
				 data, data_len);
}
static struct wiphy_vendor_command nrc_vendor_cmds[] = {
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_WOWLAN_PATTERN
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_wowlan_pattern,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE1
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce1,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE2
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce2,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE3
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce3,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE4
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce4,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE5
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce5,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_REMOTECMD
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_remotecmd,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = { .vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			  .subcmd = NRC_SUBCMD_RM_VENDOR_IE },
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_remove_vendor_ie,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_BCAST_FOTA_INFO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_bcast_fota_info,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_BCAST_FOTA_1
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_bcast_fota_1,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_BCAST_FOTA_2
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_bcast_fota_2,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_BCAST_FOTA_3
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_bcast_fota_3,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_BCAST_FOTA_4
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_bcast_fota_4,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE6
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce6,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE7
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce7,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE8
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce8,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE9
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce9,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE10
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce10,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE11
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce11,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE12
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce12,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE13
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce13,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE14
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce14,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE15
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce15,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE16
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce16,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE17
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce17,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE18
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce18,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE19
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce19,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
	{
		.info = {
			.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
			.subcmd = NRC_SUBCMD_ANNOUNCE20
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce20,
#if KERNEL_VERSION(5, 3, 0) <= NRC_TARGET_KERNEL_VERSION
		.policy = VENDOR_CMD_RAW_DATA,
		.maxattr = MAX_VENDOR_ATTR,
#endif
	},
};

static const struct nl80211_vendor_cmd_info nrc_vendor_events[] = {
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE1
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE2
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE3
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE4
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE5
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_REMOTECMD
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_WOWLAN_PATTERN
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_BCAST_FOTA_INFO
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_BCAST_FOTA_1
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_BCAST_FOTA_2
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_BCAST_FOTA_3
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_BCAST_FOTA_4
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE6
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE7
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE8
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE9
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE10
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE11
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE12
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE13
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE14
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE15
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE16
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE17
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE18
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE19
	},
	{
		.vendor_id = OUI_IEEE_REGISTRATION_AUTHORITY,
		.subcmd = NRC_SUBCMD_ANNOUNCE20
	},
};

void nrc_rm_vendor_ie_wowlan_pattern(struct work_struct *work)
{
	struct nrc *nw = container_of(work, struct nrc, rm_vendor_ie_wowlan_pattern.work);

	/* Remove the vendor ie */
	nrc_vendor_remove(nw, NRC_SUBCMD_WOWLAN_PATTERN);
	/* Update beacon */
	nrc_vendor_update_beacon(nw->hw, nw->vif[0]);
}

#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
void nrc_bcn_mon_timer(unsigned long data)
{
	struct nrc *nw = (struct nrc *)data;
#else
void nrc_bcn_mon_timer(struct timer_list *t)
{
	struct nrc *nw = from_timer(nw, t, bcn_mon_timer);
#endif
	//nrc_mac_dbg("[%s,L%d]", __func__, __LINE__);
	if (nw->drv_state == NRC_DRV_PS) {
		nrc_mac_dbg("In PS state, ignore bcn_mon_timeout\n");
		return;
	}
	nrc_send_beacon_loss(nw);
}

#if defined(ENABLE_DYNAMIC_PS)
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
static void nrc_ps_timeout_timer(unsigned long data)
{
	struct nrc *nw = (struct nrc *)data;
#else
static void nrc_ps_timeout_timer(struct timer_list *t)
{
	struct nrc *nw = from_timer(nw, t, dynamic_ps_timer);
#endif
	struct nrc_hif_device *hdev = nw->hif;

	if ((int)atomic_read(&nw->fw_state) == NRC_FW_LOADING ) {
		nrc_ps_dbg("[%s]FW is loading",__func__);
		return;
	}

#if defined(CONFIG_SUPPORT_BD)
	if ((int)atomic_read(&nw->bd_down) == NRC_BD_LOADING) {
		nrc_ps_dbg("[%s]BD is loading",__func__);
		return;
	}
#endif
	nrc_ps_dbg("\n[%s,L%d] ps_enable:%d dynamic_ps_timeout:%d",
		__func__, __LINE__, nw->ps_enabled, nw->hw->conf.dynamic_ps_timeout);

	if (nw->ps_enabled) {
		if (nw->drv_state == NRC_DRV_PS) {
			/*
			 * if the current state is already NRC_DRV_PS,
			 * there's nothing to do in here even if mac80211 notifies wake-up.
			 * the actual action to wake up for target will be done by
			 * nrc_wake_tx_queue() with changing gpio signal.
			 * (when driver receives a data frame.)
			 */
			nrc_ps_dbg("Target is already in deepsleep...");
			return;
		}
		if (nw->ps_wq != NULL)
			queue_work(nw->ps_wq, &hdev->ps_work);
	}
}
#endif /* if defined(ENABLE_DYNAMIC_PS) */

/**
 * nrc_hw_register - initialize struct ieee80211_hw instance
 */
int nrc_register_hw(struct nrc *nw)
{
	struct ieee80211_hw *hw = nw->hw;
	struct ieee80211_supported_band *sband = NULL;
#ifdef CONFIG_USE_NEW_BAND_ENUM
	enum nl80211_band band;
#else
	enum ieee80211_band band;
#endif
	int ret;
	int i;
/*	char tmp[2];*/


	for (i = 0; i < NR_NRC_VIF; i++) {
		if (!nw->has_macaddr[i])
			set_mac_address(&nw->mac_addr[i], i);
	}

	SET_IEEE80211_PERM_ADDR(hw, nw->mac_addr[0].addr);
	hw->wiphy->n_addresses = NR_NRC_VIF;
	hw->wiphy->addresses = nw->mac_addr;

	SET_IEEE80211_DEV(hw, nw->dev);

	hw->wiphy->max_scan_ssids = WIM_MAX_SCAN_SSID;
	hw->wiphy->max_scan_ie_len = WIM_MAX_TLV_SCAN_IE;
	hw->wiphy->max_remain_on_channel_duration = NRC_MAC80211_ROC_DURATION;
	hw->wiphy->interface_modes =
		BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP) |
#ifdef CONFIG_MAC80211_MESH
		BIT(NL80211_IFTYPE_MESH_POINT) |
#endif
#if !defined(CONFIG_S1G_CHANNEL)
#ifdef CONFIG_SUPPORT_P2P
		BIT(NL80211_IFTYPE_P2P_CLIENT) | BIT(NL80211_IFTYPE_P2P_GO) |
		BIT(NL80211_IFTYPE_P2P_DEVICE) |
#endif
#endif /* CONFIG_S1G_CHANNEL */
#if defined(CONFIG_WIRELESS_WDS)
		BIT(NL80211_IFTYPE_WDS) |
#endif
#if defined(CONFIG_SUPPORT_IBSS)
		BIT(NL80211_IFTYPE_ADHOC) |
#endif
		BIT(NL80211_IFTYPE_MONITOR);
	hw->queues = nw->hw_queues;
#ifdef	CONFIG_USE_HW_QUEUE /* If QUEUE_CONTROL is enabled, this must be set properly */
	hw->offchannel_tx_hw_queue = (IEEE80211_MAX_QUEUES - 1);
#endif

	for (i = 0; i < ARRAY_SIZE(nw->ntxq); i++) {
		struct nrc_txq *ntxq = &nw->ntxq[i];

		INIT_LIST_HEAD(&ntxq->list);
		ntxq->hw_queue = i;
	}

	if (nw->cap.cap_mask & WIM_SYSTEM_CAP_MULTI_VIF) {
		hw->wiphy->iface_combinations = if_comb_multi;
		hw->wiphy->n_iface_combinations = ARRAY_SIZE(if_comb_multi);
	}

	ieee80211_hw_set(hw, HAS_RATE_CONTROL);
	ieee80211_hw_set(hw, AMPDU_AGGREGATION);
	ieee80211_hw_set(hw, MFP_CAPABLE);
	ieee80211_hw_set(hw, SIGNAL_DBM);
	ieee80211_hw_set(hw, SUPPORTS_PER_STA_GTK);
	ieee80211_hw_set(hw, SINGLE_SCAN_ON_ALL_BANDS);
#ifdef CONFIG_USE_MONITOR_VIF
	ieee80211_hw_set(hw, WANT_MONITOR_VIF);
#endif
	ieee80211_hw_set(hw, CONNECTION_MONITOR);

	if (power_save >= NRC_PS_MODEMSLEEP) {
		ieee80211_hw_set(hw, SUPPORTS_PS);
#if defined(ENABLE_DYNAMIC_PS)
		/* Do NOT use HW Dynamic PS if nullfunc_enable is enabled */
		if (!nullfunc_enable) {
			ieee80211_hw_set(hw, SUPPORTS_DYNAMIC_PS);
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
			setup_timer(&nw->dynamic_ps_timer,
				nrc_ps_timeout_timer, (unsigned long)nw);
#else
			timer_setup(&nw->dynamic_ps_timer, nrc_ps_timeout_timer, 0);
#endif
		}
#endif /* ENABLE_DYNAMIC_PS */

		/* README - yj.kim 06/05/2020
		 * Target FW handles qos_null frame for power save mode
		 */
		if (nullfunc_enable) {
			ieee80211_hw_set(hw, PS_NULLFUNC_STACK);
		}
		/* ieee80211_hw_set(hw, HOST_BROADCAST_PS_BUFFERING); */
	}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	hw->wiphy->flags |=  WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	hw->wiphy->flags |=  WIPHY_FLAG_HAS_CHANNEL_SWITCH;

#ifdef CONFIG_SUPPORT_MESH_ROUTING
	if (nw->cap.bss_max_idle != 0) {
		hw->wiphy->features |= NL80211_FEATURE_INACTIVITY_TIMER;
	}
#else
	hw->wiphy->features |= NL80211_FEATURE_INACTIVITY_TIMER;
#endif

	/* hostapd ver > 2.6 need for NL80211_FEATURE_FULL_AP_CLIENT_STATE */
	hw->wiphy->features |= NL80211_FEATURE_FULL_AP_CLIENT_STATE;
#endif
	hw->vif_data_size = sizeof(struct nrc_vif);
	hw->sta_data_size = sizeof(struct nrc_sta);
#ifdef CONFIG_USE_TXQ
	hw->txq_data_size = sizeof(struct nrc_txq);
#endif
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	hw->chanctx_data_size = 0;

	/* FW handles probe-requests in AP-mode */
	hw->wiphy->flags |= WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD;
	hw->wiphy->flags |= WIPHY_FLAG_AP_UAPSD;
	hw->wiphy->flags |= WIPHY_FLAG_IBSS_RSN;

	hw->wiphy->probe_resp_offload = NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS2
		| NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS2
		| NL80211_PROBE_RESP_OFFLOAD_SUPPORT_P2P;
#endif
#ifdef CONFIG_S1G_CHANNEL
	hw->wiphy->flags |= NL80211_EXT_FEATURE_SCAN_FREQ_KHZ;
#endif
#ifdef CONFIG_USE_NEW_BAND_ENUM
	for (band = NL80211_BAND_2GHZ; band < NUM_NL80211_BANDS; band++) {
#else
	for (band = NL80211_BAND_2GHZ; band < IEEE80211_NUM_BANDS; band++) {
#endif
		sband = &nw->bands[band];

		switch (band) {
#if defined(CONFIG_S1G_CHANNEL)
		case NL80211_BAND_S1GHZ:
			memcpy(&sband->s1g_cap, &nrc_s1g_cap, sizeof(sband->s1g_cap));
			init_s1g_channels(nw);
			sband->channels = nrc_channels_s1ghz;
			sband->n_channels = nrc_get_num_channels_by_current_country();
			nrc_mac_dbg("sband->n_channels:%d", sband->n_channels);
			sband->bitrates = nrc_rates;
			sband->n_bitrates = ARRAY_SIZE(nrc_rates);
			sband->band = NL80211_BAND_S1GHZ;
			sband->ht_cap.ht_supported = true;
			hw->wiphy->bands[band] = sband;
			//continue;
			break;
#else
		case NL80211_BAND_5GHZ:
			if (!(nw->cap.cap_mask & WIM_SYSTEM_CAP_CHANNEL_5G))
				continue;
			sband->channels = nrc_channels_5ghz;
			sband->n_channels = ARRAY_SIZE(nrc_channels_5ghz);
			sband->bitrates = nrc_rates + 4;
			sband->n_bitrates = ARRAY_SIZE(nrc_rates) - 4;
			sband->band = NL80211_BAND_5GHZ;
			sband->ht_cap.ht_supported = true;
			sband->ht_cap.cap = IEEE80211_HT_CAP_SGI_20;
			sband->ht_cap.cap |= IEEE80211_HT_CAP_SGI_40;
			/* wpa_supplicant-2.10 checks HT40 allowed channel pair.
			 * possible HT40+ channels: 36, 44, 52, 60, 100, 108, 116, 124,
			 * 132, 149, 157.
			 * possible HT40- channels: 40, 48, 56, 64, 104, 112, 120, 128,
			 * 136, 153, 161.
			 * but we use other 5Ghz channels to match S1G channels.
			 *
			 * sband->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
			 */
			sband->ht_cap.cap |=
				(1 << IEEE80211_HT_CAP_RX_STBC_SHIFT);
			sband->ht_cap.ampdu_factor =
				IEEE80211_HT_MAX_AMPDU_16K;
			sband->ht_cap.ampdu_density =
				IEEE80211_HT_MPDU_DENSITY_8;
			break;

		case NL80211_BAND_2GHZ:
			if (!(nw->cap.cap_mask & WIM_SYSTEM_CAP_CHANNEL_2G))
				continue;
			sband->channels = nrc_channels_2ghz;
			sband->n_channels = ARRAY_SIZE(nrc_channels_2ghz);
			sband->bitrates = nrc_rates;
			sband->n_bitrates = ARRAY_SIZE(nrc_rates);
			sband->band = NL80211_BAND_2GHZ;
			sband->ht_cap.ht_supported = true;
			sband->ht_cap.cap = IEEE80211_HT_CAP_SGI_20;
			sband->ht_cap.cap |= IEEE80211_HT_CAP_SGI_40;
			sband->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
			sband->ht_cap.cap |= IEEE80211_HT_CAP_DSSSCCK40;
			sband->ht_cap.cap |=
				(1 << IEEE80211_HT_CAP_RX_STBC_SHIFT);
			sband->ht_cap.ampdu_factor =
				IEEE80211_HT_MAX_AMPDU_16K;
			sband->ht_cap.ampdu_density =
				IEEE80211_HT_MPDU_DENSITY_8;
			break;
#endif /* CONFIG_S1G_CHANNEL */
		default:
			continue;
		}

		memset(&sband->ht_cap.mcs, 0, sizeof(sband->ht_cap.mcs));

		sband->ht_cap.mcs.rx_mask[0] = 0xff;
		sband->ht_cap.mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;

		hw->wiphy->bands[band] = sband;
	}

	hw->wiphy->cipher_suites = nrc_cipher_supported;
	hw->wiphy->n_cipher_suites = ARRAY_SIZE(nrc_cipher_supported);

	hw->extra_tx_headroom =
		(sizeof(struct hif) + sizeof(struct frame_hdr) + 32);
#ifdef CONFIG_USE_MAX_MTU
	hw->max_mtu = IEEE80211_MAX_DATA_LEN;
#endif
	hw->max_rates = 4;
	hw->max_rate_tries = 11;

	hw->wiphy->vendor_commands = nrc_vendor_cmds;
	hw->wiphy->n_vendor_commands = ARRAY_SIZE(nrc_vendor_cmds);

	hw->wiphy->vendor_events = nrc_vendor_events;
	hw->wiphy->n_vendor_events = ARRAY_SIZE(nrc_vendor_events);

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	hw->wiphy->regulatory_flags =
		REGULATORY_CUSTOM_REG|WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
#endif

#ifdef CONFIG_PM
	if (nw->wowlan_pattern_num) { /* if configured */
		nrc_wowlan_support.n_patterns = nw->wowlan_pattern_num;
	} 
	hw->wiphy->wowlan = &nrc_wowlan_support;
#endif

	wiphy_apply_custom_regulatory(hw->wiphy, &mac80211_regdom);
	nw->alpha2[0] = '9';
	nw->alpha2[1] = '9';

#if defined(CONFIG_SUPPORT_BD)
	g_bd_valid = true;
#endif

	if (nrc_mac_is_s1g(nw)) {
		/*this is only for 802.11ah*/
		hw->wiphy->reg_notifier = nrc_reg_notifier;
	}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	hw->uapsd_queues = IEEE80211_WMM_IE_STA_QOSINFO_AC_BK
		| IEEE80211_WMM_IE_STA_QOSINFO_AC_BE
		| IEEE80211_WMM_IE_STA_QOSINFO_AC_VI
		| IEEE80211_WMM_IE_STA_QOSINFO_AC_VO;
#endif
	nw->ampdu_supported = false;
	nw->amsdu_supported = true;
	nw->block_frame = false;
	nw->ampdu_reject = false;
	nw->frag_threshold = -1;

	/* trx */
	nrc_mac_trx_init(nw);

	atomic_set(&nw->fw_state, NRC_FW_ACTIVE);

	ret = ieee80211_register_hw(hw);
	if (ret < 0) {
		nrc_mac_dbg("ieee80211_register_hw failed (%d)", ret);
		ieee80211_free_hw(hw);
		hw = NULL;
		return -EINVAL;
	}

	return 0;
}

void nrc_unregister_hw(struct nrc *nw)
{

#ifdef CONFIG_USE_TXQ
	nrc_cleanup_txq_all(nw);
#endif

	ieee80211_unregister_hw(nw->hw);
	SET_IEEE80211_DEV(nw->hw, NULL);
	nrc_hif_cleanup(nw->hif); 

	if (ieee80211_hw_check(nw->hw, SUPPORTS_DYNAMIC_PS)) {
		del_timer(&nw->dynamic_ps_timer);
	}
}

bool nrc_mac_is_s1g(struct nrc *nw)
{
#ifdef CONFIG_S1G_CHANNEL
	return 1;
#else
	return (nw->fwinfo.version != WIM_SYSTEM_VER_11N);
#endif
}

void nrc_mac_clean_txq(struct nrc *nw)
{
#ifdef CONFIG_USE_TXQ
	nrc_cleanup_txq_all(nw);
#endif
}

void nrc_mac_flush_txq(struct nrc *nw)
{
#ifdef CONFIG_USE_TXQ
	nrc_flush_txq(nw);
#endif
}

struct ieee80211_hw *nrc_mac_alloc_hw (size_t priv_data_len, const char *req_name)
{
	struct ieee80211_hw *hw;

#ifdef CONFIG_SUPPORT_HWDEV_NAME
	hw = ieee80211_alloc_hw_nm(priv_data_len, &nrc_mac80211_ops,
							req_name);
#else
	hw = ieee80211_alloc_hw(priv_data_len,
							&nrc_mac80211_ops);
#endif

	return hw;
}

void nrc_mac_free_hw (struct ieee80211_hw *hw)
{
	ieee80211_free_hw(hw);
}

/* Send loss event to proper vif */
void nrc_send_beacon_loss(struct nrc *nw)
{
	struct nrc_vif *i_vif;

	spin_lock_bh(&nw->vif_lock);
	if (nw->associated_vif == NULL) {
		nrc_mac_dbg("beacon loss event, but not associated");
		goto done;
	}
	i_vif = to_i_vif(nw->associated_vif);

	nrc_mac_dbg("beacon loss event to vif(%d)", i_vif->index);
	ieee80211_beacon_loss(nw->associated_vif);
done:
	spin_unlock_bh(&nw->vif_lock);
}

void nrc_cleanup_ba_session_sta (void *data, struct ieee80211_sta *sta)
{
	int i;

	for (i = 0; i <  IEEE80211_NUM_TIDS; i++) {
		ieee80211_stop_tx_ba_session(sta, i);
	}

	//nrc_init_sta_ba_session(sta);
}

void nrc_cleanup_ba_session_vif (struct nrc *nw, struct ieee80211_vif *vif)
{
	ieee80211_iterate_stations_atomic(nw->hw, nrc_cleanup_ba_session_sta, (void *) vif);
}


void nrc_cleanup_ba_session_all (struct nrc *nw)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(nw->vif); i++) {
		nrc_cleanup_ba_session_vif(nw, nw->vif[i]);
	}
}
