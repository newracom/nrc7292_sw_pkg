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
				     __le16 reason)
{
	struct sk_buff *skb;
	struct ieee80211_mgmt *deauth;

	skb = dev_alloc_skb(hw->extra_tx_headroom + 24 + 2);
	if (!skb)
		return NULL;

	skb_reserve(skb, hw->extra_tx_headroom);

	deauth = (struct ieee80211_mgmt *) skb_put(skb, 24 + 2);
	memset(deauth, 0, 24 + 2);
	deauth->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
					    IEEE80211_STYPE_DEAUTH);
	ether_addr_copy(deauth->da, da);
	ether_addr_copy(deauth->sa, sa);
	ether_addr_copy(deauth->bssid, bssid);
	deauth->u.deauth.reason_code  = reason;

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
