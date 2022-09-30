/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Common definitions for FTDI FT232H interface device
 *
 * Copyright (C) 2017 - 2018 DENX Software Engineering
 * Anatolij Gustschin <agust@denx.de>
 *
 * Note) This file is modified by Newracom, Inc. <www.newwracom.com>
 *
 */

#ifndef __LINUX_FT232H_INTF_H
#define __LINUX_FT232H_INTF_H

/* Used FTDI USB Requests */
#define FTDI_SIO_RESET_REQUEST				0x00
#define FTDI_SIO_SET_BAUDRATE_REQUEST		0x03
#define FTDI_SIO_SET_LATENCY_TIMER_REQUEST	0x09
#define FTDI_SIO_GET_LATENCY_TIMER_REQUEST	0x0A
#define FTDI_SIO_SET_BITMODE_REQUEST		0x0B
#define FTDI_SIO_READ_PINS_REQUEST			0x0C
#define FTDI_SIO_READ_EEPROM_REQUEST		0x90

/* MPSSE Commands */
#define TX_BYTES_RE_MSB		0x10 /* tx on ve clk (rising edge) */
#define TX_BYTES_FE_MSB		0x11 /* tx on -ve clk (falling edge) */
#define RX_BYTES_RE_MSB		0x20
#define RX_BYTES_FE_MSB		0x24
#define TXF_RXR_BYTES_MSB	0x31 /* tx on -ve clk, rx on ve */
#define TXR_RXF_BYTES_MSB	0x34 /* tx on ve clk, rx on -ve */

#define TX_BYTES_RE_LSB		0x18 /* tx on ve clk */
#define TX_BYTES_FE_LSB		0x19 /* tx on -ve clk */
#define RX_BYTES_RE_LSB		0x28
#define RX_BYTES_FE_LSB		0x2C
#define TXF_RXR_BYTES_LSB	0x39 /* tx on -ve clk, rx on ve */
#define TXR_RXF_BYTES_LSB	0x3C /* tx on ve clk, rx on -ve */

#define LOOPBACK_ON			0x84
#define LOOPBACK_OFF		0x85
#define TCK_DIVISOR			0x86
#define DIS_DIV_5			0x8A
#define EN_DIV_5			0x8B
#define EN_3_PHASE			0x8C
#define DIS_3_PHASE			0x8D
#define DIS_ADAPTIVE		0x97

#define FTDI_USB_READ_TIMEOUT	5000
#define FTDI_USB_WRITE_TIMEOUT	5000

/* Total number of MPSSE GPIOs: 4x GPIOL, 8x GPIOH */
#define FTDI_MPSSE_GPIOS	12

/* MPSSE bitbang modes (copied from libftdi) */
enum ftdi_mpsse_mode {
	BITMODE_RESET	= 0x00,	/* switch off bitbang mode */
	BITMODE_BITBANG	= 0x01,	/* asynchronous bitbang mode */
	BITMODE_MPSSE	= 0x02,	/* MPSSE mode, on 2232x chips */
	BITMODE_SYNCBB	= 0x04,	/* synchronous bitbang mode  */
	BITMODE_MCU		= 0x08,	/* MCU Host Bus Emulation mode */
							/* CPU-style fifo mode gets set via EEPROM */
	BITMODE_OPTO	= 0x10,	/* Fast Opto-Isolated Serial Interface Mode */
	BITMODE_CBUS	= 0x20,	/* Bitbang on CBUS pins, EEPROM config needed */
	BITMODE_SYNCFF	= 0x40,	/* Single Channel Synchronous FIFO mode */
	BITMODE_FT1284	= 0x80,	/* FT1284 mode, available on 232H chips */
};

/* MPSSE SPI pins */
enum ftdi_mpsse_pins {
	MPSSE_SK = BIT(0),
	MPSSE_DO = BIT(1),
	MPSSE_DI = BIT(2),
	MPSSE_CS = BIT(3),

	MPSSE_GPIOL0 = BIT(4),
	MPSSE_GPIOL1 = BIT(5),
	MPSSE_GPIOL2 = BIT(6),
	MPSSE_GPIOL3 = BIT(7),

	MPSSE_GPIOH0 = BIT(0),
	MPSSE_GPIOH1 = BIT(1),
	MPSSE_GPIOH2 = BIT(2),
	MPSSE_GPIOH3 = BIT(3),
	MPSSE_GPIOH4 = BIT(4),
	MPSSE_GPIOH5 = BIT(5),
	MPSSE_GPIOH6 = BIT(6),
	MPSSE_GPIOH7 = BIT(7),
};

