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

#ifndef _NRC_HIF_H_
#define _NRC_HIF_H_

#include <net/mac80211.h>
#include "nrc.h"
#include "nrc-debug.h"

/*#define CONFIG_NRC_HIF_DEBUG_READ*/
/*#define CONFIG_NRC_HIF_DEBUG_WRITE*/

/* struct nrc_hif_ops - callbacks from transport layer to the host layer
 *
 * This structure mainly used to handle host interface contains various
 * callbacks that the host interface may handle or, in some cases,
 * must handle, for example to start, stop, transmit data to host.
 *
 * name: Handle calls for identifying the name of host interface
 *  this function is called when host interface is initialized.
 *
 * start: Handle that the transport layer on mac80211 host driver calls
 *  for host device initialization.
 *  This function is called after wpa_supplcant or hostapd is attached.
 *
 * stop: Handle that the transport layer on mac80211 host driver calls
 *  for host device cleanup.
 *  This function is called after wpa_supplcant or hostapd is detached.
 *
 * suspend: Handle that the transport layer on mac80211 host driver calls
 *  for host device sleep.
 *  This function is called when device sleeps
 *
 * resume: Handle that the transport layer on mac80211 host driver calls
 *  for host device wakeup.
 *  This function is called when device wake-up
 *
 * write: Handle that the transport layer on mac80211 host driver calls
 *  for each transmitted frame.
 *
 * write_multi: Handle that the transport layer on mac80211 host driver calls
 *  for each transmitted frame separated with multiple parts head, body
 *  and tail.
 *
 * need_maskrom_war: apply workaround when NRC7291 maskrom work.
 *
 * sync_auto: Determine which layer controls sync.
 *  true: controlled by driver layer
 *  false: controlled by hif layer
 */
struct nrc_hif_ops {
	const char *(*name)(struct nrc_hif_device *dev);
	bool (*check_fw)(struct nrc_hif_device *dev);
	int (*start)(struct nrc_hif_device *dev);
	int (*stop)(struct nrc_hif_device *dev);
	int (*write)(struct nrc_hif_device *dev, const u8 *data, const u32 len);
	int (*suspend)(struct nrc_hif_device *dev);
	int (*resume)(struct nrc_hif_device *dev);
	int (*write_begin)(struct nrc_hif_device *dev);
	int (*write_body)(struct nrc_hif_device *dev, const u8 *data,
			const u32 len);
	int (*write_end)(struct nrc_hif_device *dev);
	int (*wait_ack)(struct nrc_hif_device *dev, u8 *data, u32 len);
	/* New APIs */
	int (*xmit)(struct nrc_hif_device *dev, struct sk_buff *skb);
	int (*wait_for_xmit)(struct nrc_hif_device *dev, struct sk_buff *skb);
	int (*receive)(struct nrc_hif_device *dev, struct sk_buff *skb);
	void (*close)(struct nrc_hif_device *dev);
	void (*reset)(struct nrc_hif_device *dev);
	void (*wakeup)(struct nrc_hif_device *dev);
	int (*test)(struct nrc_hif_device *dev);
	void (*config)(struct nrc_hif_device *dev);
	void (*sync_lock)(struct nrc_hif_device *dev);
	void (*sync_unlock)(struct nrc_hif_device *dev);
	void (*disable_irq)(struct nrc_hif_device *dev);
	void (*enable_irq)(struct nrc_hif_device *dev);
	int (*status_irq)(struct nrc_hif_device *dev);
	void (*clear_irq)(struct nrc_hif_device *dev);
	void (*update)(struct nrc_hif_device *dev);
	void (*set_gpio)(int v);
	bool need_maskrom_war;
	bool sync_auto;
	bool (*support_fastboot)(struct nrc_hif_device *dev);
	int (*suspend_rx_thread)(struct nrc_hif_device *dev);
	int (*resume_rx_thread)(struct nrc_hif_device *dev);
	int (*check_target)(struct nrc_hif_device *dev, u8 reg);
#if defined(CONFIG_CHECK_READY)
	bool (*check_ready)(struct nrc_hif_device *dev);
#endif /* defined(CONFIG_CHECK_READY) */
};

/* struct nrc_hif_device - host interface driver
 *
 * @nr: pointer to struct nr that was allocated on host driver initialization
 *
 * @hif_ops: callbacks
 *
 * @started: flag to indicates whether host interface is started
 *
 * @suspended: flag to indicates whether host interface is suspeded
 *
 * @priv: pointer to private area for each host interface
 */
