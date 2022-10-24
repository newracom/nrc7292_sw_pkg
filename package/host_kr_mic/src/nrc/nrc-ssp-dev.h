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

#ifndef _NRC_SSP_DEV_H_
#define _NRC_SSP_DEV_H_

#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include "nrc-hif.h"

struct nrc_ssp;

struct nrc_ssp_priv {
	struct nrc_ssp *ssp;

	struct workqueue_struct *rx_wq;
	struct workqueue_struct *tx_wq;
	struct work_struct rx_work;
	struct work_struct tx_work;
	struct mutex bus_lock_mutex;

	struct sk_buff_head skb_head;
	atomic_t hif_req;
	int hif_isr_count;
	bool loopback;
	bool fw_ready;
	bool gdma_ready;
};

struct nrc_ssp {
	struct spi_device *spi;
	struct nrc_hif_ssp *hif;
	struct nrc_ssp_priv *priv;
	unsigned long prev_xfer_time;
};

#endif
