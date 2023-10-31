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
#include <net/mac80211.h>
#include <asm/unaligned.h>
#include <linux/smp.h>
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#include <linux/timekeeping.h>
#else
#include <linux/kthread.h>
#endif

#include "nrc-init.h"
#include "nrc-fw.h"
#include "nrc-hif.h"
#include "nrc-debug.h"
#include "nrc-vendor.h"
#include "nrc-mac80211.h"
#include "nrc-stats.h"
#include "wim.h"

static bool once;
static bool cspi_suspend;
static atomic_t irq_enabled;
static u16 total_sta=0; /* total number of STA  connected */
static u16 remain_sta=0; /* number of STA remaining after clearing STA */
#define TCN  (2*1)
#define TCNE (0)
#define CREDIT_AC0		(TCN*2+TCNE)	/* BK (4) */
#define CREDIT_AC1		(TCN*20+TCNE)	/* BE (40) */
#define CREDIT_AC1_20	(TCN*10+TCNE)	/* BE (20) */
#define CREDIT_AC1_80	(TCN*40+TCNE)	/* BE (80) */
#define CREDIT_AC2		(TCN*4+TCNE)	/* VI (8) */
#define CREDIT_AC3		(TCN*4+TCNE)	/* VO(8) */

/* N-SPI host-side memory map */
#define C_SPI_SYS_REG 0x0
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

#define CSPI_EIRQ_MODE 0x05
#define CSPI_EIRQ_Q_ENABLE	0x3
#define CSPI_EIRQ_R_ENABLE	0x4
#define CSPI_EIRQ_S_ENABLE	0x8
#define CSPI_EIRQ_A_ENABLE 	(CSPI_EIRQ_Q_ENABLE|CSPI_EIRQ_R_ENABLE|CSPI_EIRQ_S_ENABLE)
/*#define CSPI_EIRQ_ENABLE 0x16*/ /* disable tx/rx que */

struct spi_sys_reg {
	u8 wakeup;	/* 0x0 */
	u8 status;	/* 0x1 */
	u16 chip_id;	/* 0x2-0x3 */
	u32 modem_id;	/* 0x4-0x7 */
	u32 sw_id;	/* 0x8-0xb */
	u32 board_id;	/* 0xc-0xf */
} __packed;

struct spi_status_reg {
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

/* C-SPI command
 *
 * [31:24]: start byte (0x50)
 * [23:23]: burst (0: single, 1: burst)
 * [22:22]: direction (0: read, 1: write)
 * [21:21]: fixed (0: incremental, 1: fixed)
 * [20:13]: address
 * [12:0]: length (for multi-word transfer)
 * [7:0]: wdata (for single write)
 */
#define C_SPI_READ	0x50000000
#define C_SPI_WRITE	0x50400000
#define C_SPI_BURST	0x00800000
#define C_SPI_FIXED	0x00200000
#define C_SPI_ADDR(x)	(((x) & 0xff) << 13)
#define C_SPI_LEN(x)	((x) & 0x1fff)
#define C_SPI_WDATA(x)	((x) & 0xff)
#define C_SPI_ACK	0x47

#define TX_SLOT 0
#define RX_SLOT 1

#define CREDIT_QUEUE_MAX (12)

/*#define SPI_DBG (13)*/

/* Object prepended to strut nrc_hif_device */
struct nrc_spi_priv {
	struct spi_device *spi;
	struct nrc_hif_device *hdev;

	/* work, kthread, ... */
	struct delayed_work work;
	struct task_struct *kthread;
	wait_queue_head_t wait; /* wait queue */

#if !defined(CONFIG_SUPPORT_THREADED_IRQ)
	struct workqueue_struct *irq_wq;
	struct work_struct irq_work;
#endif

	struct {
		struct spi_sys_reg sys;
		struct spi_status_reg status;
	} hw;

	spinlock_t lock;
	struct {
		u16 head;
		u16 tail;
		u16 size;
		u16 count;
	} slot[2];

	/* VIF0(AC0~AC3), BCN, CONC, VIF1(AC0~AC3), padding*/
	int hw_queues;
	u8 front[CREDIT_QUEUE_MAX];
	u8 rear[CREDIT_QUEUE_MAX];
	u8 credit_max[CREDIT_QUEUE_MAX];
#ifdef CONFIG_TRX_BACKOFF
	atomic_t trx_backoff;
#endif
	unsigned long loopback_prev_cnt;
	unsigned long loopback_total_cnt;
	unsigned long loopback_last_jiffies;
	unsigned long loopback_read_usec;
	unsigned long loopback_write_usec;
	unsigned long loopback_measure_cnt;
	struct mutex bus_lock_mutex;
	struct nrc_cspi_ops *ops;

	int polling_interval;
	struct task_struct *polling_kthread;
};

struct nrc_cspi_ops {
	int (*read_regs)(struct spi_device *spi,
		u8 addr, u8 *buf, ssize_t size);
	int (*write_reg)(struct spi_device *spi, u8 addr, u8 data);
	ssize_t (*read)(struct spi_device *spi, u8 *buf, ssize_t size);
	ssize_t (*write)(struct spi_device *spi, u8 *buf, ssize_t size);
};

int spi_test(struct nrc_hif_device *hdev);
void spi_reset_device (struct nrc_hif_device *hdev);
void spi_reset_rx (struct nrc_hif_device *hdev);
static int spi_update_status(struct spi_device *spi);
static void c_spi_enable_irq(struct spi_device *spi, bool enable, u8 mask);
static void c_spi_config(struct nrc_spi_priv *priv);;
static void spi_config_fw(struct nrc_hif_device *dev);
static void spi_enable_irq(struct nrc_hif_device *hdev);

static u8 crc7(u8 seed, u8 data)
{
	int i;
	const u8 g = 0x89;

	seed ^= data;
	for (i = 0; i < 8; i++) {
		if (seed & 0x80)
			seed ^= g;
		seed <<= 1;
	}
	return seed;
}

static u8 compute_crc7(const u8 *data, ssize_t len)
{
	int i;
	u8 crc = 0;

	for (i = 0; i < len; i++)
		crc = crc7(crc, data[i]);

	return crc >> 1;
}

void nrc_hif_cspi_read_credit(struct nrc_hif_device *hdev, int q, int *p_front, int *p_rear, int *p_credit)
{
	struct nrc_spi_priv *priv = hdev->priv;
	*p_front = priv->front[q];
	*p_rear = priv->rear[q];
	*p_credit = priv->credit_max[q];
}


static void get_sta_cnt(void *data,  struct ieee80211_sta *sta)
{
	struct ieee80211_vif *vif = data;

	if (!sta || !vif) {
		nrc_dbg(NRC_DBG_STATE, "%s Invalid argument", __func__);
		return;
	}

	if(!ieee80211_find_sta(vif, sta->addr))
		return;

	++remain_sta;
	nrc_dbg(NRC_DBG_STATE, "(AP Recovery) remaining sta_cnt:%d", remain_sta);
}

static void prepare_deauth_sta(void *data,  struct ieee80211_sta *sta)
{
	struct nrc_sta *i_sta = to_i_sta(sta);
	struct ieee80211_hw *hw = i_sta->nw->hw;
	struct ieee80211_vif *vif = data;
	struct sk_buff *skb = NULL;

	if (!sta || !vif) {
		nrc_dbg(NRC_DBG_STATE, "%s Invalid argument", __func__);
		return;
	}

	if(!ieee80211_find_sta(vif, sta->addr))
		return;

	/* (AP Recovry) Pretend to receive a deauth from @sta */
	skb = ieee80211_deauth_get(hw, vif->addr, sta->addr, vif->addr,
			WLAN_REASON_DEAUTH_LEAVING, sta, false);
	if (!skb) {
		nrc_dbg(NRC_DBG_STATE, "%s Fail to alloc skb", __func__);
		return;
	}
	nrc_dbg(NRC_DBG_STATE, "(AP Recovery) Disconnect STA(%pM) by force", sta->addr);
	ieee80211_rx_irqsafe(hw, skb);

	++total_sta;
}

static inline void spi_set_transfer(struct spi_transfer *xfer,
				void *tx, void *rx, int len)
{
	xfer->tx_buf = tx;
	xfer->rx_buf = rx;
	xfer->len = len;
}

#ifdef CONFIG_SUPPORT_SPI_SYNC_TRANSFER
#else
static inline void
spi_message_init_with_transfers(struct spi_message *m,
				struct spi_transfer *xfers, unsigned int num_xfers)
{
	unsigned int i;

