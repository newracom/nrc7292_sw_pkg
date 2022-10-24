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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include "nrc.h"
#include "nrc-hif.h"
#include "nrc-debug.h"
#include "nrc-stats.h"

//#define TRACE printk("%s %d\n", __func__, __LINE__)

struct moving_average {
	int size;
	int count;
	int min;
	int max;
	int index;
	int (*compute)(void *arr_t, int index, int count);
	uint8_t arr[];
};

static spinlock_t state_lock;
static struct list_head state_head;

static struct moving_average *
moving_average_init(int size, int count,
			   int (*compute)(void *arr, int index, int count))
{
	struct moving_average *ma;

	ma = kmalloc(sizeof(*ma) + size*count, GFP_KERNEL);
	if (!ma)
		return NULL;

	memset(ma, 0, sizeof(*ma) + size*count);

	ma->size = size;
	ma->count = count;
	ma->compute = compute;

	return ma;
}

static void moving_average_deinit(struct moving_average *handle)
{
	BUG_ON(!handle);
	kfree(handle);
}

static void moving_average_update(struct moving_average *ma, void *arg)
{
	int index, size;
	uint8_t *p;

	BUG_ON(!ma);

	index = ma->index;
	size = ma->size;
	p = ma->arr;

	BUG_ON(!p);

	p += index*size;
	memcpy(p, arg, size);

	index++;
	ma->index = index % ma->count;
}

static int moving_average_compute(struct moving_average *ma)
{
	BUG_ON(!ma);
	BUG_ON(!ma->compute);
	return ma->compute(ma->arr, ma->index, ma->count);
}

static struct moving_average *snr_h;
static struct moving_average *nrc_stats_snr_get(void)
{
	return snr_h;
}

static int snr_compute(void *arr_t, int index, int count)
{
	int i;
	uint8_t min = U8_MAX;
	uint8_t max = 0;
	int sum = 0;
	uint8_t *arr = arr_t;

	BUG_ON(!arr);

	for (i = 0; i < count; i++) {
		uint8_t snr = arr[i];

		min = min(snr, min);
		max = max(snr, max);
		sum += snr;
	}

	sum -= min;
	sum -= max;

	BUG_ON(count == 2);
	return sum/(count-2);
}

int nrc_stats_snr_init(int count)
{
	struct moving_average *h;

	h = moving_average_init(sizeof(uint8_t),
			count, snr_compute);
	if (!h)
		return -1;
	snr_h = h;

	return 0;
}

void nrc_stats_snr_deinit(void)
{
	struct moving_average *h = nrc_stats_snr_get();

	moving_average_deinit(h);
}

void nrc_stats_snr_update(uint8_t snr)
{
	struct moving_average *h = nrc_stats_snr_get();

	moving_average_update(h, &snr);
}

int nrc_stats_snr(void)
{
	struct moving_average *h = nrc_stats_snr_get();
	int snr = 1234567890;

	if (h)
		snr = moving_average_compute(h);

	return snr;
}

static struct moving_average *rssi_h;
static struct moving_average *nrc_stats_rssi_get(void)
{
	return rssi_h;
}

static int rssi_compute(void *arr_t, int index, int count)
{
	int i;
	int8_t min = S8_MAX;
	int8_t max = S8_MIN;
	int sum = 0;
	int8_t *arr = arr_t;

	BUG_ON(!arr);

	for (i = 0; i < count; i++) {
		int8_t rssi = (int8_t) arr[i];

		min = min(rssi, min);
		max = max(rssi, max);
		sum += rssi;
	}

	sum -= min;
	sum -= max;

	BUG_ON(count == 2);
	return sum/(count-2);
}

int nrc_stats_rssi_init(int count)
{
	struct moving_average *h;

	h = moving_average_init(sizeof(int8_t), count, rssi_compute);
	if (!h)
		return -1;
	rssi_h = h;

	return 0;
}

void nrc_stats_rssi_deinit(void)
{
	struct moving_average *h = nrc_stats_rssi_get();

	moving_average_deinit(h);
}

void nrc_stats_rssi_update(int8_t rssi)
{
	struct moving_average *h = nrc_stats_rssi_get();

	moving_average_update(h, &rssi);
}

int nrc_stats_rssi(void)
{
	struct moving_average *h = nrc_stats_rssi_get();
	int rssi = 1234567890;

	if (h)
		rssi = moving_average_compute(h);

	return rssi;
}

static int metric_compute(void *arr_t, int index, int count)
{
	int i;
	uint16_t min = U16_MAX;
	uint16_t max = 0;
	int sum = 0;
	uint16_t *arr = arr_t;

	BUG_ON(!arr);

	for (i = 0; i < count; i++) {
		uint16_t metric = arr[i];

		min = min(metric, min);
		max = max(metric, max);
		sum += metric;
	}

	sum -= min;
	sum -= max;

	BUG_ON(count == 2);
	return sum/(count-2);
}

int nrc_stats_metric(uint8_t *macaddr)
{
	struct stats_sta *cur, *next;
	uint16_t metric = 0;

	spin_lock(&state_lock);
	list_for_each_entry_safe(cur, next, &state_head, list) {
		if (memcmp(cur->macaddr, macaddr, 6) == 0) {
			metric = moving_average_compute(cur->metric);
			break;
		}
	}
	spin_unlock(&state_lock);

	return metric;
}

static struct moving_average *nrc_stats_rssi_init2(void)
{
	const int count = 16;

	return moving_average_init(sizeof(int8_t), count, rssi_compute);
}

