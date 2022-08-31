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
#include <linux/stat.h>
#include <asm/uaccess.h>
#include "nrc.h"
#include "nrc-bd.h"
#include "nrc-debug.h"
#include "nrc-wim-types.h"

#if defined(CONFIG_SUPPORT_BD)

#define NRC_BD_FILE_MAX_LENGTH	4096
#define NRC_BD_MAX_DATA_LENGTH	546
#define NRC_BD_HEADER_LENGTH	16
char g_bd_buf[NRC_BD_FILE_MAX_LENGTH] = {0, };
int g_bd_size=0;

enum {
	CC_US=1,
	CC_JP,
	CC_KR,
	CC_TW,
	CC_EU,
	CC_CN,
	CC_NZ,
	CC_AU,
	CC_MAX,
}CC_TYPE;

const struct bd_ch_table *g_bd_ch_table_base;
struct bd_supp_param g_supp_ch_list;

static const struct bd_ch_table g_bd_ch_table[CC_MAX][NRC_BD_MAX_CH_LIST] = {
	{
		/* US */
		{9025,	2412,	1,	1},
		{9035,	2422,	3,	3},
		{9045,	2432,	5,	5},
		{9055,	2442,	7,	7},
		{9065,	2452,	9,	9},
		{9075,	2462,	11,	11},
		{9085,	5180,	13,	36},
		{9095,	5185,	15,	37},
		{9105,	5190,	17,	38},
		{9115,	5195,	19,	39},
		{9125,	5200,	21,	40},
		{9135,	5205,	23,	41},
		{9145,	5210,	25,	42},
		{9155,	5215,	27,	43},
		{9165,	5220,	29,	44},
		{9175,	5225,	31,	45},
		{9185,	5230,	33,	46},
		{9195,	5235,	35,	47},
		{9205,	5240,	37,	48},
		{9215,	5745,	39,	149},
		{9225,	5750,	41,	150},
		{9235,	5755,	43,	151},
		{9245,	5760,	45,	152},
		{9255,	5500,	47,	100},
		{9265,	5520,	49,	104},
		{9275,	5540,	51,	108},
		{9030,	2417,	2,	2},
		{9050,	2437,	6,	6},
		{9070,	2457,	10,	10},
		{9090,	5765,	14,	153},
		{9110,	5770,	18,	154},
		{9130,	5775,	22,	155},
		{9150,	5780,	26,	156},
		{9170,	5785,	30,	157},
		{9190,	5790,	34,	158},
		{9210,	5795,	38,	159},
		{9230,	5800,	42,	160},
		{9250,	5805,	46,	161},
		{9270,	5560,	50,	112},
		{9060,	2447,	8,	8},
		{9100,	5810,	16,	162},
		{9140,	5815,	24,	163},
		{9180,	5820,	32,	164},
		{9220,	5825,	40,	165},
		{9260,	5580,	48,	116}
	},
	{
		/* New Japan */
		{9210,  5200,   9,   40},
		{9230,  5210,   13,  42},
		{9240,  5215,   15,  43},
		{9250,  5220,   17,  44},
		{9260,  5225,   19,  45},
		{9270,  5230,   21,  46},
		{9235,  5180,   2,   36},
		{9245,  5185,   4,   37},
		{9255,  5190,   6,   38},
		{9265,  5195,   8,   39},
		{9245,  5235,   36,  47},
		{9255,  5240,   38,  48}
	},
	{
		// Korea MIC Band (925MH~931MHz)
		{9255,  5180,   1,      36},
		{9265,  5185,   3,      37},
		{9275,  5190,   5,      38},
		{9285,  5195,   7,      39},
		{9295,  5200,   9,      40},
		{9305,  5205,   11,     41},
		{9280,  5210,   4,      42},
		{9300,  5215,   8,      43}
	},
	{
		/*  Taiwan */
		{8390,  5180,    1, 36},
		{8400,  5185,    3, 37},
		{8410,  5190,    5, 38},
		{8420,  5195,    7, 39},
		{8430,  5200,    9, 40},
		{8440,  5205,   11, 41},
		{8450,  5210,   13, 42},
		{8460,  5215,   15, 43},
		{8470,  5220,   17, 44},
		{8480,  5225,   19, 45},
		{8490,  5230,   21, 46},
		{8500,  5235,   23, 47},
		{8510,  5240,   25, 48},
		{8395,  5745,    2, 149},
		{8415,  5750,    6, 150},
		{8435,  5755,   10, 151},
		{8455,  5760,   14, 152},
		{8475,  5765,   18, 153},
		{8495,  5770,   22, 154},
		{8405,  5775,    4, 155},
		{8445,  5780,   12, 156},
		{8485,  5785,   20, 157}
	},
	{
		/* EU */
		{8635,  5180,   1, 36},
		{8645,  5185,   3, 37},
		{8655,  5190,   5, 38},
		{8665,  5195,   7, 39},
		{8675,  5200,   9, 40},
		{8640,  5205,   2, 41},
		{8660,  5210,   6, 42},
	},
	{
		/* CN */
		{7555,	5180,	1,	36},
		{7565,	5185,	3,	37},
		{7575,	5190,	5,	38},
		{7585,	5195,	7,	39},
		{7595,	5200,	9,	40},
		{7605,	5205,	11,	41},
		{7615,	5210,	13,	42},
		{7625,	5215,	15,	43},
		{7635,	5220,	17,	44},
		{7645,	5225,	19,	45},
		{7655,	5230,	21,	46},
		{7665,	5235,	23,	47},
		{7675,	5240,	25,	48},
		{7685,	5745,	27,	149},
		{7695,	5750,	29,	150},
		{7705,	5755,	31,	151},
		{7795,	5760,	16,	152},
		{7805,	5765,	18,	153},
		{7815,	5770,	20,	154},
		{7825,	5775,	22,	155},
		{7835,	5780,	24,	156},
		{7845,	5785,	26,	157},
		{7855,	5790,	28,	158},
		{7865,	5795,	30,	159},
		{7800,	5800,	2,	160},
		{7820,	5805,	6,	161},
		{7840,	5810,	10,	162},
		{7860,	5815,	14,	163},
		{7810,	5820,	4,	164},
		{7850,	5825,	12,	165}
	},
	{
		/* NZ */
		{9155,	5180,	27, 36},
		{9165,	5185,	29, 37},
		{9175,	5190,	31, 38},
		{9185,	5195,	33, 39},
		{9195,	5200,	35, 40},
		{9205,	5205,	37, 41},
		{9215,	5210,	39, 42},
		{9225,	5215,	41, 43},
		{9235,	5220,	43, 44},
		{9245,	5225,	45, 45},
		{9255,	5230,	47, 46},
		{9265,	5235,	49, 47},
		{9275,	5240,	51, 48},
		{9170,	5765,	30, 153},
		{9190,	5770,	34, 154},
		{9210,	5775,	38, 155},
		{9230,	5780,	42, 156},
		{9250,	5785,	46, 157},
		{9270,	5790,	50, 158},
		{9180,	5810,	32, 162},
		{9220,	5815,	40, 163},
		{9260,	5820,	48, 164}
	},
	{
		/* AU */
		{9155,	5180,	27, 36},
		{9165,	5185,	29, 37},
		{9175,	5190,	31, 38},
		{9185,	5195,	33, 39},
		{9195,	5200,	35, 40},
		{9205,	5205,	37, 41},
		{9215,	5210,	39, 42},
		{9225,	5215,	41, 43},
		{9235,	5220,	43, 44},
		{9245,	5225,	45, 45},
		{9255,	5230,	47, 46},
		{9265,	5235,	49, 47},
		{9275,	5240,	51, 48},
		{9170,	5765,	30, 153},
		{9190,	5770,	34, 154},
		{9210,	5775,	38, 155},
		{9230,	5780,	42, 156},
		{9250,	5785,	46, 157},
		{9270,	5790,	50, 158},
		{9180,	5810,	32, 162},
		{9220,	5815,	40, 163},
		{9260,	5820,	48, 164}
	},
};

