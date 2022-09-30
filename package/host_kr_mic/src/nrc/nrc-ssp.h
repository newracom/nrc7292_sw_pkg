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

#ifndef _NRC_SSP_H_
#define _NRC_SSP_H_

#include "nrc-ssp-dev.h"

#define NR_HOST_IF_HEADER_SOH  0x4E
#define NR_HOST_IF_HEADER_VER  0x00

enum {
	SSP_V_SKIP  = -1,
	SSP_V_ERROR =  0,
	SSP_V_GOOD  =  1,
};

struct ssp_header_t {
	uint8_t m_soh;
	uint8_t m_ver;
	uint8_t m_type;
	uint8_t m_sub_type;
	uint16_t m_length;
	uint8_t m_padding;
	uint8_t m_csum;
};

int ssp_read_buffer(struct nrc_ssp *ssp);
bool ssp_write_buffer(struct nrc_ssp *ssp, uint8_t *buffer, int length,
		uint8_t type);
void ssp_make_header(uint8_t *buffer, uint8_t type, uint8_t sub_type,
		uint16_t length, uint8_t padding);
int nrc_ssp_register(struct nrc_hif_ssp *hif);
void nrc_ssp_unregister(struct nrc_hif_ssp *hif);
void ssp_show_header(struct ssp_header_t *header);
void ssp_show_hex(uint8_t *buf, int len);
bool ssp_enqueue_buffer(struct nrc_ssp *ssp, struct sk_buff *skb);
void set_timer(struct nrc_ssp_priv *priv);

#endif
