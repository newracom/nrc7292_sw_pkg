/* SPDX-License-Identifier: GPL-2.0 */
/*
 * FTDI FT232H interface driver
 *
 * Copyright (C) 2017 - 2018 DENX Software Engineering
 * Anatolij Gustschin <agust@denx.de>
 *
 * Note) This file is modified by Newracom, Inc. <www.newwracom.com>
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/idr.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/usb/ch9.h>
#include <linux/usb.h>
#include <linux/version.h>

#include "ft232h-intf.h"


static int param_gpio_base = -1;
module_param_named(gpio_base_num, param_gpio_base, int, 0600);
MODULE_PARM_DESC(gpio_base_num, "GPIO controller base number (if negative, dynamic allocation)");


static DEFINE_IDA(ftdi_devid_ida);

static void ftdi_lock(struct usb_interface *intf)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);

	mutex_lock(&priv->ops_mutex);
}

static void ftdi_unlock(struct usb_interface *intf)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);

	mutex_unlock(&priv->ops_mutex);
}

/*
 * ftdi_ctrl_xfer - FTDI control endpoint transfer
 * @intf: USB interface pointer
 * @desc: pointer to descriptor struct for control transfer
 *
 * Return:
 * Return: If successful, the number of bytes transferred. Otherwise,
 * a negative error number.
 */
static int ftdi_ctrl_xfer(struct usb_interface *intf, struct ctrl_desc *desc)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	struct usb_device *udev = priv->udev;
	unsigned int pipe;
	int ret;

	mutex_lock(&priv->io_mutex);
	if (!priv->intf) {
		ret = -ENODEV;
		goto exit;
	}

	if (!desc->data && desc->size)
		desc->data = priv->bulk_in_buf;

	if (desc->dir_out)
		pipe = usb_sndctrlpipe(udev, 0);
	else
		pipe = usb_rcvctrlpipe(udev, 0);

	ret = usb_control_msg(udev, pipe, desc->request, desc->requesttype,
			      desc->value, desc->index, desc->data, desc->size,
			      desc->timeout);
	if (ret < 0)
		dev_dbg(&udev->dev, "ctrl msg failed: %d\n", ret);
exit:
	mutex_unlock(&priv->io_mutex);
	return ret;
}

/*
 * ftdi_bulk_xfer - FTDI bulk endpoint transfer
 * @intf: USB interface pointer
 * @desc: pointer to descriptor struct for bulk-in or bulk-out transfer
 *
 * Return:
 * If successful, 0. Otherwise a negative error number. The number of
 * actual bytes transferred will be stored in the @desc->act_len field
 * of the descriptor struct.
 */
static int ftdi_bulk_xfer(struct usb_interface *intf, struct bulk_desc *desc)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	struct usb_device *udev = priv->udev;
	unsigned int pipe;
	int ret;

	mutex_lock(&priv->io_mutex);
	if (!priv->intf) {
		ret = -ENODEV;
		goto exit;
	}

	if (desc->dir_out)
		pipe = usb_sndbulkpipe(udev, priv->bulk_out);
	else
		pipe = usb_rcvbulkpipe(udev, priv->bulk_in);

	ret = usb_bulk_msg(udev, pipe, desc->data, desc->len,
			   &desc->act_len, desc->timeout);
	if (ret)
		dev_err(&udev->dev, "bulk msg failed: %d\n", ret);

exit:
	mutex_unlock(&priv->io_mutex);
	return ret;
}

/*
 * ftdi_read_data - read from FTDI bulk-in endpoint
 * @intf: USB interface pointer
 * @buf:  pointer to data buffer
 * @len:  length in bytes of the data to read
 *
 * The two modem status bytes transferred in every read will
 * be removed and will not appear in the data buffer.
 *
 * Return:
 * If successful, the number of data bytes received (can be 0).
 * Otherwise, a negative error number.
 */
