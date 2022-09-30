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

#ifndef _NRC_VENDOR_H_
#define _NRC_VENDOR_H_

#define OUI_IEEE_REGISTRATION_AUTHORITY		0xFCFFAA

/**
 * A subcmd to remove the vendor specific IE which was injected before.
 * ex>$sudo iw dev wlan0 vendor send 0xfcffaa 0xde 0x2
 *   => this cmd will remove the injected vendor specific IE
 *      of which the subcmd was 0x2.
 */
#define NRC_SUBCMD_RM_VENDOR_IE				0xDE

/*
 * GPIO pin number on Raspberry Pi.
 * This is used to wake up the target in deep-sleep.
 * [direction]: OUTPUT
 */
#define RPI_GPIO_FOR_PS						(20)

/*
 * GPIO pin number on the target device.
 * This is used to designate the GPIO pin on target
 * which will read signal from RPI_GPIO_FOR_RS for waking up target.
 * [direction]: INPUT
 */
#define TARGET_GPIO_FOR_WAKEUP				(11)

/*
 * GPIO pin number on the target device.
 * This is used to designate the GPIO pin on target
 * which will raise signal to wake up host system.
 * [direction]: OUTPUT
 */
#define TARGET_GPIO_FOR_WAKEUP_HOST			(10)

#define RPI_GPIO_FOR_RST					(4)

enum nrc_vendor_event {
	NRC_SUBCMD_ANNOUNCE1 = 0,
	NRC_SUBCMD_ANNOUNCE2,
	NRC_SUBCMD_ANNOUNCE3,
	NRC_SUBCMD_ANNOUNCE4,
	NRC_SUBCMD_ANNOUNCE5,
	NRC_SUBCMD_WOWLAN_PATTERN,
	NUM_VENDOR_EVENT,
	MAX_VENDOR_EVENT = NUM_VENDOR_EVENT - 1
};

enum nrc_vendor_attributes {
	NRC_VENDOR_ATTR_DATA = 0,
	NUM_VENDOR_ATTR,
	MAX_VENDOR_ATTR = NUM_VENDOR_ATTR - 1
};

#endif /* _NRC_VENDOR_H_ */
