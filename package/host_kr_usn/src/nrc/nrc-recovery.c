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

#include "nrc.h"
#include "wim.h"
#include "nrc-debug.h"
#include "nrc-netlink.h"
#include "nrc-recovery.h"


static void trigger(struct work_struct *work)
{
	struct nrc_recovery_wdt *wdt;

	wdt = container_of(work, struct nrc_recovery_wdt, work);
	nrc_netlink_trigger_recovery(wdt->nw);
}

#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
static void bark(unsigned long data)
{
	struct nrc *nw = (struct nrc *) data;
#else
static void bark(struct timer_list *t)
{
	struct nrc_recovery_wdt *wdt = from_timer(wdt, t, wdt);
	struct nrc *nw = wdt->nw;
#endif
	nrc_mac_dbg("%s\n", __func__);
	queue_work(nw->workqueue, &nw->recovery_wdt->work);
}

#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
static void poll(unsigned long data)
{
	struct nrc *nw = (struct nrc *) data;
#else
static void poll(struct timer_list *t)
{
	struct nrc_recovery_wdt *wdt = from_timer(wdt, t, timer);
	struct nrc *nw = wdt->nw;
#endif
	if (!nrc_wim_request_keep_alive(nw))
		nrc_mac_dbg("failed to request keep-alive\n");
}

void nrc_recovery_wdt_init(struct nrc *nw, int period)
{
	if (nw->recovery_wdt)
		return;

	nw->recovery_wdt = kzalloc(sizeof(struct nrc_recovery_wdt), GFP_KERNEL);

	if (!nw->recovery_wdt)
		return;

	nw->recovery_wdt->period = period;
	nw->recovery_wdt->nw = nw;

#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
	setup_timer(&nw->recovery_wdt->wdt, bark, (unsigned long) nw);
	setup_timer(&nw->recovery_wdt->timer, poll, (unsigned long) nw);
#else
	timer_setup(&nw->recovery_wdt->wdt, bark, 0);
	timer_setup(&nw->recovery_wdt->timer, poll, 0);
#endif
	INIT_WORK(&nw->recovery_wdt->work, trigger);
}

void nrc_recovery_wdt_kick(struct nrc *nw)
{
	int p;

	if (!nw->recovery_wdt)
		return;

	p = nw->recovery_wdt->period;
	mod_timer(&nw->recovery_wdt->wdt, jiffies + msecs_to_jiffies(p));
	mod_timer(&nw->recovery_wdt->timer, jiffies + msecs_to_jiffies(p / 2));
}

void nrc_recovery_wdt_stop(struct nrc *nw)
{
	if (nw->recovery_wdt) {
		del_timer_sync(&nw->recovery_wdt->wdt);
		del_timer_sync(&nw->recovery_wdt->timer);
	}
}

void nrc_recovery_wdt_clear(struct nrc *nw)
{
	if (!nw->recovery_wdt)
		return;

	del_timer_sync(&nw->recovery_wdt->wdt);
	del_timer_sync(&nw->recovery_wdt->timer);
	kfree(nw->recovery_wdt);
	nw->recovery_wdt = NULL;
}
