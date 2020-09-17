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

#include <linux/platform_device.h>
#include "nrc-debug.h"
#include "nrc-hif.h"
#include "nrc-wim-types.h"
#include "nrc-stats.h"
#include "wim.h"

unsigned long nrc_debug_mask;
static struct nrc *nrc_nw;

void nrc_dbg_init(struct nrc *nw)
{
	nrc_debug_mask = DEFAULT_NRC_DBG_MASK;
	nrc_nw = nw;
}

void nrc_dbg_enable(enum NRC_DEBUG_MASK mk)
{
	set_bit(mk, &nrc_debug_mask);
}

void nrc_dbg_disable(enum NRC_DEBUG_MASK mk)
{
	clear_bit(mk, &nrc_debug_mask);
}

void nrc_dbg(enum NRC_DEBUG_MASK mk, const char *fmt, ...)
{
	va_list args;
	int i;
	static char buf[512] = {0,};

	if (WARN_ON(!nrc_nw))
		return;

	if (!test_bit(mk, &nrc_debug_mask))
		return;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	if (!nrc_nw || nrc_nw->loopback || !nrc_nw->pdev)
		pr_info("%s\n", buf);
	else
		dev_dbg(&nrc_nw->pdev->dev, "%s\n", buf);
}

/* TODO: steve
 *
 * Let's move to trace framework.
 */
void nrc_mac_dump_frame(struct nrc *nw, struct sk_buff *skb, const char *prefix)
{
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
	__le16 fc = hdr->frame_control;

	return;

	if (ieee80211_is_nullfunc(fc) || ieee80211_is_qos_nullfunc(fc)) {
		nrc_dbg(NRC_DBG_MAC, "%s %snullfunc (%c)",
			prefix,
			ieee80211_is_nullfunc(fc) ? "" : "qos",
			ieee80211_has_pm(fc) ? 'P' : '.');
	} else if (ieee80211_is_data(fc)) {
		nrc_dbg(NRC_DBG_MAC, "%s %sdata (%c%c%c%c)", prefix,
			is_multicast_ether_addr(hdr->addr1) ? "mc-" : "",
			(fc & cpu_to_le16(IEEE80211_FCTL_TODS)) ? 'T' : '.',
			(fc & cpu_to_le16(IEEE80211_FCTL_FROMDS)) ? 'F' : '.',
			(fc & cpu_to_le16(IEEE80211_FCTL_MOREDATA)) ? 'M' : '.',
			(fc & cpu_to_le16(IEEE80211_FCTL_PM)) ? 'P' : '.');
	} else if (ieee80211_is_probe_resp(fc)) {
		nrc_dbg(NRC_DBG_MAC, "%s probe-rsp", prefix);
	} else if (ieee80211_is_probe_req(fc)) {
		nrc_dbg(NRC_DBG_MAC, "%s probe-req", prefix);
	} else if (ieee80211_is_pspoll(fc)) {
		struct ieee80211_pspoll *pspoll;

		pspoll = (struct ieee80211_pspoll *)hdr;
		nrc_dbg(NRC_DBG_MAC, "%s ps-poll (aid=%d)", prefix,
			pspoll->aid & ~0xc000);
	} else if (ieee80211_is_beacon(fc)) {
		struct ieee80211_mgmt *mgmt = (void *)skb->data;
		const u8 *tim;
		u8 *start, *end;
		char *str = "";

		start = (u8 *)mgmt->u.beacon.variable;
		end = skb->data + skb->len;
		tim = cfg80211_find_ie(WLAN_EID_TIM, start, end - start);
		if (tim &&
		    ieee80211_check_tim((struct ieee80211_tim_ie *)&tim[2],
					tim[1], nw->aid))
			str = "(tim)";

		nrc_dbg(NRC_DBG_MAC, "%s beacon %s", prefix, str);
	}
}

static char *wim_cmd_str[] = {
	[WIM_CMD_INIT] = "cmd=init",
	[WIM_CMD_START] = "cmd=start",
	[WIM_CMD_STOP] = "cmd=stop",
	[WIM_CMD_SCAN_START] = "cmd=scan-start",
	[WIM_CMD_SCAN_STOP] = "cmd=scan-stop",
	[WIM_CMD_SET_KEY] = "cmd=set-key",
	[WIM_CMD_DISABLE_KEY] = "cmd=disable-key",
	[WIM_CMD_STA_CMD] = "cmd=sta-cmd",
	[WIM_CMD_SET] = "cmd=set",
	[WIM_CMD_REQ_FW] = "cmd=firmware",
	[WIM_CMD_FW_RELOAD] = "cmd=fw-reload",
	[WIM_CMD_AMPDU_ACTION] = "cmd=ampd-action",
	[WIM_CMD_SHELL] = "cmd=shell",
	[WIM_CMD_SLEEP] = "cmd=sleep",
};

static char *wim_event_str[] = {
	[WIM_EVENT_SCAN_COMPLETED] = "event=scan-complete",
	[WIM_EVENT_READY] = "event=ready",
	[WIM_EVENT_CREDIT_REPORT] = "event=credit",
	[WIM_EVENT_PS_READY] = "event=ps-ready",
};

