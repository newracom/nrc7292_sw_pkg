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

#include "nrc.h"
#include "nrc-hif.h"
#include "nrc-hif-uart.h"
#include "nrc-debug.h"
#include "nrc-wim-types.h"
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/poll.h>
#include <linux/delay.h>

#ifdef CONFIG_NRC_HIF_UART

static const u8 nrc_uart_soh[4] = {'N', 'R', 'C', '['};
static const u8 nrc_uart_eot[4] = {']', 'M', 'S', 'G'};

static void uart_start_user_oper(struct nrc_hif_uart *priv)
{
#if KERNEL_VERSION(5,0,0) > NRC_TARGET_KERNEL_VERSION
	priv->fs_snapshot = get_fs();
	set_fs( get_ds() );
#elif KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	priv->fs_snapshot = get_fs();
	set_fs( KERNEL_DS );
#else
	priv->fs_snapshot = force_uaccess_begin();
#endif
}

static void uart_stop_user_oper(struct nrc_hif_uart *priv)
{
#if KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	set_fs(priv->fs_snapshot);
#else
	force_uaccess_end(priv->fs_snapshot);
#endif
}

static long uart_ioctl(struct file *f, unsigned int op, unsigned long param)
{
	if (f->f_op->unlocked_ioctl)
		return f->f_op->unlocked_ioctl(f, op, param);

	/*return -ENOSYS;*/
	return -EINVAL;
}

static tcflag_t conv_baud(unsigned int baud)
{
	switch (baud) {
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 921600:
		return B921600;
	default:
		return B115200;
	}
}


static int _nrc_hif_uart_read(struct nrc_hif_uart *priv,  u8 *data, u32 len)
{
	struct file *f = priv->fp;

	f->f_pos = 0;
	return f->f_op->read(f, (__force char __user *)data, len, &f->f_pos);
}

static int _nrc_hif_uart_read_one(struct nrc_hif_uart *priv,  u8 *ch)
{
	return _nrc_hif_uart_read(priv, ch, 1);
}

static void uart_args_prepare(struct nrc_hif_uart *priv)
{
	memset(priv, 0, sizeof(struct nrc_hif_uart));
	priv->name = hifport;
	priv->device_baud = hifspeed;
	priv->fp = NULL;
#ifdef CONFIG_X86
	priv->fs_snapshot.seg = 0;
#endif
}

static int uart_open(struct nrc_hif_uart *priv)
{
	struct termios tio;
	struct serial_struct s;

	uart_start_user_oper(priv);

#ifdef CONFIG_ARM
	priv->fp = filp_open(priv->name, O_RDWR | O_NOCTTY, 0);
#else
	priv->fp = filp_open(priv->name, O_RDWR | O_NOCTTY, 0);
#endif

	if (IS_ERR(priv->fp)) {
		long err = PTR_ERR(priv->fp);

		nrc_dbg(NRC_DBG_HIF, "Failed to open %s, err=%d",
				priv->name, err);
		priv->fp = NULL;
		uart_stop_user_oper(priv);
		return err;
	}

	/*
	 * Set fixed baud rate of 115200 for boot mode HIF operation
	 */
	uart_ioctl(priv->fp, TCGETS, (unsigned long)&tio);
	memset(&tio, 0, sizeof(tio));

	tio.c_cflag =  (conv_baud(115200) | CLOCAL | CS8 | CREAD);
	tio.c_lflag = 0;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;

	uart_ioctl(priv->fp, TCSETS, (unsigned long)&tio);
	uart_ioctl(priv->fp, TIOCGSERIAL, (unsigned long)&s);

	s.flags |= ASYNC_LOW_LATENCY;

	uart_ioctl(priv->fp, TIOCSSERIAL, (unsigned long)&s);

	uart_stop_user_oper(priv);

	return 0;
}

static int uart_close(struct nrc_hif_uart *priv)
{
	uart_start_user_oper(priv);

	if (priv->fp) {
		filp_close(priv->fp, NULL);
		priv->fp = NULL;
	}

	uart_stop_user_oper(priv);

	return 0;
}

static const char *nrc_hif_uart_name(struct nrc_hif_device *dev)
{
	return "UART";
}

static void nrc_uart_poll(struct nrc_hif_uart *priv)
{
	struct poll_wqueues table;
	ktime_t start, now;
	struct file *f = priv->fp;

	const int timeout = 1000;

	start = ktime_get();
	poll_initwait(&table);

	while (1) {
		long elapsed;
		int mask;

		mask = f->f_op->poll(f, &table.pt);
		if (mask & (POLLRDNORM | POLLRDBAND | POLLIN |
			    POLLHUP | POLLERR)) {
			break;
		}
		now = ktime_get();
		elapsed = ktime_us_delta(now, start);
		if (elapsed > timeout)
			break;
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(((timeout - elapsed) * HZ) / 10000);
	}

	poll_freewait(&table);
}

static int nrc_uart_rx_data_done(struct nrc_hif_uart *priv,
		const u8 *data, const u32 len)
{
	static struct nrc_hif_rx_info info;

	struct nrc *nw = priv->dev->nw;

	WARN_ON(!nw);

	memset(&info, 0, sizeof(struct nrc_hif_rx_info));

	info.band = nw->band;
	info.freq = nw->center_freq;

	return nrc_hif_rx(priv->dev, data, len);
}