/* MPSSE GPIO direction */
enum ftdi_gpio_dir {
	MPSSE_GPIO_IN = 0,
	MPSSE_GPIO_OUT = 1
};

/* MPSSE GPIO level */
enum ftdi_gpio_level {
	MPSSE_GPIO_LOW = 0,
	MPSSE_GPIO_HIGH = 1
};

struct ctrl_desc {
	unsigned int dir_out;
	u8 request;
	u8 requesttype;
	u16 value;
	u16 index;
	u16 size;
	void *data;
	int timeout;
};

struct bulk_desc {
	unsigned int dir_out;
	void *data;
	int len;
	int act_len;
	int timeout;
};

/*
 * struct ft232h_intf_priv - FT232H interface driver data
 */
struct ft232h_intf_priv {
	struct usb_interface	*intf;
	struct usb_device	*udev;
	struct mutex		io_mutex; /* sync I/O with disconnect */
	struct mutex		ops_mutex;
	int			bitbang_enabled;
	int			id;
	int			index;
	u8			bulk_in;
	u8			bulk_out;
	size_t			bulk_in_sz;
	void			*bulk_in_buf;

	const struct usb_device_id	*usb_dev_id;
	struct platform_device		*spi_pdev;
	struct gpiod_lookup_table	*lookup_gpios;

	struct gpio_chip	mpsse_gpio;
	u8			gpiol_mask;
	u8			gpioh_mask;
	u8			gpiol_dir;
	u8			gpioh_dir;
	u8			tx_buf[4];
};

/*
 * struct ft232h_intf_ops - FT232H interface operations for upper drivers
 *
 * @lock: lock the interface for an operation sequence. Used when multiple
 *	  command and/or data operations must be executed in a specific order
 *	  (when other intermediate command/data transfers may not interfere)
 * @unlock: unlock the previously locked interface
 * @bulk_xfer: FTDI USB bulk transfer
 * @ctrl_xfer: FTDI USB control transfer
 * @read_data: read 'len' bytes from FTDI device to the given buffer
 * @write_data: write 'len' bytes from the given buffer to the FTDI device
 * @set_bitmode: configure FTDI bit mode
 * @set_clock_divisor: configure clock divisor
 * @set_latency_timer: set latency timer value
 * @get_latency_timer: get latency timer value
 * @init_pins: initialize GPIOL/GPIOH port pins in MPSSE mode
 * @cfg_bus_pins: configure MPSSE SPI bus pins
 * @set_cs_pin: set MPSSE SPI CS pin level
 *
 * Common FT232H interface USB xfer and device configuration operations used
 * in MPSSE-SPI drivers.
 */
struct ft232h_intf_ops {
	void (*lock)(struct usb_interface *intf);
	void (*unlock)(struct usb_interface *intf);
	int (*bulk_xfer)(struct usb_interface *intf, struct bulk_desc *desc);
	int (*ctrl_xfer)(struct usb_interface *intf, struct ctrl_desc *desc);
	int (*read_data)(struct usb_interface *intf, void *buf, size_t len);
	int (*write_data)(struct usb_interface *intf, const char *buf, size_t len);
	int (*set_bitmode)(struct usb_interface *intf, unsigned char bitmask, unsigned char mode);
	int (*set_clock_divisor)(struct usb_interface *intf, unsigned long clk_freq);
	int (*set_latency_timer)(struct usb_interface *intf, unsigned char latency);
	int (*get_latency_timer)(struct usb_interface *intf, unsigned char *latency);
	int (*init_pins)(struct usb_interface *intf, bool low, u8 bits, u8 direction);
	int (*cfg_bus_pins)(struct usb_interface *intf, u8 dir_bits, u8 value_bits);
	int (*set_cs_pin)(struct usb_interface *intf, int level);
};

/*
 * struct mpsse_spi_platform_data - MPSSE SPI bus platform data
 * @ops: USB interface operations used in MPSSE SPI controller driver
 *
 * MPSSE SPI bus specific platform data
 */
struct mpsse_spi_platform_data {
	const struct ft232h_intf_ops *ops;
};

#endif /* __LINUX_FT232H_INTF_H */
