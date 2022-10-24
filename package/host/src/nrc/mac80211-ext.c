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
#include <linux/list.h>
#include <linux/spinlock.h>
#include <net/mac80211.h>
#include "nrc-mac80211.h"
#include "nrc-debug.h"
#include "mac80211-ext.h"
#include "compat.h"


/**
 * ieee80211_find_all_sta() - finds a sta which has not been uploaded.
 *
 * @vif: pointers to vif currently being searched against.
 * @addr: sta address
 *
 * This is an extension to ieee80211_find_sta() which only returns
 * 'uploaded' sta. This function searches for all sta that the driver
 * has seen.
 *
 * NOTE:
 * - This function should be called under RCU lock, and the resulting
 *   pointer is only valid under RCU lock as well.
 */
struct ieee80211_sta *ieee80211_find_all_sta(struct ieee80211_vif *vif,
					     u8 *addr)
{
	struct ieee80211_sta *sta;
	struct nrc_vif *i_vif = to_i_vif(vif);
	struct nrc_sta *i_sta = NULL, *tmp = NULL;
	unsigned long flags;

	/* First find the 'uploaded' sta */
	sta = ieee80211_find_sta(vif, addr);
	if (sta)
		return sta;

	spin_lock_irqsave(&i_vif->preassoc_sta_lock, flags);

	list_for_each_entry_safe(i_sta, tmp, &i_vif->preassoc_sta_list, list) {
		sta = to_ieee80211_sta(i_sta);

		if (ether_addr_equal(sta->addr, addr))
			goto out;

	}

	sta = NULL;
 out:
	spin_unlock_irqrestore(&i_vif->preassoc_sta_lock, flags);

	return sta;
}

u8 *ieee80211_append_ie(struct sk_buff *skb, u8 eid, u8 len)
{
	u8 *pos;

	if (WARN_ON(skb_tailroom(skb) < len + 2))
		return NULL;

	pos = skb_put(skb, len + 2);
	*pos++ = eid;
	*pos++ = len;

	return pos;
}

struct sk_buff *ieee80211_deauth_get(struct ieee80211_hw *hw,
				     u8 *da, u8 *sa, u8 *bssid,
				     __le16 reason, struct ieee80211_sta *sta, bool tosta)
{
	struct nrc *nw = hw->priv;
	struct sk_buff *skb;
	struct ieee80211_mgmt *deauth;
	struct ieee80211_rx_status *status;
	struct nrc_sta *i_sta;
	struct ieee80211_tx_info *txi;

	__le16 fc;
	u32 ccmp_mic_len = 0;
	u16 *reason_code;
	u8 ccmp_hdr[8] = {0xfe,0x00,0x00,0x20,0x00,0x00,0x00,0x00};

	if (nw == NULL || sta == NULL)
		return NULL;

	nrc_mac_dbg("%s: cipher_pairwise:%d\n", __func__, nw->cipher_pairwise);
	if (nw->cipher_pairwise == WLAN_CIPHER_SUITE_AES_CMAC) {
		/* PMF Handling */
		if(tosta) ccmp_mic_len = 8; /* only need CCMP hdr. MIC will appended on Target */
		else ccmp_mic_len = 16; /* need CCMP + MIC hdr */
	}

	nrc_mac_dbg("%s: ccmp_mic_len:%d\n", __func__, ccmp_mic_len);

	/* Normal Deauth (TRX) : MAC HDR (24B) + REASON(2B)
	  PMF Deauth (RX): MAC HDR (24B) + CCMP(8B) + REASON(2B) + MIC (8B)
	  PMF Deauth (TX): MAC HDR (24B) + CCMP(8B) + REASON(2B) */
	skb = dev_alloc_skb(hw->extra_tx_headroom + 26 + ccmp_mic_len);
	if (!skb)
		return NULL;

	skb_reserve(skb, hw->extra_tx_headroom);

	deauth = (struct ieee80211_mgmt *) skb_put(skb, 26 + ccmp_mic_len);
	memset(deauth, 0, 26 + ccmp_mic_len);

	deauth->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
						IEEE80211_STYPE_DEAUTH);

	if (ccmp_mic_len) {
		/* add protected bit in FC for PMF */
		deauth->frame_control |= cpu_to_le16(IEEE80211_FCTL_PROTECTED);
	}

	ether_addr_copy(deauth->da, da);
	ether_addr_copy(deauth->sa, sa);
	ether_addr_copy(deauth->bssid, bssid);

	fc = deauth->frame_control;
	if (ieee80211_has_protected(fc)) {
		memcpy ((u8 *)(((u8 *) deauth) + 24), ccmp_hdr, 8);
		reason_code = (u16 *)(((u8 *) deauth) + 32);
		*reason_code = reason;

		if (tosta) {
			i_sta = to_i_sta(sta);
			txi = IEEE80211_SKB_CB(skb);
			if (txi) {
				txi->control.hw_key = i_sta->ptk;
				//nrc_mac_dbg("%s: set txi\n", __func__);
			}
		} else {
			status = IEEE80211_SKB_RXCB(skb);
			status->flag |= RX_FLAG_DECRYPTED;
			status->flag |= RX_FLAG_MMIC_STRIPPED;
			status->flag |= RX_FLAG_PN_VALIDATED;
		}
	} else {
		deauth->u.deauth.reason_code = reason;
	}

#if 0
	status = IEEE80211_SKB_RXCB(skb);
	nrc_mac_dbg("%s: status->flag:%d\n", __func__, status->flag);
	print_hex_dump(KERN_DEBUG, "deauth frame: ", DUMP_PREFIX_NONE, 16, 1,
			skb->data, skb->len, false);
#endif

	return skb;
}

/**
 * ieee80211_iterate_netdev - iterate active net_device
 */
void ieee80211_iterate_active_netdev(struct ieee80211_hw *hw,
				     void (*iterator)(void *data,
						      struct net_device *dev),
				     void *data)
{
	struct net_device *dev = NULL;

	for_each_netdev_rcu(wiphy_net(hw->wiphy), dev) {

		if (dev->dev.parent != wiphy_dev(hw->wiphy) ||
		    !netif_running(dev))
			continue;

		iterator(data, dev);
	}
}
