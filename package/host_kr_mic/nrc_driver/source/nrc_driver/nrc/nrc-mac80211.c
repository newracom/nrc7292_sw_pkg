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
#include "nrc-hif.h"
#include "wim.h"
#include "nrc-debug.h"
#include "nrc-vendor.h"
#include "nrc-stats.h"
#include "nrc-recovery.h"
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
#include "compat.h"
#if defined(CONFIG_SUPPORT_BD)
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "nrc-bd.h"
#endif /* defined(CONFIG_SUPPORT_BD) */

#define CHANS1G(_freq, _freq_partial) { \
	.band = NL80211_BAND_S1GHZ,			\
	.center_freq = (_freq),				\
	.hw_value = (_freq),				\
	.max_power = 20,					\
	.center_freq_fractional = (_freq_partial) \
}

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
static struct ieee80211_channel nrc_channels_s1ghz[] = {
	CHANS1G(902, 500),  /* Channel 1 */
	CHANS1G(903, 0),    /* Channel 2 */
	CHANS1G(903, 500),  /* Channel 3 */
	CHANS1G(904, 500),  /* Channel 5 */
	CHANS1G(905, 0),    /* Channel 6 */
	CHANS1G(905, 500),  /* Channel 7 */
	CHANS1G(906, 0),    /* Channel 8 */
	CHANS1G(906, 500),  /* Channel 9 */
	CHANS1G(907, 0),    /* Channel 10 */
	CHANS1G(907, 500),  /* Channel 11 */
	CHANS1G(908, 500),  /* Channel 13 */
	CHANS1G(909, 0),    /* Channel 14 */
	CHANS1G(909, 500),  /* Channel 15 */
	CHANS1G(910, 0),    /* Channel 16 */
	CHANS1G(910, 500),  /* Channel 17 */
	CHANS1G(911, 0),    /* Channel 18 */
	CHANS1G(911, 500),  /* Channel 19 */
	CHANS1G(912, 500),  /* Channel 21 */
	CHANS1G(913, 0),    /* Channel 22 */
	CHANS1G(913, 500),  /* Channel 23 */
	CHANS1G(914, 0),    /* Channel 24 */
	CHANS1G(914, 500),  /* Channel 25 */
	CHANS1G(915, 0),    /* Channel 26 */
	CHANS1G(915, 500),  /* Channel 27 */
	CHANS1G(916, 500),  /* Channel 29 */
	CHANS1G(917, 0),    /* Channel 30 */
	CHANS1G(917, 500),  /* Channel 31 */
	CHANS1G(918, 0),    /* Channel 32 */
	CHANS1G(918, 500),  /* Channel 33 */
	CHANS1G(919, 0),    /* Channel 34 */
	CHANS1G(919, 500),  /* Channel 35 */
	CHANS1G(920, 500),  /* Channel 37 */
	CHANS1G(921, 0),    /* Channel 38 */
	CHANS1G(921, 500),  /* Channel 39 */
	CHANS1G(922, 0),    /* Channel 40 */
	CHANS1G(922, 500),  /* Channel 41 */
	CHANS1G(923, 0),    /* Channel 42 */
	CHANS1G(923, 500),  /* Channel 43 */
	CHANS1G(924, 500),  /* Channel 45 */
	CHANS1G(925, 0),    /* Channel 46 */
	CHANS1G(925, 500),  /* Channel 47 */
	CHANS1G(926, 0),    /* Channel 48 */
	CHANS1G(926, 500),  /* Channel 49 */
	CHANS1G(927, 0),    /* Channel 46 */
	CHANS1G(927, 500),  /* Channel 51 */
};
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

