/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * TX/RX routines
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

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <net/dst.h>
#include <net/xfrm.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <net/genetlink.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include "nrc-mac80211.h"
#include "nrc-hif.h"
#include "wim.h"
#include "nrc-debug.h"
#include "nrc-stats.h"
#include "compat.h"
#include "nrc-vendor.h"
#ifdef CONFIG_S1G_CHANNEL
#include "nrc-s1g.h"
#endif

#define WLAN_FC_GET_TYPE(fc)	(((fc) & 0x000c) >> 2)
#define WLAN_FC_GET_STYPE(fc)	(((fc) & 0x00f0) >> 4)

static bool is_eapol(struct sk_buff *skb);
static void setup_ba_session(struct nrc *nw, struct ieee80211_vif *vif, struct sk_buff *skb);

/* TX */
#define USF2SF(usf)	((usf == 0) ? 1 : (usf == 1) ? 10 : (usf == 2) ? 1000 : 10000)

bool nrc_is_valid_vif (struct nrc *nw, struct ieee80211_vif *vif)
{
	u8 i;
	for (i = 0; i < ARRAY_SIZE(nw->vif); i++) {
		if (vif == nw->vif[i])
			return true;
	}
	nrc_mac_dbg("[%s] Invallid vif", __func__);
	return false;
}

/**
 * nrc_mac_tx() - main tx routine
 *
 * @hw: the hardware
 * @control: tx control data
 * @skb: the skb
 */

#ifdef CONFIG_SUPPORT_NEW_MAC_TX
void nrc_mac_tx(struct ieee80211_hw *hw, struct ieee80211_tx_control *control,
		struct sk_buff *skb)
#else
void nrc_mac_tx(struct ieee80211_hw *hw,
		struct sk_buff *skb)
#endif
{
	struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
	struct nrc_trx_handler *h;
	int res = 0;
	struct nrc_trx_data tx = {
		.nw = hw->priv,
		.vif = txi->control.vif,
#ifdef CONFIG_SUPPORT_TX_CONTROL
		.sta = control->sta,
#else
		.sta = txi->control.sta,
#endif
		.skb = skb,
		.result = 0,
	};

	struct ieee80211_hdr *mh = (void*)skb->data;
	s8 vif_id = 0;

	if (!nrc_is_valid_vif(tx.nw, tx.vif))
		goto txh_out;

	vif_id = hw_vifindex(tx.vif);

	/* Set BA Session */
	if (ampdu_mode == NRC_AMPDU_AUTO) {
		if (ieee80211_is_data_qos(mh->frame_control) &&
			!is_multicast_ether_addr(mh->addr1) && !is_eapol(tx.skb)) {
			setup_ba_session(tx.nw, tx.vif, tx.skb);
		}
	}

	/* Only for PS STA */
	if (tx.nw->vif[vif_id]->type == NL80211_IFTYPE_STATION) {
#ifndef CONFIG_USE_TXQ
		if (tx.nw->drv_state == NRC_DRV_PS)
			nrc_hif_wake_target(tx.nw->hif);
#endif
		if (power_save == NRC_PS_MODEMSLEEP) {
			if (tx.nw->ps_modem_enabled) {
				struct sk_buff *skb1;
				struct wim_pm_param *p;
				/**
				 * ps_drv_state is used to check the driver's own power save state
				 * when mac80211 is aware of the driver is in power saving state.
				 * for example,
				 * when driver needs to send qos null frame by keep-alive timer,
				 * driver has to wake-up the target silently before sending frames.
				 */

				tx.nw->ps_drv_state = false;
				nrc_ps_dbg("[%s] Wakeup MODEMSLEEP...\n", __func__);
				skb1 = nrc_wim_alloc_skb(tx.nw, WIM_CMD_SET,
						tlv_len(sizeof(struct wim_pm_param)));
				p = nrc_wim_skb_add_tlv(skb1, WIM_TLV_PS_ENABLE,
								sizeof(struct wim_pm_param), NULL);
				memset(p, 0, sizeof(struct wim_pm_param));
				p->ps_mode = power_save;
				p->ps_enable = tx.nw->ps_drv_state;
				nrc_xmit_wim_request(tx.nw, skb1);
				tx.nw->ps_modem_enabled = false;
			}
		}

		if (power_save >= NRC_PS_DEEPSLEEP_TIM) {
			if (tx.nw->drv_state == NRC_DRV_PS) {
				memset(&tx.nw->d_deauth, 0, sizeof(struct nrc_delayed_deauth));
				if (ieee80211_is_deauth(mh->frame_control)) {
					nrc_common_dbg("[%s,L%d][deauth] drv_state:%d\n", __func__, __LINE__, tx.nw->drv_state);
					memcpy(&tx.nw->d_deauth.v, tx.vif, sizeof(struct ieee80211_vif));
					memcpy(&tx.nw->d_deauth.s, tx.sta, sizeof(struct ieee80211_sta));
					nrc_hif_wake_target(tx.nw->hif);
					nrc_hif_cleanup(tx.nw->hif);
					tx.nw->d_deauth.deauth_frm = skb_copy(skb, GFP_ATOMIC);
					atomic_set(&tx.nw->d_deauth.delayed_deauth, 1);
					tx.nw->d_deauth.vif_index = hw_vifindex(tx.vif);
					tx.nw->d_deauth.aid = tx.sta->aid;
					goto txh_out;
				}
				if (ieee80211_is_qos_nullfunc(mh->frame_control)) {
					nrc_ps_dbg("[%s,L%d][qos_null] make target wake for keep-alive\n", __func__, __LINE__);
					nrc_hif_wake_target(tx.nw->hif);
					goto txh_out;
				}
			} else if (tx.nw->drv_state != NRC_DRV_RUNNING) {
				/*
				 * Sometimes a deauth frame is transferred while nrc_unregister_hw() is 
				 * in processing(NRC_DRV_CLOSING) at the result of 'ifconfig down'.
				 * We don't need to handle this.
				 */
				if (ieee80211_is_deauth(mh->frame_control)) {
					nrc_ps_dbg("[%s,L%d][deauth] drv_state:%d ===> discard!!!\n", __func__, __LINE__, tx.nw->drv_state);
				}
				goto txh_out;
			}
		}

		if (ieee80211_hw_check(hw, SUPPORTS_DYNAMIC_PS) &&
			hw->conf.dynamic_ps_timeout > 0) {
			mod_timer(&tx.nw->dynamic_ps_timer,
				jiffies + msecs_to_jiffies(hw->conf.dynamic_ps_timeout));
		}
	 }//if (tx.nw->vif[vif_id]->type == NL80211_IFTYPE_STATION)

	/* Iterate over tx handlers */
	for (h = &__tx_h_start; h < &__tx_h_end; h++) {

		if (!(h->vif_types & BIT(tx.vif->type)))
			continue;

		res = h->handler(&tx);
		if (res < 0)
			goto txh_out;
	}

	if (!atomic_read(&tx.nw->d_deauth.delayed_deauth))
		nrc_xmit_frame(tx.nw, vif_id, (!!tx.sta ? tx.sta->aid : 0), tx.skb);

	return;

txh_out:
	if (tx.skb)
		dev_kfree_skb(tx.skb);
}

