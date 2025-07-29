// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * jessy 20221019 Sietium
 */

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
#include <linux/dma-contiguous.h>
#else
#include <linux/dma-map-ops.h>
#endif
#include <linux/platform_device.h>
#include <linux/dma-buf.h>

#include "common/gb_common.h"
#include "vpu_afe.h"
#include "vpu_io.h"

void AXIFEEnable(volatile unsigned char *hwregs)
{
	if (!hwregs)
		return;
	//AXI FE pass through
	iowrite32(0x0, (void *)(hwregs + GB02MAC93 + 0x2C));
	gb_printf(KERN_INFO, "AXI FE: 0x2C = 0x%x\n", ioread32((void *)(hwregs + 0x2C)));
	iowrite32(0x2, (void *)(hwregs + GB02MAC93 + 0x28));
	gb_printf(KERN_INFO, "AXI FE: 0x28 = 0x%x\n", ioread32((void *)(hwregs + 0x28)));
}




