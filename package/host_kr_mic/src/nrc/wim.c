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

#include <net/mac80211.h>
#include "nrc.h"
#include "wim.h"
#include "nrc-hif.h"
#include "nrc-fw.h"
#include "nrc-recovery.h"
#include "nrc-mac80211.h"

#if defined(CONFIG_SUPPORT_BD)
#include "nrc-bd.h"
#endif /* defined(CONFIG_SUPPORT_BD) */

static void nrc_wim_skb_bind_vif(struct sk_buff *skb, struct ieee80211_vif *vif)
{
	struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);

	txi->control.vif = vif;
}


struct sk_buff *nrc_wim_alloc_skb(struct nrc *nw, u16 cmd, int size)
{
	struct sk_buff *skb;
	struct wim *wim;

	skb = dev_alloc_skb(size + sizeof(struct hif) + sizeof(struct wim));
	if (!skb)
		return NULL;

	/* Increase the headroom */
	skb_reserve(skb, sizeof(struct hif));

	/* Put wim header */
	wim = (struct wim *)skb_put(skb, sizeof(*wim));
	memset(wim, 0, sizeof(*wim));
	wim->cmd = cmd;
	wim->seqno = nw->wim_seqno++;

	nrc_wim_skb_bind_vif(skb, NULL);

	return skb;
}

struct sk_buff *nrc_wim_alloc_skb_vif(struct nrc *nw, struct ieee80211_vif *vif,
				      u16 cmd, int size)
{
	struct sk_buff *skb;

	skb = nrc_wim_alloc_skb(nw, cmd, size);
	if (!skb)
		return NULL;

	nrc_wim_skb_bind_vif(skb, vif);

	return skb;
}

int nrc_xmit_wim_simple_request(struct nrc *nw, int cmd)
{
	struct sk_buff *skb = nrc_wim_alloc_skb(nw, cmd, 0);

	return nrc_xmit_wim_request(nw, skb);
}

struct sk_buff *nrc_xmit_wim_simple_request_wait(struct nrc *nw, int cmd,
		int timeout)
{
	struct sk_buff *skb = nrc_wim_alloc_skb(nw, cmd, 0);

	return nrc_xmit_wim_request_wait(nw, skb, timeout);
}

static void wim_set_tlv(struct wim_tlv *tlv, u16 t, u16 l)
{
	tlv->t = t;
	tlv->l = l;
}

void *nrc_wim_skb_add_tlv(struct sk_buff *skb, u16 T, u16 L, void *V)
{
	struct wim_tlv *tlv;

	if (L == 0) {
		tlv = (struct wim_tlv *)(skb_put(skb, sizeof(struct wim_tlv)));
		wim_set_tlv(tlv, T, L);
		return (void *)skb->data;
	}

	tlv = (struct wim_tlv *)(skb_put(skb, tlv_len(L)));
	wim_set_tlv(tlv, T, L);

	if (V)
		memcpy(tlv->v, V, L);

	return (void *)tlv->v;
}

void nrc_wim_set_aid(struct nrc *nw, struct sk_buff *skb, u16 aid)
{
	nrc_wim_skb_add_tlv(skb, WIM_TLV_AID, sizeof(u16), &aid);
}

void nrc_wim_add_mac_addr(struct nrc *nw, struct sk_buff *skb, u8 *addr)
{
	nrc_wim_skb_add_tlv(skb, WIM_TLV_MACADDR, ETH_ALEN, addr);
}

void nrc_wim_set_bssid(struct nrc *nw, struct sk_buff *skb, u8 *bssid)
{
	nrc_wim_skb_add_tlv(skb, WIM_TLV_BSSID, ETH_ALEN, bssid);
}

void nrc_wim_set_ndp_preq(struct nrc *nw, struct sk_buff *skb, u8 enable)
{
	nrc_wim_skb_add_tlv(skb, WIM_TLV_NDP_PREQ, sizeof(u8), &enable);
}

void nrc_wim_set_legacy_ack(struct nrc *nw, struct sk_buff *skb, u8 enable)
{
	nrc_wim_skb_add_tlv(skb, WIM_TLV_LEGACY_ACK, sizeof(u8), &enable);
}

