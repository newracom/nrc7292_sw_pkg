/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef _CLI_UTIL_
#define _CLI_UTIL_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <termios.h>

#include "cli_cmd.h"
#include "cli_util.h"
#include "cli_xfer.h"

#define AWK_BUF_LENGTH 512
#define KEY_LIST_MAX_SIZE 100

#define MAX_TAB_NUMBER 5
#define TAB_TO_SPACE_LENGTH 8

static char log_file_prefix[64] = "nrc_signal";
static FILE *fp_log_file = NULL;


static struct termios old, new;
/*******************************************************************************
* util functions
*******************************************************************************/
static int isblank2(char c)
{
	return ((c == ' ') || (c == '\t'));
}

int util_cmd_parse_line(char *s, char *argv[])
{
	int argc = 0;

	while (argc < NRC_MAX_ARGV) {
		while (isblank2(*s))
			s++;

		if (*s == '\0')
			goto out;

		argv[argc++] = s;

		while (*s && !isblank2(*s))
			s++;

		if (*s == '\0')
			goto out;

		*s++ = '\0';
	}
	printf("Too many args\n");

out:
	argv[argc] = NULL;
	return argc;
}

static cmd_tbl_t *find_cmd(char *cmd, cmd_tbl_t *list, int list_size)
{
	cmd_tbl_t *t = NULL;
	int i = 0;

	for (i = 0; i < list_size; i++) {
		t = (cmd_tbl_t *)(list + i);
		if (strcmp(cmd, t->name) == 0) {
			return t;
		}
	}
	return NULL;
}

int run_sub_cmd(cmd_tbl_t *t, int argc, char *argv[], cmd_tbl_t *list, int list_size, int depth)
{
	cmd_tbl_t *sub_t = find_cmd(argv[depth], list, list_size);

	if (!sub_t) {
		return CMD_RET_FAILURE;
	}
	else {
		if(argc < 3){
			if((strcmp("set", argv[0]) == 0)||(strcmp("test",  argv[0]) == 0)){
				printf("usage : %s\n", (char*)sub_t->usage);
				return CMD_RET_FAILURE;
			}
		}
		return sub_t->handler(sub_t, argc, argv);
	}
}

static int cmd_process(int argc, char* argv[])
{
	char *cmd = argv[0];
	cmd_tbl_t *t = NULL;
	int main_cmd_list_size, main_cmd_list_depth;
	int sys_cmd_list_size, sys_cmd_list_depth;

	cmd_tbl_t *main_cmd_list = get_cmd_list(MAIN_CMD, &main_cmd_list_size, &main_cmd_list_depth);
	cmd_tbl_t *sys_cmd_list = get_cmd_list(SYS_CMD, &sys_cmd_list_size, &sys_cmd_list_depth);

	if (!cmd)
		return CMD_RET_EMPTY_INPUT;

	t = find_cmd(cmd, main_cmd_list, main_cmd_list_size);

	if (!t) {
		return run_sub_cmd(NULL, argc, argv, sys_cmd_list, sys_cmd_list_size, sys_cmd_list_depth);
	}
	else {
		return t->handler(t, argc, argv);
	}
}

int cli_app_run_command(char *cmd)
{
	char *argv[NRC_MAX_ARGV] = { NULL, };
	int argc = 0;
	int ret = CMD_RET_FAILURE;
	int nTry =10;

	if (!cmd || *cmd == '\0')
		goto ret_fail;

	if (strlen(cmd) >= NRC_MAX_CMDLINE_SIZE) {
		printf("Command too long\n");
		goto ret_fail;
	}
	argc = util_cmd_parse_line(cmd, argv);

	if (argc == 0)
		goto ret_fail;

	while(nTry--){
		ret = cmd_process(argc, argv);
		if (ret == CMD_RET_SUCCESS || ret == CMD_RET_FAILURE) {
			break;
		}else{
			cli_delay_ms(COMMAND_DELAY_MS);
		}
	}

	return ret;

ret_fail:
	return CMD_RET_EMPTY_INPUT;
}