	spi_message_init(m);
	for (i = 0; i < num_xfers; ++i)
		spi_message_add_tail(&xfers[i], m);
}

static inline int
spi_sync_transfer(struct spi_device *spi, struct spi_transfer *xfers,
	unsigned int num_xfers)
{
	struct spi_message msg;

	spi_message_init_with_transfers(&msg, xfers, num_xfers);

	return spi_sync(spi, &msg);
}
#endif

static int _c_spi_write_dummy(struct spi_device *spi)
{
	struct spi_transfer xfer[2] = {{0},};
	u32 dummy=0xffffffff;
	u8 tx[8];
#ifndef CONFIG_SPI_HALF_DUPLEX
	u8 rx[8];
#endif
	ssize_t status;

	memset(tx, 0xff, sizeof(tx));
#ifndef CONFIG_SPI_HALF_DUPLEX
	spi_set_transfer(&xfer[0], tx, rx, 8);
#else
	spi_set_transfer(&xfer[0], tx, NULL, 8);
#endif
	dummy = 0xffffffff;
	spi_set_transfer(&xfer[1], &dummy, NULL, sizeof(dummy));

	status = spi_sync_transfer(spi, xfer, 2);

	return 0;
}

static int _c_spi_read_regs(struct spi_device *spi,
		u8 addr, u8 *buf, ssize_t size)
{
	struct spi_transfer xfer[4] = {{0},};
	u32 cmd, crc, dummy;
	u8 tx[8];
#ifndef CONFIG_SPI_HALF_DUPLEX
	u8 rx[8];
#endif
	ssize_t status;
	int arr_len;

	if (size == 0 || buf == NULL)
		return -EINVAL;

	cmd = C_SPI_READ | C_SPI_ADDR(addr);
	if (size > 1)
		cmd |= C_SPI_BURST | C_SPI_LEN(size);
	else
		cmd |= C_SPI_FIXED | C_SPI_LEN(0);

	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;

#ifndef CONFIG_SPI_HALF_DUPLEX
	spi_set_transfer(&xfer[0], tx, rx, 8);
#else
	spi_set_transfer(&xfer[0], tx, NULL, 8);
#endif
	spi_set_transfer(&xfer[1], NULL, buf, size);
	spi_set_transfer(&xfer[2], NULL, &crc, sizeof(crc));

	dummy = 0xffffffff;
	if (size > 1)
		spi_set_transfer(&xfer[3], &dummy, NULL, sizeof(dummy));
	else
		spi_set_transfer(&xfer[1], &dummy, NULL, sizeof(dummy));

	arr_len = (size > 1) ? ARRAY_SIZE(xfer) : 2;
	status = spi_sync_transfer(spi, xfer, arr_len);
	if (status < 0) {
		pr_err("[%s] reading spi failed(%zd).", __func__, status);
		return status;
	}

#ifndef CONFIG_SPI_HALF_DUPLEX
	if (WARN_ON_ONCE(rx[7] != C_SPI_ACK)) {
		nrc_common_dbg("[%s] try to read register but SPI ACK is invalid\n", __func__);
		return -EIO;
	}
#endif

#ifndef CONFIG_SPI_HALF_DUPLEX
	if (size == 1)
		buf[0] = rx[6];
#else
	/* Half duplex can't handle size 1 and 2 */
	if (size <= 2)
		nrc_common_dbg("[%s] Half duplex can't handle size 1 and 2...\n", __func__);
#endif
	return 0;
}

static int _c_spi_write_reg(struct spi_device *spi, u8 addr, u8 data)
{
	struct spi_transfer xfer[2] = {{0},};
	u32 cmd, dummy;
	u8 tx[8];
#ifndef CONFIG_SPI_HALF_DUPLEX
	u8 rx[8];
#endif
	ssize_t status;

	cmd = C_SPI_WRITE | C_SPI_FIXED | C_SPI_ADDR(addr) | C_SPI_WDATA(data);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;
#ifndef CONFIG_SPI_HALF_DUPLEX
	spi_set_transfer(&xfer[0], tx, rx, 8);
#else
	spi_set_transfer(&xfer[0], tx, NULL, 8);
#endif
	dummy = 0xffffffff;
	spi_set_transfer(&xfer[1], &dummy, NULL, sizeof(dummy));

	status = spi_sync_transfer(spi, xfer, 2);
	if (status < 0) {
		pr_err("[%s] writing spi failed(%zd).", __func__, status);
		return status;
	}

#ifndef CONFIG_SPI_HALF_DUPLEX
	/* In case of spi reset, skip a process for confirming spi ack */
	if (C_SPI_WDATA(data) != 0xC8) {
		if (WARN_ON_ONCE(rx[7] != C_SPI_ACK)) {
			nrc_common_dbg("[%s] try to read register but SPI ACK is invalid\n", __func__);
			return -EIO;
		}
	}
#endif

	return 0;
}

static ssize_t _c_spi_read(struct spi_device *spi, u8 *buf, ssize_t size)
{
	struct spi_transfer xfer[4] = {{0},};
	u32 cmd, crc, dummy;
	u8 tx[8];
#ifndef CONFIG_SPI_HALF_DUPLEX
	u8 rx[8];
#endif
	ssize_t status;

	if (size == 0 || buf == NULL)
		return -EINVAL;

	cmd = C_SPI_READ | C_SPI_BURST | C_SPI_FIXED;
	cmd |= C_SPI_ADDR(C_SPI_TXQ_WINDOW) | C_SPI_LEN(size);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;
	tx[5] = 0xff;

#ifndef CONFIG_SPI_HALF_DUPLEX
	spi_set_transfer(&xfer[0], tx, rx, 8);
#else
	spi_set_transfer(&xfer[0], tx, NULL, 8);
#endif
	spi_set_transfer(&xfer[1], NULL, buf, size);
	spi_set_transfer(&xfer[2], NULL, &crc, sizeof(crc));

	dummy = 0xffffffff;
	spi_set_transfer(&xfer[3], &dummy, NULL, sizeof(dummy));
	status = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (status < 0) {
		pr_err("[%s] reading spi failed(%zd).", __func__, status);
		return status;
	}

#ifndef CONFIG_SPI_HALF_DUPLEX
	if (WARN_ON_ONCE(rx[7] != C_SPI_ACK)) {
		nrc_common_dbg("[%s] try to read register but SPI ACK is invalid\n", __func__);
		return -EIO;
	}
#endif

	return size;
}

static ssize_t _c_spi_write(struct spi_device *spi, u8 *buf, ssize_t size)
{

	struct spi_transfer xfer[4] = {{0},};
	u32 cmd, dummy = 0xffffffff;
	u8 tx[8];
#ifndef CONFIG_SPI_HALF_DUPLEX
	u8 rx[8];
#endif
	ssize_t status;

	if (size == 0 || buf == NULL)
		return -EINVAL;

	cmd = C_SPI_WRITE | C_SPI_BURST | C_SPI_FIXED;
	cmd |= C_SPI_ADDR(C_SPI_RXQ_WINDOW) | C_SPI_LEN(size);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;
	tx[5] = 0xff;

#ifndef CONFIG_SPI_HALF_DUPLEX
	spi_set_transfer(&xfer[0], tx, rx, 8);
#else
	spi_set_transfer(&xfer[0], tx, NULL, 8);
#endif
	spi_set_transfer(&xfer[1], buf, NULL, size);
	spi_set_transfer(&xfer[2], &dummy, NULL, sizeof(dummy));
	dummy = 0xffffffff;
	spi_set_transfer(&xfer[3], &dummy, NULL, sizeof(dummy));

	status = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (status < 0)
	{
		pr_err("[%s] writing spi failed(%zd).", __func__, status);
		return status;
	}

#ifndef CONFIG_SPI_HALF_DUPLEX
	if (WARN_ON_ONCE(rx[7] != C_SPI_ACK))
	{
		// nrc_common_dbg("[%s] try to read register but SPI ACK is invalid\n", __func__);
		return -EIO;
	}
#endif

	return size;
}

static struct nrc_cspi_ops cspi_ops = {
	.read_regs = _c_spi_read_regs,
	.write_reg = _c_spi_write_reg,
	.read = _c_spi_read,
	.write = _c_spi_write,
};

static int c_spi_read_regs(struct spi_device *spi,
		u8 addr, u8 *buf, ssize_t size)
{
	struct nrc_spi_priv *priv = dev_get_platdata(&spi->dev);

	WARN_ON(!priv->ops);
	WARN_ON(!priv->ops->read_regs);

	return (*priv->ops->read_regs)(spi, addr, buf, size);
}

static int c_spi_write_reg(struct spi_device *spi, u8 addr, u8 data)
{
	struct nrc_spi_priv *priv = dev_get_platdata(&spi->dev);

	WARN_ON(!priv->ops);
	WARN_ON(!priv->ops->write_reg);

	return (*priv->ops->write_reg)(spi, addr, data);
}

static ssize_t c_spi_read(struct spi_device *spi, u8 *buf, ssize_t size)
{
	struct nrc_spi_priv *priv = dev_get_platdata(&spi->dev);

	WARN_ON(!priv->ops);
	WARN_ON(!priv->ops->read);

	return (*priv->ops->read)(spi, buf, size);
}

static ssize_t c_spi_write(struct spi_device *spi, u8 *buf, ssize_t size)
{
	struct nrc_spi_priv *priv = dev_get_platdata(&spi->dev);

	WARN_ON(!priv->ops);
	WARN_ON(!priv->ops->write);

	return (*priv->ops->write)(spi, buf, size);
}


static inline u16 c_spi_num_slots(struct nrc_spi_priv *priv, int dir)
{
	return (priv->slot[dir].head - priv->slot[dir].tail);
}

/**
 * spi_rx_skb - fetch a single hif packet from the target
 */

static struct sk_buff *spi_rx_skb(struct spi_device *spi,
				struct nrc_spi_priv *priv)
{
	struct sk_buff *skb;
	struct hif *hif;
	ssize_t size;
	u32 nr_slot;
	int ret;
	u32 second_length = 0;
	struct nrc *nw = spi_get_drvdata(spi);
	struct nrc_hif_device *hdev = nw->hif;
#ifdef CONFIG_TRX_BACKOFF
	int backoff;
#endif
	static uint cnt1 = 0;
	static uint cnt2 = 0;
	static const int def_slot = 4;

	skb = dev_alloc_skb(priv->slot[RX_SLOT].size * def_slot);
	if (!skb)
		goto fail;

	if (c_spi_num_slots(priv, RX_SLOT) == 0)
		spi_update_status(priv->spi);

	/* Wait until at least one rx slot is non-empty */
	ret = wait_event_interruptible(priv->wait,
			((c_spi_num_slots(priv, RX_SLOT) > 0) ||
			 kthread_should_stop() || kthread_should_park()));
	if (ret < 0)
		goto fail;

	if (kthread_should_stop() || kthread_should_park())
		goto fail;

#ifdef CONFIG_TRX_BACKOFF
	if (!nw->ampdu_supported) {
		backoff = atomic_inc_return(&priv->trx_backoff);

		if ((backoff % 3) != 0) {
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
			nrc_dbg(NRC_DBG_HIF, "rx-irq: backoff=%d", backoff);
#endif
			usleep_range(800, 1000);
		}
	}
#endif
	SYNC_LOCK(hdev);

	if (c_spi_num_slots(priv, RX_SLOT) > 32) {
		SYNC_UNLOCK(hdev);
		if (cnt1++ < 10) {
			pr_err("!!!!! garbage rx data");
			spi_reset_rx(hdev);
		}
		goto fail;
	}
	cnt1 = 0;

	/*
	 * For the first time, the slot should be read entirely
	 * since we cannot know the data size until the hif->len is checked.
	 * And, the current RX_SLOT size is already word aligned(456 bytes).
	 */
	priv->slot[RX_SLOT].tail++;
	size = c_spi_read(spi, skb->data, priv->slot[RX_SLOT].size);
	SYNC_UNLOCK(hdev);
	if (size < 0) {
		priv->slot[RX_SLOT].tail--;
		goto fail;
	}

	/* Calculate how many more slot to read for this hif packet */
	hif = (void *)skb->data;


	if (hif->type >= HIF_TYPE_MAX || hif->len == 0) {
		nrc_dbg(NRC_DBG_HIF, "rxslot:(h=%d,t=%d)",
				priv->slot[RX_SLOT].head, priv->slot[RX_SLOT].tail);
		print_hex_dump(KERN_DEBUG, "rxskb ", DUMP_PREFIX_NONE, 16, 1,
				skb->data, 480, false);
		spi_reset_rx(hdev);
		goto fail;
	}

	nr_slot = DIV_ROUND_UP(sizeof(*hif) + hif->len, priv->slot[RX_SLOT].size);

	if (nr_slot >= def_slot) {
		struct sk_buff *skb2 = dev_alloc_skb(
				priv->slot[RX_SLOT].size * (nr_slot+1));

		memcpy(skb2->data, skb->data, priv->slot[RX_SLOT].size);
		dev_kfree_skb(skb);
		skb = skb2;
		hif = (void *)skb->data;
	}

	nr_slot--;

	if (nr_slot == 0)
		goto out;

	if (c_spi_num_slots(priv, RX_SLOT) < nr_slot)
		spi_update_status(priv->spi);