#if defined(CONFIG_S1G_CHANNEL)
static const struct ieee80211_regdomain mac80211_regdom = {
	.n_reg_rules = 1,
	.alpha2 =  "99",
	.reg_rules = {
		REG_RULE(800, 1000, 4, 0, 20, 0),
	},
};
#else
static const struct ieee80211_regdomain mac80211_regdom = {
	.n_reg_rules = 4,
	.alpha2 =  "99",
	.reg_rules = {
		REG_RULE(2412-10, 2484+10, 40, 0, 20, 0),
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
static void nrc_init_txq(struct ieee80211_txq *txq, struct ieee80211_vif *vif)
{
	struct nrc_txq *q;

	if (!txq)
		return;

	q = (void *)txq->drv_priv;
	INIT_LIST_HEAD(&q->list);
	skb_queue_head_init(&q->queue);
	q->hw_queue = vif->hw_queue[txq->ac];
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
	list_for_each_entry_safe(cur, next, &nw->txq, list) {
		ac = cur->hw_queue;
		len += skb_queue_len(&cur->queue);
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

/* Just for debugging */
static unsigned long long fail, total;

/**
 * nrc_pull_txq - pull skb's from txq within available credits.
 *
 *
 * Return the number of remaining packets.
 */
static __u32 nrc_pull_txq(struct nrc *nw, struct nrc_txq *q)
{
	struct sk_buff *skb;
	struct ieee80211_tx_control control;
	//struct nrc_txq *q = (void *)txq->drv_priv;
	int ac = q->hw_queue, credit;
	__u32 ret;
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	int i;
#endif
	control.sta = q->sta;

	while ((ret = skb_queue_len(&q->queue)) > 0) {
		unsigned long flags;

		spin_lock_irqsave(&q->queue.lock, flags);

		total++;
		skb = skb_peek(&q->queue);
		BUG_ON(!skb);
		credit = DIV_ROUND_UP(skb->len + nw->fwinfo.tx_head_size,
				nw->fwinfo.buffer_size);
		if (credit > nrc_ac_credit(nw, ac)) {
			fail++;
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
			nrc_dbg(NRC_DBG_MAC,
			"%s: failed to push skb %p (%d/%d): fail/total=%llu/%llu\n",
					__func__, skb,
					credit,
					nrc_ac_credit(nw, ac),
					fail, total);
#endif
			/*
			 * We do not have enough credit to transmit
			 * @skb immediately. Backoff for next chances.
			 */
			spin_unlock_irqrestore(&q->queue.lock, flags);
			break;
		}

		__skb_unlink(skb, &q->queue);

		spin_unlock_irqrestore(&q->queue.lock, flags);

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
		nrc_mac_dbg("tx: ac=%d skb=%p qlen=%d credit=%d %c: ",
				ac, skb, skb_queue_len(&q->queue),
				nrc_ac_credit(nw, ac),
				in_softirq() ? 'w' : 'i');
		nrc_mac_dbg("credits: ");
		for (i = 0; i < 4; i++)
			nrc_mac_dbg("%d ", nrc_ac_credit(nw, i));
		nrc_mac_dbg("\n");
#endif
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
		nrc_mac_tx(nw->hw, &control, skb);
#else
		nrc_mac_tx(nw->hw, skb);
#endif

		/* It is no longer needed */
		/* atomic_sub(credit, &nw->tx_credit[ac]); */
	}
	return skb_queue_len(&q->queue);
}

static bool
nrc_txq_pullable(struct nrc_txq *ntxq)
{
	return (skb_queue_len(&ntxq->queue) < NR_NRC_MAX_TXQ);
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
	struct nrc_txq *local, *priv_ntxq = (void *)txq->drv_priv;
	struct sk_buff *skb;
	int max = 16;

	local = &nw->ntxq[priv_ntxq->hw_queue];

	while (nrc_txq_pullable(local) && max--) {
		skb = ieee80211_tx_dequeue(nw->hw, txq);
		if (!skb)
			break;

		spin_lock_bh(&nw->txq_lock);
		skb_queue_tail(&local->queue, skb);
		spin_unlock_bh(&nw->txq_lock);
	}

	ieee80211_stop_queue(hw, priv_ntxq->hw_queue);
	nrc_kick_txq(nw->hw);
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
void nrc_kick_txq(struct ieee80211_hw *hw)
{
	struct nrc *nw = hw->priv;
	struct nrc_txq *cur;
	int res, cnt = 0, i = 0;

	if (nw->drv_state != NRC_DRV_RUNNING)
		return;

	/* Before pushing frame to HIF, check pending TXQs exits.
	 * If so, pulling them first.
	 */
	spin_lock_bh(&nw->txq_lock);
	rcu_read_lock();

	for (i = 0; i < NRC_QUEUE_MAX; i++) {
		cur = &nw->ntxq[i];
		res = nrc_pull_txq(nw, cur);
		cnt += res;
	}

	rcu_read_unlock();
	spin_unlock_bh(&nw->txq_lock);

	ieee80211_wake_queues(nw->hw);
}

/*
 * nrc_cleanup_txq - cleanup txq
 *
 * This function is called from nrc_unregister_hw()
 */
static
void nrc_cleanup_txq(struct nrc *nw)
{
	struct nrc_txq *cur, *next;
	int res, ret;
	struct sk_buff *skb;

	spin_lock_bh(&nw->txq_lock);

	list_for_each_entry_safe(cur, next, &nw->txq, list) {
		res = nrc_pull_txq(nw, cur);
		while ((ret = skb_queue_len(&cur->queue)) > 0) {
			skb = skb_peek(&cur->queue);
			BUG_ON(!skb);

			__skb_unlink(skb, &cur->queue);
			dev_kfree_skb_any(skb);
		}
		list_del_init(&cur->list);
	}

	spin_unlock_bh(&nw->txq_lock);
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

	nrc_mac_dbg("%s: aid=%u, bssid=%pM", __func__,
		info->aid, info->bssid);

	nw->aid = info->aid;
#ifdef CONFIG_TRX_BACKOFF
	nw->ampdu_supported = 0;
#endif

	nrc_wim_skb_add_tlv(skb, WIM_TLV_AID, sizeof(info->aid), &info->aid);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_BSSID, ETH_ALEN, (void *)info->bssid);

	/* Enable later when rate adaptation is supported in the target */
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	conf = rcu_dereference(vif->chanctx_conf);
	band = conf->def.chan->band;
#else
	band = conf_chan->band;
#endif
	nrc_wim_skb_add_tlv(skb, WIM_TLV_SUPPORTED_RATES,
			    sizeof(sta->supp_rates[band]),
			    &sta->supp_rates[band]);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_BASIC_RATE,
			    sizeof(info->basic_rates), &info->basic_rates);
}


static
void nrc_assoc_h_ht(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		    struct ieee80211_bss_conf *bss_conf,
		    struct ieee80211_sta *sta, struct sk_buff *skb)
{
	struct ieee80211_sta_ht_cap *ht_cap = &sta->ht_cap;

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

	/* HT_MF, non-HT, 11b */
	if (sta->ht_cap.ht_supported)
		phymode = PHY_HT_MF;
	else if (sta->supp_rates[NL80211_BAND_2GHZ] >> 4)
		phymode = PHY_HT_NONE;
	else
		phymode = PHY_11B;

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

	b = ieee80211_beacon_get_template(hw, vif, NULL);
	if (!b)
		return -EINVAL;

	if (nw->vendor_skb) {
		need_headroom = skb_headroom(b);
		need_tailroom = nw->vendor_skb->len;

		if (skb_tailroom(b) < need_tailroom) {
			if (pskb_expand_head(b, need_headroom, need_tailroom,
						GFP_ATOMIC)) {
				nrc_mac_dbg("Fail to expand Beacon for vendor elem (need: %d)",
					need_tailroom);
				dev_kfree_skb_any(b);
				dev_kfree_skb_any(nw->vendor_skb);
				nw->vendor_skb = NULL;
				return 0;
			}
		}
		pos = skb_put(b, nw->vendor_skb->len);
		memcpy(pos, nw->vendor_skb->data, nw->vendor_skb->len);
	}

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);
	pos = nrc_wim_skb_add_tlv(skb, WIM_TLV_BEACON, b->len, b->data);

	dev_kfree_skb_any(b);
	return nrc_xmit_wim_request(nw, skb);
}

static int nrc_mac_start(struct ieee80211_hw *hw)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;

	nw->drv_state = NRC_DRV_RUNNING;
	nw->aid = 0;

	skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET,
				tlv_len(sizeof(u16)) + tlv_len(ETH_ALEN));
	nrc_wim_set_aid(nw, skb, nw->aid);
	nrc_wim_add_mac_addr(nw, skb, nw->mac_addr[0].addr);

	if (nrc_mac_is_s1g(nw)) {
		nrc_wim_set_ndp_preq(nw, skb, ndp_preq);
	}

	nrc_xmit_wim_request(nw, skb);
	nrc_recovery_wdt_kick(nw);

	return 0;
}