struct nrc_hif_device {
	struct nrc *nw;
	struct nrc_hif_ops *hif_ops;
	bool started;
	bool suspended;
	void *priv;

	struct sk_buff_head queue[2]; /* 0: frame, 1: wim */
	struct work_struct work;
	struct work_struct ps_work;
};

/* struct nrc_hif_rx_info - additional information on rx
 *
 * @offset: offset
 *
 * @need_free: nrc_hif is responsible for mem-free
 *
 * @in_interrupt: called in IRQ context
 *
 */
struct nrc_hif_rx_info {
	int offset;
	bool need_free;
	bool in_interrupt;
	u8 band;
	u16 freq;
};

/* nrc_hif_init - initialize host interface
 *
 * You must call this function before any other host functions.
 *
 * @nrc: pointer to struct nrc
 *
 * Return: pointer to struct nrc_hif_device PTR_ERR otherwise.
 */
struct nrc_hif_device *nrc_hif_init(struct nrc *nr);


/* nrc_hif_rx - receive data from host interface
 *
 * This function should be called when the host interface
 * receives data from Target
 *
 * @dev: pointer to host interface descriptor
 * @info: additional rx information
 * @data: the received data
 * @len: the length of the received data
 */
/*
 * int nrc_hif_rx(struct nrc_hif_device *dev, struct nrc_hif_rx_info *info,
 * const u8 *data, const u32 len);
 */
int nrc_hif_rx(struct nrc_hif_device *dev, const u8 *data, const u32 len);

/* nrc_hif_exit - free hostinterface descriptor
 *
 * @dev: pointer to host interface desciptor
 * Return: 0 on success. An error code other wires
 */
int nrc_hif_exit(struct nrc_hif_device *dev);

static inline const char *nrc_hif_name(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "name()");
	BUG_ON(!dev->hif_ops->name);
	return dev->hif_ops->name(dev);
}

static inline bool nrc_hif_check_fw(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->check_fw)
		return dev->hif_ops->check_fw(dev);
	else
		return false;
}

#if defined(CONFIG_CHECK_READY)
static inline bool nrc_hif_check_ready(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->check_ready)
		return dev->hif_ops->check_ready(dev);
	else
		return true;
}
#endif /* defined(CONFIG_CHECK_READY) */

static inline int nrc_hif_start(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "start()");
	BUG_ON(!dev->hif_ops->start);
	dev->started = true;
	return dev->hif_ops->start(dev);
}

static inline int nrc_hif_stop(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "stop()");
	BUG_ON(!dev->hif_ops->stop);

	if (!dev->started)
		return 0;

	dev->hif_ops->stop(dev);
	dev->started = false;
	return true;
}

static inline int nrc_hif_write(struct nrc_hif_device *dev, u8 *data, u32 len)
{
	/*nrc_dbg(NRC_DBG_HIF, "write(len=%d)", len);*/
	BUG_ON(!dev->hif_ops->write);
	return dev->hif_ops->write(dev, data, len);
}

static inline int nrc_hif_write_begin(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "write_begin()");
	if (dev->hif_ops->write_begin)
		return dev->hif_ops->write_begin(dev);
	else
		return -1;
}

static inline int nrc_hif_write_body(struct nrc_hif_device *dev, u8 *body,
		u32 len)
{
	nrc_dbg(NRC_DBG_HIF, "write_body(body_len=%d)", len);
	if (dev->hif_ops->write_body)
		return dev->hif_ops->write_body(dev, body, len);
	else
		return -1;
}

static inline int nrc_hif_write_end(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "write_end()");
	if (dev->hif_ops->write_end)
		return dev->hif_ops->write_end(dev);
	else
		return -1;
}

static inline int nrc_hif_suspend(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "suspend()");
	if (dev->suspended)
		return 0;

	dev->suspended = true;

	if (dev->hif_ops->suspend)
		return dev->hif_ops->suspend(dev);

	return 0;
}

static inline int nrc_hif_resume(struct nrc_hif_device *dev)
{
	dev->suspended = false;
	if (dev->hif_ops->resume)
		dev->hif_ops->resume(dev);

	return 0;
}

static inline int nrc_hif_wait_ack(struct nrc_hif_device *dev,
		u8 *data, u32 len)
{
	BUG_ON(!dev->hif_ops->wait_ack);
	return dev->hif_ops->wait_ack(dev, data, len);
}

static inline bool nrc_hif_check_maskrom_war(struct nrc_hif_device *dev)
{
	return dev->hif_ops->need_maskrom_war;
}

