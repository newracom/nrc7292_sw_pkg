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

#ifndef _NRC_DEBUG_H_
#define _NRC_DEBUG_H_

#include <net/mac80211.h>
#include "nrc.h"

enum NRC_DEBUG_MASK {
	NRC_DBG_HIF		= 0,
	NRC_DBG_WIM		= 1,
	NRC_DBG_TX		= 2,
	NRC_DBG_RX		= 3,
	NRC_DBG_MAC		= 4,
	NRC_DBG_CAPI	= 5,
	NRC_DBG_PS		= 6,
	NRC_DBG_STATS	= 7,
	NRC_DBG_STATE	= 8,
};
#define NRC_DBG_MASK_ANY   (0xFFFFFFFF)

#define DEFAULT_NRC_DBG_MASK_ALL (NRC_DBG_MASK_ANY)
#define DEFAULT_NRC_DBG_MASK (BIT(NRC_DBG_PS) | BIT(NRC_DBG_STATE))

#define NRC_DBG_PRINT_FRAME 0 /* print trx frames for debug */

enum LOOPBACK_MODE {
	LOOPBACK_MODE_ROUNDTRIP,
	LOOPBACK_MODE_TX_ONLY,
	LOOPBACK_MODE_RX_ONLY,
	LOOPBACK_MODE_MAX
};

struct lb_time_info {
	int _i;
	s64 _txt;
	s64 _rxt;
};

extern unsigned long nrc_debug_mask;
extern s64 tx_time_first;
extern s64 tx_time_last;
extern s64 rcv_time_first;
extern s64 rcv_time_last;
extern u32 arv_time_first;
extern u32 arv_time_last;
extern struct lb_time_info *time_info_array;
extern u32 lb_hexdump;

void nrc_dbg_init(struct nrc *nw);
void nrc_dbg_enable(enum NRC_DEBUG_MASK mk);
void nrc_dbg_disable(enum NRC_DEBUG_MASK mk);

void nrc_dbg(enum NRC_DEBUG_MASK mk, const char *fmt, ...);

void nrc_mac_dump_frame(struct nrc *nw, struct sk_buff *skb,
			const char *prefix);

void nrc_dump_wim(struct sk_buff *skb);

void nrc_init_debugfs(struct nrc *nw);
void nrc_exit_debugfs(void);

#define nrc_mac_dbg(fmt, ...) nrc_dbg(NRC_DBG_MAC, fmt, ##__VA_ARGS__)
#define nrc_ps_dbg(fmt, ...) nrc_dbg(NRC_DBG_PS, fmt, ##__VA_ARGS__)
#define nrc_stats_dbg(fmt, ...) nrc_dbg(NRC_DBG_STATS, fmt, ##__VA_ARGS__)

#endif