static const struct bd_ch_table g_bd_ch_table_usn[CC_MAX][NRC_BD_MAX_CH_LIST] = {
	{
		/* US */
		{9025,	2412,	1,	1},
		{9035,	2422,	3,	3},
		{9045,	2432,	5,	5},
		{9055,	2442,	7,	7},
		{9065,	2452,	9,	9},
		{9075,	2462,	11,	11},
		{9085,	5180,	13,	36},
		{9095,	5185,	15,	37},
		{9105,	5190,	17,	38},
		{9115,	5195,	19,	39},
		{9125,	5200,	21,	40},
		{9135,	5205,	23,	41},
		{9145,	5210,	25,	42},
		{9155,	5215,	27,	43},
		{9165,	5220,	29,	44},
		{9175,	5225,	31,	45},
		{9185,	5230,	33,	46},
		{9195,	5235,	35,	47},
		{9205,	5240,	37,	48},
		{9215,	5745,	39,	149},
		{9225,	5750,	41,	150},
		{9235,	5755,	43,	151},
		{9245,	5760,	45,	152},
		{9255,	5500,	47,	100},
		{9265,	5520,	49,	104},
		{9275,	5540,	51,	108},
		{9030,	2417,	2,	2},
		{9050,	2437,	6,	6},
		{9070,	2457,	10,	10},
		{9090,	5765,	14,	153},
		{9110,	5770,	18,	154},
		{9130,	5775,	22,	155},
		{9150,	5780,	26,	156},
		{9170,	5785,	30,	157},
		{9190,	5790,	34,	158},
		{9210,	5795,	38,	159},
		{9230,	5800,	42,	160},
		{9250,	5805,	46,	161},
		{9270,	5560,	50,	112},
		{9060,	2447,	8,	8},
		{9100,	5810,	16,	162},
		{9140,	5815,	24,	163},
		{9180,	5820,	32,	164},
		{9220,	5825,	40,	165},
		{9260,	5580,	48,	116}
	},
	{
		/* New Japan */
		{9210,  5200,   9,   40},
		{9230,  5210,   13,  42},
		{9240,  5215,   15,  43},
		{9250,  5220,   17,  44},
		{9260,  5225,   19,  45},
		{9270,  5230,   21,  46},
		{9235,  5180,   2,   36},
		{9245,  5185,   4,   37},
		{9255,  5190,   6,   38},
		{9265,  5195,   8,   39},
		{9245,  5235,   36,  47},
		{9255,  5240,   38,  48}
	},
	{
		// Korea USN band (921MH~923MH)
		{9215,  5220,   1,      44},
		{9225,  5225,   3,      45}
	},
	{
		/*  Taiwan */
		{8390,  5180,    1, 36},
		{8400,  5185,    3, 37},
		{8410,  5190,    5, 38},
		{8420,  5195,    7, 39},
		{8430,  5200,    9, 40},
		{8440,  5205,   11, 41},
		{8450,  5210,   13, 42},
		{8460,  5215,   15, 43},
		{8470,  5220,   17, 44},
		{8480,  5225,   19, 45},
		{8490,  5230,   21, 46},
		{8500,  5235,   23, 47},
		{8510,  5240,   25, 48},
		{8395,  5745,    2, 149},
		{8415,  5750,    6, 150},
		{8435,  5755,   10, 151},
		{8455,  5760,   14, 152},
		{8475,  5765,   18, 153},
		{8495,  5770,   22, 154},
		{8405,  5775,    4, 155},
		{8445,  5780,   12, 156},
		{8485,  5785,   20, 157}
	},
	{
		/* EU */
		{8635,  5180,   1, 36},
		{8645,  5185,   3, 37},
		{8655,  5190,   5, 38},
		{8665,  5195,   7, 39},
		{8675,  5200,   9, 40},
		{8640,  5205,   2, 41},
		{8660,  5210,   6, 42},
	},
	{
		/* CN */
		{7555,	5180,	1,	36},
		{7565,	5185,	3,	37},
		{7575,	5190,	5,	38},
		{7585,	5195,	7,	39},
		{7595,	5200,	9,	40},
		{7605,	5205,	11,	41},
		{7615,	5210,	13,	42},
		{7625,	5215,	15,	43},
		{7635,	5220,	17,	44},
		{7645,	5225,	19,	45},
		{7655,	5230,	21,	46},
		{7665,	5235,	23,	47},
		{7675,	5240,	25,	48},
		{7685,	5745,	27,	149},
		{7695,	5750,	29,	150},
		{7705,	5755,	31,	151},
		{7795,	5760,	16,	152},
		{7805,	5765,	18,	153},
		{7815,	5770,	20,	154},
		{7825,	5775,	22,	155},
		{7835,	5780,	24,	156},
		{7845,	5785,	26,	157},
		{7855,	5790,	28,	158},
		{7865,	5795,	30,	159},
		{7800,	5800,	2,	160},
		{7820,	5805,	6,	161},
		{7840,	5810,	10,	162},
		{7860,	5815,	14,	163},
		{7810,	5820,	4,	164},
		{7850,	5825,	12,	165}
	},
	{
		/* NZ */
		{9155,	5180,	27, 36},
		{9165,	5185,	29, 37},
		{9175,	5190,	31, 38},
		{9185,	5195,	33, 39},
		{9195,	5200,	35, 40},
		{9205,	5205,	37, 41},
		{9215,	5210,	39, 42},
		{9225,	5215,	41, 43},
		{9235,	5220,	43, 44},
		{9245,	5225,	45, 45},
		{9255,	5230,	47, 46},
		{9265,	5235,	49, 47},
		{9275,	5240,	51, 48},
		{9170,	5765,	30, 153},
		{9190,	5770,	34, 154},
		{9210,	5775,	38, 155},
		{9230,	5780,	42, 156},
		{9250,	5785,	46, 157},
		{9270,	5790,	50, 158},
		{9180,	5810,	32, 162},
		{9220,	5815,	40, 163},
		{9260,	5820,	48, 164}
	},
	{
		/* AU */
		{9155,	5180,	27, 36},
		{9165,	5185,	29, 37},
		{9175,	5190,	31, 38},
		{9185,	5195,	33, 39},
		{9195,	5200,	35, 40},
		{9205,	5205,	37, 41},
		{9215,	5210,	39, 42},
		{9225,	5215,	41, 43},
		{9235,	5220,	43, 44},
		{9245,	5225,	45, 45},
		{9255,	5230,	47, 46},
		{9265,	5235,	49, 47},
		{9275,	5240,	51, 48},
		{9170,	5765,	30, 153},
		{9190,	5770,	34, 154},
		{9210,	5775,	38, 155},
		{9230,	5780,	42, 156},
		{9250,	5785,	46, 157},
		{9270,	5790,	50, 158},
		{9180,	5810,	32, 162},
		{9220,	5815,	40, 163},
		{9260,	5820,	48, 164}
	},
};

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