static void nrc_mac_stop(struct ieee80211_hw *hw)
{
	struct nrc *nw = hw->priv;

	nw->drv_state = NRC_DRV_STOP;
	nrc_recovery_wdt_stop(nw);
	nrc_xmit_wim_simple_request(nw, WIM_CMD_STOP);

	nrc_hif_suspend(nw->hif);
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
		}
	}

	spin_unlock_bh(&nw->vif_lock);
	/* All h/w vifs are in use */
	return -EBUSY;
}

static void nrc_free_vif_index(struct nrc *nw, struct ieee80211_vif *vif)
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

static void nrc_mac_roc_finish(struct work_struct *work)
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
	int hw_queue_start = 0;
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

	if (WARN_ON(nrc_alloc_vif_index(nw, vif) < 0))
		return -1;

	hw_queue_start = i_vif->index * NR_NRC_VIF_HW_QUEUE;
	if (i_vif->index)
		hw_queue_start += 2;

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	if (vif->type == NL80211_IFTYPE_AP ||
	    vif->type == NL80211_IFTYPE_WDS ||
	    vif->type == NL80211_IFTYPE_P2P_GO)
		vif->cab_queue = hw_queue_start + 4;
	else
		vif->cab_queue = IEEE80211_INVAL_HW_QUEUE;

	vif->hw_queue[IEEE80211_AC_VO] = hw_queue_start + 3;
	vif->hw_queue[IEEE80211_AC_VI] = hw_queue_start + 2;
	vif->hw_queue[IEEE80211_AC_BE] = hw_queue_start + 1;
	vif->hw_queue[IEEE80211_AC_BK] = hw_queue_start + 0;
#endif

	nrc_init_txq(vif->txq, vif);

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

	nrc_mac_dbg("%s %s addr:%pM hwindex:%d", __func__,
			i_vif->dev ? netdev_name(i_vif->dev):"NULL", vif->addr,
			i_vif->index);

 out:

	if (vif->p2p)
		nrc_wim_set_p2p_addr(nw, vif);
	else
		nrc_wim_set_mac_addr(nw, vif);

	nrc_wim_set_sta_type(nw, vif);

	return 0;
}

static int nrc_mac_change_interface(struct ieee80211_hw *hw,
				    struct ieee80211_vif *vif,
				    enum nl80211_iftype newtype,
				    bool newp2p)
{
	struct nrc *nw = hw->priv;

	/*
	 * interface may change from non-AP to AP in
	 * which case this needs to be set up again
	 */
	vif->type = newtype;
	vif->p2p = newp2p;

	nrc_wim_set_mac_addr(nw, vif);
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
		    i_vif->dev ? netdev_name(i_vif->dev):"NULL", vif->addr,
		    i_vif->index,
		    iftype_string(vif->type));

#ifdef CONFIG_USE_SCAN_TIMEOUT
	cancel_delayed_work_sync(&i_vif->scan_timeout);