	/*
	 * Block until priv->nr_rx_slot >= nr_slot).
	 * The irq thread will wake me up.
	 */
	ret = wait_event_interruptible(priv->wait,
				(c_spi_num_slots(priv, RX_SLOT) >= nr_slot) ||
				kthread_should_stop() || kthread_should_park());
	if (ret < 0)
		goto fail;

	if (kthread_should_stop() || kthread_should_park())
		goto fail;

	priv->slot[RX_SLOT].tail += nr_slot;

	second_length = hif->len + sizeof(*hif) - priv->slot[RX_SLOT].size;
	/*
	 * If it's necessary to read more data over other slots,
	 * the size to read must be a multiple of 4.
	 * because the HSPI HW stores the data length as a word unit.
	 */
	if (second_length & 0x3) {
		second_length = (second_length + 4) & 0xFFFFFFFC;
	}

	SYNC_LOCK(hdev);
	if (c_spi_num_slots(priv, RX_SLOT) > 32) {
		SYNC_UNLOCK(hdev);
		if (cnt2++ < 10) {
			pr_err("@@@@@@ garbage rx data");
		}
		spi_reset_rx(hdev);
		goto fail;
	}
	cnt2 = 0;
	size = c_spi_read(spi, skb->data + priv->slot[RX_SLOT].size,
			second_length);
	SYNC_UNLOCK(hdev);

	if (size < 0)
		goto fail;

out:
	skb_put(skb, sizeof(*hif) + hif->len);
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "rx-irq: skb=%p len:%d, hif_type=%d",
			skb, skb->len, hif->type);
#endif
	return skb;

fail:
	if (skb)
		dev_kfree_skb(skb);
	return NULL;
}

/* Use this function in other function */
static int spi_read_sys_reg (struct spi_device *spi, struct spi_sys_reg *sys)
{
	int ret;

	ret = c_spi_read_regs(spi, C_SPI_SYS_REG, (void *)sys,
								sizeof(struct spi_sys_reg));

	if (ret) {
		dev_err(&spi->dev, "Fail to c_spi_read_regs\n");
		return -1;
	}

	sys->chip_id = be16_to_cpu(sys->chip_id);
	sys->modem_id = be32_to_cpu(sys->modem_id);
	sys->sw_id = be32_to_cpu(sys->sw_id);
	sys->board_id = be32_to_cpu(sys->board_id);

	return 0;
}

static void spi_set_default_credit(struct nrc_spi_priv *priv)
{
	int i;

	for (i = 0; i < CREDIT_QUEUE_MAX; i++)
		priv->credit_max[i] = 0;

	switch (priv->hw.sys.chip_id) {
	case 0x7292:
	case 0x7394:
		priv->credit_max[0] = CREDIT_AC0;
		priv->credit_max[1] = CREDIT_AC1_80;
		priv->credit_max[2] = CREDIT_AC2;
		priv->credit_max[3] = CREDIT_AC3;
		priv->credit_max[5] = CREDIT_AC1;

		priv->credit_max[6] = CREDIT_AC0;
		priv->credit_max[7] = CREDIT_AC1_80;
		priv->credit_max[8] = CREDIT_AC2;
		priv->credit_max[9] = CREDIT_AC3;
		break;

	case 0x7391:
	case 0x7392:
	case 0x4791:
	case 0x5291:
		priv->credit_max[0] = 4;
		priv->credit_max[1] = CREDIT_AC1_20;
		priv->credit_max[2] = 4;
		priv->credit_max[3] = 4;

		priv->credit_max[6] = 4;
		priv->credit_max[7] = CREDIT_AC1_20;
		priv->credit_max[8] = 4;
		priv->credit_max[9] = 4;
		break;
	}

	for (i = 0; i < CREDIT_QUEUE_MAX; i++)  {
		nrc_dbg(NRC_DBG_HIF, "credit[%2d] :%3d", i, priv->credit_max[i]);
	}
}


static void spi_credit_skb(struct spi_device *spi)
{
	struct nrc *nw = spi_get_drvdata(spi);
	struct nrc_hif_device *hdev = nw->hif;
	struct nrc_spi_priv *priv = hdev->priv;
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
		u8 room = 0;
		if (priv->front[i] >= priv->rear[i]) {
			room = priv->front[i] - priv->rear[i];
		} else {
			room = (255 - priv->rear[i]) + priv->front[i];
		}

		room = min(priv->credit_max[i], room);
		cr->v.ac[i] = priv->credit_max[i] - room;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
		nrc_dbg(NRC_DBG_HIF, "credit[%d] %d f:%d, r:%d",
				i, cr->v.ac[i], priv->front[i],
				priv->rear[i]);
#endif
	}

	skb_put(skb, hif->len+sizeof(*hif));

	hdev->hif_ops->receive(hdev, skb);
}

/**
 * spi_loopback - fetch a single hif packet from the target
 *					and send it back
 */

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

static int spi_loopback(struct spi_device *spi,
				struct nrc_spi_priv *priv, int lb_cnt)
{
	struct sk_buff *skb;
	ssize_t size;
	u32 nr_slot;
	u32 second_length = 0;
	int ret = 0;
	unsigned long t1, t2;
	struct hif *hif;

	if (priv->loopback_total_cnt == 0) {
		priv->loopback_last_jiffies = jiffies;
		priv->loopback_read_usec = 0;
		priv->loopback_write_usec = 0;
		priv->loopback_measure_cnt = 0;
	}

	skb = dev_alloc_skb(priv->slot[RX_SLOT].size * lb_cnt);
	if (!skb)
		goto end;

	t1 = get_tod_usec();

	/* Wait until at least one rx slot is non-empty */
	ret = wait_event_interruptible(priv->wait,
			(c_spi_num_slots(priv, RX_SLOT) >= lb_cnt
			 || kthread_should_stop()));

	if (ret < 0)
		goto end;

	if (kthread_should_stop())
		goto end;

	priv->slot[RX_SLOT].tail++;
	size = c_spi_read(spi, skb->data, priv->slot[RX_SLOT].size + 4);
	if (size < 0) {
		priv->slot[RX_SLOT].tail--;
		goto end;
	}

	hif = (void*)skb->data;

	nr_slot = DIV_ROUND_UP(sizeof(*hif) + hif->len,
			priv->slot[RX_SLOT].size);

	t2 = get_tod_usec();

	if (t2 > t1)
		priv->loopback_read_usec += (t2 - t1);

	t1 = get_tod_usec();

	nr_slot--;
	if (nr_slot == 0)
		goto loopback_tx;

	priv->slot[RX_SLOT].tail += nr_slot;
	second_length = hif->len + sizeof(*hif) - priv->slot[RX_SLOT].size;
	/* README: align with 4bytes dummy data */
	second_length = (second_length + 4) & 0xFFFFFFFC;
	size = c_spi_read(spi, skb->data + priv->slot[RX_SLOT].size,
			second_length);

loopback_tx:
	schedule_delayed_work(&priv->work, msecs_to_jiffies(5));
	ret = wait_event_interruptible(priv->wait,
			c_spi_num_slots(priv, TX_SLOT) >= lb_cnt);
	if (ret < 0)
		goto end;

	cancel_delayed_work_sync(&priv->work);
	priv->slot[TX_SLOT].tail += lb_cnt;

	ret = c_spi_write(priv->spi, skb->data,
			(priv->slot[TX_SLOT].size * lb_cnt));
	if (ret < 0)
		goto end;

	t2 = get_tod_usec();

	if (t2 > t1)
		priv->loopback_write_usec += (t2 - t1);

	dev_kfree_skb(skb);
	priv->loopback_total_cnt += ((priv->slot[TX_SLOT].size * lb_cnt) * 2);
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
		nrc_dbg(NRC_DBG_HIF, "loopback throughput(%d kbps @ %d)",
				kilo_bits, spi->max_speed_hz);

		priv->loopback_last_jiffies = jiffies;
		priv->loopback_prev_cnt = priv->loopback_total_cnt;
		priv->loopback_measure_cnt = 0;
	}
end:
	if(skb)
		dev_kfree_skb(skb);

	return ret;
}

/**
 * spi_rx_thread
 *
 */
static int spi_rx_thread(void *data)
{
	struct nrc_hif_device *hdev = data;
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	struct sk_buff *skb;
	struct hif *hif;
	struct nrc *nw = hdev->nw;
	int ret;

	while (!kthread_should_stop()) {
		if (nw->loopback) {
			ret = spi_loopback(spi, priv, nw->lb_count);
			if (ret <= 0)
				nrc_dbg(NRC_DBG_HIF,
						"loopback (%d) error.", ret);
			continue;
		}

		if (!kthread_should_park()) {
			skb = spi_rx_skb(spi, priv);
			if (!skb) continue;

			hif = (void *)skb->data;
			if (hif->type != HIF_TYPE_LOOPBACK && cspi_suspend) {
				dev_kfree_skb(skb);
			} else {
				hdev->hif_ops->receive(hdev, skb);
			}
		} else {
			nrc_dbg(NRC_DBG_HIF, "spi_rx_thread parked.");
			kthread_parkme();
			/*
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			*/
		}
	}
	return 0;
}

static int spi_read_status(struct spi_device *spi)
{
	struct spi_status_reg status;
	//struct spi_status_reg *priv = &status;

	c_spi_read_regs(spi, C_SPI_EIRQ_MODE, (void *)&status,
			sizeof(status));
	//spi_print_status(priv);

	return 0;
}