static int ftdi_read_data(struct usb_interface *intf, void *buf, size_t len)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	struct bulk_desc desc;
	int ret;

	desc.act_len = 0;
	desc.dir_out = false;
	desc.data = priv->bulk_in_buf;
	/* Device sends 2 additional status bytes, read at least len + 2 */
	desc.len = min_t(size_t, len + 2, priv->bulk_in_sz);
	desc.timeout = FTDI_USB_READ_TIMEOUT;

	ret = ftdi_bulk_xfer(intf, &desc);
	if (ret)
		return ret;

	/* Only status bytes and no data? */
	if (desc.act_len <= 2)
		return 0;

	/* Skip first two status bytes */
	ret = desc.act_len - 2;
	if (ret > len)
		ret = len;
	memcpy(buf, desc.data + 2, ret);
	return ret;
}

/*
 * ftdi_write_data - write to FTDI bulk-out endpoint
 * @intf: USB interface pointer
 * @buf:  pointer to data buffer
 * @len:  length in bytes of the data to send
 *
 * Return:
 * If successful, the number of bytes transferred. Otherwise a negative
 * error number.
 */
static int ftdi_write_data(struct usb_interface *intf,
			   const char *buf, size_t len)
{
	struct bulk_desc desc;
	int ret;

	desc.act_len = 0;
	desc.dir_out = true;
	desc.data = (char *)buf;
	desc.len = len;
	desc.timeout = FTDI_USB_WRITE_TIMEOUT;

	ret = ftdi_bulk_xfer(intf, &desc);
	if (ret < 0)
		return ret;

	return desc.act_len;
}

/*
 * ftdi_set_bitmode - configure bitbang mode
 * @intf: USB interface pointer
 * @bitmask: line configuration bitmask
 * @mode: bitbang mode to set
 *
 * Return:
 * If successful, 0. Otherwise a negative error number.
 */
static int ftdi_set_bitmode(struct usb_interface *intf, unsigned char bitmask,
			    unsigned char mode)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	struct ctrl_desc desc;
	int ret;

	desc.dir_out = true;
	desc.data = NULL;
	desc.request = FTDI_SIO_SET_BITMODE_REQUEST;
	desc.requesttype = USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT;
	desc.index = 1;
	desc.value = (mode << 8) | bitmask;
	desc.size = 0;
	desc.timeout = USB_CTRL_SET_TIMEOUT;

	ret = ftdi_ctrl_xfer(intf, &desc);
	if (ret < 0)
		return ret;

	switch (mode) {
	case BITMODE_BITBANG:
	case BITMODE_CBUS:
	case BITMODE_SYNCBB:
	case BITMODE_SYNCFF:
		priv->bitbang_enabled = 1;
		break;
	case BITMODE_MPSSE:
	case BITMODE_RESET:
	default:
		priv->bitbang_enabled = 0;
		break;
	}

	return 0;
}

/*
 * ftdi_set_clock_divisor - set clock divisor
 * @intf: USB interface pointer
 * @clk_freq: clock frequency (Hz)
 *
 * Return:
 * If successful, 0. Otherwise a negative error number.
 */
