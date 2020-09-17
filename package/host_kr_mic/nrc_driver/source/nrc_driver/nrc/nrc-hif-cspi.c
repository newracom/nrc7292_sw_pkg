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

#include "nrc-fw.h"
#include "nrc-hif.h"
#include "wim.h"

static bool once;
static atomic_t suspend;
static atomic_t irq_enabled;
#define TCN  (2*1)
#define TCNE (0)
#define CREDIT_AC0	(TCN*2+TCNE)	/* BK */
#define CREDIT_AC1	(TCN*20+TCNE)	/* BE */
#define CREDIT_AC2	(TCN*4+TCNE)	/* VI */
#define CREDIT_AC3	(TCN*4+TCNE)	/* VO */

/* only use for NRC7291 others are using same value */
#define CREDIT_AC3_7291	(TCN*4+TCNE)	/* VO */

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

#define CSPI_EIRQ_MODE 0x05
#define CSPI_EIRQ_ENABLE 0x1f /* enable tx/rx que */
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
	struct spi_device spi_r;

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
	u8 front[CREDIT_QUEUE_MAX];
	u8 rear[CREDIT_QUEUE_MAX];
	u8 credit_max[CREDIT_QUEUE_MAX];
#ifdef CONFIG_TRX_BACKOFF
	atomic_t trx_backoff;
#endif
	bool fastboot;
	unsigned long loopback_prev_cnt;
	unsigned long loopback_total_cnt;
	unsigned long loopback_last_jiffies;
	unsigned long loopback_read_usec;
	unsigned long loopback_write_usec;
	unsigned long loopback_measure_cnt;
	struct mutex bus_lock_mutex;
	struct nrc_cspi_ops *ops;
};

struct nrc_cspi_ops {
	int (*read_regs)(struct spi_device *spi,
		u8 addr, u8 *buf, ssize_t size);
	int (*write_reg)(struct spi_device *spi, u8 addr, u8 data);
	ssize_t (*read)(struct spi_device *spi, u8 *buf, ssize_t size);
	ssize_t (*write)(struct spi_device *spi, u8 *buf, ssize_t size);
};

static int spi_update_status(struct spi_device *spi);
static void c_spi_enable_irq(struct spi_device *spi, bool enable);

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

static int _c_spi_read_regs_alt(struct spi_device *spi,
		u8 addr, u8 *buf, ssize_t size)
{
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi_r = &priv->spi_r;
	struct spi_transfer xfer[5] = {{0},};
	u32 cmd, crc, dummy;
	u8 tx[6], rx[2];
	ssize_t status;

	if (size == 0 || buf == NULL)
		return -EINVAL;
	cmd = C_SPI_READ | C_SPI_ADDR(addr);
	cmd |= ((size > 1) ? (C_SPI_BURST | C_SPI_LEN(size))
			: (C_SPI_FIXED | C_SPI_LEN(0)));
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;
	dummy = 0xffffffff;

	mutex_lock(&priv->bus_lock_mutex);

	if (size > 1) {
		/* write cmd, crc */
		spi_set_transfer(&xfer[0], tx, NULL, 6);
		status = spi_sync_transfer(spi, &xfer[0], 1);
		if (status < 0)
			goto failed;

		/* read ack, regs, and crc */
		spi_set_transfer(&xfer[1], NULL, rx, 2);
		spi_set_transfer(&xfer[2], NULL, buf, size);
		spi_set_transfer(&xfer[3], NULL, &crc, sizeof(crc));
		status = spi_sync_transfer(spi_r, &xfer[1], 3);
		if (status < 0 || WARN_ON(rx[1] != C_SPI_ACK))
			goto failed;

		/* write dummy */
		spi_set_transfer(&xfer[4], &dummy, NULL, sizeof(dummy));
		status = spi_sync_transfer(spi, &xfer[4], 1);
		if (status < 0)
			goto failed;
	} else {
		/* write cmd, crc */
		spi_set_transfer(&xfer[0], tx, NULL, 6);
		status = spi_sync_transfer(spi, &xfer[0], 1);
		if (status < 0)
			goto failed;

		/* read reg, ack */
		spi_set_transfer(&xfer[1], NULL, rx, 2);
		status = spi_sync_transfer(spi_r, &xfer[1], 1);
		if (status < 0 || WARN_ON(rx[1] != C_SPI_ACK))
			goto failed;

		/* write dummy */
		spi_set_transfer(&xfer[2], &dummy, NULL, sizeof(dummy));
		status = spi_sync_transfer(spi, &xfer[2], 1);
		if (status < 0)
			goto failed;
		buf[0] = rx[0];
	}

	mutex_unlock(&priv->bus_lock_mutex);

	return 0;

failed:
	mutex_unlock(&priv->bus_lock_mutex);

	return -EIO;
}

static int _c_spi_write_dummy(struct spi_device *spi)
{
	struct spi_transfer xfer[2] = {{0},};
	u32 dummy=0xffffffff;
	u8 tx[8], rx[8];
	ssize_t status;

	memset(tx, 0xff, sizeof(tx));
	spi_set_transfer(&xfer[0], tx, rx, 8);
	dummy = 0xffffffff;
	spi_set_transfer(&xfer[1], &dummy, NULL, sizeof(dummy));

	status = spi_sync_transfer(spi, xfer, 2);

	return 0;
}

