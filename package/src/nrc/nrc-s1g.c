#include "nrc-s1g.h"
#include "nrc-debug.h"
#if defined(CONFIG_S1G_CHANNEL)

static char s1g_alpha2[3] = "US";			//default country

enum {
	US = 0,
	JP,
	K0,
	K1,
	TW,
	EU,
	CN,
	NZ,
	AU,
	K2,
	MAX_COUNTRY_CODE
};

#define MAX_BD_COUNTRY_CODE (AU+1)


const struct s1g_channel_table *ptr_nrc_s1g_ch_table;

// Japan
static const struct s1g_channel_table s1g_ch_table_jp[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"JP",	9170,	1,	BW_1M,	1,	8,		5,	0},
	{"JP",	9180,	3,	BW_1M,	1,	8,		-5,	1},
	{"JP",	9190,	5,	BW_1M,	1,	8,		5,	0},
	{"JP",	9200,	7,	BW_1M,	1,	8,		-5,	1},
	{"JP",	9210,	9,	BW_1M,	1,	8,		5,	0},
	{"JP",	9220,	11,	BW_1M,	1,	8,		-5,	1},
	{"JP",	9230,	13,	BW_1M,	1,	8,		5,	0},
	{"JP",	9240,	15,	BW_1M,	1,	8,		-5,	1},
	{"JP",	9250,	17,	BW_1M,	1,	8,		5,	0},
	{"JP",	9260,	19,	BW_1M,	1,	8,		-5,	1},
	{"JP",	9270,	21,	BW_1M,	1,	8,		5,	0},
	{}
};

// Korea (K1) USN band (921MH~923MH)
static const struct s1g_channel_table s1g_ch_table_k1_usn[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"KR",	9215,	1,	BW_1M,	1,	14,		-5,	1},
	{"KR",	9225,	3,	BW_1M,	1,	14,		-5,	1},
	{}
};

// Korea (K2) MIC Band (925MH~931MHz)
static const struct s1g_channel_table s1g_ch_table_k2_mic[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"KR",	9255,	1,	BW_1M,	1,	14,		5,	0},
	{"KR",	9265,	3,	BW_1M,	1,	14,		-5,	1},
	{"KR",	9275,	5,	BW_1M,	1,	14,		5,	0},
	{"KR",	9285,	7,	BW_1M,	1,	14,		-5,	1},
	{"KR",	9295,	9,	BW_1M,	1,	14,		5,	0},
	{"KR",	9305,	11,	BW_1M,	1,	14,		-5,	1},
#if defined (S1G_INCLUDE_4M_OP_2M_TX)
	{"KR",	9280,	4,	BW_4M,	1,	15,		0,	1},
	{"KR",	9300,	8,	BW_4M,	1,	15,		0,	1},
#else
	{"KR",	9270,	4,	BW_4M,	1,	15,		0,	0},
	{"KR",	9290,	8,	BW_4M,	1,	15,		0,	0},
#endif
	{}
};

// Taiwan
static const struct s1g_channel_table s1g_ch_table_tw[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"TW",	8390,	1,	BW_1M,	2,	1,		5,	0},
	{"TW",	8400,	3,	BW_1M,	2,	1,		-5,	1},
	{"TW",	8410,	5,	BW_1M,	2,	1,		5,	0},
	{"TW",	8420,	7,	BW_1M,	2,	1,		-5,	1},
	{"TW",	8430,	9,	BW_1M,	2,	1,		5,	0},
	{"TW",	8440,	11,	BW_1M,	2,	1,		-5,	1},
	{"TW",	8450,	13,	BW_1M,	2,	1,		5,	0},
	{"TW",	8460,	15,	BW_1M,	2,	1,		-5,	1},
	{"TW",	8470,	17,	BW_1M,	2,	1,		5,	0},
	{"TW",	8480,	19,	BW_1M,	2,	1,		-5,	1},
	{"TW",	8490,	21,	BW_1M,	2,	1,		5,	0},
	{"TW",	8500,	23,	BW_1M,	2,	1,		-5,	1},
	{"TW",	8510,	25,	BW_1M,	2,	1,		-5,	1},
	{"TW",	8395,	2,	BW_2M,	2,	1,		0,	0},
	{"TW",	8415,	6,	BW_2M,	2,	1,		0,	0},
	{"TW",	8435,	10,	BW_2M,	2,	1,		0,	0},
	{"TW",	8455,	14,	BW_2M,	2,	1,		0,	0},
	{"TW",	8475,	18,	BW_2M,	2,	1,		0,	0},
	{"TW",	8495,	22,	BW_2M,	2,	1,		0,	0},
	{"TW",	8405,	4,	BW_4M,	2,	1,		0,	1},
	{"TW",	8445,	12,	BW_4M,	2,	1,		0,	1},
	{"TW",	8485,	20,	BW_4M,	2,	1,		0,	1},
	{}
};