int cli_app_run_onetime(int argc, char* argv[]){
	int ret = CMD_RET_SUCCESS;
	char buffer[NRC_MAX_CMDLINE_SIZE] = { 0, };

	int i =0, j=0;
	for(i=1; i<argc; i++){
	        sprintf((buffer+j), "%s ", argv[i]);
		j = strlen(buffer);
	}
	*(buffer + strlen(buffer)-1) = '\0';

	ret = cli_app_run_command(buffer);
	if (ret == CMD_RET_SUCCESS) {
		printf("OK\n");
	}else {
		printf("FAIL\n");
	}
	return 0;
}

int cli_app_list_version_check(void)
{
	int ret = CMD_RET_SUCCESS;
	char buffer[NRC_MAX_CMDLINE_SIZE] = { 0, };
	sprintf(buffer, "show cli_app_list_version -sr");
	ret = cli_app_run_command(buffer);
	cli_delay_ms(COMMAND_DELAY_MS);

	return ret;
}

int get_data_number(char* input)
{
	char temp_input[512];
	int len = strlen(input);
	memcpy(temp_input, input, len);
	char* token = strtok(temp_input, ",");
	int number=0;
	while (token != NULL) {
		++number;
		token = strtok(NULL, ",");
	}
	return number;
}

static  int get_tab_number(int max_tab_number, int string_length)
{
	int max_length = TAB_TO_SPACE_LENGTH*max_tab_number - 1;
	return (max_length - string_length) / TAB_TO_SPACE_LENGTH;
}

cmd_tbl_t *cmd_list_display(enum cmd_list_type type)
{
	int list_size, list_depth;
	cmd_tbl_t *list = get_cmd_list(type, &list_size, &list_depth);

	cmd_tbl_t *t = NULL;
	int i = 0, j = 0;
	int tab_len =0;
	int max_tab_number = 11;

	for (i = 0; i < list_size; i++) {
		t = (cmd_tbl_t *)(list + i);
		if(t->flag == 0){
			tab_len = get_tab_number(max_tab_number, strlen(t->usage)+1);
			printf(" %s",t->usage);
			for(j=0; j<tab_len; j++)
				printf("\t");
			printf(":%s\n", t->desc);
		}
	}
	return NULL;
}

static int get_space_number_including_tab(int num, int tab_num) {
	int result = (num/TAB_TO_SPACE_LENGTH)+ (TAB_TO_SPACE_LENGTH*tab_num);
	return result;
}

static void add_print_tab(int *j, int tab_number, int string_length) {
	int i = *j;
	int k = 0, max_tab_number = 4;
	int iteration = get_tab_number(max_tab_number, string_length);

	if (i < tab_number) {
		for(k=0; k<iteration;k++)
			printf("\t");
		i++;
	}
	else {
		if(tab_number == 0) // diplay one per line
			printf("\n");
		i = 0;
	}
	*j = i;
}

static int print_merged_result(char* t1, char *t2, int displayPerLine)
{
	int i = 0, tab_number = 0, max_tab_number = 0;
	int t1_length, t2_length;

	t1_length = strlen_last_line(t1);
	t2_length = strlen(t2);

	if (displayPerLine > 1) {
		max_tab_number = 2;
	}
	else {
		max_tab_number = MAX_TAB_NUMBER;
	}
	tab_number = get_tab_number(max_tab_number, t1_length);

	printf("%s", t1);
	for (i = 0; i<tab_number; i++)
		printf("\t");
	printf(" : %s", t2);

	return (get_space_number_including_tab(t1_length, tab_number)+ t2_length + 3);
}

