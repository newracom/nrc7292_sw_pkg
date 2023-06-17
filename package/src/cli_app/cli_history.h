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

#ifndef _CLI_HISTORY_H_
#define _CLI_HISTORY_H_
#include <stdint.h>
#include "cli_cmd.h"

typedef struct {
	char command[NRC_MAX_CMDLINE_SIZE];
} command_t;

typedef struct {
	command_t* commands;
	int capacity;
	int size;
	int head;
	int tail;
} cli_history_t;

void cli_history_init(cli_history_t* history, int capacity);
void cli_history_free(cli_history_t* history);
void cli_history_add(cli_history_t* history, char* command);
char* cli_history_get(cli_history_t* history, int index);
void cli_history_print_all(cli_history_t* history);
int cli_history_get_total_size(cli_history_t* history);

#endif /* _CLI_HISTORY_H_ */
