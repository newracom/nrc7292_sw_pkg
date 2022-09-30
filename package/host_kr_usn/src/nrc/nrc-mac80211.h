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

#ifndef _NRC_MAC80211_H_
#define _NRC_MAC80211_H_

#include "nrc.h"
#include "mac80211-ext.h"

#define NRC_MAC80211_MAX_SCANSSID   (32)
#define NRC_MAC80211_MAX_SCANIELEN  (IEEE80211_MAX_DATA_LEN)
#define NRC_MAC80211_ROC_DURATION   (1000)
#define NRC_MAC80211_RCU_LOCK_THRESHOLD	(1000)

#ifdef CONFIG_SUPPORT_TX_CONTROL
#else
struct ieee80211_tx_control {
	struct ieee80211_sta *sta;
};
#endif

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#else
#define NL80211_IFTYPE_P2P_DEVICE	10
#define WLAN_EID_BSS_MAX_IDLE_PERIOD	90

struct ieee80211_scan_ies {
	const u8 *ies[IEEE80211_NUM_BANDS];
	size_t len[IEEE80211_NUM_BANDS];
	const u8 *common_ies;
	size_t common_ie_len;
};
#endif

int nrc_mac80211_init(struct nrc *nr);
void nrc_mac80211_exit(struct nrc *nr);

struct nrc *nrc_alloc_hw(struct platform_device *pdev);
void nrc_free_hw(struct nrc *nw);

int nrc_register_hw(struct nrc *nw);
void nrc_unregister_hw(struct nrc *nw);

void nrc_kick_txq(struct ieee80211_hw *hw);
int nrc_handle_frame(struct nrc *nw, struct sk_buff *skb);
void nrc_mac_cancel_hw_scan(struct ieee80211_hw *hw, struct ieee80211_vif *vif);

struct net_device *nrc_get_intf_by_name(const char *intf_name);

#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
void nrc_mac_tx(struct ieee80211_hw *hw, struct ieee80211_tx_control *control,
		struct sk_buff *skb);
#else
void nrc_mac_tx(struct ieee80211_hw *hw,
		struct sk_buff *skb);
#endif
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
int nrc_mac_conf_tx(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		    u16 ac, const struct ieee80211_tx_queue_params *params);
#else
int nrc_mac_conf_tx(struct ieee80211_hw *hw,
		    u16 ac, const struct ieee80211_tx_queue_params *params);
#endif
void nrc_mac_bss_info_changed(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif,
				     struct ieee80211_bss_conf *info,
				     u32 changed);
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
void nrc_mac_add_tlv_channel(struct sk_buff *skb,
					struct cfg80211_chan_def *chandef);
#else
void nrc_mac_add_tlv_channel(struct sk_buff *skb,
			struct ieee80211_conf *chandef);
#endif
int nrc_mac_sta_remove(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta);
void nrc_mac_stop(struct ieee80211_hw *hw);

int nrc_mac_rx(struct nrc *nw, struct sk_buff *skb);
void nrc_mac_trx_init(struct nrc *nw);

bool nrc_mac_is_s1g(struct nrc *nw);
bool nrc_access_vif(struct nrc *nw);

void nrc_free_vif_index(struct nrc *nw, struct ieee80211_vif *vif);

#ifdef CONFIG_NEW_REG_NOTIFIER
void nrc_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request);
#else
int nrc_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request);
#endif

void nrc_mac_clean_txq(struct nrc *nw);

#endif