static int spi_update_status(struct spi_device *spi)
{
	struct nrc *nw = spi_get_drvdata(spi);
	struct nrc_hif_device *hdev = nw->hif;
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_status_reg *status = &priv->hw.status;

	int ret, ac, i, is_relay, retry_cnt=0;
	u32 rear;
	u16 cleared_sta=0;

#if defined(CONFIG_SUPPORT_BD)
	struct regulatory_request request;
#endif

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	//int index = 0;
	int cpuid = smp_processor_id();
	nrc_dbg(NRC_DBG_HIF, "+[%d] %s", cpuid,  __func__);
#endif

	if (cspi_suspend) {
		return 0;
	}

	if ((priv->hw.sys.chip_id == 0x7393) ||
		(priv->hw.sys.chip_id == 0x7394)) {
		if (nw->drv_state == NRC_DRV_PS) c_spi_enable_irq(spi, true, CSPI_EIRQ_A_ENABLE);
	}

	SYNC_LOCK(hdev);
	ret = c_spi_read_regs(spi, C_SPI_EIRQ_MODE, (void *)status, sizeof(*status));
	SYNC_UNLOCK(hdev);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	// for debugging
	nrc_dbg(NRC_DBG_HIF, "drv_state:%d status:0x%02x mode:0x%02x enable:0x%02x 0x%08X",
		nw->drv_state, status->eirq.status, status->eirq.mode,
		status->eirq.enable, status->msg[3]);
#endif

	/**
	 * The EIRQ_STATUS_DEVICE_READY interrupt typically occurs when the target is
	 * in a stopped state and does not overlap with EIRQ_STATUS_DEVICE_SLEEP.
	 * On the other hand, the TARGET_NOTI_FAILED_TO_ENTER_PS interrupt occurs
	 * when the target is functioning normally and can occur simultaneously
	 * with credit/slot update interrupts.
	 */
	if ((status->eirq.status & EIRQ_STATUS_DEVICE_READY) &&
		((status->msg[3] & 0xffff) == TARGET_NOTI_FAILED_TO_ENTER_PS)) {
		nrc_ps_dbg("TARGET_NOTI_FAILED_TO_ENTER_PS");
		atomic_set(&nw->fw_state, NRC_FW_ACTIVE);
		nrc_hif_resume(hdev);
		goto done;
	}

	/* update */
	if (status->eirq.status & EIRQ_STATUS_DEVICE_SLEEP) {
		if ((status->msg[3] & 0xffff) == TARGET_NOTI_REQUEST_FW_DOWNLOAD) {
			// for debugging - will be removed
			nrc_dbg(NRC_DBG_COMMON, "!!!!!!-----drv_state:%d status:0x%02x mode:0x%02x enable:0x%02x 0x%08X",
				nw->drv_state, status->eirq.status, status->eirq.mode,
				status->eirq.enable, status->msg[3]);
		}
	}

	/* 7292 : update, 7393/7394 : check WDT/FWDW and update */
	if (status->eirq.status == EIRQ_STATUS_DEVICE_ROM) {
		if (priv->hw.sys.chip_id == 0x7393 ||priv->hw.sys.chip_id == 0x7394) {
			if (status->msg[3] == TARGET_NOTI_WDT_EXPIRED) {
				nrc_dbg(NRC_DBG_HIF, "WDT EXPIRE msg[3]=0x%X, eirq.status:0x%X",
					status->msg[3], status->eirq.status);
				c_spi_enable_irq(spi, true, CSPI_EIRQ_A_ENABLE);
				goto done;
			}
		}
		goto update;
	}

	/* Device Ready after FW download, WDT Reset, or Deep sleep*/
	if (status->eirq.status & EIRQ_STATUS_DEVICE_READY) {
#if defined(CONFIG_SUPPORT_BD)
		request.alpha2[0] = nw->alpha2[0];
		request.alpha2[1] = nw->alpha2[1];
		request.initiator = NL80211_REGDOM_SET_BY_DRIVER;
#endif

		if (ieee80211_hw_check(nw->hw, SUPPORTS_DYNAMIC_PS) &&
			nw->drv_state >= NRC_DRV_RUNNING &&
			nw->hw->conf.dynamic_ps_timeout > 0) {
			mod_timer(&nw->dynamic_ps_timer,
				jiffies + msecs_to_jiffies(nw->hw->conf.dynamic_ps_timeout));
		}

		/* Handling Status events */
		switch (status->msg[3] & 0xffff) {
			case TARGET_NOTI_REQUEST_FW_DOWNLOAD:
				/* uCode requested the fw downloading. */
				nrc_ps_dbg("TARGET_NOTI_REQUEST_FW_DOWNLOAD");
				/* Do NOT send other frames to target
					while downloading FW to target_rom(7292/7394) or rom_cspi(7393)*/
				atomic_set(&nw->fw_state, NRC_FW_LOADING);
				if (nrc_check_fw_file(nw)) {
					nrc_download_fw(nw);
					spi_enable_irq(hdev);
					spi_config_fw(hdev);
					nrc_release_fw(nw);
				}
				break;

			case TARGET_NOTI_W_DISABLE_ASSERTED:
				ieee80211_connection_loss(nw->vif[0]);
				mdelay(300);
				nrc_hif_cleanup(nw->hif);
				nrc_mac_clean_txq(nw);
				nrc_mac_cancel_hw_scan(nw->hw, nw->vif[0]);
				break;

			case TARGET_NOTI_WDT_EXPIRED:
				nw->drv_state = NRC_DRV_REBOOT;
				nrc_dbg(NRC_DBG_STATE, "TARGET_NOTI_WDT_EXPIRED");
				spi_config_fw(hdev);
				nrc_hif_cleanup(nw->hif);
				nrc_mac_clean_txq(nw);
				break;

			case TARGET_NOTI_FW_READY_FROM_WDT:
				nrc_dbg(NRC_DBG_STATE, "TARGET_NOTI_FW_READY_FROM_WDT");
				is_relay = (nw->vif[0] && nw->vif[1]);
				spi_config_fw(hdev);
				for (i = 0; i < ARRAY_SIZE(nw->vif); i++) {
					if (nw->vif[i]) {
						if (nw->vif[i]->type == NL80211_IFTYPE_STATION) {
							/*
							* The target was rebooted due to watchdog.
							* Then, driver always requests deauth for trying to reconnect.
							*/
							if ((int)atomic_read(&nw->fw_state) != NRC_FW_ACTIVE) {
								atomic_set(&nw->fw_state, NRC_FW_ACTIVE);
								nrc_hif_resume(hdev);
							}
							nrc_dbg(NRC_DBG_STATE, "STA(%d) : Reconnect to AP", i);
							mdelay(300);
							nrc_mac_cancel_hw_scan(nw->hw, nw->vif[i]);
							ieee80211_connection_loss(nw->vif[i]);
							if(!is_relay) {
								goto no_restart;
							} else {
								nrc_free_vif_index(nw, nw->vif[i]);
							}
						} else if (nw->vif[i]->type == NL80211_IFTYPE_AP) {
							ieee80211_iterate_stations_atomic(nw->hw, prepare_deauth_sta, (void *)nw->vif[i]);
							nrc_dbg(NRC_DBG_STATE, "AP(%d) : Now try to clear all STAs(total cnt:%d)", i, total_sta);
							while (1) {
								//wait for all the sta are locally deauthenticated by mac80211
								if (!total_sta) break;
								msleep(2000);
								remain_sta = 0;
								ieee80211_iterate_stations_atomic(nw->hw, get_sta_cnt, (void *)nw->vif[i]);
								cleared_sta = total_sta - remain_sta;
								if (!remain_sta) {
									nrc_dbg(NRC_DBG_STATE, "Completed! (Remaining STA cnt:%d)", remain_sta);
									remain_sta = 0;
									break;
								}
								retry_cnt++;
								if (retry_cnt > 10) {
									nrc_dbg(NRC_DBG_STATE, "10 Trials but fail to clear STAs on mac80211. Reset by Force (Remaining STA cnt:%d)",
										remain_sta);
									break;
								}
								nrc_dbg(NRC_DBG_STATE, "NOT completed yet. Try again. (cleared STA:%d vs remained STA:%d, retry_cnt:%d)",
									cleared_sta, remain_sta, retry_cnt);
								total_sta = 0;
								ieee80211_iterate_stations_atomic(nw->hw, prepare_deauth_sta, (void *)nw->vif[i]);
							}
							nrc_dbg(NRC_DBG_STATE, "All STAs are cleared.(retry_cnt:%d remaining sta cnt:%d)",
								retry_cnt, nrc_stats_report_count());
							total_sta = 0;
							mdelay(5000); //it's for STA's reconnect by CQM
							nrc_hif_cleanup(nw->hif);
							nrc_mac_clean_txq(nw);
							nrc_free_vif_index(nw, nw->vif[i]);
						}
						else if (nw->vif[i]->type == NL80211_IFTYPE_MESH_POINT) {
							nrc_dbg(NRC_DBG_STATE, "mesh(%d) : Restart and do not repeering", i);
							nrc_hif_cleanup(nw->hif);
							nrc_mac_clean_txq(nw);
							nrc_mac_flush_txq(nw);
						}
#ifdef CONFIG_S1G_CHANNEL
						init_s1g_channels(nw);
#endif /* #ifdef CONFIG_S1G_CHANNEL */
					}
				}

				nrc_dbg(NRC_DBG_STATE, "Restart hw because target reset by WDT");
				ieee80211_restart_hw(nw->hw);
no_restart:
				nw->drv_state = NRC_DRV_RUNNING;

#if defined(CONFIG_SUPPORT_BD)
				nrc_ps_dbg("[%s,L%d] load board data", __func__, __LINE__);
				nrc_reg_notifier(nw->hw->wiphy, &request);
#endif
				goto update;

			case TARGET_NOTI_FW_READY_FROM_PS:
				/*
				 * FW notified the target reboot is done.
				 */
				nrc_ps_dbg("TARGET_NOTI_FW_READY_FROM_PS");
				if (fw_name == NULL) {
					/* It's for CSPI RAM Mode (no FW download) */
					spi_config_fw(hdev);
				}
				atomic_set(&nw->fw_state, NRC_FW_ACTIVE);
				nrc_hif_resume(hdev);
#if defined(CONFIG_SUPPORT_BD)
				nrc_ps_dbg("[%s,L%d] load board data", __func__, __LINE__);
				nrc_reg_notifier(nw->hw->wiphy, &request);
#endif
#ifdef CONFIG_S1G_CHANNEL
				init_s1g_channels(nw);
#endif /* #ifdef CONFIG_S1G_CHANNEL */
				if (ieee80211_hw_check(nw->hw, SUPPORTS_DYNAMIC_PS)) {
					if (nw->drv_state >= NRC_DRV_RUNNING &&
						nw->hw->conf.dynamic_ps_timeout > 0) {
						mod_timer(&nw->dynamic_ps_timer,
							jiffies + msecs_to_jiffies(nw->hw->conf.dynamic_ps_timeout));
					}
				} else if (ieee80211_hw_check(nw->hw, SUPPORTS_PS)) {
				} else {
					if (power_save >= NRC_PS_DEEPSLEEP_NONTIM) {
						if (!atomic_read(&nw->d_deauth.delayed_deauth))
							nrc_send_beacon_loss(nw);
					}
					else
						nw->invoke_beacon_loss = true;
				}
				if (!disable_cqm) {
					mod_timer(&nw->bcn_mon_timer,
						jiffies + msecs_to_jiffies(nw->beacon_timeout));
				}

				if (atomic_read(&nw->d_deauth.delayed_deauth)) {
					struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(nw->d_deauth.deauth_frm);
					struct ieee80211_key_conf *key = txi->control.hw_key;
					struct sk_buff *skb;
					int i;

					/* Send deauth frame */
					nrc_xmit_frame(nw, nw->d_deauth.vif_index, nw->d_deauth.aid, nw->d_deauth.deauth_frm);
					nw->d_deauth.deauth_frm = NULL;
					msleep(50);

					/* Finalize data : Common routine */
					if (nw->d_deauth.p.flags & IEEE80211_KEY_FLAG_PAIRWISE)
						nrc_wim_install_key(nw, DISABLE_KEY, &nw->d_deauth.v, &nw->d_deauth.s, &nw->d_deauth.p);
					else if (key)
						nrc_wim_install_key(nw, DISABLE_KEY, &nw->d_deauth.v, &nw->d_deauth.s, &nw->d_deauth.g);
					nrc_mac_sta_remove(nw->hw, &nw->d_deauth.v, &nw->d_deauth.s);
					nrc_mac_bss_info_changed(nw->hw, &nw->d_deauth.v, &nw->d_deauth.b, 0x80309f);
					for (i=0; i < IEEE80211_NUM_ACS; i++) {
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
#ifdef CONFIG_USE_LINK_ID
						nrc_mac_conf_tx(nw->hw, &nw->d_deauth.v,
						    nw->vif[nw->d_deauth.vif_index]->bss_conf.link_id,
						    i, &nw->d_deauth.tqp[i]);
#else
						nrc_mac_conf_tx(nw->hw, &nw->d_deauth.v, i,
						    &nw->d_deauth.tqp[i]);
#endif /* ifdef CONFIG_USE_LINK_ID */
#else
						nrc_mac_conf_tx(nw->hw, i, &nw->d_deauth.tqp[i]);
#endif
					}
					skb = nrc_wim_alloc_skb(nw, WIM_CMD_SET, WIM_MAX_SIZE);
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
					nrc_mac_add_tlv_channel(skb, &nw->d_deauth.c);
#else
					nrc_mac_add_tlv_channel(skb, &nw->d_deauth.c);
#endif
					nrc_xmit_wim_request(nw, skb);
					if (nw->d_deauth.p.flags & IEEE80211_KEY_FLAG_PAIRWISE && key)
						nrc_wim_install_key(nw, DISABLE_KEY, &nw->d_deauth.v, &nw->d_deauth.s, &nw->d_deauth.g);

					/* Remove interface : when 'ifconfig wlan0 down' or 'rmmod' */
					if (nw->d_deauth.removed) {
						nrc_wim_unset_sta_type(nw, &nw->d_deauth.v);
						nw->vif[nw->d_deauth.vif_index] = NULL;
						nw->enable_vif[nw->d_deauth.vif_index] = false;
						atomic_set(&nw->d_deauth.delayed_deauth, 0);
						nrc_mac_stop(nw->hw);
					}
					while (atomic_read(&nw->d_deauth.delayed_deauth)) {
						atomic_set(&nw->d_deauth.delayed_deauth, 0);
					}
				}
				break;
		}

		/* don't update slot and credit */
		goto done;
	}

update:
	/* update slot according to register set by target */
	priv->slot[TX_SLOT].count = status->rxq_status[1] & RXQ_SLOT_COUNT;
	priv->slot[RX_SLOT].count = status->txq_status[1] & TXQ_SLOT_COUNT;
	priv->slot[RX_SLOT].head = __be32_to_cpu(status->msg[0]) & 0xffff;
	priv->slot[TX_SLOT].head = __be32_to_cpu(status->msg[0]) >> 16;

	if (c_spi_num_slots(priv, TX_SLOT) > 32) {
		nrc_dbg(NRC_DBG_COMMON,"[%s,L%d]* TX_gap:%u head:%u vs tail:%u",
			__func__, __LINE__, c_spi_num_slots(priv, TX_SLOT),
			priv->slot[TX_SLOT].head, priv->slot[TX_SLOT].tail);
		priv->slot[TX_SLOT].tail = priv->slot[TX_SLOT].head = 0;
	}

	if (c_spi_num_slots(priv, RX_SLOT) > 33) {
		nrc_dbg(NRC_DBG_COMMON,"[%s,L%d]* RX_gap:%u head:%u vs tail:%u",
			__func__, __LINE__, c_spi_num_slots(priv, RX_SLOT),
			priv->slot[RX_SLOT].head, priv->slot[RX_SLOT].tail);
		//priv->slot[RX_SLOT].tail = priv->slot[RX_SLOT].head = 0;
		spi_reset_rx(hdev);
	}

	/* no need to update credit while loopback test */
	if (nw->loopback) {
		goto done;
	}

	/* Update VIF0 credit */
	rear = __be32_to_cpu(status->msg[1]);
	for (ac = 0; ac < 4; ac++) {
		priv->rear[ac] = (rear >> 8*ac) & 0xff;
	}

	/* Update VIF1 credit */
	rear = __be32_to_cpu(status->msg[2]);
	if (nw->hw_queues == 6) {
		for (ac = 0; ac < 4; ac++)
			priv->rear[4+ac] = (rear >> 8*ac) & 0xff; /* Actually rear[5] is used for GP */
	} else if (nw->hw_queues == 11) {
		for (ac = 0; ac < 4; ac++)
			priv->rear[6+ac] = (rear >> 8*ac) & 0xff;
	} else {
		//pr_err("Invalid queue (%d)\n", nw->hw_queues);
		//BUG();
	}

	/* For flow control debug */
	if (dbg_flow_control) {
		nrc_dbg(NRC_DBG_HIF, "+%s", __func__);
		//RX : HEAD (count of enqueue on target) vs TAIL (count of dequeue on host)
		nrc_dbg(NRC_DBG_HIF,"* [SLOT] RX(t_snt:%5d h_rcv:%5d diff:%2d)",
			priv->slot[RX_SLOT].head, priv->slot[RX_SLOT].tail,
			priv->slot[RX_SLOT].head -priv->slot[RX_SLOT].tail);

		//TX : HEAD (count of queue ready on target) vs TAIL (count of dequeue on host)
		nrc_dbg(NRC_DBG_HIF,"* [SLOT] TX(t_rdy:%5d h_snt:%5d diff:%2d)",
			priv->slot[TX_SLOT].head, priv->slot[TX_SLOT].tail,
			priv->slot[TX_SLOT].head -priv->slot[TX_SLOT].tail);

		/*TX CREDIT : front (count of enqueue per AC on host) vs
			rear (count of dequeue (free) per AC on target) */
		for (ac = 0; ac < nw->hw_queues; ac++) {
			nrc_dbg(NRC_DBG_HIF, "* [CREDIT_AC%d] (h_snt:%3d t_rcv:%3d) (credit:%3d pend:%3d)",
				ac, priv->front[ac], priv->rear[ac], nw->tx_credit[ac], nw->tx_pend[ac]);
		}
		nrc_dbg(NRC_DBG_HIF, "-%s", __func__);
	}

	spi_credit_skb(spi);

done:
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "-%s", __func__);
#endif

	return 0;
}