static int ftdi_set_clock_divisor (struct usb_interface *intf, unsigned long clk_freq)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	unsigned char clk_div[4];
	unsigned long *p_clk_div = (unsigned long *)clk_div;
	unsigned long master_clk_freq = 60000000; /* 60 MHz */
	unsigned long output_clk_freq = 30000000; /* 30 MHz */
	bool divide_by_5 = false;
	int ret;

	*p_clk_div = ((master_clk_freq >> 1) / clk_freq) - 1;
	if (*p_clk_div > 0xffff) {
		*p_clk_div /= 5;
		divide_by_5 = true;
		master_clk_freq = 12000000; /* 12 MHz */
	}

	output_clk_freq = (master_clk_freq / 2) / (*p_clk_div + 1);

	dev_dbg(&intf->dev, "clock_divisor: divide_by_5=%s div=%lu master=%lu output=%lu",
				divide_by_5 ? "on" : "off", *p_clk_div, master_clk_freq, output_clk_freq);

	if (output_clk_freq != clk_freq) {
		dev_info(&intf->dev, "clock frequency: %lu -> %lu\n",
			   		clk_freq, output_clk_freq);
	}

	mutex_lock(&priv->ops_mutex);

	if (divide_by_5)
		priv->tx_buf[0] = EN_DIV_5;
	else
		priv->tx_buf[0] = DIS_DIV_5;
	priv->tx_buf[1] = DIS_ADAPTIVE;
	priv->tx_buf[2] = DIS_3_PHASE;

	ret = ftdi_write_data(intf, priv->tx_buf, 3);
	if (ret < 0)
		goto err;

	udelay(1);

	priv->tx_buf[0] = TCK_DIVISOR;
	priv->tx_buf[1] = clk_div[0];
	priv->tx_buf[2] = clk_div[1];

	ret = ftdi_write_data(intf, priv->tx_buf, 3);
	if (ret < 0)
		goto err;

	ret = 0;

err:
	mutex_unlock(&priv->ops_mutex);

	return ret;
}

/*
 * ftdi_set_latency_timer - set latency timer value
 * @intf: USB interface pointer
 * @latency: latency timer value
 *
 * Return:
 * If successful, 0. Otherwise a negative error number.
 */
