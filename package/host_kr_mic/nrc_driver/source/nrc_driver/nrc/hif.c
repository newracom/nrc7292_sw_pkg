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

#include "nrc-hif.h"
#include "nrc-recovery.h"
#include "nrc-build-config.h"

#if defined(CONFIG_NRC_HIF_SSP)
#include "nrc-hif-ssp.h"
#elif defined(CONFIG_NRC_HIF_UART)
#include "nrc-hif-uart.h"
#elif defined(CONFIG_NRC_HIF_DEBUG)
#include "nrc-hif-debug.h"
#elif defined(CONFIG_NRC_HIF_CSPI)
#include "nrc-hif-cspi.h"
#elif defined(CONFIG_NRC_HIF_SDIO)
#include "nrc-hif-sdio.h"
#endif

#include "nrc-mac80211.h"
#include "nrc-netlink.h"
#include "wim.h"
#include "nrc-dump.h"

#define to_nw(dev)	((dev)->nw)

static void nrc_hif_work(struct work_struct *work);

static int hif_receive_skb(struct nrc_hif_device *dev, struct sk_buff *skb);

struct nrc_hif_device *nrc_hif_init(struct nrc *nw)
{
	struct nrc_hif_device *dev = NULL;
	int i;

#ifdef CONFIG_NRC_HIF_SPI
	dev = nrc_hif_spi_init(nw);
#elif defined(CONFIG_NRC_HIF_SSP)
	dev = nrc_hif_ssp_init(nw);
#elif defined(CONFIG_NRC_HIF_UART)
	dev = nrc_hif_uart_init(nw);
#elif defined(CONFIG_NRC_HIF_DEBUG)
	dev = nrc_hif_debug_init(nw);
#elif defined(CONFIG_NRC_HIF_CSPI)
	dev = nrc_hif_cspi_init(nw);
#elif defined(CONFIG_NRC_HIF_SDIO)
	dev = nrc_hif_sdio_init(nw);
#endif
	if (!dev)
		goto out;

	dev->suspended = false;

	/* Initialize wim and frame queue */
	for (i = 0; i < ARRAY_SIZE(dev->queue); i++)
		skb_queue_head_init(&dev->queue[i]);

	INIT_WORK(&dev->work, nrc_hif_work);

	dev->hif_ops->receive = hif_receive_skb;

	nrc_hif_start(dev);

out:
	return dev;
}

void nrc_hif_free_skb(struct nrc *nw, struct sk_buff *skb)
{
	struct hif *hif = (void *)skb->data;
	struct frame_hdr *fh = (struct frame_hdr *)(hif + 1);
	bool ack = true;
	int credit;

	if (hif->type == HIF_TYPE_FRAME) {
		struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);

		ieee80211_tx_info_clear_status(txi);
		if (!(txi->flags & IEEE80211_TX_CTL_NO_ACK) && ack)
			txi->flags |= IEEE80211_TX_STAT_ACK;

		credit = DIV_ROUND_UP(skb->len, nw->fwinfo.buffer_size);
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
		nrc_dbg(NRC_DBG_HIF, "%s ac:%d, pend:%d, credit:%d\n",
				__func__, fh->flags.tx.ac,
				atomic_read(&nw->tx_pend[fh->flags.tx.ac]),
				credit);
#endif
		atomic_sub(credit, &nw->tx_pend[fh->flags.tx.ac]);
		/* Peel out our headers */
		skb_pull(skb, nw->fwinfo.tx_head_size);

		ieee80211_tx_status_irqsafe(nw->hw, skb);
#ifdef CONFIG_USE_TXQ
		nrc_kick_txq(nw->hw);
#endif
		return;
	}

	/* Other types not created by mac80211 */
	dev_kfree_skb(skb);
}

