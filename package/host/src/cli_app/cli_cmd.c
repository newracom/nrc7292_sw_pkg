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

#ifndef _CLI_CMD_
#define _CLI_CMD_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "cli_cmd.h"
#include "cli_netlink.h"
#include "cli_util.h"
#include "cli_key_list.h"

/*******************************************************************************
* Main commands
*******************************************************************************/
static int cmd_help(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_memory(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_test(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_gpio(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_exit(cmd_tbl_t *t, int argc, char *argv[]);

/*******************************************************************************
* system commands
*******************************************************************************/
static int cmd_start_ap(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_stop_ap(cmd_tbl_t *t, int argc, char *argv[]);

/*******************************************************************************
* sub commands on show
*******************************************************************************/
/* 1st sub commands on show */
static int cmd_show_version(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_config(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_stats(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_edca(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_umac_info(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_ampdu(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_mac(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_signal(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_maxagg(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_duty(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_autotxgain(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_cal_use(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_bdf_use(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_recovery(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_detection(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_temperature(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_wakeup_pin(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_wakeup_source(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_sta(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_cli_app_list_version(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_tx_time(cmd_tbl_t *t, int argc, char *argv[]);

/* 2nd sub commands on show */
static int cmd_show_stats_simple_rx(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_mac_tx(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_mac_rx(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_mac_clear(cmd_tbl_t *t, int argc, char *argv[]);

/* 3nd sub commands on show */
static int cmd_show_mac_stats(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_show_mac_clear(cmd_tbl_t *t, int argc, char *argv[]);

/* results display for show mac [tx|rx] stats */
static void cmd_show_mac_result_display(char *response, int dir, int type);

/*******************************************************************************
* sub commands on set
*******************************************************************************/
/* 1st sub commands on set */
static int cmd_set_guard_interval(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_max_aggregation(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_config(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_rate_control(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_duty(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_cal_use(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_bdf_use(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_txpwr(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_wakeup_pin(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_wakeup_source(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_addba(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_delba(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_rts(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_tx_time(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_drop_frame(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_temp_sensor(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_set_self_configuration(cmd_tbl_t *t, int argc, char *argv[]);

/*******************************************************************************
* sub commands on test
*******************************************************************************/
/* 1st sub commands on set */
static int cmd_test_mcs(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_test_country(cmd_tbl_t *t, int argc, char *argv[]);

/*******************************************************************************
* sub commands on gpio
*******************************************************************************/
/* 1st sub commands on set */
static int cmd_gpio_read(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_gpio_write(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_gpio_direction(cmd_tbl_t *t, int argc, char *argv[]);
static int cmd_gpio_pullup(cmd_tbl_t *t, int argc, char *argv[]);

/*******************************************************************************
* function for periodic show signal command
*******************************************************************************/
void *showRxThreadRun();

/*******************************************************************************
* defines
*******************************************************************************/
#define STATS_TYPES_MAX 4
#define STATS_STATUS_MAX 2
#define STATS_MAX_AC 6
#define STATS_MCS_MAX 9

enum {
	DIR_TX 	= 0,
	DIR_RX
};

#define MAX_CONECTION_NUM 1024

/*******************************************************************************
* variables
*******************************************************************************/
pthread_t showRxThread;
static int show_rx_force_stop ;
static int rssi_sum[MAX_CONECTION_NUM] ,snr_sum[MAX_CONECTION_NUM];
static int rssi_sum_sqrs[MAX_CONECTION_NUM] ,snr_sum_sqrs[MAX_CONECTION_NUM];
static char show_signal_mac_addr[MAX_CONECTION_NUM][MAX_ADDR_SIZE];

static int count[MAX_CONECTION_NUM];
static int show_signal_periodic_timer;
static int show_signal_try_number;
const char response_timeout_str[] = "Failed";
const char no_self_conf_str[] = "no_self_conf";

/*******************************************************************************
* command list
*******************************************************************************/
/* main command list */
cmd_tbl_t cli_list[] = {
	{ "help", cmd_help, "show CLI tree", "help",  "", 0},
	{ "read", cmd_memory, "read memory", "read <address> <size in byte>",  "", 1},
	{ "write", cmd_memory, "write a 32-bit value to memory", "write <address> <data>",	"", 0},
	{ "show", cmd_show, "show information", "show subcommand [value1]",  "", 1},
	{ "set", cmd_set, "set command","set subcommand [value1]",  "", 1},
	{ "test", cmd_test, "test command","test subcommand [value1]",  "", 1},
	{ "gpio", cmd_gpio, "gpio command","gpio subcommand [value1]",  "", 1},
	{ "exit", cmd_exit, "exit program", "exit",  "", 0},
};

/* sub command list on show */
cmd_tbl_t show_sub_list[] = {
	{ "version", cmd_show_version, "show version","show version",  SHOW_VERSION_KEY_LIST, 0},
	{ "config", cmd_show_config, "show configuration", "show config [vif_id]", SHOW_CONFIG_KEY_LIST, 0},
	{ "stats", cmd_show_stats, "show QM status", "show stats subcommand",  "", 1},
	{ "edca", cmd_show_edca, "show EDCA parameters", "show edca",  SHOW_EDCA_KEY_LIST, 0},
	{ "uinfo", cmd_show_umac_info, "show UMAC information", "show uinfo [vif_id]",  "", 0},
	{ "ampdu", cmd_show_ampdu, "show/clear AMPDU count", "show ampdu | show ampdu clear",  "", 0},
	{ "mac", cmd_show_mac, "mac command", "show mac {tx|rx|clear} {stats|clear}",  "", 1},
	{ "signal", cmd_show_signal, "show rssi/snr", "show signal {start|stop} [interval] [number]",  SHOW_SIGNAL_KEY_LIST, 0},
	{ "maxagg", cmd_show_maxagg, "show max aggregation", "show maxagg",  "", 0},
#if defined(INCLUDE_DUTYCYCLE) && (INCLUDE_DUTYCYCLE == 1)
	{ "duty", cmd_show_duty, "show duty cycle", "show duty",  SHOW_DUTY_KEY_LIST, 0},
#endif
	{ "autotxgain", cmd_show_autotxgain, "show autotxgain", "show autotxgain",  SHOW_AUTOTXGAIN_KEY_LIST, 0},
	{ "cal_use", cmd_show_cal_use, "show cal_use", "show cal_use",  SHOW_CAL_USE_KEY_LIST, 0},
	{ "bdf_use", cmd_show_bdf_use, "show board data use", "show bdf_use",  SHOW_BDF_USE_KEY_LIST, 0},
	{ "recovery", cmd_show_recovery, "show recovery", "show recovery stats",  "", 0},
	{ "detection", cmd_show_detection, "show detection", "show detection stats",  "", 0},
	{ "temp", cmd_show_temperature, "show temp","show temp",  SHOW_TEMPERATURE_KEY_LIST, 0},
	{ "wakeup_pin", cmd_show_wakeup_pin, "show wakeup pin configuration","show wakeup_pin",  SHOW_WAKEUP_PIN_KEY_LIST, 0},
	{ "wakeup_source", cmd_show_wakeup_source, "show wakeup source configuration","show wakeup_source", SHOW_WAKEUP_SOURCE_KEY_LIST, 0},
	{ "sta", cmd_show_sta, "show station information", "show sta [vif_id] {all|aid [aid_index]}",  "", 0},
	{ "tx_time", cmd_show_tx_time, "show tx_time about <CS time> <Blank time>", "show tx_time", SHOW_TX_TIME_KEY_LIST, 0},
	{ "cli_app_list_version", cmd_show_cli_app_list_version, "show cli app list version", "show cli_app_list_version",  "", 1},
};

/* sub command list on set */
cmd_tbl_t set_sub_list[] = {
	{ "gi", cmd_set_guard_interval, "set guard interval", "set gi {short|long|auto}", "", 0},
	{ "maxagg", cmd_set_max_aggregation, "set aggregation", "set maxagg {AC(0-3)} <Max(0-13,0:off)> {size:default=0}", SET_MAXAGG_KEY_LIST, 0},
	{ "config", cmd_set_config, "set ack, aggregation, mcs", "set config {ack(0,1)} {agg(0,1)} [mcs]", SET_CONFIG_KEY_LIST, 0},
	{ "rc", cmd_set_rate_control, "set rate control", "set rc {on|off} [vif_id] [mode]", SET_RC_KEY_LIST, 0},
#if defined(INCLUDE_DUTYCYCLE) && (INCLUDE_DUTYCYCLE == 1)
	{ "duty", cmd_set_duty, "set duty cycle", "set duty {on|off} {duty window} {tx duration} {duty margin}", SET_DUTY_KEY_LIST, 0},
#endif
	{ "cal_use", cmd_set_cal_use, "set cal_use", "set cal_use {on|off}", SET_CAL_USE_KEY_LIST, 0},
	{ "bdf_use", cmd_set_bdf_use, "set board data use", "set bdf_use {on|off}", SET_BDF_USE_KEY_LIST, 0},
	{ "txpwr", cmd_set_txpwr, "set txpwrt", "set txpwr {value(1~30)}", SET_TXPWR_KEY_LIST, 0},
	{ "wakeup_pin", cmd_set_wakeup_pin, "set wakeup pin for deepsleep", "set wakeup_pin {Debounce(on|off)} {PIN Number(0~31)}", SET_WAKEUP_PIN_KEY_LIST, 0},
	{ "wakeup_source", cmd_set_wakeup_source, "set wakeup source for deepsleep", "set wakeup_soruce rtc gpio hspi", SET_WAKEUP_SOURCE_KEY_LIST, 0},
	{ "addba", cmd_set_addba, "set addba tid / send addba with mac address", "set addba [tid] {mac address}", "", 0},
	{ "delba", cmd_set_delba, "set delba tid / send delba with mac address", "set delba [tid] {mac address}", "", 0},
	{ "rts", cmd_set_rts, "set rts on/off", "set rts {on|off|default} <threshold> <vif_id>", "", 0},
	{ "tx_time", cmd_set_tx_time, "set tx_time about <CS time> <Blank time>", "set tx_time <CS time> <Blank time>", SET_TXTIME_KEY_LIST, 0},
	{ "drop", cmd_set_drop_frame, "set drop frames from configured mac address", "set drop [vif id] [mac address] {on|off}", SET_DROP_KEY_LIST, 0},
	{ "tsensor", cmd_set_temp_sensor, "set temperature sensor scl, sda", "set tsensor [GPIO for SCL] [GPIO for SDA]", "", 0},
	{ "self_config", cmd_set_self_configuration, "set self_config", "set self_config {Country(KR,US...)}{BW}{dwell time}", "", 0},
};

/* sub command list on test */
cmd_tbl_t test_sub_list[] = {
	{ "mcs", cmd_test_mcs, "set mcs", "test mcs [mcs index]", "", 0},
#if defined(INCLUDE_AH_JPPC) && (INCLUDE_AH_JPPC == 1)
	{ "country", cmd_test_country, "set/show tx time control for JP(Japan)", \
		"test country JP <CS time> <Blank time> | test country JP show", TEST_COUNTRY_KEY_LIST, 0},
#endif
};

/* sub command list on gpio */
cmd_tbl_t gpio_sub_list[] = {
	{ "read", cmd_gpio_read, "gpio read", "gpio read [pin number]", "", 0},
	{ "write", cmd_gpio_write, "gpio write", "gpio write [pin number] [0|1]", "", 0},
	{ "direction", cmd_gpio_direction, "read/write gpio direction", "gpio direction [pin index] {[0(input)|1(output)]}", "", 0},
	{ "pullup", cmd_gpio_pullup, "read/write gpio pullup enable|disable", "gpio pullup [pin index] {[0(off)|1(on)]}", "", 0}
};

/* 2rd sub command list on show stats */
cmd_tbl_t show_stats_sub_list[] = {
	{ "simple_rx", cmd_show_stats_simple_rx, "show received packet information", "show stats simple_rx",  SHOW_STATS_SIMPLE_RX_KEY_LIST, 0}
};

/* 2rd sub command list on show mac */
cmd_tbl_t show_mac_sub_list[] = {
	{ "tx", cmd_show_mac_tx, "show TX Statistics", "show mac tx {stats|clear}",  "", 1},
	{ "rx", cmd_show_mac_rx, "show RX Statistics", "show mac rx {stats|clear}",  "", 1},
	{ "clear", cmd_show_mac_clear, "clear TX/RX Statistics", "show mac clear",  "", 0},
};

/* 3rd sub command list on show mac tx*/
cmd_tbl_t show_mac_tx_sub_list[] = {
	{ "stats", cmd_show_mac_stats, "show TX Statistics", "show mac tx stats",  "", 0},
	{ "clear", cmd_show_mac_clear, "clear TX Statistics", "show mac tx clear",  "", 0},
};

/* 3rd sub command list on show mac rx*/
cmd_tbl_t show_mac_rx_sub_list[] = {
	{ "stats", cmd_show_mac_stats, "show RX Statistics", "show mac rx stats",  "", 0},
	{ "clear", cmd_show_mac_clear, "clear RX Statistics", "show mac rx clear",  "", 0},
};

/* system command list*/
cmd_tbl_t sys_cmd_list[] = {
	{ "start-ap", cmd_start_ap,	"", "  ",  "", 0},
	{ "stop-ap", cmd_stop_ap,	"", " ",  "", 0},
};

/*******************************************************************************
* function for getting command list
*******************************************************************************/
cmd_tbl_t * get_cmd_list(enum cmd_list_type type, int *list_size, int *list_depth)
{
	cmd_tbl_t *ret;
	switch(type){
		case MAIN_CMD:
			ret = cli_list;
			*list_size = sizeof(cli_list)/sizeof(cmd_tbl_t);
			*list_depth = 0;
			break;
		case SYS_CMD:
			ret= sys_cmd_list;
			*list_size = sizeof(sys_cmd_list)/sizeof(cmd_tbl_t);
			*list_depth = 0;
			break;
		case SHOW_SUB_CMD:
			ret = show_sub_list;
			*list_size = sizeof(show_sub_list)/sizeof(cmd_tbl_t);
			*list_depth = 1;
			break;
		case SHOW_STATS_SUB_CMD:
			ret= show_stats_sub_list;
			*list_size = sizeof(show_stats_sub_list)/sizeof(cmd_tbl_t);
			*list_depth = 2;
			break;
		case SHWO_MAC_SUB_CMD:
			ret = show_mac_sub_list;
			*list_size = sizeof(show_mac_sub_list)/sizeof(cmd_tbl_t);
			*list_depth = 2;
			break;
		case SHOW_MAC_TX_SUB_CMD:
			ret= show_mac_tx_sub_list;
			*list_size = sizeof(show_mac_tx_sub_list)/sizeof(cmd_tbl_t);
			*list_depth = 3;
			break;
		case SHOW_MAC_RX_SUB_CMD:
			ret = show_mac_rx_sub_list;
			*list_size = sizeof(show_mac_rx_sub_list)/sizeof(cmd_tbl_t);
			*list_depth = 3;
			break;
		case SET_SUB_CMD:
			ret= set_sub_list;
			*list_size = sizeof(set_sub_list)/sizeof(cmd_tbl_t);
			*list_depth = 1;
			break;
		case TEST_SUB_CMD:
			ret= test_sub_list;
			*list_size = sizeof(test_sub_list)/sizeof(cmd_tbl_t);
			*list_depth = 1;
			break;
		case GPIO_SUB_CMD:
			ret= gpio_sub_list;
			*list_size = sizeof(gpio_sub_list)/sizeof(cmd_tbl_t);
			*list_depth = 1;
			break;
		default :
			ret = cli_list;
			*list_size = sizeof(cli_list)/sizeof(cmd_tbl_t);
			*list_depth = 0;
	}
	return ret;
}

/*******************************************************************************
* system command
*******************************************************************************/
static int cmd_start_ap(cmd_tbl_t *t, int argc, char *argv[])
{
	char cli_str[NRC_MAX_CMDLINE_SIZE];
	sprintf(cli_str, "start-ap");
	system(cli_str);
	printf("%s : %s [Not Implemented]\n", __func__, t->name);
	return CMD_RET_SUCCESS;
}

static int cmd_stop_ap(cmd_tbl_t *t, int argc, char *argv[])
{
	char cli_str[NRC_MAX_CMDLINE_SIZE];
	sprintf(cli_str, "stop-ap");
	system(cli_str);
	printf("%s : %s [Not Implemented]\n", __func__, t->name);
	return CMD_RET_SUCCESS;
}

/*******************************************************************************
* help command
*******************************************************************************/
static int cmd_help(cmd_tbl_t *t, int argc, char *argv[])
{
	int print_line_len = 100;
	print_line('=', print_line_len,"", 0,0);
	cmd_list_display(MAIN_CMD);
	cmd_list_display(SHOW_SUB_CMD);
	cmd_list_display(SHOW_STATS_SUB_CMD);
	cmd_list_display(SHWO_MAC_SUB_CMD);
	cmd_list_display(SHOW_MAC_TX_SUB_CMD);
	cmd_list_display(SHOW_MAC_RX_SUB_CMD);
	cmd_list_display(SET_SUB_CMD);
	cmd_list_display(TEST_SUB_CMD);
	cmd_list_display(GPIO_SUB_CMD);
	print_line('=', print_line_len,"", 0,0);
	return CMD_RET_SUCCESS;
}

/*******************************************************************************
* memory command
*******************************************************************************/
static int cmd_memory(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int print_line_len = 20;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	if(strcmp("read", argv[0]) == 0){
		sprintf(param, "read %s %s -sr", argv[1], argv[2]);
	}else if(strcmp("write", argv[0]) == 0){
		sprintf(param, "write %s %s", argv[1], argv[2]);
	}else{
		return CMD_RET_FAILURE;
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			print_line('-', print_line_len,"", 0,0);
			print_line(' ', print_line_len,"Address :  Value ", 0,0);
			print_line('-', print_line_len,"", 0,0);
			printf("%s",response);
			print_line('-', print_line_len,"", 0,0);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

/*******************************************************************************
* show command
*******************************************************************************/
static int cmd_show(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	int sub_cmd_list_size, sub_cmd_list_depth;
	cmd_tbl_t * sub_cmd_list = get_cmd_list(SHOW_SUB_CMD, &sub_cmd_list_size, &sub_cmd_list_depth);

	if(argc == sub_cmd_list_depth){
		printf("There is no sub command. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}
	ret = run_sub_cmd(t, argc, argv, sub_cmd_list, sub_cmd_list_size, sub_cmd_list_depth);
	return ret;
}

/*******************************************************************************
* set command
*******************************************************************************/
static int cmd_set(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	int sub_cmd_list_size, sub_cmd_list_depth;
	cmd_tbl_t * sub_cmd_list = get_cmd_list(SET_SUB_CMD, &sub_cmd_list_size, &sub_cmd_list_depth);

	if(argc == sub_cmd_list_depth){
		printf("There is no sub command. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}
	ret = run_sub_cmd(t, argc, argv, sub_cmd_list, sub_cmd_list_size, sub_cmd_list_depth);
	return ret;
}

/*******************************************************************************
* test command
*******************************************************************************/
static int cmd_test(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	int sub_cmd_list_size, sub_cmd_list_depth;
	cmd_tbl_t * sub_cmd_list = get_cmd_list(TEST_SUB_CMD, &sub_cmd_list_size, &sub_cmd_list_depth);

	if(argc == sub_cmd_list_depth){
		printf("There is no sub command. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}
	ret = run_sub_cmd(t, argc, argv, sub_cmd_list, sub_cmd_list_size, sub_cmd_list_depth);
	return ret;
}

/*******************************************************************************
* gpio command
*******************************************************************************/
static int cmd_gpio(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	int sub_cmd_list_size, sub_cmd_list_depth;
	cmd_tbl_t * sub_cmd_list = get_cmd_list(GPIO_SUB_CMD, &sub_cmd_list_size, &sub_cmd_list_depth);

	if(argc == sub_cmd_list_depth){
		printf("There is no sub command. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}
	ret = run_sub_cmd(t, argc, argv, sub_cmd_list, sub_cmd_list_size, sub_cmd_list_depth);
	return ret;
}

/*******************************************************************************
* exit program
*******************************************************************************/
static int cmd_exit(cmd_tbl_t *t, int argc, char *argv[])
{
	int i=0;
	if(showRxThread){
		while(1){
			if(count[i] == 0){
				break;
			}else{
				signal_log_display(show_signal_mac_addr[i], snr_sum[i],
					snr_sum_sqrs[i], rssi_sum[i], rssi_sum_sqrs[i], count[i]);
			}
			i++;
		}
		show_rx_force_stop = 1;
		cli_delay_ms(COMMAND_DELAY_MS);	// 50 ms
	}
	return CMD_RET_EXIT_PROGRAM;
}

/*******************************************************************************
* sub commands on show
*******************************************************************************/
static int cmd_show_version(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show version -sr");

	netlink_ret= netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_config(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;
	int print_line_len = DEFAULT_PRINT_LINE_LEN;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if(argv[2] == NULL){
		sprintf(param, "show config %d -sr", 0);
	}else if(atoi(argv[2]) == 0 || atoi(argv[2]) == 1){
		sprintf(param, "show config %s -sr", argv[2]);
	}else {
		printf("invalid value %s : (ex) show config [vif_id]\n", argv[2]);
		return CMD_RET_FAILURE;
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			print_line('-', print_line_len,"", 0,0);
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			print_line('-', print_line_len,"", 0,0);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}

	return ret;
}

static int cmd_show_stats(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	int sub_cmd_list_size, sub_cmd_list_depth;
	cmd_tbl_t * sub_cmd_list = get_cmd_list(SHOW_STATS_SUB_CMD, &sub_cmd_list_size, &sub_cmd_list_depth);

	if(argc == sub_cmd_list_depth){
		printf("There is no sub command. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}
	ret = run_sub_cmd(t, argc, argv, sub_cmd_list, sub_cmd_list_size, sub_cmd_list_depth);
	return ret;
}

static int cmd_show_edca(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;
	int print_line_len = DEFAULT_PRINT_LINE_LEN;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show edca -sr");

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			print_line('-', print_line_len,"", 0,0);
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			print_line('-', print_line_len,"", 0,0);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_umac_info(cmd_tbl_t *t, int argc, char *argv[])
{
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int print_line_len = 130;
	int ret = CMD_RET_FAILURE;
	int start_point = 0;
	int next_sta_index = 0;

	if(argc < 3){
		printf("There is invalid parameter. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}

	if(atoi(argv[2]) != 0 && atoi(argv[2]) != 1){
		printf("invalid value %s : (ex) show uinfo [vif_id]\n", argv[2]);
		return CMD_RET_FAILURE;
	}

	/* For AP Information */
	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	sprintf(param, "show uinfo %s ap -sr", argv[2]);
	netlink_send_data_with_retry(NL_SHELL_RUN_RAW, param, response, -1);
	print_line('-', print_line_len,"|* AP INFO *|", 0,0);
	cmd_umac_info_result_parse(response, AP_UMAC_INFO, &next_sta_index);
	ret = CMD_RET_SUCCESS;

	if(ret != CMD_RET_SUCCESS)
		return ret;

	/* For STA Information */
	print_line('-', print_line_len,"|* STA INFO *|", 0,0);
	int sta_remaining = 1;
	while(sta_remaining)
	{
		memset(param, 0x0, sizeof(param));
		sprintf(param, "show uinfo %s sta %d -sr", argv[2], next_sta_index);
		netlink_send_data_with_retry(NL_SHELL_RUN_RAW, param, response, -1);
		sta_remaining = cmd_umac_info_result_parse(response, STA_UMAC_INFO, &next_sta_index);
	}
	ret = CMD_RET_SUCCESS;
	print_line('-', print_line_len,"", 0,0);
	return ret;
}

static int cmd_show_ampdu(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	int print_line_len = 20;

	if( argv[2]!= NULL && strcmp("clear", argv[2]) == 0){
		strcpy(param, "show ampdu clear -sr");
	}else{
		strcpy(param, "show ampdu -sr");
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if( argv[2]!= NULL && strcmp("clear", argv[2]) == 0){
			printf("%s\n",response);
			ret = CMD_RET_SUCCESS;
		}else{
			if(strcmp(response, response_timeout_str)== 0){
				ret =  CMD_RET_RESPONSE_TIMEOUT;
			}else{
				print_line('-', print_line_len,"", 0,0);
				print_line(' ', print_line_len,"AMPDU\t: Value ", 0,0);
				print_line('-', print_line_len,"", 0,0);
				printf("%s",response);
				print_line('-', print_line_len,"", 0,0);
				ret = CMD_RET_SUCCESS;
			}
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_mac(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	int sub_cmd_list_size, sub_cmd_list_depth;
	cmd_tbl_t * sub_cmd_list = get_cmd_list(SHWO_MAC_SUB_CMD, &sub_cmd_list_size, &sub_cmd_list_depth);

	if(argc == sub_cmd_list_depth){
		printf("There is no sub command. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}
	ret = run_sub_cmd(t, argc, argv, sub_cmd_list, sub_cmd_list_size, sub_cmd_list_depth);
	return ret;
}

static int cmd_show_signal(cmd_tbl_t *t, int argc, char *argv[])
{
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int ret = CMD_RET_SUCCESS;
	int display_per_line = 3;
	int threadErr = 0;

	int total_sta_number = 0;
	int iteration_number = 0;
	int max_response_number = 0;
	char *key_list_temp = NULL;
	char *next_ptr = NULL;
	char * t1;
	int start_point = 0;
	int i = 0;
	int nTry = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	key_list_temp = malloc(DISP_CMD_RESULT_BUF);
	memset(key_list_temp, 0x0, DISP_CMD_RESULT_BUF);

	if(!argv[2]){
		memset(param, 0x0, sizeof(param));
		strcpy(param, "show signal -sr -num");

		nTry = 100;
		while(nTry--){
			netlink_ret = netlink_send_data(NL_CLI_APP_GET_INFO, param, response);
			if(!netlink_ret){
				ret = CMD_RET_SUCCESS;
				break;
			}else{
				ret = CMD_RET_FAILURE;
				cli_delay_ms(COMMAND_DELAY_MS);
			}
		}

		memcpy(key_list_temp, response, strlen(response));
		t1 =  strtok_r(key_list_temp, ",", &next_ptr);
		if(!t1) {
			printf("N/A\n");
		}else {
			total_sta_number = atoi(t1);
			t1 =  strtok_r(NULL, ",", &next_ptr);
			max_response_number = atoi(t1);
			if(total_sta_number == 0){
				iteration_number = 0;
			}else{
				if((total_sta_number%max_response_number) ==0){
					iteration_number = (total_sta_number/max_response_number);
				}else{
					iteration_number = (total_sta_number/max_response_number)+1;
				}
			}
		}

		if(key_list_temp)
			free(key_list_temp);

		for(i=0; i<iteration_number; i++){
			memset(param, 0x0, sizeof(param));
			sprintf(param, "show signal -sr %d", start_point);
			netlink_ret = netlink_send_data(NL_CLI_APP_GET_INFO, param, response);

			if(strcmp(response, response_timeout_str)== 0){
				ret =  CMD_RET_RESPONSE_TIMEOUT;
				break;
			}else{
				if(strlen(response) == 0 && iteration_number == 1){
					printf("N/A\n");
				}else{
					cmd_result_parse((char*)t->key_list, response, display_per_line);
					start_point +=  max_response_number;
				}
				ret=  CMD_RET_SUCCESS;
			}
		}
	}else{
		if(strncmp("start", argv[2], 5)  == 0){
			if(!showRxThread){
				if(argv[3]  != NULL){
					show_signal_periodic_timer = atoi(argv[3]);
				}else{
					show_signal_periodic_timer = SHOW_RX_DISPLAY_DEFAULT_INTERVAL;
				}

				if(argv[4]  != NULL){
					show_signal_try_number = atoi(argv[4]);
				}else{
					show_signal_try_number = 0; /* 0 means infinite */
				}

				threadErr = pthread_create(&showRxThread,NULL,showRxThreadRun,(cmd_tbl_t *)t);
				if(threadErr){
					printf("Thread Err = %d",threadErr);
					ret = CMD_RET_FAILURE;
				}else{
					ret = CMD_RET_SUCCESS;
				}
			}else{
				printf("show signal start already running!!\n");
				ret = CMD_RET_FAILURE;
			}
		}else if(strncmp("stop", argv[2], 4)  == 0){
			if(!showRxThread){
				printf("show signal start not running!!\n");
				ret = CMD_RET_FAILURE;
			}else{
				memset(param, 0x0, sizeof(param));
				sprintf(param, "show signal stop");
				netlink_ret = netlink_send_data(NL_CLI_APP_GET_INFO, param, response);
				if(!netlink_ret){
					ret = CMD_RET_SUCCESS;
				}else{
					ret = CMD_RET_FAILURE;
					cli_delay_ms(COMMAND_DELAY_MS);
				}

				show_rx_force_stop = 1;
				while(show_rx_force_stop){
					sleep(1);
				}
				ret =  CMD_RET_SUCCESS;
			}
		}else{
			printf("There is invalid parameter. Please see the help.\n");
			cmd_help(NULL, 0, NULL);
			ret = CMD_RET_FAILURE;
		}
	}
	return ret;
}

static int cmd_show_maxagg(cmd_tbl_t *t, int argc, char *argv[])
{
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int print_line_len = 130;
	int ret = CMD_RET_FAILURE;
	int start_point = 0;
	int next_sta_index = 0;

	int sta_remaining = 1;
	while(sta_remaining)
	{
		memset(param, 0x0, sizeof(param));
		sprintf(param, "show maxagg %d -sr",  next_sta_index);
		netlink_send_data_with_retry(NL_SHELL_RUN_RAW, param, response, -1);
		sta_remaining = cmd_show_maxagg_result_parse(response, &next_sta_index);
	}
	ret = CMD_RET_SUCCESS;
	return ret;
}

static int cmd_show_duty(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show duty -sr");
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_autotxgain(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show autotxgain -sr");
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_cal_use(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 2;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show cal_use -sr");
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_bdf_use(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show bdf_use -sr");
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_recovery(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	char *key_list_temp = NULL;
	char *next_ptr = NULL;
	char * t1;
	int qm_num, missing_count, max_diff;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show recovery stats -sr");
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			key_list_temp = malloc(DISP_CMD_RESULT_BUF);
			memset(key_list_temp, 0x0, DISP_CMD_RESULT_BUF);
			if(strlen(response)){
				memcpy(key_list_temp, response, strlen(response));
			}

			t1 =  strtok_r(key_list_temp, ",", &next_ptr);
			if(!t1) {
				printf("N/A\n");
			}else {
				print_line('-', 50,"", 0,0);
				printf("Number of Recovery Count : %s\n", t1);
				t1 =  strtok_r(NULL, ",", &next_ptr);
				if(t1 != NULL) {
					printf("Number of RX Frame regarding RX Buffer discard : %s\n", t1);
					printf("SN missing by QM\n");
					t1 =  strtok_r(NULL, ",", &next_ptr);
					if(t1 != NULL){
						print_line('-', 50,"", 0,0);
						printf("QM[#]	 Missing Count		Max diff\n");
						print_line('-', 50,"", 0,0);
					}

					while(t1 != NULL){
						qm_num = atoi(t1);
						t1 =  strtok_r(NULL, ",", &next_ptr);
						missing_count = atoi(t1);
						t1 =  strtok_r(NULL, ",", &next_ptr);
						max_diff = atoi(t1);
						t1 =  strtok_r(NULL, ",", &next_ptr);
						printf("QM[%d] %9d\t\t%9d\n", qm_num, missing_count, max_diff);
					}
				}
				print_line('-', 50,"", 0,0);
			}
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}

	if(key_list_temp)
		free(key_list_temp);

	return ret;
}

static int cmd_show_detection(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	char *key_list_temp = NULL;
	char *next_ptr = NULL;
	char * t1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show detection stats -sr");
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			key_list_temp = malloc(DISP_CMD_RESULT_BUF);
			memset(key_list_temp, 0x0, DISP_CMD_RESULT_BUF);
			if(strlen(response)){
				memcpy(key_list_temp, response, strlen(response));
			}

			t1 =  strtok_r(key_list_temp, ",", &next_ptr);
			if(!t1) {
				printf("N/A\n");
			}else {
				print_line('-', 50,"", 0,0);
				printf("Tx Triggered Detection Count : %s\n", t1);
				t1 =  strtok_r(NULL, ",", &next_ptr);
				printf("Rx Triggered Detection Count : %s\n", t1);
				t1 =  strtok_r(NULL, ",", &next_ptr);
				if(t1 != NULL) {
					print_line('-', 50,"", 0,0);
					printf(" --Non-zero Length NDP:  %s\n", t1);
					t1 =  strtok_r(NULL, ",", &next_ptr);
					printf(" --IP Length:            %s\n", t1);
					t1 =  strtok_r(NULL, ",", &next_ptr);
					printf(" --MPDU Length Mismatch: %s\n", t1);
					t1 =  strtok_r(NULL, ",", &next_ptr);
					printf(" --Buffer Mismatch:      %s\n", t1);
					t1 =  strtok_r(NULL, ",", &next_ptr);
					printf(" --MPDU Length Size:     %s\n", t1);
				}
				print_line('-', 50,"", 0,0);
			}
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}

	if(key_list_temp)
		free(key_list_temp);

	return ret;
}

static int cmd_show_temperature(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show temp -sr");

	netlink_ret= netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_wakeup_pin(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 2;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show wakeup_pin -sr");

	netlink_ret= netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_wakeup_source(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show wakeup_source -sr");

	netlink_ret= netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_sta(cmd_tbl_t *t, int argc, char *argv[])
{
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int print_line_len = 130;
	int nTry = 0;
	int ret = CMD_RET_FAILURE;
	int total_sta_number = 0;
	int max_response_number = 0;
	char *key_list_temp = NULL;
	char *next_ptr = NULL;
	char * t1;
	int start_point = 0;
	int remain = 0;
	int request_number = 0;
	int show_duplicate_aid_count = 0;
	int duplicate_aid_count = 0;
	int i = 0;
	key_list_temp = malloc(DISP_CMD_RESULT_BUF);
	memset(key_list_temp, 0x0, DISP_CMD_RESULT_BUF);

	if(argc < 4)
	{
		printf("There is invalid parameter. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}

	if(atoi(argv[2]) != 0 && atoi(argv[2]) != 1)
	{
		printf("invalid value %s : (ex) show uinfo [vif_id]\n", argv[2]);
		return CMD_RET_FAILURE;
	}

	/* Get STA number and max_response number */
	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);


	printf("STATION    MAC_ADDR             AID    STATE\n");
	printf("==============================================\n");

	int next_sta_index = 0;
	if(argc == 5 && strcmp(argv[3],"aid")==0)
	{
		memset(param, 0x0, sizeof(param));
		sprintf(param, "show sta %s aid %s -sr", argv[2], argv[4]);
		netlink_send_data_with_retry(NL_SHELL_RUN_RAW, param, response, -1);
		cmd_sta_umac_info_mini_result_parse(response, &next_sta_index, NULL);
		ret = CMD_RET_SUCCESS;
	}
	else if(argc == 4 && strcmp(argv[3],"all")==0)
	{
		show_duplicate_aid_count = 1;
		int sta_remaining = 1;
		int aid_max_index = 2047;
		int aid_count[aid_max_index + 1];
		memset(aid_count, 0, sizeof(aid_count));
		while(sta_remaining)
		{
			memset(param, 0x0, sizeof(param));
			sprintf(param, "show sta %s all %d -sr", argv[2], next_sta_index);
			netlink_send_data_with_retry(NL_SHELL_RUN_RAW, param, response, -1);
			sta_remaining = cmd_sta_umac_info_mini_result_parse(response, &next_sta_index, aid_count);
		}
		for(int aid=1;aid<=aid_max_index;aid++)
			if(aid_count[aid] > 1)
				duplicate_aid_count++;
		ret = CMD_RET_SUCCESS;
	}
	else
	{
		printf("There is invalid parameter. Please see the help.\n");
		ret = CMD_RET_FAILURE;
	}

	if(show_duplicate_aid_count)
		printf("Duplicate AID count : %d\n", duplicate_aid_count);

	printf("==============================================\n");
	return ret;
}

static int cmd_show_tx_time(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	sprintf(param, "test tx_time show -sr");
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_cli_app_list_version(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	char snum[10];

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show cli_app_list_version -sr");

	memset(snum, 0x0, sizeof(snum));
	sprintf(snum, "%d", CLI_APP_LIST_VERSION);

	netlink_ret= netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){

		if(!strcmp(response,snum)){
			ret = CMD_RET_SUCCESS;
		}else{
			if(strcmp(response, response_timeout_str)== 0){
				ret =  CMD_RET_RESPONSE_TIMEOUT;
			}else{
				printf("Cli app list version is different. [App:%s,Target:%s]\n", snum, response);
				printf("Please match the version!!!\n");
				ret = CMD_RET_FAILURE;
			}
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_stats_simple_rx(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;
	int print_line_len = DEFAULT_PRINT_LINE_LEN;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));
	strcpy(param, "show stats simple_rx -sr\0");

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			print_line('-', print_line_len,"", 0,0);
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			print_line('-', print_line_len,"", 0,0);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_mac_tx(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	int sub_cmd_list_size, sub_cmd_list_depth;
	cmd_tbl_t * sub_cmd_list = get_cmd_list(SHOW_MAC_TX_SUB_CMD, &sub_cmd_list_size, &sub_cmd_list_depth);

	if(argc == sub_cmd_list_depth){
		printf("There is no sub command. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}
	ret = run_sub_cmd(t, argc, argv, sub_cmd_list, sub_cmd_list_size, sub_cmd_list_depth);
	return ret;
}

static int cmd_show_mac_rx(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	int sub_cmd_list_size, sub_cmd_list_depth;
	cmd_tbl_t * sub_cmd_list = get_cmd_list(SHOW_MAC_RX_SUB_CMD, &sub_cmd_list_size, &sub_cmd_list_depth);

	if(argc == sub_cmd_list_depth){
		printf("There is no sub command. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}
	ret = run_sub_cmd(t, argc, argv, sub_cmd_list, sub_cmd_list_size, sub_cmd_list_depth);
	return ret;
}

static int cmd_show_mac_clear(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;
	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if(strcmp(argv[2], "tx") == 0){
		strcpy(param, "show mac tx clear -sr");
	}else if(strcmp(argv[2], "rx") == 0){
		strcpy(param, "show mac rx clear -sr");
	}else{
		strcpy(param, "show mac clear -sr");
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_show_mac_stats(cmd_tbl_t *t, int argc, char *argv[])
{
	const int request_number = 3;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[request_number][NL_MSG_MAX_RESPONSE_SIZE];
	int ret = CMD_RET_FAILURE;
	int netlink_ret = 0;
	int dir = 0;
	int type = 0;	/* type - 0: AC, 1: Data type, 2: MCS */
	int nTry = 0;
	int response_number = 0;

	for(type=0; type<request_number; type++){
		memset(response[type], 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	}
	memset(param, 0x0, sizeof(param));

	for(type=0; type<request_number; type++){
		memset(param, 0x0, sizeof(param));
		if(strcmp(argv[2], "tx") == 0){
			sprintf(param, "show mac tx stats -sr %d", type);
			dir = DIR_TX;
		}else{
			sprintf(param, "show mac rx stats -sr %d", type);
			dir = DIR_RX;
		}

		nTry = 100;
		while(nTry--){
			netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response[type]);
			if(!netlink_ret){
				if(strcmp(response[type], response_timeout_str)== 0){
					cli_delay_ms(COMMAND_DELAY_MS);
				}else{
					response_number++;
					if(response_number == request_number) {
						ret = CMD_RET_SUCCESS;
					}
					break;
				}
			}else{
				ret = CMD_RET_FAILURE;
				cli_delay_ms(COMMAND_DELAY_MS);
			}
		}
		cli_delay_ms(100);
	}

	if(ret == CMD_RET_SUCCESS){
		for(type=0; type<request_number; type++){
			cmd_show_mac_result_display(response[type], dir, type);
		}
	}

	return ret;
}

static void cmd_show_mac_result_display(char *response, int dir, int type)
{
	char *key_list_temp = NULL;
	char *next_ptr = NULL;
	char * t1;
	int print_line_len =72;
	int i = 0, j=0;

	char temp[5][16] = {0,};
	int mac_stats_start_get_element  = 3;
	int mac_stats_get_element  = 5;

	key_list_temp = malloc(DISP_CMD_RESULT_BUF);
	memcpy(key_list_temp, response, DISP_CMD_RESULT_BUF);

	/* type - 0: AC, 1: Data type, 2: MCS */
	switch(type){
		case 0 :
				print_line('-', print_line_len,"", 0,0);
				printf(" MAC %s Statistics ",(dir == DIR_TX)? "TX":"RX");
				memset(temp, 0x0, sizeof(temp));
				for(i=0; i<mac_stats_start_get_element; i++){
					if(i==0){
						t1 =  strtok_r(key_list_temp, ",", &next_ptr);
					}else{
						t1 =  strtok_r(NULL, ",", &next_ptr);
					}
					memcpy(temp[i], t1, sizeof(temp[i]));
				}
				printf("(OK count:%d, %s count:%d, last MCS:%d)\n",\
					atoi(temp[0]),(dir == DIR_TX)? "RTX":"NOK", atoi(temp[1]), atoi(temp[2]));
				print_line('-', print_line_len,"", 0,0);

				/* mac stats ac */
				for(j=0; j <STATS_MAX_AC;j++){
					memset(temp, 0x0, sizeof(temp));
					for(i=0; i<mac_stats_get_element; i++){
						t1 =  strtok_r(NULL, ",", &next_ptr);
						memcpy(temp[i], t1, sizeof(temp[i]));
					}
					printf("- AC[%s]\t: OK(%10d/%10d)  %s(%10d/%10d)\n",
						temp[0],atoi(temp[1]),atoi(temp[2]),(dir == DIR_TX)? "RTX":"NOK",atoi(temp[3]),atoi(temp[4]));
				}
				print_line('-', print_line_len,"", 0,0);
				break;

		case 1 :
				/* mac stats type */
				for(j=0; j <STATS_TYPES_MAX;j++){
					memset(temp, 0x0, sizeof(temp));
					for(i=0; i<mac_stats_get_element; i++){
						if(i==0 && j ==0){
							t1 =  strtok_r(key_list_temp, ",", &next_ptr);
						}else{
							t1 =  strtok_r(NULL, ",", &next_ptr);
						}
						memcpy(temp[i], t1, sizeof(temp[i]));
					}
					printf("- TYPE[%s]\t: OK(%10d/%10d)  %s(%10d/%10d)\n",
						temp[0],atoi(temp[1]),atoi(temp[2]),(dir == DIR_TX)? "RTX":"NOK",atoi(temp[3]),atoi(temp[4]));
				}
				print_line('-', print_line_len,"", 0,0);
				break;

		case 2 :
				/* mac stats mcs */
				for(j=0; j <STATS_MCS_MAX;j++){
					memset(temp, 0x0, sizeof(temp));
					for(i=0; i<mac_stats_get_element; i++){
						if(i==0 && j ==0){
							t1 =  strtok_r(key_list_temp, ",", &next_ptr);
						}else{
							t1 =  strtok_r(NULL, ",", &next_ptr);
						}
						memcpy(temp[i], t1, sizeof(temp[i]));
					}
					printf("- MCS[%2d]\t: OK(%10d/%10d)  %s(%10d/%10d)\n",
						atoi(temp[0]),atoi(temp[1]),atoi(temp[2]),(dir == DIR_TX)? "RTX":"NOK",atoi(temp[3]),atoi(temp[4]));
				}
				print_line('-', print_line_len,"", 0,0);
				break;

		default:
				printf("Unknown type : %d\n", type);
	}

	if(key_list_temp)
		free(key_list_temp);
}

/*******************************************************************************
* sub commands on set
*******************************************************************************/
static int cmd_set_guard_interval(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if(!argv[2]||((strcmp("short", argv[2]) != 0)&&
		(strcmp("long", argv[2]) != 0)&&
		(strcmp("auto", argv[2]) != 0))){
		printf("There is invalid parameter. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		ret = CMD_RET_FAILURE;
	}else{
		sprintf(param, "set gi %s", argv[2]);
		netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
		if(!netlink_ret){
			if(strcmp(response, response_timeout_str)== 0){
				ret =  CMD_RET_RESPONSE_TIMEOUT;
			}else if(strcmp(response, "succss")){
				printf("guard interval : %s\n", argv[2]);
				ret = CMD_RET_SUCCESS;
			}
		}else{
			ret = CMD_RET_FAILURE;
		}
	}
	return ret;
}

static int cmd_set_max_aggregation(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 4;
	int print_line_len = 88;
	int i = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if(argc <4 || argc > 7) {
		printf("There is invalid parameter. Please see the help.\n");
		cmd_help(NULL, 0, NULL);
		return CMD_RET_FAILURE;
	}

	sprintf(param, "set maxagg");
	for(i=2; i<argc ; i++){
		sprintf(param+strlen(param), " %s", argv[i]);
	}
	sprintf(param+strlen(param), " -sr");

	netlink_send_data_with_retry(NL_SHELL_RUN_RAW, param, response, -1);
	cmd_set_maxagg_result_parse(response);
	ret = CMD_RET_SUCCESS;

	return ret;
}

static int cmd_set_config(cmd_tbl_t *t, int argc, char *argv[]) {
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line = 3;
	int print_line_len = 72;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	sprintf(param, "set config %s %s %s -sr", argv[2], argv[3], argv[4]);
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			print_line('-', print_line_len," updated ack, aggregation, mcs ", 0,0);
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			print_line('-', print_line_len,"", 0,0);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_rate_control(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int vif_id = 0;
	int mode = 0;
	int display_per_line = 3;
	int print_line_len = 72;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc < 3) {
		return CMD_RET_FAILURE;
	}

	if (strcmp(argv[2], "on") != 0 && strcmp(argv[2], "off") != 0) {
		return CMD_RET_FAILURE;
	}

	if(argc>3){
		if(atoi(argv[3]) != 0 && atoi(argv[3]) != 1){
			printf("invalid value %s : (ex) set rc <on|off> [vif_id] [mode]\n", argv[2]);
			return CMD_RET_FAILURE;
		}else{
			vif_id = atoi(argv[3]);	// vif_id
		}
	}

	if(argc>4){
		mode = atoi(argv[4]);	// mode [0:NRF, 1:ARF]
	}

	if(argc>5){
		sprintf(param, "set rc %s %d %d %s %s %s -sr", argv[2],  vif_id, mode, argv[5],argv[6],argv[7]);
	}else{
		sprintf(param, "set rc %s %d %d -sr", argv[2],  vif_id, mode);
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, "fail") != 0){
			if(strcmp(response, response_timeout_str)== 0){
				ret =  CMD_RET_RESPONSE_TIMEOUT;
			}else {
				print_line('-', print_line_len," updated rate control ", 0,0);
				cmd_result_parse((char*)t->key_list, response, display_per_line);
				print_line('-', print_line_len,"", 0,0);
				ret = CMD_RET_SUCCESS;
			}
		}else{
			ret = CMD_RET_FAILURE;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_duty(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc < 3) {
		return CMD_RET_FAILURE;
	}

	if (strcmp(argv[2], "on") != 0 && strcmp(argv[2], "off") != 0) {
		return CMD_RET_FAILURE;
	}

	if(argc>3){
		sprintf(param, "set duty %s %s %s %s -sr", argv[2], argv[3], argv[4], argv[5]);
	}else{
		sprintf(param, "set duty %s -sr", argv[2]);
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_cal_use(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 2;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc < 3) {
		return CMD_RET_FAILURE;
	}

	if (strcmp(argv[2], "on") != 0 && strcmp(argv[2], "off") != 0) {
		return CMD_RET_FAILURE;
	}

	sprintf(param, "set cal_use %s -sr", argv[2]);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_bdf_use(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc < 3) {
		return CMD_RET_FAILURE;
	}

	if (strcmp(argv[2], "on") != 0 && strcmp(argv[2], "off") != 0) {
		return CMD_RET_FAILURE;
	}

	sprintf(param, "set bdf_use %s -sr", argv[2]);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_txpwr(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 2;
	int txpwr = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc < 3) {
		return CMD_RET_FAILURE;
	}

	txpwr = atoi(argv[2]);
	if(txpwr<0 || txpwr >30){
		return CMD_RET_FAILURE;
	}

	sprintf(param, "nrf txpwr %d -sr", txpwr);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_wakeup_pin(cmd_tbl_t *t, int argc, char *argv[]) {
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line = 2;

	sprintf(param, "set wakeup_pin %s %s -sr", argv[2], argv[3]);
	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_wakeup_source(cmd_tbl_t *t, int argc, char *argv[]) {
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char wakeup_source_str[32];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line = 1;
	int i;

	if(argc <3)
		return CMD_RET_FAILURE;

	memset(wakeup_source_str, 0x0, sizeof(wakeup_source_str));

	for(i=2; i<argc; i++) {
		sprintf(wakeup_source_str+strlen(wakeup_source_str), "%s ",argv[i]);
	}
	sprintf(param, "set wakeup_source %s -sr", wakeup_source_str);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_addba(cmd_tbl_t *t, int argc, char *argv[]) {
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	char mac_addr[18];
	int netlink_ret = 0;
	int display_per_line = 1;
	int tid;

	if (argc < 3) {
		return CMD_RET_FAILURE;
	}

	if(argc == 3) {
		tid = atoi( argv[2]);
		sprintf(param, "%x", tid);
	} else if(argc == 4) {
		tid = atoi( argv[2]);
		memset(mac_addr, 0x0, sizeof(mac_addr));
		sprintf(mac_addr, "%s", argv[3]);
		eliminate_char(mac_addr, ':');
		sprintf(param, "%x %s", tid, mac_addr);
	}
	netlink_ret = netlink_send_data(NL_WFA_CAPI_SEND_ADDBA, param, response);
	if(!netlink_ret){
		ret = CMD_RET_SUCCESS;
	}
	return ret;
}

static int cmd_set_delba(cmd_tbl_t *t, int argc, char *argv[]) {
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	char mac_addr[18];
	int netlink_ret = 0;
	int display_per_line = 1;
	int tid;

	if (argc < 3) {
		return CMD_RET_FAILURE;
	}

	if(argc == 3) {
		tid = atoi( argv[2]);
		sprintf(param, "%x", tid);
	} else if(argc == 4) {
		tid = atoi( argv[2]);
		memset(mac_addr, 0x0, sizeof(mac_addr));
		sprintf(mac_addr, "%s", argv[3]);
		eliminate_char(mac_addr, ':');
		sprintf(param, "%x %s", tid, mac_addr);
	}
	netlink_ret = netlink_send_data(NL_WFA_CAPI_SEND_DELBA, param, response);
	if(!netlink_ret){
		ret = CMD_RET_SUCCESS;
	}
	return ret;
}

static int cmd_set_rts(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int i = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (strcmp(argv[2], "on") != 0 && strcmp(argv[2], "off") != 0 && strcmp(argv[2], "default") != 0) {
		return CMD_RET_FAILURE;
	}

	sprintf(param, "set rts %s %s %s", argv[2], argv[3], argv[4] );

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else{
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_tx_time(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if(argc == 4){
		sprintf(param, "test tx_time %s %s -sr", argv[2], argv[3]);
	}else{
		return CMD_RET_FAILURE;
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_drop_frame(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line = 3;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc != 5) {
		return CMD_RET_FAILURE;
	}

	sprintf(param, "set drop %s %s %s", argv[2], argv[3], argv[4]);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret) {
		if(strcmp(response, response_timeout_str) == 0) {
			ret = CMD_RET_RESPONSE_TIMEOUT;
		} else {
			cmd_result_parse((char*)t->key_list, response, display_per_line);
			ret = CMD_RET_SUCCESS;
		}
	} else {
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_temp_sensor(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc != 4) {
		return CMD_RET_FAILURE;
	}

	sprintf(param, "set tsensor %s %s", argv[2], argv[3]);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			printf("%s\n", response);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_set_self_configuration(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	unsigned short best_freq = 0;
	unsigned short best_cca = 0;
	unsigned short best_nons1g_freq_idx = 0;
	char best_bw=0;
	int result_idx_ptr =0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if(argc == 5){
		sprintf(param, "set self_config %s %s %s -sr", argv[2], argv[3], argv[4]);
	} else {
		return CMD_RET_FAILURE;
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN_RAW, param, response);

	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		} else if (strcmp(response, no_self_conf_str)==0){
			printf("%s\n",no_self_conf_str);
		} else {
			printf("\tFrequency\tCCA\tbandwidth\n");

			memcpy(&best_freq, &response[result_idx_ptr], sizeof(best_freq));
			result_idx_ptr+=2;
			memcpy(&best_cca, &response[result_idx_ptr], sizeof(best_cca));
			result_idx_ptr+=2;
			memcpy(&best_bw, &response[result_idx_ptr], sizeof(best_bw));
			result_idx_ptr+=1;
			memcpy(&best_nons1g_freq_idx, &response[result_idx_ptr], sizeof(best_nons1g_freq_idx));
			result_idx_ptr+=2;

			for ( ; result_idx_ptr< NL_MSG_MAX_RESPONSE_SIZE ; ){
				unsigned short freq =0 ;
				unsigned short cca =0 ;
				char bw=0;

				if (!response[result_idx_ptr])
					break;

				memcpy(&freq, &response[result_idx_ptr], sizeof(freq));
				result_idx_ptr+=2;

				memcpy(&cca, &response[result_idx_ptr], sizeof(cca));
				result_idx_ptr+=2;

				memcpy(&bw, &response[result_idx_ptr], sizeof(bw));
				result_idx_ptr+=1;

				// Outputs only the results for the preferred bandwidth.
				if (atoi(argv[3]) == 1){
					if (bw == 0)
						printf("--\t%4.1f MHz\t%3.1f%%\t%dM\n", freq/10.0, cca/10.0, (bw == 0)?1:(bw == 1)?2:4);
				} else if (atoi(argv[3]) == 2){
					if (bw == 1)
						printf("--\t%4.1f MHz\t%3.1f%%\t%dM\n", freq/10.0, cca/10.0, (bw == 0)?1:(bw == 1)?2:4);
				} else if (atoi(argv[3]) == 4){
					if (bw == 2)
						printf("--\t%4.1f MHz\t%3.1f%%\t%dM\n", freq/10.0, cca/10.0, (bw == 0)?1:(bw == 1)?2:4);
				} else {
					printf("--\t%4.1f MHz\t%3.1f%%\t%dM\n", freq/10.0, cca/10.0, (bw == 0)?1:(bw == 1)?2:4);
				}
			}

			printf("[Optimal freq.]\t%4.1f MHz (CCA:%3.1f%%, BW:%dM)\n", \
				best_freq/10.0, best_cca/10.0, (best_bw == 0)?1:(best_bw == 1)?2:4);
			printf("[*]ch_num:%d\n",best_nons1g_freq_idx );

			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

/*******************************************************************************
* sub commands on test
*******************************************************************************/
static int cmd_test_mcs(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc != 3) {
		return CMD_RET_FAILURE;
	}

	sprintf(param, "test mcs %s -sr", argv[2]);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			printf("%s\n", response);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}

static int cmd_test_country(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int display_per_line= 1;

	if(argc == 4 && strcmp(argv[3], "show") == 0){
		sprintf(param, "test country %s show -sr", argv[2]);
	}else if(argc == 5){
		sprintf(param, "test country %s %s %s -sr", argv[2], argv[3], argv[4]);
	}else{
		return CMD_RET_FAILURE;
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			if(strcmp(argv[3], "show") == 0){
				cmd_result_parse((char*)t->key_list, response, display_per_line);
			}
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
	return ret;
}


/*******************************************************************************
* sub commands on gpio
*******************************************************************************/
static int cmd_gpio_read(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc != 3) {
		return CMD_RET_FAILURE;
	}

	sprintf(param, "gpio read %s -sr", argv[2]);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			printf("%s\n", response);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
}

static int cmd_gpio_write(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if (argc != 4) {
		return CMD_RET_FAILURE;
	}

	sprintf(param, "gpio write %s %s -sr", argv[2], argv[3]);

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			printf("%s\n", response);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}
}

static int cmd_gpio_direction(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if(argc == 3){
		sprintf(param, "gpio direction %s -sr", argv[2]);
	}else if(argc == 4){
		sprintf(param, "gpio direction %s %s -sr", argv[2], argv[3]);
	}else{
		return CMD_RET_FAILURE;
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			printf("%s\n", response);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}

	return ret;
}

static int cmd_gpio_pullup(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_FAILURE;
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	if(argc == 3){
		sprintf(param, "gpio pullup %s -sr", argv[2]);
	}else if(argc == 4){
		sprintf(param, "gpio pullup %s %s -sr", argv[2], argv[3]);
	}else{
		return CMD_RET_FAILURE;
	}

	netlink_ret = netlink_send_data(NL_SHELL_RUN, param, response);
	if(!netlink_ret){
		if(strcmp(response, response_timeout_str)== 0){
			ret =  CMD_RET_RESPONSE_TIMEOUT;
		}else {
			printf("%s\n", response);
			ret = CMD_RET_SUCCESS;
		}
	}else{
		ret = CMD_RET_FAILURE;
	}

	return ret;
}

/*******************************************************************************
* function for periodic show signal command
*******************************************************************************/
void *showRxThreadRun(cmd_tbl_t *t)
{
	char param[NRC_MAX_CMDLINE_SIZE];
	char response[NL_MSG_MAX_RESPONSE_SIZE];
	int netlink_ret = 0;
	int rssi, snr;
	int i = 0;
	int device_number = 0;

	char mac_addr[MAX_ADDR_SIZE];
	char *key_list_temp = NULL;
	char *next_ptr = NULL;
	char * t1;
	int total_sta_number = 0;
	int iteration_number = 0;
	int max_response_number = 0;
	int start_point = 0;
	int try_count = 0;

	memset(response, 0x0, NL_MSG_MAX_RESPONSE_SIZE);
	memset(param, 0x0, sizeof(param));

	key_list_temp = malloc(DISP_CMD_RESULT_BUF);
	memset(mac_addr, 0x0, MAX_ADDR_SIZE);

	/* create log file and initialize value */
	if(signal_log_create(show_signal_periodic_timer) == 0){
		for(i=0; i<MAX_CONECTION_NUM; i++){
			 rssi_sum[i] = 0;
			 rssi_sum_sqrs[i] = 0;
			 snr_sum[i] = 0;
			 snr_sum_sqrs[i] =0;
		}
		 try_count = 0;
	}

	while(1)
	{
		memset(param, 0x0, sizeof(param));
		strcpy(param, "show signal -sr -num");
		netlink_ret = netlink_send_data(NL_CLI_APP_GET_INFO, param, response);
		if(netlink_ret == -1)
			continue;

		memset(key_list_temp, 0x0, DISP_CMD_RESULT_BUF);
		if(strlen(response)){
			memcpy(key_list_temp, response, strlen(response));
		}

		t1 =  strtok_r(key_list_temp, ",", &next_ptr);
		if(!t1) {
			printf("N/A\n");
		}else {
			total_sta_number = atoi(t1);
			t1 =  strtok_r(NULL, ",", &next_ptr);
			max_response_number = atoi(t1);
			if(total_sta_number == 0){
				iteration_number = 0;
			}else{
				if((total_sta_number%max_response_number) ==0){
					iteration_number = (total_sta_number/max_response_number);
				}else{
					iteration_number = (total_sta_number/max_response_number)+1;
				}
			}
		}

		try_count++;
		device_number = 0;
		start_point = 0;

		for(i=0; i<iteration_number; i++){
			memset(param, 0x0, sizeof(param));
			sprintf(param, "show signal -sr %d", start_point);
			netlink_ret = netlink_send_data(NL_CLI_APP_GET_INFO, param, response);

			if(strlen(response)>0){
				memcpy(key_list_temp, response, DISP_CMD_RESULT_BUF);
				if(strcmp(response, response_timeout_str)== 0){
					break;
				}else{
					if(strlen(response) == 0 && iteration_number == 1){
						printf("N/A\n");
					}else{
						while(1){
							if((device_number%max_response_number)==0){
								t1 =  strtok_r(key_list_temp, ",", &next_ptr);
								if(!t1 && iteration_number == 1) {
									printf("N/A");
									break;
								}

								memcpy(show_signal_mac_addr[device_number], t1, MAX_ADDR_SIZE);
								t1 =  strtok_r(NULL, ",", &next_ptr);
								rssi = atoi(t1);
								t1 =  strtok_r(NULL, ",", &next_ptr);
								snr = atoi(t1);
							}else{
								t1 =  strtok_r(NULL, ",", &next_ptr);
								if(!t1)
									break;

								memcpy(show_signal_mac_addr[device_number], t1, MAX_ADDR_SIZE);
								t1 =  strtok_r(NULL, ",", &next_ptr);
								rssi = atoi(t1);
								t1 =  strtok_r(NULL, ",", &next_ptr);
								snr = atoi(t1);
							}

							memcpy(mac_addr, show_signal_mac_addr[device_number], MAX_ADDR_SIZE);
							signal_log_update(device_number, mac_addr, rssi, snr);

							rssi_sum[device_number] += rssi;
							rssi_sum_sqrs[device_number] += (rssi*rssi);

							snr_sum[device_number] += snr;
							snr_sum_sqrs[device_number] += (snr*snr);

							count[device_number]++;

							device_number++;

							if((device_number%max_response_number) == 0){
								break;
							}
						}
						start_point +=  max_response_number;
					}
				}
			}
		}
		printf("\n");

		if(try_count == show_signal_try_number && show_signal_try_number != 0){
			show_rx_force_stop  =1;
		}

		for (i=0; i<show_signal_periodic_timer; i++) {
			if(show_rx_force_stop){
				break;
			}
			sleep(1);
		}

		if(show_rx_force_stop){
			memset(param, 0x0, sizeof(param));
			strcpy(param, "show signal stop");
			netlink_send_data(NL_CLI_APP_GET_INFO, param, response);

			for(i=0; i<device_number ; i++){
				signal_log_display(show_signal_mac_addr[i], snr_sum[i], snr_sum_sqrs[i], rssi_sum[i], rssi_sum_sqrs[i], count[i]);
			}
			signal_log_close();		/* close log file */
			pthread_cancel(showRxThread);
			showRxThread = 0;
			show_rx_force_stop=0;
			try_count = 0;
			show_signal_try_number = 0;
			break;
		}
	}

	if(key_list_temp)
		free(key_list_temp);

	return 0;
}

#endif /* _CLI_CMD_ */
