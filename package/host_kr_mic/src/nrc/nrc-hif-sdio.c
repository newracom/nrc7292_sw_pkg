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
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <asm/unaligned.h>
#include <linux/smp.h>
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#include <linux/timekeeping.h>
#else
#include <linux/kthread.h>
#endif

#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sd.h>
#include <linux/string.h>

#include "nrc-fw.h"
#include "nrc-hif.h"
#include "wim.h"

//static bool once;
static atomic_t suspend;

#define SDIO_ADMA_EN	1


#define SDIO_512_INFO	0
#define TCN  (4)
#define TCNE (0)
#define CREDIT_AC0	(TCN*4+TCNE)	/* BK */
#define CREDIT_AC1	(TCN*4+TCNE)	/* BE */
#define CREDIT_AC2	(TCN*4+TCNE)	/* VI */
#define CREDIT_AC3	(TCN*4+TCNE)	/* VO */

/* only use for NRC7291 others are using same value */
#define CREDIT_AC3_7291	(TCN*2+TCNE)	/* VO */

/* N-SPI host-side memory map */
#define C_SPI_WAKE_UP 0x0
#define C_SPI_DEVICE_STATUS 0x1
#define C_SPI_CHIP_ID_HIGH 0x2
#define C_SPI_CHIP_ID_LOW 0x3
#define C_SPI_MODEM_ID 0x4
#define C_SPI_SOFTWARE_VERSION 0x8
#define C_SPI_BOARD_ID 0xc
#define C_SPI_EIRQ_MODE 0x10
#define C_SPI_EIRQ_ENABLE 0x11
#define C_SPI_EIRQ_STATUS_LATCH 0x12
#define C_SPI_EIRQ_STATUS 0x13
#define C_SPI_QUEUE_STATUS 0x14 /* 0x1f */
#define C_SPI_MESSAGE 0x20 /* 0x2f */

#define C_SPI_RXQ_THRESHOLD 0x30
#define C_SPI_RXQ_WINDOW 0x31

#define C_SPI_TXQ_THRESHOLD 0x40
#define C_SPI_TXQ_WINDOW 0x41

#define SW_MAGIC_FOR_BOOT	(0x01020716)
#define SW_MAGIC_FOR_FW		(0x01210630)

struct sdio_sys_reg {
	u8 wakeup;	/* 0x0 */
	u8 status;	/* 0x1 */
	u16 chip_id;	/* 0x2-0x3 */
	u32 modem_id;	/* 0x4-0x7 */
	u32 sw_id;	/* 0x8-0xb */
	u32 board_id;	/* 0xc-0xf */
} __packed;

struct sdio_status_reg {
	struct {
		u8 mode;
		u8 enable;
		u8 latched_status;
		u8 status;
	} eirq;
	u8 txq_status[6];
	u8 rxq_status[6];
	u32 msg[4];

#define EIRQ_IO_ENABLE	(1<<2)
#define EIRQ_EDGE	(1<<1)
#define EIRQ_ACTIVE_LO	(1<<0)

#define EIRQ_DEV_SLEEP	(1<<3)
#define EIRQ_DEV_READY	(1<<2)
#define EIRQ_RXQ	(1<<1)
#define EIRQ_TXQ	(1<<0)

#define TXQ_ERROR	(1<<7)
#define TXQ_SLOT_COUNT	(0x7F)
#define RXQ_SLOT_COUNT	(0x7F)

} __packed;

#define SPI_BUFFER_SIZE (496-20)

#define TX_SLOT 0
#define RX_SLOT 1

#define CREDIT_QUEUE_MAX (12)

#define  TRANSA_SDIO_DEBUG
#define  TRANSA_SDIO_VENDER  0x0296
#define  TRANSA_SDIO_DEVICE  0x5347

#define SDIO_ADDR_INFO		        (unsigned int)(0x101)
#define SDIO_ADDR_INFO_ASYNC		(unsigned int)(0x111)
#define SDIO_ADDR_DATA		        (unsigned int)(0x200)

static const struct sdio_device_id transa_sdio_dev[] = {
	{ SDIO_DEVICE(TRANSA_SDIO_VENDER, TRANSA_SDIO_DEVICE) },
	{},
};

struct sdio_data_t {
	unsigned int	  credit_vif0;
	unsigned int	  credit_vif1;
	unsigned int	  info_wr;
	unsigned int	  info_rd;
};

/* Object prepended to strut nrc_hif_device */
struct transa_sdio_priv_t {
	struct sdio_func   *func;

