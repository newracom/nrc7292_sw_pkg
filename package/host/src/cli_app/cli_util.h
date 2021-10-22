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

#define DEFAULT_PRINT_LINE_LEN	52
#define DISP_CMD_RESULT_BUF     1024
#define COMMAND_DELAY_MS 50
#define MAX_ADDR_SIZE 18

int run_sub_cmd(cmd_tbl_t *t, int argc, char *argv[], cmd_tbl_t *list, int list_size, int depth);
int get_data_number(char* input);
cmd_tbl_t *cmd_list_display(enum cmd_list_type type);
void cmd_result_parse(char* key, char *value, int displayPerLine);
int cmd_sta_umac_info_mini_result_parse(char *value, int *display_start_index, int *aid_count);
int cmd_set_maxagg_result_parse(char *value);
int cmd_show_maxagg_result_parse(char *value, int *display_start_index);
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
#endif /* _CLI_UTIL_H_ */
