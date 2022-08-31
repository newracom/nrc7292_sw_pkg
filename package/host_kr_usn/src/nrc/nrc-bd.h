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

#ifndef _NRC_BD_H_
#define _NRC_BD_H_

#define NRC_BD_MAX_CH_LIST		45

#define BD_DEBUG	0

struct BDF {
	uint8_t	ver_major;
	uint8_t	ver_minor;
	uint16_t total_len;
	
	uint16_t num_data_groups;
	uint16_t reserved[4];
	uint16_t checksum_data;
	
	uint8_t data[0];
};

struct bd_ch_table {
	uint16_t    s1g_freq;
	uint16_t    nons1g_freq;
	uint8_t     s1g_freq_index;
	uint16_t    nons1g_freq_index;
};

struct bd_supp_param {
	uint8_t num_ch;
	uint8_t s1g_ch_index[NRC_BD_MAX_CH_LIST];
	uint16_t nons1g_ch_freq[NRC_BD_MAX_CH_LIST];
};

#if defined(CONFIG_SUPPORT_BD)
extern struct bd_supp_param g_supp_ch_list;
extern bool g_bd_valid;

struct wim_bd_param * nrc_read_bd_tx_pwr(struct nrc *nw, uint8_t *cc);
int nrc_check_bd(void);
#endif /* defined(CONFIG_SUPPORT_BD) */

#endif //_NRC_BD_H_