	/* work, kthread, ... */
	struct delayed_work work;
	struct task_struct *kthread;
	wait_queue_head_t wait; /* wait queue */

	struct {
		struct sdio_sys_reg    sys;
		struct sdio_status_reg status;
	} hw;

	spinlock_t lock;
	struct {
		unsigned int head;
		unsigned int tail;
		unsigned int size;
		unsigned int count;
	} slot[2];
	/* VIF0(AC0~AC3), BCN, CONC, VIF1(AC0~AC3), padding*/
	u8 front[CREDIT_QUEUE_MAX];
	u8 rear[CREDIT_QUEUE_MAX];
	u8 credit_max[CREDIT_QUEUE_MAX];

	unsigned long loopback_prev_cnt;
	unsigned long loopback_total_cnt;
	unsigned long loopback_last_jiffies;
	unsigned long loopback_read_usec;
	unsigned long loopback_write_usec;
	unsigned long loopback_measure_cnt;

	unsigned int	          recv_len;
	unsigned int	          recv_num;
	struct dentry *debugfs;

	unsigned int	  credit_vif0;
	unsigned int	  credit_vif1;

	struct sdio_data_t sdio_info;
	struct sk_buff  *skb_tx_last;
};

struct transa_sdio_priv_t *transa_sdio_priv;
static bool once;

#ifdef TRANSA_SDIO_DEBUG

static unsigned long get_tod_usec(void)
{
#if KERNEL_VERSION(5, 0, 0) > LINUX_VERSION_CODE
	struct timeval tv;

	do_gettimeofday(&tv);
	return tv.tv_usec;
#else
	return (unsigned long) ktime_to_us(ktime_get());
#endif
}

static int nrc_debugfs_sdio_read(void *data, u64 *val)
{
	int ret, cnt = 1;
	struct sk_buff *skb = dev_alloc_skb(2048);

	pr_info("[RRRRR] %s\n", __func__);

	sdio_claim_host(transa_sdio_priv->func);
	ret = sdio_memcpy_fromio(transa_sdio_priv->func, skb->data,
						    SDIO_ADDR_INFO, 0x10);
	sdio_release_host(transa_sdio_priv->func);
	print_hex_dump(KERN_DEBUG, "wangc: ",
					  DUMP_PREFIX_NONE, 8, 1,
					  skb->data, 16, false);
	sdio_claim_host(transa_sdio_priv->func);
	ret = sdio_memcpy_fromio(transa_sdio_priv->func, skb->data,
				   SDIO_ADDR_DATA, 0x20);
	sdio_release_host(transa_sdio_priv->func);
	print_hex_dump(KERN_DEBUG, "wangc: ",
					  DUMP_PREFIX_NONE, 8, 1,
					  skb->data, 16, false);
	pr_info("%s, ret: %d, cnt: %d\n", __func__,
				   ret, cnt);

	return ret;
}