static void nrc_uart_rx_state_init(struct nrc_hif_uart *priv)
{
	priv->rx_state = NRC_HIF_RX_SOH;
	priv->rx_data_pos = 0;
	priv->rx_data_size = 0;
}

static int nrc_uart_rx_push(struct nrc_hif_uart *priv, u8 ch)
{
	if (priv->dev->suspended) {
		priv->ack_val = ch;
		priv->ack_received = true;
		return 0;
	}

	priv->rx_data[priv->rx_data_pos++] = ch;

	if (priv->rx_state == NRC_HIF_RX_SOH) {
		if (priv->rx_data_pos > 4) {
			nrc_dbg(NRC_DBG_HIF, "the size of SOH exceeds (State:%d rx_data_pos:%d)",
				priv->rx_state, priv->rx_data_pos);
			BUG();
		}
		if (priv->rx_data[priv->rx_data_pos-1]
				== nrc_uart_soh[priv->rx_data_pos-1]) {
			if (priv->rx_data_pos == 4) {
				priv->rx_state = NRC_HIF_RX_HIF_HDR;
				priv->rx_data_pos = 0;
			}
		} else {
			nrc_dbg(NRC_DBG_HIF, "Wrong SOH received (State:%d rx_data_pos:%d, 0x%02X)",
				priv->rx_state, priv->rx_data_pos, ch);
			nrc_uart_rx_state_init(priv);
		}
	} else if (priv->rx_state == NRC_HIF_RX_HIF_HDR) {
		if (priv->rx_data_pos == sizeof(struct hif_hdr)) {
			struct hif_hdr *hdr = (struct hif_hdr *)priv->rx_data;

			priv->rx_data_size = hdr->len;
			priv->rx_state = NRC_HIF_RX_PAYLOAD;
		}
	} else if (priv->rx_state == NRC_HIF_RX_PAYLOAD) {
		WARN_ON(priv->rx_data_pos == 0);
		WARN_ON(priv->rx_data_pos > (sizeof(struct hif_hdr)
					+ priv->rx_data_size));

		if (priv->rx_data_pos >= (sizeof(struct hif_hdr)
					+ priv->rx_data_size))
			priv->rx_state = NRC_HIF_RX_EOT;
	} else if (priv->rx_state == NRC_HIF_RX_EOT) {
		int eot_idx = (priv->rx_data_pos - priv->rx_data_size
				- sizeof(struct hif_hdr));

		if (priv->rx_data[priv->rx_data_pos-1]
				== nrc_uart_eot[eot_idx-1]) {
			if (eot_idx == 4) {
				nrc_uart_rx_data_done(priv, priv->rx_data,
						priv->rx_data_pos-4);
				nrc_uart_rx_state_init(priv);
			}
		} else {
			nrc_dbg(NRC_DBG_HIF, "Wrong EOT received (State:%d rx_data_pos:%d)",
				priv->rx_state, priv->rx_data_pos);
			nrc_uart_rx_state_init(priv);
		}
	}
	return 0;
}

static int nrc_uart_rx_thread(void *arg)
{
	u8 ch = 0;
	struct nrc_hif_uart *priv = (struct nrc_hif_uart *) arg;

	set_current_state(TASK_INTERRUPTIBLE);

	while (!kthread_should_stop()) {
		uart_start_user_oper(priv);

		nrc_uart_poll(priv);

		while (_nrc_hif_uart_read_one(priv, &ch))
			nrc_uart_rx_push(priv, ch);

		uart_stop_user_oper(priv);

		if (signal_pending(current))
			break;
	}

	while (!kthread_should_stop()) {
		flush_signals(current);
		set_current_state(TASK_INTERRUPTIBLE);
		if (!kthread_should_stop())
			schedule();
		set_current_state(TASK_RUNNING);
	}

	return 0;
}

static int nrc_hif_uart_start(struct nrc_hif_device *dev)
{
	struct nrc_hif_uart *priv = dev->priv;

	nrc_dbg(NRC_DBG_HIF, "%s", __func__);
	priv->dev = dev;

	if (uart_open(priv) != 0)
		return -EINVAL;

	if (priv) {
		priv->thread_rx = kthread_run(nrc_uart_rx_thread, priv,
				"uart_rx_thread");
		if (IS_ERR(priv->thread_rx)) {
			nrc_dbg(NRC_DBG_HIF, "Failed to run rx thread");
			return -EINVAL;
		}
		nrc_dbg(NRC_DBG_HIF, "thread run");
	}

	return 0;
}

static int nrc_hif_uart_stop(struct nrc_hif_device *dev)
{
	struct nrc_hif_uart *priv = dev->priv;

	if (priv->thread_rx) {
		kthread_stop(priv->thread_rx);
		nrc_dbg(NRC_DBG_HIF, "thread stop by force");
	}
	return uart_close(priv);
}

static struct file *get_uart_fp(struct nrc_hif_device *dev)
{
	struct nrc_hif_uart *uart = dev->priv;

