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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "nrc-bd.h"
#include "nrc.h"
#include "nrc-debug.h"
#include "nrc-wim-types.h"

#define NRC_BD_MAX_DATA_LENGTH	546
#define NRC_BD_HEADER_LENGTH	16
char g_bd_buf[4096] = {0, };

enum {
	CC_US=1,
	CC_JP,
	CC_KR,
	CC_TW,
	CC_EU,
	CC_CN,
	CC_MAX,
}CC_TYPE;

static uint16_t nrc_checksum_16(uint16_t len, uint8_t* buf)
{
	uint32_t checksum = 0;
	int i = 0;

	//len = Total num of bytes
	while(len > 0)
	{
		//get two bytes at a time and  add previous calculated checsum value
		checksum = ((buf[i]) + (buf[i+1]<<8)) + checksum;

		//decrease by 2 for 2 byte boundaries
		len -= 2;
		i += 2;
	}

	//Add the carryout
	checksum = (checksum>>16) + checksum;

	// if 1's complement
	//checksum = (unsigned int)~checksum;

	return checksum;
}

extern char *bd_name;
static void * nrc_dump_load(int len)
{
	struct file *filp;
	loff_t pos=0;
	size_t length = (size_t) len;
	int ret;
	mm_segment_t old_fs;
	int i;
	char filepath[64];

	sprintf(filepath, "/lib/firmware/%s", bd_name);
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("Failed to load borad data, error:%d",IS_ERR(filp));
		set_fs(old_fs);
		return NULL;
	}

#if KERNEL_VERSION(4, 14, 0) <= NRC_TARGET_KERNEL_VERSION
	ret = kernel_read(filp, g_bd_buf, length, &pos);
#else
	ret = kernel_read(filp, g_bd_buf, length, pos);
#endif

	filp_close(filp, NULL);
	set_fs(old_fs);

	if(ret < NRC_BD_HEADER_LENGTH) {
		pr_err("Invalid data size(%d)", ret);
		return NULL;
	}

	for(i=0; i < ret;) {
		nrc_dbg(NRC_DBG_STATE,"%02X %02X %02X %02X %02X %02X %02X %02X ",
							 g_bd_buf[i],
							 g_bd_buf[i+1],
							 g_bd_buf[i+2],
							 g_bd_buf[i+3],
							 g_bd_buf[i+4],
							 g_bd_buf[i+5],
							 g_bd_buf[i+6],
							 g_bd_buf[i+7]
							 );
		i += 8;
	}

#if 0 // for single country code
	if(ret >= NRC_BD_HEADER_LENGTH)
		ret -= NRC_BD_HEADER_LENGTH;
	if(ret != (int)(g_bd_buf[2] + (g_bd_buf[3]<<8))) {
		nrc_dbg(NRC_DBG_STATE, "Invalid length %u %u buf addr %p %p",
				(unsigned int)ret,
				(unsigned int)(g_bd_buf[2] + (g_bd_buf[3]<<8)),
				g_bd_buf, &g_bd_buf[0]);	
	}
#endif
	
	return &g_bd_buf[0];
}

struct wim_bd_param * nrc_read_bd_tx_pwr(uint8_t *country_code)
{
	uint8_t cc_index = CC_MAX;
	uint16_t ret = 0;
	uint16_t len = 0;
	uint8_t type = 0;
	int i, j;
	struct BDF *bd;
	uint8_t cc[2] = {0,0};
	struct wim_bd_param *bd_sel;

	bd_sel = kmalloc(sizeof(*bd_sel), GFP_KERNEL);
	if (!bd_sel) {
		nrc_dbg(NRC_DBG_STATE,"bd_sel is NULL");
		return NULL;
	}
	memset(bd_sel, 0, sizeof(*bd_sel));
	
	cc[0] = *country_code;
	cc[1] = *(country_code + 1);
	
	if(cc[0] == 'J' && cc[1] == 'P')
		cc_index = CC_JP;
	else if(cc[0] == 'K' && cc[1] == 'R')
		cc_index = CC_KR;
	else if(cc[0] == 'T' && cc[1] == 'W')
		cc_index = CC_TW;
	else if(cc[0] == 'U' && cc[1] == 'S')
		cc_index = CC_US;
	else if(cc[0] == 'D' && cc[1] == 'E')
		cc_index = CC_EU;
	else if(cc[0] == 'C' && cc[1] == 'N')
		cc_index = CC_CN;
	else {
		nrc_dbg(NRC_DBG_STATE,
			"Invalid country code(%c%c). Set default value(%d)",
			cc[0], cc[1], cc_index);
		kfree(bd_sel);
		return NULL;
	}
		
	bd = (struct BDF*)nrc_dump_load(NRC_BD_MAX_DATA_LENGTH + NRC_BD_HEADER_LENGTH);

	if(!bd) {
		nrc_dbg(NRC_DBG_STATE,"bd is NULL");
		kfree(bd_sel);
		return NULL;
	}

	nrc_dbg(NRC_DBG_STATE, "Major %02X Minor %02X Total len %04X Num_country %04X Checksum %04X",
		bd->ver_major, bd->ver_minor, bd->total_len, bd->num_country, bd->checksum_data);

	for(i=0; i < bd->total_len;) {
		nrc_dbg(NRC_DBG_STATE,"%02X %02X %02X %02X %02X %02X %02X %02X",
				bd->data[i],
				bd->data[i+1],
				bd->data[i+2],
				bd->data[i+3],
				bd->data[i+4],
				bd->data[i+5],
				bd->data[i+6],
				bd->data[i+7]
				);
		i += 8;
	}

	// checksum for all country code's data
	ret = nrc_checksum_16(bd->total_len, (uint8_t *)&bd->data[0]);

	if(ret != bd->checksum_data) {
		nrc_dbg(NRC_DBG_STATE, "Invalid checksum(%u : %u)",
			bd->checksum_data, ret);
		kfree(bd_sel);
		return NULL;
	}

	for(i = 0; i < bd->num_country; i++) {
		type = g_bd_buf[NRC_BD_HEADER_LENGTH + len + 4*i];
		if(type == cc_index) {
			// copy data for specific country code
			nrc_dbg(NRC_DBG_STATE, "cc_index is matched(%u : %u)",
				type, cc_index);
			bd_sel->type = (uint16_t)type;
			bd_sel->length = (uint16_t)(g_bd_buf[NRC_BD_HEADER_LENGTH + 2 + len + 4*i] +
					(g_bd_buf[NRC_BD_HEADER_LENGTH + 3 + len + 4*i]<<8));
			bd_sel->checksum = (uint16_t)(g_bd_buf[NRC_BD_HEADER_LENGTH + 4 + len + 4*i] +
					(g_bd_buf[NRC_BD_HEADER_LENGTH + 5 + len + 4*i]<<8));

			for(j=0; j < bd_sel->length - 2; j++) {
				bd_sel->value[j] = g_bd_buf[NRC_BD_HEADER_LENGTH + 6 + len + 4*i + j];
			}

			nrc_dbg(NRC_DBG_STATE, "type %04X, len %04X, checksum %04X",
				bd_sel->type, bd_sel->length, bd_sel->checksum);
			break;
		}
		len += (uint16_t)(g_bd_buf[NRC_BD_HEADER_LENGTH + 2 + len + 4*i] +
					(g_bd_buf[NRC_BD_HEADER_LENGTH + 3 + len + 4*i]<<8));
	}

	return bd_sel;
}