void cmd_result_parse(char* key, char *value, const int displayPerLine)
{
	char *key_list_temp = NULL;
	char *next_ptr1 = NULL, *next_ptr2 = NULL;
	char * t1, *t2;
	int tab_number = displayPerLine - 1;
	int i = 0;
	int merged_string_length = 0;

	if( value == NULL)
		return;

	if( key == NULL || *key=='\0' ){
		printf("%s\n", value);
		return;
	}

	if((strcmp("success", value) == 0)||(strcmp("fail", value) == 0)){
		return;
	}

	key_list_temp = malloc(DISP_CMD_RESULT_BUF);
	memset(key_list_temp, 0x0, DISP_CMD_RESULT_BUF);
	memcpy(key_list_temp, key, strlen(key));

	t1 = strtok_r(key_list_temp, ",", &next_ptr1);
	t2 = strtok_r(value, ",", &next_ptr2);

	while (1)
	{
		if(t2 == NULL)
			break;

		if (t1 != NULL) {
			if(strcmp(t2, "N/A")!=0){
				merged_string_length = print_merged_result(t1, t2, displayPerLine);
			}else{
				merged_string_length = 0;
			}
			t1 = strtok_r(NULL, ",", &next_ptr1);
			t2 = strtok_r(NULL, ",", &next_ptr2);
			if(merged_string_length>0){
				add_print_tab(&i, tab_number, merged_string_length);
			}
		}else {
			if(key_list_temp){
				free(key_list_temp);
				key_list_temp = NULL;
			}
			next_ptr1 = NULL;
			key_list_temp = malloc(DISP_CMD_RESULT_BUF);
			memset(key_list_temp, 0x0, DISP_CMD_RESULT_BUF);
			memcpy(key_list_temp, key, strlen(key));
			t1 = strtok_r(key_list_temp, ",", &next_ptr1);
			printf("\n");	// Line for iteration division
			if(strcmp(t2, "N/A")!=0){
				merged_string_length = print_merged_result(t1, t2, displayPerLine);
			}else{
				merged_string_length = 0;
			}
			t1 = strtok_r(NULL, ",", &next_ptr1);
			t2 = strtok_r(NULL, ",", &next_ptr2);
			if(merged_string_length>0){
				add_print_tab(&i, tab_number, merged_string_length);
			}
		}
	}

	if(key_list_temp)
		free(key_list_temp);
}

int cmd_umac_info_result_parse(char *value, int mode, int *display_start_index)
{
	if(value == NULL || strncmp("success", value, 7) == 0 || strncmp("fail", value, 4) == 0)
		return 0;

	uint8_t header = value[0];
	uint8_t count = (header & 0x7F);
	uint8_t more  = (header >> 7) & 1;

	if(!count)
		return 0;

	if(mode == AP_UMAC_INFO)
	{
		xfer_umac_apinfo* apinfo = (xfer_umac_apinfo*) &value[1];
		printf("[%5d]  bssid(%02x:%02x:%02x:%02x:%02x:%02x)\tssid(%s)\tssid_len(%u)\tsecurity(%d)\tbeacon_interval(%d)\n",
			*display_start_index, apinfo->bssid[0], apinfo->bssid[1], apinfo->bssid[2], apinfo->bssid[3], apinfo->bssid[4], apinfo->bssid[5], apinfo->ssid, apinfo->ssid_len, apinfo->security, apinfo->beacon_interval);
		printf("\t short bi(%u)\t\t\tassoc_s1g_channel(%u)\tcssid(0x%x)\tchange_seq_num(%u)\n",
			apinfo->short_beacon_interval, apinfo->assoc_s1g_channel, apinfo->comp_ssid, apinfo->change_seq_num);
		printf("support: s1g_long(%d)\tpv1(%d)\t\tnontim(%d)\t\ttwt(%d)\t\tampdu(%d)\n",
			apinfo->s1g_long_support, apinfo->pv1_frame_support, apinfo->nontim_support, apinfo->twtoption_activated,
			apinfo->ampdu_support);
		printf("\t ndp_pspoll(%d)\t\t\ttraveling pilot(%u)\tshortgi(1mhz:%d, 2mhz:%d, 4mhz:%d)\t1m_ctrl_resp_preamble(%d) \n",
			apinfo->ndp_pspoll_support, apinfo->traveling_pilot_support, apinfo->shortgi_1mhz_support, apinfo->shortgi_2mhz_support,
			apinfo->shortgi_4mhz_support, apinfo->s1g_1mctrlresppreamble_support);
		printf("\t maximum mpdu_len(%u)\t\tampdu_len_exp(%u)\tminimum mpdu_start_spacing(%u)\trx_s1gmcs_map(0x%x)\tcolor(%u)\n",
			apinfo->maximum_mpdu_length, apinfo->maximum_ampdu_length_exp, apinfo->minimum_mpdu_start_spacing,
			apinfo->rx_s1gmcs_map, apinfo->color);
		printf("\n");
	}
	else if(mode == STA_UMAC_INFO)
	{
		xfer_umac_stainfo* stainfo = (xfer_umac_stainfo*) &value[1];
		for(int i=0;i<count;i++)
		{
			printf("[%5d]  mac_addr(%02x:%02x:%02x:%02x:%02x:%02x)\taid(%u)  \t\tlisten_interval(%u)\n",
				i + *display_start_index, stainfo->maddr[0], stainfo->maddr[1], stainfo->maddr[2], stainfo->maddr[3], stainfo->maddr[4], stainfo->maddr[5], stainfo->aid, stainfo->listen_interval);
			printf("support: s1g_long(%d)\tpv1(%d)\t\tnontim(%d)\t\ttwt(%d)\t\t\t\tampdu(%d)\n",
				stainfo->s1g_long_support, stainfo->pv1_frame_support, stainfo->nontim_support,
				stainfo->twtoption_activated, stainfo->ampdu_support);
			printf("\t ndp_pspoll(%d)\t\t\ttraveling_pilot(%u)\tshortgi(1mhz:%d, 2mhz:%d, 4mhz:%d)\t1m_ctrl_resp_preamble(%d)\n",
				stainfo->ndp_pspoll_support, stainfo->traveling_pilot_support, stainfo->shortgi_1mhz_support, stainfo->shortgi_2mhz_support,
				stainfo->shortgi_4mhz_support, stainfo->s1g_1mctrlresppreamble_support);
			printf("\t maximum mpdu_len(%u)\t\tampdu_len_exp(%u)\tminimum mpdu_start_spacing(%u)\trx_s1gmcs_map(0x%x)\n",
				stainfo->maximum_mpdu_length, stainfo->maximum_ampdu_length_exp, stainfo->minimum_mpdu_start_spacing, stainfo->rx_s1gmcs_map);
			printf("\n");
			stainfo++;
		}
		*display_start_index += count;
	}
	return more;
}