static int _c_spi_write_reg_alt(struct spi_device *spi, u8 addr, u8 data)
{
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi_r = &priv->spi_r;
	struct spi_transfer xfer[3] = {{0},};
	u32 cmd, dummy;
	u8 tx[6], rx[2];
	ssize_t status;

	cmd = C_SPI_WRITE | C_SPI_FIXED | C_SPI_ADDR(addr) | C_SPI_WDATA(data);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;

	dummy = 0xffffffff;

	mutex_lock(&priv->bus_lock_mutex);

	/* write cmd, reg, and crc */
	spi_set_transfer(&xfer[0], tx, NULL, 6);
	status = spi_sync_transfer(spi, &xfer[0], 1);
	if (status < 0)
		goto failed;

	/* read ack */
	spi_set_transfer(&xfer[1], NULL, rx, 2);
	status = spi_sync_transfer(spi_r, &xfer[1], 1);
	if (status < 0 || WARN_ON(rx[1] != C_SPI_ACK))
		goto failed;

	/* write dummy */
	spi_set_transfer(&xfer[2], &dummy, NULL, sizeof(dummy));
	status = spi_sync_transfer(spi, &xfer[2], 1);
	if (status < 0)
		goto failed;

	mutex_unlock(&priv->bus_lock_mutex);

	return 0;

failed:
	mutex_unlock(&priv->bus_lock_mutex);

	return -EIO;
}

static ssize_t _c_spi_read_alt(struct spi_device *spi, u8 *buf, ssize_t size)
{
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi_r = &priv->spi_r;
	struct spi_transfer xfer[5] = {{0},};
	u32 cmd, crc, dummy;
	u8 tx[6], rx[2];
	ssize_t status;

	if (size == 0 || buf == NULL)
		return -EINVAL;

	cmd = C_SPI_READ | C_SPI_BURST | C_SPI_FIXED;
	cmd |= C_SPI_ADDR(C_SPI_TXQ_WINDOW) | C_SPI_LEN(size);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;

	dummy = 0xffffffff;

	mutex_lock(&priv->bus_lock_mutex);

	/* write cmd, crc */
	spi_set_transfer(&xfer[0], tx, NULL, 6);
	status = spi_sync_transfer(spi, &xfer[0], 1);
	if (status < 0)
		goto failed;

	/* read ack, data, and crc */
	spi_set_transfer(&xfer[1], NULL, rx, 2);
	spi_set_transfer(&xfer[2], NULL, buf, size);
	spi_set_transfer(&xfer[3], NULL, &crc, sizeof(crc));
	status = spi_sync_transfer(spi_r, &xfer[1], 3);
	if (status < 0 || WARN_ON(rx[1] != C_SPI_ACK))
		goto failed;

	/* write dummy */
	spi_set_transfer(&xfer[4], &dummy, NULL, sizeof(dummy));
	status = spi_sync_transfer(spi, &xfer[4], 1);
	if (status < 0)
		goto failed;

	mutex_unlock(&priv->bus_lock_mutex);

	return size;

failed:
	mutex_unlock(&priv->bus_lock_mutex);

	return -EIO;
}

static ssize_t _c_spi_write_alt(struct spi_device *spi, u8 *buf, ssize_t size)
{
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi_r = &priv->spi_r;
	struct spi_transfer xfer[5] = {{0},};
	u32 cmd, dummy = 0xffffffff;
	u8 tx[6], rx[2];
	ssize_t status;

	if (size == 0 || buf == NULL)
		return -EINVAL;

	cmd = C_SPI_WRITE | C_SPI_BURST | C_SPI_FIXED;
	cmd |= C_SPI_ADDR(C_SPI_RXQ_WINDOW) | C_SPI_LEN(size);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;

	dummy = 0xffffffff;

	mutex_lock(&priv->bus_lock_mutex);

	/* write cmd, crc */
	spi_set_transfer(&xfer[0], tx, NULL, 6);
	status = spi_sync_transfer(spi, &xfer[0], 1);
	if (status < 0)
		goto failed;

	/* read ack */
	spi_set_transfer(&xfer[1], NULL, rx, 2);
	status = spi_sync_transfer(spi_r, &xfer[1], 1);
	if (status < 0 || WARN_ON(rx[1] != C_SPI_ACK))
		goto failed;

	/* write data, dummy */
	spi_set_transfer(&xfer[2], buf, NULL, size);
	spi_set_transfer(&xfer[3], &dummy, NULL, sizeof(dummy));
	spi_set_transfer(&xfer[4], &dummy, NULL, sizeof(dummy));
	status = spi_sync_transfer(spi, &xfer[2], 3);
	if (status < 0)
		goto failed;

	mutex_unlock(&priv->bus_lock_mutex);

	return size;

failed:
	mutex_unlock(&priv->bus_lock_mutex);

	return -EIO;
}

static struct nrc_cspi_ops cspi_ops_alt = {
	.read_regs = _c_spi_read_regs_alt,
	.write_reg = _c_spi_write_reg_alt,
	.read = _c_spi_read_alt,
	.write = _c_spi_write_alt,
};

