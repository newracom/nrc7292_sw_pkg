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
#ifndef _NRC_INIT_H_
#define _NRC_INIT_H_

#include "nrc-hif.h"

#define NRC_DRIVER_NAME "nrc80211"

struct nrc_test_ops;
struct nrc_uart_priv;
#if 0
#include "nrc_monitor_rx.h"

struct nrc_monitor {
	struct nrc_priv *priv;
	struct wireless_dev *wdev;
	struct nrc_monitor_rx_priv rx_priv;
};
#endif

struct nrc *nrc_nw_alloc (struct device *dev, struct nrc_hif_device *hdev);
void nrc_nw_free(struct nrc *nw);
int nrc_nw_start (struct nrc *nw);
int nrc_nw_stop (struct nrc *nw);
int nrc_nw_set_model_conf (struct nrc *nw, u16 chip_id);
int country_match(const char *const cc[], const char *const country);
#endif