static int ftdi_set_latency_timer(struct usb_interface *intf, unsigned char latency)
{
	struct ctrl_desc desc;
	int ret;

	dev_dbg(&intf->dev, "%s: setting latency timer = %i\n", __func__, latency);

	desc.dir_out = true;
	desc.data = NULL;
	desc.request = FTDI_SIO_SET_LATENCY_TIMER_REQUEST,
	desc.requesttype = USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT;
	desc.index = 1;
	desc.value = latency;
	desc.size = 0;
	desc.timeout = USB_CTRL_SET_TIMEOUT;

	ret = ftdi_ctrl_xfer(intf, &desc);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * ftdi_get_latency_timer - get latency timer value
 * @intf: USB interface pointer
 * @latency: latency timer value pointer
 *
 * Return:
 * If successful, 0. Otherwise a negative error number.
 */
static int ftdi_get_latency_timer(struct usb_interface *intf, unsigned char *latency)
{
	struct ctrl_desc desc;
	unsigned char *buf;
	int ret;

	buf = kmalloc(1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	desc.dir_out = false;
	desc.request = FTDI_SIO_GET_LATENCY_TIMER_REQUEST,
	desc.requesttype = USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN;
	desc.value = 0;
	desc.index = 1;
	desc.size = 1;
	desc.data = buf;
	desc.timeout = USB_CTRL_SET_TIMEOUT;

	ret = ftdi_ctrl_xfer(intf, &desc);
	if (ret < 0)
		return ret;
	else if (ret != 1)
		return -EIO;

	*latency = *buf;
	kfree(buf);

	return 0;
}

/*
 * MPSSE CS and GPIO-L/-H support
 */
#define SET_BITS_LOW	0x80
#define GET_BITS_LOW	0x81
#define SET_BITS_HIGH	0x82
#define GET_BITS_HIGH	0x83

static int ftdi_mpsse_get_port_pins(struct ft232h_intf_priv *priv, bool low)
{
	struct device *dev = &priv->intf->dev;
	int ret, tout = 10;
	u8 rxbuf[4];

	if (low)
		priv->tx_buf[0] = GET_BITS_LOW;
	else
		priv->tx_buf[0] = GET_BITS_HIGH;

	ret = ftdi_write_data(priv->intf, priv->tx_buf, 1);
	if (ret < 0) {
		dev_dbg_ratelimited(dev, "Writing port pins cmd failed: %d\n",
				    ret);
		return ret;
	}

	rxbuf[0] = 0;
	do {
		/* usleep_range(5000, 5200); */
		ret = ftdi_read_data(priv->intf, rxbuf, 1);
		tout--;
		if (!tout) {
			dev_err(dev, "Timeout when getting port pins\n");
			return -ETIMEDOUT;
		}
	} while (ret == 0);

	if (ret < 0)
		return ret;

	if (ret != 1)
		return -EINVAL;

	if (low)
		priv->gpiol_mask = rxbuf[0];
	else
		priv->gpioh_mask = rxbuf[0];

	return 0;
}

static int ftdi_mpsse_set_port_pins(struct ft232h_intf_priv *priv, bool low)
{
	struct device *dev = &priv->intf->dev;
	int ret;

	if (low) {
		priv->tx_buf[0] = SET_BITS_LOW;
		priv->tx_buf[1] = priv->gpiol_mask;
		priv->tx_buf[2] = priv->gpiol_dir;
	} else {
		priv->tx_buf[0] = SET_BITS_HIGH;
		priv->tx_buf[1] = priv->gpioh_mask;
		priv->tx_buf[2] = priv->gpioh_dir;
	}

	ret = ftdi_write_data(priv->intf, priv->tx_buf, 3);
	if (ret < 0) {
		dev_dbg_ratelimited(dev, "Failed to set GPIO pins: %d\n",
				    ret);
		return ret;
	}

	return 0;
}

static int ftdi_mpsse_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	struct ft232h_intf_priv *priv = gpiochip_get_data(chip);
	int ret, val;
	bool low;

	mutex_lock(&priv->io_mutex);
	if (!priv->intf) {
		mutex_unlock(&priv->io_mutex);
		return -ENODEV;
	}
	mutex_unlock(&priv->io_mutex);

	dev_dbg(chip->parent, "%s: offset %d\n", __func__, offset);

	low = offset < 5;

	mutex_lock(&priv->ops_mutex);

	ret = ftdi_mpsse_get_port_pins(priv, low);
	if (ret < 0) {
		mutex_unlock(&priv->ops_mutex);
		return ret;
	}

	if (low)
		val = priv->gpiol_mask & (BIT(offset) << 3);
	else
		val = priv->gpioh_mask & BIT(offset - 5);

	mutex_unlock(&priv->ops_mutex);

	return !!val;
}

static void ftdi_mpsse_gpio_set(struct gpio_chip *chip, unsigned int offset,
				int value)
{
	struct ft232h_intf_priv *priv = gpiochip_get_data(chip);
	bool low;

	mutex_lock(&priv->io_mutex);
	if (!priv->intf) {
		mutex_unlock(&priv->io_mutex);
		return;
	}
	mutex_unlock(&priv->io_mutex);

	dev_dbg(chip->parent, "%s: offset %d, val %d\n",
		__func__, offset, value);

	mutex_lock(&priv->ops_mutex);

	if (offset < 4) {
		low = true;
		if (value)
			priv->gpiol_mask |= (BIT(offset) << 4);
		else
			priv->gpiol_mask &= ~(BIT(offset) << 4);
	} else {
		low = false;
		if (value)
			priv->gpioh_mask |= BIT(offset - 4);
		else
			priv->gpioh_mask &= ~BIT(offset - 4);
	}

	ftdi_mpsse_set_port_pins(priv, low);

	mutex_unlock(&priv->ops_mutex);
}

static int ftdi_mpsse_gpio_direction_input(struct gpio_chip *chip,
					   unsigned int offset)
{
	struct ft232h_intf_priv *priv = gpiochip_get_data(chip);
	bool low;
	int ret;

	mutex_lock(&priv->io_mutex);
	if (!priv->intf) {
		mutex_unlock(&priv->io_mutex);
		return -ENODEV;
	}
	mutex_unlock(&priv->io_mutex);

	dev_dbg(chip->parent, "%s: offset %d\n", __func__, offset);

	mutex_lock(&priv->ops_mutex);

	if (offset < 4) {
		low = true;
		priv->gpiol_dir &= ~(BIT(offset) << 4);
	} else {
		low = false;
		priv->gpioh_dir &= ~BIT(offset - 4);
	}

	ret = ftdi_mpsse_set_port_pins(priv, low);

	mutex_unlock(&priv->ops_mutex);

	return ret;
}

static int ftdi_mpsse_gpio_direction_output(struct gpio_chip *chip,
					    unsigned int offset, int value)
{
	struct ft232h_intf_priv *priv = gpiochip_get_data(chip);
	bool low;
	int ret;

	mutex_lock(&priv->io_mutex);
	if (!priv->intf) {
		mutex_unlock(&priv->io_mutex);
		return -ENODEV;
	}
	mutex_unlock(&priv->io_mutex);

	dev_dbg(chip->parent, "%s: offset %d, val %d\n",
		__func__, offset, value);

	mutex_lock(&priv->ops_mutex);

	if (offset < 4) {
		low = true;
		priv->gpiol_dir |= BIT(offset) << 4;

		if (value)
			priv->gpiol_mask |= BIT(offset) << 4;
		else
			priv->gpiol_mask &= ~(BIT(offset) << 4);
	} else {
		low = false;
		priv->gpioh_dir |= BIT(offset - 4);

		if (value)
			priv->gpioh_mask |= BIT(offset - 4);
		else
			priv->gpioh_mask &= ~BIT(offset - 4);
	}

	ret = ftdi_mpsse_set_port_pins(priv, low);

	mutex_unlock(&priv->ops_mutex);

	return ret;
}

static int ftdi_mpsse_init_pins(struct usb_interface *intf, bool low,
				u8 bits, u8 direction)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	int ret;

	mutex_lock(&priv->ops_mutex);

	if (low) {
		priv->gpiol_mask = bits;
		priv->gpiol_dir = direction;
	} else {
		priv->gpioh_mask = bits;
		priv->gpioh_dir = direction;
	}

	ret = ftdi_mpsse_set_port_pins(priv, low);

	mutex_unlock(&priv->ops_mutex);

	return ret;
}