static int _c_spi_read_regs(struct spi_device *spi,
		u8 addr, u8 *buf, ssize_t size)
{
	struct spi_transfer xfer[4] = {{0},};
	u32 cmd, crc, dummy;
	u8 tx[8], rx[8];
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

	spi_set_transfer(&xfer[0], tx, rx, 8);
	spi_set_transfer(&xfer[1], NULL, buf, size);
	spi_set_transfer(&xfer[2], NULL, &crc, sizeof(crc));

	dummy = 0xffffffff;
	if (size > 1)
		spi_set_transfer(&xfer[3], &dummy, NULL, sizeof(dummy));
	else
		spi_set_transfer(&xfer[1], &dummy, NULL, sizeof(dummy));

	arr_len = (size > 1) ? ARRAY_SIZE(xfer) : 2;
	status = spi_sync_transfer(spi, xfer, arr_len);

	if (status < 0 || WARN_ON(rx[7] != C_SPI_ACK))
		return -EIO;

	if (size == 1)
		buf[0] = rx[6];

	return 0;
}

static int _c_spi_write_reg(struct spi_device *spi, u8 addr, u8 data)
{
	struct spi_transfer xfer[2] = {{0},};
	u32 cmd, dummy;
	u8 tx[8], rx[8];
	ssize_t status;

	cmd = C_SPI_WRITE | C_SPI_FIXED | C_SPI_ADDR(addr) | C_SPI_WDATA(data);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;

	spi_set_transfer(&xfer[0], tx, rx, 8);
	dummy = 0xffffffff;
	spi_set_transfer(&xfer[1], &dummy, NULL, sizeof(dummy));

		status = spi_sync_transfer(spi, xfer, 2);

	if (status < 0 || WARN_ON(rx[7] != C_SPI_ACK))
		return -EIO;

	return 0;
}

static ssize_t _c_spi_read(struct spi_device *spi, u8 *buf, ssize_t size)
{
	struct spi_transfer xfer[4] = {{0},};
	u32 cmd, crc, dummy;
	u8 tx[8], rx[8];
	ssize_t status;

	if (size == 0 || buf == NULL)
		return -EINVAL;

	cmd = C_SPI_READ | C_SPI_BURST | C_SPI_FIXED;
	cmd |= C_SPI_ADDR(C_SPI_TXQ_WINDOW) | C_SPI_LEN(size);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;

	spi_set_transfer(&xfer[0], tx, rx, 8);
	spi_set_transfer(&xfer[1], NULL, buf, size);
	spi_set_transfer(&xfer[2], NULL, &crc, sizeof(crc));

	dummy = 0xffffffff;
	spi_set_transfer(&xfer[3], &dummy, NULL, sizeof(dummy));
	status = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));

	if (status < 0 || WARN_ON(rx[7] != C_SPI_ACK))
		return -EIO;

	return size;
}

static ssize_t _c_spi_write(struct spi_device *spi, u8 *buf, ssize_t size)
{

	struct spi_transfer xfer[4] = {{0},};
	u32 cmd, dummy = 0xffffffff;
	u8 tx[8], rx[8];
	ssize_t status;

	if (size == 0 || buf == NULL)
		return -EINVAL;

	cmd = C_SPI_WRITE | C_SPI_BURST | C_SPI_FIXED;
	cmd |= C_SPI_ADDR(C_SPI_RXQ_WINDOW) | C_SPI_LEN(size);
	put_unaligned_be32(cmd, (u32 *)tx);
	tx[4] = (compute_crc7(tx, 4) << 1) | 0x1;

	spi_set_transfer(&xfer[0], tx, rx, 8);
	spi_set_transfer(&xfer[1], buf, NULL, size);
	spi_set_transfer(&xfer[2], &dummy, NULL, sizeof(dummy));
	dummy = 0xffffffff;
	spi_set_transfer(&xfer[3], &dummy, NULL, sizeof(dummy));

	status = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (status < 0 || WARN_ON(rx[7] != C_SPI_ACK))
		return -EIO;

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
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;

	WARN_ON(!priv->ops);
	WARN_ON(!priv->ops->read_regs);

	return (*priv->ops->read_regs)(spi, addr, buf, size);
}

static int c_spi_write_reg(struct spi_device *spi, u8 addr, u8 data)
{
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;

	WARN_ON(!priv->ops);
	WARN_ON(!priv->ops->write_reg);

	return (*priv->ops->write_reg)(spi, addr, data);
}

static ssize_t c_spi_read(struct spi_device *spi, u8 *buf, ssize_t size)
{
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;

	WARN_ON(!priv->ops);
	WARN_ON(!priv->ops->read);

	return (*priv->ops->read)(spi, buf, size);
}

static ssize_t c_spi_write(struct spi_device *spi, u8 *buf, ssize_t size)
{
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;

	WARN_ON(!priv->ops);
	WARN_ON(!priv->ops->write);

	return (*priv->ops->write)(spi, buf, size);
}