#ifdef CONFIG_SUPPORT_THREADED_IRQ
/* threaded irq handler */
static irqreturn_t spi_irq(int irq, void *data)
{
	struct nrc_hif_device *hdev = data;
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "%s", __func__);
#endif

	spi_update_status(spi);

	wake_up_interruptible(&priv->wait);

	return IRQ_HANDLED;
}
#else
static irqreturn_t spi_irq(int irq, void *data)
{
	struct nrc_hif_device *hdev = data;
	struct nrc_spi_priv *priv = hdev->priv;

	queue_work(priv->irq_wq, &priv->irq_work);

	return IRQ_HANDLED;
}

static void irq_worker(struct work_struct *work)
{
	struct nrc_spi_priv *priv = container_of(work,
			struct nrc_spi_priv, irq_work);
	struct spi_device *spi = priv->spi;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "%s", __func__);
#endif

	spi_update_status(spi);
	wake_up_interruptible(&priv->wait);
}
#endif

/* hif lower interface */
static const char *spi_name(struct nrc_hif_device *dev)
{
	return "c-spi";
}

static bool spi_check_fw(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_sys_reg sys;
	int ret;

	nrc_dbg(NRC_DBG_HIF, "[%s++]", __func__);
	ret = spi_read_sys_reg(priv->spi, &sys);
	nrc_dbg(NRC_DBG_HIF, "[%s, %d] sys.sw_id = 0x%x",
			__func__, __LINE__, sys.sw_id);
	if (ret < 0) {
		nrc_dbg(NRC_DBG_HIF, "failed to read register 0x0");
		return false;
	}
	nrc_dbg(NRC_DBG_HIF, "[%s--]", __func__);
	return (sys.sw_id == SW_MAGIC_FOR_BOOT);
}

static int spi_raw_write(struct nrc_hif_device *hdev,
		const u8 *data, const u32 len)
{
	struct nrc_spi_priv *priv = hdev->priv;

	c_spi_write(priv->spi, (u8 *)data, (u32)len);
	return HIF_TX_COMPLETE;
}

static int spi_wait_ack(struct nrc_hif_device *hdev, u8 *data, u32 len)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_status_reg status;
	int ret;

	do {
		ret = c_spi_read_regs(priv->spi, C_SPI_EIRQ_MODE,
				(void *)&status, sizeof(status));
		if (ret < 0) {
			nrc_dbg(NRC_DBG_HIF, "[%s, %d] Failed to read regs",
					__func__, __LINE__);
			return ret;
		}
	} while ((status.rxq_status[1] & RXQ_SLOT_COUNT) < 1);
	return 0;
}

static int spi_wait_for_xmit(struct nrc_hif_device *hdev, struct sk_buff *skb)
{
	struct nrc_spi_priv *priv = hdev->priv;
	int nr_slot = DIV_ROUND_UP(skb->len, priv->slot[TX_SLOT].size);
	int ret;

	if (c_spi_num_slots(priv, TX_SLOT) >= nr_slot)
		return 0;

	spi_update_status(priv->spi);
	if (c_spi_num_slots(priv, TX_SLOT) >= nr_slot) {
		wake_up_interruptible(&priv->wait);
		return 0;
	}

//	schedule_delayed_work(&priv->work, msecs_to_jiffies(5));

	ret = wait_event_interruptible_timeout(priv->wait,
			(c_spi_num_slots(priv, TX_SLOT) >= nr_slot) ||
				kthread_should_stop(), 5*HZ);
	if (ret == 0) { /* Timeout */
		nrc_mac_dbg("spi xmit timeout\n");
		return -1;
	}
	if (ret < 0)
		return ret;

//	cancel_delayed_work_sync(&priv->work);

	return 0;
}

