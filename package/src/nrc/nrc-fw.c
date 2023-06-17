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

#include <linux/kernel.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include "nrc.h"
#include "wim.h"
#include "nrc-hif.h"
#include "nrc-debug.h"
#include "nrc-fw.h"

#define BOOT_START_ADDR	(0x10480000)
#define FW_START_ADDR	(0x10400000)
#define PACKET_START	"NRC["
#define PACKET_END	"]MSG"

static unsigned int checksum(unsigned char *buf, int size)
{
	int sum = 0;
	int i;

	for (i = 0; i < size; i++)
		sum += buf[i];
	return sum;
}

/*
 * Firmware download packet format
 *
 * | eof (4B) | address (4B) | len (4B) | payload (1KB - 16) | checksum (4B) |
 *
 * Last packet with "eof" being 1 may have padding bytes in payload
 * but checksum will not include padding bytes
 */

void nrc_fw_update_frag(struct nrc_fw_priv *priv, struct fw_frag *frag)
{
	struct fw_frag_hdr *frag_hdr = &priv->frag_hdr;

	frag_hdr->eof = (priv->cur_chunk == (priv->num_chunks - 1));

	if (priv->cur_chunk == 0)
		frag_hdr->address = priv->start_addr;
	else
		frag_hdr->address += frag_hdr->len;

	frag_hdr->len = min_t(u32, FRAG_BYTES, priv->remain_bytes);
	frag->eof = frag_hdr->eof;
	frag->address = frag_hdr->address;
	frag->len = frag_hdr->len;
	memcpy(frag->payload, priv->fw_data_pos, frag_hdr->len);

	frag->checksum = 0;
	if (priv->csum)
		frag->checksum = checksum(frag->payload, frag_hdr->len);
}

static void nrc_fw_send_frag(struct nrc *nw, struct nrc_fw_priv *priv)
{
	struct nrc_hif_device *hdev = nw->hif;
	struct sk_buff *skb, *skb_fw;
	int skb_len;
	struct hif *hif;

	skb = nrc_wim_alloc_skb(nw, WIM_CMD_REQ_FW,
			tlv_len(sizeof(struct fw_frag)));

	nrc_fw_update_frag(priv, nrc_wim_skb_add_tlv(skb,
			WIM_TLV_FIRMWARE, sizeof(struct fw_frag), NULL));
	skb_len = skb->len;
	skb_fw = skb_copy_expand(skb, (4 + sizeof(struct hif)),
			4, GFP_ATOMIC);

	/* Prepend HIF header */
	hif = (struct hif *)skb_push(skb_fw, sizeof(struct hif));
	hif->type = HIF_TYPE_WIM;
	hif->subtype = HIF_WIM_SUB_RESPONSE;
	hif->len = skb_len;

	/* Prepend prefix and append postfix */
	memcpy(skb_push(skb_fw, 4), PACKET_START, 4);
	memcpy(skb_put(skb_fw, 4), PACKET_END, 4);

	nrc_hif_write(hdev, skb_fw->data, skb_fw->len);

	dev_kfree_skb(skb);
	dev_kfree_skb(skb_fw);
}

static bool nrc_fw_check_next_frag(struct nrc *nw, struct nrc_fw_priv *priv)
{
	struct nrc_hif_device *hdev = nw->hif;
	u8 index;
	int ret;

	if (priv->cur_chunk == (priv->num_chunks - 1)) {
		return false;
	}

	priv->cur_chunk++;
	priv->fw_data_pos += priv->frag_hdr.len;
	priv->remain_bytes -= priv->frag_hdr.len;

	index = priv->index;

	if (priv->ack) {
		ret = nrc_hif_wait_ack(hdev, &index, 1);
		BUG_ON(ret < 0);
	}

	BUG_ON(index != priv->index);
	priv->index++;

	return true;
}

struct nrc_fw_priv *nrc_fw_init(struct nrc *nw)
{
	struct nrc_fw_priv *priv;

	priv = kzalloc(sizeof(struct nrc_fw_priv), GFP_KERNEL);
	if (!priv) {
		/*nrc_dbg(NRC_DBG_HIF, "failed to allocate nrc_fw_priv");*/
		return NULL;
	}

	return priv;
}

void nrc_fw_exit(struct nrc_fw_priv *priv)
{
	kfree(priv);
}

/**
 * nrc_download_fw - download firmware binary to the target
 */
void nrc_download_fw(struct nrc *nw)
{
	struct firmware *fw = nw->fw;
	struct nrc_fw_priv *priv = nw->fw_priv;
	struct nrc_hif_device *hdev = nw->hif;

	nrc_dbg(NRC_DBG_HIF, "FW download....%s", fw_name);
	priv->num_chunks = DIV_ROUND_UP(fw->size, FRAG_BYTES);
	priv->csum = true;
	priv->fw = fw;
	priv->fw_data_pos = fw->data;
	priv->remain_bytes = fw->size;
	priv->cur_chunk = 0;
	priv->index = 0;
	priv->index_fb = 0;
	priv->start_addr = FW_START_ADDR;
	priv->ack = true;

	nrc_dbg(NRC_DBG_HIF, "[%s] priv->", __func__);
	nrc_dbg(NRC_DBG_HIF, "- fw->data:%p", priv->fw->data);
	nrc_dbg(NRC_DBG_HIF, "- fw->size:%zd", priv->fw->size);
	nrc_dbg(NRC_DBG_HIF, "- fw_data_pos:%p", priv->fw_data_pos);
	nrc_dbg(NRC_DBG_HIF, "- remain_bytes:%d", priv->remain_bytes);

	nrc_hif_disable_irq(hdev);

	pr_err("start FW %d", priv->num_chunks);
	do {
		nrc_fw_send_frag(nw, priv);
	} while (nrc_fw_check_next_frag(nw, priv));
	pr_err("end FW");

	priv->fw_requested = false;
}

bool nrc_check_fw_file(struct nrc *nw)
{
	int status;

	if (fw_name == NULL)
		goto err_fw;

	if (nw->fw)
		return true;

	status = request_firmware((const struct firmware **)&nw->fw,
			fw_name, nw->dev);

	nrc_dbg(NRC_DBG_HIF, "[%s, %d] Checking firmware... (%s)",
			__func__, __LINE__, fw_name);

	if (status != 0) {
		nrc_dbg(NRC_DBG_HIF, "request_firmware() is failed, status = %d, fw = %p",
				status, nw->fw);
		goto err_fw;
	}

	nrc_dbg(NRC_DBG_HIF, "[%s, %d] OK...(%p, 0x%zx)",
			__func__, __LINE__, nw->fw->data, nw->fw->size);
	return true;

err_fw:
	nw->fw = NULL;
	return false;
}

bool nrc_check_boot_ready(struct nrc *nw)
{
	BUG_ON(!nw);
	BUG_ON(!nw->fw_priv);
	return (nw->fw_priv->fw_requested ||
			nrc_hif_check_fw(nw->hif));
}

bool nrc_check_fw_ready(struct nrc *nw)
{
	BUG_ON(!nw);
	return (nrc_hif_check_ready(nw->hif));
}

void nrc_set_boot_ready(struct nrc *nw)
{
	BUG_ON(!nw);
	BUG_ON(!nw->fw_priv);

	nw->fw_priv->fw_requested = true;
}

void nrc_release_fw(struct nrc *nw)
{
	release_firmware(nw->fw);
	nw->fw = NULL;
}