static int nrc_debugfs_sdio_write(void *data1, u64 val)
{
	int ret = 0;
	struct sk_buff *skb, *skb2, *skb3;
	void *data;
	u64 i = val;

	pr_info("[WWWWW] %s, cout: %d\n", __func__, (int)i);

	skb = dev_alloc_skb(2048);
	skb2 = dev_alloc_skb(1536);
	skb3 = dev_alloc_skb(1536);

	memset(skb->data, 0x58, 2048);
	memset(skb2->data, 0x34, 1024);

	if (val == 0) {
		struct transa_sdio_priv_t *priv = transa_sdio_priv;
		int i = 0;

		while (ret == 0) {
			pr_info("[wangc] loop  count: %d\n",  i);
			sdio_claim_host(priv->func);
			ret = sdio_memcpy_toio(priv->func, 0xaa,
							   skb->data, 1562);
			ret = sdio_memcpy_toio(priv->func, 0xaa,
							   skb->data, 1562);
			ret = sdio_memcpy_toio(priv->func, 0xaa,
							   skb->data, 1562);
			ret = sdio_memcpy_toio(priv->func, 0xaa,
							   skb->data, 1562);
			sdio_release_host(priv->func);
			i++;
		}
	}

	if ((val == 123) || (val == 321)) {
		unsigned long t1, t2;
		struct transa_sdio_priv_t *priv = transa_sdio_priv;

		if (priv->loopback_total_cnt == 0) {
			priv->loopback_last_jiffies = jiffies;
			priv->loopback_read_usec = 0;
			priv->loopback_write_usec = 0;
			priv->loopback_measure_cnt = 0;
		}
		val = 0xFFFFFFFF;
		priv->recv_len = 512;

		while (val) {
			if (priv->loopback_measure_cnt % 2)
				data = (void *)skb->data;
			else
				data = (void *)skb2->data;

			t1 = get_tod_usec();
			sdio_claim_host(priv->func);
			ret = sdio_memcpy_toio(priv->func, 0xaa,
						   data, priv->recv_len);
			sdio_release_host(priv->func);
			t2 = get_tod_usec();
			if (t2 > t1)
				priv->loopback_write_usec += (t2 - t1);

			if (ret) {
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
				pr_info("[wangc]sdio host write ret: %d, count: %u, skb->data: %p\n",
					ret,
					(u32) (priv->loopback_measure_cnt % 2),
					data);
#endif
				print_hex_dump(KERN_DEBUG, "wangc: ",
						 DUMP_PREFIX_NONE, 16, 1,
						 data, 496, false);
				break;
			}
			priv->loopback_total_cnt += (priv->recv_len * 2);
			priv->loopback_measure_cnt++;

			if (time_after(jiffies,
					(priv->loopback_last_jiffies
					 + msecs_to_jiffies(1000)))) {
				unsigned long kilo_bits;

				kilo_bits = ((priv->loopback_total_cnt
						- priv->loopback_prev_cnt) * 8);
				kilo_bits = (kilo_bits / 1024);
				priv->loopback_read_usec /=
						  priv->loopback_measure_cnt;
				priv->loopback_write_usec /=
						  priv->loopback_measure_cnt;
				nrc_dbg(NRC_DBG_HIF, "loopback throughput(%d kbps @ %d)",
					 kilo_bits,
					 priv->func->card->host->ios.clock);

				pr_info("[wangc]loopback_measure_cnt : %d\n",
					 (int) priv->loopback_measure_cnt);

				priv->loopback_last_jiffies = jiffies;
				priv->loopback_prev_cnt =
						  priv->loopback_total_cnt;
				priv->loopback_measure_cnt = 0;
				val--;
			}
		}
		dev_kfree_skb(skb);
		dev_kfree_skb(skb2);
		dev_kfree_skb(skb3);

		return 0;
	}

	sdio_claim_host(transa_sdio_priv->func);
	ret = sdio_memcpy_toio(transa_sdio_priv->func, 0xaa,
						   skb->data, (int) i);
	sdio_release_host(transa_sdio_priv->func);
	pr_info("%s, ret: %d\n", __func__, ret);

	dev_kfree_skb(skb);
	dev_kfree_skb(skb2);
	dev_kfree_skb(skb3);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nrc_debugfs_sdio,
			nrc_debugfs_sdio_read,
			nrc_debugfs_sdio_write,
			"%llu\n");

void nrc_init_debugfs_sdio(void)
{
	pr_info("[XXXXX] %s\n", __func__);
	transa_sdio_priv->debugfs = debugfs_create_file("nrc_debugfs_sdio",
				0600, NULL, NULL, &nrc_debugfs_sdio);
}
#endif

static inline u16 sdio_num_slots(struct transa_sdio_priv_t *priv, int dir)
{
	return (priv->slot[dir].head - priv->slot[dir].tail);
}

/**
 * spi_rx_skb - fetch a single hif packet from the target
 */
int stop_tx;
static struct sk_buff *sdio_rx_skb(struct transa_sdio_priv_t *priv)
{
	struct sk_buff *skb = NULL;
	struct hif *hif;
	int ret;
	int recv_len;

#ifdef CONFIG_TRX_BACKOFF
	struct nrc_hif_device *hdev = sdio_get_drvdata(priv->func);
	struct nrc *nw = hdev->nw;
	int backoff;
#endif
	/* Wait until at least one rx slot is non-empty */
	ret = wait_event_interruptible(priv->wait,
			  (priv->recv_len > 1 || kthread_should_stop()));
	if (ret < 0)
		goto fail;