static bool nrc_hif_sleep_find(struct sk_buff *skb, u32 *duration_ms)
{
	struct hif *hif = (void *)skb->data;
	struct wim *wim = (void *)(hif+1);
	struct wim_sleep_duration *sleep_duration = (void *)(wim + 1);

	if ((hif->type != HIF_TYPE_WIM) ||
		(hif->subtype != HIF_WIM_SUB_REQUEST)) {
		return false;
	}

	if (wim->cmd == WIM_CMD_SLEEP &&
		sleep_duration->h.type == WIM_TLV_SLEEP_DURATION) {
		*duration_ms = sleep_duration->v.sleep_ms;
		return true;
	}

	return false;
}

static int nrc_hif_ctrl_ps(struct nrc_hif_device *hdev, struct sk_buff *skb)
{
	struct nrc *nw;
	int ret;
	u32 sleep_ms = 0;
	const u32 wakeup_ms = 10;

	if (!nrc_hif_sleep_find(skb, &sleep_ms))
		return HIF_TX_PASSOVER;

	nw = to_nw(hdev);
	nrc_hif_disable_irq(hdev);

	ret = nrc_hif_xmit(hdev, skb);
	nrc_hif_set_gpio(hdev, 0);
	msleep(sleep_ms+wakeup_ms);

	hdev->hif_ops->update(hdev);
	hdev->hif_ops->enable_irq(hdev);
	nrc_hif_set_gpio(hdev, 1);
	return HIF_TX_COMPLETE;
}

static void nrc_hif_work(struct work_struct *work)
{
	struct nrc *nw;
	struct nrc_hif_device *hdev;
	struct sk_buff *skb;
	int i, ret = 0;

	hdev = container_of(work, struct nrc_hif_device, work);
	nw = to_nw(hdev);

	for (i = ARRAY_SIZE(hdev->queue)-1; i >= 0; i--) {
		for (;;) {
			skb = skb_dequeue(&hdev->queue[i]);
			if (!skb)
				break;

			if (hdev->hif_ops->xmit) {
				if (nrc_hif_wait_for_xmit(hdev, skb) < 0) {
					nrc_hif_free_skb(to_nw(hdev), skb);
					return;
				}
				SYNC_LOCK(hdev);
				ret = nrc_hif_ctrl_ps(hdev, skb);
				if (ret == HIF_TX_PASSOVER)
					ret = nrc_hif_xmit(hdev, skb);
				SYNC_UNLOCK(hdev);
			} else {
				ret = nrc_hif_write(hdev, skb->data, skb->len);
				ret = HIF_TX_COMPLETE;
			}
			WARN_ON(ret < 0);

			if (ret != HIF_TX_QUEUED)
				nrc_hif_free_skb(to_nw(hdev), skb);
		}
	}
}

static int nrc_hif_enqueue_skb(struct nrc *nw, struct sk_buff *skb)
{
	struct hif *hif = (void *)skb->data;
	struct nrc_hif_device *hdev = nw->hif;

	if ((int)atomic_read(&nw->fw_state) != NRC_FW_ACTIVE)
		return -1;

	if ((hif == NULL) || (hdev == NULL))
		return -1;

	skb_queue_tail(&hdev->queue[hif->type], skb);

	if (nw->workqueue == NULL)
		return -1;

	queue_work(nw->workqueue, &hdev->work);
	return 0;
}

/**
 * nrc_hif_tx_wim - trasmit a wim message to target
 */
int nrc_xmit_wim(struct nrc *nw, struct sk_buff *skb, enum HIF_SUBTYPE stype)
{
	struct hif *hif;
	struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
	struct ieee80211_vif *vif = txi->control.vif;
	int len = skb->len;
	int ret = 0;

	/* Prepend HIF header */
	hif = (struct hif *)skb_push(skb, sizeof(struct hif));
	memset(hif, 0, sizeof(*hif));
	hif->type = HIF_TYPE_WIM;
	hif->subtype = stype;
	hif->len = len;
	hif->vifindex = hw_vifindex(vif);

#if defined(CONFIG_NRC_HIF_PRINT_TX_DATA)
	nrc_dump_wim(skb);
	print_hex_dump(KERN_DEBUG, "wim: ", DUMP_PREFIX_NONE, 16, 1,
		       skb->data, skb->len, false);
#endif

	if (!nw->loopback)
		ret = nrc_hif_enqueue_skb(nw, skb);
	else
		nrc_hif_free_skb(nw, skb);

	if (ret < 0)
		nrc_hif_free_skb(nw, skb);

	return ret;
}

