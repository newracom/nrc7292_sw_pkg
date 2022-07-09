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
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include "nrc.h"
#include "nrc-hif.h"
#include "nrc-debug.h"
#include "nrc-mac80211.h"
#include "nrc-fw.h"
#include "nrc-netlink.h"
#include "nrc-stats.h"
#include "wim.h"
#include "nrc-recovery.h"
#include "nrc-vendor.h"
#if defined(CONFIG_SUPPORT_BD)
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "nrc-bd.h"
#endif /* defined(CONFIG_SUPPORT_BD) */

char *fw_name;
module_param(fw_name, charp, 0444);
MODULE_PARM_DESC(fw_name, "Firmware file name");

#if defined(CONFIG_SUPPORT_BD)
char *bd_name ="nrc7292_bd.dat";
module_param(bd_name, charp, 0600);
MODULE_PARM_DESC(bd_name, "Board Data file name");
#endif /* defined(CONFIG_SUPPORT_BD) */

/**
 * default port name
 */
#if defined(CONFIG_ARM)
#if defined(CONFIG_NRC_HIF_UART)
char *hifport = "/dev/ttyAMA0";
#else
char *hifport = "/dev/ttyUSB0";
#endif
#else
char *hifport = "/dev/ttyUSB0";
#endif
module_param(hifport, charp, 0600);
MODULE_PARM_DESC(hifport, "HIF port device name");

/**
 * default port speed
 */
#if defined(CONFIG_NRC_HIF_CSPI)
int hifspeed = (20*1000*1000);
#elif defined(CONFIG_NRC_HIF_SSP)
int hifspeed = (1300*1000);
#elif defined(CONFIG_NRC_HIF_UART)
int hifspeed = 115200;
#else
int hifspeed = 115200;
#endif
module_param(hifspeed, int, 0600);
MODULE_PARM_DESC(hifspeed, "HIF port speed");

int spi_bus_num;
module_param(spi_bus_num, int, 0600);
MODULE_PARM_DESC(spi_bus_num, "SPI controller bus number");

int spi_cs_num;
module_param(spi_cs_num, int, 0600);
MODULE_PARM_DESC(spi_cs_num, "SPI chip select number");

int spi_gpio_irq = 5;
module_param(spi_gpio_irq, int, 0600);
MODULE_PARM_DESC(spi_gpio_irq, "SPI gpio irq");

int spi_polling_interval = 0;
module_param(spi_polling_interval, int, 0600);
MODULE_PARM_DESC(spi_polling_interval, "SPI polling interval (msec)");

int spi_gdma_irq = 6;
module_param(spi_gdma_irq, int, 0600);
MODULE_PARM_DESC(spi_gdma_irq, "SPI gdma irq");

bool loopback;
module_param(loopback, bool, 0600);
MODULE_PARM_DESC(loopback, "HIF loopback");

int lb_count = 1;
module_param(lb_count, int, 0600);
MODULE_PARM_DESC(lb_count, "HIF loopback Buffer count");

int disable_cqm = 0;
module_param(disable_cqm, int, 0600);
MODULE_PARM_DESC(disable_cqm, "Disable CQM (0: enable, 1: disable)");

int listen_interval = 100;
module_param(listen_interval, int, 0600);
MODULE_PARM_DESC(listen_interval, "Listen Interval");

/**
 * bss_max_idle (in unit of 1000TUs(1024ms)
 */
int bss_max_idle;
module_param(bss_max_idle, int, 0600);
MODULE_PARM_DESC(bss_max_idle, "BSS Max Idle");

/**
 * bss_max_idle_usf_format
 */
bool bss_max_idle_usf_format=true;
module_param(bss_max_idle_usf_format, bool, 0600);
MODULE_PARM_DESC(bss_max_idle_usf_format, "BSS Max Idle specified in units of usf");

/**
 * default enable_short_bi
 */
bool enable_short_bi = 1;
module_param(enable_short_bi, bool, 0600);
MODULE_PARM_DESC(enable_short_bi, "Enable Short Beacon Interval");

/**
 * enable/disable the legacy ack mode
 */
