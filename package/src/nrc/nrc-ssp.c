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

#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/errno.h>

#include "nrc-hif-ssp.h"
#include "nrc-ssp-dev.h"
#include "nrc-ssp.h"
#include "nrc-fw.h"
#include "nrc-wim-types.h"

static struct nrc_ssp g_ssp = {0,};

static uint8_t cal_csum(struct ssp_header_t *header)
{
	uint8_t csum = 0;
	uint8_t *buf = (uint8_t *)header;
	uint8_t byte_len = sizeof(struct ssp_header_t) - sizeof(header->m_csum);
	uint32_t i;

	for (i = 0; i < byte_len; i++)
		csum ^= buf[i];
	return csum;
}

static void spu_transaction_init(struct nrc_ssp *ssp)
{
	if (!time_after(jiffies, ssp->prev_xfer_time + 1))
		ndelay(40);
}

static void spu_transaction_finish(struct nrc_ssp *ssp)
{
	ssp->prev_xfer_time = jiffies;
}

static bool ssp_xfer(struct nrc_ssp *ssp, uint8_t *tx, uint8_t *rx, int count)
{
	int err = 0;
	struct spi_message m;
	struct spi_transfer data_trans;

	spi_message_init(&m);
	memset(&data_trans, 0, sizeof(data_trans));

	spu_transaction_init(ssp);

	if (tx)
		data_trans.tx_buf = tx;
	if (rx)
		data_trans.rx_buf = rx;
	data_trans.len = count;

	spi_message_add_tail(&data_trans, &m);

	err = spi_sync(ssp->spi, &m);
	spu_transaction_finish(ssp);

	return !!(err == 0);
}

void ssp_make_header(uint8_t *buffer, uint8_t type, uint8_t sub_type,
		uint16_t length, uint8_t padding)
{
	struct ssp_header_t *header = (struct ssp_header_t *) buffer;

	memset(buffer, 0, 8);

	header->m_soh = NR_HOST_IF_HEADER_SOH;
	header->m_ver = NR_HOST_IF_HEADER_VER;
	header->m_type  = type;
	header->m_sub_type = sub_type;
	header->m_length = length;
	header->m_padding = padding;
	header->m_csum = cal_csum(header);
}

static uint8_t recover_buf[6*1024];

static bool ssp_ping_pong(struct nrc_ssp *ssp)
{
	struct ssp_header_t header;

	ssp_make_header((uint8_t *)&header, HIF_TYPE_SSP_PING, 0, 0, 0);

	ssp_xfer(ssp, (uint8_t *)&header, NULL, sizeof(struct ssp_header_t));
	mdelay(1);

	memset((uint8_t *)&header, 0xFF, sizeof(struct ssp_header_t));

	ssp_xfer(ssp, NULL, (uint8_t *)&header, sizeof(struct ssp_header_t));
	mdelay(1);

	if ((header.m_csum == cal_csum(&header)) &&
		(header.m_type == HIF_TYPE_SSP_PONG)) {
		return true;
	} else {
		return false;
	}
}

static void ssp_recovery(struct nrc_ssp *ssp)
{
	int err = 0;
	int max_count = 100;

	nrc_dbg(NRC_DBG_HIF, "----------[%s]--------\n", __func__);

	memset(recover_buf, 0xff, sizeof(recover_buf));

	while (max_count--) {
		if (ssp_ping_pong(ssp)) {
			nrc_dbg(NRC_DBG_HIF, "<<< Pong\n");
			break;
		}
		err = ssp_xfer(ssp, recover_buf, NULL, 1);
		mdelay(10);
	}
}

int ssp_vaild_check(struct nrc_ssp_priv *priv, struct ssp_header_t *header)
{
	if (header->m_csum != cal_csum(header)) {
		nrc_dbg(NRC_DBG_HIF, "csum error\n");
		return SSP_V_ERROR;
	}

	if (header->m_soh != 0x4e) {
		nrc_dbg(NRC_DBG_HIF, "soh error(0x%02X)\n", header->m_soh);
		return SSP_V_ERROR;
	}

	if (header->m_ver != 0) {
		nrc_dbg(NRC_DBG_HIF, "version error(0x%02X)\n", header->m_ver);
		return SSP_V_ERROR;
	}

	if (header->m_type == HIF_TYPE_SSP_SKIP) {
		nrc_dbg(NRC_DBG_HIF, "Skip state report (received:%d, current:%d)\n",
				header->m_length, priv->hif_isr_count);
		priv->hif_isr_count = header->m_length;
		return SSP_V_SKIP;
	}

	if (header->m_length == 0) {
		nrc_dbg(NRC_DBG_HIF, "length is 0\n");
		return SSP_V_ERROR;
	}

	return SSP_V_GOOD;
}