// US
static const struct s1g_channel_table s1g_ch_table_us[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"US",	9025,	1,	BW_1M,	1,	1,		5,	0},
	{"US",	9035,	3,	BW_1M,	1,	1,		-5,	1},
	{"US",	9045,	5,	BW_1M,	2,	1,		5,	0},
	{"US",	9055,	7,	BW_1M,	2,	1,		-5,	1},
	{"US",	9065,	9,	BW_1M,	2,	1,		5,	0},
	{"US",	9075,	11,	BW_1M,	2,	1,		-5,	1},
	{"US",	9085,	13,	BW_1M,	2,	1,		5,	0},
	{"US",	9095,	15,	BW_1M,	2,	1,		-5,	1},
	{"US",	9105,	17,	BW_1M,	2,	1,		5,	0},
	{"US",	9115,	19,	BW_1M,	2,	1,		-5,	1},
	{"US",	9125,	21,	BW_1M,	2,	1,		5,	0},
	{"US",	9135,	23,	BW_1M,	2,	1,		-5,	1},
	{"US",	9145,	25,	BW_1M,	2,	1,		5,	0},
	{"US",	9155,	27,	BW_1M,	2,	1,		-5,	1},
	{"US",	9165,	29,	BW_1M,	2,	1,		5,	0},
	{"US",	9175,	31,	BW_1M,	2,	1,		-5,	1},
	{"US",	9185,	33,	BW_1M,	2,	1,		5,	0},
	{"US",	9195,	35,	BW_1M,	2,	1,		-5,	1},
	{"US",	9205,	37,	BW_1M,	1,	1,		5,	0},
	{"US",	9215,	39,	BW_1M,	1,	1,		-5,	1},
	{"US",	9225,	41,	BW_1M,	1,	1,		5,	0},
	{"US",	9235,	43,	BW_1M,	1,	1,		-5,	1},
	{"US",	9245,	45,	BW_1M,	1,	1,		5,	0},
	{"US",	9255,	47,	BW_1M,	1,	1,		-5,	1},
	{"US",	9265,	49,	BW_1M,	1,	1,		5,	0},
	{"US",	9275,	51,	BW_1M,	1,	1,		-5,	1},
	{"US",	9030,	2,	BW_2M,	1,	2,		0,	0},
	{"US",	9050,	6,	BW_2M,	2,	2,		0,	0},
	{"US",	9070,	10,	BW_2M,	2,	2,		0,	0},
	{"US",	9090,	14,	BW_2M,	2,	2,		0,	0},
	{"US",	9110,	18,	BW_2M,	2,	2,		0,	0},
	{"US",	9130,	22,	BW_2M,	2,	2,		0,	0},
	{"US",	9150,	26,	BW_2M,	2,	2,		0,	0},
	{"US",	9170,	30,	BW_2M,	2,	2,		0,	0},
	{"US",	9190,	34,	BW_2M,	2,	2,		0,	0},
	{"US",	9210,	38,	BW_2M,	1,	2,		0,	0},
	{"US",	9230,	42,	BW_2M,	1,	2,		0,	0},
	{"US",	9250,	46,	BW_2M,	1,	2,		0,	0},
	{"US",	9270,	50,	BW_2M,	1,	2,		0,	0},
	{"US",	9060,	8,	BW_4M,	2,	2,		0,	1},
	{"US",	9100,	16,	BW_4M,	2,	3,		0,	1},
	{"US",	9140,	24,	BW_4M,	2,	3,		0,	1},
	{"US",	9180,	32,	BW_4M,	2,	3,		0,	1},
	{"US",	9220,	40,	BW_4M,	1,	3,		0,	1},
	{"US",	9260,	48,	BW_4M,	1,	3,		0,	1},
	{}
};