#endif
	nrc_wim_unset_sta_type(nw, vif);
	nrc_free_vif_index(hw->priv, vif);
	i_vif->dev = NULL;
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
#endif /* CONFIG_S1G_CHANNEL */
	default:
		return CH_WIDTH_20;
	}
}

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
static void nrc_mac_add_tlv_channel(struct sk_buff *skb,
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
	struct wim_s1g_channel_param param;

	param.pr_freq = FREQ_TO_100KHZ(chandef->chan->center_freq,
				chandef->chan->center_freq_fractional);
	param.op_freq = FREQ_TO_100KHZ(chandef->center_freq1,
				chandef->center_freq1_fractional);
	param.width = get_wim_channel_width(chandef->width);
	pr_err("nrc: pri(%d), op(%d), width(%d)\n", param.pr_freq,
			param.op_freq, param.width);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_S1G_CHANNEL, sizeof(param),
			&param);
#endif
}
#else
static void nrc_mac_add_tlv_channel(struct sk_buff *skb,
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
#endif

static int nrc_mac_config(struct ieee80211_hw *hw, u32 changed)
{
	struct nrc *nw = hw->priv;
	int ret = 0;
	struct sk_buff *skb;
	bool ps_enable = false;

	skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, WIM_MAX_SIZE);

	if (changed & IEEE80211_CONF_CHANGE_CHANNEL) {
		/* TODO: Remove the following line */
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
		nw->band = hw->conf.chandef.chan->band;
		nw->center_freq = hw->conf.chandef.chan->center_freq;
#else
		nw->band = hw->conf.channel->band;
		nw->center_freq = hw->conf.channel->center_freq;
#endif

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
		nrc_mac_add_tlv_channel(skb, &hw->conf.chandef);
#else
		nrc_mac_add_tlv_channel(skb, &hw->conf);
#endif
		/* TODO: band (2G, 5G, etc) and bandwidth (20MHz, 40MHz, etc) */
	}

	if (changed & IEEE80211_CONF_CHANGE_PS) {
		if (hw->conf.flags & IEEE80211_CONF_PS) {
			ps_enable = true;
			nw->ps_enabled = true;
		} else {
			ps_enable = false;
			nw->ps_enabled = false;
		}

		nrc_wim_skb_add_tlv(skb,
				WIM_TLV_PS_ENABLE,
				sizeof(ps_enable),
				&ps_enable);

		nrc_ps_dbg("WIM_TLV_PS_ENABLE(%d)", ps_enable);

	}

	/* i.e., if we added at least one tlv */
	if (skb->len > sizeof(struct wim))
		ret = nrc_xmit_wim_request(nw, skb);

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

static void nrc_mac_bss_info_changed(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif,
				     struct ieee80211_bss_conf *info,
				     u32 changed)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
	int ret;

	nrc_mac_dbg("%s: changed=0x%x", __func__, changed);

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, WIM_MAX_SIZE);

	if (changed & BSS_CHANGED_ASSOC) {
		if (info->assoc)
			nrc_bss_assoc(hw, vif, info, skb);
	}

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

	if (changed & BSS_CHANGED_BSSID) {
		nrc_mac_dbg("bssid=%pM", info->bssid);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_BSSID, ETH_ALEN,
				    (void *)info->bssid);
	}

	if (changed & BSS_CHANGED_BEACON_INT) {
		u16 bi = info->beacon_int, short_bi = 0;
		u8 dtim_period = info->dtim_period;

		nrc_mac_dbg("beacon interval: %u, dtim_period: %u",
				info->beacon_int,
				info->dtim_period);

		if (nrc_mac_is_s1g(nw)) {
			short_bi = info->beacon_int;
			bi = DEF_CFG_S1G_SHORT_BEACON_COUNT * short_bi;
		}

		nrc_wim_skb_add_tlv(skb, WIM_TLV_BCN_INTV, sizeof(bi), &bi);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_SHORT_BCN_INTV,
				    sizeof(short_bi), &short_bi);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_DTIM_PERIOD,
				sizeof(dtim_period), &dtim_period);
	}

	if (changed & BSS_CHANGED_BEACON_ENABLED) {
		u16 bi = info->beacon_int, short_bi = 0;
		u8 dtim_period = info->dtim_period;

		nrc_mac_dbg("beacon: %s, interval=%u, dtim_period:%u",
			    info->enable_beacon ? "enabled" : "disabled",
			    info->beacon_int,
				info->dtim_period);

		if (nrc_mac_is_s1g(nw)) {
			short_bi = info->beacon_int;
			bi = DEF_CFG_S1G_SHORT_BEACON_COUNT * short_bi;
		}

		nrc_wim_skb_add_tlv(skb, WIM_TLV_BEACON_ENABLE,
				    sizeof(info->enable_beacon),
				    &info->enable_beacon);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_BCN_INTV, sizeof(bi), &bi);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_SHORT_BCN_INTV,
				    sizeof(short_bi), &short_bi);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_DTIM_PERIOD,
				sizeof(dtim_period), &dtim_period);
	}

	if (changed & BSS_CHANGED_BEACON)
		nrc_vendor_update_beacon(hw, vif);
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	if (changed & BSS_CHANGED_SSID) {
		nrc_mac_dbg("ssid=%s", info->ssid);

		nrc_wim_skb_add_tlv(skb, WIM_TLV_SSID, info->ssid_len,
				info->ssid);
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
	if (changed & BSS_CHANGED_QOS)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_QOS");
	if (changed & BSS_CHANGED_IDLE)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_IDLE");
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	if (changed & BSS_CHANGED_TXPOWER)
		nrc_mac_dbg("%s(changed:%s[PW=%d])", __func__,
			"BSS_CHANGED_TXPOWER", info->txpower);
	if (changed & BSS_CHANGED_P2P_PS) {
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_P2P_PS");
		nrc_mac_update_p2p_ps(skb, vif);
	}
	if (changed & BSS_CHANGED_BEACON_INFO)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_BEACON_INFO");
	if (changed & BSS_CHANGED_BANDWIDTH)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_BANDWIDTH");
#ifdef CONFIG_SUPPORT_IFTYPE_OCB
	if (changed & BSS_CHANGED_OCB)
		nrc_mac_dbg("%s(changed:%s)", __func__,
				"BSS_CHANGED_OCB");
#endif
#endif
	ret = nrc_xmit_wim_request(nw, skb);
	if (ret < 0)
		nrc_mac_dbg("failed to transmit a wim request");

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
	nrc_stats_print();

	nrc_wim_change_sta(nw, vif, sta, WIM_STA_CMD_ADD, 0);

#ifdef CONFIG_USE_TXQ
	/* Initialize txq driver data.
	 * Assumptions are:
	 * (1) per-sta per-tid txq is not in use up to this point (associated).
	 * (2) EAPOL frames do not resort to txq.
	 */
	for (i = 0; i < ARRAY_SIZE(sta->txq); i++)
		nrc_init_txq(sta->txq[i], vif);
#endif
	return 0;
}