int nrc_wim_change_sta(struct nrc *nw, struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta, u8 cmd, bool sleep)
{
	struct sk_buff *skb;
	struct wim_sta_param *p;

	if (!sta)
		return -EINVAL;

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_STA_CMD,
				    tlv_len(sizeof(*p)));

	p = nrc_wim_skb_add_tlv(skb, WIM_TLV_STA_PARAM, sizeof(*p), NULL);
	memset(p, 0, sizeof(*p));

	p->cmd = cmd;
	p->flags = 0;
	p->sleep = sleep;
	ether_addr_copy(p->addr, sta->addr);
	p->aid = sta->aid;

	return nrc_xmit_wim_request(nw, skb);
}

int nrc_wim_hw_scan(struct nrc *nw, struct ieee80211_vif *vif,
		    struct cfg80211_scan_request *req,
		    struct ieee80211_scan_ies *ies)
{
	struct sk_buff *skb;
	struct wim_scan_param *p;
	int i, size = tlv_len(sizeof(struct wim_scan_param));
#if defined(CONFIG_SUPPORT_BD)
	int j;
	bool avail_ch_flag = false;
#endif /* defined(CONFIG_SUPPORT_BD) */
	if (ies) {
		size += tlv_len(ies->common_ie_len);
		size += ies->len[NL80211_BAND_2GHZ];
		size += ies->len[NL80211_BAND_5GHZ];
	} else {
		size += tlv_len(req->ie_len);
	}

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SCAN_START, size);

	/* WIM_TL_SCAN_PARAM */
	p = nrc_wim_skb_add_tlv(skb, WIM_TLV_SCAN_PARAM, sizeof(*p), NULL);
	memset(p, 0, sizeof(*p));

	if (WARN_ON(req->n_channels > WIM_MAX_SCAN_CHANNEL))
		req->n_channels = WIM_MAX_SCAN_CHANNEL;

	p->n_channels = req->n_channels;
	for (i = 0; i < req->n_channels; i++)
		p->channel[i] = req->channels[i]->center_freq;
#if defined(CONFIG_SUPPORT_BD)
#if BD_DEBUG
	nrc_dbg(NRC_DBG_MAC, "org_ch_freq");
	for (i = 0; i < p->n_channels; i++) {
		nrc_dbg(NRC_DBG_MAC, "%u", p->channel[i]);
	}
#endif

	if(g_supp_ch_list.num_ch) {
		for (i = 0; i < req->n_channels; i++) {
			for(j=0; j< g_supp_ch_list.num_ch; j++) {
				if(p->channel[i] == g_supp_ch_list.nons1g_ch_freq[j])
					avail_ch_flag = true;
			}
			if(!avail_ch_flag) {
				p->n_channels--;
				p->channel[i] = 0;
			} else
				avail_ch_flag = false;
		}

		j = 0;
		for (i = 0; i < req->n_channels; i++) {
			if(p->channel[i]) {
				p->channel[j] = req->channels[i]->center_freq;
				j++;
			}
		}
#if BD_DEBUG	
		nrc_dbg(NRC_DBG_MAC, "%s: org_num_ch %u  num_ch %u", __func__,
			req->n_channels,
			p->n_channels);

		nrc_dbg(NRC_DBG_MAC, "mod_ch_freq");
		for (i = 0; i < p->n_channels; i++) {
			nrc_dbg(NRC_DBG_MAC, "%u", p->channel[i]);
		}
#endif
	}
#endif /* defined(CONFIG_SUPPORT_BD) */

	p->n_ssids = req->n_ssids;
	for (i = 0; i < req->n_ssids; i++) {
		p->ssid[i].ssid_len = req->ssids[i].ssid_len;
		memcpy(p->ssid[i].ssid, req->ssids[i].ssid,
				req->ssids[i].ssid_len);
	}

	if (ies) {
		u8 *p;
		int b;

		p = nrc_wim_skb_add_tlv(skb,
					WIM_TLV_SCAN_PROBE_REQ_IE,
					ies->common_ie_len +
					ies->len[NL80211_BAND_2GHZ] +
					ies->len[NL80211_BAND_5GHZ],
					NULL);

		for (b = NL80211_BAND_2GHZ; b < ARRAY_SIZE(ies->ies); b++)
			if (ies->ies[b] && ies->len[b] > 0) {
				memcpy(p, ies->ies[b], ies->len[b]);
				p += ies->len[b];
			}

		if (ies->common_ies && ies->common_ie_len > 0) {
			memcpy(p, ies->common_ies, ies->common_ie_len);
			p += ies->common_ie_len;
		}

	} else if (req->ie)
		nrc_wim_skb_add_tlv(skb,
				    WIM_TLV_SCAN_PROBE_REQ_IE,
				    req->ie_len,
				    (void *)req->ie);

	return nrc_xmit_wim_request(nw, skb);
}