int cmd_sta_umac_info_mini_result_parse(char *value, int *display_start_index, int *aid_count)
{
	if(value == NULL || strncmp("success", value, 7) == 0 || strncmp("fail", value, 4) == 0)
		return 0;

	uint8_t header = value[0];
	uint8_t count = (header & 0x7F);
	uint8_t more  = (header >> 7) & 1;

	if(!count)
		return 0;

	xfer_umac_stainfo_mini* stainfo = (xfer_umac_stainfo_mini*) &value[1];
	int aid;
	for(int i=0;i<count;i++)
	{
		aid = stainfo->aid;

		double tx_rate = 0;
		double rx_rate = 0;
		uint8_t bw = stainfo->bw;
		uint8_t sgi = stainfo->sgi;
		uint8_t tx_mcs = stainfo->tx_mcs;
		uint8_t rx_mcs = stainfo->rx_mcs;

		if (sgi) {
			tx_rate = mcs_to_phy_rate_sgi[bw][tx_mcs]/1000.0;
			rx_rate = mcs_to_phy_rate_sgi[bw][rx_mcs]/1000.0;
		} else {
			tx_rate = mcs_to_phy_rate_lgi[bw][tx_mcs]/1000.0;
			rx_rate = mcs_to_phy_rate_lgi[bw][rx_mcs]/1000.0;
		}

		printf("%-7d    %02x:%02x:%02x:%02x:%02x:%02x    %-5d  %s   %-2.2fMBit/s(MCS%2d)  %-2.2fMBit/s(MCS%2d)\n",
			i + *display_start_index, stainfo->maddr[0], stainfo->maddr[1], stainfo->maddr[2], stainfo->maddr[3], stainfo->maddr[4], stainfo->maddr[5],
			aid, stainfo->state == 0 ? "AUTH" : stainfo->state == 1 ? "ASSOC" : "INVALID",
			tx_rate, tx_mcs, rx_rate, rx_mcs);
		stainfo++;
		if(aid_count)
			aid_count[(0 < aid && aid < 2048) ? aid : 0]++;
	}
	*display_start_index += count;
	return more;
}


