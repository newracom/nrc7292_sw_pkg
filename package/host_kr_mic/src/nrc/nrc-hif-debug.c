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

#include "nrc-hif.h"
#include "nrc-hif-debug.h"
#include "nrc-debug.h"

#ifdef CONFIG_NRC_HIF_DEBUG

static const char *nrc_hif_debug_name(struct nrc_hif_device *dev)
{
	return "DEBUG";
}

static int nrc_hif_debug_start(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	return 0;
}

static int nrc_hif_debug_stop(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	return 0;
}

static int nrc_hif_debug_write(struct nrc_hif_device *dev, const u8 *data,
		const u32 len)
{
	print_hex_dump(KERN_DEBUG, " ", DUMP_PREFIX_NONE, 16, 1, data, len,
			false);

	return 0;
}

static int nrc_hif_debug_write_begin(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	return 0;
}

static int nrc_hif_debug_write_body(struct nrc_hif_device *dev,
		const u8 *body, const u32 len)
{
	nrc_dbg(NRC_DBG_HIF, "%s(body:%d)", __func__, len);
	print_hex_dump(KERN_DEBUG, " ", DUMP_PREFIX_NONE, 16, 1, body,
			len, false);
	return 0;
}

static int nrc_hif_debug_write_end(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	return 0;
}

static int nrc_hif_debug_suspend(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	return 0;
}

static int nrc_hif_debug_resume(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	return 0;
}

static struct nrc_hif_ops nrc_hif_debug_ops = {
	.name = nrc_hif_debug_name,
	.start = nrc_hif_debug_start,
	.stop = nrc_hif_debug_stop,
	.write = nrc_hif_debug_write,
	.write_begin = nrc_hif_debug_write_begin,
	.write_body = nrc_hif_debug_write_body,
	.write_end = nrc_hif_debug_write_end,
	.suspend = nrc_hif_debug_suspend,
	.resume = nrc_hif_debug_resume
};

struct nrc_hif_device *nrc_hif_debug_init(void)
{
	struct nrc_hif_device *dev = kmalloc(sizeof(*dev), GFP_KERNEL);

	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);

	if (!dev)
		return NULL;

	dev->hif_ops = &nrc_hif_debug_ops;
	dev->priv = NULL;

	return dev;
}

int nrc_hif_debug_exit(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);

	if (!dev)
		return -EINVAL;
	kfree(dev);

	return 0;
}

#endif

