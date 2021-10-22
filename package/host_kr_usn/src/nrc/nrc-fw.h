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

#ifndef _NRC_FW_H_
#define _NRC_FW_H_

#define DEFAULT_FIRMWARE_NAME	"uni.bin"

#define CHUNK_SIZE	(1024) /*(2*1024)*/
#define FRAG_BYTES (CHUNK_SIZE - 16)

struct fw_frag_hdr {
	u32 eof;
	u32 address;
	u32 len;
} __packed;

struct fw_frag {
	u32 eof;
	u32 address;
	u32 len;
	u8 payload[FRAG_BYTES];
	u32 checksum;
} __packed;

struct nrc_fw_priv {
	struct nrc *nw;
	struct firmware *fw;
	struct fw_frag_hdr frag_hdr;
	int offset;
	/* 0 : inside first 256 chunks, > 0 : inside 256-aligned chunks */
	const u8 *fw_data_pos;
	int remain_bytes;
	int num_chunks;
	int cur_chunk; /* index of chunck to be transferred */

	bool fw_requested;
	u8 index;
	u32 index_fb;
	uint32_t start_addr;
	bool ack;
	bool csum;
};

extern char *fw_name;

struct nrc_fw_priv *nrc_fw_init(struct nrc *nw);
void nrc_fw_exit(struct nrc_fw_priv *priv);
bool nrc_check_fw_file(struct nrc *nw);
bool nrc_check_boot_ready(struct nrc *nw);
void nrc_set_boot_ready(struct nrc *nw);
void nrc_download_fw(struct nrc *nw);
void nrc_release_fw(struct nrc *nw);

#endif
