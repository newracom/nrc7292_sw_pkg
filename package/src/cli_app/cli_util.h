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

#ifndef _CLI_UTIL_H_
#define _CLI_UTIL_H_

#include <stdbool.h>
#include "cli_cmd.h"
#include "cli_xfer.h"
#include "cli_config.h"

#define DEFAULT_PRINT_LINE_LEN	52
#define DISP_CMD_RESULT_BUF     1024
#define COMMAND_DELAY_MS 50
#define MAX_ADDR_SIZE 18

#define IS_WHITESPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == '\v' || (c) == '\f')
#define IS_PRINTABLE_ASCII(c) ((c) >= 32 && (c) <= 126)

static int mcs_to_phy_rate_lgi[3][11] = {
{300, 600, 900, 1200, 1800, 2400, 2700, 3000, 3600, 4000, 150},
{650, 1300, 1950, 2600, 3900, 5200, 5850, 6500, 7800, 0, 150},
{1350, 2700, 4050, 5400, 8100, 10800, 12200, 13500, 16200, 18000, 150}};
static int mcs_to_phy_rate_sgi[3][11] ={
{330, 670, 1000, 1330, 2000, 2670, 3000, 3340, 4000, 4440, 170},
{720, 1440, 2170, 2890, 4330, 5780, 6500, 7220, 8670,0, 170},
{1500, 3000, 4500, 6000, 9000, 12000, 13500, 15000, 18000, 20000, 170}};

int run_sub_cmd(cmd_tbl_t *t, int argc, char *argv[], cmd_tbl_t *list, int list_size, int depth);
int get_data_number(char* input);
cmd_tbl_t *cmd_list_display(enum cmd_list_type type);
void cmd_result_parse(char* key, char *value, const int displayPerLine);
int cmd_sta_umac_info_mini_result_parse(char *value, int *display_start_index, int *aid_count);
int cmd_set_maxagg_result_parse(char *value);
int cmd_show_maxagg_result_parse(char *value, int vif, int *display_start_index);
int cmd_umac_info_result_parse(char *value, int mode, int *display_start_index);
int cli_app_run_command(char *cmd);
int cli_app_run_onetime(int argc, char* argv[]);
int cli_app_list_version_check(void);
void print_line(char c, int size, char* string, int upper_newline, int lower_newline);
int  strlen_last_line(char* string);
void run_awk(char* filename, int num, char* delim, int* pos, char** result);
int signal_log_create(int interval);
int signal_log_close(void);
int signal_log_display(char* mac_addr, int snr_sum, int snr_sum_sqrs, int rssi_sum, int rssi_sum_sqrs, int n);
int signal_log_update(int device_number, char* mac_addr, int rssi, int snr);
void cli_delay_ms(int ms);
double calculate_avergage(int sum, int n);
double calculate_std_dev(int sum, int sum_sqrs, int n);
int util_cmd_parse_line(char *s, char *argv[]);
void eliminate_char(char *str, char ch);
void string_to_hexString(char* input, char* output);
char hex_to_int(char c);
void macaddr_to_ascii(char* input, char* output);
void print_hex(void *address, int count);
void print_mac_address(char mac_addr[6]);
char cli_getch(void);
char cli_getche(void);
void cli_input_prompt(const char* prompt_name, char* input);
void cli_sysconfig_print(xfer_sys_config_t *sysconfig, bool hex, int model);
void cli_user_factory_print(xfer_sys_config_t *sysconfig, bool hex, int model);
void cmd_show_sysconfig_parse(xfer_sys_config_t *sysconfig, int display_mode, int model);
#endif /* _CLI_UTIL_H_ */