static
int nrc_mac_sta_remove(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta)
{
	nrc_stats_del(sta->addr);
	nrc_stats_print();
	nrc_wim_change_sta(hw->priv, vif, sta, WIM_STA_CMD_REMOVE, 0);

	return 0;
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

	nrc_dbg(NRC_DBG_STATE, "%s: sta:%pM, %d->%d", __func__, sta->addr,
		    old_state, new_state);

	i_sta->state = new_state;

#define state_changed(old, new) \
(old_state == IEEE80211_STA_##old && new_state == IEEE80211_STA_##new)

	if (state_changed(NOTEXIST, NONE)) {

		memset(i_sta, 0, sizeof(*i_sta));
		i_sta->nw = hw->priv;
		i_sta->vif = vif;

		INIT_LIST_HEAD(&i_sta->list);

		spin_lock_irqsave(&i_vif->preassoc_sta_lock, flags);
		list_add_tail(&i_sta->list, &i_vif->preassoc_sta_list);
		spin_unlock_irqrestore(&i_vif->preassoc_sta_lock, flags);

	} else if (state_changed(NONE, AUTH)) {

	} else if (state_changed(AUTH, ASSOC)) {

		spin_lock_irqsave(&i_vif->preassoc_sta_lock, flags);
		list_del_init(&i_sta->list);
		spin_unlock_irqrestore(&i_vif->preassoc_sta_lock, flags);

		i_sta->vif = vif;
		nrc_mac_sta_add(hw, vif, sta);

	} else if (state_changed(ASSOC, AUTH))
		nrc_mac_sta_remove(hw, vif, sta);

	nrc_wim_change_sta_state(hw->priv, vif, sta, new_state);

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
		struct sk_buff *b = ieee80211_beacon_get_template(hw,
				i_sta->vif, NULL);
		if (b) {
			nrc_wim_skb_add_tlv(skb, WIM_TLV_BEACON, b->len,
					b->data);
			dev_kfree_skb_any(b);
		}
	}

	return nrc_xmit_wim_request(nw, skb);
}

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
static
int nrc_mac_conf_tx(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		    u16 ac, const struct ieee80211_tx_queue_params *params)
#else
int nrc_mac_conf_tx(struct ieee80211_hw *hw,
		    u16 ac, const struct ieee80211_tx_queue_params *params)
#endif
{
	struct nrc *nw = hw->priv;
	struct wim_tx_queue_param *q;
	struct sk_buff *skb;
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#else
	struct ieee80211_vif *vif = nw->vif[0];
#endif

	nrc_mac_dbg("%s: ac=%d txop=%d cw_min=%d cw_max=%d aifs=%d uapsd=%d",
		    __func__, ac, params->txop, params->cw_min,
		    params->cw_max, params->aifs, params->uapsd);

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, tlv_len(sizeof(*q)));

	q = nrc_wim_skb_add_tlv(skb, WIM_TLV_TXQ_PARAM, sizeof(*q), NULL);
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	q->ac = vif->hw_queue[ac];
#else
	q->ac = (u8)ac;
#endif
	q->txop = params->txop;
	q->cw_min = params->cw_min;
	q->cw_max = params->cw_max;
	q->aifsn = params->aifs;
	q->uapsd = params->uapsd;
	q->sta_type = 0; /* FIXME */

	return nrc_xmit_wim_request(nw, skb);
}

static int nrc_mac_get_survey(struct ieee80211_hw *hw, int idx,
			      struct survey_info *survey)
{
	struct ieee80211_conf *conf = &hw->conf;

	nrc_mac_dbg("%s (idx=%d)", __func__, idx);

	if (idx != 0)
		return -ENOENT;

	/* Current channel */
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	survey->channel = conf->chandef.chan;
#else
	survey->channel = conf->channel;
#endif