// EU
static const struct s1g_channel_table s1g_ch_table_eu[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"EU",	8635,	1,	BW_1M,	1,	6,		5,	0},
	{"EU",	8645,	3,	BW_1M,	1,	6,		-5,	1},
	{"EU",	8655,	5,	BW_1M,	1,	6,		5,	0},
	{"EU",	8665,	7,	BW_1M,	1,	6,		-5,	1},
	{"EU",	8675,	9,	BW_1M,	1,	6,		-5,	1},
	{"EU",	8640,	2,	BW_2M,	2,	7,		0,	0},
	{"EU",	8660,	6,	BW_2M,	2,	7,		0,	0},
	{}
};

// China - test channel
/*
static const struct s1g_channel_table s1g_ch_table_cn[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"CN",	4330,	1,	BW_1M,	1,	20,		5,	0},

	{"CN",	4725,	3,	BW_1M,	1,	20,		5,	0},
	{"CN",	4720,	2,	BW_2M,	2,	21,		0,	0},
	{"CN",	4730,	4,	BW_2M,	2,	22,		0,	1},

	{"CN",	4785,	6,	BW_1M,	1,	20,		5,	0},
	{"CN",	4780,	5,	BW_2M,	2,	21,		0,	0},
	{"CN",	4790,	7,	BW_4M,	2,	22,		0,	1},

	{"CN",	4845,	9,	BW_1M,	1,	20, 	5,	0},
	{"CN",	4840,	8,	BW_2M,	2,	21, 	0,	0},
	{"CN",	4850,	10,	BW_4M,	2,	22, 	0,	1},

	{"CN",	4905,	12,	BW_1M,	1,	20, 	5,	0},
	{"CN",	4900,	11,	BW_2M,	2,	21, 	0,	0},
	{"CN",	4910,	13,	BW_4M,	2,	22, 	0,	1},

	{"CN",	4965,	15,	BW_1M,	1,	20, 	5,	0},
	{"CN",	4960,	14,	BW_2M,	2,	21, 	0,	0},
	{"CN",	4970,	16,	BW_4M,	2,	22, 	0,	1},

	{"CN",	5025,	18,	BW_1M,	1,	20, 	5,	0},
	{"CN",	5020,	17,	BW_2M,	2,	21, 	0,	0},
	{"CN",	5030,	19,	BW_4M,	2,	22, 	0,	1},

	{"CN",	5085,	21,	BW_1M,	1,	20, 	5,	0},
	{"CN",	5080,	20,	BW_2M,	2,	21, 	0,	0},
	{"CN",	5090,	22,	BW_4M,	2,	22, 	0,	1},

	{"CN",	5055,	24,	BW_1M,	1,	20, 	5,	0},
	{"CN",	5065,	23,	BW_1M,	1,	20, 	5,	0},
	{}
};
*/
// China
static const struct s1g_channel_table s1g_ch_table_cn[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"CN",	7555,	1,	BW_1M,	1,	9,		5,	0},
	{"CN",	7565,	3,	BW_1M,	1,	9,		-5,	1},
	{"CN",	7575,	5,	BW_1M,	1,	9,		5,	0},
	{"CN",	7585,	7,	BW_1M,	1,	9,		-5,	1},
	{"CN",	7595,	9,	BW_1M,	1,	9,		5,	0},
	{"CN",	7605,	11,	BW_1M,	1,	9,		-5,	1},
	{"CN",	7615,	13,	BW_1M,	1,	9,		5,	0},
	{"CN",	7625,	15,	BW_1M,	1,	9,		-5,	1},
	{"CN",	7635,	17,	BW_1M,	1,	9,		5,	0},
	{"CN",	7645,	19,	BW_1M,	1,	9,		-5,	1},
	{"CN",	7655,	21,	BW_1M,	1,	9,		5,	0},
	{"CN",	7665,	23,	BW_1M,	1,	9,		-5,	1},
	{"CN",	7675,	25,	BW_1M,	1,	9,		5,	0},
	{"CN",	7685,	27,	BW_1M,	1,	9,		-5,	1},
	{"CN",	7695,	29,	BW_1M,	1,	9,		5,	0},
	{"CN",	7705,	31,	BW_1M,	1,	9,		-5,	1},
	{"CN",	7795,	16,	BW_1M,	1,	10,		5,	0},
	{"CN",	7805,	18,	BW_1M,	1,	10,		-5,	1},
	{"CN",	7815,	20,	BW_1M,	1,	10,		5,	0},
	{"CN",	7825,	22,	BW_1M,	1,	10,		-5,	1},
	{"CN",	7835,	24,	BW_1M,	1,	10,		5,	0},
	{"CN",	7845,	26,	BW_1M,	1,	10,		-5,	1},
	{"CN",	7855,	28,	BW_1M,	1,	10,		5,	0},
	{"CN",	7865,	30,	BW_1M,	1,	10,		-5,	1},
	{"CN",	7800,	2,	BW_2M,	2,	11,		0,	0},
	{"CN",	7820,	6,	BW_2M,	2,	11,		0,	0},
	{"CN",	7840,	10,	BW_2M,	2,	11,		0,	0},
	{"CN",	7860,	14,	BW_2M,	2,	11,		0,	0},
	{"CN",	7810,	4,	BW_4M,	2,	12,		0,	1},
	{"CN",	7850,	12,	BW_4M,	2,	12,		0,	1},
	{}
};