	return uart->fp;
}

static int _nrc_hif_uart_write(struct file *f, const u8 *data, const u32 len)
{
	if (!f->f_op) {
		nrc_dbg(NRC_DBG_HIF, "f is not ready");
		return -EOPNOTSUPP;
	}

	f->f_pos = 0;

	if (!f->f_op) {
		nrc_dbg(NRC_DBG_HIF, "f_op is not ready");
		return -EOPNOTSUPP;
	}

	return f->f_op->write(f, (__force const char __user *)data,
			len, &f->f_pos);
}

#ifndef CONFIG_NRC_HIF_AH_UART
static int nrc_hif_uart_write(struct nrc_hif_device *dev, const u8 *data,
		const u32 len)
{
	int cnt = 0;
	struct file *f = get_uart_fp(dev);

	if (len == 0 || data == NULL)
		return 0;

	uart_start_user_oper(dev->priv);

	cnt += _nrc_hif_uart_write(f, nrc_uart_soh, 4);
	cnt += _nrc_hif_uart_write(f, data, len);
	cnt += _nrc_hif_uart_write(f, nrc_uart_eot, 4);

	uart_stop_user_oper(dev->priv);

	if (cnt == len + 8)
		return 0;
	return -1;
}
#endif

static int nrc_hif_uart_write_data(struct nrc_hif_device *dev, const u8 *data,
		const u32 len)
{
	struct file *f = get_uart_fp(dev);
	int ret = -1;

	if (f == NULL) {
		WARN_ON(1);
		return -1;
	}

	uart_start_user_oper(dev->priv);

	if (_nrc_hif_uart_write(f, data, len) == len)
		ret = 0;

	uart_stop_user_oper(dev->priv);

	return ret;
}

static int nrc_hif_uart_write_begin(struct nrc_hif_device *dev)
{
	return nrc_hif_uart_write_data(dev, nrc_uart_soh, 4);
}

static int nrc_hif_uart_write_end(struct nrc_hif_device *dev)
{
	return nrc_hif_uart_write_data(dev, nrc_uart_eot, 4);
}

static int nrc_hif_uart_write_body(struct nrc_hif_device *dev, const u8 *body,
		const u32 len)
{
	return nrc_hif_uart_write_data(dev, body, len);

}
static int nrc_hif_uart_suspend(struct nrc_hif_device *dev)
{
	return 0;
}

static int nrc_hif_uart_resume(struct nrc_hif_device *dev)
{
	struct termios tio;
	struct nrc_hif_uart *priv = dev->priv;

	uart_start_user_oper(priv);

	/*
	 * Set baud rate configured for normal HIF operation
	 */
	uart_ioctl(priv->fp, TCGETS, (unsigned long)&tio);
	memset(&tio, 0, sizeof(tio));

	tio.c_cflag =  (conv_baud(priv->device_baud) | CLOCAL | CS8 | CREAD);
	tio.c_lflag = 0;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;

	uart_ioctl(priv->fp, TCSETS, (unsigned long)&tio);

	uart_stop_user_oper(priv);

	return 0;
}

static int nrc_hif_uart_wait_ack(struct nrc_hif_device *dev, u8 *data, u32 len)
{
	int max = 1000;
	struct nrc_hif_uart *priv = dev->priv;

	while (!priv->ack_received && --max)
		udelay(1000);
	BUG_ON(!max);
	*data = priv->ack_val;
	priv->ack_received = false;
	return 1;
}

static struct nrc_hif_ops nrc_hif_uart_ops = {
	.name = nrc_hif_uart_name,
	.start = nrc_hif_uart_start,
	.stop = nrc_hif_uart_stop,
#ifdef CONFIG_NRC_HIF_AH_UART
	.write = nrc_hif_uart_write_data,
#else
	.write = nrc_hif_uart_write,
#endif
	.write_begin = nrc_hif_uart_write_begin,
	.write_body = nrc_hif_uart_write_body,
	.write_end = nrc_hif_uart_write_end,
	.suspend = nrc_hif_uart_suspend,
	.resume = nrc_hif_uart_resume,
	.wait_ack = nrc_hif_uart_wait_ack
};

struct nrc_hif_device *nrc_hif_uart_init(void)
{
	struct nrc_hif_device *dev;

	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);

	dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		/*nrc_dbg(NRC_DBG_HIF, "Failed to allocate nrc_hif_device");*/
		return NULL;
	}

	dev->priv = kmalloc(sizeof(struct nrc_hif_uart), GFP_KERNEL);

	if (!dev->priv) {
		/*nrc_dbg(NRC_DBG_HIF, "Failed to allocate nrc_hif priv");*/
		kfree(dev);
		return NULL;
	}

	uart_args_prepare(dev->priv);
	dev->hif_ops = &nrc_hif_uart_ops;

	return dev;
}

int nrc_hif_uart_exit(struct nrc_hif_device *dev)
{
	nrc_dbg(NRC_DBG_HIF, "%s()", __func__);

	kfree(dev->priv);
	dev->priv = NULL;
	kfree(dev);
	dev = NULL;
	return 0;
}

#endif