static int spi_xmit(struct nrc_hif_device *hdev, struct sk_buff *skb)
{
	struct nrc *nw = hdev->nw;
	struct nrc_spi_priv *priv = hdev->priv;
	//struct spi_device *spi = priv->spi;

	int ret, nr_slot = DIV_ROUND_UP(skb->len, priv->slot[TX_SLOT].size);
#ifdef CONFIG_TRX_BACKOFF
	int backoff;
#endif
	struct hif_hdr *hif;
	struct frame_hdr *fh;

	hif = (void *)skb->data;
	fh = (void *)(hif + 1);

	if (nw->drv_state <= NRC_DRV_CLOSING || nw->loopback)
		return 0;

	priv->slot[TX_SLOT].tail += nr_slot;

	if ((hif->type == HIF_TYPE_FRAME)
			&& ((hif->subtype == HIF_FRAME_SUB_DATA_BE)
				|| (hif->subtype == HIF_FRAME_SUB_MGMT)))
		priv->front[fh->flags.tx.ac] += nr_slot;

#ifdef CONFIG_TRX_BACKOFF
	if (!nw->ampdu_supported) {
		backoff = atomic_inc_return(&priv->trx_backoff);

		if ((backoff % 3) == 0) {
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
			nrc_dbg(NRC_DBG_HIF, "%s: backoff=%d",
				__func__, backoff);
#endif
			usleep_range(800, 1000);
		}
	}
#endif

	//c_spi_write_reg(priv->spi, C_SPI_WAKE_UP, 0x79);
	/* Yes, I know we are accessing beyound skb->data + skb->len */
	ret = c_spi_write(priv->spi, skb->data,
			(nr_slot * priv->slot[TX_SLOT].size));
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF,
	"%s ac=%d skb=%p, slot=%d(%d/%d),fwpend:%d/%d, pending=%d",
			__func__,
			fh->flags.tx.ac, skb, nr_slot,
			priv->slot[TX_SLOT].head, priv->slot[TX_SLOT].tail,
			priv->front[fh->flags.tx.ac],
			priv->rear[fh->flags.tx.ac],
			skb_queue_len(&hdev->queue[0]));
#endif

	return (ret == nr_slot * priv->slot[TX_SLOT].size) ?
		HIF_TX_COMPLETE : ret;
}

static void spi_poll_status(struct work_struct *work)
{
	struct nrc_spi_priv *priv = container_of(to_delayed_work(work),
			struct nrc_spi_priv, work);
	struct spi_device *spi = priv->spi;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "%s", __func__);
#endif

	if (cspi_suspend) return;

	spi_update_status(spi);
	wake_up_interruptible(&priv->wait);
}

static int spi_poll_thread (void *data)
{
	struct nrc_hif_device *hdev = (struct nrc_hif_device *)data;
	struct nrc_spi_priv *priv = (struct nrc_spi_priv *)hdev->priv;
	int gpio = spi_gpio_irq;
	int interval = priv->polling_interval;
	int ret;

	dev_info(&priv->spi->dev, "%s: gpio=%d interval=%d\n", __func__, gpio, interval);

	if (WARN_ON(interval <= 0))
		return -1;

	interval *= 1000;

	while (!kthread_should_stop()) {
		if (gpio < 0) {
			spi_irq(-1, hdev);
			wake_up_interruptible(&priv->wait);
		}
		else {
			ret = gpio_get_value_cansleep(gpio);

			if (ret < 0)
				pr_err("%s: gpio_get_value_cansleep() failed, ret=%d", __func__, ret);
			else if (ret == !!(CSPI_EIRQ_MODE & 1))
				spi_irq(gpio, hdev);
		}

		usleep_range(interval, interval + 100);
	}

	return 0;
}

#define MAX_PROBE_CNT   3
static int spi_probe(struct nrc_hif_device *dev)
{
	struct nrc_spi_priv *priv = dev->priv;
	struct spi_device *spi;
	struct spi_sys_reg *sys = &priv->hw.sys;

	int i;
	int ret;

	spi = priv->spi;

	for (i = 0; i < MAX_PROBE_CNT; i++) {
		mdelay(50);
		ret = spi_read_sys_reg(spi, sys);

		if (ret) {
			dev_info(&spi->dev, "Target not ready...");
			continue;
		}

		dev_info(&spi->dev, "SPI probing. chip_id:%04x modem_id:%08x status:%d",
					sys->chip_id, sys->modem_id, sys->status);

		if (fw_name && !(sys->status & 0x1)) {
			dev_info(&spi->dev, "Invalid target status\n");
			return -1;
		}

		switch (sys->chip_id) {
			case 0x4791:
			case 0x7292:
			case 0x7392:
			case 0x7393:
			case 0x7394:
				c_spi_config(priv);
				spi_set_default_credit(priv);
				return 0;
			default:
				dev_err(&spi->dev, "Invalid target chip\n");
				BUG();
		}
	}
	return -1;
}

static int spi_start(struct nrc_hif_device *dev)
{
	struct nrc_spi_priv *priv = dev->priv;
	struct spi_device *spi = priv->spi;
	struct spi_status_reg *status = &priv->hw.status;
	int ret = 0;

	/* Start rx thread */
	priv->kthread = kthread_run(spi_rx_thread, dev, "nrc-spi-rx");
	if (IS_ERR(priv->kthread)) {
		pr_err("kthread_run() is failed");
		return PTR_ERR(priv->kthread);
	}

	atomic_set(&irq_enabled, 0);

	/* Enable interrupt or polling */
	if (priv->polling_interval > 0) {
		priv->polling_kthread = kthread_run(spi_poll_thread, dev, "spi-poll");
		if (IS_ERR(priv->polling_kthread)) {
			nrc_dbg(NRC_DBG_HIF, "kthread_run() is failed");
			ret = PTR_ERR(priv->polling_kthread);
			priv->polling_kthread = NULL;
			goto kill_kthread;
		}
	} else if (spi->irq >= 0) {
#ifdef CONFIG_SUPPORT_THREADED_IRQ
		ret = request_threaded_irq(spi->irq, NULL, spi_irq,
				IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
				"nrc-spi-irq", dev);
#else
		ret = request_irq(spi->irq, spi_irq,
				IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
				"nrc-spi-irq", dev);
#endif

		if (ret < 0) {
#ifdef CONFIG_SUPPORT_THREADED_IRQ
			pr_err("request_irq() is failed");
#else
			pr_err("request_threaded_irq() is failed");
#endif
			goto kill_kthread;
		}

		atomic_set(&irq_enabled, 1);
	}
	else {
		pr_err("invalid module parameters: spi_gpio_irq < 0 && spi_gpio_poll <= 0 && spi_regs_poll <= 0");
		goto kill_kthread;
	}

	ret = spi_update_status(spi);

	/* Restore last state */
	priv->slot[RX_SLOT].tail = priv->slot[RX_SLOT].head;
	priv->slot[TX_SLOT].tail = __be32_to_cpu(status->msg[3]) & 0xffff;

	cspi_suspend = false;
	return ret;

 kill_kthread:
	kthread_stop(priv->kthread);
	priv->kthread = NULL;

	return ret;
}

static int spi_suspend(struct nrc_hif_device *dev)
{
	struct nrc_spi_priv *priv = dev->priv;
	struct spi_device *spi = priv->spi;
	struct nrc *nw = dev->nw;

	nrc_ps_dbg("[%s] drv_state:%d wowlan_enabled:%x power_save:%d\n",
			__func__, nw->drv_state, nw->wowlan_enabled, power_save);

	if (priv->kthread) {
		kthread_park(priv->kthread); /* pause kthread(spi_rx_skb) */
	}

	if (nw->drv_state == NRC_DRV_RUNNING &&
		(nw->wowlan_enabled || (power_save >= NRC_PS_DEEPSLEEP_TIM))) {
		nw->drv_state = NRC_DRV_PS;
		/**
		 * this delay is necessary to achive sending WIM meg completly.
		 * (especially, WIM_TLV_PS_ENABLE)
		 */
		usleep_range(20 * 1000, 30 * 1000);

		if (spi->irq >= 0 && priv->polling_interval <= 0) {
			/* Waits for any pending IRQ handlers for this interrupt to complete */
			synchronize_irq(spi->irq);

			/**
			 * The SPI IRQ will be never disabled because it needs to be asserted
			 * by sending FW ready WIM message from target when uCode received a TIM
			 * and it has to notify wake-up to the host.
			 */
			//disable_irq(gpio_to_irq(spi->irq));
			//atomic_set(&irq_enabled, 0);
		}

		nrc_hif_flush_wq(dev);
#if 0 /* It is called as needed in the interrupt handler. */
		spi_config_fw(dev);
#endif
	} else {
		cspi_suspend = true;
	}

	return 0;
}

static int spi_resume(struct nrc_hif_device *dev)
{
	struct nrc_spi_priv *priv = dev->priv;
	struct spi_device *spi = priv->spi;
	struct spi_sys_reg *sys = &priv->hw.sys;
	int ret;


	if (power_save >= NRC_PS_DEEPSLEEP_TIM) {
		nrc_ps_dbg("[%s] drv_state:%d wowlan_enabled:%x power_save:%d", __func__,
				dev->nw->drv_state, dev->nw->wowlan_enabled, power_save);
		if (dev->nw->drv_state == NRC_DRV_PS) {
			ret = spi_read_sys_reg(spi, sys);
			dev->nw->drv_state = NRC_DRV_RUNNING;
			ieee80211_wake_queues(dev->nw->hw);
			gpio_set_value(power_save_gpio[0], 0);
		} else {
			cspi_suspend = false;

			if (spi->irq >= 0 && priv->polling_interval <= 0) {
				/*
				 * The SPI IRQ will be never disabled because it needs to be asserted
				 * by sending FW ready WIM message from target when uCode received a TIM
				 * and it has to notify wake-up to the host.
				 * So, it is not necessary the codes as below.
				 */
				//if (atomic_read(&irq_enabled) == 0) {
				//	enable_irq(gpio_to_irq(spi->irq));
				//	atomic_set(&irq_enabled, 1);
				//}

				/* README:
				 *  At this point, I assume the firmware has ready to use CSPI.
				 */
			}

			/* Read the register */
			ret = spi_read_sys_reg(spi, sys);

			spi_config_fw(dev);
			c_spi_config(priv);
			spi_set_default_credit(priv);

			spi_update_status(spi);
		}
	} else {
		cspi_suspend = false;
		if (dev->nw->wowlan_enabled && dev->nw->drv_state == NRC_DRV_PS) {
			if (spi->irq >= 0 && priv->polling_interval <= 0) {
				if (atomic_read(&irq_enabled) == 0) {
					enable_irq(spi->irq);
					atomic_set(&irq_enabled, 1);
				}
			}

			/* Read the register */
			ret = spi_read_sys_reg(spi, sys);
			dev->nw->drv_state = NRC_DRV_RUNNING;

			//spi_config_fw(dev);
			c_spi_config(priv);
			spi_set_default_credit(priv);

			spi_update_status(spi);
		}
	}

	if (priv->kthread) {
		kthread_unpark(priv->kthread); /* resume  kthread(spi_rx_skb) */
	}

	c_spi_enable_irq(spi, true, CSPI_EIRQ_S_ENABLE);

	return 0;
}

void spi_reset_device(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	int i;

	if (enable_hspi_init) {
		for(i=0; i<180; i++)
			_c_spi_write_dummy(spi);
	}
	/* 0xC8 is magic number for reset the device */
	c_spi_write_reg(spi, C_SPI_DEVICE_STATUS, 0xC8);
}

void spi_reset_rx (struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	struct nrc *nw = spi_get_drvdata(spi);

	dev_err(&spi->dev, "Reset SPI RX\n");
	priv->slot[RX_SLOT].tail = priv->slot[RX_SLOT].head = 0;
	nrc_wim_reset_hif_tx(nw);
}

static void spi_disable_irq(struct nrc_hif_device *hdev);