static bool ssp_wait_gdma_ready(void)
{
	uint32_t timeout = 0;
	int val = 0;

	udelay(50);

	while (timeout < 100) {
		val = gpio_get_value(spi_gdma_irq);
		if (val)
			break;

		udelay(10);
		timeout++;
	}

	return !!(val);
}

int ssp_read_buffer(struct nrc_ssp *ssp)
{
	bool ret;
	int ssp_validity;
	struct ssp_header_t header;
	struct nrc_ssp_priv *priv;
	struct nrc_hif_device *dev;
	struct sk_buff *skb;

	priv = ssp->priv;

	ssp_make_header((uint8_t *)&header, HIF_TYPE_SSP_READYRX, 0, 0, 0);
	ret = ssp_xfer(ssp, (uint8_t *)&header, NULL,
			sizeof(struct ssp_header_t));

	if (!ssp_wait_gdma_ready()) {
		nrc_dbg(NRC_DBG_HIF, "fail to get gdma ready(Read)\n");
		return -1;
	}

	if (!ret) {
		nrc_dbg(NRC_DBG_HIF, "fail to send RxReady\n");
		return -2;
	}

	memset((uint8_t *)&header, 0, sizeof(struct ssp_header_t));
	ret = ssp_xfer(ssp, NULL, (uint8_t *)&header,
			sizeof(struct ssp_header_t));
	if (!ret) {
		nrc_dbg(NRC_DBG_HIF, "fail to read header\n");
		return -3;
	}

	ssp_validity = ssp_vaild_check(priv, &header);
	if (ssp_validity != SSP_V_GOOD) {
		if (ssp_validity == SSP_V_SKIP)
			return -4;
		nrc_dbg(NRC_DBG_HIF,
		"soh:%02X, ver:%02X, type:%02X subtype:%02X, len:%d, pad:%02X, cs:%02X\n",
				header.m_soh, header.m_ver, header.m_type,
				header.m_sub_type, header.m_length,
				header.m_padding, header.m_csum);

		ssp_recovery(ssp);
		return -5;
	}

	if (header.m_padding) {
		skb = dev_alloc_skb(header.m_length);
		ret = ssp_xfer(ssp, NULL, skb_put(skb, header.m_length),
				header.m_length);

		if (!ret) {
			nrc_dbg(NRC_DBG_HIF, "fail to read payload\n");
			dev_kfree_skb_any(skb);
			return -6;
		}

		dev = ssp->hif->dev;
		dev->hif_ops->receive(dev, skb);

		if (!ssp_wait_gdma_ready()) {
			nrc_dbg(NRC_DBG_HIF, "fail to get gdma ready(Read done)\n");
			dev_kfree_skb_any(skb);
			return -7;
		}
	}

	return header.m_padding;
}

bool ssp_enqueue_buffer(struct nrc_ssp *ssp, struct sk_buff *skb)
{
	struct nrc_ssp_priv *priv = ssp->priv;

	skb_queue_tail(&priv->skb_head, skb);
	queue_work(priv->tx_wq, &priv->tx_work);

	return true;
}

bool ssp_write_buffer(struct nrc_ssp *ssp, uint8_t *buffer,
		int length, uint8_t type)
{
	bool ret;
	struct ssp_header_t header;

	ssp_make_header((uint8_t *)&header, type, 0, length, 0);
	ret = ssp_xfer(ssp, (uint8_t *)&header, NULL,
			sizeof(struct ssp_header_t));

	if (!ssp_wait_gdma_ready()) {
		nrc_dbg(NRC_DBG_HIF, "fail to get gdma ready(Write)\n");
		return false;
	}

	if (!ret) {
		nrc_dbg(NRC_DBG_HIF, "fail to send Tx Header\n");
		return false;
	}

	ret = ssp_xfer(ssp, buffer, NULL, length);

	if (!ret) {
		nrc_dbg(NRC_DBG_HIF, "fail to send Tx payload\n");
		return false;
	}

	if (!ssp_wait_gdma_ready()) {
		nrc_dbg(NRC_DBG_HIF, "fail to get gdma ready(Write done)\n");
		return false;
	}

	return true;
}


void ssp_show_header(struct ssp_header_t *header)
{
	nrc_dbg(NRC_DBG_HIF, "version: 0x%02X\n",
			header->m_ver);
	nrc_dbg(NRC_DBG_HIF, "type: 0x%02X\n",
			header->m_type);
	nrc_dbg(NRC_DBG_HIF, "sub_type: 0x%02X\n",
			header->m_sub_type);
	nrc_dbg(NRC_DBG_HIF, "length: 0x%04X(%d)\n",
			header->m_length, header->m_length);
	nrc_dbg(NRC_DBG_HIF, "padding: 0x%02X\n",
			header->m_padding);
	nrc_dbg(NRC_DBG_HIF, "csum: 0x%02X\n",
			header->m_csum);
}

