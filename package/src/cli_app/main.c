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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli_cmd.h"
#include "cli_util.h"
#include "cli_config.h"
#include "cli_history.h"

#define ESCAPE 27
#define UP_ARROW 65
#define DOWN_ARROW 66
#define LEFT_ARROW 67
#define RIGHT_ARROW 68
#define BACKSPACE 8
#define DELETE 127
#define TAB 9
#define LEFT_BRACKET 91

int main(int argc, char* argv[])
{
	int ret = CMD_RET_SUCCESS;
	int print_line_len = DEFAULT_PRINT_LINE_LEN;
	char input[NRC_MAX_CMDLINE_SIZE] = "";
	char* greeting_msg = NULL;
#if ENABLE_CLI_HISTORY
	cli_history_t history;
	int input_index = 0;
	int history_index = -1;
	char c;
	cli_history_init(&history, CLI_BUFFER_CMD_NUM);
#endif // ENABLE_CLI_HISTORY

	if (argc > 1) {
		cli_app_run_onetime(argc, argv);
		return 0;
	}

	greeting_msg = malloc(strlen(NRC_CLI_APP_NAME)+strlen(NRC_CLI_APP_VER)+4);
	sprintf(greeting_msg, "%s (%s)", NRC_CLI_APP_NAME,NRC_CLI_APP_VER);

	print_line('=', print_line_len,"", 0,0);
	print_line(' ', print_line_len,greeting_msg, 0,0);
	print_line('=', print_line_len,"", 0,0);

	if(greeting_msg)
		free(greeting_msg);

	while (1) {
#if ENABLE_CLI_HISTORY
		if(input_index == 0 && c != BACKSPACE && c != LEFT_BRACKET && c != ESCAPE && c != DELETE &&
			c != UP_ARROW && c != DOWN_ARROW && c != LEFT_ARROW && c != RIGHT_ARROW  && c != TAB)
			printf("\n%s> ",NRC_CLI_APP_PROMPT);
		c = cli_getch();
		if (c == '\n') {
			if((int)strlen(input) > 0){
				cli_history_add(&history, input);
				history_index = cli_history_get_total_size(&history);
				printf("\n");
				ret = cli_app_run_command(input);
				if (ret == CMD_RET_FAILURE) {
					printf("FAIL\n");
				}else if (ret == CMD_RET_SUCCESS) {
					printf("OK\n");
				}else if (ret == CMD_RET_EXIT_PROGRAM) {
					break;
				}
				input[0] = '\0';
				input_index = 0;
				printf("\n");
			} else {
				input[0] = '\0';
				input_index = 0;
			}
		} else if ((c == BACKSPACE)||(c == DELETE)) {
			if (input_index > 0) {
				input_index--;
				input[input_index] = '\0';
				printf("\b \b");
				fflush(stdout);
			}
		} else if ((c == ESCAPE)||(c == LEFT_BRACKET )||(c == LEFT_ARROW )||(c == RIGHT_ARROW )||(c == TAB )) {
			;
		} else if (c == DOWN_ARROW) {
			if (history_index < history.size - 1) {
				history_index++;
				memset(input, 0x0, sizeof(input));
				strncpy(input, cli_history_get(&history, history_index), NRC_MAX_CMDLINE_SIZE);
				input_index = strlen(input);
			}else {
				history_index = history.size;
				input[0] = '\0';
				input_index = 0;
			}
			cli_input_prompt(NRC_CLI_APP_PROMPT, input);
		} else if (c == UP_ARROW) {
			if (history_index > 0) {
				history_index--;
				strncpy(input, cli_history_get(&history, history_index), NRC_MAX_CMDLINE_SIZE);
				input_index = strlen(input);
			} else {
				history_index = -1;
				input[0] = '\0';
				input_index = 0;
			}
			cli_input_prompt(NRC_CLI_APP_PROMPT, input);
		} else {
			// Handle regular characters
			if (input_index < NRC_MAX_CMDLINE_SIZE - 1) {
				input[input_index] = c;
				input_index++;
				input[input_index] = '\0';
				printf("%c", c);
				fflush(stdout);
			}
		}
#else
		printf("%s> ",NRC_CLI_APP_PROMPT);
		if (NULL == fgets(input, sizeof(input), stdin)){
			break;
		}

		int input_len = strlen(input);
		if (input_len > 0 && input[input_len - 1] == '\n') {
			input[input_len - 1] = '\0';
		}

		ret = cli_app_run_command(input);
		if (ret == CMD_RET_FAILURE) {
			printf("FAIL\n");
		}else if (ret == CMD_RET_SUCCESS) {
			printf("OK\n");
		}else if (ret == CMD_RET_EXIT_PROGRAM) {
			break;
		}
#endif
	}
	print_line('=', print_line_len,"", 0,0);
	print_line(' ', print_line_len,(char*)NRC_CLI_APP_EXIT_MSG, 0,0);
	print_line('=', print_line_len,"", 0,0);
	return 0;
}