static char *ieee80211_cipher_str(u32 cipher)
{
	switch (cipher) {
	case WLAN_CIPHER_SUITE_WEP40:
		return "WEP40";
	case WLAN_CIPHER_SUITE_WEP104:
		return "WEP104";
	case WLAN_CIPHER_SUITE_TKIP:
		return "TKIP";
	case WLAN_CIPHER_SUITE_CCMP:
		return "CCMP";
	default:
		return NULL;
	}
};

enum wim_cipher_type nrc_to_wim_cipher_type(u32 cipher)
{
	switch (cipher) {
	case WLAN_CIPHER_SUITE_WEP40:
		return WIM_CIPHER_TYPE_WEP40;
	case WLAN_CIPHER_SUITE_WEP104:
		return WIM_CIPHER_TYPE_WEP104;
	case WLAN_CIPHER_SUITE_TKIP:
		return WIM_CIPHER_TYPE_TKIP;
	case WLAN_CIPHER_SUITE_CCMP:
		return WIM_CIPHER_TYPE_CCMP;
	case WLAN_CIPHER_SUITE_AES_CMAC:
	case WLAN_CIPHER_SUITE_BIP_GMAC_128:
	case WLAN_CIPHER_SUITE_BIP_GMAC_256:
		return WIM_CIPHER_TYPE_NONE;
	default:
		return WIM_CIPHER_TYPE_INVALID;
	}
}

u32 nrc_to_ieee80211_cipher(enum wim_cipher_type cipher)
{
	switch (cipher) {
	case WIM_CIPHER_TYPE_WEP40:
		return WLAN_CIPHER_SUITE_WEP40;
	case WIM_CIPHER_TYPE_WEP104:
		return WLAN_CIPHER_SUITE_WEP104;
	case WIM_CIPHER_TYPE_TKIP:
		return WLAN_CIPHER_SUITE_TKIP;
	case WIM_CIPHER_TYPE_CCMP:
		return WLAN_CIPHER_SUITE_TKIP;
	default:
		return -1;
	}
}

int nrc_wim_install_key(struct nrc *nw, enum set_key_cmd cmd,
			struct ieee80211_vif *vif,
			struct ieee80211_sta *sta,
			struct ieee80211_key_conf *key)
{
	struct sk_buff *skb;
	struct wim_key_param *p;
	u8 cipher;
	const u8 *addr;
	u16 aid = 0;

	cipher = nrc_to_wim_cipher_type(key->cipher);
	if (cipher == -1)
		return -ENOTSUPP;

	if (key->keyidx > WIM_KEY_MAX_INDEX)
		return -EINVAL;

	nrc_dbg(NRC_DBG_MAC, "%s: cmd=%s, key=(%s,%s,%d)\n", __func__,
		(cmd == SET_KEY) ? "install" : "delete",
		(key->flags & IEEE80211_KEY_FLAG_PAIRWISE) ? "PTK" : "GTK",
		ieee80211_cipher_str(key->cipher),
		key->keyidx);

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET_KEY + (cmd - SET_KEY),
				tlv_len(sizeof(*p)));

	p = nrc_wim_skb_add_tlv(skb, WIM_TLV_KEY_PARAM, sizeof(*p), NULL);

	memset(p, 0, sizeof(*p));

	if (sta) {
		addr = sta->addr;
	} else if (vif->type == NL80211_IFTYPE_AP
			|| vif->type == NL80211_IFTYPE_MESH_POINT
			|| vif->type == NL80211_IFTYPE_P2P_GO) {
		addr = vif->addr;
	} else {
		addr = vif->bss_conf.bssid;
	}

	if (key->flags & IEEE80211_KEY_FLAG_PAIRWISE) {
		if (!((vif->type == NL80211_IFTYPE_AP)
				|| (vif->type == NL80211_IFTYPE_MESH_POINT)
				|| (vif->type == NL80211_IFTYPE_P2P_GO)))
			aid = vif->bss_conf.aid;
		else if (sta)
			aid = sta->aid;
	} else
		aid = 0;

	ether_addr_copy(p->mac_addr, addr);
	p->aid = aid;
	memcpy(p->key, key->key, key->keylen);
	p->cipher_type = cipher;
	p->key_index = key->keyidx;
	p->key_len = key->keylen;
	p->key_flags = (key->flags & IEEE80211_KEY_FLAG_PAIRWISE) ?
		WIM_KEY_FLAG_PAIRWISE : WIM_KEY_FLAG_GROUP;

	nrc_dbg(NRC_DBG_MAC, "%s: cmd=%s, key=(%s,%s,%d), aid=%d\n", __func__,
		(cmd == SET_KEY) ? "install" : "delete",
		(key->flags & IEEE80211_KEY_FLAG_PAIRWISE) ? "PTK" : "GTK",
		ieee80211_cipher_str(key->cipher),
		key->keyidx,
		p->aid);


	return nrc_xmit_wim_request(nw, skb);
}


