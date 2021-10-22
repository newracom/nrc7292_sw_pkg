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

int main(int argc, char* argv[])
{
	int ret = CMD_RET_SUCCESS;
	int print_line_len = DEFAULT_PRINT_LINE_LEN;
	char buffer[NRC_MAX_CMDLINE_SIZE] = { 0, };
	char *pos;
	char* greeting_msg = NULL;

	if (argc > 1) {
		cli_app_run_onetime(argc, argv);
		return 0;
	}

	greeting_msg = malloc(strlen(app_name)+strlen(nrc_cli_app_ver)+4);
	sprintf(greeting_msg, "%s (%s)", app_name,nrc_cli_app_ver);

	print_line('=', print_line_len,"", 0,0);
	print_line(' ', print_line_len,greeting_msg, 0,0);
	print_line('=', print_line_len,"", 0,0);

	if(greeting_msg)
		free(greeting_msg);

	while (1) {
		printf("%s> ",prompt_name);
		if (NULL == fgets(buffer, sizeof(buffer), stdin)){
			break;
		}

		if ((pos = strchr(buffer, '\n')) != NULL){
			*pos = '\0';
		}

		ret = cli_app_run_command(buffer);
		if (ret == CMD_RET_FAILURE) {
			printf("FAIL\n");
		}else if (ret == CMD_RET_SUCCESS) {
			printf("OK\n");
		}else if (ret == CMD_RET_EXIT_PROGRAM) {
			break;
		}
	}
	print_line('=', print_line_len,"", 0,0);
	print_line(' ', print_line_len,(char*)exit_msg, 0,0);
	print_line('=', print_line_len,"", 0,0);
	return 0;
}