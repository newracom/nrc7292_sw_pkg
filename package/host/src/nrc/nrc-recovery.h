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

#ifndef _NRC_RECOVERY_H_
#define _NRC_RECOVERY_H_

struct nrc_recovery_wdt {
	int period;
	struct nrc *nw;
	struct timer_list wdt;
	struct timer_list timer;
	struct work_struct work;
};

void nrc_recovery_wdt_init(struct nrc *nw, int period);
void nrc_recovery_wdt_kick(struct nrc *nw);
void nrc_recovery_wdt_stop(struct nrc *nw);
void nrc_recovery_wdt_clear(struct nrc *nw);

#endif
