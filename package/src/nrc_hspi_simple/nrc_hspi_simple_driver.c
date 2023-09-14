/*
 * Copyright (c) 2016-2021 Newracom, Inc.
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
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <asm/unaligned.h>


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
#define C_SPI_READ		(0x50000000)
#define C_SPI_WRITE		(0x50400000)
#define C_SPI_BURST		(0x00800000)
#define C_SPI_FIXED		(0x00200000)
#define C_SPI_ADDR(x)	(((x) & 0xff) << 13)
#define C_SPI_LEN(x)	((x) & 0x1fff)
#define C_SPI_WDATA(x)	((x) & 0xff)
#define C_SPI_ACK		(0x47)

/* HSPI registers */
#define C_SPI_WAKE_UP 0x0

struct spi_device *spi;

struct spi_sys_reg {
	u8 wakeup;		/* 0x0 */
	u8 status;		/* 0x1 */
	u16 chip_id;	/* 0x2-0x3 */
	u32 modem_id;	/* 0x4-0x7 */
	u32 sw_id;		/* 0x8-0xb */
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
} __packed;

int hifspeed = (20*1000*1000);
module_param(hifspeed, int, 0600);
MODULE_PARM_DESC(hifspeed, "SPI master max speed");

int spi_bus_num;
module_param(spi_bus_num, int, 0600);
MODULE_PARM_DESC(spi_bus_num, "SPI controller bus number");

int spi_cs_num;
module_param(spi_cs_num, int, 0600);
MODULE_PARM_DESC(spi_cs_num, "SPI chip select number");

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

static int c_spi_read_regs(struct spi_device *spi,
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

	if (status < 0 || WARN_ON(rx[7] != C_SPI_ACK)) {
		return -EIO;
	}

	if (size == 1)
		buf[0] = rx[6];

	return 0;
}

static int nrc_hspi_probe(struct spi_device *spi)
{
	int ret = 0;
	struct spi_sys_reg sys = {0,};

	pr_err("[%s,L%d]", __func__, __LINE__);

	/* Read the register */
	ret = c_spi_read_regs(spi, C_SPI_WAKE_UP, (void *)&sys, sizeof(struct spi_sys_reg));
	if (ret < 0) {
		pr_err("[Error] failed to read register(0x0).");
		return ret;
	}

	print_hex_dump(KERN_DEBUG, "spi_sys_reg ", DUMP_PREFIX_NONE, 16, 1,
				&sys, sizeof(struct spi_sys_reg), false);

	return ret;
}

static int nrc_hspi_remove(struct spi_device *spi)
{
	pr_err("[%s,L%d]", __func__, __LINE__);
	return 0;
}

static struct spi_driver hspi_simple_driver = {
	.driver = {
		.name	= "nrc-hspi-simple",
	},
	.probe		= nrc_hspi_probe,
	.remove		= nrc_hspi_remove,
};

static struct spi_board_info bi = {
	.modalias = "nrc-hspi-simple",
	.mode = SPI_MODE_0,
};

static int __init nrc_init(void)
{
	struct spi_master *master;
	int ret;

	/* Apply module parameters */
	bi.bus_num = spi_bus_num;
	bi.chip_select = spi_cs_num;
	bi.max_speed_hz = hifspeed;

	pr_err("### [nrc_simple] Value of paramters ###");
	pr_err("- bus_num: %d", bi.bus_num);
	pr_err("- chip_select: %d", bi.chip_select);
	pr_err("- max_speed_hz: %d", bi.max_speed_hz);

	/* Find the spi master that our device is attached to */
	master = spi_busnum_to_master(spi_bus_num);
	if (!master) {
		pr_err("[Error] could not find spi master with the bus number %d.",
			spi_bus_num);
		return -EINVAL;
	}

	/* Instantiate and add a spi device */
	spi = spi_new_device(master, &bi);
	if (!spi) {
		pr_err("[Error] failed to instantiate a new spi device.");
		return -EINVAL;
	}

	/* Register spi driver */
	ret = spi_register_driver(&hspi_simple_driver);
	if (ret) {
		pr_err("[Error %d] failed to register spi driver(%s).",
			ret, hspi_simple_driver.driver.name);
		return ret;
	}

	pr_err("done successfully.");
	return 0;
}
module_init(nrc_init);

static void __exit nrc_exit(void)
{
	pr_err("+%s", __func__);

	spi_unregister_driver(&hspi_simple_driver);
	pr_err("[%s,L%d]", __func__, __LINE__);
	spi_unregister_device(spi);

	pr_err("-%s\n\n", __func__);
}
module_exit(nrc_exit);

MODULE_AUTHOR("Newracom, Inc.(http://www.newracom.com)");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Newracom HSPI simple driver");
#if KERNEL_VERSION(5, 12, 0) > LINUX_VERSION_CODE
MODULE_SUPPORTED_DEVICE("Newracom 802.11 devices");
#endif