// New Zealand
static const struct s1g_channel_table s1g_ch_table_nz[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"NZ",	9155,	27,	BW_1M,	1,	26,		5,	0},
	{"NZ",	9165,	29,	BW_1M,	1,	26,		-5,	1},
	{"NZ",	9175,	31,	BW_1M,	1,	26,		5,	0},
	{"NZ",	9185,	33,	BW_1M,	1,	26,		-5,	1},
	{"NZ",	9195,	35,	BW_1M,	1,	26,		5,	0},
	{"NZ",	9205,	37,	BW_1M,	1,	26,		-5,	1},
	{"NZ",	9215,	39,	BW_1M,	1,	26,		5,	0},
	{"NZ",	9225,	41,	BW_1M,	1,	26,		-5,	1},
	{"NZ",	9235,	43,	BW_1M,	1,	26,		5,	0},
	{"NZ",	9245,	45,	BW_1M,	2,	26,		5,	0},
	{"NZ",	9255,	47,	BW_1M,	2,	26,		-5,	1},
	{"NZ",	9265,	49,	BW_1M,	2,	26,		5,	0},
	{"NZ",	9275,	51,	BW_1M,	2,	26,		-5,	1},
	{"NZ",	9170,	30,	BW_2M,	1,	27,		0,	0},
	{"NZ",	9190,	34,	BW_2M,	1,	27,		0,	0},
	{"NZ",	9210,	38,	BW_2M,	1,	27,		0,	0},
	{"NZ",	9230,	42,	BW_2M,	1,	27,		0,	0},
	{"NZ",	9250,	46,	BW_2M,	2,	27,		0,	0},
	{"NZ",	9270,	50,	BW_2M,	2,	27,		0,	0},
	{"NZ",	9180,	32,	BW_4M,	1,	28,		0,	1},
	{"NZ",	9220,	40,	BW_4M,	1,	28,		0,	1},
	{"NZ",	9260,	48,	BW_4M,	2,	28,		0,	1},
	{}
};

