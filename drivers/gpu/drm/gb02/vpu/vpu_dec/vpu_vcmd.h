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
#ifndef __VPU_VCMD_H__
#define __VPU_VCMD_H__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <drm/drm_device.h>
#include <errno.h>
#include "common/gb_uk.h"
#include "vpu/vpu_comm/vpu_basetype.h"
#include "vpu/vpu_comm/vpu_list.h"
#include "vpu/vpu_comm/vpu_vcmd_registers.h"
#include "vpu_subsys.h"
#include "vpu/vpu_comm/vpu_comm.h"

int GB02FUNC955(struct GB02STR20 dec_pcie, int total_vcmd_core_num,
	struct GB02STR3 *vcmd_buff, struct GB02STR3 *vcmd_reg,
	u64 dec_offset);

int GB02FUNC898(struct drm_file *filp, void **dec_priv,
	void *data);
long GB02FUNC980(struct drm_file *filp, u16 cmdbuf_id);

long GB02FUNC987(struct drm_file *filp,
		unsigned int cmd, void *arg);
void GB02FUNC976(int total_vcmd_core_num);
struct GB02STR43 *GB02FUNC700(void);
int GB02FUNC934(struct drm_file *filp, void *dec_priv);
void GB02FUNC790(struct GB02STR37 *dev);
void GB02FUNC813(struct GB02STR37 *dev,
	      bi_list_node *first_linked_cmdbuf_node);
void GB02FUNC793(struct GB02STR37 *dev,
	       bi_list_node *last_linked_cmdbuf_node);
void GB02FUNC705(struct GB02STR37 *dev,
	       bi_list_node *last_linked_cmdbuf_node);

long GB02FUNC845(struct drm_file *filp,
	int cmdbuf_id, int cmdbuf_size, int *core_id);
unsigned int GB02FUNC868(struct drm_file *filp,
	u32 cmdbuf_id, u32 *irq_status_ret);
int GB02FUNC985(struct drm_file *filp, void *data);
int GB02FUNC948(int irq_id);
void GB02FUNC717(struct GB02STR37 *hantrovcmd_data,
	int total_vcmd_core_num);
int GB02FUNC718(struct GB02STR37 *hantrovcmd_data,
	int total_vcmd_core_num, u8 *vpu_vcmd_base);
void GB02FUNC710(struct GB02STR37  *dev,
		int total_vcmd_core_num);
void GB02FUNC841(struct GB02STR37 *dev,
		u32 *jmp_addr, struct GB02STR18 *GB02STR18);
void GB02FUNC953(struct GB02STR43 *dec_data,
	struct GB02STR20 dec_pcie, struct GB02STR3 *vcmd_pool,
	struct GB02STR3 *vcmd_reg);
void GB02FUNC944(void);
void GB02FUNC966(void);
void GB02FUNC969(void);

#endif