void ssp_show_hex(uint8_t *buf, int len)
{
	int i = 0;

	for (i = 1; i <= len; i++) {
		if ((i & 0xf) == 1) {
			if (i != 1)
				nrc_dbg(NRC_DBG_HIF, "\n");
		}
		nrc_dbg(NRC_DBG_HIF, "%02x ", (uint8_t) *buf);
		buf++;
	}
	nrc_dbg(NRC_DBG_HIF, "\n");
}

static irqreturn_t ssp_hif_isr(int irq, void *dev)
{
	struct nrc_ssp *ssp = (struct nrc_ssp *) dev;
	struct nrc_ssp_priv *priv = ssp->priv;

	atomic_inc(&priv->hif_req);
	priv->hif_isr_count++;

	queue_work(priv->rx_wq, &priv->rx_work);

	return IRQ_HANDLED;
}

#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
static void priodic_call(unsigned long data)
{
	struct nrc_ssp_priv *priv = (struct nrc_ssp_priv *)data;

	queue_work(priv->tx_wq, &priv->tx_work);

	set_timer(priv);
}

void set_timer(struct nrc_ssp_priv *priv)
{
	int delay = 1;
	struct nrc_hif_ssp *hif = priv->ssp->hif;

	init_timer_on_stack(&hif->exp_timer);
	hif->exp_timer.expires = jiffies + delay * HZ / 100;
	hif->exp_timer.data = (unsigned long) priv;
	hif->exp_timer.function = priodic_call;
	add_timer(&hif->exp_timer);
}
#else
static void priodic_call(struct timer_list *t)
{
	struct nrc_hif_ssp *hif = from_timer(hif, t,
			exp_timer);
	struct nrc_ssp_priv *priv = hif->ssp->priv;

	queue_work(priv->tx_wq, &priv->tx_work);
	set_timer(priv);
}

void set_timer(struct nrc_ssp_priv *priv)
{
	struct nrc_hif_ssp *hif = priv->ssp->hif;
	int delay = 1;

	hif->exp_timer.expires = jiffies + delay * HZ / 100;
	timer_setup(&hif->exp_timer, priodic_call, 0);
}
#endif

static void rx_worker(struct work_struct *work)
{
	struct nrc_ssp *ssp;
	struct nrc_ssp_priv *priv;
	int q_cnt;

	priv = container_of(work, struct nrc_ssp_priv, rx_work);
	if (atomic_read(&priv->hif_req) == 0)
		return;
	ssp = priv->ssp;
	do {
		mutex_lock(&priv->bus_lock_mutex);
		q_cnt = ssp_read_buffer(ssp);
		mutex_unlock(&priv->bus_lock_mutex);
		if (q_cnt <= 0) {
			nrc_dbg(NRC_DBG_HIF, "[%s] read error %d req=%d\n",
					__func__, q_cnt,
					atomic_read(&priv->hif_req));
		} else if (atomic_read(&priv->hif_req) > 0)
			atomic_dec(&priv->hif_req);
	} while (q_cnt > 1);
}

static void tx_worker(struct work_struct *work)
{
	struct nrc_ssp *ssp;
	struct nrc_ssp_priv *priv;
	int count;
	struct nrc_hif_ssp *hif_ssp;
	struct nrc *nw;

	priv = container_of(work, struct nrc_ssp_priv, tx_work);
	hif_ssp = priv->ssp->hif;
	nw = hif_ssp->dev->nw;
	ssp = priv->ssp;

	count = skb_queue_len(&priv->skb_head);
	if (count) {
		struct sk_buff *skb;
		struct hif *hif_hdr;

		skb = skb_dequeue(&priv->skb_head);
		hif_hdr = (struct hif *)skb->data;

		mutex_lock(&priv->bus_lock_mutex);
		ssp_write_buffer(ssp, skb->data, skb->len, hif_hdr->type);
		mutex_unlock(&priv->bus_lock_mutex);
		nrc_hif_free_skb(nw, skb);
	}
}