int nrc_xmit_wim_request(struct nrc *nw, struct sk_buff *skb)
{
	return nrc_xmit_wim(nw, skb, HIF_WIM_SUB_REQUEST);
}

struct sk_buff *nrc_xmit_wim_request_wait(struct nrc *nw,
		struct sk_buff *skb, int timeout)
{
	nw->last_wim_responded = NULL;

	if (nrc_xmit_wim(nw, skb, HIF_WIM_SUB_REQUEST) < 0)
		return NULL;

	if (nw->last_wim_responded) {
		pr_err("received already");
		return nw->last_wim_responded;
	}

	reinit_completion(&nw->wim_responded);
	if (wait_for_completion_timeout(&nw->wim_responded,
			timeout) == 0)
		return NULL;

	return nw->last_wim_responded;
}

int nrc_xmit_wim_response(struct nrc *nw, struct sk_buff *skb)
{
	return nrc_xmit_wim(nw, skb, HIF_WIM_SUB_RESPONSE);
}

/**
 * nrc_skb_append_tx_info - Appends tx meta data to the end of skb
 *
 */
static u32 nrc_skb_append_tx_info(struct nrc *nw,
		   struct ieee80211_vif *vif,
		   struct ieee80211_sta *sta,
		   struct sk_buff *skb,
		   bool frame_injection)
{
	struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
	struct frame_tx_info_param *p;

	if (skb_tailroom(skb) < tlv_len(sizeof(*p))) {
		if (WARN_ON(pskb_expand_head(skb, 0, tlv_len(sizeof(*p)),
					GFP_ATOMIC)))
			return 0;
	}

	p = nrc_wim_skb_add_tlv(skb, WIM_TLV_EXTRA_TX_INFO, sizeof(*p), NULL);
#ifdef CONFIG_SUPPORT_TX_CONTROL
	p->use_rts = txi->control.use_rts;
	p->use_11b_protection = txi->control.use_cts_prot;
	p->short_preamble = txi->control.short_preamble;
#endif
	p->ampdu = !!(txi->flags & IEEE80211_TX_CTL_AMPDU);
	p->after_dtim = !!(txi->flags & IEEE80211_TX_CTL_SEND_AFTER_DTIM);
	p->no_ack = !!(txi->flags & IEEE80211_TX_CTL_NO_ACK);
	p->eosp = 0;
	p->inject = !!(frame_injection || wlantest);
	p->aid = !!sta ? sta->aid : 0;

#if defined(CONFIG_NRC_HIF_PRINT_TX_INFO)
	nrc_dbg(NRC_DBG_TX,
			"rts:%d cts_prot:%d sp:%d amdpu:%d noack:%d eosp:%d",
			p->use_rts, p->use_cts_prot,
			p->short_preamble, p->ampdu, p->no_ack, p->eosp);
#endif

	return tlv_len(sizeof(*p));
}

/**
 * nrc_xmit_injected_frame - transmit a injected 802.11 frame
 */
int nrc_xmit_injected_frame(struct nrc *nw,
		   struct ieee80211_vif *vif,
		   struct ieee80211_sta *sta,
		   struct sk_buff *skb)
{
	struct ieee80211_hdr *hdr = (void *)skb->data;
	struct frame_hdr *fh;
	struct hif *hif;
	__le16 fc = hdr->frame_control;
	struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
	int extra_len, ret = 0;
	int credit;

	extra_len = nrc_skb_append_tx_info(nw, vif, sta, skb, true);

	/* Prepend a HIF and frame header */
	hif = (void *)skb_push(skb, nw->fwinfo.tx_head_size);
	memset(hif, 0, nw->fwinfo.tx_head_size);
	hif->type = HIF_TYPE_FRAME;
	hif->len = skb->len - sizeof(*hif);
	hif->vifindex = hw_vifindex(vif);

