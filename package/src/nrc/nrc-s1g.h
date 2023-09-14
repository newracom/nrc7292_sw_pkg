

#ifndef _NRC_S1G_H_
#define _NRC_S1G_H_

#include "nrc.h"
#include "nrc-wim-types.h"
#include <linux/types.h>
#if KERNEL_VERSION(5,18,0) > NRC_TARGET_KERNEL_VERSION
#include <stddef.h>
#endif
#if defined(CONFIG_S1G_CHANNEL)
#define FREQ_TO_100KHZ(mhz, khz) (mhz * 10 + khz / 100)

#define MAX_S1G_CHANNEL_NUM 70 	/* Max Num of S1G Channels */
#define BW_1M	0
#define BW_2M	1
#define BW_4M	2

int nrc_get_current_ccid_by_country(char * country_code);
char* nrc_get_current_s1g_country(void);
void nrc_set_s1g_country(char* country_code);
int nrc_get_num_channels_by_current_country(void);
int nrc_get_s1g_freq_by_arr_idx(int arr_index);
int nrc_get_s1g_width_by_freq(int freq);
int nrc_get_s1g_width_by_arr_idx(int arr_index);
uint8_t nrc_get_channel_idx_by_freq(int freq);
uint8_t nrc_get_cca_by_freq(int freq);
uint8_t nrc_get_oper_class_by_freq(int freq);
uint8_t nrc_get_offset_by_freq(int freq);
uint8_t nrc_get_pri_loc_by_freq(int freq);
void nrc_s1g_set_channel_bw(int freq, struct cfg80211_chan_def *chandef);
const struct s1g_channel_table * nrc_get_current_s1g_cc_table(void);
#endif /* CONFIG_S1G_CHANNEL */
#endif