int cmd_set_maxagg_result_parse(char *value)
{
	const char *str_ac[4] ={"BK", "BE", "VI", "VO"};
	const char *str_state[2] ={"OFF", "ON"};
	const char *str_ba_session[2] ={"NO", "YES"};
	int index = 0;
	int vif_id = 0;
	if(value == NULL || strncmp("success", value, 7) == 0 || strncmp("fail", value, 4) == 0)
		return 0;

	uint8_t header = value[index++];
	uint8_t count = (header & 0x7F);
	uint8_t more  = (header >> 7) & 1;

	if(!count)
		return 0;

	xfer_maxagg_info* maxagg_info = (xfer_maxagg_info*) &value[index];

	printf("-- updated aggregation\n");

	for(int i=0;i<count;i++)
	{
		if(maxagg_info->aid == 1)
			printf("------- VIF%d -------\n", vif_id++);

		if(maxagg_info->is_ap)
			printf("[STA AID: %4d]\n",  maxagg_info->aid);

		printf("AC(%s): STATUS(%s) MAXNUM(%2d) SIZE(%d)\n",
			str_ac[maxagg_info->ac], str_state[maxagg_info->state],
			maxagg_info->max_agg_num, maxagg_info->agg_num_size);
	}

	return 0;
}


int cmd_show_maxagg_result_parse(char *value, int vif, int *display_start_index)
{
	const char *str_ac[4] ={"BK", "BE", "VI", "VO"};
	const char *str_state[2] ={"OFF", "ON"};
	const char *str_ba_session[2] ={"NO", "YES"};
	int index = 0;
	int vif_id = vif;
	if (value == NULL || strncmp("success", value, 7) == 0 || strncmp("fail", value, 4) == 0)
		return 0;

	uint8_t header = value[index++];
	uint8_t count = (header & 0x7F);
	uint8_t more  = (header >> 7) & 1;

	if (!count) {
		printf("No information (VIF%d)\n", vif_id);
		return 0;
	}

	xfer_maxagg_info* maxagg_info = (xfer_maxagg_info*) &value[index];
	printf("------- VIF%d -------\n", vif_id);
	for(int i=0;i<count;i++) {
		if ( 0 == maxagg_info->aid) {
			printf("[Base info]\n");
		} else {
			printf("[STA AID: %4d]\n", maxagg_info->aid);
		}

		for (int j=0; j<4; j++) {
			if ( 0 == maxagg_info->aid) {
				printf("AC_%s: %-3s (%2d, %4d bytes)\n",
					str_ac[maxagg_info->ac], str_state[maxagg_info->state], maxagg_info->max_agg_num,
					maxagg_info->agg_num_size );
			} else {
				printf("AC_%s: %-3s",str_ac[maxagg_info->ac], str_state[maxagg_info->state]);
				if (maxagg_info->state) {
					printf(" (%2d, %4d bytes)\n",maxagg_info->max_agg_num, maxagg_info->agg_num_size);
				} else {
					printf("\n");
				}
 			}
			index += sizeof(xfer_maxagg_info);
			maxagg_info = (xfer_maxagg_info*) &value[index];
		}
	}
	*display_start_index += count;
	return more;
}

void print_line(char c, int size, char* string, int upper_newline, int lower_newline)
{
	int i = 0, len = 0, start = 0;

	if (string != NULL) {
		len = strlen(string);
	}
	start = (size - len) / 2;

	if(upper_newline>0){
		for(i=0;i<upper_newline; i++){
			printf("\n");
		}
	}

	for (i = 0; i < start-1; i++){
		printf("%c", c);
	}
	if (string)
		printf("%s", string);

	for (i = (start + len); i < size ; i++){
		printf("%c", c);
	}
	printf("\n");

	if(lower_newline>0){
		for(i=0;i<lower_newline; i++)
			printf("\n");
	}
}

int  strlen_last_line(char* string)
{
	char* str = NULL;
	char* s1 = NULL;

	if(!string)
		return 0;

	str = string;
	for (s1 = str; ; s1++){
 		if (!(*s1)){
			 return s1 - str;
		}else if(*s1=='\n'){
			str = (s1+1);
		}
	}
}

void run_awk(char* filename, int num, char* delim, int* pos, char** result)
{
	FILE *f;
	int count = 0, i=0;
	char read_file[AWK_BUF_LENGTH];

	f = fopen(filename, "r+");
	if (f != NULL) {
		if (fgets(read_file, AWK_BUF_LENGTH, f) != NULL) {
			char* token = strtok(read_file, delim);
			while (token != NULL) {
				if (count == pos[i]) {
					strcpy(result[i], token);
					i++;
					if (i == num)
						break;
				}
				token = strtok(NULL, delim);
				++count;
			}
			while(i != num){
				strcpy(result[i], "");
				i++;
			}
		}
		fclose(f);
	}
}

