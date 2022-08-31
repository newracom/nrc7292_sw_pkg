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

#ifndef _NRC_BUILD_CONFIG_H_
#define _NRC_BUILD_CONFIG_H_

#include <linux/version.h>

#define NRC_BUILD_USE_HWSCAN
/* #define CONFIG_NRC_HIF_PRINT_BEACON */
/* #define CONFIG_NRC_HIF_PRINT_RX_AUTH */
/* #define CONFIG_NRC_HIF_PRINT_RX_DATA */
/* #define CONFIG_NRC_HIF_PRINT_TX_MGMT */
/* #define CONFIG_NRC_HIF_PRINT_TX_DATA */
/* #define CONFIG_NRC_HIF_PRINT_FLOW_CONTROL */
/* #define CONFIG_NRC_HIF_DUMP_S1G_RXINFO */

/*#define NRC_TEST_SUPPRESS_STA_KEEEP_ALIVE*/

/*
 * README This is a temporary feature.
 * Use only NRC7392
 */
#define CONFIG_CHECK_READY

/*
 * README This is a temporary feature.
 * Use only 11n certification
 */
/* #define CONFIG_TRX_BACKOFF */

/*
 * README This is a temporary feature.
 * To change the transmission order of txq in the nrc driver.
 */
/* #define CONFIG_TXQ_ORDER_CHANGE_NRC_DRV */


/*
 * These depend on kernel version.
 */
/*
 * #define NRC_TARGET_KERNEL_VERSION KERNEL_VERSION(4, 4, 1)
 */
#define NRC_TARGET_KERNEL_VERSION LINUX_VERSION_CODE

#if KERNEL_VERSION(4, 10, 0) <= NRC_TARGET_KERNEL_VERSION
#define GENL_ID_GENERATE 0
#endif
#if KERNEL_VERSION(4, 8, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_USE_CFG80211_SCAN_INFO
#endif
#if KERNEL_VERSION(4, 7, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_USE_NEW_BAND_ENUM
#endif
#if KERNEL_VERSION(4, 6, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_USE_IEEE80211_AMPDU_PARAMS
#endif
#if KERNEL_VERSION(4, 4, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_NEW_AMPDU_ACTION
#endif
#if KERNEL_VERSION(4, 3, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_USE_IFF_NO_QUEUE_FLAG
#endif
#if KERNEL_VERSION(4, 2, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_LONG_HWFLAG
#endif
#if KERNEL_VERSION(4, 1, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_USE_TXQ
#define CONFIG_USE_IFACE_ITER
#define CONFIG_USE_CHANNEL_CONTEXT
#endif
#if KERNEL_VERSION(4, 0, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_CCMP_256
#define CONFIG_SUPPORT_KEY_RESERVE_TAILROOM
#endif
#if KERNEL_VERSION(3, 19, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_IFTYPE_OCB
#define CONFIG_SUPPORT_HWDEV_NAME
#endif
#if KERNEL_VERSION(3, 17, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_USE_NEW_SCAN_REQ
#define CONFIG_USE_NEW_ALLOC_NETDEV
#endif
#if KERNEL_VERSION(3, 16, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_BEACON_TEMPLATE
#define CONFIG_SUPPORT_NEW_FLUSH
#endif
#if KERNEL_VERSION(3, 15, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_BSS_MAX_IDLE_PERIOD
#endif
#if KERNEL_VERSION(3, 0, 37) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
#define CONFIG_SUPPORT_TX_CONTROL
#define CONFIG_SUPPORT_CHANNEL_INFO
#define CONFIG_SUPPORT_ITERATE_INTERFACE
#define CONFIG_SUPPORT_GENLMSG_DEFAULT
#define CONFIG_SUPPORT_SPI_SYNC_TRANSFER
#define CONFIG_NEW_REG_NOTIFIER
#define CONFIG_USE_HW_QUEUE
#define CONFIG_USE_MONITOR_VIF
#define CONFIG_SUPPORT_NEW_NETLINK
#define CONFIG_SUPPORT_PS
#define CONFIG_SUPPORT_NEW_MAC_TX
#define CONFIG_SUPPORT_P2P
#define CONFIG_SUPPORT_BD
/* To use JPPC board data file & FW */
#undef CONFIG_SUPPORT_JPPC
#endif
#if KERNEL_VERSION(3, 0, 0) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_TX_FRAMES_PENDING
#endif
#if KERNEL_VERSION(2, 6, 30) <= NRC_TARGET_KERNEL_VERSION
#define CONFIG_SUPPORT_THREADED_IRQ
#endif

#if KERNEL_VERSION(4, 10, 0) > NRC_TARGET_KERNEL_VERSION
/* WDS was deprecated and re-enabled with CONFIG_WIRELESS_WDS since 4.10 */
#define CONFIG_WIRELESS_WDS
#endif

#if KERNEL_VERSION(4, 2, 0) > NRC_TARGET_KERNEL_VERSION
#define ieee80211_hw_check(hw, flg) (hw->flags & IEEE80211_HW_##flg)
#endif

#if defined(CONFIG_WIRELESS_WDS) || defined(CONFIG_USE_CHANNEL_CONTEXT)
/* Cannot use CONFIG_USE_CHANNEL_CONTEXT) */
#undef CONFIG_USE_CHANNEL_CONTEXT
#endif

/* for backports */
/*
 * #ifdef CONFIG_USE_IFF_NO_QUEUE_FLAG
 * #undef CONFIG_USE_IFF_NO_QUEUE_FLAG
 * #endif
 */

/* Check tx queue total data size, not just queue length */
#define CONFIG_CHECK_DATA_SIZE

#endif