static int to_wim_sta_type(enum nl80211_iftype type)
{
	int sta_type;

	switch (type) {
	case NL80211_IFTYPE_STATION:
		sta_type = WIM_STA_TYPE_STA;
		break;
	case NL80211_IFTYPE_AP:
		sta_type = WIM_STA_TYPE_AP;
		break;
	case NL80211_IFTYPE_P2P_GO:
		sta_type = WIM_STA_TYPE_P2P_GO;
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
		sta_type = WIM_STA_TYPE_P2P_GC;
		break;
#ifdef CONFIG_SUPPORT_P2P
	case NL80211_IFTYPE_P2P_DEVICE:
		sta_type = WIM_STA_TYPE_P2P_DEVICE;
		break;
#endif
	case NL80211_IFTYPE_MONITOR:
		sta_type = WIM_STA_TYPE_MONITOR;
		break;
	case NL80211_IFTYPE_MESH_POINT:
		sta_type = WIM_STA_TYPE_MESH_POINT;
		break;
	case NL80211_IFTYPE_UNSPECIFIED:
		sta_type = WIM_STA_TYPE_NONE;
		break;
	default:
		sta_type = -ENOTSUPP;
		break;
	}

	return sta_type;
}

int nrc_wim_set_sta_type(struct nrc *nw, struct ieee80211_vif *vif)
{
	struct sk_buff *skb;
	int sta_type, skb_len;

	if ((nw->vif[0] && nw->vif[1])
	    || vif->type == NL80211_IFTYPE_MESH_POINT) {
		sw_enc = 1;
	}

	sta_type = to_wim_sta_type(ieee80211_iftype_p2p(vif->type, vif->p2p));
	if (sta_type < 0)
		return -ENOTSUPP;

	skb_len = tlv_len(sizeof(u32));
	if (nrc_mac_is_s1g(nw) && sta_type == WIM_STA_TYPE_AP) {
		skb_len += tlv_len(sizeof(u8));
	}

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, skb_len);

	nrc_wim_skb_add_tlv(skb, WIM_TLV_STA_TYPE, sizeof(u32), &sta_type);
	if (nrc_mac_is_s1g(nw) && sta_type == WIM_STA_TYPE_AP) {
		nrc_wim_skb_add_tlv(skb, WIM_TLV_NDP_ACK_1M, sizeof(u8), &ndp_ack_1m);
	}

	return nrc_xmit_wim_request(nw, skb);
}

int nrc_wim_unset_sta_type(struct nrc *nw, struct ieee80211_vif *vif)
{
	struct sk_buff *skb;
	int sta_type;

	sta_type = to_wim_sta_type(NL80211_IFTYPE_UNSPECIFIED);
	if (sta_type < 0)
		return -ENOTSUPP;

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, tlv_len(sizeof(u32)));

	nrc_wim_skb_add_tlv(skb, WIM_TLV_STA_TYPE, sizeof(u32), &sta_type);

	return nrc_xmit_wim_request(nw, skb);
}

static int nrc_wim_set_sta_mac_addr(struct nrc *nw, struct ieee80211_vif *vif,
				char *addr, bool enable, bool p2p)
{
	struct sk_buff *skb;
	struct wim_addr_param *p;

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_SET, tlv_len(ETH_ALEN));

	p = nrc_wim_skb_add_tlv(skb, WIM_TLV_MACADDR_PARAM, sizeof(*p), NULL);
	p->enable = enable;
	p->p2p = p2p;
	ether_addr_copy(p->addr, addr);

	return nrc_xmit_wim_request(nw, skb);
}
int nrc_wim_set_p2p_addr(struct nrc *nw, struct ieee80211_vif *vif)
{
	return nrc_wim_set_sta_mac_addr(nw, vif, vif->addr, true, true);
}

int nrc_wim_set_mac_addr(struct nrc *nw, struct ieee80211_vif *vif)
{
	return nrc_wim_set_sta_mac_addr(nw, vif, vif->addr, true, false);
}

