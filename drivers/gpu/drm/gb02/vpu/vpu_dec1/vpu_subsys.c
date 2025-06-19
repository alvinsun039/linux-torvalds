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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <asm/irq.h>

#include "vpu_subsys.h"
#include "vpu_dec.h"

extern struct GB02STR34 vcmd_core_array1[GB02MAC9];
/* List of subsystems */
struct SubsysDesc subsys_array1[] = {
	/* {slice_index, index, base} */
	{0, 0, 0x1000},
	// {0, 1, 0x700000}
};

/* List of all HW cores. */
struct CoreDesc core_array1[] = {
	/* {slice, subsys, core_type, offset, iosize, irq, has_apbfilter} */
	{0, 0, HW_VCMD, 0x0, 27 * 4, 0},
	{0, 0, HW_VC8000D, 0x1000, GB02MAC188 * 4, -1, 1},
	{0, 0, HW_L2CACHE, 0x2000, 231 * 4, -1, 0},
	{0, 0, HW_AXIFE, 0x5000, 64*4, -1, 1},
};

static void GB02FUNC1186(struct GB02STR23 *subsys,
	int vcmd, int total_vcmd_core_num, int num)
{
	int i;
	unsigned long *multicorebase = GB02FUNC1018();
	int *irq = GB02FUNC1020();
	unsigned int *iosize = GB02FUNC1021();

	if (vcmd) {
		for (i = 0; i < total_vcmd_core_num; i++) {
			vcmd_core_array1[i].vcmd_base_addr = subsys[i].base_addr;
			vcmd_core_array1[i].vcmd_iosize =
				subsys[i].submodule_iosize[HW_VCMD];
			vcmd_core_array1[i].vcmd_irq = subsys[i].irq;
			vcmd_core_array1[i].sub_module_type = 2;
			vcmd_core_array1[i].submodule_main_addr =
				subsys[i].submodule_offset[HW_VC8000D];
			vcmd_core_array1[i].submodule_dec400_addr =
				subsys[i].submodule_offset[HW_DEC400];
			vcmd_core_array1[i].submodule_L2Cache_addr =
				subsys[i].submodule_offset[HW_L2CACHE];
			vcmd_core_array1[i].submodule_MMU_addr =
				subsys[i].submodule_offset[HW_MMU];
			vcmd_core_array1[i].submodule_MMUWrite_addr =
				subsys[i].submodule_offset[HW_MMU_WR];
			vcmd_core_array1[i].submodule_axife_addr =
				subsys[i].submodule_offset[HW_AXIFE];
		}
	}
	memset(multicorebase, 0, sizeof(multicorebase[0]) * GB02MAC12);
	for (i = 0; i < num; i++) {
		multicorebase[i] = subsys[i].base_addr
			+ subsys[i].submodule_offset[HW_VC8000D];
		irq[i] = subsys[i].irq;
		iosize[i] = subsys[i].submodule_iosize[HW_VC8000D];
		gb_printf(KERN_INFO, "%s: [%d] multicorebase 0x%08lx, iosize %d\n",
			__func__, i, multicorebase[i], iosize[i]);
	}
}

void CheckSubsysCoreArray1(struct GB02STR23 *subsys, int *vcmd,
	int *total_vcmd_core_num)
{
	int num = sizeof(subsys_array1) / sizeof(subsys_array1[0]);
	int i, j, tmp_core_num;

	memset(subsys, 0, sizeof(subsys[0]) * GB02MAC9);
	for (i = 0; i < num; i++) {
		subsys[i].base_addr = subsys_array1[i].base;
		subsys[i].irq = -1;
		for (j = 0; j < HW_CORE_MAX; j++) {
			subsys[i].submodule_offset[j] = 0xffff;
			subsys[i].submodule_iosize[j] = 0;
			subsys[i].submodule_hwregs[j] = NULL;
		}
	}
	tmp_core_num = 0;

	for (i = 0; i < sizeof(core_array1) / sizeof(core_array1[0]); i++) {
		if (!subsys[core_array1[i].subsys].base_addr) {
			/* undefined subsystem */
			continue;
		}
		subsys[core_array1[i].subsys].submodule_offset[core_array1[i].core_type]
		 = core_array1[i].offset;
		subsys[core_array1[i].subsys].submodule_iosize[core_array1[i].core_type]
		 = core_array1[i].iosize;
		if (subsys[core_array1[i].subsys].irq != -1
			&& core_array1[i].irq != -1) {
			if (subsys[core_array1[i].subsys].irq
			 != core_array1[i].irq) {
				gb_printf(KERN_INFO, "%s: core type: %d, irq %d != sub irq %d\n",
				 __func__, core_array1[i].core_type,
				  core_array1[i].irq,
				   subsys[core_array1[i].subsys].irq);
			}
		} else if (core_array1[i].irq != -1) {
			subsys[core_array1[i].subsys].irq = core_array1[i].irq;
		}
		subsys[core_array1[i].subsys].has_apbfilter[core_array1[i].core_type]
		 = core_array1[i].has_apb;

		if (core_array1[i].core_type == HW_VCMD) {
			*vcmd = 1;
			tmp_core_num++;
		}
	}
	*total_vcmd_core_num = tmp_core_num;
	gb_printf(KERN_INFO, "hantrodec: vcmd = %d, *total_vcmd_core_num = %d\n",
		*vcmd, *total_vcmd_core_num);

	/* To plug into hantro_vcmd.c */
	GB02FUNC1186(subsys, *vcmd, *total_vcmd_core_num, num);

}