	/*
	 * Magically conjured noise level
	 * --- this is only ok for simulated hardware.
	 *
	 * A real driver which cannot determine the real channel noise MUST NOT
	 * report any noise, especially not a magically conjured one :-)
	 */
	survey->filled = SURVEY_INFO_NOISE_DBM;
	survey->noise = -92;

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
#ifdef CONFIG_USE_IEEE80211_AMPDU_PARAMS
	enum ieee80211_ampdu_mlme_action action = params->action;
	struct ieee80211_sta *sta = params->sta;
	u16 tid = params->tid;

#endif
	switch (action) {
	case IEEE80211_AMPDU_TX_START:
		if (!nw->ampdu_supported || !sta->ht_cap.ht_supported)
			return -EOPNOTSUPP;

		if (nrc_wim_ampdu_action(nw, vif, WIM_AMPDU_TX_START, sta, tid))
			return -EOPNOTSUPP;

		ieee80211_start_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		break;
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	case IEEE80211_AMPDU_TX_STOP_FLUSH:
	case IEEE80211_AMPDU_TX_STOP_FLUSH_CONT:
		break;
	case IEEE80211_AMPDU_TX_STOP_CONT:
		if (nrc_wim_ampdu_action(nw, vif, WIM_AMPDU_TX_STOP, sta, tid))
			return -EOPNOTSUPP;

		ieee80211_stop_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		break;
#endif
	case IEEE80211_AMPDU_TX_OPERATIONAL:
		return 0;
	case IEEE80211_AMPDU_RX_START:
	case IEEE80211_AMPDU_RX_STOP:
		if (nw->ampdu_reject)
			return -EOPNOTSUPP;
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

	if (nw->scan_mode != NRC_SCAN_MODE_IDLE)
		nrc_mac_cancel_hw_scan(hw, vif);

	scan_to += 120 * req->n_channels;

	nrc_wim_hw_scan(nw, vif, req, ies);

	change_scan_mode(nw, NRC_SCAN_MODE_SCANNING);

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

	return le64_to_cpu(_nrc_mac_get_tsf(nw));
}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
static void nrc_mac_set_tsf(struct ieee80211_hw *hw,
			    struct ieee80211_vif *vif, u64 tsf)
#else
static void nrc_mac_set_tsf(struct ieee80211_hw *hw,
			    u64 tsf)
#endif
{
}

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
	nrc_wim_skb_add_tlv(skb, WIM_TLV_RTS_THREASHOLD, sizeof(u32), &value);
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

	if (!(nw->cap.vif_caps[vif_id].cap_mask & WIM_SYSTEM_CAP_HWSEC)) {
		pr_err("failed to set caps");
		return 1;
	}

	if (sta == NULL && vif->type == NL80211_IFTYPE_STATION &&
			!(key->flags & IEEE80211_KEY_FLAG_PAIRWISE)) {
		sta = ieee80211_find_sta(vif, vif->bss_conf.bssid);
	}

	if (vif->type == NL80211_IFTYPE_MESH_POINT)
		return 1;

	/* Record key information to per-STA driver data structure for RX */
	/* TODO: DISABLE_KEY -> later */
	if (cmd == SET_KEY) {
		if (key->cipher == WLAN_CIPHER_SUITE_AES_CMAC ||
			key->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_128 ||
			key->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_256) {
			key->flags |= IEEE80211_KEY_FLAG_SW_MGMT_TX;
			return 1;
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
				  vif->type == NL80211_IFTYPE_P2P_GO));
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
#else
	struct ieee80211_conf *chan_to_follow =
		(struct ieee80211_conf *)data;
	struct wireless_dev *wdev = i_vif->dev->ieee80211_ptr;
#endif

	if (!wdev)
		return;

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	if (wdev->chandef.chan &&
		wdev->chandef.chan->center_freq ==
		chan_to_follow->chan->center_freq)
#else
	if (wdev->channel &&
		wdev->channel->center_freq ==
		chan_to_follow->channel->center_freq)
#endif
		return;

	if (!(vif->type == NL80211_IFTYPE_STATION ||
		vif->type == NL80211_IFTYPE_MESH_POINT))
		return;

	if (vif->bss_conf.assoc)
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
		chandef.width = NL80211_CHAN_WIDTH_20;
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

static int nrc_mac_cancel_roc(struct ieee80211_hw *hw)
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

	nrc_wim_skb_add_tlv(skb,
			WIM_TLV_CHANNEL,
			sizeof(new_ctx->def.chan->center_freq),
			&new_ctx->def.chan->center_freq);

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

static const struct ieee80211_ops nrc_mac80211_ops = {
	.tx = nrc_mac_tx,
	.start = nrc_mac_start,
	.stop = nrc_mac_stop,
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
	.switch_vif_chanctx = nrc_mac_switch_vif_chanctx
#endif
};

struct nrc *nrc_alloc_hw(struct platform_device *pdev)
{
	struct ieee80211_hw *hw;
	struct nrc *nw;

#ifdef CONFIG_SUPPORT_HWDEV_NAME
	hw = ieee80211_alloc_hw_nm(sizeof(struct nrc), &nrc_mac80211_ops,
				   dev_name(&pdev->dev));
#else
	hw = ieee80211_alloc_hw(sizeof(struct nrc),
				   &nrc_mac80211_ops);
#endif

	if (!hw)
		return NULL;

	nw = hw->priv;
	nw->hw = hw;
	nw->pdev = pdev;