static inline void nrc_hif_disable_irq(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->disable_irq)
		dev->hif_ops->disable_irq(dev);
}

static inline int nrc_hif_xmit(struct nrc_hif_device *dev, struct sk_buff *skb)
{
	if (dev->hif_ops->xmit)
		return dev->hif_ops->xmit(dev, skb);
	return -1;
}

static inline void nrc_hif_set_gpio(struct nrc_hif_device *dev, int hi_lo)
{
	if (dev->hif_ops->set_gpio)
		dev->hif_ops->set_gpio(hi_lo);
}

static inline void nrc_hif_update(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->update)
		dev->hif_ops->update(dev);
}

static inline void nrc_hif_set_enable_irq(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->enable_irq)
		dev->hif_ops->enable_irq(dev);
}

static inline int  nrc_hif_wait_for_xmit(struct nrc_hif_device *dev,
					struct sk_buff *skb)
{
	if (dev->hif_ops->wait_for_xmit)
		return dev->hif_ops->wait_for_xmit(dev, skb);
	return -1;
}

static inline bool nrc_hif_support_fastboot(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->support_fastboot)
		return dev->hif_ops->support_fastboot(dev);

	return false;
}

static inline void nrc_hif_config(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->config)
		dev->hif_ops->config(dev);
}

static inline int nrc_hif_suspend_rx_thread(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->suspend_rx_thread)
		return dev->hif_ops->suspend_rx_thread(dev);

	return 0;
}

static inline int nrc_hif_resume_rx_thread(struct nrc_hif_device *dev)
{
	if (dev->hif_ops->resume_rx_thread)
		return dev->hif_ops->resume_rx_thread(dev);

	return 0;
}

static inline int nrc_hif_check_target(struct nrc_hif_device *dev, u8 reg)
{
	if (dev->hif_ops->check_target)
		return dev->hif_ops->check_target(dev, reg);

	return -1;
}

struct hif {
	u8 type;
	u8 subtype;
	u8 flags;
	s8 vifindex;
	u16 len;
	u16 tlv_len;
	u8 payload[0];
} __packed;

#define SYNC_LOCK(x) do {\
	if (!x->hif_ops->sync_auto && x->hif_ops->sync_lock)\
		x->hif_ops->sync_lock(x); } while (0)
#define SYNC_UNLOCK(x) do {;\
	if (!x->hif_ops->sync_auto && x->hif_ops->sync_unlock)\
		x->hif_ops->sync_unlock(x); } while (0)

#define HIF_TX_COMPLETE 0
#define HIF_TX_QUEUED   1
#define HIF_TX_FAILED   2
#define HIF_TX_PASSOVER 3

int nrc_xmit_wim_request(struct nrc *nw, struct sk_buff *skb);
/*
 * nrc_xmit_wim_request_wait: - Transmit WIM REQUEST message and wait
 * until WIM RESPONSE message received or timeout reached.
 *
 * Return: WIM RESPONSE message, null if timeout reached.
 */

int nrc_xmit_wim_powersave(struct nrc *nw, struct sk_buff *skb_src, uint16_t ps_enable, uint64_t duration);
struct sk_buff *nrc_xmit_wim_request_wait(struct nrc *nw,
		struct sk_buff *skb, int timeout);
int nrc_xmit_wim_response(struct nrc *nw, struct sk_buff *skb);
int nrc_xmit_wim_simple_request(struct nrc *nw, int cmd);
int nrc_xmit_frame(struct nrc *nw, s8 vif_index, u16 aid, struct sk_buff *skb);
int nrc_xmit_injected_frame(struct nrc *nw,
		   struct ieee80211_vif *vif,
		   struct ieee80211_sta *sta,
		   struct sk_buff *skb);
void nrc_hif_free_skb(struct nrc *nw, struct sk_buff *skb);
void nrc_hif_debug_send(struct nrc *nw, struct sk_buff *skb);
int nrc_hif_debug_rx(void);
int nrc_hif_close(struct nrc_hif_device *dev);
int nrc_hif_wakeup_device(struct nrc_hif_device *dev);
int nrc_hif_reset_device(struct nrc_hif_device *dev);
int nrc_hif_test_status(struct nrc_hif_device *dev);
void nrc_hif_down(struct nrc *nw);
void nrc_hif_up(struct nrc *nw);
void nrc_hif_sync_lock(struct nrc_hif_device *dev);
void nrc_hif_sync_unlock(struct nrc_hif_device *dev);
void nrc_hif_flush_wq(struct nrc_hif_device *dev);
#endif