static inline struct spi_device *hifdev_to_spi(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = (void *)(hdev + 1);

	return priv->spi;
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
	struct nrc_hif_device *hdev = spi_get_drvdata(spi);
	//struct nrc_spi_priv *priv = hdev->priv;
	//struct nrc *nw = hdev->nw;

	static const int def_slot = 4;
#ifdef CONFIG_TRX_BACKOFF
	int backoff;
#endif
	skb = dev_alloc_skb(priv->slot[RX_SLOT].size * def_slot);

	/* Wait until at least one rx slot is non-empty */
	ret = wait_event_interruptible(priv->wait,
			((c_spi_num_slots(priv, RX_SLOT) > 0) ||
			 kthread_should_stop()));
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
	SYNC_LOCK(hdev);

	if (c_spi_num_slots(priv, RX_SLOT) > 32) {
		SYNC_UNLOCK(hdev);
		pr_err("!!!!! garbage rx data\n");
		goto fail;
	}

	/* README: read dummy data 4bytes for workaround */
	size = c_spi_read(spi, skb->data, priv->slot[RX_SLOT].size + 4);
	SYNC_UNLOCK(hdev);
	if (size < 0)
		goto fail;

	/* Calculate how many more slot to read for this hif packet */
	hif = (void *)skb->data;

	if (hif->type >= HIF_TYPE_MAX || hif->len == 0) {
		print_hex_dump(KERN_DEBUG, "rxskb ", DUMP_PREFIX_NONE, 16, 1,
				skb->data, 480, false);
		msleep(100);
		BUG();
	}

	nr_slot = DIV_ROUND_UP(sizeof(*hif) + hif->len,
			priv->slot[RX_SLOT].size);
	priv->slot[RX_SLOT].tail++;

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

	/*
	 * Block until priv->nr_rx_slot >= nr_slot).
	 * The irq thread will wake me up.
	 */
	if (wait_event_interruptible(priv->wait,
				(c_spi_num_slots(priv, RX_SLOT) >= nr_slot) ||
				 kthread_should_stop()))
		goto fail;

	if (kthread_should_stop())
		goto fail;

	priv->slot[RX_SLOT].tail += nr_slot;

	second_length = hif->len + sizeof(*hif) - priv->slot[RX_SLOT].size;
	/* README: align with 4bytes dummy data */
	second_length = (second_length + 4) & 0xFFFFFFFC;

	/* README
	 * To increase RX data CSPI efficiency,
	 * it use the actual spi length when spi read at second stage
	 */
	SYNC_LOCK(hdev);

	if (c_spi_num_slots(priv, RX_SLOT) > 32) {
		SYNC_UNLOCK(hdev);
		pr_err("@@@@@@ garbage rx data\n");
		goto fail;
	}
	size = c_spi_read(spi, skb->data + priv->slot[RX_SLOT].size,
			second_length);
	SYNC_UNLOCK(hdev);

	if (size < 0)
		goto fail;

out:
	skb_put(skb, sizeof(*hif) + hif->len);
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


static void spi_credit_skb(struct spi_device *spi)
{
	struct nrc_hif_device *hdev = spi_get_drvdata(spi);
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

/**
 * spi_loopback - fetch a single hif packet from the target
 *                and send it back
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

	size = c_spi_read(spi, skb->data, priv->slot[RX_SLOT].size + 4);
	if (size < 0)
		goto end;

	hif = (void*)skb->data;

	nr_slot = DIV_ROUND_UP(sizeof(*hif) + hif->len,
			priv->slot[RX_SLOT].size);
	priv->slot[RX_SLOT].tail++;

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
	return ret;
}

/**
 * spi_rx_thread
 *
 */
static int spi_rx_thread(void *data)
{
	struct nrc_hif_device *hdev = data;
	struct nrc_spi_priv *priv = (void *)(hdev + 1);
	struct spi_device *spi = priv->spi;
	struct sk_buff *skb;
	struct nrc *nw = hdev->nw;
	int ret;
	int is_suspend;

	while (!kthread_should_stop()) {
		if (nw->loopback) {
			ret = spi_loopback(spi, priv, nw->lb_count);
			if (ret <= 0)
				nrc_dbg(NRC_DBG_HIF,
						"loopback (%d) error.\n", ret);
			continue;
		}

		skb = spi_rx_skb(spi, priv);
		is_suspend = atomic_read(&suspend);
		if (skb && is_suspend) {
			dev_kfree_skb(skb);
			atomic_set(&suspend, 0);
		} else if (skb)
			hdev->hif_ops->receive(hdev, skb);
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
	struct nrc_hif_device *hdev = spi_get_drvdata(spi);
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_status_reg *status = &priv->hw.status;
	struct nrc *nw = hdev->nw;
	int ret, ac;
	u32 rear;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	int cpuid = smp_processor_id();

	nrc_dbg(NRC_DBG_HIF, "+[%d] %s\n", cpuid,  __func__);
#endif

	SYNC_LOCK(hdev);
	ret = c_spi_read_regs(spi, C_SPI_EIRQ_MODE, (void *)status,
			sizeof(*status));
	SYNC_UNLOCK(hdev);
	if (ret < 0)
		return ret;

	priv->slot[TX_SLOT].count = status->rxq_status[1] & RXQ_SLOT_COUNT;
	priv->slot[RX_SLOT].count = status->txq_status[1] & TXQ_SLOT_COUNT;
	priv->slot[RX_SLOT].head = __be32_to_cpu(status->msg[0]) & 0xffff;
	priv->slot[TX_SLOT].head = __be32_to_cpu(status->msg[0]) >> 16;

	if (nw->loopback)
		return 0;

	/* Update VIF0 credit */
	rear = __be32_to_cpu(status->msg[1]);
	for (ac = 0; ac < 4; ac++)
		priv->rear[ac] = (rear >> 8*ac) & 0xff;

	/* Update VIF1 credit */
	rear = __be32_to_cpu(status->msg[2]);
	for (ac = 0; ac < 4; ac++)
		priv->rear[6+ac] = (rear >> 8*ac) & 0xff;


#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF,
	"* rxslot:%d, txslot:h=%d,t=%d, front:%d,%d,%d,%d,%d,%d,%d,%d rear:%d,%d,%d,%d,%d,%d,%d,%d\n",
			c_spi_num_slots(priv, RX_SLOT),
			priv->slot[TX_SLOT].head, priv->slot[TX_SLOT].tail,
			priv->front[0], priv->front[1],
			priv->front[2], priv->front[3],
			priv->front[4], priv->front[5],
			priv->front[6], priv->front[7],
			priv->rear[0], priv->rear[1],
			priv->rear[2], priv->rear[3],
			priv->rear[4], priv->rear[5],
			priv->rear[6], priv->rear[7]);

	if (c_spi_num_slots(priv, RX_SLOT) > 32)
		spi_print_status(status);
#endif

	spi_credit_skb(spi);
#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "-%s\n", __func__);
#endif
	return 0;
}

#ifdef CONFIG_SUPPORT_THREADED_IRQ
/* threaded irq handler */
static irqreturn_t spi_irq(int irq, void *data)
{
	struct nrc_hif_device *hdev = data;
	struct nrc_spi_priv *priv = (void *)(hdev + 1);
	struct spi_device *spi = priv->spi;
	//struct nrc *nw = hdev->nw;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "%s\n", __func__);
#endif

	spi_update_status(spi);
	wake_up_interruptible(&priv->wait);
	return IRQ_HANDLED;
}
#else
static irqreturn_t spi_irq(int irq, void *data)
{
	struct nrc_hif_device *hdev = data;
	struct nrc_spi_priv *priv = (void *)(hdev + 1);

	queue_work(priv->irq_wq, &priv->irq_work);

	return IRQ_HANDLED;
}

static void irq_worker(struct work_struct *work)
{
	struct nrc_spi_priv *priv = container_of(work,
			struct nrc_spi_priv, irq_work);
	struct spi_device *spi = priv->spi;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "%s\n", __func__);
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

	nrc_dbg(NRC_DBG_HIF, "[%s++]\n", __func__);
	ret = c_spi_read_regs(priv->spi, C_SPI_WAKE_UP, (void *)&sys,
			sizeof(sys));
	sys.sw_id = be32_to_cpu(sys.sw_id);
	nrc_dbg(NRC_DBG_HIF, "[%s, %d] sys.sw_id = 0x%x\n",
			__func__, __LINE__, sys.sw_id);
	if (ret < 0) {
		nrc_dbg(NRC_DBG_HIF, "failed to read register 0x0\n");
		return false;
	}
	nrc_dbg(NRC_DBG_HIF, "[%s--]\n", __func__);
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
			nrc_dbg(NRC_DBG_HIF, "[%s, %d] Failed to read regs\n",
					__func__, __LINE__);
			return ret;
		}
	} while ((status.rxq_status[1] & RXQ_SLOT_COUNT) < 1);
	return 0;
}