	if (kthread_should_stop())
		goto fail;

#ifdef CONFIG_TRX_BACKOFF
	if (!nw->ampdu_supported) {
		backoff = atomic_inc_return(&priv->trx_backoff);

		if ((backoff % 3) != 0) {
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
			nrc_dbg(NRC_DBG_HIF, "rx-irq: backoff=%d\n", backoff);
#endif
			usleep_range(800, 1000);
		}
	}
#endif
	recv_len = priv->recv_len;
	skb = dev_alloc_skb(recv_len);
	skb_put(skb, recv_len);
	sdio_claim_host(priv->func);
	ret = sdio_memcpy_fromio(priv->func, skb->data,
						  SDIO_ADDR_DATA, recv_len);
	if (ret) {
		print_hex_dump(KERN_DEBUG, "wangc-rx: ",
					   DUMP_PREFIX_NONE, 16, 1,
					   skb->data, skb->len, false);
		pr_info("[wangc-err] rx-len: %d\n", skb->len);
		stop_tx = 1;
		sdio_release_host(priv->func);
		goto fail;
	}

	priv->recv_len = 1;
	sdio_release_host(priv->func);

	/* Calculate how many more slot to read for this hif packet */
	hif = (void *)skb->data;

	if (hif->type >= HIF_TYPE_MAX || hif->len == 0) {
		nrc_dbg(NRC_DBG_HIF,
				"[wangc-error111]type: %d, len: %d, slot[%d/%d], info_rd: %d\n",
				hif->type, hif->len,
				priv->slot[RX_SLOT].head,
				priv->slot[RX_SLOT].tail,
				priv->sdio_info.info_rd);

		print_hex_dump(KERN_DEBUG, "rxskb ",
					   DUMP_PREFIX_NONE, 16, 1,
					   skb->data, skb->len,
					   false);
		msleep(200);
		BUG();
	}

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "rx-irq: skb=%p len:%d, hif_type=%d\n",
			skb, skb->len, hif->type);
#endif

	return skb;

fail:
	if (skb)
		dev_kfree_skb(skb);
	return NULL;
}


static void sdio_credit_skb(struct nrc_hif_device *hdev)
{
	struct transa_sdio_priv_t *priv = hdev->priv;
	struct sk_buff *skb;
	struct hif *hif;
	struct wim *wim;
	struct wim_credit_report *cr;
	u8 *p;
	int i;
	int size = sizeof(*hif) + sizeof(*wim) + sizeof(*cr);

	if (!once) {
		once = true;
		return;
	}

	skb = dev_alloc_skb(size);

	p = skb->data;
	hif = (void *)p;
	hif->type = HIF_TYPE_WIM;
	hif->subtype = HIF_WIM_SUB_EVENT;
	hif->vifindex = 0;
	hif->len = sizeof(*wim) + sizeof(*cr);

	p += sizeof(*hif);
	wim = (void *)p;
	wim->event = WIM_EVENT_CREDIT_REPORT;

	p += sizeof(*wim);
	cr = (void *)p;
	cr->h.type = WIM_TLV_AC_CREDIT_REPORT;
	cr->h.len = sizeof(struct wim_credit_report_param);

	cr->v.change_index = 0;

	for (i = 0; i < CREDIT_QUEUE_MAX; i++) {
		u8 room = priv->front[i] - priv->rear[i];

		room = min(priv->credit_max[i], room);
		cr->v.ac[i] = priv->credit_max[i] - room;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
		nrc_dbg(NRC_DBG_HIF, "credit[%d] %d f:%d, r:%d\n",
				i, cr->v.ac[i], priv->front[i],
				priv->rear[i]);
#endif
	}

	skb_put(skb, hif->len+sizeof(*hif));

	hdev->hif_ops->receive(hdev, skb);
}

static int sdio_update_status(struct nrc_hif_device *hdev)
{
	struct transa_sdio_priv_t *priv = hdev->priv;
	u32 rear;
	int ac, ret;

	sdio_claim_host(priv->func);
	ret = sdio_memcpy_fromio(priv->func, &priv->sdio_info,
					    SDIO_ADDR_INFO_ASYNC, 0x10);
	if (ret < 0) {
		pr_info("[wangc] %s, ret: %d\n", __func__, ret);
		sdio_release_host(priv->func);
		return ret;
	}
	priv->slot[TX_SLOT].head = priv->sdio_info.info_wr;
	priv->slot[RX_SLOT].head = priv->sdio_info.info_rd;

	if (hdev->nw->loopback) {
		sdio_release_host(priv->func);
		return 0;
	}

	if ((priv->sdio_info.credit_vif0 != priv->credit_vif0) ||
		(priv->sdio_info.credit_vif1 != priv->credit_vif1)) {
		/* Update VIF0 credit */
		rear = priv->sdio_info.credit_vif0;
		priv->credit_vif0 = rear;
		for (ac = 0; ac < 4; ac++)
			priv->rear[ac] = (rear >> 8*ac) & 0xff;

		/* Update VIF1 credit */
		rear = priv->sdio_info.credit_vif1;
		priv->credit_vif1 = rear;
		for (ac = 0; ac < 4; ac++)
			priv->rear[6+ac] = (rear >> 8*ac) & 0xff;

		sdio_release_host(priv->func);
		sdio_credit_skb(hdev);
	} else {
		sdio_release_host(priv->func);
	}

	return 0;
}