static void * nrc_dump_load(int len)
{
	struct file *filp;
	loff_t pos=0;
	mm_segment_t old_fs;
	char filepath[64];
	char *buf = NULL;
#if BD_DEBUG
	int i;
#endif

	sprintf(filepath, "/lib/firmware/%s", bd_name);
#if KERNEL_VERSION(5,0,0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs( get_ds() );
#elif KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs( KERNEL_DS );
#else
	old_fs = force_uaccess_begin();
#endif

	filp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("Failed to load board data, error:%d",IS_ERR(filp));
#if KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	set_fs(old_fs);
#else
	force_uaccess_end(old_fs);
#endif
		return NULL;
	}

	buf = (char *) kmalloc(len, GFP_KERNEL);
	if (!buf) {
		pr_err("malloc input buf error!\n");
		return NULL;
	}

#if KERNEL_VERSION(4, 14, 0) <= NRC_TARGET_KERNEL_VERSION
	kernel_read(filp, buf, len, &pos);
#else
	kernel_read(filp, pos, buf, len);
#endif

	filp_close(filp, NULL);
#if KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	set_fs(old_fs);
#else
	force_uaccess_end(old_fs);
#endif

#if BD_DEBUG
	for(i=0; i < len;) {
		nrc_dbg(NRC_DBG_STATE,"%02X %02X %02X %02X %02X %02X %02X %02X",
				buf[i + 0],
				buf[i + 1],
				buf[i + 2],
				buf[i + 3],
				buf[i + 4],
				buf[i + 5],
				buf[i + 6],
				buf[i + 7]);
		i += 8;
	}