	/* Frame header */
	fh = (void *)(hif + 1);
	fh->info.tx.tlv_len = extra_len;
	fh->info.tx.cipher = WIM_CIPHER_TYPE_NONE;
#ifdef CONFIG_SUPPORT_TX_CONTROL
	fh->flags.tx.ac = txi->hw_queue;
#endif

	if (ieee80211_is_data(fc)) {
		hif->subtype = HIF_FRAME_SUB_DATA_BE;
	} else if (ieee80211_is_mgmt(fc)) {
		hif->subtype = HIF_FRAME_SUB_MGMT;
		fh->flags.tx.ac = (hif->vifindex == 0 ? 3 : 9);
	} else if (ieee80211_is_ctl(fc)) {
		hif->subtype = HIF_FRAME_SUB_CTRL;
		fh->flags.tx.ac = (hif->vifindex == 0 ? 3 : 9);
	} else {
		WARN_ON(true);
	}

	credit = DIV_ROUND_UP(skb->len, nw->fwinfo.buffer_size);
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "%s ac:%d, pend:%d, credit:%d\n", __func__,
			fh->flags.tx.ac,
			atomic_read(&nw->tx_pend[fh->flags.tx.ac]), credit);
#endif
	atomic_add(credit, &nw->tx_pend[fh->flags.tx.ac]);
	ret = nrc_hif_enqueue_skb(nw, skb);
	if (ret < 0)
		nrc_hif_free_skb(nw, skb);

	return ret;
}

/**
 * nrc_xmit_frame - transmit a 802.11 frame
 */
int nrc_xmit_frame(struct nrc *nw,
		   struct ieee80211_vif *vif,
		   struct ieee80211_sta *sta,
		   struct sk_buff *skb)
{
	struct ieee80211_hdr *hdr = (void *)skb->data;
	struct frame_hdr *fh;
	struct hif *hif;
	__le16 fc = hdr->frame_control;
	struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
	struct ieee80211_key_conf *key = txi->control.hw_key;
	int extra_len, ret = 0;
	int credit;

#if defined(CONFIG_SUPPORT_KEY_RESERVE_TAILROOM)
	int crypto_tail_len = 0;

	if ((key && (key->flags & IEEE80211_KEY_FLAG_RESERVE_TAILROOM))
			&& (nw->cap.cap_mask & WIM_SYSTEM_CAP_HWSEC_OFFL)) {
		switch (key->cipher) {
		case WLAN_CIPHER_SUITE_WEP40:
		case WLAN_CIPHER_SUITE_WEP104:
			crypto_tail_len = IEEE80211_WEP_ICV_LEN;
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			crypto_tail_len = IEEE80211_TKIP_ICV_LEN;
			break;
		case WLAN_CIPHER_SUITE_CCMP:
			crypto_tail_len = IEEE80211_CCMP_MIC_LEN;
			break;
#ifdef CONFIG_SUPPORT_CCMP_256
		case WLAN_CIPHER_SUITE_CCMP_256:
			crypto_tail_len = IEEE80211_CCMP_256_MIC_LEN;
			break;
#endif
		default:
			nrc_dbg(NRC_DBG_MAC,
				"%s: Unknown cipher detected.(%d)"
					, __func__);
		}
		skb_put(skb, crypto_tail_len);
	}
#endif

	extra_len = nrc_skb_append_tx_info(nw, vif, sta, skb, false);

	/* Prepend a HIF and frame header */
	hif = (void *)skb_push(skb, nw->fwinfo.tx_head_size);
	memset(hif, 0, nw->fwinfo.tx_head_size);
	hif->type = HIF_TYPE_FRAME;
	hif->len = skb->len - sizeof(*hif);
	hif->vifindex = hw_vifindex(vif);

	/* Frame header */
	fh = (void *)(hif + 1);
	fh->info.tx.tlv_len = extra_len;
	fh->info.tx.cipher = WIM_CIPHER_TYPE_NONE;
#ifdef CONFIG_SUPPORT_TX_CONTROL
	fh->flags.tx.ac = txi->hw_queue;
#endif