static int ftdi_mpsse_cfg_bus_pins(struct usb_interface *intf,
				   u8 dir_bits, u8 value_bits)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	int ret;

	mutex_lock(&priv->ops_mutex);

	priv->gpiol_dir &= ~7;
	priv->gpiol_dir |= (dir_bits & 7);

	priv->gpiol_mask &= ~7;
	priv->gpiol_mask |= (value_bits & 7);

	ret = ftdi_mpsse_set_port_pins(priv, true);

	mutex_unlock(&priv->ops_mutex);

	return ret;
}

static int ftdi_mpsse_set_cs_pin(struct usb_interface *intf, int level)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	int ret;

	mutex_lock(&priv->ops_mutex);

	priv->gpiol_dir |= MPSSE_CS;

	switch (level) {
		case MPSSE_GPIO_LOW:
			priv->gpiol_mask &= ~MPSSE_CS;
			break;

		case MPSSE_GPIO_HIGH:
			priv->gpiol_mask |= MPSSE_CS;
			break;

		default:
			mutex_unlock(&priv->ops_mutex);
			return -EINVAL;
	}

	ret = ftdi_mpsse_set_port_pins(priv, true);

	mutex_unlock(&priv->ops_mutex);

	return ret;
}

static const struct ft232h_intf_ops ft232h_intf_ops = {
	.lock = ftdi_lock,
	.unlock = ftdi_unlock,
	.ctrl_xfer = ftdi_ctrl_xfer,
	.bulk_xfer = ftdi_bulk_xfer,
	.read_data = ftdi_read_data,
	.write_data = ftdi_write_data,
	.set_bitmode = ftdi_set_bitmode,
	.set_clock_divisor = ftdi_set_clock_divisor,
	.set_latency_timer = ftdi_set_latency_timer,
	.get_latency_timer = ftdi_get_latency_timer,
	.init_pins = ftdi_mpsse_init_pins,
	.cfg_bus_pins = ftdi_mpsse_cfg_bus_pins,
	.set_cs_pin = ftdi_mpsse_set_cs_pin,
};