// Australia
static const struct s1g_channel_table s1g_ch_table_au[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"AU",	9155,	27,	BW_1M,	1,	22,		5,	0},
	{"AU",	9165,	29,	BW_1M,	1,	22,		-5,	1},
	{"AU",	9175,	31,	BW_1M,	1,	22,		5,	0},
	{"AU",	9185,	33,	BW_1M,	1,	22,		-5,	1},
	{"AU",	9195,	35,	BW_1M,	1,	22,		5,	0},
	{"AU",	9205,	37,	BW_1M,	2,	22,		5,	0},
	{"AU",	9215,	39,	BW_1M,	2,	22,		-5,	1},
	{"AU",	9225,	41,	BW_1M,	2,	22,		5,	0},
	{"AU",	9235,	43,	BW_1M,	2,	22,		-5,	1},
	{"AU",	9245,	45,	BW_1M,	2,	22,		5,	0},
	{"AU",	9255,	47,	BW_1M,	2,	22,		-5,	1},
	{"AU",	9265,	49,	BW_1M,	2,	22,		5,	0},
	{"AU",	9275,	51,	BW_1M,	2,	22,		-5,	1},
	{"AU",	9170,	30,	BW_2M,	1,	23,		0,	0},
	{"AU",	9190,	34,	BW_2M,	1,	23,		0,	0},
	{"AU",	9210,	38,	BW_2M,	2,	23,		0,	0},
	{"AU",	9230,	42,	BW_2M,	2,	23,		0,	0},
	{"AU",	9250,	46,	BW_2M,	2,	23,		0,	0},
	{"AU",	9270,	50,	BW_2M,	2,	23,		0,	0},
	{"AU",	9180,	32,	BW_4M,	1,	24,		0,	1},
	{"AU",	9220,	40,	BW_4M,	2,	24,		0,	1},
	{"AU",	9260,	48,	BW_4M,	2,	24,		0,	1},
	{}
};


const struct s1g_channel_table * s1g_ch_table_set[MAX_COUNTRY_CODE] = {
	s1g_ch_table_jp,
	s1g_ch_table_k1_usn,
	s1g_ch_table_tw,
	s1g_ch_table_us,
	s1g_ch_table_eu,
	s1g_ch_table_cn,
	s1g_ch_table_nz,
	s1g_ch_table_au,
	s1g_ch_table_k2_mic,
};


int nrc_get_current_ccid_by_country(char * country_code)
{
	uint8_t cc_index;

	if (strcmp(country_code, "JP") == 0) {
		cc_index = JP;
	} else if (strcmp(country_code, "K1") == 0) {
		cc_index = K1;
	} else if (strcmp(country_code, "TW") == 0) {
		cc_index = TW;
	} else if (strcmp(country_code, "US") == 0) {
		cc_index = US;
	} else if (strcmp(country_code, "EU") == 0) {
		cc_index = EU;
	} else if (strcmp(country_code, "DE") == 0) {
		cc_index = EU;
	} else if (strcmp(country_code, "CN") == 0) {
		cc_index = CN;
	} else if (strcmp(country_code, "NZ") == 0) {
		cc_index = NZ;
	} else if (strcmp(country_code, "AU") == 0) {
		cc_index = AU;
	} else if (strcmp(country_code, "K2") == 0) {
		cc_index = K2;
	} else {
		cc_index = -1;
	}
	return cc_index;
}

char* nrc_get_current_s1g_country(void)
{
	return s1g_alpha2;
}