static void sdio_poll_status(struct work_struct *work)
{
	struct transa_sdio_priv_t *priv = container_of(to_delayed_work(work),
			struct transa_sdio_priv_t, work);
	struct sdio_func *func = priv->func;

	sdio_update_status(sdio_get_drvdata(func));
	wake_up_interruptible(&priv->wait);
}

static int sdio_loopback(struct transa_sdio_priv_t *priv, int count)
{
	struct sk_buff *skb;
	int ret;
	char str[32] = "0";
	unsigned long t1, t2;

	if (priv->loopback_total_cnt == 0) {
		priv->loopback_last_jiffies = jiffies;
		priv->loopback_read_usec = 0;
		priv->loopback_write_usec = 0;
		priv->loopback_measure_cnt = 0;
	}

	skb = dev_alloc_skb(1024);
	if (!skb)
		goto end;

	wait_event_interruptible(priv->wait,
			(priv->recv_num || kthread_should_stop()));

	t1 = get_tod_usec();
	sdio_claim_host(priv->func);
	ret = sdio_memcpy_fromio(priv->func, skb->data, 0xc00, priv->recv_len);
	sdio_release_host(priv->func);
	t2 = get_tod_usec();
	if (t2 > t1)
		priv->loopback_read_usec += (t2 - t1);

	if (ret) {
		pr_info("[wangc]sdio host read ret: %d\n", ret);
		pr_info("%s\n", str);
	}

	priv->recv_num = 0;

	t1 = get_tod_usec();
	sdio_claim_host(priv->func);
	ret = sdio_memcpy_toio(priv->func, 0xaa, skb->data, priv->recv_len);
	sdio_release_host(priv->func);
	t2 = get_tod_usec();
	if (t2 > t1)
		priv->loopback_write_usec += (t2 - t1);

	if (ret) {
		pr_info("[wangc]sdio host write ret: %d\n", ret);
		pr_info("%s\n", str);
	}

	dev_kfree_skb(skb);
	priv->loopback_total_cnt += (priv->recv_len * 2);
	priv->loopback_measure_cnt++;

	if (time_after(jiffies,
			(priv->loopback_last_jiffies
			 + msecs_to_jiffies(1000)))) {
		unsigned long kilo_bits;

		kilo_bits = ((priv->loopback_total_cnt
					- priv->loopback_prev_cnt) * 8);
		kilo_bits = (kilo_bits / 1024);
		priv->loopback_read_usec /= priv->loopback_measure_cnt;
		priv->loopback_write_usec /= priv->loopback_measure_cnt;
		nrc_dbg(NRC_DBG_HIF, "loopback throughput(%d kbps @ %d), packet len: %d",
				kilo_bits,
				priv->func->card->host->ios.clock,
				priv->recv_len);

		priv->loopback_last_jiffies = jiffies;
		priv->loopback_prev_cnt = priv->loopback_total_cnt;
		priv->loopback_measure_cnt = 0;
	}

	return 1;
end:
	if (skb)
		dev_kfree_skb(skb);

	return ret;
}

static int sdio_rx_thread(void *data)
{
	struct nrc_hif_device *hdev = data;
	struct transa_sdio_priv_t *priv = (void *)(hdev + 1);
	struct nrc *nw = hdev->nw;
	struct sk_buff *skb;
	int is_suspend;
	int ret;

	pr_info("[wangc]rx thread entry, loopbakc: %d\n", nw->loopback);

	while (!kthread_should_stop()) {
		if (nw->loopback == 1) {
			nrc_debugfs_sdio_write(NULL, 123);
			ret = sdio_loopback(priv, nw->lb_count);
			if (ret <= 0)
				nrc_dbg(NRC_DBG_HIF, "loopback (%d) error.\n",
					   ret);
			continue;
		}

		skb = sdio_rx_skb(priv);
		is_suspend = atomic_read(&suspend);
		if (skb && is_suspend) {
			dev_kfree_skb(skb);
			atomic_set(&suspend, 0);
		} else if (skb) {
			hdev->hif_ops->receive(hdev, skb);
		} else {
			nw->drv_state = NRC_DRV_CLOSING;
			pr_info("[wangc]rx-head: %d, rx-tail: %d, rx-cnt: %d\n",
					priv->slot[RX_SLOT].head,
					priv->slot[RX_SLOT].tail,
					priv->sdio_info.info_rd);
			break;
		}
	}
	pr_info("rx thread exit\n");

	return 0;
}