static bool is_eapol(struct sk_buff *skb)
{
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
	u16 hdrlen = ieee80211_hdrlen(hdr->frame_control);

	struct {
		u8 hdr[6] __aligned(2);
		__be16 proto;
	} payload;

	__le16 fc = hdr->frame_control;
	if (ieee80211_has_protected(fc)) {
		if (sw_enc != WIM_ENCDEC_HW)
			return false;
		hdrlen += 8; // CCMP Header
	}

	skb_copy_bits(skb, hdrlen, &payload, sizeof(payload));

	return (payload.proto == htons(ETH_P_PAE));
}

static void setup_ba_session(struct nrc *nw, struct ieee80211_vif *vif, struct sk_buff *skb)
{
	struct ieee80211_sta *peer_sta= NULL;
	struct nrc_sta *i_sta = NULL;
	struct ieee80211_hdr *qmh = (struct ieee80211_hdr *) skb->data;
	int tid = *ieee80211_get_qos_ctl(qmh) & IEEE80211_QOS_CTL_TID_MASK;
	int ret;

	//nrc_mac_dbg("Start BA session (%pM, %d)\n", qmh->addr1, tid);

	if (nw->frag_threshold != -1) { /* Fragmentation enabled by iwconfig */
		//nrc_mac_dbg("Since fragmentation enabled by iwconfig, ignore to setup BA session");
		return;
	}

	/* tid range : 0 ~ 7 */
	if (tid < 0 || tid >= NRC_MAX_TID) {
		nrc_mac_dbg("Invalid TID(%d) with peer %pM", tid, qmh->addr1);
		return;
	}

	peer_sta = ieee80211_find_sta(vif, qmh->addr1);
	if (!peer_sta) {
		nrc_mac_dbg("Fail to set up BA. Fail to find peer_sta (%pM)", qmh->addr1);
		return;
	}

	/* Set up BA session if not created */
	/* Find Peer STA with ADDR1  to set up BA Session */
#ifdef CONFIG_S1G_CHANNEL
	peer_sta->ht_cap.ht_supported=true;
#endif /* #ifdef CONFIG_S1G_CHANNEL */
	i_sta = to_i_sta(peer_sta);
	if (!i_sta) {
		nrc_mac_dbg("Fail to set up BA. Fail to find nrc_sta (%pM)", qmh->addr1);
		return;
	}


	switch (i_sta->tx_ba_session[tid]) 
	{
		case IEEE80211_BA_NONE:
		case IEEE80211_BA_CLOSE:
			nrc_dbg(NRC_DBG_STATE, "%s: Setting up BA session for Tx TID %d with peer (%pM)",
					__func__, tid, peer_sta->addr);
			nw->ampdu_supported = true;
			nw->ampdu_reject = false;
			if((ret = ieee80211_start_tx_ba_session(peer_sta, tid, 0)) != 0) {
				if (ret == -EBUSY) {
					nrc_dbg(NRC_DBG_STATE, "%s: receiver does not want A-MPDU so disable BA session (TID:%d)", __func__, tid);
					i_sta->tx_ba_session[tid] = IEEE80211_BA_DISABLE;
				}
				if (ret == -EAGAIN) {
					nrc_dbg(NRC_DBG_STATE, "%s: session is not idle (TID:%d)", __func__, tid);
					ieee80211_stop_tx_ba_session(peer_sta, tid);
					i_sta->tx_ba_session[tid] = IEEE80211_BA_NONE;
					i_sta->ba_req_last_jiffies[tid] = 0;
				}
			}
			break;
		case IEEE80211_BA_REJECT:
			if(jiffies_to_msecs(jiffies - i_sta->ba_req_last_jiffies[tid]) > 5000) {
				i_sta->tx_ba_session[tid] = IEEE80211_BA_NONE;
				i_sta->ba_req_last_jiffies[tid] = 0;
				nrc_dbg(NRC_DBG_STATE, "%s: reset ba status(TID:%d)", __func__, tid);
			} 
			break;
		default:
			break;
	}
}

static const char *
to_state(enum ieee80211_sta_state state)
{
	switch (state) {
	case IEEE80211_STA_NOTEXIST:
		return "NOTEXIST";
	case IEEE80211_STA_NONE:
		return "NONE";
	case IEEE80211_STA_AUTH:
		return "AUTH";
	case IEEE80211_STA_ASSOC:
		return "ASSOC";
	case IEEE80211_STA_AUTHORIZED:
		return "AUTHORIZED";
	default:
		return "unknown";
	};

}

#if NRC_DBG_PRINT_FRAME_TX
static int tx_h_debug_print(struct nrc_trx_data *tx)
{
	struct ieee80211_sta *sta;
    struct nrc_vif *i_vif;
	struct nrc_sta *i_sta;
	const struct ieee80211_hdr *hdr;
	__le16 fc;

	if (!tx) {
		nrc_mac_dbg("[%s] tx is NULL", __func__);
		return 0;
	}

    i_vif = to_i_vif(tx->vif);
	if (!i_vif) {
		nrc_mac_dbg("[%s] vif is NULL", __func__);
		return 0;
	}

	hdr = (const struct ieee80211_hdr *) tx->skb->data;
	fc = hdr->frame_control;

	nrc_mac_dbg("[%s] %s vif:%d type:%d sype:%d, protected:%d\n",
		__func__, (tx->vif->type == NL80211_IFTYPE_STATION)?"STA":
		(tx->vif->type == NL80211_IFTYPE_AP)?"AP":(tx->vif->type == NL80211_IFTYPE_ADHOC)?
			"ADHOC":"MONITORP",
		i_vif->index, WLAN_FC_GET_TYPE(fc), WLAN_FC_GET_STYPE(fc),
		ieee80211_has_protected(fc));

	if (is_unicast_ether_addr(hdr->addr1)) {
		sta = ieee80211_find_sta(tx->vif, hdr->addr1);
		if (!sta) {
			nrc_mac_dbg("[%s] Unable to find %pM in mac80211",__func__, hdr->addr1);
		return 0;
		}
	}

	i_sta = to_i_sta(sta);
	if (!i_sta) {
		nrc_mac_dbg("[%s] Unable to find %pM in nrc drv",
			__func__, hdr->addr1);
		return 0;
	}

	// set filter here : add filter as you wish
	//eg. ieee80211_is_data(fc) ieee80211_is_mgmt(fc)
	//eg. ieee80211_is_assoc_req
	if (ieee80211_is_deauth(fc)) {
		print_hex_dump(KERN_DEBUG, "tx deauth frame: ", DUMP_PREFIX_NONE, 16, 1,
			tx->skb->data, tx->skb->len, false);
	}
	return 0;
}
TXH(tx_h_debug_print, NL80211_IFTYPE_ALL);
#endif //#if NRC_DBG_PRINT_FRAME_TX

static int tx_h_debug_state(struct nrc_trx_data *tx)
{
	struct ieee80211_sta *sta;
	struct nrc_sta *i_sta;
	const struct ieee80211_hdr *hdr =
		(const struct ieee80211_hdr *) tx->skb->data;

	if (!tx || !tx->sta ||
	      (!ieee80211_is_data(hdr->frame_control) ||
	       is_eapol(tx->skb)))
		return 0;

	sta = ieee80211_find_sta(tx->vif, hdr->addr1);

	if (!sta) {
		nrc_mac_dbg("Unable to find %pM", hdr->addr1);
		return 0;
	}

	i_sta = to_i_sta(sta);

	if (!i_sta)
		return 0;

	if (i_sta->state != IEEE80211_STA_AUTHORIZED)
		nrc_mac_dbg("%s: Wrong state (%s)/%pM",
		     __func__, to_state(i_sta->state), hdr->addr1);

	return 0;
}
TXH(tx_h_debug_state, NL80211_IFTYPE_ALL);


