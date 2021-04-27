/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * mac80211 extension
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
#ifndef __MAC80211_EXT_H__
#define __MAC80211_EXT_H__

#include <net/mac80211.h>

struct bss_max_idle_period_ie {
	__le16 max_idle_period;
	u8 idle_option;
} __packed;

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#else
static inline void eth_zero_addr(u8 *addr)
{
	memset(addr, 0x00, ETH_ALEN);
}

static inline bool ether_addr_equal(const u8 *addr1, const u8 *addr2)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)
	u32 fold = ((*(const u32 *)addr1) ^ (*(const u32 *)addr2)) |
		   ((*(const u16 *)(addr1 + 4)) ^ (*(const u16 *)(addr2 + 4)));

	return fold == 0;
#else
	const u16 *a = (const u16 *)addr1;
	const u16 *b = (const u16 *)addr2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
#endif
}

static inline void ether_addr_copy(u8 *dst, const u8 *src)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)
	*(u32 *)dst = *(const u32 *)src;
	*(u16 *)(dst + 4) = *(const u16 *)(src + 4);
#else
	u16 *a = (u16 *)dst;
	const u16 *b = (const u16 *)src;

	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
#endif
}
#endif

static inline u32 ieee80211_usf_to_sf(u8 usf)
{
	return (usf == 1) ? 10 : (usf == 2) ? 1000 : (usf == 3) ? 10000 : 1;
}

struct ieee80211_sta *ieee80211_find_all_sta(struct ieee80211_vif *vif,
					     u8 *adddr);

u8 *ieee80211_append_ie(struct sk_buff *skb, u8 eid, u8 len);

struct sk_buff *ieee80211_deauth_get(struct ieee80211_hw *hw, u8 *da, u8 *sa,
				     u8 *bssid, __le16 reason, struct ieee80211_sta *sta, bool tosta);

void ieee80211_iterate_active_netdev(struct ieee80211_hw *hw,
				     void (*iterator)(void *data,
						      struct net_device *dev),
				     void *data);

#endif