/* hif lower interface */
static const char *sdio_name(struct nrc_hif_device *dev)
{
	return "sdio";
}


static int sdio_noop(struct nrc_hif_device *dev)
{
	return 0;
}

static int sdio_suspend(struct nrc_hif_device *dev)
{
	atomic_set(&suspend, 1);
	return 0;
}

static int sdio_resume(struct nrc_hif_device *dev)
{
	atomic_set(&suspend, 0);
	return 0;
}

static bool sdio_check_fw(struct nrc_hif_device *hdev)
{
	return 1;
}

static int sdio_xmit(struct nrc_hif_device *hdev, struct sk_buff *skb)
{
	struct nrc *nw = hdev->nw;
	struct transa_sdio_priv_t *priv = hdev->priv;
	int ret, nr_slot = DIV_ROUND_UP(skb->len, priv->slot[TX_SLOT].size);
#ifdef CONFIG_TRX_BACKOFF
	int backoff;
#endif
	struct hif_hdr *hif;
	struct frame_hdr *fh;
	int n = 0, rev = 0;

	hif = (void *)skb->data;
	fh = (void *)(hif + 1);

	if (nw->drv_state == NRC_DRV_CLOSING)
		return 0;

	if (nw->loopback)
		return 0;

	if (stop_tx)
		return 0;

	schedule_delayed_work(&priv->work, msecs_to_jiffies(5));
	ret = wait_event_interruptible(priv->wait,
			sdio_num_slots(priv, TX_SLOT) >= nr_slot);
	if (ret < 0)
		return ret;

	cancel_delayed_work_sync(&priv->work);

	if (nw->drv_state == NRC_DRV_CLOSING)
		return 0;
	priv->slot[TX_SLOT].tail += nr_slot;
	priv->slot[TX_SLOT].tail &= 0xFFFF;

	if ((hif->type == HIF_TYPE_FRAME)
			&& ((hif->subtype == HIF_FRAME_SUB_DATA_BE)
				|| (hif->subtype == HIF_FRAME_SUB_MGMT)))
		priv->front[fh->flags.tx.ac] += nr_slot;

#ifdef CONFIG_TRX_BACKOFF
	if (!nw->ampdu_supported) {
		backoff = atomic_inc_return(&priv->trx_backoff);

		if ((backoff % 3) == 0) {
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
			nrc_dbg(NRC_DBG_HIF, "spi_xmit: backoff=%d\n",
					backoff);
#endif
			usleep_range(800, 1000);
		}
	}
#endif
	/* Yes, I know we are accessing beyound skb->data + skb->len */
		n = skb->len;
		rev = 0;

		sdio_claim_host(priv->func);
		if (n) {
			ret = sdio_memcpy_toio(priv->func, SDIO_ADDR_DATA,
				    skb->data, n);
			if (ret) {
				stop_tx = 1;
				pr_info("[wangc]%s, len = %d, nr_slot = %d\n",
						   __func__, n * 512, nr_slot);
				print_hex_dump(KERN_DEBUG, "wangc-tx: ",
					   DUMP_PREFIX_NONE, 32, 1, skb->data,
					   64, false);
				return ret;
			}
		}

		if (rev) {
			if (n)
				udelay(200);
			ret = sdio_memcpy_toio(priv->func, SDIO_ADDR_DATA,
						   skb->data + n * 512,  rev);
			if (ret) {
				stop_tx = 1;
				pr_info("[wangc]%s, len = %d, nr_slot = %d\n",
						   __func__, rev, nr_slot);
				print_hex_dump(KERN_DEBUG, "wangc-tx: ",
						  DUMP_PREFIX_NONE, 32, 1,
						  skb->data, 64, false);
				return ret;
			}
		}
		sdio_release_host(priv->func);
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF,
	"spi_xmit ac=%d skb=%p, slot=%d(%d/%d),fwpend:%d/%d, pending=%d\n",
			fh->flags.tx.ac, skb, nr_slot,
			priv->slot[TX_SLOT].head, priv->slot[TX_SLOT].tail,
			priv->front[fh->flags.tx.ac],
			priv->rear[fh->flags.tx.ac],
			skb_queue_len(&hdev->queue[0]));
