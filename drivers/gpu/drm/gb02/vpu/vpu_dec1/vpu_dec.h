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

#ifndef __VPU_DEC1_H__
#define __VPU_DEC1_H__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/vmalloc.h>


//#include "vpu_subsys.h"
#include "vpu/vpu_comm/vpu_comm.h"
#include "vpu/vpu_comm/vpu_vcmd_registers.h"

extern unsigned long multicorebase1[GB02MAC12];
extern int irq1[GB02MAC12];
extern unsigned int iosize1[GB02MAC12];


void GB02FUNC1033(int i, int j, struct GB02STR31 *dec_data);
void GB02FUNC1035(int i, int j, struct GB02STR31 *dec_data);
struct GB02STR31 *GB02FUNC1137(void);
unsigned long *GB02FUNC1018(void);
int *GB02FUNC1020(void);
unsigned int *GB02FUNC1021(void);
long GB02FUNC1109(struct drm_file *filp, unsigned int cmd, void *arg);
void dec1_cleanup(int total_vcmd_core_num);
void GB02FUNC1138(struct GB02STR31 *dec_data,
			    struct GB02STR69 *pci_bars,
			    struct GB02STR67 *dev_info,
			    struct GB02STR126 vpu_offset);
#endif