	return nw;
}

void nrc_free_hw(struct nrc *nw)
{
	ieee80211_free_hw(nw->hw);
	nw->hw = NULL; /* MERGE CHECK */
}

#ifdef CONFIG_NEW_REG_NOTIFIER
static void nrc_reg_notifier(struct wiphy *wiphy,
		struct regulatory_request *request)
#else
static int nrc_reg_notifier(struct wiphy *wiphy,
		struct regulatory_request *request)
#endif
{
	int i;
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
	//struct wim_bd_param *tmp;
	struct wim_bd_param *bd_param;

	nrc_mac_dbg("info: cfg80211 regulatory domain callback for %c%c",
			request->alpha2[0], request->alpha2[1]);
	nrc_mac_dbg("request->initiator:%d", request->initiator);

	if (request->alpha2[0] == '0' && request->alpha2[1] == '0')
#ifdef CONFIG_NEW_REG_NOTIFIER
		return;
#else
		return 0;
#endif

#if defined(CONFIG_SUPPORT_BD)
	//Read board data and save buffer
	bd_param = nrc_read_bd_tx_pwr(request->alpha2);
	if(bd_param) {
		nrc_dbg(NRC_DBG_STATE,"type %04X length %04X checksum %04X",
				bd_param->type, bd_param->length, bd_param->checksum);
		for(i=0; i < bd_param->length - 2;) {
			nrc_dbg(NRC_DBG_STATE,"%02X %02X %02X %02X %02X %02X %02X %02X",
				(bd_param->value[i]),
				(bd_param->value[i+1]),
				(bd_param->value[i+2]),
				(bd_param->value[i+3]),
				(bd_param->value[i+4]),
				(bd_param->value[i+5]),
				(bd_param->value[i+6]),
				(bd_param->value[i+7])
				);
			i += 8;
		}
	}
#endif /* defined(CONFIG_SUPPORT_BD) */

	if (request->alpha2[0] != nw->alpha2[0] ||
			request->alpha2[1] != nw->alpha2[1]) {
		nw->alpha2[0] = request->alpha2[0];
		nw->alpha2[1] = request->alpha2[1];

		skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_COUNTRY_CODE,
				sizeof(u16), request->alpha2);

#if defined(CONFIG_SUPPORT_BD)
 		if(bd_param) {
			nrc_wim_skb_add_tlv(skb, WIM_TLV_BD, sizeof(*bd_param), bd_param);
			kfree(bd_param);
 		}
 #endif /* defined(CONFIG_SUPPORT_BD) */

		nrc_xmit_wim_request(nw, skb);
	}

#ifdef CONFIG_NEW_REG_NOTIFIER
	return;
#else
	return 0;
#endif
}

static int nrc_vendor_update(struct nrc *nw, u8 subcmd,
				const u8 *data, int data_len)
{
	const int OUI_LEN = 3;
	const int MAX_DATALEN = 255;
	int new_elem_len = data_len + 2 /* EID + LEN */ + OUI_LEN + 1;
	u8 *pos;

	if (!data || data_len < 1 || (data_len + OUI_LEN + 1) > MAX_DATALEN)
		return -EINVAL;

	if (!nw->vendor_skb)
		nw->vendor_skb = dev_alloc_skb(IEEE80211_MAX_FRAME_LEN);

	pos = (u8 *) cfg80211_find_vendor_ie(OUI_NRC, subcmd,
			nw->vendor_skb->data, nw->vendor_skb->len);

	if (pos) {
		u8 len = *(pos + 1);
		/* Remove element first */
		memmove(pos, pos + len + 2, nw->vendor_skb->len -
			(pos - nw->vendor_skb->data) - (len + 2));
		skb_trim(nw->vendor_skb, nw->vendor_skb->len - (len + 2));
	}
	/* Append new data */
	pos = skb_put(nw->vendor_skb, new_elem_len);
	*pos++ = WLAN_EID_VENDOR_SPECIFIC;
	*pos++ = data_len + OUI_LEN + 1;
	*pos++ = 0xFC;
	*pos++ = 0xFF;
	*pos++ = 0xAA;
	*pos++ = subcmd;
	memcpy(pos, data, data_len);

	print_hex_dump(KERN_DEBUG, "new vendor elem: ", DUMP_PREFIX_NONE,
		   16, 1, nw->vendor_skb->data, nw->vendor_skb->len, false);

	return 0;
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
	/* Update beacon */
	return nrc_vendor_update_beacon(hw, vif);
}

static int nrc_vendor_cmd_announce1(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_OUI_SUBCMD_ANNOUNCE1,
				 data, data_len);
}

static int nrc_vendor_cmd_announce2(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_OUI_SUBCMD_ANNOUNCE2,
				 data, data_len);
}

static int nrc_vendor_cmd_announce3(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_OUI_SUBCMD_ANNOUNCE3,
				 data, data_len);
}

static int nrc_vendor_cmd_announce4(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_OUI_SUBCMD_ANNOUNCE4,
				 data, data_len);
}

static int nrc_vendor_cmd_announce5(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	return nrc_vendor_cmd_append(wiphy, wdev, NRC_OUI_SUBCMD_ANNOUNCE5,
				 data, data_len);
}