static int spi_wait_for_xmit(struct nrc_hif_device *hdev, struct sk_buff *skb)
{
	struct nrc_spi_priv *priv = hdev->priv;

	int ret, nr_slot = DIV_ROUND_UP(skb->len, priv->slot[TX_SLOT].size);

	schedule_delayed_work(&priv->work, msecs_to_jiffies(5));
	ret = wait_event_interruptible(priv->wait,
			(c_spi_num_slots(priv, TX_SLOT) >= nr_slot) ||
				 kthread_should_stop());
	if (ret < 0)
		return ret;

	cancel_delayed_work_sync(&priv->work);

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

	if (nw->drv_state == NRC_DRV_CLOSING)
		return 0;

	if (nw->loopback)
		return 0;

	if (nw->drv_state == NRC_DRV_CLOSING)
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
			nrc_dbg(NRC_DBG_HIF, "%s: backoff=%d\n",
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
	"%s ac=%d skb=%p, slot=%d(%d/%d),fwpend:%d/%d, pending=%d\n",
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
	//struct nrc_hif_device *hdev = spi_get_drvdata(spi);
	//struct nrc *nw = hdev->nw;

#ifdef CONFIG_NRC_HIF_PRINT_FLOW_CONTROL
	nrc_dbg(NRC_DBG_HIF, "%s\n", __func__);
#endif

	spi_update_status(spi);
	wake_up_interruptible(&priv->wait);
}

static int spi_start(struct nrc_hif_device *dev)
{
	struct nrc_spi_priv *priv = dev->priv;
	struct spi_device *spi = priv->spi;
	struct spi_status_reg *status = &priv->hw.status;
	int ret;

	/* Start rx thread */
	priv->kthread = kthread_run(spi_rx_thread, dev, "spi-rx");
	INIT_DELAYED_WORK(&priv->work, spi_poll_status);

	/* Enable interrupt */
#ifdef CONFIG_SUPPORT_THREADED_IRQ
	ret = request_threaded_irq(gpio_to_irq(spi->irq), NULL, spi_irq,
			  IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
			  "spi-irq", dev);
#else
	ret = request_irq(gpio_to_irq(spi->irq), spi_irq,
			  IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
			  "spi-irq", dev);
#endif
	atomic_set(&irq_enabled, 1);

	if (ret < 0) {
#ifdef CONFIG_SUPPORT_THREADED_IRQ
		nrc_dbg(NRC_DBG_HIF, "request_irq() is failed\n");
#else
		nrc_dbg(NRC_DBG_HIF, "request_threaded_irq() is failed\n");
#endif
		goto kill_kthread;
	}

	ret = spi_update_status(spi);
	/* Restore last state */
	priv->slot[RX_SLOT].tail = priv->slot[RX_SLOT].head;
	priv->slot[TX_SLOT].tail = __be32_to_cpu(status->msg[3]) & 0xffff;

	atomic_set(&suspend, 0);
	return ret;

 kill_kthread:
	kthread_stop(priv->kthread);
	priv->kthread = NULL;

	return 0;
}

static int spi_suspend(struct nrc_hif_device *dev)
{
	atomic_set(&suspend, 1);
	return 0;
}

static int spi_resume(struct nrc_hif_device *dev)
{
	atomic_set(&suspend, 0);
	return 0;
}

static int spi_stop(struct nrc_hif_device *dev)
{
	//struct nrc_spi_priv *priv = dev->priv;
	//struct spi_device *spi = priv->spi;
	//BUG_ON(priv == NULL);
	//BUG_ON(spi == NULL);

	//synchronize_irq(gpio_to_irq(spi->irq));
	//free_irq(gpio_to_irq(spi->irq), dev);
	return 0;
}

static void spi_close(struct nrc_hif_device *dev)
{
	struct nrc_spi_priv *priv = dev->priv;

	priv->slot[TX_SLOT].count = 999;
	kthread_stop(priv->kthread);
	wake_up_interruptible(&priv->wait);
}


int spi_test(struct nrc_hif_device *hdev)
{
/* temporary function for soc */
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	struct spi_status_reg *status = &priv->hw.status;
	int ret;

	nrc_dbg(NRC_DBG_HIF, "+ read status\n");
	ret = c_spi_read_regs(spi, C_SPI_EIRQ_MODE, (void *)status,
			sizeof(*status));
	nrc_dbg(NRC_DBG_HIF, "- read status\n");

	print_hex_dump(KERN_DEBUG, "result ", DUMP_PREFIX_NONE, 16, 1,
			status, sizeof(*status), false);

	if (ret < 0)
		return ret;

	return 0;
}

void spi_reset(struct nrc_hif_device *hdev)
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
	struct nrc_hif_device *hdev = dev;
	struct nrc_spi_priv *priv = (void *)(hdev + 1);
	struct spi_device *spi = priv->spi;
	int ac;

	priv->slot[RX_SLOT].tail = priv->slot[RX_SLOT].head = 0;
	priv->slot[TX_SLOT].tail = priv->slot[TX_SLOT].head = 0;

	for (ac = 0; ac < CREDIT_QUEUE_MAX; ac++) {
		priv->front[ac] = 0;
		priv->rear[ac] = 0;
	}

	c_spi_write_reg(spi, C_SPI_EIRQ_MODE, CSPI_EIRQ_MODE);
	c_spi_write_reg(spi, C_SPI_EIRQ_ENABLE, CSPI_EIRQ_ENABLE);
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

	//disable_irq(gpio_to_irq(spi->irq));
	disable_irq_nosync(gpio_to_irq(spi->irq));
}