static int spi_stop(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

	spi_disable_irq(hdev);
	priv->slot[TX_SLOT].count = 999;
	if (priv->polling_kthread)
		kthread_stop(priv->polling_kthread);
	if (priv->kthread)
		kthread_stop(priv->kthread);
	wake_up_interruptible(&priv->wait);

	cancel_delayed_work(&priv->work);

	if (spi->irq >= 0) {
		if (priv->polling_interval <= 0) {
			synchronize_irq(spi->irq);
			free_irq(spi->irq, hdev);
		}
	}

	return 0;
}

static void spi_close(struct nrc_hif_device *dev)
{
	return;
}

int spi_test(struct nrc_hif_device *hdev)
{
/* temporary function for soc */
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	struct spi_status_reg *status = &priv->hw.status;
	struct spi_sys_reg sys;
	int ret;

	nrc_dbg(NRC_DBG_HIF, "+ read sys");
	ret = c_spi_read_regs(spi, C_SPI_WAKE_UP, (void *)&sys,
			sizeof(sys));
	nrc_dbg(NRC_DBG_HIF, "- read sys");

	print_hex_dump(KERN_DEBUG, "sys ", DUMP_PREFIX_NONE, 16, 1,
			&sys, sizeof(sys), false);

	if (ret < 0)
		return ret;

	nrc_dbg(NRC_DBG_HIF, "+ read status");
	ret = c_spi_read_regs(spi, C_SPI_EIRQ_MODE, (void *)status,
			sizeof(*status));
	nrc_dbg(NRC_DBG_HIF, "- read status");

	print_hex_dump(KERN_DEBUG, "status ", DUMP_PREFIX_NONE, 16, 1,
			status, sizeof(*status), false);

	if (ret < 0)
		return ret;

	return 0;
}

void spi_wakeup(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

	//spi_enable_irq(hdev);
	//c_spi_enable_irq(spi, true);
	//nrc_ps_dbg("spi-wakeup");
	/* 0x79 is magic number for wakeup the device from sleep mode */
	c_spi_write_reg(spi, C_SPI_WAKE_UP, 0x79);

}

static void spi_config_fw(struct nrc_hif_device *dev)
{
	struct nrc_spi_priv *priv = dev->priv;
	int ac;

	priv->slot[RX_SLOT].tail = priv->slot[RX_SLOT].head = 0;
	priv->slot[TX_SLOT].tail = priv->slot[TX_SLOT].head = 0;

	for (ac = 0; ac < CREDIT_QUEUE_MAX; ac++) {
		priv->front[ac] = 0;
		priv->rear[ac] = 0;
	}
}

static void spi_sync_lock(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;

	mutex_lock(&priv->bus_lock_mutex);
}

static void spi_sync_unlock(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;

	mutex_unlock(&priv->bus_lock_mutex);
}

static void spi_disable_irq(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

	if (spi->irq >= 0) {
		if (priv->polling_interval <= 0) {
			//disable_irq(spi->irq);
			disable_irq_nosync(spi->irq);
		}
	}
}

static void spi_enable_irq(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

	if (spi->irq >= 0)
		if (priv->polling_interval <= 0)
			enable_irq(spi->irq);
}

static int spi_status_irq(struct nrc_hif_device *hdev)
{
	//struct nrc_spi_priv *priv = hdev->priv;
	//struct spi_device *spi = priv->spi;
	int irq = gpio_get_value(spi_gpio_irq);

	return irq;
}

static void spi_clear_irq(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

	spi_read_status(spi);
}

static void spi_update(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	struct spi_status_reg *status = &priv->hw.status;
	int ret;
	u32 rear;
	int ac;

	ret = c_spi_read_regs(spi, C_SPI_EIRQ_MODE, (void *)status,
			sizeof(*status));
	if (ret < 0)
		return;

	priv->slot[TX_SLOT].count = status->rxq_status[1] & RXQ_SLOT_COUNT;
	priv->slot[RX_SLOT].count = status->txq_status[1] & TXQ_SLOT_COUNT;
	priv->slot[RX_SLOT].head = __be32_to_cpu(status->msg[0]) & 0xffff;
	priv->slot[TX_SLOT].head = __be32_to_cpu(status->msg[0]) >> 16;

	/* Update VIF0 credit */
	rear = __be32_to_cpu(status->msg[1]);
	for (ac = 0; ac < 4; ac++)
		priv->rear[ac] = (rear >> 8*ac) & 0xff;

	/* Update VIF1 credit */
	rear = __be32_to_cpu(status->msg[2]);
	for (ac = 0; ac < 4; ac++)
		priv->rear[6+ac] = (rear >> 8*ac) & 0xff;
}

static void spi_gpio(int v)
{
#if defined(SPI_DBG)
	gpio_set_value(SPI_DBG, v);
#endif
}

static int spi_suspend_rx_thread(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

	c_spi_enable_irq(spi, false, CSPI_EIRQ_S_ENABLE); /* before issuing wim cmd */

	if (spi->irq >= 0 && priv->polling_interval <= 0) {
		if (atomic_read(&irq_enabled) == 1) {
			disable_irq_nosync(spi->irq);
			atomic_set(&irq_enabled, 0);
		}
	}

	cspi_suspend = true;
	return 0;
}

static int spi_resume_rx_thread(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

	if (spi->irq >= 0 && priv->polling_interval <= 0) {
		if (atomic_read(&irq_enabled) == 0) {
			enable_irq(spi->irq);
			atomic_set(&irq_enabled, 1);
		}
	}

	cspi_suspend = false;
	return 0;
}

static int spi_check_target(struct nrc_hif_device *hdev, u8 reg)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	struct spi_status_reg *status = &priv->hw.status;
	int ret;

	ret = c_spi_read_regs(spi, C_SPI_EIRQ_MODE, (void *)status,
			sizeof(*status));

	if (ret < 0)
		return ret;

	return (int)(*(((u8*)status) + reg - C_SPI_EIRQ_MODE));
}

static bool spi_check_ready(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	struct spi_sys_reg sys;
	int ret = 0;

	nrc_dbg(NRC_DBG_HIF, "[%s++]", __func__);
	ret = spi_read_sys_reg(spi, &sys);

	if (ret < 0)
		return false;

	dev_info(&spi->dev, "[%s] chip_id:%04x build_chip_id:%04x target_sw_id:%u host_sw_id:%u  status:%d",
				__func__, sys.chip_id, (sys.sw_id >> 16), sys.sw_id & 0xFFFF, NRC_SW_ID, sys.status);

	if (fw_name && !(sys.status & 0x1)) {
		return false;
	}
	if ((sys.sw_id & 0xFFFF) != NRC_SW_ID) { /* Low 2byte: build sw id */
		return false;
	}
	if (((sys.sw_id >> 16)&0xFFFF) != sys.chip_id) { /* High 2byte: build chip id */
		return false;
	}

	return true;
}

static struct nrc_hif_ops spi_ops = {
	.name = spi_name,
	.check_fw = spi_check_fw,
	.xmit = spi_xmit,
	.wait_for_xmit = spi_wait_for_xmit,
	.write = spi_raw_write,
	.wait_ack = spi_wait_ack,
	.probe = spi_probe,
	.start = spi_start,
	.stop = spi_stop,
	.suspend = spi_suspend,
	.resume = spi_resume,
	.close = spi_close,
	.reset_device = spi_reset_device,
	.reset_rx = spi_reset_rx,
	.wakeup = spi_wakeup,
	.test = spi_test,
	.config = spi_config_fw,
	.sync_lock = spi_sync_lock,
	.sync_unlock = spi_sync_unlock,
	.disable_irq = spi_disable_irq,
	.enable_irq = spi_enable_irq,
	.status_irq = spi_status_irq,
	.clear_irq = spi_clear_irq,
	.update = spi_update,
	.set_gpio = spi_gpio,
	.suspend_rx_thread = spi_suspend_rx_thread,
	.resume_rx_thread = spi_resume_rx_thread,
	.check_target = spi_check_target,
	.check_ready = spi_check_ready,
};

#define MAX_ENABLE_IRQ_RETRY	3
#define MAX_ENABLE_IRQ_DELAY	5

static void c_spi_enable_irq(struct spi_device *spi, bool enable, u8 mask)
{
	int ret = 0, retry = 0;
	u8 m, e = 0x00;

	if (enable) {
		m = CSPI_EIRQ_MODE;
	}

	for (retry = 0; retry < MAX_ENABLE_IRQ_RETRY; retry ++) {
		ret = c_spi_read_regs(spi, C_SPI_EIRQ_ENABLE, &e, sizeof(u8));
		if (ret) {
			nrc_dbg(NRC_DBG_HIF, "%s:retry0(%d)", __func__, retry + 1);
			mdelay(MAX_ENABLE_IRQ_DELAY);
			continue;
		}
	}

	e = enable?(e | mask):(e & ~mask);

	/* uCode Wake-up --> uCode Interrupt --> Host IRQ Handler --> HIF Enabled case */

	for (retry = 0; retry < MAX_ENABLE_IRQ_RETRY; retry ++) {
		ret = c_spi_write_reg(spi, C_SPI_EIRQ_MODE, m);
		if (ret) {
			nrc_dbg(NRC_DBG_HIF, "%s:retry1(%d)", __func__, retry + 1);
			mdelay(MAX_ENABLE_IRQ_DELAY);
			continue;
		}
		ret = c_spi_write_reg(spi, C_SPI_EIRQ_ENABLE, e);
		if (ret) {
			nrc_dbg(NRC_DBG_HIF, "%s:retry2(%d)", __func__, retry + 1);
			mdelay(MAX_ENABLE_IRQ_DELAY);
			continue;
		}
	} 
}

static void c_spi_config(struct nrc_spi_priv *priv)
{
	struct spi_sys_reg *sys = &priv->hw.sys;

	priv->slot[TX_SLOT].size = 456;
	priv->slot[RX_SLOT].size = 492;
	switch (sys->chip_id) {
	case 0x7391:
	case 0x7292:
	case 0x7392:
	case 0x4791:
	case 0x5291:
	case 0x7393:
	case 0x7394:
		spi_ops.sync_auto = false;
		break;
	case 0x6201:
		spi_ops.sync_auto = true;
		break;
	default:
		nrc_dbg(NRC_DBG_HIF,
			"Unknown Newracom IEEE80211 chipset %04x",
			sys->chip_id);
		BUG();
	}

	nrc_dbg(NRC_DBG_HIF,
	"Newracom IEEE802.11 C-SPI: chipid=%04x, sw_id=%04x, board_id=%04X",
		sys->chip_id, sys->sw_id, sys->board_id);
	if (sys->sw_id == SW_MAGIC_FOR_BOOT)
		nrc_dbg(NRC_DBG_HIF, "Boot loader");
	else if (sys->sw_id == SW_MAGIC_FOR_FW)
		nrc_dbg(NRC_DBG_HIF, "Firmware");

	c_spi_enable_irq(priv->spi, priv->spi->irq >= 0 ? true : false, CSPI_EIRQ_A_ENABLE);
}