bool nrc_wim_request_keep_alive(struct nrc *nw)
{
	int ret;

	ret = nrc_xmit_wim_simple_request(nw, WIM_CMD_KEEP_ALIVE);
	return (ret == 0);
}

void nrc_wim_handle_fw_ready(struct nrc *nw)
{
	struct nrc_hif_device *hdev = nw->hif;

#if defined(CONFIG_SUPPORT_BD)
	struct regulatory_request request;
	request.alpha2[0] = nw->alpha2[0];
	request.alpha2[1] = nw->alpha2[1];
#endif
	nrc_ps_dbg("[%s,L%d]\n", __func__, __LINE__);
	atomic_set(&nw->fw_state, NRC_FW_ACTIVE);
	nrc_hif_resume(hdev);
#if defined(CONFIG_SUPPORT_BD)
	nrc_reg_notifier(nw->hw->wiphy, &request);
#endif
}

#define MAC_ADDR_LEN 6
void nrc_wim_handle_fw_reload(struct nrc *nw)
{
	nrc_ps_dbg("[%s,L%d]\n", __func__, __LINE__);
	atomic_set(&nw->fw_state, NRC_FW_LOADING);
	//nrc_recovery_wdt_stop(nw);
	nrc_hif_cleanup(nw->hif);
	msleep(200);
	if (nrc_check_fw_file(nw)) {
		nrc_download_fw(nw);
		nw->hif->hif_ops->config(nw->hif);
		msleep(500);
		nrc_release_fw(nw);
	}
}

void nrc_wim_handle_req_deauth(struct nrc *nw)
{
	nrc_ps_dbg("[%s,L%d]\n", __func__, __LINE__);

	if (power_save >= NRC_PS_DEEPSLEEP_TIM)
		ieee80211_connection_loss(nw->vif[0]);
}

static int nrc_wim_request_handler(struct nrc *nw,
				   struct sk_buff *skb)
{
	struct wim *wim = (struct wim *)skb->data;

	switch (wim->cmd) {
	case WIM_CMD_FW_RELOAD:
		nrc_wim_handle_fw_reload(nw);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int nrc_wim_response_handler(struct nrc *nw,
				    struct sk_buff *skb)
{
	nw->last_wim_responded = skb;
	if (completion_done(&nw->wim_responded)) {
	/* No completion waiters, free the SKB */
		pr_err("no completion");
		return 0;
	}
	complete(&nw->wim_responded);
	return 1;
}

#ifdef CONFIG_USE_TXQ
static int nrc_wim_update_tx_credit(struct nrc *nw, struct wim *wim)
{
	struct wim_credit_report *r = (void *)(wim + 1);
	int ac;

	for (ac = 0; ac < (IEEE80211_NUM_ACS*3); ac++)
		atomic_set(&nw->tx_credit[ac], r->v.ac[ac]);

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_WIM,
		"accepted credit ac[0]=%d, ac[1]=%d, ac[2]=%d, ac[3]=%d",
			(int)atomic_read(&nw->tx_credit[0]),
			(int)atomic_read(&nw->tx_credit[1]),
			(int)atomic_read(&nw->tx_credit[2]),
			(int)atomic_read(&nw->tx_credit[3]));
	nrc_dbg(NRC_DBG_WIM,
		"accepted credit ac[6]=%d, ac[7]=%d, ac[8]=%d, ac[9]=%d",
			(int)atomic_read(&nw->tx_credit[6]),
			(int)atomic_read(&nw->tx_credit[7]),
			(int)atomic_read(&nw->tx_credit[8]),
			(int)atomic_read(&nw->tx_credit[9]));
#endif

	nrc_kick_txq(nw->hw);

	return 0;
}
#endif

static int nrc_wim_event_handler(struct nrc *nw,
				 struct sk_buff *skb)
{
	struct ieee80211_vif *vif = NULL;
	struct wim *wim = (void *) skb->data;
	struct hif *hif = (void *)(skb->data - sizeof(*hif));
	//struct wim_sleep_duration *sleep_duration;
	//struct sk_buff *ps_skb;

	/* hif->vifindex to vif */
	if (hif->vifindex != -1)
		vif = nw->vif[hif->vifindex];

	if ((!atomic_read(&nw->d_deauth.delayed_deauth) && !vif) || 
		  vif->type == NL80211_IFTYPE_MONITOR) {
		return 0;
	}