static struct wiphy_vendor_command nrc_vendor_cmds[] = {
	{
		.info = { .vendor_id = OUI_NRC,
			  .subcmd = NRC_OUI_SUBCMD_ANNOUNCE1 },
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce1
	},
	{
		.info = { .vendor_id = OUI_NRC,
			  .subcmd = NRC_OUI_SUBCMD_ANNOUNCE2 },
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce2
	},
	{
		.info = { .vendor_id = OUI_NRC,
			  .subcmd = NRC_OUI_SUBCMD_ANNOUNCE3 },
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce3
	},
	{
		.info = { .vendor_id = OUI_NRC,
			  .subcmd = NRC_OUI_SUBCMD_ANNOUNCE4 },
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce4
	},
	{
		.info = { .vendor_id = OUI_NRC,
			  .subcmd = NRC_OUI_SUBCMD_ANNOUNCE5 },
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = nrc_vendor_cmd_announce5
	},

};

static const struct nl80211_vendor_cmd_info nrc_vendor_events[] = {
	{
		.vendor_id = OUI_NRC,
		.subcmd = NRC_VENDOR_EVENT_ANNOUNCE
	},
};


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

	mutex_init(&nw->target_mtx);
	mutex_init(&nw->state_mtx);

	for (i = 0; i < NR_NRC_VIF; i++) {
		if (!nw->has_macaddr[i])
			set_mac_address(&nw->mac_addr[i], i);
	}

	SET_IEEE80211_PERM_ADDR(hw, nw->mac_addr[0].addr);
	hw->wiphy->n_addresses = NR_NRC_VIF;
	hw->wiphy->addresses = nw->mac_addr;

	SET_IEEE80211_DEV(hw, &nw->pdev->dev);

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
		BIT(NL80211_IFTYPE_MONITOR);
	hw->queues = IEEE80211_MAX_QUEUES;
#ifdef	CONFIG_USE_HW_QUEUE
	hw->offchannel_tx_hw_queue = (IEEE80211_MAX_QUEUES - 1);
#endif

	for (i = 0; i < ARRAY_SIZE(nw->ntxq); i++) {
		struct nrc_txq *ntxq = &nw->ntxq[i];

		INIT_LIST_HEAD(&ntxq->list);
		skb_queue_head_init(&ntxq->queue);
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

#ifdef CONFIG_USE_MONITOR_VIF
	ieee80211_hw_set(hw, WANT_MONITOR_VIF);
#endif

	if (disable_cqm) {
		nrc_mac_dbg("CQM is disabled");
		ieee80211_hw_set(hw, CONNECTION_MONITOR);
	}

	if (power_save) {
		ieee80211_hw_set(hw, SUPPORTS_PS);
		/* README - yj.kim 06/05/2020
		 * Target FW handles qos_null frame for power save mode
		 */
		/* ieee80211_hw_set(hw, PS_NULLFUNC_STACK); */
		/* ieee80211_hw_set(hw, HOST_BROADCAST_PS_BUFFERING); */
	}

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	hw->wiphy->flags |=  WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	hw->wiphy->flags |=  WIPHY_FLAG_HAS_CHANNEL_SWITCH;

	hw->wiphy->features |= NL80211_FEATURE_INACTIVITY_TIMER;
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

#ifdef CONFIG_USE_NEW_BAND_ENUM
	for (band = NL80211_BAND_2GHZ; band < NUM_NL80211_BANDS; band++) {
#else
	for (band = NL80211_BAND_2GHZ; band < IEEE80211_NUM_BANDS; band++) {
#endif
		sband = &nw->bands[band];

		switch (band) {
#if defined(CONFIG_S1G_CHANNEL)
		case NL80211_BAND_S1GHZ:
			sband->channels = nrc_channels_s1ghz;
			sband->n_channels = ARRAY_SIZE(nrc_channels_s1ghz);
			sband->bitrates = nrc_rates;
			sband->n_bitrates = ARRAY_SIZE(nrc_rates);

			sband->s1g_cap.s1g_supported = true;
			hw->wiphy->bands[band] = sband;
			continue;
			break;
#else
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
			sband->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
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

	wiphy_apply_custom_regulatory(hw->wiphy, &mac80211_regdom);
	nw->alpha2[0] = '9';
	nw->alpha2[1] = '9';

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

	INIT_DELAYED_WORK(&nw->roc_finish, nrc_mac_roc_finish);

	/* trx */
	nrc_mac_trx_init(nw);
	spin_lock_init(&nw->vif_lock);

	init_completion(&nw->hif_tx_stopped);
	init_completion(&nw->hif_rx_stopped);
	init_completion(&nw->hif_irq_stopped);
	atomic_set(&nw->fw_state, NRC_FW_ACTIVE);

	ret = ieee80211_register_hw(hw);
	if (ret < 0) {
		nrc_mac_dbg("ieee80211_register_hw failed (%d)", ret);
		ieee80211_free_hw(hw);
		hw = NULL;
		return -EINVAL;
	}

	/* debugfs */
	nrc_init_debugfs(nw);

	return 0;
}

void nrc_unregister_hw(struct nrc *nw)
{
	ieee80211_unregister_hw(nw->hw);
	SET_IEEE80211_DEV(nw->hw, NULL);

	flush_workqueue(nw->workqueue);
	destroy_workqueue(nw->workqueue);
	nw->workqueue = NULL;
	if (nw->vendor_skb) {
		dev_kfree_skb_any(nw->vendor_skb);
		nw->vendor_skb = NULL;
	}

	nrc_cleanup_txq(nw);
}

bool nrc_mac_is_s1g(struct nrc *nw)
{
	return (nw->fwinfo.version != WIM_SYSTEM_VER_11N);
}
