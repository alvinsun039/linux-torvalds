/* SPDX-License-Identifier: GPL-3.0-or-later */
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
#ifndef __VPU_DEC1_IRQ_H__
#define __VPU_DEC1_IRQ_H__

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/interrupt.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
irqreturn_t hantrovcmd_d1isr(int irq, void *dev_id, struct pt_regs *regs);
#else
irqreturn_t hantrovcmd_d1isr(int irq, void *dev_id);
#endif

#endif