static struct moving_average *nrc_stats_snr_init2(void)
{
	const int count = 16;

	return moving_average_init(sizeof(uint8_t), count, snr_compute);
}

static struct moving_average *nrc_stats_metric_init(void)
{
	const int count = 4;

	return moving_average_init(sizeof(uint16_t), count, metric_compute);
}

static void nrc_stats_rssi_update2(struct moving_average *h, int8_t rssi)
{
	moving_average_update(h, &rssi);
}

static void nrc_stats_snr_update2(struct moving_average *h, uint8_t snr)
{
	moving_average_update(h, &snr);
}

#define TX_OFFSET (100)
#define PATH_LOSS_CONSTANT (1)
static void nrc_stats_metric_update(struct moving_average *h, int8_t rssi)
{
	uint16_t metric;

	/* simplified rssi conversion formula */
	metric = (TX_OFFSET + rssi) * 100 / PATH_LOSS_CONSTANT;
	moving_average_update(h, &metric);
}

int nrc_stats_init(void)
{
	spin_lock_init(&state_lock);
	INIT_LIST_HEAD(&state_head);

	return 0;
}

void nrc_stats_deinit(void)
{
	struct stats_sta *cur, *next;

	spin_lock(&state_lock);
	list_for_each_entry_safe(cur, next, &state_head, list) {
		nrc_stats_dbg("[deinit] %pM\n", cur->macaddr);
		list_del(&cur->list);
		kfree(cur->rssi);
		kfree(cur->snr);
		kfree(cur->metric);
		kfree(cur);
	}
	spin_unlock(&state_lock);
}

int nrc_stats_update(uint8_t *macaddr, int8_t snr, int8_t rssi)
{
	struct stats_sta *cur, *next;

	spin_lock(&state_lock);
	list_for_each_entry_safe(cur, next, &state_head, list) {
		if (memcmp(cur->macaddr, macaddr, 6) == 0) {
			nrc_stats_rssi_update2(cur->rssi, rssi);
			nrc_stats_snr_update2(cur->snr, snr);
			nrc_stats_metric_update(cur->metric, rssi);
			spin_unlock(&state_lock);
			return 0;
		}
	}
	spin_unlock(&state_lock);

	return 0;
}

int nrc_stats_add(uint8_t *macaddr, int count)
{
	struct stats_sta *sta;
	struct stats_sta *cur, *next;

	spin_lock(&state_lock);
	list_for_each_entry_safe(cur, next, &state_head, list) {
		if (memcmp(cur->macaddr, macaddr, 6) == 0) {
			nrc_stats_dbg("[exist] %pM\n", cur->macaddr);
			spin_unlock(&state_lock);
			return 0;
		}
	}
	spin_unlock(&state_lock);

	sta = kmalloc(sizeof(*sta), GFP_KERNEL);
	if (!sta)
		return -1;

	INIT_LIST_HEAD(&sta->list);

	spin_lock(&state_lock);
	list_add_tail(&sta->list, &state_head);

	ether_addr_copy(sta->macaddr, macaddr);

	sta->rssi = nrc_stats_rssi_init2();
	sta->snr = nrc_stats_snr_init2();
	sta->metric = nrc_stats_metric_init();

	nrc_stats_dbg("[add] %pM\n", macaddr);
	spin_unlock(&state_lock);

	return 0;
}

int nrc_stats_del(uint8_t *macaddr)
{
	struct stats_sta *cur, *next;

	spin_lock(&state_lock);
	list_for_each_entry_safe(cur, next, &state_head, list) {
		if (memcmp(cur->macaddr, macaddr, 6) == 0) {
			nrc_mac_dbg("[remove] %pM\n", cur->macaddr);
			list_del(&cur->list);
			kfree(cur->rssi);
			kfree(cur->snr);
			kfree(cur->metric);
			kfree(cur);
		}
	}
	spin_unlock(&state_lock);

	return 0;
}

void nrc_stats_print(void)
{
	struct stats_sta *cur, *next;
	int i = 0;

	spin_lock(&state_lock);
	list_for_each_entry_safe(cur, next, &state_head, list) {
		nrc_stats_dbg("[%d] %pM snr:%d, rssi:%d\n",
			i++,
			cur->macaddr,
			moving_average_compute(cur->snr),
			moving_average_compute(cur->rssi));
	}
	spin_unlock(&state_lock);
}

int nrc_stats_report(uint8_t *output, int index, int number)
{
	struct stats_sta *cur, *next;
	int i = 0;
	int start = index;
	int count = 0;

	if (!output)
		return -1;

	if (!signal_monitor) {
		nrc_dbg(NRC_DBG_CAPI, "%s Failure. Signal Monitor is disabled.", __func__);
		return -1;
	}

	spin_lock(&state_lock);
	list_for_each_entry_safe(cur, next, &state_head, list) {
		if (i >= start) {
			if (count > 0)
				sprintf((output+strlen(output)), ",");
			sprintf((output+strlen(output)), "%pM,%d,%d",
				cur->macaddr,
				moving_average_compute(cur->rssi),
				moving_average_compute(cur->snr));
			count++;
			if (count == number)
				break;
		}
		i++;
	}

	if (count > 0)
		sprintf((output+strlen(output)), "\n");

	spin_unlock(&state_lock);
	return 0;
}

int nrc_stats_report_count(void)
{
	struct stats_sta *cur, *next;
	int i = 0;

	spin_lock(&state_lock);
	list_for_each_entry_safe(cur, next, &state_head, list) {
		i++;
	}
	spin_unlock(&state_lock);
	return i;
}