static int ssp_probe(struct spi_device *spi)
{
	int ret = 0;
	struct nrc *nw;
	struct nrc_ssp *ssp;
	struct nrc_hif_ssp *hif;
	int err;
	struct nrc_ssp_priv *priv;

	nrc_dbg(NRC_DBG_HIF, "+%s", __func__);

	ssp = &g_ssp;

	priv = kzalloc(sizeof(struct nrc_ssp_priv), GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		goto out;
	}

	spi_set_drvdata(spi, ssp);
	ssp->spi = spi;
	ssp->prev_xfer_time = jiffies;
	hif = ssp->hif;
	nw = hif->dev->nw;

	skb_queue_head_init(&priv->skb_head);

	priv->rx_wq = create_singlethread_workqueue("nrc72xx_hif_rx");
	INIT_WORK(&priv->rx_work, rx_worker);

	priv->tx_wq = create_singlethread_workqueue("nrc72xx_hif_tx");
	INIT_WORK(&priv->tx_work, tx_worker);

	mutex_init(&priv->bus_lock_mutex);

	priv->hif_isr_count = 0;

	/* TODO: check FW(target) ready to work */
	priv->fw_ready = true;

	priv->ssp = ssp;
	ssp->priv = priv;

	ret = gpio_request(spi->irq, "ssp_int");
	if (ret < 0)
		goto out;
	gpio_direction_input(spi->irq);

	err = request_irq(gpio_to_irq(spi->irq), ssp_hif_isr,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT, "ssp_hif", ssp);

	nrc_dbg(NRC_DBG_HIF, "--spi_gpio_irq Set Done");
	if (err) {
		dev_err(nw->dev, "can't get hif IRQ");
		goto ssp_free;
	}

	/* Prepare GDMA_READY pin */
	ret = gpio_request(spi_gdma_irq, "sysfs");
	gpio_direction_input(spi_gdma_irq);
	if (ret < 0) {
		nrc_dbg(NRC_DBG_HIF,
			"could not request ssp_gdma_ready gpio(%d)", ret);
		goto ssp_free;
	}

	set_timer(ssp->priv);

	goto out;


ssp_free:
	kfree(priv);
out:

	nrc_dbg(NRC_DBG_HIF, "-%s", __func__);
	return ret;
}

static int ssp_remove(struct spi_device *spi)
{
	struct nrc_ssp *ssp = spi_get_drvdata(spi);
	struct nrc_ssp_priv *priv;
	struct nrc_hif_ssp *hif = ssp->hif;

	priv = ssp->priv;

	nrc_dbg(NRC_DBG_HIF, "+%s", __func__);

	free_irq(gpio_to_irq(spi_gpio_irq), ssp);
	gpio_free(spi_gpio_irq);
	gpio_free(spi_gdma_irq);
	flush_workqueue(priv->rx_wq);
	flush_workqueue(priv->tx_wq);
	destroy_workqueue(priv->rx_wq);
	destroy_workqueue(priv->tx_wq);
	spi_set_drvdata(spi, NULL);

	del_timer(&hif->exp_timer);

	kfree(priv);
	nrc_dbg(NRC_DBG_HIF, "-%s", __func__);
	return 0;

}

static struct spi_driver ssp_driver = {
	.probe	= ssp_probe,
	.remove = ssp_remove,
	.driver = {
		.name = "nrc-nspi",
	},
};

static struct spi_board_info bi = {
	.modalias = "nrc-nspi",
	.chip_select = 0,
	.mode = SPI_MODE_1,
};

int nrc_ssp_register(struct nrc_hif_ssp *hif)
{
	int ret = 0;
	struct spi_device *spi;
	struct spi_master *master;

	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);

	g_ssp.hif = hif;
	hif->ssp = &g_ssp;

	/* Apply module parameters */
	bi.bus_num = spi_bus_num;
	bi.irq = spi_gpio_irq;
	bi.platform_data = hif->dev;
	bi.max_speed_hz = hifspeed;

	/* Find the spi master that our device is attached to */
	master = spi_busnum_to_master(spi_bus_num);
	if (!master) {
		nrc_dbg(NRC_DBG_HIF,
			"could not find spi master with busnum=%d\n",
			spi_bus_num);
		goto fail;
	}

	/* Instantiate and add a spi device */
	spi = spi_new_device(master, &bi);
	if (!spi) {
		nrc_dbg(NRC_DBG_HIF, "failed to add spi device\n");
		goto fail;
	}

	hif->ssp->spi = spi;

	/* Register spi driver */
	ret = spi_register_driver(&ssp_driver);
	if (ret < 0) {
		nrc_dbg(NRC_DBG_HIF, "failed to register driver %s\n",
			ssp_driver.driver.name);
		goto unregister_device;
	}

	return ret;

unregister_device:
	spi_unregister_device(spi);
fail:

	return -1;
}

void nrc_ssp_unregister(struct nrc_hif_ssp *hif)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);
	spi_unregister_device(hif->ssp->spi);
	spi_unregister_driver(&ssp_driver);
}