#endif
#ifdef CONFIG_FAST_CREDIT_UPDATE
	sdio_update_status(hdev);
#endif
	return (ret == 0) ? HIF_TX_COMPLETE : ret;
}

static int
sdio_raw_write(struct nrc_hif_device *hdev,	const u8 *data,
				    const u32 len)
{
	return 0;
}

static int sdio_wait_ack(struct nrc_hif_device *hdev, u8 *data, u32 len)
{
	return 0;
}

static int sdio_start(struct nrc_hif_device *hdev)
{
	int ret;
	struct transa_sdio_priv_t *priv = hdev->priv;

	/* Start rx thread */
	if (hdev->nw->loopback == 1)
		return 0;

	priv->kthread = kthread_run(sdio_rx_thread, hdev, "sdio-rx");

	if (hdev->nw->loopback == 1)
		return 0;


	INIT_DELAYED_WORK(&priv->work, sdio_poll_status);

	sdio_claim_host(priv->func);
	ret = sdio_update_status(hdev);
	sdio_release_host(priv->func);
	atomic_set(&suspend, 0);
	pr_info("[wangc]%s\n", __func__);

	return ret;
}


static void sdio_close(struct nrc_hif_device *hdev)
{
	struct transa_sdio_priv_t *priv = hdev->priv;

	priv->slot[TX_SLOT].count = 999;
	kthread_stop(priv->kthread);
	wake_up_interruptible(&priv->wait);

	pr_info("[XXXXXXXXXX] %s\n", __func__);
}


void sdio_reset(struct nrc_hif_device *hdev)
{
	pr_info("[XXXXXXXXXX] %s\n", __func__);
}


void sdio_wakeup(struct nrc_hif_device *hdev)
{
	pr_info("[XXXXXXXXXX] %s\n", __func__);
}


int sdio_test(struct nrc_hif_device *hdev)
{
	pr_info("[XXXXXXXXXX] %s\n", __func__);
	return 0;
}

void transa_sdio_irq_handler(struct sdio_func *func)
{
	struct nrc_hif_device     *hdev = sdio_get_drvdata(func);
	struct transa_sdio_priv_t *priv = hdev->priv;
	u32 rear;
	unsigned char lowbyte, highbyte;
	int ret, ac;

	sdio_claim_host(priv->func);
	lowbyte  = sdio_readb(priv->func, 0x00, &ret);
	highbyte = sdio_readb(priv->func, 0x01, &ret);
	priv->recv_len  = (highbyte << 8) | lowbyte;

	if (priv->recv_len == 1) {
		ret = sdio_memcpy_fromio(priv->func, &priv->sdio_info,
				   SDIO_ADDR_INFO, 0x10 /*priv->recv_len*/);

		if (ret < 0) {
			pr_info("[wangc] %s, info-ret: %d\n", __func__, ret);
			sdio_release_host(priv->func);
			return;
		}
		priv->slot[TX_SLOT].head = priv->sdio_info.info_wr;
		priv->slot[RX_SLOT].head = priv->sdio_info.info_rd;

		if ((priv->sdio_info.credit_vif0 != priv->credit_vif0) ||
			(priv->sdio_info.credit_vif1 != priv->credit_vif1)) {
			/* Update VIF0 credit */
			rear = priv->sdio_info.credit_vif0;
			priv->credit_vif0 = rear;
			for (ac = 0; ac < 4; ac++)
				priv->rear[ac] = (rear >> 8*ac) & 0xff;


			/* Update VIF1 credit */
			rear = priv->sdio_info.credit_vif1;
			priv->credit_vif1 = rear;
			for (ac = 0; ac < 4; ac++)
				priv->rear[6+ac] = (rear >> 8*ac) & 0xff;

			//need_credit = 1;
			sdio_release_host(priv->func);
			sdio_credit_skb(hdev);
			sdio_claim_host(func);
		}
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
		nrc_dbg(NRC_DBG_HIF, "-%s\n", __func__);
#endif
	}
	sdio_release_host(priv->func);
	wake_up_interruptible(&priv->wait);
}

