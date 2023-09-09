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
 * iw vendor command can be worked and accepted which includes the same
 * OUI with this 'VENDOR_OUI'
 */
#define VENDOR_OUI							(OUI_IEEE_REGISTRATION_AUTHORITY)

/**
 * A subcmd to remove the vendor specific IE which was injected before.
 * ex>$sudo iw dev wlan0 vendor send 0xfcffaa 0xde 0x2
 *   => this cmd will remove the injected vendor specific IE
 *      of which the subcmd was 0x2.
 */
#define NRC_SUBCMD_RM_VENDOR_IE				0xDE

/**
 * GPIO pin number on Raspberry Pi.
 * This is used to wake up the target in deep-sleep.
 * [direction]: OUTPUT
 */
#define RPI_GPIO_FOR_PS						(20) //NRC7292 EVB

/**
 * GPIO pin number on the target device.
 * This is used to designate the GPIO pin on target
 * which will read signal from RPI_GPIO_FOR_RS for waking up target.
 * [direction]: INPUT
 */
#define TARGET_GPIO_FOR_WAKEUP				(11) //NRC7292 EVB

/**
 * GPIO pin number on the target device.
 * This is used to designate the GPIO pin on target
 * which will raise signal to wake up host system.
 * [direction]: OUTPUT
 */
#define TARGET_GPIO_FOR_WAKEUP_HOST			(10)

/**
 * GPIO configuration during deep sleep operation.
 * Each direction(input:0, output:1), out(low:0, high:1)
 * and pull-up(floating:0, pull-up:1) can be configured with
 * a bit-wise values.
 * For example, bit-0 corresponds to GPIO0 in target.
 */
#define TARGET_DEEP_SLEEP_GPIO_DIR_7292		(0x07FFFF7F) // NRC default: 07, 27-31 => input
#define TARGET_DEEP_SLEEP_GPIO_DIR_739X		(0xCFEF8D07) // NRC default: 03-07, 09, 12-14, 20, 28-29 => input
#define TARGET_DEEP_SLEEP_GPIO_OUT			(0x0)
#define TARGET_DEEP_SLEEP_GPIO_PULLUP		(0x0)

#define RPI_GPIO_FOR_RST					(4)

enum nrc_vendor_event {
	NRC_SUBCMD_ANNOUNCE1 = 0,
	NRC_SUBCMD_ANNOUNCE2,
	NRC_SUBCMD_ANNOUNCE3,
	NRC_SUBCMD_ANNOUNCE4,
	NRC_SUBCMD_ANNOUNCE5,
	NRC_SUBCMD_REMOTECMD,
	NRC_SUBCMD_WOWLAN_PATTERN,
	NRC_SUBCMD_BCAST_FOTA_INFO,
	NRC_SUBCMD_BCAST_FOTA_1,
	NRC_SUBCMD_BCAST_FOTA_2,
	NRC_SUBCMD_BCAST_FOTA_3,
	NRC_SUBCMD_BCAST_FOTA_4,
	NRC_SUBCMD_ANNOUNCE6,	// vendor ie in probe request (1).  // 12
	NRC_SUBCMD_ANNOUNCE7,	// vendor ie in probe request (2).
	NRC_SUBCMD_ANNOUNCE8,	// vendor ie in probe request (3).
	NRC_SUBCMD_ANNOUNCE9,	// vendor ie in probe request (4).
	NRC_SUBCMD_ANNOUNCE10,	// vendor ie in probe request (5).  // 16
	NRC_SUBCMD_ANNOUNCE11,	// vendor ie in probe response (1). // 17
	NRC_SUBCMD_ANNOUNCE12,	// vendor ie in probe response (2).
	NRC_SUBCMD_ANNOUNCE13,	// vendor ie in probe response (3).
	NRC_SUBCMD_ANNOUNCE14,	// vendor ie in probe response (4).
	NRC_SUBCMD_ANNOUNCE15,	// vendor ie in probe response (5). // 21
	NRC_SUBCMD_ANNOUNCE16,	// vendor ie in assoc request (1).  // 22
	NRC_SUBCMD_ANNOUNCE17,	// vendor ie in assoc request (2).
	NRC_SUBCMD_ANNOUNCE18,	// vendor ie in assoc request (3).
	NRC_SUBCMD_ANNOUNCE19,	// vendor ie in assoc request (4).
	NRC_SUBCMD_ANNOUNCE20,	// vendor ie in assoc request (5).  // 26
	NUM_VENDOR_EVENT,
	MAX_VENDOR_EVENT = NUM_VENDOR_EVENT - 1
};

enum nrc_vendor_attributes {
	NRC_VENDOR_ATTR_DATA = 0,
	NUM_VENDOR_ATTR,
	MAX_VENDOR_ATTR = NUM_VENDOR_ATTR - 1
};

struct remotecmd_params {
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	u8 subcmd;
};

#endif /* _NRC_VENDOR_H_ */
