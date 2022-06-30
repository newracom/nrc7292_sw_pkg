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

#ifndef _WIM_H_
#define _WIM_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/if_ether.h>
#include <net/mac80211.h>
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#else
#include "nrc-mac80211.h"
#endif

#include "nrc-wim-types.h"

struct nrc;

struct wim {
	union {
		u16 cmd;
		u16 resp;
		u16 event;
	};
	u8 seqno;
	u8 n_tlvs;
	u8 payload[0];
} __packed;

struct wim_tlv {
	u16 t;
	u16 l;
	u8  v[0];
} __packed;

#define tlv_len(l) (sizeof(struct wim_tlv) + (l))

extern void nrc_hif_cleanup(struct nrc_hif_device *dev);

/**
 * nrc_wim_alloc_skb - allocate a sk_buff for wim message transmission
 * @nw: pointer to nrc hw
 * @vif: vif
 * @cmd: wim command (enum WIM_CMD_ID)
 * @size: wim payload size in byte (excluding wim header)
 *
 * This function allocates a sk_buff,  prepends a wim header, and fills
 * a couple of wim header fields. A pointer to the allocated sk_buff is
 * returned.
 */
struct sk_buff *nrc_wim_alloc_skb(struct nrc *nw, u16 cmd, int size);

struct sk_buff *nrc_wim_alloc_skb_vif(struct nrc *nw, struct ieee80211_vif *vif,
				      u16 cmd, int size);
/**
 * nrc_xmit_wim_simple_request - sends a wim request with no payload
 * @nw: pointer to nrc hw
 * @cmd: wim command to send (enum WIM_CMD_ID)
 */
int nrc_xmit_wim_simple_request(struct nrc *nw, int cmd);

/**
 * nrc_xmit_wim_simple_request_wait - sends a wim request with no payload
 * and wait for response within given timeout
 * @nw: pointer to nrc hw
 * @cmd: wim command to send (enum WIM_CMD_ID)
 * @timeout: timeout in jiffies
 */
struct sk_buff *nrc_xmit_wim_simple_request_wait(struct nrc *nw,
		int cmd, int timeout);


/**
 * nrc_wim_skb_add_tlv - append a TLV parameter
 * @skb: buffer to use.
 * @T: type of the parameter to add.
 * @L: length of the parameter to add.
 * @V: value of the parameter.
 *
 * This function appends a TLV parameter to @skb. If @V is non-NULL,
 * @L bytes are copied from the buffer pointed to by it. Ohterwise,
 * the copy does not take place. A pointer to the first byte of V in
 * @skb is returned.
 */
void *nrc_wim_skb_add_tlv(struct sk_buff *skb, u16 T, u16 L, void *V);

/**
 * nrc_wim_set_aid - adds AID TLV
 * @nw: pointer to nrc hw
 * @skb: sk_buff containing wim set command message.
 * @aid: AID to be set
 */
void nrc_wim_set_aid(struct nrc *nw, struct sk_buff *skb, u16 aid);

/**
 * nrc_wim_add_mac_addr - adds MACADDR TLV
 * @nw: pointer to nrc hw
 * @skb: sk_buff containing wim set command message.
 * @addr: mac address to set
 */
void nrc_wim_add_mac_addr(struct nrc *nw, struct sk_buff *skb, u8 *addr);

/**
 * nrc_wim_set_bssid - adds BSSID TLV
 * @nw: pointer to nrc hw
 * @skb: sk_buff containing wim set command message.
 * @bssid: bssid to set
 */
void nrc_wim_set_bssid(struct nrc *nw, struct sk_buff *skb, u8 *bssid);

/**
 * nrc_wim_set_ndp_preq - set NDP probe request
 * @nw: pointer to nrc hw
 * @skb: sk_buff containing wim set command message.
 * @bssid: bssid to set
 */
void nrc_wim_set_ndp_preq(struct nrc *nw, struct sk_buff *skb, u8 enable);

/**
 * nrc_wim_set_legacy_ack - set legacy ack mode
 * @nw: pointer to nrc hw
 * @skb: sk_buff containing wim set command message.
 * @enable: enable(1)/disable(0)
 */
void nrc_wim_set_legacy_ack(struct nrc *nw, struct sk_buff *skb, u8 enable);

int nrc_wim_change_sta(struct nrc *nw, struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta, u8 cmd, bool sleep);

int nrc_wim_hw_scan(struct nrc *nw, struct ieee80211_vif *vif,
		    struct cfg80211_scan_request *req,
		    struct ieee80211_scan_ies *ies);

int nrc_wim_set_sta_type(struct nrc *nw, struct ieee80211_vif *vif);
int nrc_wim_unset_sta_type(struct nrc *nw, struct ieee80211_vif *vif);
int nrc_wim_set_mac_addr(struct nrc *nw, struct ieee80211_vif *vif);
int nrc_wim_set_p2p_addr(struct nrc *nw, struct ieee80211_vif *vif);

bool nrc_wim_request_keep_alive(struct nrc *nw);
void nrc_wim_handle_fw_request(struct nrc *nw);

enum wim_cipher_type nrc_to_wim_cipher_type(u32 cipher);
u32 nrc_to_ieee80211_cipher(enum wim_cipher_type cipher);

int nrc_wim_install_key(struct nrc *nw, enum set_key_cmd cmd,
			struct ieee80211_vif *vif,
			struct ieee80211_sta *sta,
			struct ieee80211_key_conf *key);


int nrc_wim_ampdu_action(struct nrc *nw, struct ieee80211_vif *vif,
			 enum WIM_AMPDU_ACTION action,
			 struct ieee80211_sta *sta, u16 tid);
/**
 * nrc_wim_rx - handles a received wim message from the target
 * @nw: pointer to nrc hw.
 * @skb: received wim message
 * @subtype: HIF_WIM_SUB_XXX, where XXX is either of REQUEST/RESPONSE/EVENT.
 *
 * This function is a callback invoked by HIF receive thread for
 * incoming WIM message.
 *
 * The function returns zero if the received @skb is successfully handled,
 * and negative value on failure.
 */
int nrc_wim_rx(struct nrc *nw, struct sk_buff *skb, u8 subtype);
int nrc_wim_pm_req(struct nrc *nw, uint32_t cmd, uint64_t arg);

#endif