static const struct mpsse_spi_platform_data ft232h_mpsse_spi_plat_data = {
	.ops		= &ft232h_intf_ops,
};

static int ftdi_mpsse_gpio_probe(struct usb_interface *intf)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	struct device *parent = &intf->dev;
	struct gpiod_lookup_table *lookup;
	size_t lookup_size;
	char **names, *label;
	int i, ret;

	label = devm_kasprintf(parent, GFP_KERNEL, "ftdi-mpsse-gpio.%d", priv->id);
	if (!label)
		return -ENOMEM;

	priv->mpsse_gpio.label = label;
	priv->mpsse_gpio.parent = parent;
	priv->mpsse_gpio.owner = THIS_MODULE;
	priv->mpsse_gpio.base = (param_gpio_base >= 0) ? param_gpio_base : -1;
	priv->mpsse_gpio.ngpio = FTDI_MPSSE_GPIOS;
	priv->mpsse_gpio.can_sleep = true;
	priv->mpsse_gpio.set = ftdi_mpsse_gpio_set;
	priv->mpsse_gpio.get = ftdi_mpsse_gpio_get;
	priv->mpsse_gpio.direction_input = ftdi_mpsse_gpio_direction_input;
	priv->mpsse_gpio.direction_output = ftdi_mpsse_gpio_direction_output;

	names = devm_kcalloc(parent, priv->mpsse_gpio.ngpio, sizeof(char *),
			     GFP_KERNEL);
	if (!names)
		return -ENOMEM;

	for (i = 0; i < priv->mpsse_gpio.ngpio; i++) {
		int offs;

		offs = i < 4 ? 0 : 4;
		names[i] = devm_kasprintf(parent, GFP_KERNEL,
					  "mpsse.%d-GPIO%c%d", priv->id,
					  i < 4 ? 'L' : 'H', i - offs);
		if (!names[i])
			return -ENOMEM;
	}

	priv->mpsse_gpio.names = (const char *const *)names;

	ret = devm_gpiochip_add_data(parent, &priv->mpsse_gpio, priv);
	if (ret < 0) {
		dev_err(parent, "Failed to add MPSSE GPIO chip: %d\n", ret);
		return ret;
	}

	dev_info(parent, "gpiochip: label=%s base=%d ngpio=%d\n", 
			priv->mpsse_gpio.label, priv->mpsse_gpio.base, priv->mpsse_gpio.ngpio);

	lookup_size = sizeof(struct gpiod_lookup_table);
   	lookup_size += (priv->mpsse_gpio.ngpio + 1) * sizeof(struct gpiod_lookup);

	lookup = devm_kzalloc(parent, lookup_size, GFP_KERNEL);
	if (!lookup)
		return -ENOMEM;

	lookup->dev_id = devm_kasprintf(parent, GFP_KERNEL, "%s.%d",
						priv->spi_pdev->name, priv->spi_pdev->id);
	if (!lookup->dev_id)
		return -ENOMEM;

	for (i = 0; i < priv->mpsse_gpio.ngpio ; i++) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,7,19)
		lookup->table[i].chip_label = priv->mpsse_gpio.label;
#else
		lookup->table[i].key = priv->mpsse_gpio.label;
#endif
		lookup->table[i].chip_hwnum = i;
		if (i < 4) {
			lookup->table[i].idx = i;
			lookup->table[i].con_id = "gpiol";
		} else {
			lookup->table[i].idx = i - 4;
			lookup->table[i].con_id = "gpioh";
		}

		lookup->table[i].flags = GPIO_ACTIVE_LOW;
	}

	priv->lookup_gpios = lookup;
	gpiod_add_lookup_table(priv->lookup_gpios);

	return 0;
}