void nrc_dump_wim(struct sk_buff *skb)
{
	struct hif *hif = (void *) skb->data;
	struct wim *wim = (void *) (hif + 1);
	u8 stype = hif->subtype;

	nrc_dbg(NRC_DBG_MAC, "wim: %s, vif=%d, seqno=%d, len=%d",
		stype == HIF_WIM_SUB_REQUEST ? wim_cmd_str[wim->cmd] :
		stype == HIF_WIM_SUB_EVENT ? wim_event_str[wim->event] :
		"resp",
		hif->vifindex, wim->seqno, hif->len);
}

/* Debugfs */

/* Debug message mask
 * - TODO: make it more human friendly
 */

static int nrc_debugfs_debug_read(void *data, u64 *val)
{
	*val = nrc_debug_mask;
	return 0;
}

static int nrc_debugfs_debug_write(void *data, u64 val)
{
	nrc_debug_mask = val;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_debug_fops,
			nrc_debugfs_debug_read,
			nrc_debugfs_debug_write,
			"%llu\n");

static int nrc_debugfs_cspi_status_read(void *data, u64 *val)
{
	struct nrc *nw = (struct nrc *)data;
	struct nrc_hif_device *hdev = nw->hif;

	*val = nrc_hif_test_status(hdev);
	return 0;
}

static int nrc_debugfs_cspi_status_write(void *data, u64 val)
{
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_cspi,
			nrc_debugfs_cspi_status_read,
			nrc_debugfs_cspi_status_write,
			"%llu\n");

static int nrc_debugfs_hif_read(void *data, u64 *val)
{
	*val = nrc_hif_debug_rx();
	return 0;
}

static int nrc_debugfs_hif_write(void *data, u64 val)
{
	struct nrc *nw = (struct nrc *)data;
	struct sk_buff *skb;
	struct hif *hif;
	uint8_t *ptr;
	int i = 0;

	skb = dev_alloc_skb(val + sizeof(struct hif));
	if (!skb)
		return 0;

	ptr = (uint8_t *)skb_put(skb, val);
	for (i = 0; i < val; i++)
		*ptr = i;

	skb_reserve(skb, sizeof(struct hif));
	hif = (struct hif *)skb_push(skb, sizeof(struct hif));
	hif->type = HIF_TYPE_FRAME;
	hif->subtype = 0;
	hif->len = val;

	nw->loopback = true;
	nrc_hif_debug_send(nw, skb);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_hif,
			nrc_debugfs_hif_read,
			nrc_debugfs_hif_write,
			"%llu\n");

static int nrc_debugfs_wakeup_device_read(void *data, u64 *val)
{
	*val = 0;
	return 0;
}

static int nrc_debugfs_wakeup_device_write(void *data, u64 val)
{
	struct nrc *nw = (struct nrc *)data;
	struct nrc_hif_device *hdev = nw->hif;

	nrc_hif_wakeup_device(hdev);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_wakeup_device,
			nrc_debugfs_wakeup_device_read,
			nrc_debugfs_wakeup_device_write,
			"%llu\n");


static int nrc_debugfs_reset_device_read(void *data, u64 *val)
{
	*val = 0;
	return 0;
}

static int nrc_debugfs_reset_device_write(void *data, u64 val)
{
	struct nrc *nw = (struct nrc *)data;
	struct nrc_hif_device *hdev = nw->hif;

	nrc_hif_reset_device(hdev);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_reset_device,
			nrc_debugfs_reset_device_read,
			nrc_debugfs_reset_device_write,
			"%llu\n");

static int nrc_debugfs_snr_read(void *data, u64 *val)
{
	*val = nrc_stats_snr();
	return 0;
}

static int nrc_debugfs_snr_write(void *data, u64 val)
{
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_snr,
			nrc_debugfs_snr_read,
			nrc_debugfs_snr_write,
			"%llu\n");

static int nrc_debugfs_rssi_read(void *data, u64 *val)
{
	*val = nrc_stats_rssi();
	return 0;
}

static int nrc_debugfs_rssi_write(void *data, u64 val)
{
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_rssi,
			nrc_debugfs_rssi_read,
			nrc_debugfs_rssi_write,
			"%lld\n");

void nrc_init_debugfs(struct nrc *nw)
{

#define nrc_debugfs_create_file(name, fops) \
	debugfs_create_file(name, 0664, nw->debugfs, nw, fops)

	nw->debugfs = nw->hw->wiphy->debugfsdir;

	nrc_debugfs_create_file("debug", &nrc_debugfs_debug_fops);
	nrc_debugfs_create_file("cspi", &nrc_debugfs_cspi);
	nrc_debugfs_create_file("hif", &nrc_debugfs_hif);
	nrc_debugfs_create_file("wakeup", &nrc_debugfs_wakeup_device);
	nrc_debugfs_create_file("reset", &nrc_debugfs_reset_device);
	nrc_debugfs_create_file("snr", &nrc_debugfs_snr);
	nrc_debugfs_create_file("rssi", &nrc_debugfs_rssi);
}