#define SDIO_BLK_SIZE		(512)
static int transa_sdio_probe(struct sdio_func *func,
					 const struct sdio_device_id *id)
{
	struct transa_sdio_priv_t *priv = transa_sdio_priv;
	int ret = 0;

	priv->func = func;

	priv->loopback_prev_cnt = 0;
	priv->loopback_total_cnt = 0;
	priv->loopback_last_jiffies = 0;
	priv->loopback_read_usec = 0;
	priv->loopback_write_usec = 0;

	//SDK version
	priv->slot[TX_SLOT].size = 456;
	priv->slot[RX_SLOT].size = 492;

	priv->credit_max[0] = CREDIT_AC0;
	priv->credit_max[1] = CREDIT_AC1;
	priv->credit_max[2] = CREDIT_AC2;
	priv->credit_max[3] = CREDIT_AC3;

	priv->credit_max[6] = CREDIT_AC0;
	priv->credit_max[7] = CREDIT_AC1;
	priv->credit_max[8] = CREDIT_AC2;
	priv->credit_max[9] = CREDIT_AC3;

	init_waitqueue_head(&priv->wait);

	func->max_blksize = SDIO_BLK_SIZE;

	sdio_claim_host(func);

	ret = sdio_enable_func(func);
	if (ret) {
		nrc_dbg(NRC_DBG_HIF, "failed to enable sdio func\n");
		sdio_release_host(func);
		return ret;
	}

	ret = sdio_set_block_size(func, SDIO_BLK_SIZE);
	if (ret) {
		nrc_dbg(NRC_DBG_HIF, "failed to set sdio func block size\n");
		sdio_release_host(func);
		return ret;
	}

	ret = sdio_claim_irq(func, transa_sdio_irq_handler);
	if (ret) {
		nrc_dbg(NRC_DBG_HIF, "failed to claim sdio irq\n");
		sdio_release_host(func);
		return ret;
	}

	sdio_release_host(func);

	priv->skb_tx_last = dev_alloc_skb(512);

#ifdef CONFIG_TRX_BACKOFF
	atomic_set(&priv->trx_backoff, 0);
#endif

#ifdef TRANSA_SDIO_DEBUG
	nrc_init_debugfs_sdio();
#endif

	return ret;
}

static void transa_sdio_remove(struct sdio_func *func)
{
	sdio_claim_host(func);
	sdio_release_irq(func);
	sdio_disable_func(func);
	sdio_release_host(func);
	debugfs_remove(transa_sdio_priv->debugfs);

	nrc_dbg(NRC_DBG_HIF, "%s\n", __func__);
}

static struct nrc_hif_ops transa_sdio_ops = {
	.name     = sdio_name,
	.check_fw = sdio_check_fw,
	.xmit     = sdio_xmit,
	.write    = sdio_raw_write,
	.wait_ack = sdio_wait_ack,
	.start    = sdio_start,
	.stop     = sdio_noop,
	.suspend  = sdio_suspend,
	.resume   = sdio_resume,
	.close    = sdio_close,
	.reset    = sdio_reset,
	.wakeup   = sdio_wakeup,
	.test     = sdio_test,
};


static struct sdio_driver transa_sdio_driver = {
	.name     = "transa_sdio",
	.id_table = transa_sdio_dev,
	.probe    = transa_sdio_probe,
	.remove   = transa_sdio_remove,
};

struct nrc_hif_device *nrc_hif_sdio_init(void)
{
	struct nrc_hif_device     *hdev;
	struct transa_sdio_priv_t *priv;

	hdev = kzalloc(sizeof(*hdev) + sizeof(*priv), GFP_KERNEL);
	if (!hdev)
		return NULL;

	priv          = (void *)(hdev + 1);
	hdev->priv    = priv;
	hdev->hif_ops = &transa_sdio_ops;

	transa_sdio_priv = priv;
	transa_sdio_ops.sync_auto = true;

	/* Register sdio driver */
	if (sdio_register_driver(&transa_sdio_driver)) {
		nrc_dbg(NRC_DBG_HIF, "failed to register driver %s\n",
					   transa_sdio_driver.name);
		goto failed_register_driver;
	}

	sdio_set_drvdata(priv->func, hdev);
	return hdev;

failed_register_driver:

	kfree(hdev);
	transa_sdio_priv = NULL;
	return NULL;
}

int nrc_hif_sdio_exit(struct nrc_hif_device *hdev)
{
	nrc_dbg(NRC_DBG_HIF, "%s\n", __func__);

	if (!hdev)
		return 0;

	sdio_unregister_driver(&transa_sdio_driver);
	kfree(hdev);

	return 0;
}