static int tx_h_wfa_halow_filter(struct nrc_trx_data *tx)
{
	struct nrc *nw = tx->nw;

	if (nw->block_frame) {
		nrc_mac_dbg("%s: TX is blocked (Halow)",
				__func__);
		return -EINVAL;
	}

	return 0;
}
TXH(tx_h_wfa_halow_filter, NL80211_IFTYPE_ALL);

static int tx_h_frame_filter(struct nrc_trx_data *tx)
{
	const struct ieee80211_hdr *hdr;
	__le16 fc;

	if (!tx) {
		nrc_mac_dbg("[%s] tx is NULL", __func__);
		return 0;
	}

	if (discard_deauth && tx->vif->type == NL80211_IFTYPE_STATION) {
		hdr = (const struct ieee80211_hdr *) tx->skb->data;
		fc = hdr->frame_control;
		if (ieee80211_is_deauth(fc)) {
			nrc_mac_dbg("[%s] discard TX deauth frame\n", __func__);
			return -1;
		}
	}
	return 0;
}
TXH(tx_h_frame_filter, NL80211_IFTYPE_ALL);

#ifdef CONFIG_SUPPORT_P2P
static int tx_h_managed_p2p_intf_addr(struct nrc_trx_data *tx)
{
	struct ieee80211_mgmt *mgmt = NULL;
	const u8 *p2p_ie = NULL;
	u8 *pos = NULL, *p2p_len = NULL, *cnt = NULL;
	u16 *attr_len = NULL, len = 0;
	const int P2P_ATTR_INTERFACE = 0x10;
	u8 target[ETH_ALEN];
	u8 i;

	if (tx->vif->p2p)
		return 0;

	mgmt = (void *) tx->skb->data;

	if (!(ieee80211_is_assoc_req(mgmt->frame_control) &&
		!ieee80211_is_reassoc_req(mgmt->frame_control)))
		return 0;

	p2p_ie = cfg80211_find_vendor_ie(WLAN_OUI_WFA,
			WLAN_OUI_TYPE_WFA_P2P,
			mgmt->u.assoc_req.variable,
			tx->skb->len - (mgmt->u.assoc_req.variable
			- tx->skb->data));

	if (!p2p_ie)
		return 0;

	ether_addr_copy(target, tx->vif->addr);
	target[0] = 0x6;
	target[1] = 0x1;

	pos = (u8 *)p2p_ie;
	p2p_len = (pos+1);
	pos += 6; /* Skip OUI + OUI Type */

	while (pos - p2p_ie < *p2p_len) {
		attr_len = (u16 *)(pos+1);
		len = le16_to_cpu(*attr_len);

		if (len == 0)
			return 0;

		if (*pos != P2P_ATTR_INTERFACE) {
			pos += (3 /*ID+Len*/ + len);
			continue;
		}
		pos += 3; /* Skip id + len */
		pos += 6; /* Skip Device Address */

		cnt = pos++;
		for (i = 0; i < *cnt; i++, pos += 6) {
			if (ether_addr_equal(target, pos))
				return 0;
		}
		*cnt += 1;
		*p2p_len += 6;
		*attr_len = cpu_to_le16(len + 6);
		skb_put(tx->skb, 6);
		memmove(pos + 6, pos, tx->skb->len - (pos - tx->skb->data));
		ether_addr_copy(pos, target);

		return 0;
	}

	return 0;
}
TXH(tx_h_managed_p2p_intf_addr, NL80211_IFTYPE_ALL);
#endif

/**
 * tx_h_put_iv() - Put a space for IV header.
 *
 * @tx: points to the tx context.
 *
 * If IEEE80211_SKB_CB(@tx->skb)
 */
static int tx_h_put_iv(struct nrc_trx_data *tx)
{
	struct sk_buff *skb = tx->skb;
	struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
	struct ieee80211_key_conf *key = txi->control.hw_key;
	struct ieee80211_hdr *mh = (void *) skb->data;
	__le16 fc = mh->frame_control;
	int hdrlen = ieee80211_hdrlen(fc);

	if (ieee80211_has_protected(fc) && key &&
			!(key->flags & IEEE80211_KEY_FLAG_GENERATE_IV))
		memcpy(skb_push(skb, key->iv_len), mh, hdrlen);

	return 0;
}
TXH(tx_h_put_iv, NL80211_IFTYPE_ALL);

