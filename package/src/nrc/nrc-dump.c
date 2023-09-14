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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <asm/uaccess.h>
#include "nrc-dump.h"
#include "nrc.h"

static void write_file(char *filename, char *data, int len)
{
	struct file *filp;
	/*
	 * function force_uaccess_begin(), force_uaccess_end() and type mm_segment_t
	 * are removed in 5.18
	 * (https://patchwork.ozlabs.org/project/linux-arc/patch/20220216131332.1489939-19-arnd@kernel.org/#2847918)
	 * function get_fs(), and set_fs() are removed in 5.18
	 * (https://patchwork.kernel.org/project/linux-arm-kernel/patch/20201001141233.119343-11-arnd@arndb.de/)
	 */
#if KERNEL_VERSION(5,18,0) > NRC_TARGET_KERNEL_VERSION
	mm_segment_t old_fs;
#if KERNEL_VERSION(5,0,0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs( get_ds() );
#elif KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs( KERNEL_DS );
#elif KERNEL_VERSION(5,18,0) > NRC_TARGET_KERNEL_VERSION
#else
	old_fs = force_uaccess_begin();
#endif
#endif /* if KERNEL_VERSION(5,18,0) < NRC_TARGET_KERNEL_VERSION */
	filp = filp_open(filename, O_CREAT|O_RDWR, 0606);
	if (IS_ERR(filp)) {
		pr_err("[%s] error:%d", __func__, IS_ERR(filp));
		return;
	}
#if KERNEL_VERSION(4, 14, 0) <= NRC_TARGET_KERNEL_VERSION
	kernel_write(filp, data, len, &filp->f_pos);
#else
	kernel_write(filp, data, len, filp->f_pos);
#endif

	filp_close(filp, NULL);
#if KERNEL_VERSION(5,18,0) > NRC_TARGET_KERNEL_VERSION
#if KERNEL_VERSION(5,10,0) > NRC_TARGET_KERNEL_VERSION
	set_fs(old_fs);
#else
	force_uaccess_end(old_fs);
#endif
#endif /* if KERNEL_VERSION(5,18,0) < NRC_TARGET_KERNEL_VERSION */
}

void nrc_dump_init(void)
{
}

static int cnt;
void nrc_dump_store(char *src, int len)
{
	char str[32];

	sprintf(str, "./host_core_dump_%d.bin", cnt);
	write_file(str, src, len);
	cnt++;
}