bool enable_legacy_ack = false;
module_param(enable_legacy_ack, bool, 0600);
MODULE_PARM_DESC(enable_legacy_ack, "Enable Legacy ACK mode");

bool enable_monitor;
module_param(enable_monitor, bool, 0600);
MODULE_PARM_DESC(enable_monitor, "Enable Monitor");

/**
 * milisecond unit
 * to resolve QoS null frame variable-interval issue
 */
int bss_max_idle_offset;
module_param(bss_max_idle_offset, int, 0600);
MODULE_PARM_DESC(bss_max_idle_offset, "BSS Max Idle Offset");

/**
 * override  mac-address on nrc driver, follow below format
 * xx:xx:xx:xx:xx:xx
 */
static char *macaddr = ":";
module_param(macaddr, charp, 0);
MODULE_PARM_DESC(macaddr, "MAC Address");

/**
 * enable/disable the power save mode by default
 */
int power_save = NRC_PS_NONE;;
module_param(power_save, int, 0600);
MODULE_PARM_DESC(power_save, "power save");

/**
 * deepsleep duration of non-TIM mode power save
 */
int sleep_duration[2] = {0,};
module_param_array(sleep_duration, int, NULL, 0600);
MODULE_PARM_DESC(sleep_duration, "deepsleep duration of non-TIM mode power save");

/**
 * wlantest mode
 */
bool wlantest = false;
module_param(wlantest, bool, 0600);
MODULE_PARM_DESC(wlantest, "wlantest");

/**
 * Set NDP Probe Request mode
 */
bool ndp_preq = false;
module_param(ndp_preq, bool, 0600);
MODULE_PARM_DESC(ndp_preq, "Enable NDP Probe Request");

/**
 * Set 1M NDP ACK
 */
bool ndp_ack_1m = false;
module_param(ndp_ack_1m, bool, 0600);
MODULE_PARM_DESC(ndp_ack_1m, "Enable 1M NDP ACK");

/**
 * Enable HSPI init
 */
