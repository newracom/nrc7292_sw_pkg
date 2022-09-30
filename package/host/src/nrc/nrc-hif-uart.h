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

#ifndef _NRC_HIF_UART_H_
#define _NRC_HIF_UART_H_

#ifdef CONFIG_NRC_HIF_UART
#include <asm/uaccess.h>
#include <linux/kthread.h>

struct file;
struct nrc_hif_device;

enum nrc_hif_rx_state {
	NRC_HIF_RX_SOH,
	NRC_HIF_RX_IDLE = NRC_HIF_RX_SOH,
	NRC_HIF_RX_HIF_HDR,
	NRC_HIF_RX_PAYLOAD,
	NRC_HIF_RX_EOT,
};

struct nrc_hif_uart {
	struct nrc_hif_device *dev;
	const char *name;
	unsigned int device_baud;
	struct file *fp;
	mm_segment_t fs_snapshot;
	struct task_struct *thread_rx;
	enum nrc_hif_rx_state rx_state;
	u8 rx_data[IEEE80211_MAX_DATA_LEN];
	u32 rx_data_pos;
	u32 rx_data_size;
	u8 ack_val;
	bool ack_received;
};

struct nrc_hif_device *nrc_hif_uart_init(void);
int nrc_hif_uart_exit(struct nrc_hif_device *dev);

#else

static inline struct nrc_hif_device *nrc_hif_uart_init(void)
{
	return NULL;
}

static inline int nrc_hif_uart_exit(struct nrc_hif_device *dev)
{
	return 0;
}

#endif

#endif