#endif

	return buf;
}

uint16_t nrc_get_non_s1g_freq(uint8_t cc_index, uint8_t s1g_ch_index)
{
	int i;
	uint16_t ret = 0;

	if (enable_usn) {
		g_bd_ch_table_base = &g_bd_ch_table_usn[cc_index - 1][0];
	} else {
		g_bd_ch_table_base = &g_bd_ch_table[cc_index - 1][0];
	}
	for(i=0; i < NRC_BD_MAX_CH_LIST; i++) {
		if(s1g_ch_index == g_bd_ch_table_base[i].s1g_freq_index) {
			ret = g_bd_ch_table_base[i].nons1g_freq;
			break;
		}
	}
	return ret;
}

bool nrc_set_supp_ch_list(struct wim_bd_param *bd)
{
	int i, j;
	bool ret = false;
	int length = (int)bd->length - 4;
	uint8_t *pos = bd->value;
	uint8_t cc_idx = bd->type;
	uint8_t s1g_ch_idx = 0;

	memset(&g_supp_ch_list, 0, sizeof(struct bd_supp_param));

	if(!(*pos))
		return ret;
	else
		ret = true;

	for(i=0; i < NRC_BD_MAX_CH_LIST; i++) {
		if((*pos) && (length > 0)) {
			g_supp_ch_list.num_ch++;
			g_supp_ch_list.s1g_ch_index[i] = *pos;
			length -= 12;
			pos += 12;
		} else {
			break;
		}
	}

	for(j=0; j < g_supp_ch_list.num_ch; j++) {
		s1g_ch_idx = g_supp_ch_list.s1g_ch_index[j];
		g_supp_ch_list.nons1g_ch_freq[j] = 
			nrc_get_non_s1g_freq(cc_idx, s1g_ch_idx);
	}

#if BD_DEBUG
	nrc_dbg(NRC_DBG_STATE,"Supported Channel(%u) Index",g_supp_ch_list.num_ch);
	for(i=0; i < g_supp_ch_list.num_ch; i++) {
		nrc_dbg(NRC_DBG_STATE,"%u %u",
				g_supp_ch_list.s1g_ch_index[i],
				g_supp_ch_list.nons1g_ch_freq[i]);
	}
#endif

	return ret;
}