bool enable_hspi_init = false;
module_param(enable_hspi_init, bool, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(enable_hspi_init, "Enable HSPI Initialization");

/**
 * Enable handling null func (ps-poll) on mac80211
 */
bool nullfunc_enable = false;
module_param(nullfunc_enable, bool, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(nullfunc_enable, "Enable null func on mac80211");

/**
 * Set up automatic TX BA session on connection and QoS data Tx
 */
bool auto_ba = false;
module_param(auto_ba, bool, 0600);
MODULE_PARM_DESC(auto_ba, "Enable auto ba session setup on connection / QoS data Tx");

/**
 * Use SW Encryption instead of HW Encryption
 */
bool sw_enc = false;
module_param(sw_enc, bool, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(sw_enc, "Use SW Encryption instead of HW Encryption");

/**
 * Set Singal Monitor  mode
 */
bool signal_monitor = false;
module_param(signal_monitor, bool, 0600);
MODULE_PARM_DESC(signal_monitor, "Enable SIGNAL(RSSI/SNR) Monitor");

/**
 * Set configuration of KR USN
 */
bool enable_usn = false;
module_param(enable_usn, bool, 0600);
MODULE_PARM_DESC(enable_usn, "Use configuration of KR USN (Same ac between data and beacon)");

/**
 * Debug Level All
 */
bool debug_level_all = false;
module_param(debug_level_all, bool, 0600);
MODULE_PARM_DESC(debug_level_all, "Driver debug level all");

/**
 * credit number for AC_BE
 */
int credit_ac_be = 40; //default 40
module_param(credit_ac_be, int, 0600);
MODULE_PARM_DESC(credit_ac_be, "credit number for AC_BE");

/**
 * discard TX deauth frame for Mult-STA test
 */
bool discard_deauth = false;
module_param(discard_deauth , bool, 0600);
MODULE_PARM_DESC(discard_deauth , "(Test only) discard TX deauth for Multi-STA test");

/**
 * Use bitmap encoding for block ack
 */
bool bitmap_encoding = true;
module_param(bitmap_encoding , bool, 0600);
MODULE_PARM_DESC(bitmap_encoding , "(NRC7292 only) Use bitmap encoding for block ack");

/**
 * Apply scrambler reversely
 */
bool reverse_scrambler = true;
module_param(reverse_scrambler , bool, 0600);
MODULE_PARM_DESC(reverse_scrambler , "(NRC7292 only) Apply scrambler reversely");

static bool has_macaddr_param(uint8_t *dev_mac)
{
	int res;

	if (macaddr[0] == ':')
		return false;

	res = sscanf(macaddr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
			   &dev_mac[0], &dev_mac[1], &dev_mac[2],
			   &dev_mac[3], &dev_mac[4], &dev_mac[5]);

	return (res == 6);
}


static int s1g_unscaled_interval_max = 0x3fff;
static int convert_usf(int interval)
{
	int ui, usf = 0, interval_usf;

	if (interval <= s1g_unscaled_interval_max) {
		ui = interval;
		usf = 0;
	} else if (interval / 10 <= s1g_unscaled_interval_max) {
		ui= interval / 10;
		usf = 1;
	} else if (interval / 1000 <= s1g_unscaled_interval_max) {
		ui = interval / 1000;
		usf = 2;
	} else if (interval / 10000 <= s1g_unscaled_interval_max) {
		ui = interval / 10000;
		usf = 3;
	} else {
		ui = 0;
		usf = 0;
	}

	interval_usf = (usf << 14) + ui;

	return interval_usf;
}

/****************************************************************************
 * FunctionName : nrc_set_macaddr_from_fw
 * Description : This function set MAC Addresses from Serial Flash.
 *	Case1: macaddr0, macaddr1 are both written in Serial Flash
 *	VIF0 = macaddr0 / VIF1 = macaddr1
 *
 *	Case2: only macaddr0 is written in Serial Flash
 *	VIF0 = macaddr0 / VIF1 = macaddr0 with private bit
 *
 *	Case3: only macaddr1 is written in Serial Flash
 *	VIF0 = macaddr1 with private bit / VIF1 = macaddr1
 *
 *	Case4: macaddr0,macaddr1 are both not written in Serial Flash
 *	VIF0/VIF1 = generated macaddr by Host(RP)
 *
 * Returns : NONE
 *****************************************************************************/
static void nrc_set_macaddr_from_fw(struct nrc *nw, struct wim_ready *ready)
{
	int i;

	for (i = 0; i < NR_NRC_VIF; i++)
		nw->has_macaddr[i] = true;

	if (ready->v.has_macaddr[0] && ready->v.has_macaddr[1]) {
		for (i = 0; i < NR_NRC_VIF; i++) {
			memcpy(&nw->mac_addr[i].addr[0],
				&ready->v.macaddr[i][0], ETH_ALEN);
		}
	} else if (ready->v.has_macaddr[0] && !ready->v.has_macaddr[1]) {
		memcpy(&nw->mac_addr[0].addr[0],
			&ready->v.macaddr[0][0], ETH_ALEN);
		if (!(nw->mac_addr[0].addr[0]&0x2)) {
			memcpy(&nw->mac_addr[1].addr[0],
				&ready->v.macaddr[0][0], ETH_ALEN);
			nw->mac_addr[1].addr[0] |= 0x2;
		} else {
			nw->has_macaddr[1] = false;
		}
	} else if (!ready->v.has_macaddr[0] && ready->v.has_macaddr[1]) {
		memcpy(&nw->mac_addr[1].addr[0],
			&ready->v.macaddr[1][0], ETH_ALEN);
		if (!(nw->mac_addr[1].addr[0]&0x2)) {
			memcpy(&nw->mac_addr[0].addr[0],
				&ready->v.macaddr[1][0], ETH_ALEN);
			nw->mac_addr[0].addr[0] |= 0x2;
		} else {
			nw->has_macaddr[0] = false;
		}
	} else {
		for (i = 0; i < NR_NRC_VIF; i++)
			nw->has_macaddr[i] = false;
	}
}

static void nrc_on_fw_ready(struct sk_buff *skb, struct nrc *nw)
{
	struct ieee80211_hw *hw = nw->hw;
	struct wim_ready *ready;
	struct wim *wim = (struct wim *)skb->data;
	int i;

	nrc_dbg(NRC_DBG_HIF, "system ready");
	ready = (struct wim_ready *) (wim + 1);
	nrc_dbg(NRC_DBG_HIF, "ready");
	nrc_dbg(NRC_DBG_HIF, "  -- type: %d", ready->h.type);
	nrc_dbg(NRC_DBG_HIF, "  -- length: %d", ready->h.len);
	nrc_dbg(NRC_DBG_HIF, "  -- version: 0x%08X",
			ready->v.version);
	nrc_dbg(NRC_DBG_HIF, "  -- tx_head_size: %d",
			ready->v.tx_head_size);
	nrc_dbg(NRC_DBG_HIF, "  -- rx_head_size: %d",
			ready->v.rx_head_size);
	nrc_dbg(NRC_DBG_HIF, "  -- payload_align: %d",
			ready->v.payload_align);
	nw->fwinfo.ready = NRC_FW_ACTIVE;
	nw->fwinfo.version = ready->v.version;
	nw->fwinfo.rx_head_size = ready->v.rx_head_size;
	nw->fwinfo.tx_head_size = ready->v.tx_head_size;
	nw->fwinfo.payload_align = ready->v.payload_align;
	nw->fwinfo.buffer_size = ready->v.buffer_size;
	nrc_dbg(NRC_DBG_HIF, "  -- hw_version: %d",
			ready->v.hw_version);
	nw->fwinfo.hw_version = ready->v.hw_version;
	nrc_dbg(NRC_DBG_HIF, "  -- cap_mask: 0x%x",
			ready->v.cap.cap);
	nrc_dbg(NRC_DBG_HIF, "  -- cap_li: %d, %d",
			ready->v.cap.listen_interval, listen_interval);
	nrc_dbg(NRC_DBG_HIF, "  -- cap_idle: %d, %d",
			ready->v.cap.bss_max_idle, bss_max_idle);

	nw->cap.cap_mask = ready->v.cap.cap;
	nw->cap.listen_interval = ready->v.cap.listen_interval;
	nw->cap.bss_max_idle = ready->v.cap.bss_max_idle;
	nw->cap.max_vif = ready->v.cap.max_vif;

	if (has_macaddr_param(nw->mac_addr[0].addr)) {
		nw->has_macaddr[0] = true;
		nw->has_macaddr[1] = true;
		memcpy(nw->mac_addr[1].addr, nw->mac_addr[0].addr, ETH_ALEN);
		nw->mac_addr[1].addr[1]++;
		nw->mac_addr[1].addr[5]++;
	} else {
		nrc_set_macaddr_from_fw(nw, ready);
	}

	for (i = 0; i < NR_NRC_VIF; i++) {
		if (nw->has_macaddr[i])
			nrc_dbg(NRC_DBG_HIF, "  -- mac_addr[%d]: %pM",
					i, nw->mac_addr[i].addr);
	}

	for (i = 0; i < nw->cap.max_vif; i++) {
		nw->cap.vif_caps[i].cap_mask = ready->v.cap.vif_caps[i].cap;
		nrc_dbg(NRC_DBG_HIF, "  -- cap_mask[%d]: 0x%x", i,
			ready->v.cap.vif_caps[i].cap);
	}
	/* Override with insmod parameters */
	if (listen_interval) {
		hw->max_listen_interval = listen_interval;
		nw->cap.listen_interval = listen_interval;
	}

	if (nrc_mac_is_s1g(hw->priv) && bss_max_idle_usf_format) {
		/* bss_max_idle: in unit of 1000 TUs (1024ms = 1.024 seconds) */
		if (bss_max_idle > 16383 * 10000 || bss_max_idle <= 0) {
			nw->cap.bss_max_idle = 0;
		} else {
			/* Convert in USF Format (Value (14bit) * USF(2bit)) and save it */
			nw->cap.bss_max_idle = convert_usf(bss_max_idle);
		}
	} else {
		if (bss_max_idle > 65535 || bss_max_idle <= 0) {
			nw->cap.bss_max_idle = 0;
		} else {
			nw->cap.bss_max_idle = bss_max_idle;
		}
	}

	dev_kfree_skb(skb);
}

static void nrc_check_start(struct work_struct *work)
{
	struct nrc *nw = container_of(work, struct nrc, check_start.work);
	int ret;
	struct sk_buff *skb_req, *skb_resp;
	struct wim_drv_info_param *p;

	if (nw->drv_state == NRC_DRV_CLOSING)
		return;
	if (nrc_check_fw_file(nw)) {
		if (nrc_check_boot_ready(nw)) {
			/* ready to download fw, so kick it */
			nw->drv_state = NRC_DRV_BOOT;
			nrc_download_fw(nw);
			/* check the normal fw after 3 sec. */
			schedule_delayed_work(&nw->check_start,
					msecs_to_jiffies(3000));
			return;
		} else if (nw->drv_state == NRC_DRV_INIT) {
			/*
			 * not ready to download,
			 * so check it again after 10 ms
			 */
			schedule_delayed_work(&nw->check_start,
					msecs_to_jiffies(10));
			return;
		}
		/* just finished downloading, so go forward as normal flow */
		nw->drv_state = NRC_DRV_INIT;
		nrc_release_fw(nw);
	}

#if defined(CONFIG_CHECK_READY)
	while (!nrc_hif_check_ready(nw->hif)) {
		nrc_dbg(NRC_DBG_HIF, "Target doesn't ready yet.\n");
		mdelay(100);
	}
#endif /* defined(CONFIG_CHECK_READY) */

	nw->drv_state = NRC_DRV_START;
	nw->c_bcn = NULL;
	nw->c_prb_resp = NULL;

	nrc_hif_resume(nw->hif);

	init_completion(&nw->wim_responded);
	nw->workqueue = create_singlethread_workqueue("nrc_wq");
	nw->ps_wq = create_singlethread_workqueue("nrc_ps_wq");

	skb_req = nrc_wim_alloc_skb(nw, WIM_CMD_START, sizeof(int));
	if (!skb_req)
		goto fail_start;
	p = nrc_wim_skb_add_tlv(skb_req, WIM_TLV_DRV_INFO,
			sizeof(struct wim_drv_info_param), NULL);
	p->boot_mode = (nw->fw_priv->num_chunks > 0) ? 1 : 0;
	p->cqm_off = disable_cqm;
	p->bitmap_encoding = bitmap_encoding;
	p->reverse_scrambler = reverse_scrambler;
	skb_resp = nrc_xmit_wim_request_wait(nw, skb_req, (WIM_RESP_TIMEOUT * 30));
	if (skb_resp)
		nrc_on_fw_ready(skb_resp, nw);
	else
		goto fail_start;

	ret = nrc_register_hw(nw);
	if (ret) {
		pr_err("failed to register hw\n");
		goto fail_start;
	}

	ret = nrc_netlink_init(nw);
	if (ret) {
		pr_err("failed to register netlink\n");
		goto fail_start;
	}

	return;

 fail_start:
	nrc_free_hw(nw);

	nw->drv_state = NRC_DRV_INIT;

	nrc_dbg(NRC_DBG_MAC, "-%s:error!!", __func__);
}

static int nrc_platform_probe(struct platform_device *pdev)
{
	struct nrc *nw;
	int ret;

	nw = nrc_alloc_hw(pdev);
	if (!nw)
		return -ENOMEM;

	nrc_dbg_init(nw);
	nw->loopback = loopback;
	nw->lb_count = lb_count;

	nw->fw_priv = nrc_fw_init(nw);
	if (!nw->fw_priv) {
		pr_err("failed to initialize fw");
		ret = -EINVAL;
		goto fail;
	}

	nw->hif = nrc_hif_init(nw);
	if (!nw->hif) {
		pr_err("failed to initialize hif");
		ret = -EINVAL;
		goto fail;
	}

	INIT_DELAYED_WORK(&nw->check_start, nrc_check_start);

	platform_set_drvdata(pdev, nw);

	nw->drv_state = NRC_DRV_INIT;
	nw->fw = NULL;

	schedule_delayed_work(&nw->check_start, msecs_to_jiffies(10));

	return 0;

 fail:
	nrc_free_hw(nw);
	platform_set_drvdata(pdev, NULL);

	return ret;
}

static int nrc_platform_remove(struct platform_device *pdev)
{
	struct nrc *nw = platform_get_drvdata(pdev);
	int counter = 0;

	while(atomic_read(&nw->d_deauth.delayed_deauth)) {
		msleep(100);
		if (counter++ > 10) {
			atomic_set(&nw->d_deauth.delayed_deauth, 0);
			gpio_set_value(RPI_GPIO_FOR_PS, 0);
			break;
		}
	}

	if (nw->drv_state == NRC_DRV_PS) {
		gpio_set_value(RPI_GPIO_FOR_PS, 1);
		msleep(20);
	}
	nw->drv_state = NRC_DRV_CLOSING;

	cancel_delayed_work(&nw->check_start);
	cancel_delayed_work(&nw->fake_bcn);
	flush_delayed_work(&nw->fake_bcn);
	cancel_delayed_work(&nw->fake_prb_resp);
	flush_delayed_work(&nw->fake_prb_resp);
	if (nw->c_bcn) {
		dev_kfree_skb_any(nw->c_bcn);
		nw->c_bcn = NULL;
	}
	if (nw->c_prb_resp) {
		dev_kfree_skb_any(nw->c_prb_resp);
		nw->c_prb_resp = NULL;
	}

	if (ieee80211_hw_check(nw->hw, SUPPORTS_DYNAMIC_PS)) {
		del_timer_sync(&nw->dynamic_ps_timer);
	}

	if (!loopback) {
		nrc_netlink_exit();
		gpio_set_value(RPI_GPIO_FOR_PS, 0);
		nrc_unregister_hw(nw);
		nrc_fw_exit(nw->fw_priv);
	}
	nrc_hif_close(nw->hif);
	nrc_hif_exit(nw->hif);

	if (nw->hw)
		nrc_free_hw(nw);

	platform_set_drvdata(pdev, NULL);
	nw->drv_state = NRC_DRV_CLOSED;
	nw->pdev = NULL;
#if defined(ENABLE_HW_RESET)
	gpio_set_value(RPI_GPIO_FOR_RST, 0);
	gpio_free(RPI_GPIO_FOR_RST);
#endif

	return 0;
}

static struct platform_driver nrc_driver = {
	.driver = {
		.name = "nrc80211",
	},
	.probe = nrc_platform_probe,
	.remove = nrc_platform_remove,
};

static void nrc_device_release(struct device *dev)
{
}

static struct platform_device nrc_device = {
	.name = "nrc80211",
	.id = -1,
	.dev.release = nrc_device_release,
};

void nrc_set_bss_max_idle_offset(int value)
{
	bss_max_idle_offset = value;
}

void nrc_set_auto_ba(bool toggle)
{
	auto_ba = toggle;
	nrc_dbg(NRC_DBG_MAC, "%s: Auto BA session feature %s", __func__, auto_ba ? "ON" : "OFF");
}

static int __init nrc_init(void)
{
	int err;

	err = nrc_stats_init();
	if (err)
		return err;

#if defined(CONFIG_SUPPORT_BD)
	err = nrc_check_bd();
	if (err)
		return err;
#endif /* defined(CONFIG_SUPPORT_BD) */

	err = platform_device_register(&nrc_device);
	if (err)
		return err;

	err = platform_driver_register(&nrc_driver);
	if (err)
		goto fail_driver_register;

	nrc_set_bss_max_idle_offset(bss_max_idle_offset);

	return 0;

fail_driver_register:
	platform_device_unregister(&nrc_device);

	return err;
}
module_init(nrc_init);

static void __exit nrc_exit(void)
{
	nrc_dbg(NRC_DBG_MAC, "+%s", __func__);

	nrc_exit_debugfs();

	platform_device_unregister(&nrc_device);
	platform_driver_unregister(&nrc_driver);
	nrc_stats_deinit();

	nrc_dbg(NRC_DBG_MAC, "-%s", __func__);
}
module_exit(nrc_exit);

MODULE_AUTHOR("Newracom, Inc.(http://www.newracom.com)");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Newracom 802.11 driver");
#if KERNEL_VERSION(5, 12, 0) > NRC_TARGET_KERNEL_VERSION
MODULE_SUPPORTED_DEVICE("Newracom 802.11 devices");
#endif