void nrc_set_s1g_country(char* country_code)
{
	uint8_t cc_index = nrc_get_current_ccid_by_country(country_code);

	// Set "US" as the default if it doesn't exist in the S1G channel table
	if (cc_index >= MAX_COUNTRY_CODE) {
                cc_index = US;
                s1g_alpha2[0] = 'U';
                s1g_alpha2[1] = 'S';
                nrc_dbg(NRC_DBG_MAC, "%s: Country index %d out of range. Setting default country (US)!", __func__, cc_index);
        } if (cc_index < 0) {
		cc_index = US;
		s1g_alpha2[0] = 'U';
		s1g_alpha2[1] = 'S';
		// nrc_dbg(NRC_DBG_MAC, "%s: No country code %s. Setting default country (US)", __func__, country_code);
	} else if (cc_index == EU) {
		// convert country code DE to EU for target firmware
		s1g_alpha2[0] = 'E';
		s1g_alpha2[1] = 'U';
	} else {
		s1g_alpha2[0] = country_code[0];
		s1g_alpha2[1] = country_code[1];
	}
	s1g_alpha2[2] = '\0';
	ptr_nrc_s1g_ch_table = &s1g_ch_table_set[cc_index][0];

	nrc_dbg(NRC_DBG_MAC, "%s: Country code %s, number of channels: %d", __func__, country_code, nrc_get_num_channels_by_current_country());
}

const struct s1g_channel_table * nrc_get_current_s1g_cc_table(void)
{
	return &s1g_ch_table_set[nrc_get_current_ccid_by_country(nrc_get_current_s1g_country())][0];
}


int nrc_get_num_channels_by_current_country(void)
{
	int i = 0;
	for (i=0; i< MAX_S1G_CHANNEL_NUM; i++) {
		if (!ptr_nrc_s1g_ch_table[i].s1g_freq)
			return i;
	}
	return MAX_S1G_CHANNEL_NUM;
}


int nrc_get_s1g_freq_by_arr_idx(int arr_index)
{
	return ptr_nrc_s1g_ch_table[arr_index].s1g_freq;
}


int nrc_get_s1g_width_by_freq(int freq)
{
	int i = 0;

	for (i=0; i< nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].chan_spacing;
	}
	return ptr_nrc_s1g_ch_table[0].chan_spacing;
}


int nrc_get_s1g_width_by_arr_idx(int arr_index)
{
	return ptr_nrc_s1g_ch_table[arr_index].chan_spacing;
}

void nrc_s1g_set_channel_bw(int freq, struct cfg80211_chan_def *chandef)
{
	int w;

	w = nrc_get_s1g_width_by_freq(freq);
	switch(w)
	{
		default:
		case 1:
			chandef->width = NL80211_CHAN_WIDTH_1;
			break;
		case 2:
			chandef->width = NL80211_CHAN_WIDTH_2;
			break;
		case 4:
			chandef->width = NL80211_CHAN_WIDTH_4;
			break;
	}
}


uint8_t nrc_get_channel_idx_by_freq(int freq)
{
	int i = 0;
	for (i=0; i< nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].s1g_freq_index;
	}
	return ptr_nrc_s1g_ch_table[0].s1g_freq_index;
}


uint8_t nrc_get_cca_by_freq(int freq)
{
	int i = 0;
	for (i=0; i< nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].cca_level_type;
	}
	return ptr_nrc_s1g_ch_table[0].cca_level_type;
}


uint8_t nrc_get_oper_class_by_freq(int freq)
{
	int i = 0;
	for (i=0; i< nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].global_oper_class;
	}
	return ptr_nrc_s1g_ch_table[0].global_oper_class;
}


uint8_t nrc_get_offset_by_freq(int freq)
{
	int i = 0;
	for (i=0; i< nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].offset;
	}
	return ptr_nrc_s1g_ch_table[0].offset;
}


uint8_t nrc_get_pri_loc_by_freq(int freq)
{
	int i = 0;
	for (i=0; i< nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].primary_loc;
	}
	return ptr_nrc_s1g_ch_table[0].primary_loc;
}
#endif /* CONFIG_S1G_CHANNEL */