int signal_log_create(int interval)
{
	char filename[1024];
	struct timeval tv;
	struct tm *tm1;
	gettimeofday(&tv, NULL);
	tm1 = localtime(&tv.tv_sec);

	snprintf(filename, 1024, "%s_%04d%02d%02d_%02d%02d%02d.log",log_file_prefix, \
		1900 + tm1->tm_year, tm1->tm_mon+ 1, tm1->tm_mday, tm1->tm_hour, tm1->tm_min, tm1->tm_sec);

	if(fp_log_file != NULL) {
		fclose(fp_log_file);
		fp_log_file = NULL;
	}

	if((fp_log_file = fopen(filename, "w")) == NULL) {
		return -1;
	}
	fprintf(fp_log_file, "---------------------\n");
	fprintf(fp_log_file, " Date\t: %04d.%02d.%02d\n",1900 + tm1->tm_year, tm1->tm_mon+ 1,tm1->tm_mday);
	fprintf(fp_log_file, " Time\t: %02d:%02d:%02d\n", tm1->tm_hour, tm1->tm_min, tm1->tm_sec);
	fprintf(fp_log_file, " Int.\t: %d sec\n",interval);
	fprintf(fp_log_file, "---------------------\n");
	setvbuf(fp_log_file, NULL, _IOLBF, 0);
	return 0;
}

int signal_log_close(void)
{
	if(fp_log_file != NULL) {
		fclose(fp_log_file);
		fp_log_file = NULL;
	}
	return 0;
}

int signal_log_display(char* mac_addr, int snr_sum, int snr_sum_sqrs, int rssi_sum, int rssi_sum_sqrs, int n)
{
	double avg_rssi, avg_snr;
	double std_dev_rssi, std_dev_snr;
	char mac_addr_temp[MAX_ADDR_SIZE];
	memset(mac_addr_temp, 0x0, MAX_ADDR_SIZE);
	memcpy(mac_addr_temp, mac_addr, MAX_ADDR_SIZE);
	mac_addr_temp[MAX_ADDR_SIZE]='\0';

	avg_rssi = calculate_avergage(rssi_sum, n);
	avg_snr = calculate_avergage(snr_sum, n);
	std_dev_rssi = calculate_std_dev(rssi_sum, rssi_sum_sqrs, n);
	std_dev_snr = calculate_std_dev(snr_sum, snr_sum_sqrs, n);

	printf("--------------------------------------------------\n");
	printf("[MAC Addr]: %s\n", mac_addr_temp);
	printf("[Total]   : %d\n", n);
	printf("[RSSI]\n");
	printf(" average  : %.3lf\n", avg_rssi);
	printf(" std_dev  : %.3lf\n", std_dev_rssi);
	printf("[SNR]\n");
	printf(" average  : %.3lf\n", avg_snr);
	printf(" std_dev  : %.3lf\n", std_dev_snr);
	printf("--------------------------------------------------\n");

	if(fp_log_file != NULL) {
		fprintf(fp_log_file, "--------------------------------------------------\n");
		fprintf(fp_log_file, "[MAC Addr]: %s\n", mac_addr_temp);
		fprintf(fp_log_file, "[Total]   : %d\n", n);
		fprintf(fp_log_file, "[RSSI]\n");
		fprintf(fp_log_file, " average  : %.3lf\n", avg_rssi);
		fprintf(fp_log_file, " std_dev  : %.3lf\n", std_dev_rssi);
		fprintf(fp_log_file, "[SNR]\n");
		fprintf(fp_log_file, " average  : %.3lf\n", avg_snr);
		fprintf(fp_log_file, " std_dev  : %.3lf\n", std_dev_snr);
		fprintf(fp_log_file, "--------------------------------------------------\n");
	}
	return 0;
}

int signal_log_update(int device_number, char* mac_addr, int rssi, int snr){
	printf("Mac Addr : %s\trssi: %d  \tsnr: %d\n", mac_addr, rssi, snr);
	if(fp_log_file != NULL) {
		fprintf(fp_log_file, "Mac Addr : %s\trssi: %d  \tsnr: %d\n", mac_addr, rssi, snr);
	}
	return 0;
}

void cli_delay_ms(int ms)
{
	usleep(ms*1000);
}

/*	Calculate the average	*/
double calculate_avergage(int sum, int n)
{
	double average;
	average = (double)sum / (double)n;
	return average;
}

