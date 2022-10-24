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
#include <linux/seq_file.h>
#include <linux/math64.h>
#include "nrc-debug.h"
#include "nrc-hif.h"
#include "nrc-wim-types.h"
#include "nrc-stats.h"
#include "wim.h"
#include "nrc-hif-cspi.h"

unsigned long nrc_debug_mask;
static struct nrc *nrc_nw;

void nrc_dbg_init(struct nrc *nw)
{
	if (debug_level_all) {
		nrc_debug_mask = DEFAULT_NRC_DBG_MASK_ALL;
	} else {
		nrc_debug_mask = DEFAULT_NRC_DBG_MASK;
	}
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

#if defined(DEBUG)
static struct dentry *hspi_debugfs_root;
#endif
static struct sk_buff *g_skb;
static u32 g_skb_len;
static u32 bunch = 0;
static u8 lb_subtype;
static u32 lb_count = 1;
u32 lb_hexdump = 0;
s64 tx_time_first;
s64 tx_time_last;
s64 rcv_time_first;
s64 rcv_time_last;
u32 arv_time_first;
u32 arv_time_last;
struct lb_time_info *time_info_array = NULL;

#define TX_SLOT_SIZE	456
#define RX_SLOT_SIZE	492

static int nrc_debugfs_hexdump_read(void *data, u64 *val)
{
	*val = (u64)lb_hexdump;
	return 0;
}

static int nrc_debugfs_hexdump_write(void *data, u64 val)
{
	lb_hexdump = (u32)val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_hexdump,
			nrc_debugfs_hexdump_read,
			nrc_debugfs_hexdump_write,
			"%llu\n");

static int nrc_debugfs_loopback_read(void *data, u64 *val)
{
	*val = (u64)lb_subtype;
	return 0;
}

static int nrc_debugfs_loopback_write(void *data, u64 val)
{
	struct nrc *nw = (struct nrc *)data;
	struct nrc_hif_device *hdev = nw->hif;
	struct sk_buff *skb;
	struct hif_lb_hdr *hif;
	u32 c = 0;

	lb_subtype = (u8)val;
	if (lb_subtype >= LOOPBACK_MODE_MAX) {
		lb_subtype = LOOPBACK_MODE_ROUNDTRIP;
	}

	if (!g_skb)
		return 0;

	if (time_info_array) {
		kfree(time_info_array);
		time_info_array = NULL;
	}

	time_info_array = (struct lb_time_info*)kzalloc(sizeof(struct lb_time_info) * lb_count, GFP_KERNEL);
	if (!time_info_array) {
		pr_err("Failed to alloc buffers...\n");
		return 0;
	}
	hif = (struct hif_lb_hdr *)g_skb->data;

	if (lb_subtype < LOOPBACK_MODE_RX_ONLY) {
		while(c < lb_count) {
			skb = skb_copy(g_skb, GFP_ATOMIC);
			hif = (struct hif_lb_hdr *)skb->data;
			hif->index = c++;
			hif->subtype = lb_subtype;
			if (c == 1 && lb_hexdump) {
				print_hex_dump(KERN_DEBUG, "HIF ", DUMP_PREFIX_NONE,
					16, 1, skb->data, sizeof(struct hif_lb_hdr), false);
				print_hex_dump(KERN_DEBUG, "ORIGIN ", DUMP_PREFIX_NONE,
					16, 1, skb->data + sizeof(struct hif_lb_hdr), skb->len - sizeof(struct hif_lb_hdr), false);
			}
			skb_queue_tail(&hdev->queue[(hif->type) % 2], skb);
			if (nw->workqueue == NULL) {
				return -1;
			}
			if (c < 5) {
				queue_work(nw->workqueue, &hdev->work);
			}
		}
	} else if (lb_subtype == LOOPBACK_MODE_RX_ONLY) {
		skb = skb_copy(g_skb, GFP_ATOMIC);
		/*
		 * In rx only mode, it's not necessary to send all slots.
		 */
		skb_trim(skb, 400);
		hif = (struct hif_lb_hdr *)skb->data;
		hif->subtype = lb_subtype;
		if (c == 0 && lb_hexdump) {
			print_hex_dump(KERN_DEBUG, "HIF ", DUMP_PREFIX_NONE,
				16, 1, skb->data, sizeof(struct hif_lb_hdr), false);
			//print_hex_dump(KERN_DEBUG, "ORIGIN ", DUMP_PREFIX_NONE,
			//	16, 1, skb->data + sizeof(struct hif_lb_hdr), skb->len - sizeof(struct hif_lb_hdr), false);
		}
		skb_queue_tail(&hdev->queue[(hif->type) % 2], skb);
		if (nw->workqueue == NULL) {
			return -1;
		}
		queue_work(nw->workqueue, &hdev->work);
	}
	nrc_hif_test_status(hdev);
	pr_err("[Loopback Test] TX Done.\n");

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_loopback,
			nrc_debugfs_loopback_read,
			nrc_debugfs_loopback_write,
			"%llu\n");

static int nrc_debugfs_lb_count_read(void *data, u64 *val)
{
	*val = (u64)lb_count;
	return 0;
}

static int nrc_debugfs_lb_count_write(void *data, u64 val)
{
	lb_count = (u32)val;
	if (g_skb) {
		struct hif_lb_hdr *h = (struct hif_lb_hdr*)g_skb->data;
		h->count = lb_count;
	}
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_lb_count,
			nrc_debugfs_lb_count_read,
			nrc_debugfs_lb_count_write,
			"%llu\n");

static int nrc_debugfs_hspi_sample_skb(struct seq_file *s, void *data)
{
	int i = 0;
	u8 *p;

	if (!g_skb || g_skb_len == 0) {
		seq_puts(s, "the sample data is not created yet.\n");
		return 0;
	}

	p = (void*)g_skb->data + sizeof(struct hif_lb_hdr);
	seq_printf(s, "\n* Total length: %d\n", g_skb_len);
	seq_puts(s, "----------------------------------------------------");
	while(i < g_skb_len) {
		if (i % 16 == 0) {
			seq_printf(s, "\n%03d | ", i / 16 + 1);
		}
		seq_printf(s, "%02x ", *(p + i++));
	}
	seq_puts(s, "\n\n");

	return 0;
}

static ssize_t nrc_debugfs_hspi_sample_write(struct file *file, const char __user *ubuf,
						size_t count, loff_t *ppos)
{
	long ret;
	struct hif_lb_hdr *hif;
	u8 *p;
	int i;

	ret = kstrtol_from_user(ubuf, count, 10, (long int*)&g_skb_len);
	if (ret)
		return ret;
	ret = count;

	if (g_skb && g_skb_len > 0 && (g_skb->len != g_skb_len)) {
		dev_kfree_skb(g_skb);
	}

	/*
	 * Maximum length is 1600 bytes.
	 */
	if (g_skb_len > 1600) {
		g_skb_len = 1600;
	}

	if (g_skb_len) {
		g_skb = dev_alloc_skb(g_skb_len + sizeof(struct hif_lb_hdr));
		if (!g_skb)
			return ret;
	} else {
		pr_err("[Loopback Teat] the length of sample data must be bigger than %d.\n", g_skb_len);
		return ret;
	}

	p = (u8 *)skb_put(g_skb, (g_skb_len + sizeof(struct hif)));
	for (i = 0; i < g_skb_len; i++) {
		*(p + sizeof(struct hif_lb_hdr) + i) = i;
	}

	hif = (struct hif_lb_hdr *)p;
	hif->type = HIF_TYPE_LOOPBACK;
	hif->count = lb_count;
	hif->subtype = 0;
	/*
	 * hif->len: sample size
	 * skb->len: sample size + sizeof(struct hif)
	 */
	hif->len = g_skb_len;

	return ret;
}

static int nrc_debugfs_hspi_sample_open(struct inode *inode, struct file *file)
{
	return single_open(file, nrc_debugfs_hspi_sample_skb, NULL);
}

static const struct file_operations hspi_sample_ops = {
	.open		= nrc_debugfs_hspi_sample_open,
	.read		= seq_read,
	.write		= nrc_debugfs_hspi_sample_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static ssize_t nrc_debugfs_hspi_report_write(struct file *file, const char __user *ubuf,
						size_t count, loff_t *ppos)
{
	long ret;

	ret = kstrtol_from_user(ubuf, count, 10, (long int*)&bunch);
	if (ret)
		return ret;
	ret = count;

	return ret;
}

#define TIMESTAMP_SUB(x, y)	((x) - (y))
#define RESET_VARS()	(c = sum_tx = sum_rx = 0)
#define LB_INFO(x)		((time_info_array + i)->_##x)
#define LB_INFO_(x)		((time_info_array + i - 1)->_##x)
#define LB_SUB(x)		((unsigned long)TIMESTAMP_SUB(LB_INFO(x), LB_INFO_(x)))

static int nrc_debugfs_hspi_report(struct seq_file *s, void *data)
{
	unsigned int i, slots, tx, rx, c, rl;
	unsigned long diff;
	unsigned long long t;
	unsigned long long sum_tx, sum_rx;
	u8 *str_type[] = {"Round-trip", "TX only", "RX only"};

	seq_printf(s, "########## SUMMARY (%s) ##########\n", str_type[lb_subtype]);
	slots = DIV_ROUND_UP(g_skb_len, TX_SLOT_SIZE);
	seq_printf(s, "1. Total frame counts: %d\n", lb_count);
	seq_printf(s, "2. Frame length: %d bytes (%d slots)\n", g_skb_len, slots);

	if (lb_subtype == LOOPBACK_MODE_ROUNDTRIP || lb_subtype >= LOOPBACK_MODE_MAX) {
		rl = (g_skb_len > RX_SLOT_SIZE) ? g_skb_len : RX_SLOT_SIZE;
		seq_printf(s, "   => Actual tx bytes: %d, Actual rx bytes: %d\n\n", slots * TX_SLOT_SIZE, rl);
		seq_printf(s, "3. Total tx bytes (HOST -> TARGET): %d bytes\n", tx = lb_count * slots * TX_SLOT_SIZE);
		seq_printf(s, "4. Total rx bytes (TARGET -> HOST): %d bytes\n", rx = lb_count * rl);
		seq_printf(s, "   => Total transferred bytes (No.3 + No.4): %llu bytes\n\n", t = (unsigned long long)(tx + rx));
		seq_printf(s, "5. First frame transmit time: %llu us\n", tx_time_first);
		seq_printf(s, "6. Last frame transmit time: %llu us\n", tx_time_last);
		seq_printf(s, "   (diff: %lu us)\n", (unsigned long)TIMESTAMP_SUB(tx_time_last, tx_time_first));
		seq_printf(s, "7. First frame received time: %llu us\n", rcv_time_first);
		seq_printf(s, "8. Last frame received time: %llu us\n", rcv_time_last);
		seq_printf(s, "   (diff: %lu us)\n", (unsigned long)TIMESTAMP_SUB(rcv_time_last, rcv_time_first));
		seq_printf(s, "   --------------------------------------------\n");
		seq_printf(s, "   => First frame RTT (No.7 - No.5) : %lu us\n", (unsigned long)TIMESTAMP_SUB(rcv_time_first, tx_time_first));
		seq_printf(s, "   => Last frame RTT (No.8 - No.6) : %lu us\n", (unsigned long)TIMESTAMP_SUB(rcv_time_last, tx_time_last));
		seq_printf(s, "   => Time diff (No.8 - No.5) : %lu us\n", diff = (unsigned long)TIMESTAMP_SUB(rcv_time_last, tx_time_first));
		t *= 7812; // 8(bit) / 1024(kbit) * 1000000(sec) = 7812.5
		seq_printf(s, "   => Throughput: %llu kbps\n", div_u64(t, diff));
	} else if (lb_subtype == LOOPBACK_MODE_TX_ONLY) {
		seq_printf(s, "   => Actual tx bytes: %d\n\n", slots * TX_SLOT_SIZE);
		seq_printf(s, "3. Total tx bytes (HOST -> TARGET): %lld bytes\n\n", t = (lb_count - 1) * slots * TX_SLOT_SIZE);
		seq_printf(s, "4. First frame transmit time: %llu us\n", tx_time_first);
		seq_printf(s, "5. Last frame transmit time: %llu us\n", tx_time_last);
		seq_printf(s, "   (diff: %lu us)\n", (unsigned long)TIMESTAMP_SUB(tx_time_last, tx_time_first));
		seq_printf(s, "6. First frame arrival time(TSF in target): %u us\n", arv_time_first);
		seq_printf(s, "7. Last frame arrival time(TSF in target): %u us\n", arv_time_last);
		seq_printf(s, "   (diff: %lu us)\n", diff = (arv_time_last - arv_time_first));
		seq_printf(s, "   --------------------------------------------\n");
		t *= 7812; // 8(bit) / 1024(kbit) * 1000000(sec) = 7812.5
		seq_printf(s, "   => Throughput: %llu kbps\n", div_u64(t, diff));
	} else if (lb_subtype == LOOPBACK_MODE_RX_ONLY) {
		rl = (g_skb_len > RX_SLOT_SIZE) ? g_skb_len : RX_SLOT_SIZE;
		seq_printf(s, "   => Actual rx bytes: %d\n\n", rl);
		seq_printf(s, "3. Total rx bytes (TARGET -> HOST): %lld bytes\n\n", t = (lb_count - 1) * rl);
		seq_printf(s, "7. First frame received time: %llu us\n", rcv_time_first);
		seq_printf(s, "8. Last frame received time: %llu us\n", rcv_time_last);
		seq_printf(s, "   (diff: %lu us)\n", diff = (unsigned long)TIMESTAMP_SUB(rcv_time_last, rcv_time_first));
		seq_printf(s, "   --------------------------------------------\n");
		t *= 7812; // 8(bit) / 1024(kbit) * 1000000(sec) = 7812.5
		seq_printf(s, "   => Throughput: %llu kbps\n", div_u64(t, diff));
	}
	if (bunch > 0) {
		if (time_info_array) {
			int x = lb_count < 50 ? lb_count : 50;
			seq_printf(s, "\n########## DETAIL ##########\n");
			seq_printf(s, "[frame index] [tx time(us)] [time diff with previous(us)] [rx time(us)] [time diff with previous(us)]\n");
			for (i = 0; i < x; i++) {
				if (LB_INFO(i) == i) {
					seq_printf(s, "[%5d] \t%lld \t%lld \t%lld \t%lld\n", i,
						LB_INFO(txt), (i==0)?0LL:LB_SUB(txt), LB_INFO(rxt), (i==0)?0LL:LB_SUB(rxt));
				} else {
					seq_printf(s, "[----] this frame might be lost.\n");
				}
			}
			if (lb_count > 50) {
				RESET_VARS();
				for (i = 50; i < lb_count; i++) {
					if (c < bunch) {
						sum_tx += LB_SUB(txt);
						sum_rx += LB_SUB(rxt);
						++c;
					}
					if (c == bunch) {
						if (bunch == 1) {
							if (LB_INFO(i) == i) {
								seq_printf(s, "[%5d] \t%lld \t%ld \t%lld \t%ld\n", i,
									LB_INFO(txt), LB_SUB(txt), LB_INFO(rxt), LB_SUB(rxt));
							} else {
								seq_printf(s, "[----] this frame might be lost.\n");
							}
						} else {
							seq_printf(s, "[%d - %d] \t%llu \t%llu\n", i + 1 - bunch, i, div_u64(sum_tx, bunch), div_u64(sum_rx, bunch));
						}
						RESET_VARS();
					}
				}
				if (c != 0) {
					seq_printf(s, "[%d - %d] \t%llu \t%llu\n", i - c, i, div_u64(sum_tx, c), div_u64(sum_rx, c));
				}
			}
		}
	}

	return 0;
}

static int nrc_debugfs_hspi_report_open(struct inode *inode, struct file *file)
{
	return single_open(file, nrc_debugfs_hspi_report, NULL);
}

static const struct file_operations hspi_report_ops = {
	.open		= nrc_debugfs_hspi_report_open,
	.read		= seq_read,
	.write		= nrc_debugfs_hspi_report_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

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
	//nrc_debugfs_create_file("pm", &nrc_debugfs_pm);

#if defined(DEBUG)
	hspi_debugfs_root = debugfs_create_dir("hspi", nw->debugfs);
	debugfs_create_file("report", 0600, hspi_debugfs_root, nw, &hspi_report_ops);
	debugfs_create_file("sample", 0600, hspi_debugfs_root, nw, &hspi_sample_ops);
	debugfs_create_file("test", 0600, hspi_debugfs_root, nw, &nrc_debugfs_loopback);
	debugfs_create_file("count", 0600, hspi_debugfs_root, nw, &nrc_debugfs_lb_count);
	debugfs_create_file("hexdump", 0600, hspi_debugfs_root, nw, &nrc_debugfs_hexdump);
#endif
}

void nrc_exit_debugfs(void)
{
	if (time_info_array) {
		kfree(time_info_array);
	}
}