static void ftdi_mpsse_gpio_remove(struct usb_interface *intf)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);

	if (priv->lookup_gpios)
		gpiod_remove_lookup_table(priv->lookup_gpios);
}

static int ftdi_mpsse_spi_probe(struct usb_interface *intf)
{
	const struct mpsse_spi_platform_data *plat_data = &ft232h_mpsse_spi_plat_data;
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	struct device *parent = &intf->dev;
	struct platform_device *pdev;
	int ret;

	ret = ftdi_set_bitmode(intf, 0x00, BITMODE_RESET);
	if (ret < 0)
		return ret;

	ret = ftdi_set_bitmode(intf, 0x00, BITMODE_MPSSE);
	if (ret < 0)
		return ret;

	pdev = platform_device_alloc("ftdi-mpsse-spi", 0);
	if (!pdev)
		return -ENOMEM;

	pdev->dev.parent = parent;
	pdev->dev.fwnode = NULL;
	pdev->id = priv->id;

	ret = platform_device_add_data(pdev, plat_data, sizeof(*plat_data));
	if (ret < 0)
		goto err;

	ret = platform_device_add(pdev);
	if (ret < 0)
		goto err;

	dev_dbg(&pdev->dev, "%s done\n", __func__);
	priv->spi_pdev = pdev;
	return 0;

err:
	dev_err(parent, "%s: Can't create MPSSE SPI device %d\n", __func__, ret);
	platform_device_put(pdev);
	return ret;
}

static int ftdi_mpsse_spi_remove(struct usb_interface *intf)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);
	struct device *dev = &intf->dev;

	dev_dbg(dev, "%s: spi pdev %p\n", __func__, priv->spi_pdev);
	platform_device_unregister(priv->spi_pdev);
	return 0;
}

int ft232h_intf_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct ft232h_intf_priv *priv;
	struct device *dev = &intf->dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	unsigned int i;
	int ret = 0;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	iface_desc = intf->cur_altsetting;

	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (usb_endpoint_is_bulk_out(endpoint))
			priv->bulk_out = endpoint->bEndpointAddress;

		if (usb_endpoint_is_bulk_in(endpoint)) {
			priv->bulk_in = endpoint->bEndpointAddress;
			priv->bulk_in_sz = usb_endpoint_maxp(endpoint);
		}
	}

	priv->usb_dev_id = id;
	priv->index = 1;
	priv->intf = intf;

	mutex_init(&priv->io_mutex);
	mutex_init(&priv->ops_mutex);
	usb_set_intfdata(intf, priv);

	priv->bulk_in_buf = devm_kmalloc(dev, priv->bulk_in_sz, GFP_KERNEL);
	if (!priv->bulk_in_buf)
		return -ENOMEM;

	priv->udev = usb_get_dev(interface_to_usbdev(intf));

	priv->id = ida_simple_get(&ftdi_devid_ida, 0, 0, GFP_KERNEL);
	if (priv->id < 0)
		return priv->id;

	ret = ftdi_mpsse_spi_probe(intf);
	if (ret < 0)
		goto err;

	ret = ftdi_mpsse_gpio_probe(intf);
	if (ret < 0)
		goto err;

	return 0;

err:
	if (priv->spi_pdev)
		ftdi_mpsse_spi_remove(intf);
	ida_simple_remove(&ftdi_devid_ida, priv->id);
	return ret;
}

void ft232h_intf_disconnect(struct usb_interface *intf)
{
	struct ft232h_intf_priv *priv = usb_get_intfdata(intf);

	ftdi_mpsse_gpio_remove(intf);
	ftdi_mpsse_spi_remove(intf);

	mutex_lock(&priv->io_mutex);
	priv->intf = NULL;
	usb_set_intfdata(intf, NULL);
	mutex_unlock(&priv->io_mutex);

	usb_put_dev(priv->udev);
	ida_simple_remove(&ftdi_devid_ida, priv->id);
}