	switch (wim->event) {
	case WIM_EVENT_SCAN_COMPLETED:
		nrc_dbg(NRC_DBG_HIF, "scan completed");
		nrc_mac_cancel_hw_scan(nw->hw, vif);
		break;
	case WIM_EVENT_READY:
		nrc_wim_handle_fw_ready(nw);
		break;
	case WIM_EVENT_CREDIT_REPORT:
#ifdef CONFIG_USE_TXQ
		nrc_wim_update_tx_credit(nw, wim);
#endif
		break;

	case WIM_EVENT_PS_READY:
		/* yj.kim 06/03/2020 - It needs to add more procedures when clock is off during modem_sleep
		*sleep_duration = (void *)(wim + 1);
		*nw->sleep_ms = sleep_duration->v.sleep_ms;
		*nrc_ps_dbg("WIM_EVENT_PS_READY:%d ms", nw->sleep_ms);
		*ps_skb = nrc_wim_alloc_skb_vif(nw, vif,
		*		   WIM_CMD_SLEEP, WIM_MAX_SIZE);
		*nrc_wim_skb_add_tlv(ps_skb, WIM_TLV_SLEEP_DURATION,
		*			sizeof(struct wim_sleep_duration_param),
		*			&nw->sleep_ms);
		*nrc_xmit_wim_request(nw, ps_skb);
		*/ 
		break;

	case WIM_EVENT_PS_WAKEUP:
		/* yj.kim 06/03/2020 - It needs to add more procedures when clock is off during modem_sleep
		* nrc_ps_dbg("WIM_EVENT_PS_WAKEUP");
		*/
		//atomic_set(&nw->fw_state, NRC_FW_ACTIVE);
		break;

	case WIM_EVENT_KEEP_ALIVE:
		break;
	case WIM_EVENT_REQ_DEAUTH:
		nrc_wim_handle_req_deauth(nw);
		break;
	case WIM_EVENT_CSA:
		ieee80211_csa_finish(vif);
		break;
	case WIM_EVENT_CH_SWITCH:
		ieee80211_chswitch_done(vif, true);
		break;
	}

	return 0;
}

typedef int (*wim_rx_func)(struct nrc *nw, struct sk_buff *skb);

static wim_rx_func wim_rx_handler[] = {
	[HIF_WIM_SUB_REQUEST] = nrc_wim_request_handler,
	[HIF_WIM_SUB_RESPONSE] = nrc_wim_response_handler,
	[HIF_WIM_SUB_EVENT] = nrc_wim_event_handler,
};


int nrc_wim_rx(struct nrc *nw, struct sk_buff *skb, u8 subtype)
{
	int ret;

#if defined(CONFIG_NRC_HIF_PRINT_RX_DATA)
	skb_push(skb, sizeof(struct hif));
	nrc_dump_wim(skb);
	skb_pull(skb, sizeof(struct hif));
#endif

	ret = wim_rx_handler[subtype](nw, skb);

	if (ret != 1)
	/* Free the skb later (e.g. on Response WIM handler) */
		dev_kfree_skb(skb);

	return 0; /* for the time being */
}

/* to be removed */
int __nrc_wim_ampdu_action(struct nrc *nw, enum WIM_AMPDU_ACTION action,
		struct ieee80211_sta *sta, u16 tid)
{
	struct sk_buff *skb;

	skb = nrc_wim_alloc_skb(nw, WIM_CMD_AMPDU_ACTION,
			tlv_len(sizeof(u16)) + tlv_len(ETH_ALEN));

	nrc_wim_skb_add_tlv(skb, WIM_TLV_AMPDU_MODE, sizeof(u16), &action);
	nrc_wim_add_mac_addr(nw, skb, sta->addr);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_TID, sizeof(u16), &tid);

	return nrc_xmit_wim_request(nw, skb);
}

int nrc_wim_ampdu_action(struct nrc *nw, struct ieee80211_vif *vif,
			enum WIM_AMPDU_ACTION action,
			struct ieee80211_sta *sta, u16 tid)
{
	struct sk_buff *skb;

	skb = nrc_wim_alloc_skb_vif(nw, vif, WIM_CMD_AMPDU_ACTION,
				tlv_len(sizeof(u16)) + tlv_len(ETH_ALEN));

	nrc_wim_skb_add_tlv(skb, WIM_TLV_AMPDU_MODE, sizeof(u16), &action);
	nrc_wim_add_mac_addr(nw, skb, sta->addr);
	nrc_wim_skb_add_tlv(skb, WIM_TLV_TID, sizeof(u16), &tid);

	return nrc_xmit_wim_request(nw, skb);
}
