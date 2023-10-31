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
#ifndef _NRC_STATS_H_
#define _NRC_STATS_H_

#define MAX_SHOWN_RSSI		(-10)

#define MAX_CHANNEL_NUM		40
#define TIMEOUT_CHANNEL_NOISE	60	//second

struct stats_sta {
	uint8_t macaddr[6];
	struct moving_average *snr;
	struct moving_average *rssi;

	struct list_head list;
};

struct stats_channel_noise {
	int8_t noise;
	struct ieee80211_channel *chan;
};

int nrc_stats_snr_init(int count);
void nrc_stats_snr_deinit(void);
void nrc_stats_snr_update(uint8_t snr);
int nrc_stats_snr(void);

int nrc_stats_rssi_init(int count);
void nrc_stats_rssi_deinit(void);
void nrc_stats_rssi_update(int8_t rssi);
int nrc_stats_rssi(void);

uint32_t nrc_stats_calc_metric(int rssi);
uint32_t nrc_stats_metric(uint8_t *macaddr);
int nrc_stats_set_mesh_rssi_threshold(int rssi);
int nrc_stats_get_mesh_rssi_threshold(void);

int nrc_stats_init(void);
void nrc_stats_deinit(void);
int nrc_stats_update(uint8_t *macaddr, int8_t snr, int8_t rssi);
int nrc_stats_add(uint8_t *macaddr, int count);
int nrc_stats_del(uint8_t *macaddr);
void nrc_stats_print(void);
int nrc_stats_report(struct nrc* nw, uint8_t *output, int index, int number);
int nrc_stats_report_count(void);

int nrc_stats_channel_noise_update(uint32_t freq, int8_t noise);
int nrc_stats_channel_noise_reset(void);
struct stats_channel_noise *nrc_stats_channel_noise_report(int report_num, uint32_t freq);
#endif