struct wim_bd_param * nrc_read_bd_tx_pwr(struct nrc *nw, uint8_t *country_code)
{
	uint8_t cc_index = CC_US;
	uint16_t len = 0;
	uint8_t type = 0;
	int i, j;
	struct BDF *bd;
	uint8_t cc[2] = {0,0};
	struct wim_bd_param *bd_sel;
	bool check_bd_flag=false;
	uint16_t target_version;

	if(!g_bd_size)
		return NULL;
	else
		nrc_dbg(NRC_DBG_STATE,"size of bd file is %d",g_bd_size);

	cc[0] = *country_code;
	cc[1] = *(country_code + 1);

	if(cc[0] == 'U' && cc[1] == 'S')
		cc_index = CC_US;
	else if(cc[0] == 'J' && cc[1] == 'P')
		cc_index = CC_JP;
	else if(cc[0] == 'K' && cc[1] == 'R')
		cc_index = CC_KR;
	else if(cc[0] == 'T' && cc[1] == 'W')
		cc_index = CC_TW;
	else if(cc[0] == 'D' && cc[1] == 'E')
		cc_index = CC_EU;
	else if(cc[0] == 'C' && cc[1] == 'N')
		cc_index = CC_CN;
	else if(cc[0] == 'N' && cc[1] == 'Z')
		cc_index = CC_NZ;
	else if(cc[0] == 'A' && cc[1] == 'U')
		cc_index = CC_AU;
	else {
		nrc_dbg(NRC_DBG_STATE,
				"[ERR] Invalid country code(%c%c). Set default value(%d)",
				cc[0], cc[1], cc_index);
		return NULL;
	}

	bd = (struct BDF *)nrc_dump_load(g_bd_size);
	if(!bd) {
		nrc_dbg(NRC_DBG_STATE,"bd is NULL");
		return NULL;
	}

	nrc_dbg(NRC_DBG_STATE, "Major %02X Minor %02X Total len %04X Num_Data_Groups %04X Checksum %04X",
			bd->ver_major, bd->ver_minor, bd->total_len, bd->num_data_groups, bd->checksum_data);

#if BD_DEBUG
	for(i=0; i < bd->total_len;) {
		nrc_dbg(NRC_DBG_STATE,"%02X %02X %02X %02X %02X %02X %02X %02X",
				bd->data[i + 0],
				bd->data[i + 1],
				bd->data[i + 2],
				bd->data[i + 3],
				bd->data[i + 4],
				bd->data[i + 5],
				bd->data[i + 6],
				bd->data[i + 7]);
		i += 8;
	}
#endif

	bd_sel = kmalloc(sizeof(*bd_sel), GFP_KERNEL);
	if (!bd_sel) {
		nrc_dbg(NRC_DBG_STATE,"[ERROR] bd_sel is NULL");
		kfree(bd);
		return NULL;
	}
	memset(bd_sel, 0, sizeof(*bd_sel));

	//find target version from board data file and compare it with one getting from serial flash 
	target_version = nw->fwinfo.hw_version;

	// if a value of h/w version is invalid, then set it to 0xFFFF
	if(target_version > 0x7FF && target_version != 0xFFFF)
		target_version = 0xFFFF;