/*	Calculate the standard deviation	*/
double calculate_std_dev(int sum, int sum_sqrs, int n)
{
	double variance=0.0, average=0.0, std_dev=0.0;

	/*
		Compute the average and variance,using the equality
		Var(X) = E(X^2) - E(X)*E(X)
	*/
	average = (double)calculate_avergage(sum, n);
	variance = (double)sum_sqrs / (double)n - (average)*(average);
	/* Compute the standard deviation. */
	std_dev = sqrt(variance);
	return std_dev;
}

void eliminate_char(char *str, char ch)
{
	for (; *str != '\0'; str++) {
		if (*str == ch){
			strcpy(str, str + 1);
			str--;
		}
	}
}

void string_to_hexString(char* input, char* output)
{
	int loop = 0;
	int i = 0;

	while(input[loop] != '\0') {
		sprintf((char*)(output+i),"%02X", input[loop]);
		loop+=1;
		i+=2;
	}
	output[i++] = '\0';
}

char hex_to_int(char c)
{
	if (c <= '9') return c - '0';
	c = tolower(c);
	if (c == 'a') return 10;
	if (c == 'b') return 11;
	if (c == 'c') return 12;
	if (c == 'd') return 13;
	if (c == 'e') return 14;
	if (c == 'f') return 15;
	return 0;
}

void macaddr_to_ascii(char* input, char* output)
{
	int i = 0;
	char* src = input;
	char* dst = output;

	for(i=0; i<6; i++) {
		const unsigned char high = hex_to_int(*src++);
		const unsigned char low  = hex_to_int(*src++);
		*dst++ = (high << 4) | low;
	}
	*dst = '\0';
}

void print_hex(void *address, int count)
{
	int data_size = count;
	uint8_t *p = (uint8_t*) address;
	int i;

	printf("\n-- %p data_size=%d\n", address, data_size);
	for (i = 0; i < data_size; i++) {
		if (0 == (i & 7))
			printf("\n%03d :", i);

		printf("%02X ", *p);
		p++;
	}
	printf("\n");
}

void print_mac_address(char mac_addr[6])
{
	for(uint8_t i=0;i<6;i++)
		printf("%02x%s", mac_addr[i], i == 5 ? "" : ":");
	printf("\n");
}


// Initialize new terminal i/o settings
static void initTermios(int echo)
{
	tcgetattr(0, &old); // grab old terminal i/o settings
	new = old; // make new settings same as old settings
	new.c_lflag &= ~ICANON; // disable buffered i/o
	new.c_lflag &= echo ? ECHO : ~ECHO; // set echo mode
	tcsetattr(0, TCSANOW, &new); // use these new terminal i/o settings now
}

// Restore old terminal i/o settings
static void resetTermios(void)
{
	tcsetattr(0, TCSANOW, &old);
}

// Read 1 character - echo defines echo mode
static char getch_(int echo)
{
	char ch;
	initTermios(echo);
	ch = getchar();
	resetTermios();
	return ch;
}

// Read 1 character without echo
char cli_getch(void)
{
	return getch_(0);
}

// Read 1 character with echo
char cli_getche(void)
{
	return getch_(1);
}

void cli_input_prompt(const char* prompt_name, char* input)
{
	printf("\r");
	printf("%*s\r", NRC_MAX_CMDLINE_SIZE, " ");
	printf("\r%s> %s",prompt_name, input);
}

