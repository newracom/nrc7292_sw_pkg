/* SPDX-License-Identifier: GPL-2.0 */
/*
 * FTDI FT232H MPSSE SPI driver
 *
 * Copyright (C) 2017 - 2018 DENX Software Engineering
 * Anatolij Gustschin <agust@denx.de>
 *
 * Note) This file is modified by Newracom, Inc. <www.newwracom.com>
 * 			- Original File Name : spi_ftdi_mpsse.c
 *
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/machine.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <linux/usb.h>

#include "ft232h-intf.h"


static int param_latency = 1;
module_param_named(latency, param_latency, int, 0600);
MODULE_PARM_DESC(latency, "latency timer value (1ms ~ 255ms, default 1ms)");

static int param_bus_num = -1;
module_param_named(spi_bus_num, param_bus_num, int, 0600);
MODULE_PARM_DESC(spi_bus_num, "SPI controller bus number (if negative, dynamic allocation)");

struct ftdi_spi {
	struct platform_device *pdev;
	struct usb_interface *intf;
	struct spi_controller *master;
	const struct ft232h_intf_ops *iops;

	u8 txrx_cmd;
	u8 rx_cmd;
	u8 tx_cmd;
	u8 xfer_buf[SZ_64K];
	u16 last_mode;
	u32 last_speed_hz;
};

static int ftdi_spi_setup (struct spi_device *spi)
{
	struct spi_master *master = spi->master;
	struct ftdi_spi *priv = spi_controller_get_devdata(master);
	int ret;

	if (spi->chip_select != 0 || spi->bits_per_word != 8)
		return -EINVAL;

	if (spi->max_speed_hz < master->min_speed_hz ||
		spi->max_speed_hz > master->max_speed_hz)
		return -EINVAL;

	ret = priv->iops->set_clock_divisor(priv->intf, spi->max_speed_hz);
	if (ret < 0) {
		dev_err(&priv->pdev->dev, "Clk divisor setting failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static void ftdi_spi_set_cs(struct spi_device *spi, bool enable)
{
	struct ftdi_spi *priv = spi_controller_get_devdata(spi->master);
	int ret;

	dev_dbg(&priv->pdev->dev, "%s: CS %u, cs mode %d, val %d\n",
			__func__, spi->chip_select, (spi->mode & SPI_CS_HIGH), enable);

	ret = priv->iops->set_cs_pin(priv->intf, enable ? MPSSE_GPIO_HIGH : MPSSE_GPIO_LOW);
	if (ret < 0)
		dev_err(&priv->pdev->dev, "CS pin setting failed: %d\n", ret);
}

static inline u8 ftdi_spi_txrx_byte_cmd(struct spi_device *spi)
{
	u8 mode = spi->mode & (SPI_CPOL | SPI_CPHA);
	u8 cmd;

	if (spi->mode & SPI_LSB_FIRST) {
		switch (mode) {
			case SPI_MODE_0:
			case SPI_MODE_1:
				cmd = TXF_RXR_BYTES_LSB;
				break;
			case SPI_MODE_2:
			case SPI_MODE_3:
				cmd = TXR_RXF_BYTES_LSB;
				break;
		}
	} else {
		switch (mode) {
			case SPI_MODE_0:
			case SPI_MODE_1:
				cmd = TXF_RXR_BYTES_MSB;
				break;
			case SPI_MODE_2:
			case SPI_MODE_3:
				cmd = TXR_RXF_BYTES_MSB;
				break;
		}
	}
	return cmd;
}

static inline int ftdi_spi_loopback_cfg(struct ftdi_spi *priv, int on)
{
	int ret;

	priv->xfer_buf[0] = on ? LOOPBACK_ON : LOOPBACK_OFF;

	ret = priv->iops->write_data(priv->intf, priv->xfer_buf, 1);
	if (ret < 0)
		dev_warn(&priv->pdev->dev, "loopback %d failed\n", on);
	return ret;
}

static int ftdi_spi_tx_rx(struct ftdi_spi *priv, struct spi_device *spi,
							struct spi_transfer *xfer)
{
	struct usb_interface *intf = priv->intf;
	struct device *dev = &priv->pdev->dev;
	const struct ft232h_intf_ops *ops = priv->iops;
	const char *tx_buf = xfer->tx_buf;
	char *rx_buf = xfer->rx_buf;
	uint32_t trx_len = xfer->len;
	char *wr_buf = priv->xfer_buf;
	uint32_t wr_len_max = sizeof(priv->xfer_buf) - 3;
	uint32_t wr_len;
	uint32_t rd_len;
	int retry;
	int ret;
	int i, j;

	ops->lock(intf);

	wr_buf[0] = priv->txrx_cmd;

	for (i = 0 ; i < trx_len ; i += wr_len)
	{
		wr_len = min_t(size_t, trx_len - i, wr_len_max);

		wr_buf[1] = wr_len - 1;
		wr_buf[2] = (wr_len - 1) >> 8;

		memcpy(&wr_buf[3], tx_buf + i, wr_len);

		for (retry = j = 0 ; j < (3 + wr_len) ; j += ret)
		{
			ret = ops->write_data(intf, wr_buf + j, 3 + wr_len - j);
			if (ret < 0)
				goto trx_failed;

			if (ret == 0)
			{
				if (++retry > 10)
				{
					ret = -ETIMEDOUT;
					goto trx_failed;
				}

				dev_dbg(dev, "%s: retry=%d\n", __func__, retry);
				continue;
			}

			retry = 0;
			if ((j + ret) > 3)
				dev_dbg(dev, "%s: write, %u/%u byte(s) done\n", __func__, i + j + ret - 3, trx_len);
		}

		rd_len = wr_len;

		for (retry = j = 0 ; j < rd_len ; j += ret)
		{
			ret = ops->read_data(intf, rx_buf + i + j, rd_len - j);
			if (ret < 0)
				goto trx_failed;

			if (ret == 0)
			{
				if (++retry > 10)
				{
					ret = -ETIMEDOUT;
					goto trx_failed;
				}

				dev_dbg(dev, "%s: retry=%d\n", __func__, retry);
				continue;
			}

			retry = 0;
			dev_dbg(dev, "%s: read, %u/%u byte(s) done\n", __func__, i + j + ret, trx_len);
		}
	}

	ret = 0;

trx_failed:

	ops->unlock(intf);

	if (ret < 0)
		dev_err(dev, "%s: trx failed %d\n",	__func__, ret);

	return ret;
}

static int ftdi_spi_tx(struct ftdi_spi *priv, struct spi_transfer *xfer)
{
	struct usb_interface *intf = priv->intf;
	struct device *dev = &priv->pdev->dev;
	const struct ft232h_intf_ops *ops = priv->iops;
	const char *tx_buf = xfer->tx_buf;
	uint32_t tx_len = xfer->len;
	char *wr_buf = priv->xfer_buf;
	uint32_t wr_len_max = sizeof(priv->xfer_buf) - 3;
	uint32_t wr_len;
	int retry;
	int ret;
	int i, j;

	ops->lock(intf);

	wr_buf[0] = priv->tx_cmd;

	for (i = 0 ; i < tx_len ; i += wr_len)
	{
		wr_len = min_t(size_t, tx_len - i , wr_len_max);

		wr_buf[1] = wr_len - 1;
		wr_buf[2] = (wr_len - 1) >> 8;

		memcpy(&wr_buf[3], tx_buf + i, wr_len);

		for (retry = j = 0 ; j < (3 + wr_len) ; j += ret)
		{
			ret = ops->write_data(intf, wr_buf + j, (3 + wr_len) - j);
			if (ret < 0)
				goto tx_failed;

			if (ret == 0)
			{
				if (++retry > 10)
				{
					ret = -ETIMEDOUT;
					goto tx_failed;
				}

				dev_dbg(dev, "%s: retry=%d\n", __func__, retry);
				continue;
			}

			retry = 0;
			if ((j + ret) > 3)
				dev_dbg(dev, "%s: %u/%u byte(s) done\n", __func__, i + j + ret - 3, tx_len);
		}
	}

	ret = 0;

tx_failed:

	ops->unlock(intf);

	if (ret < 0)
		dev_err(dev, "%s: tx failed %d\n",	__func__, ret);

	return ret;
}

static int ftdi_spi_rx(struct ftdi_spi *priv, struct spi_transfer *xfer)
{
	struct usb_interface *intf = priv->intf;
	struct device *dev = &priv->pdev->dev;
	const struct ft232h_intf_ops *ops = priv->iops;
	char *rx_buf = xfer->rx_buf;
	uint32_t rx_len = xfer->len;
	char *rd_buf = priv->xfer_buf;
	uint32_t rd_len_max = sizeof(priv->xfer_buf) - 3;
	uint32_t rd_len;
	int retry;
	int ret;
	int i, j;

	ops->lock(priv->intf);

	rd_buf[0] = priv->rx_cmd;

	for (i = 0 ; i < rx_len ; i += rd_len)
	{
		rd_len = min_t(size_t, rx_len - i , rd_len_max);

		rd_buf[1] = rd_len - 1;
		rd_buf[2] = (rd_len - 1) >> 8;

		ret = ops->write_data(intf, rd_buf, 3);
		if (ret < 0)
			goto rx_failed;

		for (retry = j = 0 ; j < rd_len ; j += ret)
		{
			ret = ops->read_data(intf, rx_buf + i + j, rd_len - j);
			if (ret < 0)
				goto rx_failed;

			if (ret == 0)
			{
				if (++retry > 10)
				{
					ret = -ETIMEDOUT;
					goto rx_failed;
				}

				dev_dbg(dev, "%s: retry=%d\n", __func__, retry);
				continue;
			}

			retry = 0;
			dev_dbg(dev, "%s: %u/%u byte(s) done\n", __func__, i + j + ret, rx_len);
		}
	}

	ret = 0;

rx_failed:

	ops->unlock(priv->intf);

	if (ret < 0)
		dev_err(dev, "%s: rx failed %d\n",	__func__, ret);

	return ret;
}

static int ftdi_spi_transfer_one(struct spi_controller *ctlr,
		struct spi_device *spi,
		struct spi_transfer *xfer)
{
	struct ftdi_spi *priv = spi_controller_get_devdata(ctlr);
	struct device *dev = &priv->pdev->dev;
	int ret = 0;

	if (!xfer->len)
		return 0;

	if (xfer->bits_per_word != 8)
		return -EINVAL;

	if (priv->last_speed_hz != xfer->speed_hz) {
		ret = priv->iops->set_clock_divisor(priv->intf, xfer->speed_hz);
		if (ret < 0) {
			dev_err(&priv->pdev->dev, "Clk divisor setting failed: %d\n", ret);
			return ret;
		}

		dev_dbg(&priv->pdev->dev, "new clock frequency: %u -> %u\n",
				priv->last_speed_hz, xfer->speed_hz);

		priv->last_speed_hz = xfer->speed_hz;
	}

	if (priv->last_mode != spi->mode) {
		u8 spi_mode = spi->mode & (SPI_CPOL | SPI_CPHA);
		u8 pins = 0;

		dev_dbg(dev, "%s: MODE 0x%x\n", __func__, spi->mode);

		if (spi->mode & SPI_LSB_FIRST) {
			switch (spi_mode) {
				case SPI_MODE_0:
				case SPI_MODE_3:
					priv->tx_cmd = TX_BYTES_FE_LSB;
					priv->rx_cmd = RX_BYTES_RE_LSB;
					break;
				case SPI_MODE_1:
				case SPI_MODE_2:
					priv->tx_cmd = TX_BYTES_RE_LSB;
					priv->rx_cmd = RX_BYTES_FE_LSB;
					break;
			}
		} else {
			switch (spi_mode) {
				case SPI_MODE_0:
				case SPI_MODE_3:
					priv->tx_cmd = TX_BYTES_FE_MSB;
					priv->rx_cmd = RX_BYTES_RE_MSB;
					break;
				case SPI_MODE_1:
				case SPI_MODE_2:
					priv->tx_cmd = TX_BYTES_RE_MSB;
					priv->rx_cmd = RX_BYTES_FE_MSB;
					break;
			}
		}

		priv->txrx_cmd = ftdi_spi_txrx_byte_cmd(spi);

		switch (spi_mode) {
			case SPI_MODE_2:
			case SPI_MODE_3:
				pins |= MPSSE_SK;
				break;
		}

		ret = priv->iops->cfg_bus_pins(priv->intf, MPSSE_SK | MPSSE_DO, pins);
		if (ret < 0) {
			dev_err(dev, "IO cfg failed: %d\n", ret);
			return ret;
		}
		priv->last_mode = spi->mode;
	}

	dev_dbg(dev, "%s: mode 0x%x, CMD RX/TX 0x%x/0x%x\n",
			__func__, spi->mode, priv->rx_cmd, priv->tx_cmd);

	if (xfer->tx_buf && xfer->rx_buf)
		ret = ftdi_spi_tx_rx(priv, spi, xfer);
	else if (xfer->tx_buf)
		ret = ftdi_spi_tx(priv, xfer);
	else if (xfer->rx_buf)
		ret = ftdi_spi_rx(priv, xfer);

	dev_dbg(dev, "%s: xfer ret %d\n", __func__, ret);

	spi_finalize_current_transfer(ctlr);
	return ret;
}

static int ftdi_spi_probe(struct platform_device *pdev)
{
	const int max_cs_num = 1;
	const int min_speed_hz = 450;
	const int max_speed_hz = 30000000;
	const struct mpsse_spi_platform_data *pd;
	struct device *dev = &pdev->dev;
	struct spi_controller *master;
	struct ftdi_spi *priv;
	int ret;

	pd = dev->platform_data;
	if (!pd) {
		dev_err(dev, "Missing platform data.\n");
		return -EINVAL;
	}

	if (!pd->ops ||
			!pd->ops->read_data || !pd->ops->write_data ||
			!pd->ops->lock || !pd->ops->unlock ||
			!pd->ops->set_bitmode || !pd->ops->set_clock_divisor ||
			!pd->ops->get_latency_timer || !pd->ops->set_latency_timer ||
			!pd->ops->cfg_bus_pins || !pd->ops->set_cs_pin)
		return -EINVAL;

	master = spi_alloc_master(&pdev->dev, sizeof(*priv));
	if (!master)
		return -ENOMEM;

	platform_set_drvdata(pdev, master);

	priv = spi_controller_get_devdata(master);
	priv->master = master;
	priv->pdev = pdev;
	priv->intf = to_usb_interface(dev->parent);
	priv->iops = pd->ops;
	priv->last_mode = 0xffff;
	priv->last_speed_hz = max_speed_hz;

	master->bus_num = (param_bus_num >= 0) ? param_bus_num : -1;
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LOOP | SPI_CS_HIGH | SPI_LSB_FIRST;
	master->num_chipselect = max_cs_num;
	master->min_speed_hz = min_speed_hz;
	master->max_speed_hz = max_speed_hz;
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->setup = ftdi_spi_setup;
	master->set_cs = ftdi_spi_set_cs;
	master->transfer_one = ftdi_spi_transfer_one;
	master->auto_runtime_pm = false;

	ret = spi_register_controller(master);
	if (ret < 0) {
		dev_err(dev, "Failed to register spi master\n");
		spi_controller_put(master);
		return ret;
	}

	dev_info(dev, "spi_master: bus_num=%d\n", master->bus_num);

	ret = priv->iops->set_clock_divisor(priv->intf, master->max_speed_hz);
	if (ret < 0) {
		dev_err(dev, "Clk divisor setting failed: %d\n", ret);
		goto err;
	}

	ret = priv->iops->cfg_bus_pins(priv->intf, MPSSE_SK | MPSSE_DO, 0);
	if (ret < 0) {
		dev_err(dev, "Can't init SPI bus pins: %d\n", ret);
		goto err;
	}

	if (param_latency > 0 && param_latency <= 255) {
		unsigned char latency;

		ret = priv->iops->get_latency_timer(priv->intf, &latency);
		if (ret < 0)
			goto err;

		if (latency == param_latency)
			dev_dbg(dev, "latency timer: %ums\n", latency);
		else {
			dev_dbg(dev, "latency timer: %ums -> %ums\n", latency, param_latency);

			ret = priv->iops->set_latency_timer(priv->intf, param_latency);
			if (ret < 0)
				goto err;

			ret = priv->iops->get_latency_timer(priv->intf, &latency);
			if (ret < 0)
				goto err;

			if (latency != param_latency) {
				dev_err(dev, "failed to configure latency timer: %u\n", latency);
				ret = -EIO;
				goto err;
			}
		}
	}

	return 0;
err:
	platform_set_drvdata(pdev, NULL);
	spi_unregister_controller(master);
	return ret;
}

static int ftdi_spi_slave_release(struct device *dev, void *data)
{
	struct spi_device *spi = to_spi_device(dev);

	dev_dbg(dev, "%s: remove CS %u\n", __func__, spi->chip_select);
	spi_unregister_device(spi);

	return 0;
}

static int ftdi_spi_remove(struct platform_device *pdev)
{
	struct spi_controller *master;
	struct ftdi_spi *priv;

	master = platform_get_drvdata(pdev);
	priv = spi_controller_get_devdata(master);

	device_for_each_child(&master->dev, priv, ftdi_spi_slave_release);

	spi_unregister_controller(master);

	return 0;
}

int ft232h_spi_probe(struct platform_device *pdev)
{
	return ftdi_spi_probe(pdev);
}

int ft232h_spi_remove(struct platform_device *pdev)
{
	return ftdi_spi_remove(pdev);
}