	for(i = 0; i < bd->num_data_groups; i++)
	{
		type = bd->data[len + 4*i];
		//nrc_dbg(NRC_DBG_STATE, "type : %u, cc_index: %u",type, cc_index);
		if(type == cc_index) {
			// copy data for specific country code
			//nrc_dbg(NRC_DBG_STATE, "cc_index is matched(%u : %u)",type, cc_index);
			bd_sel->type = (uint16_t)type;

			bd_sel->hw_version = (uint16_t)(bd->data[6 + len + 4*i] +
					(bd->data[7 + len + 4*i]<<8));

			// Add a condition if target version is initial value(65535)
			if((target_version == bd_sel->hw_version) || (target_version == 0xFFFF)) {
				nrc_dbg(NRC_DBG_STATE, "target version is matched(%u : %u)",
						target_version, bd_sel->hw_version);

				bd_sel->length = (uint16_t)(bd->data[2 + len + 4*i] +
						(bd->data[3 + len + 4*i]<<8));
				bd_sel->checksum = (uint16_t)(bd->data[4 + len + 4*i] +
						(bd->data[5 + len + 4*i]<<8));

				for(j=0; j < bd_sel->length -2; j++) {
					bd_sel->value[j] = bd->data[8 + len + 4*i + j];
				}
				check_bd_flag = true;
				nrc_dbg(NRC_DBG_STATE, "type %04X, len %04X, checksum %04X target_ver %04X",
						bd_sel->type, bd_sel->length, bd_sel->checksum, bd_sel->hw_version);
				break;
			} else {
				nrc_dbg(NRC_DBG_STATE, "target version is not matched(%u : %u)",
						target_version, bd_sel->hw_version);

				
			}
		}
		len += (uint16_t)(bd->data[2 + len + 4*i] +
				(bd->data[3 + len + 4*i]<<8));
	}

	kfree(bd);

	if(check_bd_flag && nrc_set_supp_ch_list(bd_sel)) {
		return bd_sel;
	} else {
		kfree(bd_sel);
		return NULL;
	}
}

int nrc_check_bd(void)
{
	struct BDF *bd;
	struct file *filp;
	loff_t pos=0;
#if KERNEL_VERSION(5, 10, 0) <= NRC_TARGET_KERNEL_VERSION
	int rc;
#endif

	struct kstat *stat;
	char *buf;
	size_t length;
	int ret;
	mm_segment_t old_fs;
	char filepath[64];

	sprintf(filepath, "/lib/firmware/%s", bd_name);
#if KERNEL_VERSION(5,0,0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs(get_ds());
#elif KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#else
	old_fs = force_uaccess_begin();
#endif

	filp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("Failed to load board data :error: %d",IS_ERR(filp));
#if KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
		set_fs(old_fs);
#else
		force_uaccess_end(old_fs);
#endif
		return -EIO;
	}

	stat = (struct kstat *) kmalloc(sizeof(struct kstat), GFP_KERNEL);
	if(!stat)
		return -ENOMEM;

#if KERNEL_VERSION(5, 10, 0) <= NRC_TARGET_KERNEL_VERSION
	rc = vfs_getattr(&filp->f_path, stat, STATX_SIZE, AT_STATX_SYNC_AS_STAT);
	if(rc != 0){
        printk("vfs_getattr Error");
    }
	length = (size_t)stat->size;
#else
	vfs_stat(filepath, stat);
	length = (size_t)stat->size;
#endif

	buf = (char *) kmalloc((int)length, GFP_KERNEL);
	if(!buf) {
		kfree(stat);
		pr_err("buf is NULL");
		return -ENOMEM;
	}

#if KERNEL_VERSION(4, 14, 0) <= NRC_TARGET_KERNEL_VERSION
	g_bd_size = kernel_read(filp, buf, (int)length, &pos);
#else
	g_bd_size = kernel_read(filp, pos, buf, (int)length);
#endif
	filp_close(filp, NULL);
#if KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	set_fs(old_fs);
#else
	force_uaccess_end(old_fs);
#endif
	kfree(stat);

	if(g_bd_size < NRC_BD_HEADER_LENGTH) {
		pr_err("Invalid data size(%d)", g_bd_size);
		kfree(buf);
		return -EINVAL;
	}

	bd = (struct BDF *)buf;

	if((bd->total_len > g_bd_size-NRC_BD_HEADER_LENGTH) || (bd->total_len < NRC_BD_HEADER_LENGTH)) {
		pr_err("Invalid total length(%d)", bd->total_len);
		kfree(buf);
		return -EINVAL;
	}

	ret = nrc_checksum_16(bd->total_len, (uint8_t *)&bd->data[0]);
	if(bd->checksum_data != ret) {
		pr_err("Invalid checksum(%u : %u)", bd->checksum_data, ret);
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);

	return 0;
}
#endif /* #if defined(CONFIG_SUPPORT_BD) */