	if (key) {
		if (!ieee80211_has_protected(fc))
			nrc_dbg(NRC_DBG_TX, "Key found but protected frame bit is 0.");
		fh->info.tx.cipher = nrc_to_wim_cipher_type(key->cipher);
		if (fh->info.tx.cipher == (uint8_t) WIM_CIPHER_TYPE_INVALID) {
			if (ieee80211_has_protected(fc)) {
				nrc_dbg(NRC_DBG_TX, "protected frame bit is 1 but invalid cipher type(%d).",
						 key->cipher);
				dev_kfree_skb(skb);
				return ret;
			}
			fh->info.tx.cipher = WIM_CIPHER_TYPE_NONE;
		}
	}

	/*
	 * TODO:
	 * - vif index
	 */
	if (ieee80211_is_data(fc)) {
		hif->subtype = HIF_FRAME_SUB_DATA_BE;
	} else if (ieee80211_is_mgmt(fc)) {
		hif->subtype = HIF_FRAME_SUB_MGMT;
		fh->flags.tx.ac = (hif->vifindex == 0 ? 3 : 9);
	} else if (ieee80211_is_ctl(fc)) {
		hif->subtype = HIF_FRAME_SUB_CTRL;
		fh->flags.tx.ac = (hif->vifindex == 0 ? 3 : 9);
	} else {
		WARN_ON(true);
	}

#if defined(CONFIG_NRC_HIF_PRINT_TX_DATA)
	print_hex_dump(KERN_DEBUG, "frame: ", DUMP_PREFIX_NONE, 16, 1,
		       skb->data, skb->len, false);
#endif
	credit = DIV_ROUND_UP(skb->len, nw->fwinfo.buffer_size);
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "%s ac:%d, pend:%d, credit:%d\n", __func__,
			fh->flags.tx.ac,
			atomic_read(&nw->tx_pend[fh->flags.tx.ac]), credit);
#endif
	atomic_add(credit, &nw->tx_pend[fh->flags.tx.ac]);

	if (!nw->loopback)
		ret = nrc_hif_enqueue_skb(nw, skb);
	else
		nrc_hif_free_skb(nw, skb);

	if (ret < 0)
		nrc_hif_free_skb(nw, skb);

	return ret;
}

static int lb_cnt;
int nrc_hif_rx(struct nrc_hif_device *dev, const u8 *data, const u32 len)
{
	struct nrc *nw = to_nw(dev);
	struct sk_buff *skb;
	struct hif *hif = (void *)data;

	WARN_ON(len < sizeof(struct hif));

	if (nw->loopback) {
		int i;
		uint8_t *ptr;

		nrc_dbg(NRC_DBG_HIF, "[%s] loopback receive success.\n",
				__func__);
		lb_cnt++;
		for (i = 0; i < len; i++)
			nrc_dbg(NRC_DBG_HIF, "%02X ", *(data + i));

		skb = dev_alloc_skb(len);
		if (!skb)
			return 0;

		ptr = (uint8_t *)skb_put(skb, len);
		memcpy(ptr, data, len);
		nrc_hif_debug_send(nw, skb);

		return 0;
	}

	skb = dev_alloc_skb(hif->len);
	memcpy(skb_put(skb, hif->len), data + sizeof(*hif), hif->len);

	switch (hif->type) {
	case HIF_TYPE_FRAME:
		nrc_mac_rx(nw, skb);
		break;
	case HIF_TYPE_WIM:
		nrc_wim_rx(nw, skb, hif->subtype);
		break;
	default:
		BUG();
	}
	return 0;
}

void nrc_hif_debug_send(struct nrc *nw, struct sk_buff *skb)
{
	int ret = 0;

	ret = nrc_hif_enqueue_skb(nw, skb);
	if (ret < 0)
		nrc_hif_free_skb(nw, skb);
}

int nrc_hif_debug_rx(void)
{
	nrc_dbg(NRC_DBG_HIF, "Disabled HIF Loopback mode.(%d)\n", lb_cnt);
	return lb_cnt;
}

