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
#include "nrc-hif-ssp.h"
#include "nrc-debug.h"
#include "nrc-ssp.h"
#include "wim.h"
#include "nrc-mac80211.h"

static void ssp_args_prepare(struct nrc_hif_ssp *priv)
{
	memset(priv, 0, sizeof(struct nrc_hif_ssp));
	priv->name = hifport;
	priv->data_rate = hifspeed;
}

static const char *nrc_hif_ssp_name(struct nrc_hif_device *dev)
{
	return "SSP";
}

static int nrc_hif_ssp_start(struct nrc_hif_device *dev)
{
	struct nrc_hif_ssp *priv = dev->priv;

	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	priv->dev = dev;

	return nrc_ssp_register(priv);
}

static int nrc_hif_ssp_stop(struct nrc_hif_device *dev)
{
	struct nrc_hif_ssp *priv = dev->priv;

	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	priv->dev = dev;

	nrc_ssp_unregister(priv);
	return 0;
}

static int nrc_hif_ssp_write(struct nrc_hif_device *dev, const u8 *data,
		const u32 len)
{
	return -EOPNOTSUPP;
}

static int nrc_hif_ssp_suspend(struct nrc_hif_device *dev)
{
	return -EOPNOTSUPP;
}

static int nrc_hif_ssp_resume(struct nrc_hif_device *dev)
{
	return -EOPNOTSUPP;
}

static int nrc_hif_ssp_write_begin(struct nrc_hif_device *dev)
{
	return -EOPNOTSUPP;
}

static int nrc_hif_ssp_write_body(struct nrc_hif_device *dev,
		const u8 *data, const u32 len)
{
	return -EOPNOTSUPP;
}

static int nrc_hif_ssp_write_end(struct nrc_hif_device *dev)
{
	return -EOPNOTSUPP;
}

static int nrc_hif_ssp_wait_ack(struct nrc_hif_device *dev, u8 *data, u32 len)
{
	return -EOPNOTSUPP;
}

static int nrc_hif_ssp_receive_skb(struct nrc_hif_device *dev,
		struct sk_buff *skb)
{
	struct nrc *nw = dev->nw;
	struct hif *hif = (void *)skb->data;

	if (nw->drv_state < NRC_DRV_START)
		return -EIO;

	skb_pull(skb, sizeof(struct hif)); /*remove hif header from skb*/

	switch (hif->type) {
	case HIF_TYPE_FRAME:
		nrc_mac_rx(nw, skb);
		break;
	case HIF_TYPE_WIM:
		nrc_wim_rx(nw, skb, hif->subtype);
		break;
	default:
		BUG();
	}
	return 0;
}

static int nrc_hif_ssp_xmit(struct nrc_hif_device *dev, struct sk_buff *skb)
{
	struct nrc_hif_ssp *hif = (struct nrc_hif_ssp *)dev->priv;
	struct nrc_ssp *ssp = hif->ssp;

	ssp_enqueue_buffer(ssp, skb);

	return HIF_TX_QUEUED;
}

static struct nrc_hif_ops nrc_hif_ssp_ops = {
	.name = nrc_hif_ssp_name,
	.start = nrc_hif_ssp_start,
	.stop = nrc_hif_ssp_stop,
	.write = nrc_hif_ssp_write,
	.suspend = nrc_hif_ssp_suspend,
	.resume = nrc_hif_ssp_resume,
	.write_begin = nrc_hif_ssp_write_begin,
	.write_body = nrc_hif_ssp_write_body,
	.write_end = nrc_hif_ssp_write_end,
	.wait_ack = nrc_hif_ssp_wait_ack,
	.receive = nrc_hif_ssp_receive_skb,
	.xmit = nrc_hif_ssp_xmit
};

struct nrc_hif_device *nrc_hif_ssp_init(void)
{
	struct nrc_hif_device *dev = kmalloc(sizeof(*dev), GFP_KERNEL);

	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);

	if (!dev)
		return NULL;

	dev->priv = kmalloc(sizeof(struct nrc_hif_ssp), GFP_KERNEL);
	if (!dev->priv) {
		/*nrc_dbg(NRC_DBG_HIF, "Failed to allocate nrc_hif priv");*/
		kfree(dev);
		return NULL;
	}

	ssp_args_prepare(dev->priv);
	nrc_hif_ssp_ops.sync_auto = true;
	dev->hif_ops = &nrc_hif_ssp_ops;

	return dev;
}

int nrc_hif_ssp_exit(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);

	if (!dev)
		return -EINVAL;
	kfree(dev);

	return 0;
}