static void spi_enable_irq(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;

	enable_irq(gpio_to_irq(spi->irq));
}

static int spi_status_irq(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_device *spi = priv->spi;
	int irq = gpio_get_value(spi->irq);

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

static bool spi_support_fastboot(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = hdev->priv;

	return priv->fastboot;
}

static struct nrc_hif_ops spi_ops = {
	.name = spi_name,
	.check_fw = spi_check_fw,
	.xmit = spi_xmit,
	.wait_for_xmit = spi_wait_for_xmit,
	.write = spi_raw_write,
	.wait_ack = spi_wait_ack,
	.start = spi_start,
	.stop = spi_stop,
	.suspend = spi_suspend,
	.resume = spi_resume,
	.close = spi_close,
	.reset = spi_reset,
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
	.support_fastboot = spi_support_fastboot,
};

static void c_spi_enable_irq(struct spi_device *spi, bool enable)
{

	if (enable) {
		c_spi_write_reg(spi, C_SPI_EIRQ_MODE, CSPI_EIRQ_MODE);
		c_spi_write_reg(spi, C_SPI_EIRQ_ENABLE, CSPI_EIRQ_ENABLE);
	} else {
		c_spi_write_reg(spi, C_SPI_EIRQ_MODE, 0x0);
		c_spi_write_reg(spi, C_SPI_EIRQ_ENABLE, 0x0);
	}
}

static void c_spi_config(struct spi_device *spi)
{
	struct nrc_hif_device *hdev = spi_get_drvdata(spi);
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_sys_reg *sys = &priv->hw.sys;

	sys->chip_id = be16_to_cpu(sys->chip_id);
	sys->sw_id = be32_to_cpu(sys->sw_id);

	switch (sys->chip_id) {
	case 0x7291:
		priv->slot[TX_SLOT].size = 456;
		priv->slot[RX_SLOT].size = 492;
		spi_ops.need_maskrom_war = true;
		spi_ops.sync_auto = true;
		break;
	case 0x7391:
	case 0x7292:
		priv->slot[TX_SLOT].size = 456;
		priv->slot[RX_SLOT].size = 492;
		spi_ops.need_maskrom_war = false;
		spi_ops.sync_auto = false;
		//spi_ops.sync_auto = true;
		break;
	case 0x6201:
		priv->slot[TX_SLOT].size = 456;
		priv->slot[RX_SLOT].size = 492;
		spi_ops.need_maskrom_war = false;
		spi_ops.sync_auto = true;
		break;
	default:
		nrc_dbg(NRC_DBG_HIF,
			"Unknown Newracom IEEE80211 chipset %04x\n",
			sys->chip_id);
		BUG();
	}

	nrc_dbg(NRC_DBG_HIF,
	"Newracom IEEE802.11 C-SPI: chipid=%04x, sw_id=%04x, board_id=%04X\n",
		sys->chip_id, sys->sw_id, sys->board_id);
	if (sys->sw_id == SW_MAGIC_FOR_BOOT)
		nrc_dbg(NRC_DBG_HIF, "Boot loader\n");
	else if (sys->sw_id == SW_MAGIC_FOR_FW)
		nrc_dbg(NRC_DBG_HIF, "Firmware\n");

	c_spi_enable_irq(spi, true);
}

static int c_spi_probe(struct spi_device *spi)
{
	struct nrc_hif_device *hdev = spi->dev.platform_data;
	struct nrc_spi_priv *priv = hdev->priv;
	struct spi_sys_reg *sys = &priv->hw.sys;
	int ret;
	int i;

	priv->spi = spi;
	priv->loopback_prev_cnt = 0;
	priv->loopback_total_cnt = 0;
	priv->loopback_last_jiffies = 0;
	priv->loopback_read_usec = 0;
	priv->loopback_write_usec = 0;
	priv->fastboot = false;

	init_waitqueue_head(&priv->wait);
	spin_lock_init(&priv->lock);
	spi_set_drvdata(spi, hdev);
	memcpy(&priv->spi_r, spi, sizeof(*spi));
	priv->ops = &cspi_ops;
	if (alternate_mode) {
		priv->spi_r.mode = SPI_MODE_1;
		priv->ops = &cspi_ops_alt;
	}

	if (fw_name && enable_hspi_init)
		spi_reset(hdev);

	/* Read the register */
	ret = c_spi_read_regs(spi, C_SPI_WAKE_UP, (void *)sys, sizeof(*sys));
	if (ret < 0) {
		nrc_dbg(NRC_DBG_HIF, "failed to read register 0x0\n");
		goto fail;
	}

	if (fw_name)
		spi_reset(hdev);
	c_spi_config(spi);

	for (i = 0; i < CREDIT_QUEUE_MAX; i++)
		priv->credit_max[i] = 0;

	switch (sys->chip_id) {
	case 0x7291:
		priv->credit_max[0] = CREDIT_AC0;
		priv->credit_max[1] = CREDIT_AC1;
		priv->credit_max[2] = CREDIT_AC2;
		priv->credit_max[3] = CREDIT_AC3_7291;

		priv->credit_max[6] = CREDIT_AC0;
		priv->credit_max[7] = CREDIT_AC1;
		priv->credit_max[8] = CREDIT_AC2;
		priv->credit_max[9] = CREDIT_AC3_7291;
		priv->fastboot = false;
	case 0x7292:
		priv->credit_max[0] = CREDIT_AC0;
		priv->credit_max[1] = CREDIT_AC1;
		priv->credit_max[2] = CREDIT_AC2;
		priv->credit_max[3] = CREDIT_AC3_7291;

		priv->credit_max[6] = CREDIT_AC0;
		priv->credit_max[7] = CREDIT_AC1;
		priv->credit_max[8] = CREDIT_AC2;
		priv->credit_max[9] = CREDIT_AC3_7291;

		/* README: Temporary Disable Fastboot function of CM0.
		 * To testing BA Encoding Workaround -- swki -- 2020-0518
		 */
		priv->fastboot = false;
		break;

	case 0x7391:
		priv->credit_max[0] = 4;
		priv->credit_max[1] = 20;
		priv->credit_max[2] = 4;
		priv->credit_max[3] = 4;

		priv->credit_max[6] = 4;
		priv->credit_max[7] = 20;
		priv->credit_max[8] = 4;
		priv->credit_max[9] = 4;
		priv->fastboot = false;
		break;

	case 0x6201:
		priv->credit_max[0] = CREDIT_AC0;
		priv->credit_max[1] = CREDIT_AC1;
		priv->credit_max[2] = CREDIT_AC2;
		priv->credit_max[3] = CREDIT_AC3;

		priv->credit_max[6] = CREDIT_AC0;
		priv->credit_max[7] = CREDIT_AC1;
		priv->credit_max[8] = CREDIT_AC2;
		priv->credit_max[9] = CREDIT_AC3;
		priv->fastboot = false;
		break;
	}

	/* Claim gpio used for irq */
	if (gpio_request(spi->irq, "spi-irq") < 0) {
		nrc_dbg(NRC_DBG_HIF, "gpio_reqeust() is failed\n");
		gpio_free(spi->irq);
		goto fail;
	}
	gpio_direction_input(spi->irq);

#if defined(SPI_DBG)
	/* Claim gpio used for debugging */
	if (gpio_request(SPI_DBG, "spi-dgb") < 0) {
		nrc_dbg(NRC_DBG_HIF, "gpio_reqeust() is failed\n");
		gpio_free(SPI_DBG);
		goto fail;
	}
	gpio_direction_output(SPI_DBG, 1);
#endif

	return 0;
fail:
	return -EINVAL;
}

static int c_spi_remove(struct spi_device *spi)
{
	//struct nrc_hif_device *hdev = spi->dev.platform_data;
#if !defined(CONFIG_SUPPORT_THREADED_IRQ)
	struct nrc_spi_priv *priv = hdev->priv;
#endif
	//free_irq(gpio_to_irq(spi->irq), hdev);
	gpio_free(spi->irq);
#if defined(SPI_DBG)
	gpio_free(SPI_DBG);
#endif

#if !defined(CONFIG_SUPPORT_THREADED_IRQ)
	flush_workqueue(priv->irq_wq);
	destroy_workqueue(priv->irq_wq);
#endif
	return 0;
}

static struct spi_driver spi_driver = {
	.probe = c_spi_probe,
	.remove = c_spi_remove,
	.driver = {
		.name = "nrc-cspi",
	},
};

static struct spi_board_info bi = {
	.modalias = "nrc-cspi",
//	.chip_select = 0,
	.mode = SPI_MODE_0,
};

struct nrc_hif_device *nrc_hif_cspi_init(struct nrc *nw)
{
	struct nrc_hif_device *hdev;
	struct nrc_spi_priv *priv;
	struct spi_device *spi;
	struct spi_master *master;
	int ret;

	hdev = kzalloc(sizeof(*hdev) + sizeof(*priv), GFP_KERNEL);
	if (!hdev) {
		/*nrc_dbg(NRC_DBG_HIF, "failed to allocate nrc_hif_device");*/
		return NULL;
	}
	priv = (void *)(hdev + 1);
	mutex_init(&priv->bus_lock_mutex);
	hdev->priv = priv;
	hdev->nw = nw;
	hdev->hif_ops = &spi_ops;

#if !defined(CONFIG_SUPPORT_THREADED_IRQ)
	priv->irq_wq = create_singlethread_workqueue("nrc_cspi_irq");
	INIT_WORK(&priv->irq_work, irq_worker);
#endif

	/* Apply module parameters */
	bi.bus_num = spi_bus_num;
	bi.chip_select = spi_cs_num;
	bi.irq = spi_gpio_irq;
	bi.platform_data = hdev;
	bi.max_speed_hz = hifspeed;

	nrc_dbg(NRC_DBG_HIF, "max_speed_hz = %d\n", bi.max_speed_hz);

	/* Find the spi master that our device is attached to */
	master = spi_busnum_to_master(spi_bus_num);
	if (!master) {
		nrc_dbg(NRC_DBG_HIF, "could not find spi master with busnum=%d\n",
			spi_bus_num);
		goto fail;
	}

	/* Instantiate and add a spi device */
	spi = spi_new_device(master, &bi);
	if (!spi) {
		nrc_dbg(NRC_DBG_HIF, "failed to add spi device\n");
		goto fail;
	}

	/* Register spi driver */
	ret = spi_register_driver(&spi_driver);
	if (ret < 0) {
		nrc_dbg(NRC_DBG_HIF, "failed to register driver %s\n",
			spi_driver.driver.name);
		goto unregister_device;
	}

#ifdef CONFIG_TRX_BACKOFF
	atomic_set(&priv->trx_backoff, 0);
#endif
	return hdev;

unregister_device:
	spi_unregister_device(spi);
fail:
	kfree(hdev);

	return NULL;
}

int nrc_hif_cspi_exit(struct nrc_hif_device *hdev)
{
	struct nrc_spi_priv *priv = NULL;
	struct spi_device *spi = NULL;

	if (!hdev)
		return 0;

	priv = hdev->priv;
	spi = priv->spi;

	cancel_delayed_work(&priv->work);

	synchronize_irq(gpio_to_irq(spi->irq));
	free_irq(gpio_to_irq(spi->irq), hdev);

	spi_unregister_device(spi);
	spi_unregister_driver(&spi_driver);

	kfree(hdev);

	return 0;
}