static int hif_receive_skb(struct nrc_hif_device *dev, struct sk_buff *skb)
{
	struct nrc *nw = to_nw(dev);
	struct hif *hif = (void *)skb->data;

	WARN_ON(skb->len != hif->len + sizeof(*hif));

	if (nw->drv_state < NRC_DRV_START) {
		dev_kfree_skb(skb);
		return -EIO;
	}
	skb_pull(skb, sizeof(*hif));

	nrc_recovery_wdt_kick(nw);

	switch (hif->type) {
	case HIF_TYPE_FRAME:
		/*nrc_handle_frame(nw, skb);*/
		nrc_mac_rx(nw, skb);
		break;
	case HIF_TYPE_WIM:
		nrc_wim_rx(nw, skb, hif->subtype);
		break;
	case HIF_TYPE_LOG:
		nrc_netlink_rx(nw, skb, hif->subtype);
		break;
	case HIF_TYPE_DUMP:
		nrc_dbg(NRC_DBG_HIF, "HIF_TYPE_DUMP - lenth: %d", hif->len);
		/* print_hex_dump(KERN_DEBUG, "core dump ", DUMP_PREFIX_NONE,
		 *		16, 1, skb->data, hif->len, false);
		 */
		nrc_dump_store((char *)skb->data, hif->len);
		break;
	default:
		print_hex_dump(KERN_DEBUG, "hif type err ", DUMP_PREFIX_NONE,
				16, 1, skb->data, skb->len, false);
		BUG();
	}
	return 0;
}

int nrc_hif_wakeup_device(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->wakeup) {
		dev->hif_ops->wakeup(dev);
		return 0;
	}

	return -1;
}

int nrc_hif_reset_device(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->reset) {
		dev->hif_ops->reset(dev);
		return 0;
	}

	return -1;
}
int nrc_hif_test_status(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->test)
		dev->hif_ops->test(dev);
	return 0;
}

void nrc_hif_sync_lock(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->sync_lock && !dev->hif_ops->sync_auto)
		dev->hif_ops->sync_lock(dev);
}

void nrc_hif_sync_unlock(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->sync_unlock && !dev->hif_ops->sync_auto)
		dev->hif_ops->sync_unlock(dev);
}

/**
 * nrc_hif_cleanup - cleanup skb in hdev queue
 *
 * This function is called from nrc_hif_exit()
 */

void nrc_hif_cleanup(struct nrc_hif_device *dev)
{
	struct nrc *nw;
	struct sk_buff *skb;
	int i;

	nw = to_nw(dev);

	for (i = ARRAY_SIZE(dev->queue)-1; i >= 0; i--) {
		for (;;) {
			skb = skb_dequeue(&dev->queue[i]);
			if (!skb)
				break;
			dev_kfree_skb(skb);
		}
	}
}

int nrc_hif_close(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->close)
		dev->hif_ops->close(dev);
	return 0;
}

int nrc_hif_exit(struct nrc_hif_device *dev)
{
	struct nrc *nw;

	if (dev == NULL)
		return 0;

	nw = to_nw(dev);

	BUG_ON(dev == NULL);
	BUG_ON(nw == NULL);

	if (nw->workqueue != NULL)
		flush_work(&dev->work);

	nrc_hif_stop(dev);
	nrc_hif_cleanup(dev);

#ifdef CONFIG_NRC_HIF_SPI
	return nrc_hif_spi_exit(dev);
#elif defined(CONFIG_NRC_HIF_SSP)
	return nrc_hif_ssp_exit(dev);
#elif defined(CONFIG_NRC_HIF_UART)
	return nrc_hif_uart_exit(dev);
#elif defined(CONFIG_NRC_HIF_DEBUG)
	return nrc_hif_debug_exit(dev);
#elif defined(CONFIG_NRC_HIF_CSPI)
	return nrc_hif_cspi_exit(dev);
#elif defined(CONFIG_NRC_HIF_SDIO)
	return nrc_hif_sdio_exit(dev);
#else
	return 0;
#endif
}