#if defined (CONFIG_CONVERT_NON_QOSDATA)
bool ieee80211_is_data_data(__le16 fc)
{
	return (fc & cpu_to_le16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
	        cpu_to_le16(IEEE80211_FTYPE_DATA | IEEE80211_STYPE_DATA);
}

void insert_qos_ctrl_field_in_skb(struct sk_buff *skb, unsigned int hdr_len) {

	struct ieee80211_hdr *mh = (void *) skb->data;
	bool is_multi = ((mh->addr1[0] & 0x01) != 0);
	u16 fc = 0;
	u16 qos_ctrl = 0;	//Set TID '0' in QoS Field when converting to qos data.

	if (!skb) {
		nrc_mac_dbg("invalid skb [%s, %d] ## \n", __func__, __LINE__);
		BUG();
	}

	memcpy(&fc, &mh->frame_control, sizeof(mh->frame_control));
	fc |= cpu_to_le16(IEEE80211_STYPE_QOS_DATA);
	if (is_multi)
		qos_ctrl |= cpu_to_le16(0x01 << 5);	// no ack

	skb_push(skb, sizeof(qos_ctrl));
	memmove(skb->data, skb->data + sizeof(qos_ctrl), hdr_len);

	memcpy(skb->data + hdr_len, &qos_ctrl, sizeof(qos_ctrl));
	memcpy(skb->data, &fc, sizeof(mh->frame_control));
}

/**
 * tx_h_put_qos_control() - change non-QoS data frame to QoS data frame.
 *
 * @tx: points to the tx context.
 *
 */
static int tx_h_put_qos_control(struct nrc_trx_data *tx)
{
	struct sk_buff *skb = tx->skb;
	struct ieee80211_hdr *mh = (void *) skb->data;
	u16 fc = mh->frame_control;

	if (ieee80211_is_data_data(fc)) {
		if (!(ieee80211_has_protected(fc) && sw_enc != WIM_ENCDEC_HW))
			insert_qos_ctrl_field_in_skb(skb, ieee80211_hdrlen(fc));
	}
	return 0;
}
TXH(tx_h_put_qos_control, NL80211_IFTYPE_ALL);
#endif /* if defined (CONFIG_CONVERT_NON_QOSDATA) */

/* RX */

static void nrc_mac_rx_h_status(struct nrc *nw, struct sk_buff *skb)
{
	struct frame_hdr *fh;
	struct ieee80211_rx_status *status = IEEE80211_SKB_RXCB(skb);
	struct ieee80211_hdr *mh;


	fh = (void *)skb->data; /* frame header */
	mh = (void *)(skb->data+nw->fwinfo.rx_head_size - sizeof(struct hif));

	memset(status, 0, sizeof(*status));

	status->signal = fh->flags.rx.rssi;
#if defined(CONFIG_S1G_CHANNEL)
	status->freq =  (fh->info.rx.frequency) / 10;
	status->freq_offset = (fh->info.rx.frequency % 10 ? 1 : 0);
#else
	status->freq = fh->info.rx.frequency;
#endif /* defined(CONFIG_S1G_CHANNEL) */
	status->band = nw->band; /* I hate this */
	status->rate_idx = 0;
	if (fh->flags.rx.error_mic)
		status->flag |= RX_FLAG_MMIC_ERROR;
	if (fh->flags.rx.iv_stripped)
		status->flag |= RX_FLAG_IV_STRIPPED;

#if ((KERNEL_VERSION(4, 4, 132) <= NRC_TARGET_KERNEL_VERSION) && (KERNEL_VERSION(4, 5, 0) > NRC_TARGET_KERNEL_VERSION)) ||\
	(KERNEL_VERSION(4, 7, 0) <= NRC_TARGET_KERNEL_VERSION)
	if(mh->frame_control & 0x0400)
	{
		status->flag |= RX_FLAG_ALLOW_SAME_PN;
	}
#endif

	if (signal_monitor) {
		//update snr and rssi only if signal monitor is enabled
		nrc_stats_update(mh->addr2, fh->flags.rx.snr, fh->flags.rx.rssi);
		//nrc_stats_print();
	}
	if(ieee80211_is_probe_resp(mh->frame_control))
		if(nrc_stats_channel_noise_update(status->freq, fh->flags.rx.rssi - fh->flags.rx.snr) < 0)
			nrc_mac_dbg("Channel noise update fail : freq(%d)", status->freq);

}

static int nrc_vendor_ann_event(struct nrc *nw,
			const u8 *data, u16 len, enum nrc_vendor_event eid)
{
	struct ieee80211_hw *hw = nw->hw;
	struct sk_buff *skb;

	print_hex_dump(KERN_DEBUG, "event: ", DUMP_PREFIX_NONE,
			16, 1, data, len, false);

	skb = cfg80211_vendor_event_alloc(hw->wiphy, NULL, 255, eid, GFP_KERNEL);

	if (!skb)
		return -ENOMEM;

	if (nla_put(skb, NRC_VENDOR_ATTR_DATA, len, data)) {
		kfree_skb(skb);
		return -EMSGSIZE;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	return 0;
}

static int rx_h_vendor(struct nrc_trx_data *rx)
{
	struct ieee80211_hdr *mh = (void *) rx->skb->data;
	__le16 fc = mh->frame_control;
	u16 ies_offset = 0, len = 0;
	const u8 *pos, *data, ann_subs[] = { NRC_SUBCMD_WOWLAN_PATTERN,
			NRC_SUBCMD_ANNOUNCE1, NRC_SUBCMD_ANNOUNCE2, NRC_SUBCMD_ANNOUNCE3,
			NRC_SUBCMD_ANNOUNCE4, NRC_SUBCMD_ANNOUNCE5, NRC_SUBCMD_REMOTECMD,
			NRC_SUBCMD_ANNOUNCE6, NRC_SUBCMD_ANNOUNCE7, NRC_SUBCMD_ANNOUNCE8,
			NRC_SUBCMD_ANNOUNCE9, NRC_SUBCMD_ANNOUNCE10, NRC_SUBCMD_ANNOUNCE11,
			NRC_SUBCMD_ANNOUNCE12, NRC_SUBCMD_ANNOUNCE13, NRC_SUBCMD_ANNOUNCE14,
			NRC_SUBCMD_ANNOUNCE15, NRC_SUBCMD_ANNOUNCE16, NRC_SUBCMD_ANNOUNCE17,
			NRC_SUBCMD_ANNOUNCE18, NRC_SUBCMD_ANNOUNCE19, NRC_SUBCMD_ANNOUNCE20
			};
	const int OUIT_LEN = 4;
	u8 i;

	if (ieee80211_is_beacon(fc)) {
		if (!disable_cqm && rx->nw->associated_vif) {
			mod_timer(&rx->nw->bcn_mon_timer,
				jiffies + msecs_to_jiffies(rx->nw->beacon_timeout));
		}

		ies_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);

	} else if (ieee80211_is_probe_req(fc)) {
		ies_offset = offsetof(struct ieee80211_mgmt, u.probe_req.variable);
	} else if (ieee80211_is_probe_resp(fc)) {
		ies_offset = offsetof(struct ieee80211_mgmt, u.probe_resp.variable);
	} else if (ieee80211_is_assoc_req(fc)) {
		ies_offset = offsetof(struct ieee80211_mgmt, u.assoc_req.variable);
	}

	for (i = 0; i < ARRAY_SIZE(ann_subs); i++) {
		pos = cfg80211_find_vendor_ie(
			VENDOR_OUI, ann_subs[i],
			rx->skb->data + ies_offset,
			rx->skb->len - ies_offset);

		if (pos) {
			len = *(pos + 1);
			data = pos + 2 /*ID(1)+LEN(1)*/ + OUIT_LEN;
			len -= OUIT_LEN;
			nrc_vendor_ann_event(rx->nw, data, len, ann_subs[i]);
		}
	}
	return 0;
}
RXH(rx_h_vendor, NL80211_IFTYPE_ALL);


static void nrc_rx_handler(void *data, u8 *mac, struct ieee80211_vif *vif)
{
	struct nrc_trx_data *rx	= data;
	struct ieee80211_hdr *mh = (struct ieee80211_hdr *)rx->skb->data;
	struct ieee80211_sta *sta;
	struct nrc_trx_handler *h;
	int res = 0;

	rcu_read_lock();

	if (ieee80211_is_probe_req(mh->frame_control) ||
		ieee80211_is_probe_resp(mh->frame_control) ||
		ieee80211_is_assoc_req(mh->frame_control)) {
		res = rx_h_vendor(rx);
		if (res < 0)
			goto rxh_out;
	}
	sta = ieee80211_find_all_sta(vif, mh->addr2);

#if defined(CONFIG_SUPPORT_BEACON_BYPASS)
	if(!enable_beacon_bypass) {
#endif /* CONFIG_SUPPORT_BEACON_BYPASS */
		if (!sta)
			goto rxh_out;
#if defined(CONFIG_SUPPORT_BEACON_BYPASS)
	}
#endif /* CONFIG_SUPPORT_BEACON_BYPASS */

	/* The received frame is from @sta to @vif */
	rx->vif = vif;
	rx->sta = sta;

	/* Call rx handlers */
	for (h = &__rx_h_start; h < &__rx_h_end; h++) {

		if (!(h->vif_types & BIT(vif->type)))
			continue;

		res = h->handler(rx);
		if (res < 0)
			goto rxh_out;
	}

rxh_out:
	rcu_read_unlock();
	rx->result = res;
}

static int nrc_mac_s1g_monitor_rx(struct nrc *nw, struct sk_buff *skb);

/**
 * nrc_mac_rx() - main rx routine
 *
 * @nw: points to NRC controller data
 * @skb: a received frame with the leading HIF header peeled off.
 */
int nrc_mac_rx(struct nrc *nw, struct sk_buff *skb)
{
	struct nrc_trx_data rx = { .nw = nw, .skb = skb, };
	struct ieee80211_hdr *mh;
	__le16 fc;
	int ret = 0;
	u64 now = 0, diff = 0;

	if (!((nw->drv_state == NRC_DRV_RUNNING) ||
		(nw->drv_state == NRC_DRV_PS)) ||
		atomic_read(&nw->d_deauth.delayed_deauth)) {
		nrc_mac_dbg("Target not ready, discarding frame)");
		dev_kfree_skb(skb);
		return 0;
	}

	nrc_mac_rx_h_status(nw, skb);

	if (nw->promisc) {
		ret = nrc_mac_s1g_monitor_rx(nw, skb);
		return ret;
	}

	/* Peel off frame header */
	skb_pull(skb, nw->fwinfo.rx_head_size - sizeof(struct hif));
	mh = (void*)skb->data;
	fc = mh->frame_control;
	now = ktime_to_us(ktime_get_real());

#ifdef CONFIG_SUPPORT_ITERATE_INTERFACE
	/* Iterate over active interfaces */
	ieee80211_iterate_interfaces(nw->hw, IEEE80211_IFACE_ITER_ACTIVE,
				     nrc_rx_handler, &rx);
#else
	/* Iterate over active interfaces */
	ieee80211_iterate_active_interfaces(nw->hw,
				     nrc_rx_handler, &rx);
#endif

	/**
	 * When associated, the scan results only include probe responses.
	 */
	if (nw->scan_mode == NRC_SCAN_MODE_ACTIVE_SCANNING) {
		if (nw->associated_vif != NULL &&
			nw->associated_vif->type == NL80211_IFTYPE_STATION &&
			ieee80211_is_beacon(fc)) {
			dev_kfree_skb(skb);
			return 0;
		}
	}

	diff = ktime_to_us(ktime_get_real()) - now;
	//if ((!diff) || (diff > NRC_MAC80211_RCU_LOCK_THRESHOLD))
	if (diff > NRC_MAC80211_RCU_LOCK_THRESHOLD)
		nrc_mac_dbg("%s, diff=%lu", __func__, (unsigned long)diff);

	if (!rx.result) {
		if (!disable_cqm) {
			if (nw->associated_vif &&
				ieee80211_is_probe_resp(fc) &&
				nw->scan_mode == NRC_SCAN_MODE_IDLE) {
				mod_timer(&nw->bcn_mon_timer,
					jiffies + msecs_to_jiffies(nw->beacon_timeout));
			}
		}

		if ((ieee80211_is_assoc_req(mh->frame_control) ||
			ieee80211_is_reassoc_req(mh->frame_control)) &&
			rx.sta) {
			struct nrc_sta *i_sta = to_i_sta(rx.sta);
			struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;
			i_sta->listen_interval = mgmt->u.assoc_req.listen_interval;
		}

		/* Modified assoc issue after STA OFF/ON (connection without deauth) */
		if (ieee80211_is_auth(mh->frame_control) && rx.sta && rx.vif && rx.vif->type == NL80211_IFTYPE_AP) {
			struct nrc_sta *i_sta = to_i_sta(rx.sta);
			if(i_sta && i_sta->state > IEEE80211_STA_NOTEXIST){
				struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;
				struct sk_buff *skb_deauth;
				/* Pretend to receive a deauth from @sta */
				skb_deauth = ieee80211_deauth_get(nw->hw, mgmt->bssid, mgmt->sa, mgmt->bssid, WLAN_REASON_DEAUTH_LEAVING, NULL, false);
				if (!skb_deauth) {
					nrc_dbg(NRC_DBG_STATE, "%s Fail to alloc skb", __func__);
					return 0;
				}
				nrc_mac_dbg("%s: %pM connected, but auth received. send to kernel after convert auth -> deauth.", __func__, mgmt->sa);
				ieee80211_rx_irqsafe(nw->hw, skb_deauth);
				return 0;
			}
		}

#if NRC_DBG_PRINT_ARP_FRAME
		if (ieee80211_is_data_qos(fc)) {
			/* find to LLC TYPE field to check ARP */
			uint8_t *ptr;
			uint16_t llc_type;
			uint8_t ccmp_hdr = 0;
			if (ieee80211_has_protected(fc)) {
				ccmp_hdr = 8;
			}
			ptr = (uint8_t *)mh + (ieee80211_hdrlen(fc) + ccmp_hdr + 6); //6(LLC)
			llc_type = *((uint16_t *)ptr);
			if (llc_type == htons(ETH_P_ARP)) {
				nrc_ps_dbg("[%s] RX ARP [type:%d sype:%d, protected:%d, len:%d]",
					__func__, WLAN_FC_GET_TYPE(fc), WLAN_FC_GET_STYPE(fc),
					ieee80211_has_protected(fc), rx.skb->len);
			}
		}
#endif
		ieee80211_rx_irqsafe(nw->hw, rx.skb);
		if (ieee80211_hw_check(nw->hw, SUPPORTS_DYNAMIC_PS)) {
			if (ieee80211_is_data(fc) && nw->ps_enabled &&
				nw->hw->conf.dynamic_ps_timeout > 0) {
				mod_timer(&nw->dynamic_ps_timer,
					jiffies + msecs_to_jiffies(nw->hw->conf.dynamic_ps_timeout));
			}
		} else if (ieee80211_hw_check(nw->hw, SUPPORTS_PS)) {
		} else {
			if (nw->invoke_beacon_loss) {
				nw->invoke_beacon_loss = false;
				nrc_send_beacon_loss(nw);
			}
		}
	}

	return 0;
}

/**
 * rx_h_decrypt() - increase the length of a frame by icv length
 *
 * TODO:
 * - Verify if using key information stored in i_sta is valid.
 * - Consider using ieee80211_iter_key to find the key.
 * - What if multiple keys for a station?
 */
static int rx_h_decrypt(struct nrc_trx_data *rx)
{
	struct ieee80211_hdr *mh = (void *) rx->skb->data;
	struct nrc *nw = rx->nw;
	struct ieee80211_rx_status *status;
	struct ieee80211_key_conf *key;
	struct nrc_sta *i_sta;
	struct nrc_vif *i_vif;
	int vif_id;
	__le16 fc;

	if( !rx->sta ){
		return 0;
	}

	i_sta = to_i_sta(rx->sta);
	i_vif = to_i_vif(rx->vif);
	vif_id = i_vif->index;
	fc = mh->frame_control;

	/* not proteced frame, just skip */
	if (!ieee80211_has_protected(fc))
		return 0;

#if 0
	nrc_dbg(NRC_DBG_MAC, "%s_1: vif_id:%d cap_mask(0X%x)",
		__func__, vif_id, nw->cap.vif_caps[vif_id].cap_mask);
#endif

	/* if SW SECURITY, just skip */
	if (!(nw->cap.vif_caps[vif_id].cap_mask & WIM_SYSTEM_CAP_HWSEC)
		|| sw_enc == WIM_ENCDEC_SW) {
		return 0;
	}

	/*
	 * @skb->len from the target does not account for ICV/MIC.
	 * Since mac80211 trims it if RX_FLAG_IV_STRIPPED, we need to
	 * increment @skb->len by that amount.
	 */
	key = is_multicast_ether_addr(mh->addr1) ? i_sta->gtk : i_sta->ptk;

	if (!key) {
		/* driver does NOT have GTK on HYBRID SEC mode */
		if (nw->cap.vif_caps[vif_id].cap_mask & WIM_SYSTEM_CAP_HYBRIDSEC) {
			if (ieee80211_is_data(mh->frame_control) && is_multicast_ether_addr(mh->addr1)) {
#if 0
				nrc_dbg(NRC_DBG_MAC, "%s_2: Request SW DEC on mac80211",__func__);
				print_hex_dump(KERN_DEBUG, "rx bcmc: ", DUMP_PREFIX_NONE, 16, 1,
					rx->skb->data, rx->skb->len, false);
#endif
				return 0;
			}
		}
		nrc_dbg(NRC_DBG_MAC, "key is NULL");
		return 0;
	}

	switch (key->cipher) {
	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
	case WLAN_CIPHER_SUITE_TKIP:
	case WLAN_CIPHER_SUITE_CCMP:
#ifdef CONFIG_SUPPORT_CCMP_256
	case WLAN_CIPHER_SUITE_CCMP_256:
#endif
#ifdef CONFIG_SUPPORT_GCMP
	case WLAN_CIPHER_SUITE_GCMP:
	case WLAN_CIPHER_SUITE_GCMP_256:
#endif
#ifdef CONFIG_SUPPORT_GMAC
	case WLAN_CIPHER_SUITE_BIP_GMAC_128:
	case WLAN_CIPHER_SUITE_BIP_GMAC_256:
#endif

		rx->skb->len += key->icv_len;
		break;
	default:
		nrc_dbg(NRC_DBG_MAC, "%s: unknown cipher (%d)",
			__func__, key->cipher);
		return 0;
	}

	/*
	 * Target stripped ICV/MIC, but not IV.
	 * We do not set RX_FLAG_IV_STRPPED.
	 */
	status = IEEE80211_SKB_RXCB(rx->skb);
	status->flag |= RX_FLAG_DECRYPTED;
	status->flag |= RX_FLAG_MMIC_STRIPPED;

#if 0
	nrc_dbg(NRC_DBG_MAC, "%s_2: HW DEC completed! vif_id:%d cap_mask(0X%x)",
		__func__, vif_id, nw->cap.vif_caps[vif_id].cap_mask);
#endif

	return 0;
}
RXH(rx_h_decrypt, NL80211_IFTYPE_ALL);

#if defined (CONFIG_SUPPORT_IBSS)
extern u64 current_bssid_beacon_timestamp;
static int rx_h_ibss_get_bssid_tsf(struct nrc_trx_data *rx)
{
	struct ieee80211_mgmt *mgmt = (void *) rx->skb->data;
    __le16 fc = mgmt->frame_control;

	if (rx->vif->type != NL80211_IFTYPE_ADHOC)
		return 0;

	if (rx->vif->type == NL80211_IFTYPE_ADHOC &&
		 ieee80211_is_beacon(fc) &&
		 !memcmp(mgmt->bssid, rx->vif->bss_conf.bssid, ETH_ALEN)) {
		current_bssid_beacon_timestamp = le64_to_cpu(mgmt->u.beacon.timestamp);
#if 0
		nrc_mac_dbg("%s, RX beacon SA=%pM BSSID=%pM  TSF=0x%llx\n",__func__,
			mgmt->sa, mgmt->bssid, current_bssid_beacon_timestamp);
#endif
		return 0;
	}

	return 0;
}
RXH(rx_h_ibss_get_bssid_tsf, NL80211_IFTYPE_ALL);
#endif /* CONFIG_SUPPORT_IBSS */


#if NRC_DBG_PRINT_FRAME_RX
static int rx_h_debug_print(struct nrc_trx_data *rx)
{
	struct ieee80211_sta *sta;
	struct ieee80211_hdr *hdr;
	struct nrc_sta *i_sta;
	struct nrc_vif *i_vif;
	struct ieee80211_rx_status *status;
	__le16 fc;

	if (!rx) {
		nrc_mac_dbg("[%s] rx is NULL", __func__);
		return 0;
	}

	if(!rx->sta){
		nrc_mac_dbg("[%s] rx->sta is NULL", __func__);
		return 0;
	}

	i_sta = to_i_sta(rx->sta);
	i_vif = to_i_vif(rx->vif);
	hdr = (struct ieee80211_hdr *) rx->skb->data;
	fc = hdr->frame_control;

	nrc_mac_dbg("[%s] %s vif:%d type:%d sype:%d, protected:%d\n",
		__func__, (rx->vif->type == NL80211_IFTYPE_STATION)?"STA":
        (rx->vif->type == NL80211_IFTYPE_AP)?"AP":"MONITORP",
        i_vif->index, WLAN_FC_GET_TYPE(fc), WLAN_FC_GET_STYPE(fc),
        ieee80211_has_protected(fc));


    if (is_unicast_ether_addr(hdr->addr2)) {
        sta = ieee80211_find_sta(rx->vif, hdr->addr2);
        if (!sta) {
            nrc_mac_dbg("[%s] Unable to find %pM in mac80211",
                    __func__, hdr->addr2);
            return 0;
        }
    }

	i_sta = to_i_sta(sta);
	if (!i_sta) {
		nrc_mac_dbg("[%s] Unable to find %pM in nrc drv",
			__func__, hdr->addr2);
		return 0;
	}

	// set filter here : add filter as you wish
	//eg. ieee80211_is_data(fc) ieee80211_is_mgmt(fc)
	//eg. ieee80211_is_assoc_req
	if (ieee80211_is_deauth(fc)) {
		status = IEEE80211_SKB_RXCB(rx->skb);
		nrc_mac_dbg("%s: status->flag:%d\n", __func__, status->flag);
		print_hex_dump(KERN_DEBUG, "rx deauth frame: ", DUMP_PREFIX_NONE, 16, 1,
			rx->skb->data, rx->skb->len, false);
	}

	return 0;
}
RXH(rx_h_debug_print, NL80211_IFTYPE_ALL);
#endif //#if NRC_DBG_PRINT_FRAME_RX

#ifdef CONFIG_SUPPORT_MESH_ROUTING
static int rx_h_mesh(struct nrc_trx_data *rx)
{
	struct ieee80211_mgmt *mgmt;
	struct nrc_sta *i_sta;
	__le16 fc;
	int rssi = nrc_stats_get_mesh_rssi_threshold();
	bool is_mesh_action = false;

	if(!rx->sta) {
		return 0;
	}

	i_sta = to_i_sta(rx->sta);
	mgmt = (void *) rx->skb->data;
	fc = mgmt->frame_control;

	if (i_sta && i_sta->state == IEEE80211_STA_AUTHORIZED &&
		ieee80211_is_action(fc)) {
		if (ieee80211_has_protected(fc) && sw_enc != WIM_ENCDEC_HW) {
			is_mesh_action = true;
		} else {
			if (mgmt->u.action.category == WLAN_CATEGORY_MESH_ACTION) {
				if (mgmt->u.action.u.mesh_action.action_code ==
					WLAN_MESH_ACTION_HWMP_PATH_SELECTION) {
					is_mesh_action = true;
				}
			}
		}
	}

	if (is_mesh_action) {
		if (nrc_stats_metric(rx->sta->addr) < nrc_stats_calc_metric(rssi)) {
			nrc_mac_dbg("%s: LOW RSSI!(< %d) drop mesh action from %pM",
				__func__, rssi, rx->sta->addr);
			return -1;
		}
	}

	return 0;
}
RXH(rx_h_mesh, BIT(NL80211_IFTYPE_MESH_POINT));
#endif

/**
 * nrc_mac_trx_init() - Initialize TRX data path
 *
 *
 * TODO:
 * Handlers are registered during compile time, not
 * via nrc_mac_{tx,rx}_h_register().
 */
void nrc_mac_trx_init(struct nrc *nw)
{
	int i;

	spin_lock_init(&nw->txq_lock);
	INIT_LIST_HEAD(&nw->txq);
	for (i = 0; i < ARRAY_SIZE(nw->tx_credit); i++) {
		atomic_set(&nw->tx_credit[i], 0);
		atomic_set(&nw->tx_pend[i], 0);
	}

}


#ifdef CONFIG_NRC_HIF_DUMP_S1G_RXINFO

/* S1G monitor mode */
#define RXIF \
"-- RX Info (v:%d ok:%d crc:%d agg:%d ndp:%d mpdu_len:%d " \
"timestamp:%d fcs:%#x bw:%d ptype:%d format:%d rcpi:%d)"

#define SIG1MF \
"-- S1G1M (len:%d agg:%d ndp:%d rsp:%d mcs:%d " \
"coding:%d stbc:%d short_gi:%d)"

#define SIGSHORTF \
"-- S1GShort (len:%d id:%d agg:%d ndp:%d rsp:%d " \
"mcs:%d coding:%d stbc:%d short_gi:%d bw:%d ul_ind:%d)"

static void nrc_dump_s1g_sig_rxinfo(struct sk_buff *skb)
{
	struct sigS1g *sig_s1g;
	struct rxInfo *rxi;
	bool s1g_1m_ppdu;

	sig_s1g  = (void *)(skb->data + sizeof(struct frame_hdr));
	rxi = (void *)(sig_s1g + 1);

	s1g_1m_ppdu = rxi->bandwidth == WIM_BW_1M ||
		(rxi->bandwidth == WIM_BW_2M && rxi->format == WIM_S1G_DUP_1M);

	nrc_mac_dbg(RXIF, rxi->valid, rxi->okay, rxi->error_crc,
		    rxi->aggregation, rxi->ndp_ind,
		    rxi->mpdu_length, rxi->timestamp,
		    rxi->fcs, rxi->bandwidth, rxi->preamble_type,
		    rxi->format, rxi->rcpi);

	if (rxi->bandwidth == WIM_BW_1M ||
	    (rxi->bandwidth == WIM_BW_2M && rxi->format == WIM_S1G_DUP_1M)) {
		/* 1M or 1M dup (or workaround : 2M & 1M_DUP) */
		nrc_mac_dbg(SIG1MF, sig_s1g->flags.s1g1M.length,
			    sig_s1g->flags.s1g1M.aggregation,
			    sig_s1g->flags.s1g1M.ndp_ind,
			    sig_s1g->flags.s1g1M.response_ind,
			    sig_s1g->flags.s1g1M.mcs,
			    sig_s1g->flags.s1g1M.coding,
			    sig_s1g->flags.s1g1M.stbc,
			    sig_s1g->flags.s1g1M.short_gi);
	} else {
		/* 2/4/8/16M */
		nrc_mac_dbg(SIGSHORTF, sig_s1g->flags.s1gShort.length,
			    sig_s1g->flags.s1gShort.id,
			    sig_s1g->flags.s1gShort.aggregation,
			    sig_s1g->flags.s1gShort.ndp_ind,
			    sig_s1g->flags.s1gShort.response_ind,
			    sig_s1g->flags.s1gShort.mcs,
			    sig_s1g->flags.s1gShort.coding,
			    sig_s1g->flags.s1gShort.stbc,
			    sig_s1g->flags.s1gShort.short_gi,
			    sig_s1g->flags.s1gShort.bandwidth,
			    sig_s1g->flags.s1gShort.uplink_ind);
	}
}
#endif

#define NRC_CHAN_700MHZ		0x0001
#define NRC_CHAN_800MHZ		0x0002
#define NRC_CHAN_900MHZ		0x0004

static void
nrc_add_rx_s1g_radiotap_header(struct nrc *nw, struct sk_buff *skb)
{
	struct nrc_radiotap_hdr rt_hdr, *prt_hdr;
	struct sigS1g *sig_s1g;
	struct rxInfo *rxi;
	u32 mcs = 0, response_ind = 0;
	bool short_gi = false, s1g_1m_ppdu;
	bool ndp_ind, agg = false;
	uint32_t fcs;
	uint8_t *p;
	struct nrc_radiotap_hdr_agg rt_hdr_agg, *prt_hdr_agg;
	struct nrc_radiotap_hdr_ndp rt_hdr_ndp, *prt_hdr_ndp;
	uint8_t color = 0;
	uint8_t uplink_ind = 0;

#ifdef CONFIG_NRC_HIF_DUMP_S1G_RXINFO
	nrc_dump_s1g_sig_rxinfo(skb);
#endif

	sig_s1g  = (void *) (skb->data + sizeof(struct frame_hdr));
	rxi = (void *) (sig_s1g + 1);
	ndp_ind = (bool) rxi->ndp_ind;
	fcs = rxi->fcs;

	s1g_1m_ppdu = rxi->bandwidth == WIM_BW_1M ||
		(rxi->bandwidth == WIM_BW_2M &&
		rxi->format == WIM_S1G_DUP_1M);

	if (!ndp_ind) {
		if (s1g_1m_ppdu) {
			/* 1M or 1M dup (or workaround : 2M & 1M_DUP) */
			mcs = sig_s1g->flags.s1g1M.mcs;
			short_gi = sig_s1g->flags.s1g1M.short_gi & 0x1;
			response_ind =
				sig_s1g->flags.s1g1M.response_ind & 0x3;
			agg = sig_s1g->flags.s1g1M.aggregation;
		} else {
			/* 2/4/8/16M */
			mcs = sig_s1g->flags.s1gShort.mcs;
			short_gi = sig_s1g->flags.s1gShort.short_gi & 0x1;
			response_ind =
				sig_s1g->flags.s1gShort.response_ind & 0x3;
			agg = sig_s1g->flags.s1gShort.aggregation;
			color = sig_s1g->flags.s1gShort.id & 0x7;
			uplink_ind = sig_s1g->flags.s1gShort.uplink_ind;
		}
	} else {
		/* NDP frame */
		skb_put(skb, 6);
		p = (uint8_t *)(rxi + 1);
		*p = ((*(char *)sig_s1g & 0x07) == 0x07) ? 0xF0 : 0x04;
		/*
		 * README: Remove reserved field of 2MHz NDP frame
		 *			  Aaron - 2019-0412 - Gerrit#1973
		 */
		if (!s1g_1m_ppdu) {
			memcpy((char *)(p + 1), (char *)&sig_s1g->flags, 3);
			memcpy((char *)(p + 4), (char *)&sig_s1g->flags + 4, 2);
		} else
			memcpy((void *)(p + 1), (void *)&sig_s1g->flags, 5);

		*(p + 5) = (*(p + 5) & 0x3F) | ((s1g_1m_ppdu) ? 0 : (0x2 << 6));
	}

	/**
	 * README: Change RSSI value refernce - Aaron - 2019-0531
	 *   original: RCPI based calculation - RSSI = (RCPI/2)-122
	 *   new: replace RCPI with RSSI value of Rx Vector
	 **/
	/* prt_hdr->rt_rssi = rxi->rcpi; */

	if (ndp_ind) {
		rt_hdr_ndp.hdr.it_version = PKTHDR_RADIOTAP_VERSION;
		rt_hdr_ndp.hdr.it_pad = 0;

		rt_hdr_ndp.rt_tsft = cpu_to_le64(rxi->timestamp);
		 /* frame include FCS */
		rt_hdr_ndp.rt_flags = ((ndp_ind) ? 0x00:0x10);
		rt_hdr_ndp.rt_ch_frequency = cpu_to_le16(rxi->center_freq);
		rt_hdr_ndp.rt_ch_flags =
			cpu_to_le16(IEEE80211_CHAN_OFDM|NRC_CHAN_900MHZ);

		rt_hdr_ndp.hdr.it_len = cpu_to_le64(sizeof(rt_hdr_ndp));
		rt_hdr_ndp.hdr.it_present =
			cpu_to_le32(BIT(IEEE80211_RADIOTAP_TSFT) |
				BIT(IEEE80211_RADIOTAP_FLAGS) |
				BIT(IEEE80211_RADIOTAP_CHANNEL) |
				/* BIT(IEEE80211_RADIOTAP_0_LENGTH_PSDU)); */
				BIT(26));
		/* S1G NDP CMAC frame */
		rt_hdr_ndp.rt_zero_length_psdu = 0x02;
		rt_hdr_ndp.rt_pad = 0;
	} else if (agg) {
		rt_hdr_agg.hdr.it_version = PKTHDR_RADIOTAP_VERSION;
		rt_hdr_agg.hdr.it_pad = 0;

		rt_hdr_agg.rt_tsft = cpu_to_le64(rxi->timestamp);
		 /* frame include FCS */
		rt_hdr_agg.rt_flags = ((ndp_ind) ? 0x00:0x10);
		rt_hdr_agg.rt_ch_frequency = cpu_to_le16(rxi->center_freq);
		rt_hdr_agg.rt_ch_flags =
			cpu_to_le16(IEEE80211_CHAN_OFDM|NRC_CHAN_900MHZ);

		rt_hdr_agg.hdr.it_len = cpu_to_le64(sizeof(rt_hdr_agg));
		rt_hdr_agg.hdr.it_present =
			cpu_to_le32(BIT(IEEE80211_RADIOTAP_TSFT) |
				BIT(IEEE80211_RADIOTAP_FLAGS) |
				BIT(IEEE80211_RADIOTAP_CHANNEL) |
				BIT(IEEE80211_RADIOTAP_AMPDU_STATUS) |
				BIT(28)); //BIT(IEEE80211_RADIOTAP_TLV));
		rt_hdr_agg.rt_ampdu_ref = 1;
		rt_hdr_agg.rt_ampdu_flags = 2;
		rt_hdr_agg.rt_ampdu_crc = 3;
		rt_hdr_agg.rt_ampdu_reserved = 0;
		rt_hdr_agg.rt_pad = 0;
		rt_hdr_agg.rt_pad2 = 0;
		rt_hdr_agg.rt_tlv_type   = cpu_to_le16(32); /* S1G */
		rt_hdr_agg.rt_tlv_length = cpu_to_le16(6);
		rt_hdr_agg.rt_s1g_known  = 0x007F;
		rt_hdr_agg.rt_s1g_data1  =
			((rxi->bandwidth == 0) ? 0x0 :
			((rxi->preamble_type) ? 0x2 : 0x1)) |
			((response_ind & 0x03) << 2) | /* RI */
			((short_gi & 0x01) << 5) |	   /* GI */
			0x0 << 6 |		/* NSS */
			((rxi->bandwidth & 0x0f) << 8) | /* BW */
			((mcs & 0x0f) << 12);		/* MCS */
		rt_hdr_agg.rt_s1g_data2 = cpu_to_le16((color & 0x07) |
			   ((uplink_ind & 0x01) << 3) |
			   (rxi->rcpi << 8));
	} else {
		rt_hdr.hdr.it_version = PKTHDR_RADIOTAP_VERSION;
		rt_hdr.hdr.it_pad = 0;

		rt_hdr.rt_tsft = cpu_to_le64(rxi->timestamp);
		 /* frame include FCS */
		rt_hdr.rt_flags = ((ndp_ind) ? 0x00 : 0x10);
		rt_hdr.rt_ch_frequency = cpu_to_le16(rxi->center_freq);
		rt_hdr.rt_ch_flags =
			cpu_to_le16(IEEE80211_CHAN_OFDM|NRC_CHAN_900MHZ);

		rt_hdr.hdr.it_len = cpu_to_le64(sizeof(rt_hdr));
		rt_hdr.hdr.it_present =
			cpu_to_le32(BIT(IEEE80211_RADIOTAP_TSFT) |
				BIT(IEEE80211_RADIOTAP_FLAGS) |
				BIT(IEEE80211_RADIOTAP_CHANNEL) |
				BIT(28)); //BIT(IEEE80211_RADIOTAP_TLV));
		rt_hdr.rt_pad = 0;
		rt_hdr.rt_pad2 = 0;
		rt_hdr.rt_tlv_type		= cpu_to_le16(32); // S1G
		rt_hdr.rt_tlv_length	= cpu_to_le16(6);
		rt_hdr.rt_s1g_known		= 0x007F;
		rt_hdr.rt_s1g_data1		=
			((rxi->bandwidth == 0) ? 0x0 :
				((rxi->preamble_type) ? 0x2 : 0x1)) |
			((response_ind & 0x03) << 2) |	 /* RI */
			((short_gi & 0x01) << 5) |		 /* GI */
			0x0 << 6 |	/* NSS */
			((rxi->bandwidth & 0x0f) << 8) | /* BW */
			((mcs & 0x0f) << 12);			 /* MCS */
		rt_hdr.rt_s1g_data2 =
				 cpu_to_le16((color & 0x07) |
				 ((uplink_ind & 0x01) << 3) |
				 (rxi->rcpi << 8));
	}

	skb_pull(skb, nw->fwinfo.rx_head_size - sizeof(struct hif));

	if (ndp_ind) {
		prt_hdr_ndp =
			(struct nrc_radiotap_hdr_ndp *)
			skb_push(skb, sizeof(rt_hdr_ndp));
		memcpy(prt_hdr_ndp, &rt_hdr_ndp, sizeof(rt_hdr_ndp));
	} else if (agg) {
		prt_hdr_agg =
			(struct nrc_radiotap_hdr_agg *)
			skb_push(skb, sizeof(rt_hdr_agg));
		memcpy(prt_hdr_agg, &rt_hdr_agg, sizeof(rt_hdr_agg));

		// add FCS
	    memcpy((void *)skb_put(skb, 4), (void *)&fcs, 4);
	} else {
		prt_hdr =
			(struct nrc_radiotap_hdr *)
			skb_push(skb, sizeof(rt_hdr));
		memcpy(prt_hdr, &rt_hdr, sizeof(rt_hdr));

		// add FCS
	    memcpy((void *)skb_put(skb, 4), (void *)&fcs, 4);
	}

	skb_set_mac_header(skb, 0);
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = htons(ETH_P_802_2);
	memset(skb->cb, 0, sizeof(skb->cb));
}

static void s1g_monitor_rx(void *data, struct net_device *dev)
{
	struct sk_buff *skb, *origskb = data;
	struct wireless_dev *wdev = dev->ieee80211_ptr;

	if (wdev->iftype != NL80211_IFTYPE_MONITOR)
		return;

	skb = skb_clone(origskb, GFP_ATOMIC);
	if (!skb)
		return;

	skb->dev = dev;
	netif_rx(skb);
}


static int nrc_mac_s1g_monitor_rx(struct nrc *nw, struct sk_buff *skb)
{
	struct ieee80211_hw *hw = nw->hw;

#if defined(NRC7291)
	struct RxInfo *rx;

	rx = RXINFO(skb->data() + RXVECTOR_SIG_SIZE);

	if (rx->error_crc && !rx->ndp_ind) {
		nrc_mac_dbg("discard this frame because of error_crc");
		dev_kfree_skb(skb);
		return 0;
	}
#endif

	if (nrc_mac_is_s1g(nw)){
		nrc_add_rx_s1g_radiotap_header(nw, skb);
		ieee80211_iterate_active_netdev(hw, s1g_monitor_rx, skb);
	}
	dev_kfree_skb(skb);

	return 0;
}
