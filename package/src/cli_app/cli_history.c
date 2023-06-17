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

#ifndef _CLI_HISTORY_
#define _CLI_HISTORY_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli_history.h"


// Initialize the command history with a circular buffer of size capacity
void cli_history_init(cli_history_t* history, int capacity) {
    history->commands = (command_t*)malloc(capacity * sizeof(command_t));
    history->capacity = capacity;
    history->size = 0;
    history->head = 0;
    history->tail = 0;
}

// Free the memory used by a command history
void cli_history_free(cli_history_t* history) {
    free(history->commands);
}

// Add a command to the command history
void cli_history_add(cli_history_t* history, char* command) {
    if (strlen(command) >= NRC_MAX_CMDLINE_SIZE) {
        // Command too long, do not add to history
        return;
    }

    // Copy the command to the history buffer
    strcpy(history->commands[history->head].command, command);

    // Move the head pointer
    history->head = (history->head + 1) % history->capacity;

    if (history->size < history->capacity) {
        // Increase the size of the history
        history->size++;
    }
    else {
        // The buffer is full, move the tail pointer
        history->tail = (history->tail + 1) % history->capacity;
    }
}

// Get the command at the given index in the history
char* cli_history_get(cli_history_t* history, int index) {
    if (index >= history->size || index < 0) {
        // Index out of range
        return NULL;
    }

    int i = (history->tail + index) % history->capacity;

    return history->commands[i].command;
}

void cli_history_print_all(cli_history_t* history) {
    printf("Command history:\n");
    for (int i = 0; i < history->size; i++) {
        printf("%s\n", cli_history_get(history, i));
    }
}

int cli_history_get_total_size(cli_history_t* history) {
	return history->size;
}
#endif /* _CLI_HISTORY_ */