void cli_sysconfig_print(xfer_sys_config_t *sysconfig, bool hex, int sysconfig_format)
{
	if (hex) {
		print_hex(sysconfig, sizeof(xfer_sys_config_t) - sizeof(sysconfig->user_factory));
	} else {
		printf(" version\t: %d\n", sysconfig->version);
		printf(" mac_addr0\t: "); print_mac_address(sysconfig->mac_addr0);
		printf(" mac_addr1\t: "); print_mac_address(sysconfig->mac_addr1);
		printf(" cal_use\t: %d\n", sysconfig->cal_use);
		printf(" hw_version\t: %d\n", sysconfig->hw_version);
		if(sysconfig_format == SYSCONFIG_FORMAT_1){
			xfer_sys_config_pllldo_t* rf_pllldo12_tr = (xfer_sys_config_pllldo_t*)sysconfig->reserved1;
			printf(" rf_pllldo12_tr\t: 0x%08X (%s)\n", rf_pllldo12_tr->value,
						(rf_pllldo12_tr->control==1)?"Use":"Disabled");
		} else {
			printf(" trx_pass_fail\t:\n");
			printf("   cfo_cal\t: %d\n",	sysconfig->trx_pass_fail.cfo_cal);
			printf("   da_cal\t: %d\n",   sysconfig->trx_pass_fail.da_cal);
			printf("   txpwr_cal\t: %d\n", sysconfig->trx_pass_fail.txpwr_cal);
			printf("   rssi_cal\t: %d\n",	 sysconfig->trx_pass_fail.rssi_cal);
			printf("   tx_test\t: %d\n",	  sysconfig->trx_pass_fail.tx_test);
			printf("   rx_test\t: %d\n",	 sysconfig->trx_pass_fail.rx_test);
			printf(" chip_type\t: %d\n", sysconfig->chip_type.type);
			printf(" module_type\t: %d\n", sysconfig->module_type.type);
			printf(" module_feature\t:\n");
			printf("   txpwr_boosting_valid\t: %d\n", sysconfig->module_feature.txpwr_boosting_valid);
			printf("   fem_polarity_valid\t	: %d\n", sysconfig->module_feature.fem_polarity_valid);
			printf("   external_pa_valid\t: %d\n", sysconfig->module_feature.external_pa_exists);
			printf("   max_txgain_valid\t: %d\n", sysconfig->module_feature.max_txgain_valid);
			printf("   max_txpwr_valid\t   : %d\n", sysconfig->module_feature.max_txpwr_valid);
			printf(" txpwr_boosting\t: %d\n", sysconfig->txpwr_boosting.tmx_gmrc);
			printf(" max_txgain\t: %d\n", sysconfig->max_txgain);
			printf(" max_txpwr\t: %d\n", sysconfig->max_txpwr);
			printf(" fem_polarity\t: \"0x%02x\"\n", sysconfig->fem_polarity);
			printf(" gpio_index_map\t:\n");
			printf("   pa_en_valid\t  : %d\n", sysconfig->gpio_index_map.pa_en_valid);
			printf("   pa_en_pin\t  : %d\n", sysconfig->gpio_index_map.pa_en_pin);
			printf("   ant_sel_valid\t: %d\n", sysconfig->gpio_index_map.ant_sel_valid);
			printf("   ant_sel_pin\t  : %d\n", sysconfig->gpio_index_map.ant_sel_pin);
			printf("   power_down_valid\t: %d\n", sysconfig->gpio_index_map.power_down_valid);
			printf("   power_down_data\t: %d\n", sysconfig->gpio_index_map.power_down_data);
			printf("   power_down_pin\t: %d\n\n",  sysconfig->gpio_index_map.power_down_pin);
			printf(" serial_number\t: ");
			for(int i=0;i<sizeof(sysconfig->serial_number);i++) {
				char c = sysconfig->serial_number[i];
				if (!c)
					break;
				printf("%c", c);
			}
			printf("\n");
			printf(" user_factory\t: ");
			for(int i=0;i<sizeof(sysconfig->user_factory);i++) {
				char c = sysconfig->user_factory[i];
				if (!c)
					break;
				printf("%c", c);
			}
		}
	}
}

void cli_user_factory_print(xfer_sys_config_t *sysconfig, bool hex, int sysconfig_format)
{
	if (hex) {
		print_hex(sysconfig->user_factory,  sizeof(sysconfig->user_factory));
	} else {
		for(int i = 0; i < sizeof(sysconfig->user_factory); i++) {
			char c = sysconfig->user_factory[i];
			if(IS_WHITESPACE(c) || IS_PRINTABLE_ASCII(c))
				printf("%c", c);
		}
		printf("\n");
	}
}

void cmd_show_sysconfig_parse(xfer_sys_config_t *sysconfig, int display_mode, int sysconfig_format)
{
	printf("[sysconfig]\n");
	cli_sysconfig_print(sysconfig, display_mode, sysconfig_format);
	printf("\n\n");
	printf("[user_factory]\n");
	cli_user_factory_print(sysconfig, display_mode, sysconfig_format);
	printf("\n");
}


#endif /* _CLI_UTIL_ */