int nrc_cspi_gpio_alloc(struct spi_device *spi)
{
#if defined(SPI_DBG)
	/* Claim gpio used for debugging */
	if (gpio_request(SPI_DBG, "nrc-spi-dgb") < 0) {
		dev_err(&spi->dev, "[Error] gpio_reqeust() is failed");
		goto err;
	}
	gpio_direction_output(SPI_DBG, 1);
#endif

#if defined(ENABLE_HW_RESET)
	if (gpio_request(RPI_GPIO_FOR_RST, "nrc-reset") < 0) {
		dev_err(&spi->dev, "[Error] gpio_reqeust(nrc-reset) is failed");
		goto err_dbg_irq_free;
	}
	gpio_direction_output(RPI_GPIO_FOR_RST, 1);
#endif

	if (power_save >= NRC_PS_DEEPSLEEP_TIM) {
		if (gpio_request(power_save_gpio[0], "nrc-wakeup") < 0) {
			dev_err(&spi->dev, ("[Error] gpio_reqeust(nrc-wakeup) is failed\n"));
			goto err_rst_free;
		}
		gpio_direction_output(power_save_gpio[0], 0);
	}

#ifndef CONFIG_SPI_USE_DT /* spi->irq is real irq number, not gpio number by setting in dts */
	if (spi->irq >= 0) {
		/* Claim gpio used for irq */
		if (gpio_request(spi_gpio_irq, "nrc-spi-irq") < 0) {
			dev_err(&spi->dev, "[Error] gpio_reqeust() is failed (%d)", spi->irq);
			goto err_free_all;
		}
		gpio_direction_input(spi->irq);
	}
#endif

	return 0;

#ifndef CONFIG_SPI_USE_DT
err_free_all:
	if (power_save >= NRC_PS_DEEPSLEEP_TIM) {
			gpio_free(power_save_gpio[0]);
	}
#endif

err_rst_free:
#if defined(ENABLE_HW_RESET)
	gpio_free(RPI_GPIO_FOR_RST);
err_dbg_irq_free:
#endif
#if  defined(SPI_DBG)
	gpio_free(SPI_DBG);
err:
#endif
	return -EINVAL;
}

void nrc_cspi_gpio_free(struct spi_device *spi)
{

#if defined(SPI_DBG)
	gpio_free(SPI_DBG);
#endif

#if defined(ENABLE_HW_RESET)
	gpio_set_value(RPI_GPIO_FOR_RST, 0);
	msleep(10);
	gpio_set_value(RPI_GPIO_FOR_RST, 1);
	gpio_free(RPI_GPIO_FOR_RST);
#endif

	if (power_save >= NRC_PS_DEEPSLEEP_TIM) {
		gpio_free(power_save_gpio[0]);
	}

#ifndef CONFIG_SPI_USE_DT /* spi->irq is real irq number, not gpio number by setting in dts */
	if (spi->irq >= 0) {
		gpio_free(spi_gpio_irq);
	}
#endif
}

static struct nrc_spi_priv *nrc_cspi_alloc (struct spi_device *dev)
{
	struct nrc_spi_priv *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		return NULL;
	}

	dev->dev.platform_data = priv;  /* spi_device can refer to nrc_spi_priv */
									/* some api still uses dev_get_platdata */
	priv->spi = dev;
	priv->ops = &cspi_ops;

	init_waitqueue_head(&priv->wait);
	spin_lock_init(&priv->lock);
	mutex_init(&priv->bus_lock_mutex);

#if !defined(CONFIG_SUPPORT_THREADED_IRQ)
	priv->irq_wq = create_singlethread_workqueue("nrc_cspi_irq");
	INIT_WORK(&priv->irq_work, irq_worker);
#endif
	INIT_DELAYED_WORK(&priv->work, spi_poll_status);

	priv->polling_interval = spi_polling_interval; /* from module param */

	return priv;
}

static void nrc_cspi_free (struct nrc_spi_priv *priv)
{	
#if !defined(CONFIG_SUPPORT_THREADED_IRQ)
	flush_workqueue(priv->irq_wq);
	destroy_workqueue(priv->irq_wq);
#endif

	priv->spi->dev.platform_data = NULL;
	kfree(priv);
}

#define MAX_RETRY_CNT  3
static int nrc_cspi_probe(struct spi_device *spi)
{
	struct nrc *nw;
	struct nrc_hif_device *hdev;
	struct nrc_spi_priv *priv;

	int ret = 0;
	int retry = 0;

	nrc_dbg_init(&spi->dev);

	priv = nrc_cspi_alloc(spi);
	if (IS_ERR(priv)) {
		dev_err(&spi->dev, "Failed to nrc_cspi_alloc\n");
		return PTR_ERR(priv);
	}

	ret = nrc_cspi_gpio_alloc(spi);
	if (ret) {
		dev_err(&spi->dev, "Failed to nrc_cspi_gpio_alloc\n");
		goto err_cspi_free;
	}

	hdev = nrc_hif_alloc(&spi->dev, (void *)priv,  &spi_ops);
	if (IS_ERR(hdev)) {
		dev_err(&spi->dev, "Failed to nrc_hif_alloc\n");
		ret = PTR_ERR(hdev);
		goto err_gpio_free;
	}

	priv->hdev = hdev;

try:
	if (fw_name) nrc_hif_reset_device(hdev);
	ret = nrc_hif_probe(hdev);
	if (ret && retry < MAX_RETRY_CNT) {
		retry++;
		goto try;
	}

	if (ret) {
		dev_err(&spi->dev, "Failed to nrc_hif_probe\n");
		goto err_hif_free;
	}

	nw = nrc_nw_alloc(&spi->dev, hdev);
	if (IS_ERR(nw)) {
		dev_err(&spi->dev, "Failed to nrc_nw_alloc\n");
		goto err_hif_free;
	}

	spi_set_drvdata(spi, nw);

	nrc_nw_set_model_conf(nw, priv->hw.sys.chip_id);

	ret = nrc_nw_start(nw);
	if (ret) {
		dev_err(&spi->dev, "Failed to nrc_nw_start (%d)\n", ret);
		goto err_nw_free;
	}

	return 0;

err_nw_free:
	nrc_nw_free(nw);
err_hif_free:
	nrc_hif_free(hdev);
err_gpio_free:
	nrc_cspi_gpio_free(spi);
err_cspi_free:
	nrc_cspi_free(priv);

	spi_set_drvdata(spi, NULL);
	return ret;
}
/*
 * The value returned by an spi driver's remove function is mostly ignored.
 * So change the prototype of the remove function to return no value.
 * (https://patchwork.kernel.org/project/spi-devel-general/patch/20220123175201.34839-6-u.kleine-koenig@pengutronix.de/)
 */
#if NRC_TARGET_KERNEL_VERSION < KERNEL_VERSION(5,18,0)
static int nrc_cspi_remove(struct spi_device *spi)
#else
static void nrc_cspi_remove(struct spi_device *spi)
#endif
{
	struct nrc *nw;
	struct nrc_hif_device *hdev;
	struct nrc_spi_priv *priv;

	nw = spi_get_drvdata(spi);
	hdev = nw->hif;
	priv = hdev->priv;

	nrc_nw_stop(nw);

	spi_set_drvdata(spi, NULL);

	nrc_nw_free(nw);

	nrc_hif_free(hdev);

	nrc_cspi_gpio_free(spi);

	nrc_cspi_free(priv);
#if NRC_TARGET_KERNEL_VERSION < KERNEL_VERSION(5,18,0)
	return 0;
#endif
}

static const struct spi_device_id nrc_spi_id[] = {
	{NRC_DRIVER_NAME, 0},
	{ }
};

MODULE_DEVICE_TABLE(spi, nrc_spi_id);

static struct spi_driver nrc_cspi_driver = {
	.id_table = nrc_spi_id,
	.probe = nrc_cspi_probe,
	.remove = nrc_cspi_remove,
	.driver = {
		.name = NRC_DRIVER_NAME,
	},
};

#ifndef CONFIG_SPI_USE_DT
static struct spi_board_info bi = {
	.modalias = NRC_DRIVER_NAME,
//	.chip_select = 0,
	.mode = SPI_MODE_0,
};
#endif

#ifndef CONFIG_SPI_USE_DT
static struct spi_device *nrc_create_spi_device (void)
{
	struct spi_master *master;
	struct spi_device *spi;

	/* Apply module parameters */
	bi.bus_num = spi_bus_num;
	bi.chip_select = spi_cs_num;
	bi.irq = spi_gpio_irq >= 0 ? gpio_to_irq(spi_gpio_irq) : -1;
	bi.max_speed_hz = hifspeed;

	/* Find the spi master that our device is attached to */
	master = spi_busnum_to_master(spi_bus_num);
	if (!master) {
		pr_err("Could not find spi master with the bus number %d.",
			spi_bus_num);
		return NULL;
	}

	/* Instantiate and add a spi device */
	spi = spi_new_device(master, &bi);
	if (!spi) {
		pr_err("Failed to instantiate a new spi device.");
		return NULL;
	}

	dev_info(&spi->dev, "SPI Device Created (bus_num:%d, cs_num:%d, irq_num:%d, max_speed:%d\n",
			spi->master->bus_num,
			spi->chip_select,
			spi->irq,
			spi->max_speed_hz);
	return spi;
}
#endif

struct spi_device *g_spi_dev;

static int __init nrc_cspi_init (void)
{
#ifndef CONFIG_SPI_USE_DT
	struct spi_device *spi;
#endif
	int ret = 0;

#ifndef CONFIG_SPI_USE_DT
	spi = nrc_create_spi_device();
	if (IS_ERR(spi)) {
		pr_err("Failed to nrc_create_spi_dev\n");
		goto out;
	}
	g_spi_dev = spi;
#endif

	ret = spi_register_driver(&nrc_cspi_driver);
	if (ret) {
		pr_info("Failed to register spi driver(%s).",
					nrc_cspi_driver.driver.name);
		goto unregister_device;
	}

	pr_info("Succeed to register spi driver(%s).", nrc_cspi_driver.driver.name);
	return ret;

unregister_device:
#ifndef CONFIG_SPI_USE_DT
	spi_unregister_device(spi);
out:
#endif
	return ret;
}

static void __exit nrc_cspi_exit (void)
{
#ifndef CONFIG_SPI_USE_DT
	spi_unregister_device(g_spi_dev);
#endif
	spi_unregister_driver(&nrc_cspi_driver);
}

module_init(nrc_cspi_init);
module_exit(nrc_cspi_exit);

MODULE_AUTHOR("Newracom, Inc.(http://www.newracom.com)");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Newracom 802.11 driver");
#if KERNEL_VERSION(5, 12, 0) > NRC_TARGET_KERNEL_VERSION
MODULE_SUPPORTED_DEVICE("Newracom 802.11 devices");
#endif
