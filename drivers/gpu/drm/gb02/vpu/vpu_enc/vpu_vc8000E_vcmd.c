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
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
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
#include <linux/platform_device.h>
#include "gpu/gb_device.h"
#include "common/gb_common.h"
#include "vpu_vc8000E_vcmd.h"
#include "vpu_vc8000E_irq.h"
#include "vpu/vpu_comm/vpu_list.h"
#include "vpu/vpu_comm/vpu_vcmd_registers.h"
#include "vpu/vpu_comm/vpu_comm.h"
#include "vpu/vpu_comm/vpu_basetype.h"
#include "common/gb_kernel_ver.h"

static void GB02FUNC1629(bi_list_node *cmdbuf_node);
/*for all vcmds, the core info should be listed here for subsequent use*/
static struct GB02STR5 enc_vcmd_core_array[] = {
	#if defined(NETINT) || defined(OYB_VCE)
	//encoder configuration
	{GB02MAC2376,
	GB02MAC2377,
	GB02MAC2378,
	GB02MAC2379,
	GB02MAC2380,
	GB02MAC2381,
	GB02MAC2383,
	{GB02MAC2384,
		GB02MAC2385},
	{GB02MAC2387,
		GB02MAC2389}},
	#endif
};

#ifdef HANTROAXIFE_SUPPORT
#define GB02MAC2037 (64*4)
volatile u8 *axife_hwregs[GB02MAC152][2];
#endif

struct GB02STR7 *enc_vcmd_manager[MAX_VCMD_TYPE][GB02MAC152];
/* dynamic allocation*/
struct GB02STR7 *hantrovcmd_enc_data;
static struct GB02STR186 *enc_private_data;

//hw_queue can be used for reserve cmdbuf memory
DECLARE_WAIT_QUEUE_HEAD(enc_vcmd_cmdbuf_memory_wait);
DEFINE_SPINLOCK(enc_vcmd_cmdbuf_alloc_lock);
DEFINE_SPINLOCK(enc_vcmd_process_manager_lock);

static u32 GB02FUNC1488(int total_vcmd_core_num)
{
#ifdef HANTROAXIFE_SUPPORT
	int i = 0, j = 0;

	for (i = 0; i < total_vcmd_core_num; i++) {
		for (j = 0; j < 2; j++) {
			if (axife_hwregs[i][j] != NULL) {
				iounmap(axife_hwregs[i][j]);
				release_mem_region(enc_vcmd_core_array[i].vcmd_base_addr +
				 enc_vcmd_core_array[i].submodule_axife_addr[j],
					GB02MAC2037);
				axife_hwregs[i][j] = NULL;
			}
		}
	}
#endif
	return 0;
}

static u32 ConfigAXIFE(u32 mode, int total_vcmd_core_num)
{
#ifdef HANTROAXIFE_SUPPORT
	u32 i = 0;
	u8 sub_module_type = 0;

	for (i = 0; i < total_vcmd_core_num; i++) {
		sub_module_type = enc_vcmd_core_array[i].sub_module_type;
		gb_printf(KERN_INFO, "%s: sub_module_type is %d\n",
			__func__, sub_module_type);
		if (enc_vcmd_core_array[i].submodule_axife_addr[0] != 0xffff) {
			if (!request_mem_region(enc_vcmd_core_array[i].vcmd_base_addr
					+ enc_vcmd_core_array[i].submodule_axife_addr[0],
					GB02MAC2037, "vc8000E")) {
				gb_printf(KERN_ERR, "%s: error request_mem for axife_hwregs[%d][0]\n",
					__func__, i);
				return -EBUSY;
			}
			axife_hwregs[i][0] =
				(volatile u8 *)ioremap_nocache(enc_vcmd_core_array[i].vcmd_base_addr
				+ enc_vcmd_core_array[i].submodule_axife_addr[0],
				GB02MAC2037);
			if (axife_hwregs[i][0] != NULL) {
				gb_printf(KERN_INFO, "%s: axife_hwregs[%d][0] = %p\n",
				 __func__, i,
					axife_hwregs[i][0]);
				AXIFEEnable(axife_hwregs[i][0], mode);
			}
		}
		if (enc_vcmd_core_array[i].submodule_axife_addr[1] != 0xffff) {
			if (!request_mem_region(enc_vcmd_core_array[i].vcmd_base_addr +
				enc_vcmd_core_array[i].submodule_axife_addr[1],
				 GB02MAC2037, "vc8000E")) {
				gb_printf(KERN_ERR, "%s: error request_mem for axife_hwregs[%d][1]\n",
					__func__, i);
			}
			axife_hwregs[i][1]  =
				(volatile u8 *)ioremap_nocache(enc_vcmd_core_array[i].vcmd_base_addr
				+ enc_vcmd_core_array[i].submodule_axife_addr[1],
				GB02MAC2037);
			if (axife_hwregs[i][1]  != NULL) {
				gb_printf(KERN_INFO, "%s: axife_hwregs[%d][1] = %p\n",
				 __func__, i,
					axife_hwregs[i][1]);
				AXIFEEnable(axife_hwregs[i][1], mode);
			}
		}
	}
#endif
	return 0;
}

static void GB02FUNC1497(struct drm_gb_cmdbuf_resv *input_para,
	struct GB02STR14 *enc_vcmd_mem_pool,
	bi_list_node **global_cmdbuf_node)
{
	u32 register_range[] = {GB02MAC189,
		GB02MAC191, GB02MAC190,
		GB02MAC192,
		 GB02MAC193};
	u32 counter_cmdbuf_size = 0;
	struct GB02STR14 *vcmd_status_buf_mem_pool = &enc_vcmd_mem_pool[1];
	u32  *set_base_addr;
	u16 submodule_main_addr =
		enc_vcmd_manager[input_para->module_type][0]->enc_vcmd_core_cfg.submodule_main_addr;
	ptr_t status_base_phy_addr;
	u32 offset_inc = 0;
	u32 offset_inc_dec400 = 0;
	u32 hw_version_id =
		enc_vcmd_manager[input_para->module_type][0]->hw_version_id;
	u16 submodule_L2Cache_addr =
		enc_vcmd_manager[input_para->module_type][0]->enc_vcmd_core_cfg.submodule_L2Cache_addr;
	u16 submodule_dec400_addr =
		enc_vcmd_manager[input_para->module_type][0]->enc_vcmd_core_cfg.submodule_dec400_addr;
	size_t base_ddr_addr;
	struct GB02STR186 *enc_data = NULL;
	struct GB02STR18 *cmdbuf =
	 (struct GB02STR18 *)(global_cmdbuf_node[input_para->cmdbuf_id]->data);

	enc_data = GB02FUNC1639();
	base_ddr_addr = enc_data->enc_base_ddr_addr;
	set_base_addr = cmdbuf->cmdbuf_virtual_address;
	status_base_phy_addr = cmdbuf->status_bus_address +
	       (submodule_main_addr / 2 + 0);

	if (hw_version_id > GB02MAC186) {
		gb_printf(KERN_INFO, "%s: > hw_version_id:0x%x\n", __func__, hw_version_id);

		*(set_base_addr + 0) = (GB02MAC177) | (1 << 16)
			| (GB02MAC143 * 4);
		counter_cmdbuf_size += 4;
		*(set_base_addr + 1) = (u32)0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 2) = (u32)0;
		counter_cmdbuf_size += 4;

		//alignment
		*(set_base_addr + 3) = 0;
		counter_cmdbuf_size += 4;

		//read main IP all registers
		*(set_base_addr + 4) = (GB02MAC177)
			| ((register_range[input_para->module_type] / 4) << 16)
			| (submodule_main_addr + 0);
		counter_cmdbuf_size += 4;
		*(set_base_addr + 5) = (u32)(status_base_phy_addr
		 - base_ddr_addr);
		counter_cmdbuf_size += 4;
		if (sizeof(size_t) == 8)
			*(set_base_addr + 6) =
			 (u32)((u64)(status_base_phy_addr -
				base_ddr_addr) >> 32);
		else
			*(set_base_addr + 6) = 0;

		counter_cmdbuf_size += 4;
		//alignment
		*(set_base_addr + 7) = 0;
		counter_cmdbuf_size += 4;

		if (submodule_L2Cache_addr != 0xffff) {
			/* read L2 cache register */
			offset_inc = 4;
			status_base_phy_addr =
			 vcmd_status_buf_mem_pool->bus_address +
				input_para->cmdbuf_id * GB02MAC156 +
				(submodule_L2Cache_addr / 2 + 0);
			/* read L2cache IP first register */
			*(set_base_addr + 8) = (GB02MAC177) | (1 << 16)
				| (submodule_L2Cache_addr + 0);
			counter_cmdbuf_size += 4;

			*(set_base_addr + 9) = (u32)(status_base_phy_addr -
				base_ddr_addr);
			counter_cmdbuf_size += 4;

			if (sizeof(size_t) == 8)
				*(set_base_addr + 10) =
					(u32)((u64)(status_base_phy_addr -
					 base_ddr_addr) >> 32);
			else
				*(set_base_addr + 10) = 0;

			counter_cmdbuf_size += 4;

			*(set_base_addr + 11) = 0;
			counter_cmdbuf_size += 4;
		}
		if (submodule_dec400_addr != 0xffff) {
			/* read dec400 register */
			offset_inc_dec400 = 4;
			status_base_phy_addr =
			 vcmd_status_buf_mem_pool->bus_address +
				input_para->cmdbuf_id * GB02MAC156 +
				(submodule_dec400_addr / 2 + 0);
			//read DEC400 IP first register
			*(set_base_addr + 8 + offset_inc) = (GB02MAC177)
				| (0x2b << 16) | (submodule_dec400_addr + 0);
			counter_cmdbuf_size += 4;
			*(set_base_addr + 9 + offset_inc) =
				(u32)(status_base_phy_addr - base_ddr_addr);
			counter_cmdbuf_size += 4;

			if (sizeof(size_t) == 8)
				*(set_base_addr + 10 + offset_inc) =
					(u32)((u64)(status_base_phy_addr -
					 base_ddr_addr) >> 32);
			else
				*(set_base_addr + 10 + offset_inc) = 0;
			counter_cmdbuf_size += 4;
			//alignment
			*(set_base_addr + 11 + offset_inc) = 0;
			counter_cmdbuf_size += 4;
		}

		/* read vcmd registers to ddr */
		*(set_base_addr + 8 + offset_inc + offset_inc_dec400) =
			(GB02MAC177) | (27 << 16) | (0);
		counter_cmdbuf_size += 4;
		*(set_base_addr + 9 + offset_inc + offset_inc_dec400) =
		 (u32)0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 10 + offset_inc + offset_inc_dec400) =
		 (u32)0;
		counter_cmdbuf_size += 4;
		//alignment
		*(set_base_addr + 11 + offset_inc + offset_inc_dec400) = 0;
		counter_cmdbuf_size += 4;
		//JMP RDY = 0
		*(set_base_addr + 12 + offset_inc + offset_inc_dec400) =
			(GB02MAC182) | 0 | GB02MAC184 | 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 13 + offset_inc + offset_inc_dec400) = 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 14 + offset_inc + offset_inc_dec400) = 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 15 + offset_inc + offset_inc_dec400) =
			input_para->cmdbuf_id;
		//counter_cmdbuf_size += 4;
		input_para->cmdbuf_size = (16 + offset_inc +
		 offset_inc_dec400) * 4;
	} else {
		gb_printf(KERN_INFO, "%s: < hw_version_id:0x%x\n", __func__, hw_version_id);
		//read all registers
		*(set_base_addr + 0) = (GB02MAC177)
			| ((register_range[input_para->module_type] / 4) << 16)
			| (submodule_main_addr + 0);
		counter_cmdbuf_size += 4;

		*(set_base_addr + 1) = (u32)(status_base_phy_addr
		 - base_ddr_addr);
		counter_cmdbuf_size += 4;

		if (sizeof(size_t) == 8)
			*(set_base_addr + 2) =
				(u32)((u64)(status_base_phy_addr -
				 base_ddr_addr) >> 32);
		else
			*(set_base_addr + 2) = 0;
		counter_cmdbuf_size += 4;

		//alignment
		*(set_base_addr + 3) = 0;
		counter_cmdbuf_size += 4;

		//JMP RDY = 0
		*(set_base_addr + 4) = (GB02MAC182) | 0 | GB02MAC184 | 0;
		counter_cmdbuf_size += 4;

		*(set_base_addr + 5) = 0;
		counter_cmdbuf_size += 4;

		*(set_base_addr + 6) = 0;
		counter_cmdbuf_size += 4;

		*(set_base_addr + 7) = input_para->cmdbuf_id;
		//counter_cmdbuf_size += 4;
		input_para->cmdbuf_size = 8 * 4;
	}
}

static int GB02FUNC1507(struct GB02STR7 *dst_dev,
	bi_list *list, bi_list_node *new_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	unsigned long flags = 0;
	int counter = 0;
	struct GB02STR7 *dev;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	while (1) {
		dev = enc_vcmd_manager[GB02STR18->module_type][enc_data->vcmd_position[GB02STR18->module_type]];
		list = &dev->list_manager;
		spin_lock_irqsave(dev->spinlock, flags);
		if (list->tail == NULL) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			enc_data->vcmd_position[GB02STR18->module_type]++;
			if (enc_data->vcmd_position[GB02STR18->module_type]
			 >= enc_data->vcmd_type_core_num[GB02STR18->module_type])
				enc_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return -1;
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		enc_data->vcmd_position[GB02STR18->module_type]++;
		if (enc_data->vcmd_position[GB02STR18->module_type] >=
			enc_data->vcmd_type_core_num[GB02STR18->module_type])
			enc_data->vcmd_position[GB02STR18->module_type]
				= 0;
		counter++;

		if (counter >= enc_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	return 0;
}

static int GB02FUNC1510(struct GB02STR7 *dst_dev,
	bi_list *list, bi_list_node *new_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	int counter = 0;
	bi_list_node *curr_cmdbuf_node = NULL;
	unsigned long flags = 0;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;
	struct GB02STR7 *dev;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	while (1) {
		dev = enc_vcmd_manager[GB02STR18->module_type][enc_data->vcmd_position[GB02STR18->module_type]];
		list = &dev->list_manager;
		spin_lock_irqsave(dev->spinlock, flags);
		curr_cmdbuf_node = list->tail;
		if (curr_cmdbuf_node == NULL) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			enc_data->vcmd_position[GB02STR18->module_type]++;
			if (enc_data->vcmd_position[GB02STR18->module_type] >=
				enc_data->vcmd_type_core_num[GB02STR18->module_type])
				enc_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return -1;
		}
		cmdbuf_obj_temp = (struct GB02STR18 *)curr_cmdbuf_node->data;
		if (cmdbuf_obj_temp->cmdbuf_run_done == 1) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			enc_data->vcmd_position[GB02STR18->module_type]++;
			if (enc_data->vcmd_position[GB02STR18->module_type] >=
				enc_data->vcmd_type_core_num[GB02STR18->module_type])
				enc_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return -1;
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		enc_data->vcmd_position[GB02STR18->module_type]++;
		if (enc_data->vcmd_position[GB02STR18->module_type] >=
			enc_data->vcmd_type_core_num[GB02STR18->module_type])
			enc_data->vcmd_position[GB02STR18->module_type]
				= 0;
		counter++;
		if (counter >=
		 enc_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	return 0;
}

static int GB02FUNC1515(struct GB02STR7 *dst_dev,
	bi_list *list, bi_list_node *new_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	u32 hw_rdy_cmdbuf_num = 0;
	unsigned long flags = 0;
	int counter = 0;
	bi_list_node *curr_cmdbuf_node = NULL;
	struct GB02STR7 *dev;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	while (1) {
		dev = enc_vcmd_manager[GB02STR18->module_type][enc_data->vcmd_position[GB02STR18->module_type]];
		list = &dev->list_manager;
		//read executing cmdbuf address
		if (dev->hw_version_id <= GB02MAC186)
			hw_rdy_cmdbuf_num =
				GB02FUNC325((const void *)dev->hwregs,
				dev->reg_mirror, HWIF_VCMD_EXE_CMDBUF_COUNT);
		else {
			hw_rdy_cmdbuf_num =
				*(dev->vcmd_reg_mem_virtual_address +
				GB02MAC155);
			if (hw_rdy_cmdbuf_num != dev->sw_cmdbuf_rdy_num)
				hw_rdy_cmdbuf_num += 1;
		}
		spin_lock_irqsave(dev->spinlock, flags);
		curr_cmdbuf_node = list->tail;
		if (curr_cmdbuf_node == NULL) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			enc_data->vcmd_position[GB02STR18->module_type]++;
			if (enc_data->vcmd_position[GB02STR18->module_type] >=
				enc_data->vcmd_type_core_num[GB02STR18->module_type])
				enc_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return -1;
		}

		if (dev->sw_cmdbuf_rdy_num == hw_rdy_cmdbuf_num) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			enc_data->vcmd_position[GB02STR18->module_type]++;
			if (enc_data->vcmd_position[GB02STR18->module_type] >=
				enc_data->vcmd_type_core_num[GB02STR18->module_type])
				enc_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return -1;
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		enc_data->vcmd_position[GB02STR18->module_type]++;
		if (enc_data->vcmd_position[GB02STR18->module_type] >=
			enc_data->vcmd_type_core_num[GB02STR18->module_type])
			enc_data->vcmd_position[GB02STR18->module_type] = 0;
		counter++;
		if (counter >=
		 enc_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	return 0;
}

static bi_list_node *GB02FUNC1517(size_t  cmdbuf_addr,
	bi_list *list)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	size_t base_ddr_addr = 0;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	base_ddr_addr = enc_data->enc_base_ddr_addr;
	new_cmdbuf_node = list->head;
	while (1) {
		if (new_cmdbuf_node == NULL)
			return NULL;
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if (((GB02STR18->cmdbuf_bus_address - base_ddr_addr)
		 <= cmdbuf_addr) && (((GB02STR18->cmdbuf_bus_address
		  - base_ddr_addr + GB02STR18->cmdbuf_size)
		   > cmdbuf_addr)))
			return new_cmdbuf_node;
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
	return NULL;
}

static int GB02FUNC1520(struct GB02STR7 *dst_dev,
	bi_list *list, bi_list_node **enc_global_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	int counter = 0;
	size_t exe_cmdbuf_addr = 0;
	unsigned long flags = 0;
	bi_list_node *curr_cmdbuf_node = NULL;
	u32 cmdbuf_id = 0;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;
	struct GB02STR7 *dev;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	while (1) {
		dev = enc_vcmd_manager[GB02STR18->module_type][enc_data->vcmd_position[GB02STR18->module_type]];
		/* read executing cmdbuf address */
		if (dev->hw_version_id <= GB02MAC186) {
			exe_cmdbuf_addr =
			 VCMDGetAddrRegisterValue((const void *)dev->hwregs,
				dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR);
			list = &dev->list_manager;
			spin_lock_irqsave(dev->spinlock, flags);
			//get the executing cmdbuf node.
			curr_cmdbuf_node =
			 GB02FUNC1517(exe_cmdbuf_addr,
				 list);

			//calculate total execute time of this device
			dev->total_exe_time =
				GB02FUNC272(curr_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
		} else {
			if (GB02STR18->priority == GB02MAC150)
				cmdbuf_id =
				 *(dev->vcmd_reg_mem_virtual_address +
					GB02MAC143 + 1);
			else	/* GB02MAC151 */
				cmdbuf_id =
				 *(dev->vcmd_reg_mem_virtual_address +
					GB02MAC143);
			spin_lock_irqsave(dev->spinlock, flags);
			gb_printf(KERN_INFO, "%s:%d-----cmdbuf id =%x----\n",
				__func__, __LINE__, cmdbuf_id);
			if (cmdbuf_id >= GB02MAC159
			 || cmdbuf_id == 0) {
				gb_printf(KERN_ERR, "cmdbuf_id greater than the ceiling !!\n");
				spin_unlock_irqrestore(dev->spinlock, flags);
				return -1;
			}
			//get the executing cmdbuf node.
			curr_cmdbuf_node = enc_global_cmdbuf_node[cmdbuf_id];
			if (curr_cmdbuf_node == NULL) {
				list = &dev->list_manager;
				curr_cmdbuf_node = list->head;
				while (1) {
					if (curr_cmdbuf_node == NULL)
						break;
					cmdbuf_obj_temp =
						(struct GB02STR18 *)curr_cmdbuf_node->data;
					if (cmdbuf_obj_temp->cmdbuf_data_linked
						&& cmdbuf_obj_temp->cmdbuf_run_done == 0)
						break;
					curr_cmdbuf_node =
					 curr_cmdbuf_node->next;
				}
			}
			/* calculate total execute time of this device */
			dev->total_exe_time =
				GB02FUNC272(curr_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
		}
		enc_data->vcmd_position[GB02STR18->module_type]++;
		if (enc_data->vcmd_position[GB02STR18->module_type] >=
			enc_data->vcmd_type_core_num[GB02STR18->module_type])
			enc_data->vcmd_position[GB02STR18->module_type] = 0;
		counter++;
		if (counter >=
			enc_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	return 0;
}

static void GB02FUNC1524(struct GB02STR7 *dst_dev,
	bi_list *list, bi_list_node *new_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	int counter = 0;
	unsigned long flags = 0;
	u32 executing_time = 0xffffffff;
	struct GB02STR7 *smallest_dev = NULL;
	struct GB02STR7 *dev;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	while (1) {
		dev = enc_vcmd_manager[GB02STR18->module_type][enc_data->vcmd_position[GB02STR18->module_type]];
		if (dev->total_exe_time <= executing_time) {
			executing_time = dev->total_exe_time;
			smallest_dev = dev;
		}
		enc_data->vcmd_position[GB02STR18->module_type]++;
		if (enc_data->vcmd_position[GB02STR18->module_type] >=
			enc_data->vcmd_type_core_num[GB02STR18->module_type])
			enc_data->vcmd_position[GB02STR18->module_type] = 0;
		counter++;
		if (counter >=
		 enc_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}

	list = &smallest_dev->list_manager;
	spin_lock_irqsave(smallest_dev->spinlock, flags);
	GB02FUNC210(list, new_cmdbuf_node);
	spin_unlock_irqrestore(smallest_dev->spinlock, flags);
	GB02STR18->core_id = smallest_dev->core_id;

}

static int GB02FUNC1527(struct GB02STR7 *dev)
{
	return dev->working_state == GB02MAC145;
}

static int GB02FUNC1528(struct GB02STR7 *dst_dev,
	bi_list *list, bi_list_node *new_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	int counter = 0;
	unsigned long flags = 0;
	bi_list_node *curr_cmdbuf_node = NULL;
	u32 executing_time = 0xffffffff;
	struct GB02STR7 *smallest_dev = NULL;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;
	struct GB02STR7 *dev;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	while (1) {
		dev = enc_vcmd_manager[GB02STR18->module_type][enc_data->vcmd_position[GB02STR18->module_type]];
		if (dev->total_exe_time <= executing_time) {
			executing_time = dev->total_exe_time;
			smallest_dev = dev;
		}
		enc_data->vcmd_position[GB02STR18->module_type]++;
		if (enc_data->vcmd_position[GB02STR18->module_type] >=
			enc_data->vcmd_type_core_num[GB02STR18->module_type])
			enc_data->vcmd_position[GB02STR18->module_type] = 0;
		counter++;
		if (counter >=
		 enc_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}

	GB02FUNC321((const void *)smallest_dev->hwregs,
	 smallest_dev->reg_mirror, HWIF_VCMD_START_TRIGGER, 0);
	enc_data->software_triger_abort = 1;
	if (wait_event_interruptible(*smallest_dev->wait_abort_queue,
		GB02FUNC1527(smallest_dev))) {
		enc_data->software_triger_abort = 0;
		return -ERESTARTSYS;
	}
		enc_data->software_triger_abort = 0;
	spin_lock_irqsave(smallest_dev->spinlock, flags);
	curr_cmdbuf_node = smallest_dev->list_manager.head;
	while (1) {
		/*if list is empty or tail,insert to tail*/
		if (curr_cmdbuf_node == NULL)
			break;
		cmdbuf_obj_temp = (struct GB02STR18 *)curr_cmdbuf_node->data;
		if ((cmdbuf_obj_temp->priority == GB02MAC150)
			&& (cmdbuf_obj_temp->cmdbuf_run_done == 0))
			break;
		curr_cmdbuf_node = curr_cmdbuf_node->next;
	}
	GB02FUNC219(list, curr_cmdbuf_node, new_cmdbuf_node);
	GB02STR18->core_id = smallest_dev->core_id;
	spin_unlock_irqrestore(smallest_dev->spinlock, flags);
	return 0;
}

static int GB02FUNC1532(struct GB02STR7 *dst_dev,
	bi_list_node *new_cmdbuf_node, bi_list_node **enc_global_cmdbuf_node)
{
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list *list = NULL;
	int ret = -1;

	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;

	/* there is an empty vcmd to be used */
	ret = GB02FUNC1507(dst_dev, list, new_cmdbuf_node, GB02STR18);
	if (-1 == ret) {
		gb_printf(KERN_INFO, "%s-%d: return, ret = %d\n", __func__, __LINE__, ret);
		return 0;
	}

	ret = GB02FUNC1510(dst_dev, list, new_cmdbuf_node, GB02STR18);
	if (-1 == ret) {
		gb_printf(KERN_INFO, "%s-%d: return, ret = %d\n", __func__, __LINE__, ret);
		return 0;
	}

	ret =  GB02FUNC1515(dst_dev, list, new_cmdbuf_node, GB02STR18);
	if (-1 == ret) {
		gb_printf(KERN_INFO, "%s-%d: return, ret = %d\n", __func__, __LINE__, ret);
		return 0;
	}

	if (GB02STR18->priority == GB02MAC150) {
		/* calculate total execute time of all devices*/
		ret = GB02FUNC1520(dst_dev, list,
			enc_global_cmdbuf_node, GB02STR18);
		if (-1 == ret) {
			gb_printf(KERN_ERR, "%s-%d: error, ret = %d\n",
			 __func__, __LINE__, ret);
			return ret;
		}

		/* find the device with the least total_exe_time.*/
		GB02FUNC1524(dst_dev, list,
			new_cmdbuf_node, GB02STR18);
		return 0;
	}
	/* calculate total execute time of all devices*/
	ret = GB02FUNC1520(dst_dev, list,
		enc_global_cmdbuf_node, GB02STR18);
	if (-1 == ret) {
		gb_printf(KERN_ERR, "%s-%d: error, ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}

	/* find the smallest device. */
	ret = GB02FUNC1528(dst_dev, list,
		new_cmdbuf_node, GB02STR18);
	if (ret != 0) {
		gb_printf(KERN_ERR, "%s-%d: error, ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	return 0;
}

static void GB02FUNC1534(struct GB02STR7 *dev,
	bi_list_node *new_cmdbuf_node)
{
	u32 *jmp_addr = NULL;
	u32 operation_code;
	struct GB02STR18 *GB02STR18;

	if (new_cmdbuf_node == NULL)
		return;
	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
	if ((GB02STR18->cmdbuf_data_linked == 0)) {
		(dev->sw_cmdbuf_rdy_num)++;
		GB02STR18->cmdbuf_data_linked = 1;
		dev->duration_without_int = 0;
		if (GB02STR18->has_end_cmdbuf == 0) {
			if (GB02STR18->no_normal_int_cmdbuf == 1) {
				dev->duration_without_int =
					GB02STR18->executing_time;
				//maybe nop is modified, so write back.
				if (dev->duration_without_int >=
					GB02MAC154) {
					jmp_addr =
					 GB02STR18->cmdbuf_virtual_address
					  + (GB02STR18->cmdbuf_size / 4);
					operation_code = *(jmp_addr - 4);
					operation_code = GB02MAC184
						| operation_code;
					*(jmp_addr - 4) = operation_code;
					dev->duration_without_int = 0;
				}
			}
		}
	}
}

static void GB02FUNC1537(struct GB02STR7 *dev,
	struct GB02STR18 *GB02STR18, struct GB02STR18 *next_cmdbuf_obj)
{
	u32 *jmp_addr = NULL;
	u32 operation_code;
	size_t base_ddr_addr = 0;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	base_ddr_addr = enc_data->enc_base_ddr_addr;

	gb_printf(KERN_INFO, "%s: Link cmdbuf %d to cmdbuf %d", __func__,
		GB02STR18->cmdbuf_id, next_cmdbuf_obj->cmdbuf_id);
	jmp_addr = GB02STR18->cmdbuf_virtual_address +
		(GB02STR18->cmdbuf_size / 4);
	if (dev->hw_version_id > GB02MAC186)
		*(jmp_addr - 1) =
		 next_cmdbuf_obj->cmdbuf_id; /* set next cmdbuf id */

	if (sizeof(size_t) == 8)
		*(jmp_addr - 2) =
		 (u32)((u64)(next_cmdbuf_obj->cmdbuf_bus_address
			- base_ddr_addr) >> 32);
	else
		*(jmp_addr - 2) = 0;

	*(jmp_addr - 3) =
		(u32)(next_cmdbuf_obj->cmdbuf_bus_address - base_ddr_addr);
	operation_code = *(jmp_addr - 4);
	operation_code >>= 16;
	operation_code <<= 16;
	*(jmp_addr - 4) = (u32)(operation_code | GB02MAC185
		| ((next_cmdbuf_obj->cmdbuf_size + 7) / 8));
	next_cmdbuf_obj->cmdbuf_data_linked = 1;
	(dev->sw_cmdbuf_rdy_num)++;
}

static void GB02FUNC1540(struct GB02STR7 *dev,
	struct GB02STR18 *next_cmdbuf_obj)
{
	u32 *jmp_addr = NULL;
	u32 operation_code;

	if (next_cmdbuf_obj->has_end_cmdbuf == 0) {
		if (next_cmdbuf_obj->no_normal_int_cmdbuf == 1) {
			dev->duration_without_int +=
			 next_cmdbuf_obj->executing_time;

			if (dev->duration_without_int >=
			 GB02MAC154) {
				jmp_addr =
				 next_cmdbuf_obj->cmdbuf_virtual_address +
					(next_cmdbuf_obj->cmdbuf_size / 4);
				operation_code = *(jmp_addr - 4);
				operation_code = GB02MAC184 | operation_code;
				*(jmp_addr - 4) = operation_code;
				dev->duration_without_int = 0;
			}
		}
	} else
		dev->duration_without_int = 0;
}

void GB02FUNC1542(struct GB02STR7 *dev,
	bi_list_node *last_linked_cmdbuf_node)
{
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *next_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR18 *next_cmdbuf_obj = NULL;

	new_cmdbuf_node = last_linked_cmdbuf_node;
	/* for the first cmdbuf. */
	GB02FUNC1534(dev, new_cmdbuf_node);

	while (1) {
		if (new_cmdbuf_node == NULL)
			break;
		if (new_cmdbuf_node->next == NULL)
			break;
		next_cmdbuf_node = new_cmdbuf_node->next;
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		next_cmdbuf_obj = (struct GB02STR18 *)next_cmdbuf_node->data;
		if (GB02STR18->has_end_cmdbuf == 0
			&& !next_cmdbuf_obj->cmdbuf_run_done) {
			//need to link, current cmdbuf link to next cmdbuf
			GB02FUNC1537(dev, GB02STR18,
			 next_cmdbuf_obj);

			//modify nop code of next cmdbuf
			GB02FUNC1540(dev, next_cmdbuf_obj);
		}
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
}

static void GB02FUNC1545(struct GB02STR7 **device)
{
#ifdef HANTROVCMD_ENABLE_IP_SUPPORT
	u32 i = 0;
	u64 address = 0;
	u32 mirror_index, register_index, register_value;
	u32 write_command = 0;

	if (!device)
		return;
	struct GB02STR7 *dev = *device;

	mirror_index = GB02MAC2;
	write_command = GB02MAC174 | (1 << 26) | (1 << 16);
#ifdef HANTROAXIFE_SUPPORT
	//enable AXIFE by VCMD
	for (i = 0; i < 2; i++) {
		if (dev->enc_vcmd_core_cfg.submodule_axife_addr[i] != 0xffff) {
			register_index = GB02MAC96;
			register_value = 0x02;
			dev->reg_mirror[mirror_index++] = write_command
				| (dev->enc_vcmd_core_cfg.submodule_axife_addr[i] +
				register_index);
			dev->reg_mirror[mirror_index++] = register_value;

			register_index = GB02MAC98;
			register_value = 0x00;
			dev->reg_mirror[mirror_index++] = write_command
				| (dev->enc_vcmd_core_cfg.submodule_axife_addr[i] +
				register_index);
			dev->reg_mirror[mirror_index++] = register_value;
		}
	}
#endif
#ifdef HANTROMMU_SUPPORT
	//enable MMU by VCMD
	address = GetMMUAddress();
	gb_printf(KERN_INFO, "%s: GetMMUAddress address = 0x%llx\n", __func__, address);
	for (i = 0; i < 2; i++) {
		if (dev->enc_vcmd_core_cfg.submodule_MMU_addr[i] != 0xffff) {
			register_index = GB02MAC172;
			register_value = address;
			dev->reg_mirror[mirror_index++] = write_command
				| (dev->enc_vcmd_core_cfg.submodule_MMU_addr[i] +
				register_index);
			dev->reg_mirror[mirror_index++] = register_value;

			register_index = GB02MAC170;
			register_value = 0x10000;
			dev->reg_mirror[mirror_index++] = write_command
				| (dev->enc_vcmd_core_cfg.submodule_MMU_addr[i] +
				register_index);
			dev->reg_mirror[mirror_index++] = register_value;

			register_index = GB02MAC170;
			register_value = 0x00000;
			dev->reg_mirror[mirror_index++] = write_command
				| (dev->enc_vcmd_core_cfg.submodule_MMU_addr[i] +
				register_index);
			dev->reg_mirror[mirror_index++] = register_value;

			register_index = GB02MAC171;
			register_value = 1;
			dev->reg_mirror[mirror_index++] = write_command
				| (dev->enc_vcmd_core_cfg.submodule_MMU_addr[i] +
				register_index);
			dev->reg_mirror[mirror_index++] = register_value;
		}
	}
#endif
	//END command
	dev->reg_mirror[mirror_index++] = GB02MAC175;
	dev->reg_mirror[mirror_index] = 0x00;

	for (i = 0; i < mirror_index - GB02MAC2; i++) {
		register_index = (i + GB02MAC2) * 4;
		GB02FUNC318((const void *)dev->hwregs, register_index,
			dev->reg_mirror[i + GB02MAC2]);
	}
#endif
}

void GB02FUNC1554(struct GB02STR7 *dev,
	bi_list_node *first_linked_cmdbuf_node)
{
	struct GB02STR18 *GB02STR18 = NULL;
	size_t base_ddr_addr = 0;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	base_ddr_addr = enc_data->enc_base_ddr_addr;

	if (dev->working_state == GB02MAC145) {
		if ((first_linked_cmdbuf_node != NULL)
		 && dev->sw_cmdbuf_rdy_num) {
			GB02STR18 =
			 (struct GB02STR18 *)first_linked_cmdbuf_node->data;
			/* 0x40 */
			#ifdef HANTROVCMD_ENABLE_IP_SUPPORT
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_INIT_MODE, 1);
			#endif
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_AXI_CLK_GATE_DISABLE, 0);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_MASTER_OUT_CLK_GATE_DISABLE, 1);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_CORE_CLK_GATE_DISABLE, 0);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_ABORT_MODE, 0);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_RESET_CORE, 0);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_RESET_ALL, 0);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_START_TRIGGER, 0);
			/* 0x48 */
			if (dev->hw_version_id <= GB02MAC186)
				GB02FUNC342(dev->reg_mirror,
					HWIF_VCMD_IRQ_INTCMD_EN, 0xffff);
			else {
				GB02FUNC342(dev->reg_mirror,
					HWIF_VCMD_IRQ_JMPP_EN, 1);
				GB02FUNC342(dev->reg_mirror,
					HWIF_VCMD_IRQ_JMPD_EN, 1);
			}

			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_IRQ_RESET_EN, 1);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_IRQ_ABORT_EN, 1);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_IRQ_CMDERR_EN, 1);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_IRQ_TIMEOUT_EN, 1);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_IRQ_BUSERR_EN, 1);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_IRQ_ENDCMD_EN, 1);
			//0x4c
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_TIMEOUT_EN, 1);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_TIMEOUT_CYCLES, 0x1dcd6500);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_EXECUTING_CMD_ADDR,
				(u32)(GB02STR18->cmdbuf_bus_address));
			gb_printf(KERN_INFO, "--%s:%d--cmdbuf bus addr = %lx-\n",
				__func__, __LINE__,
				GB02STR18->cmdbuf_bus_address);
			if (sizeof(size_t) == 8)
				GB02FUNC342(dev->reg_mirror,
				 HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
				(u32)((u64)(GB02STR18->cmdbuf_bus_address) >> 32));
			else
				GB02FUNC342(dev->reg_mirror,
					HWIF_VCMD_EXECUTING_CMD_ADDR_MSB, 0);

			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_EXE_CMDBUF_LENGTH,
				(u32)((GB02STR18->cmdbuf_size + 7) / 8));
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_RDY_CMDBUF_COUNT,
				 dev->sw_cmdbuf_rdy_num);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_MAX_BURST_LEN, 0x10);
			if (dev->hw_version_id > GB02MAC186)
				GB02FUNC321((const void *)dev->hwregs,
					dev->reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID,
					(u32)GB02STR18->cmdbuf_id);

			GB02FUNC318((const void *)dev->hwregs, 0x44,
				GB02FUNC313((const void *)dev->hwregs, 0x44));
			GB02FUNC318((const void *)dev->hwregs, 0x40,
				dev->reg_mirror[0x40 / 4]);
			GB02FUNC318((const void *)dev->hwregs, 0x48,
				dev->reg_mirror[0x48 / 4]);
			GB02FUNC318((const void *)dev->hwregs, 0x4c,
				dev->reg_mirror[0x4c / 4]);
			GB02FUNC318((const void *)dev->hwregs, 0x50,
				dev->reg_mirror[0x50 / 4]);
			GB02FUNC318((const void *)dev->hwregs, 0x54,
				dev->reg_mirror[0x54 / 4]);
			GB02FUNC318((const void *)dev->hwregs, 0x58,
				dev->reg_mirror[0x58 / 4]);
			GB02FUNC318((const void *)dev->hwregs, 0x5c,
				dev->reg_mirror[0x5c / 4]);
			GB02FUNC318((const void *)dev->hwregs, 0x60,
				dev->reg_mirror[0x60 / 4]);
			GB02FUNC318((const void *)dev->hwregs, 0x64,
			 0xffffffff);
			dev->working_state = GB02MAC146;

			GB02FUNC1545(&dev);

			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_MASTER_OUT_CLK_GATE_DISABLE, 0);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_START_TRIGGER, 1);
			GB02FUNC318((const void *)dev->hwregs, 0x40,
				dev->reg_mirror[0x40 / 4]);
		}
	}
}

int GB02FUNC1566(struct drm_file *filp,
	int cmdbuf_id, int cmdbuf_size, int *core_id)
{
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *last_cmdbuf_node;
	u32 *jmp_addr = NULL;
	u32 opCode, opcode_bak;
	u32 tempOpcode;
	u32 record_last_cmdbuf_rdy_num;
	struct GB02STR7 *dev = NULL;
	unsigned long flags;
	int return_value;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	new_cmdbuf_node = enc_data->enc_global_cmdbuf_node[cmdbuf_id];
	if (new_cmdbuf_node == NULL) {	//should not happen
		gb_printf(KERN_ERR, "%s: ERROR new_cmdbuf_node = NULL--cmdbud id =%d !!\n",
			__func__, cmdbuf_id);
		return -1;
	}
	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
	if (GB02STR18->filp != filp) { //should not happen
		gb_printf(KERN_ERR, "%s: ERROR GB02STR18->filp != filp !!\n", __func__);
		return -1;
	}
	GB02STR18->cmdbuf_data_loaded = 1;
	GB02STR18->cmdbuf_size = cmdbuf_size;
	//test nop and end opcode, then assign value.
	GB02STR18->has_end_cmdbuf = 0; /* 0: has jmp opcode, 1 has end code */
	/* 0: interrupt when JMP, 1 not interrupt when JMP */
	GB02STR18->no_normal_int_cmdbuf = 0;
	jmp_addr =
		GB02STR18->cmdbuf_virtual_address +
		 (GB02STR18->cmdbuf_size / 4);
	opcode_bak = opCode = tempOpcode = *(jmp_addr - 4);
	gb_printf(KERN_INFO, "%s:%d-cmdbuf vaddr= %llx-cmdbufsize=%x,opCode=%x\n",
	__func__, __LINE__,
	(long long)GB02STR18->cmdbuf_virtual_address,
	GB02STR18->cmdbuf_size, opCode);
	opCode >>= 27;
	opCode <<= 27;
	if (opCode == GB02MAC179) {
		//jmp
		opCode = tempOpcode;
		opCode &= 0x02000000;
		if (opCode == GB02MAC184)
			GB02STR18->no_normal_int_cmdbuf = 0;
		else
			GB02STR18->no_normal_int_cmdbuf = 1;
	} else {
		gb_printf(KERN_ERR, "%s-%d: error, so quit!!\n", __func__, __LINE__);
		mdelay(200);
		return -1;
	}

	if (down_interruptible(&(enc_data->enc_vcmd_reserve_cmdbuf_sem[GB02STR18->module_type])))
		return -ERESTARTSYS;

	dev = &hantrovcmd_enc_data[GB02STR18->core_id]; /* 0 or 1 */

	return_value = GB02FUNC1532(dev, new_cmdbuf_node,
	 enc_data->enc_global_cmdbuf_node);
	if (return_value) {
		up(&(enc_data->enc_vcmd_reserve_cmdbuf_sem[GB02STR18->module_type]));
		return return_value;
	}
	//input_para->core_id = GB02STR18->core_id;
	*core_id = GB02STR18->core_id;
	gb_printf(KERN_INFO, "%s: Allocate cmd buffer [%d]\n",
		__func__, cmdbuf_id);
	//set ddr address for vcmd registers copy.
	if (dev->hw_version_id > GB02MAC186) {
		jmp_addr = GB02STR18->cmdbuf_virtual_address;
		if (sizeof(size_t) == 8)
			*(jmp_addr + 2) =
			 (u32)((u64)(dev->vcmd_reg_mem_bus_address
				+ (GB02MAC143 + 1) * 4) >> 32);
		else
			*(jmp_addr + 2) = 0;
		*(jmp_addr + 1) = (u32)((dev->vcmd_reg_mem_bus_address
			+ (GB02MAC143 + 1) * 4));

		jmp_addr = GB02STR18->cmdbuf_virtual_address +
			(GB02STR18->cmdbuf_size / 4);

		if (sizeof(size_t) == 8)
			*(jmp_addr - 6) =
				(u32)((u64)dev->vcmd_reg_mem_bus_address >> 32);
		else
			*(jmp_addr - 6) = 0;

		*(jmp_addr - 7) = (u32)(dev->vcmd_reg_mem_bus_address);
	}

	/* start to link and/or run */
	spin_lock_irqsave(dev->spinlock, flags);
	last_cmdbuf_node = GB02FUNC260(new_cmdbuf_node);
	record_last_cmdbuf_rdy_num = dev->sw_cmdbuf_rdy_num;
	GB02FUNC1542(dev, last_cmdbuf_node);
	tempOpcode = *(jmp_addr - 4);
	*(jmp_addr - 4) = opcode_bak | (tempOpcode & 0x2000000);
	if (dev->working_state == GB02MAC145) {
		if (filp == NULL)
			((struct GB02STR18 *)last_cmdbuf_node->data)->cmdbuf_run_done = 0;
		while (last_cmdbuf_node &&
			((struct GB02STR18 *)last_cmdbuf_node->data)->cmdbuf_run_done) {
			gb_printf(KERN_INFO, "%s:%d-----while---\n", __func__, __LINE__);
			last_cmdbuf_node = last_cmdbuf_node->next;
		}

		if (last_cmdbuf_node && last_cmdbuf_node->data) {
			gb_printf(KERN_INFO, "%s:%d---vcmd start for cmdbuf id %d,\
				cmdbuf_run_done = %d\n", __func__, __LINE__,
			 	((struct GB02STR18 *)last_cmdbuf_node->data)->cmdbuf_id,
			  	((struct GB02STR18 *)last_cmdbuf_node->data)->cmdbuf_run_done);
		}
		GB02FUNC1554(dev, last_cmdbuf_node);
	} else {
		/* just update cmdbuf ready number */
		if (record_last_cmdbuf_rdy_num != dev->sw_cmdbuf_rdy_num)
			GB02FUNC321((const void *)dev->hwregs,
				dev->reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT,
				dev->sw_cmdbuf_rdy_num);
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	up(&(enc_data->enc_vcmd_reserve_cmdbuf_sem[GB02STR18->module_type]));

	return 0;
}

static int GB02FUNC1578(struct GB02STR7 *dev,
	struct GB02STR18 *GB02STR18, u32 *irq_status_ret)
{
	int rdy = 0;
	unsigned long flags;

	spin_lock_irqsave(dev->spinlock, flags);
	if (GB02STR18->cmdbuf_run_done) {
		rdy = 1;
		*irq_status_ret = GB02STR18->executing_status;
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	return rdy;
}

unsigned int GB02FUNC1582(struct drm_file *filp, u32 cmdbuf_id,
	u32 *irq_status_ret)
{
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR7 *dev = NULL;
	bi_list_node **global_cmdbuf_node = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	global_cmdbuf_node = &enc_data->enc_global_cmdbuf_node[0];
	gb_printf(KERN_INFO, "%s: GB02FUNC868\n", __func__);
	new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
	if (new_cmdbuf_node == NULL) {
		/* should not happen */
		gb_printf(KERN_ERR, "%s: ERROR new_cmdbuf_node == NULL !!\n", __func__);
		return -1;
	}
	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
	if (GB02STR18->filp != filp) {
		/* should not happen */
		gb_printf(KERN_ERR, "%s: ERROR GB02STR18->filp != filp\n", __func__);
		return -1;
	}
	dev = &hantrovcmd_enc_data[GB02STR18->core_id];
	if (wait_event_interruptible(*dev->wait_queue,
		GB02FUNC1578(dev, GB02STR18, irq_status_ret))) {
		gb_printf(KERN_ERR, "%s: wait_event_interruptible error\n", __func__);
		/* abort the vcmd */
		return -ERESTARTSYS;
	}
	return 0;
}

static void GB02FUNC1587(u16 cmdbuf_id)
{
	unsigned long flags;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	spin_lock_irqsave(&enc_vcmd_cmdbuf_alloc_lock, flags);
	enc_data->enc_cmdbuf_used[cmdbuf_id] = 0;
	enc_data->enc_cmdbuf_used_residual += 1;
	spin_unlock_irqrestore(&enc_vcmd_cmdbuf_alloc_lock, flags);
	wake_up_interruptible_all(&enc_vcmd_cmdbuf_memory_wait);
}

/* cmdbuf pool management */
static int GB02FUNC1588(struct GB02STR14 *new_cmdbuf_addr,
	struct GB02STR14 *new_status_cmdbuf_addr,
	bi_list_node **global_cmdbuf_node, struct GB02STR14 *enc_vcmd_mem_pool,
	struct GB02STR3 vcmd_buff, struct drm_gb_cmdbuf_resv *input_para)
{
	unsigned long flags;
	int cmdbuf_index, status_cmdbuf_index;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	spin_lock_irqsave(&enc_vcmd_cmdbuf_alloc_lock, flags);
	if (enc_data->enc_cmdbuf_used_residual == 0) {
		spin_unlock_irqrestore(&enc_vcmd_cmdbuf_alloc_lock, flags);
		gb_printf(KERN_ERR, "%s:%d--no empty cmdbuf--\n",
			__func__, __LINE__);
		return 0;	//no empty cmdbuf
	}
	//there is one cmdbuf at least
	while (1) {
		if (enc_data->enc_cmdbuf_used[enc_data->enc_cmdbuf_used_pos] == 0
			&& (global_cmdbuf_node[enc_data->enc_cmdbuf_used_pos] == NULL)) {
			enc_data->enc_cmdbuf_used[enc_data->enc_cmdbuf_used_pos] = 1;
			enc_data->enc_cmdbuf_used_residual -= 1;
			cmdbuf_index = (enc_data->enc_cmdbuf_used_pos - 1) * 2;
			status_cmdbuf_index = (enc_data->enc_cmdbuf_used_pos - 1) * 2 + 1;
			new_cmdbuf_addr->virtual_address =
				vcmd_buff.vir_buff + cmdbuf_index * GB02MAC156;
			new_cmdbuf_addr->bus_address =
				vcmd_buff.offset + cmdbuf_index * GB02MAC156;
			new_cmdbuf_addr->size = GB02MAC156;
			new_cmdbuf_addr->cmdbuf_id = enc_data->enc_cmdbuf_used_pos;

			new_status_cmdbuf_addr->virtual_address =
			 	vcmd_buff.vir_buff + status_cmdbuf_index * GB02MAC156;
			new_status_cmdbuf_addr->bus_address =
			 	vcmd_buff.offset + status_cmdbuf_index * GB02MAC156;
			new_status_cmdbuf_addr->size = GB02MAC156;
			new_status_cmdbuf_addr->cmdbuf_id =
			 	enc_data->enc_cmdbuf_used_pos;
			(enc_data->enc_cmdbuf_used_pos)++;
			if (enc_data->enc_cmdbuf_used_pos >= GB02MAC159)
				enc_data->enc_cmdbuf_used_pos = 0;

			input_para->cmdbuf_offset = cmdbuf_index * GB02MAC156;
			input_para->status_offset = status_cmdbuf_index * GB02MAC156;
			spin_unlock_irqrestore(&enc_vcmd_cmdbuf_alloc_lock, flags);
			return 1;
		}
		(enc_data->enc_cmdbuf_used_pos)++;
		if (enc_data->enc_cmdbuf_used_pos >= GB02MAC159)
			enc_data->enc_cmdbuf_used_pos = 0;
	}
	return 0;
}

static bi_list_node *GB02FUNC1591(bi_list_node **global_cmdbuf_node,
	struct GB02STR14 *enc_vcmd_mem_pool, struct GB02STR3 vcmd_buff,
	struct drm_gb_cmdbuf_resv *input_para)
{
	bi_list_node *current_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR14  new_cmdbuf_addr;
	struct GB02STR14  new_status_cmdbuf_addr;
	int ret_allocate = -1;

	memset((void *)&new_cmdbuf_addr, 0, sizeof(struct GB02STR14));
	memset((void *)&new_status_cmdbuf_addr, 0, sizeof(struct GB02STR14));
	ret_allocate =
	 GB02FUNC1588(&new_cmdbuf_addr, &new_status_cmdbuf_addr,
	  global_cmdbuf_node, enc_vcmd_mem_pool, vcmd_buff, input_para);
	gb_printf(KERN_INFO, "%s:%d--enter--wait allocate event--\n",
			__func__, __LINE__);
	if (wait_event_interruptible(enc_vcmd_cmdbuf_memory_wait, ret_allocate)) {
		gb_printf(KERN_ERR, "%s:%d--wait event interruptible--fail--\n",
			__func__, __LINE__);
		return NULL;
	}
	GB02STR18 = GB02FUNC249();
	if (GB02STR18 == NULL) {
		gb_printf(KERN_ERR, "%s: GB02FUNC249 fail!\n", __func__);
		GB02FUNC1587(new_cmdbuf_addr.cmdbuf_id);
		return NULL;
	}
	GB02STR18->cmdbuf_bus_address = new_cmdbuf_addr.bus_address;
	GB02STR18->mmu_cmdbuf_bus_address = new_cmdbuf_addr.mmu_bus_address;
	GB02STR18->cmdbuf_virtual_address = new_cmdbuf_addr.virtual_address;
	GB02STR18->cmdbuf_size = new_cmdbuf_addr.size;
	GB02STR18->cmdbuf_id = new_cmdbuf_addr.cmdbuf_id;
	GB02STR18->status_bus_address = new_status_cmdbuf_addr.bus_address;
	GB02STR18->mmu_status_bus_address =
		new_status_cmdbuf_addr.mmu_bus_address;
	GB02STR18->status_virtual_address =
		new_status_cmdbuf_addr.virtual_address;
	GB02STR18->status_size = new_status_cmdbuf_addr.size;
	current_node = GB02FUNC215();
	if (current_node == NULL) {
		gb_printf(KERN_ERR, "%s: GB02FUNC215 fail!\n", __func__);
		GB02FUNC1587(new_cmdbuf_addr.cmdbuf_id);
		GB02FUNC254(GB02STR18);
		return NULL;
	}
	gb_printf(KERN_INFO, "%s:%d-cmdbuf paddr=%llx-vaddr=%llx,\
	status paddr=%lx, vaddr=%llx---\n",
	__func__, __LINE__,
	(long long)GB02STR18->cmdbuf_bus_address,
	(long long)GB02STR18->cmdbuf_virtual_address,
	GB02STR18->status_bus_address,
	(long long)GB02STR18->status_virtual_address);

	current_node->data = (void *)GB02STR18;
	current_node->next = NULL;
	current_node->previous = NULL;
	return current_node;
}

static long GB02FUNC1597(struct drm_file *filp,
	struct drm_gb_cmdbuf_resv *input_para,
	bi_list *global_process_manager,
	bi_list_node **global_cmdbuf_node,
	struct GB02STR14 *enc_vcmd_mem_pool,
	struct GB02STR3 vcmd_buff)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list_node *process_manager_node = NULL;
	struct GB02STR17 *GB02STR17 = NULL;
	unsigned long flags, flags1;

	input_para->cmdbuf_id = 0;
	if (input_para->cmdbuf_size > 2 *GB02MAC156) {
		gb_printf(KERN_ERR, "%s:%d--cmdbuf size=%x  to big-\n",
		__func__, __LINE__, input_para->cmdbuf_size);
		return -1;
	}
	spin_lock_irqsave(&enc_vcmd_process_manager_lock, flags);
	process_manager_node = global_process_manager->head;
	while (1) {
		if (process_manager_node == NULL) {
			//should not happen
			gb_printf(KERN_ERR, "%s: ERROR process_manager_node !!\n", __func__);
			spin_unlock_irqrestore(&enc_vcmd_process_manager_lock,
			 flags);
			return -1;
		}
		GB02STR17 =
			(struct GB02STR17 *)process_manager_node->data;
		if (filp == GB02STR17->filp) {
			spin_lock_irqsave(&GB02STR17->spinlock,
				flags1);
			GB02STR17->total_exe_time +=
				input_para->executing_time;
			spin_unlock_irqrestore(&GB02STR17->spinlock,
				flags1);
			break;
		}
		process_manager_node = process_manager_node->next;
	}
	spin_unlock_irqrestore(&enc_vcmd_process_manager_lock, flags);
	gb_printf(KERN_INFO, "%s:%d----total exe time = %llx---\n",
		__func__, __LINE__,
		GB02STR17->total_exe_time);
	if (wait_event_interruptible(GB02STR17->wait_queue,
		GB02FUNC256(GB02STR17))) {
		gb_printf(KERN_ERR, "%s:%d--wait process resource rdy fail\n",
			__func__, __LINE__);
		return -1;
	}
	new_cmdbuf_node = GB02FUNC1591(global_cmdbuf_node,
		enc_vcmd_mem_pool, vcmd_buff, input_para);
	if (new_cmdbuf_node == NULL) {
		gb_printf(KERN_ERR, "%s:%d--create cmdbuf node fail---\n",
			__func__, __LINE__);
		return -1;
	}

	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
	GB02STR18->module_type = input_para->module_type;
	GB02STR18->priority = input_para->priority;
	GB02STR18->executing_time = input_para->executing_time;
	GB02STR18->cmdbuf_size = GB02MAC156;
	input_para->cmdbuf_size = GB02MAC156;
	GB02STR18->filp = filp;
	GB02STR18->GB02STR17 = GB02STR17;

	input_para->cmdbuf_id = GB02STR18->cmdbuf_id;
	global_cmdbuf_node[input_para->cmdbuf_id] = new_cmdbuf_node;
	return 0;
}

static void GB02FUNC1600(u32 main_module_type,
	struct GB02STR7 *hantrovcmd_enc_data)
{
	int ret;
	int core_id = 0;
	struct drm_gb_cmdbuf_resv input_para;
	u32 irq_status_ret = 0;
	u32 *status_base_virt_addr;
	struct GB02STR14 *vcmd_status_buf_mem_pool;
	u16 submodule_main_addr;
	struct GB02STR186 *enc_data = NULL;
	enc_data = GB02FUNC1639();
	vcmd_status_buf_mem_pool = &enc_data->enc_vcmd_mem_pool[1];
	input_para.executing_time = 0;
	input_para.priority = GB02MAC150;
	input_para.module_type = main_module_type;
	input_para.cmdbuf_size = 0;
	submodule_main_addr =
	 enc_vcmd_manager[input_para.module_type][0]->enc_vcmd_core_cfg.submodule_main_addr;
	ret = GB02FUNC1597(NULL, &input_para,
	 &enc_data->enc_global_process_manager,
	  enc_data->enc_global_cmdbuf_node,
	   enc_data->enc_vcmd_mem_pool, enc_data->vcmd_buff);
	enc_vcmd_manager[main_module_type][0]->status_cmdbuf_id =
		input_para.cmdbuf_id;
	GB02FUNC1497(&input_para, enc_data->enc_vcmd_mem_pool,
		       enc_data->enc_global_cmdbuf_node);
	GB02FUNC1566(NULL, input_para.cmdbuf_id, input_para.cmdbuf_size, &core_id);
	if (enc_data->hw_info.hw_id != 0)
		return;
	GB02FUNC1582(NULL, input_para.cmdbuf_id,
		&irq_status_ret);
	status_base_virt_addr = enc_data->vcmd_buff.vir_buff +
		input_para.cmdbuf_id * GB02MAC156 +
		(submodule_main_addr / 2 + 0);

	enc_data->hw_info.hw_id = *status_base_virt_addr;
	enc_data->hw_info.cfg1val = *(status_base_virt_addr + 80);
	enc_data->hw_info.cfg2val = *(status_base_virt_addr + 214);
	enc_data->hw_info.cfg3val = *(status_base_virt_addr + 226);
	enc_data->hw_info.cfg4val = *(status_base_virt_addr + 287);
	enc_data->hw_info.cfg5val = *(status_base_virt_addr + 430);
	enc_data->hw_info.cfgaxi = *(status_base_virt_addr + 319);
	gb_printf(KERN_INFO, "%s: enc hwid:0x%x\n", __func__, enc_data->hw_info.hw_id);
	gb_printf(KERN_INFO, "%s: enc cfg1val 0x%x\n", __func__, enc_data->hw_info.cfg1val);
	gb_printf(KERN_INFO, "%s: enc cfg2val 0x%x\n", __func__, enc_data->hw_info.cfg2val);
	gb_printf(KERN_INFO, "%s: enc cfg3val 0x%x\n", __func__, enc_data->hw_info.cfg3val);
	gb_printf(KERN_INFO, "%s: enc cfg4val 0x%x\n", __func__, enc_data->hw_info.cfg4val);
	gb_printf(KERN_INFO, "%s: enc cfg5val 0x%x\n", __func__, enc_data->hw_info.cfg5val);
	gb_printf(KERN_INFO, "%s: enc cfgaxi 0x%x\n", __func__, enc_data->hw_info.cfgaxi);

	gb_printf(KERN_INFO, "%s: enc reg 0:0x%x\n", __func__, *status_base_virt_addr);
	gb_printf(KERN_INFO, "%s: reg 80:0x%x\n", __func__, *(status_base_virt_addr + 80));
	gb_printf(KERN_INFO, "%s: reg 214:0x%x\n", __func__,
	 *(status_base_virt_addr + 214));
	gb_printf(KERN_INFO, "%s: reg 226:0x%x\n", __func__,
	 *(status_base_virt_addr + 226));
	gb_printf(KERN_INFO, "%s: reg 287:0x%x\n", __func__,
	 *(status_base_virt_addr + 287));
}


static int GB02FUNC1603(int total_vcmd_core_num, u64 enc_offset)
{
	int i = 0;

	for (i = 0; i < total_vcmd_core_num; i++)
		enc_vcmd_core_array[i].vcmd_base_addr =
		 enc_offset + enc_vcmd_core_array[i].vcmd_base_addr;

	return 0;
}

static int GB02FUNC1604(struct GB02STR7 *hantrovcmd_enc_data,
	int total_vcmd_core_num, u8 *vpu_vcmd_base)
{
	u32 hwid = 0;
	int i;
	u32 found_hw = 0, vcmd_iosize;
	unsigned long vcmd_base_addr;

	gb_printf(KERN_INFO, "%s: total_vcmd_core_num = %d\n",
	 __func__, total_vcmd_core_num);
	for (i = 0; i < total_vcmd_core_num; i++) {
		hantrovcmd_enc_data[i].hwregs = NULL;
		vcmd_base_addr =
		 hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_base_addr;
		vcmd_iosize =
		 hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_iosize;
		hantrovcmd_enc_data[i].hwregs = vpu_vcmd_base;
		if (hantrovcmd_enc_data[i].hwregs == NULL) {
			gb_printf(KERN_ERR, "%s: failed to ioremap HW regs\n", __func__);
			continue;
		}
		gb_printf(KERN_INFO, "%s: hantroenc_data[%d].hwregs=0x%p, hwid=0x%08x\n",
		 __func__, i, hantrovcmd_enc_data[i].hwregs, hwid);

		/*read hwid and check validness and store it*/
		hwid = (u32)ioread32((void *)hantrovcmd_enc_data[i].hwregs);
		hantrovcmd_enc_data[i].hw_version_id = hwid;
		gb_printf(KERN_INFO, "%s: hantroenc_data[%d].hwregs=0x%p, hwid=0x%08x\n",
			__func__, i, hantrovcmd_enc_data[i].hwregs, hwid);

		/* check for vcmd HW ID */
		if (((hwid >> 16) & 0xFFFF) != GB02MAC153) {
			gb_printf(KERN_ERR, "%s: err HW not found at 0x%lx\n", __func__,
				hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_base_addr);
			hantrovcmd_enc_data[i].hwregs = NULL;
			continue;
		}

		found_hw = 1;
	}

	if (found_hw == 0) {
		gb_printf(KERN_ERR, "%s NO ANY HW found!!\n", __func__);
		return -1;
	}
	gb_printf(KERN_INFO, "%s-%d: enc vcmd reserve finish!!!\n", __func__, __LINE__);
	return 0;
}

static void GB02FUNC1606(struct GB02STR7 *dev,
	int total_vcmd_core_num)
{
	int i, n;
	u32 result;

	for (n = 0; n < total_vcmd_core_num; n++) {
		if (dev[n].hwregs != NULL) {
			//disable interrupt at first
			GB02FUNC318((const void *)dev[n].hwregs,
				GB02MAC515, 0x0000);
			//reset all
			GB02FUNC318((const void *)dev[n].hwregs,
				GB02MAC513, 0x0002);
			//read status register
			result = GB02FUNC313((const void *)dev[n].hwregs,
				GB02MAC514);
			//clean status register
			GB02FUNC318((const void *)dev[n].hwregs,
				GB02MAC514, result);
			for (i = GB02MAC513;
				i < dev[n].enc_vcmd_core_cfg.vcmd_iosize;
				 i += 4) {
				//set all register 0
				GB02FUNC318((const void *)dev[n].hwregs,
				 i, 0x0000);
			}
			//enable all interrupt
			GB02FUNC318((const void *)dev[n].hwregs,
				GB02MAC515, 0xffffffff);
		}
	}
}

void GB02FUNC1607(void)
{
	bi_list_node *cmdbuf_node = NULL;
	gb_printf(KERN_INFO, "%s:%d, enc resume in\n", __func__, __LINE__);

	cmdbuf_node = hantrovcmd_enc_data[0].list_manager.tail;
	if (cmdbuf_node != NULL)
		((struct GB02STR18 *)cmdbuf_node->data)->cmdbuf_run_done = 1;

	hantrovcmd_enc_data[0].working_state = GB02MAC145;
	hantrovcmd_enc_data[0].sw_cmdbuf_rdy_num = 0;

	GB02FUNC1606(hantrovcmd_enc_data, 1);

	gb_printf(KERN_INFO, "%s:%d, enc resume out\n", __func__, __LINE__);
}
void GB02FUNC1610(void)
{
	gb_printf(KERN_INFO, "%s:%d, enc suspend in\n", __func__, __LINE__);

	for(;;) {
		if (list_empty(&hantrovcmd_enc_data[0].wait_queue->head))
			break;
	}

	gb_printf(KERN_INFO, "%s:%d, enc suspend out\n", __func__, __LINE__);
}

static void GB02FUNC1612(struct GB02STR7 *hantrovcmd_enc_data,
	int total_vcmd_core_num)
{
	u32 i;

	GB02FUNC1488(total_vcmd_core_num);
	for (i = 0; i < total_vcmd_core_num; i++) {
		if (hantrovcmd_enc_data[i].hwregs) {
			iounmap((void *)hantrovcmd_enc_data[i].hwregs);
			release_mem_region(hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_base_addr,
				hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_iosize);
			hantrovcmd_enc_data[i].hwregs = NULL;
		}
	}
}

static u32 GB02FUNC1614(struct GB02STR14 *enc_vcmd_mem_pool)
{
	iounmap((void *)enc_vcmd_mem_pool[0].virtual_address);
	release_mem_region(enc_vcmd_mem_pool[0].bus_address,
		enc_vcmd_mem_pool[0].size);
	iounmap((void *)enc_vcmd_mem_pool[1].virtual_address);
	release_mem_region(enc_vcmd_mem_pool[1].bus_address,
		enc_vcmd_mem_pool[1].size);
	iounmap((void *)enc_vcmd_mem_pool[2].virtual_address);
	release_mem_region(enc_vcmd_mem_pool[2].bus_address,
		enc_vcmd_mem_pool[2].size);
	return 0;
}

void GB02FUNC1616(struct GB02STR7 *dev,
	bi_list_node *last_linked_cmdbuf_node)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;

	new_cmdbuf_node = last_linked_cmdbuf_node;
	while (1) {
		if (new_cmdbuf_node == NULL)
			break;
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if (GB02STR18->cmdbuf_data_linked)
			GB02STR18->cmdbuf_data_linked = 0;
		else
			break;
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
	dev->sw_cmdbuf_rdy_num = 0;
}

int GB02FUNC1618(struct drm_file *filp, void **enc_priv, void *data)
{
	int result = 0;
	unsigned long flags;
	struct drm_gem_object *vcmd_obj = NULL;
	struct GB02STR7 *dev = hantrovcmd_enc_data;
	bi_list_node *process_manager_node;
	struct GB02STR17 *GB02STR17 = NULL;
	struct GB02STR186 *enc_data = NULL;
	struct GB02STR97 *args = (struct GB02STR97 *)data;

	enc_data = GB02FUNC1639();
	vcmd_obj = enc_data->vcmd_pool->obj;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	drm_vma_node_allow(&vcmd_obj->vma_node, filp);
#else
	drm_vma_node_allow(&vcmd_obj->vma_node, filp->filp);
#endif
	args->offset_va = enc_data->vcmd_pool->flp_offset;
	args->offset_pa = enc_data->vcmd_pool->offset;
	gb_printf(KERN_INFO, "%s:%d vcmdbuf offset_va=0x%llx offset_pa=0x%llx\n",
		__func__, __LINE__, args->offset_va, args->offset_pa);

	*enc_priv = (void *)dev;

	process_manager_node = GB02FUNC243();
	if (process_manager_node == NULL) {
		gb_printf(KERN_ERR, "%s:%d-----enc vcmd open fail---\n",
			__func__, __LINE__);
		return -1;
	}
	GB02STR17 =
		(struct GB02STR17 *)process_manager_node->data;
	GB02STR17->filp = filp;
	spin_lock_irqsave(&enc_vcmd_process_manager_lock, flags);
	GB02FUNC210(&(enc_data->enc_global_process_manager),
		process_manager_node);
	spin_unlock_irqrestore(&enc_vcmd_process_manager_lock, flags);

	gb_printf(KERN_INFO, "%s: dev opened finish!\n", __func__);
	return result;
}

static long GB02FUNC1621(bi_list *list, bi_list_node *cmdbuf_node)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	/*get cmdbuf object according to cmdbuf_id*/
	new_cmdbuf_node = cmdbuf_node;
	if (new_cmdbuf_node == NULL)
		return -1;
	//remove node from list
	new_cmdbuf_node = GB02FUNC176(list, new_cmdbuf_node);
	if (new_cmdbuf_node) {
		//free node
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		enc_data->enc_global_cmdbuf_node[GB02STR18->cmdbuf_id] = NULL;
		GB02FUNC1629(new_cmdbuf_node);
		return 0;
	}
	return 1;
}

int GB02FUNC1623(struct drm_file *filp, void *enc_priv)
{
	struct GB02STR7 *dev =
		(struct GB02STR7 *)enc_priv;
		//(struct GB02STR7 *)filp->driver_priv;
	u32 core_id = 0;
	unsigned long flags;
	u32 release_cmdbuf_num = 0;
	struct drm_gem_object *vcmd_obj = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;
	bi_list_node *process_manager_node;
	struct GB02STR17 *GB02STR17 = NULL;
	long retVal = 0;
	u32 total_vcmd_core_num;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	vcmd_obj = enc_data->vcmd_pool->obj;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	drm_vma_node_revoke(&vcmd_obj->vma_node, filp);
#else
	drm_vma_node_revoke(&vcmd_obj->vma_node, filp->filp);
#endif

	total_vcmd_core_num =
		sizeof(enc_vcmd_core_array) / sizeof(struct GB02STR5);
	if (down_interruptible(&(enc_data->enc_vcmd_reserve_cmdbuf_sem[dev->enc_vcmd_core_cfg.sub_module_type])))
		return -ERESTARTSYS;

	for (core_id = 0; core_id < total_vcmd_core_num; core_id++) {
		if ((&dev[core_id]) == NULL)
			continue;
		spin_lock_irqsave(dev[core_id].spinlock, flags);
		new_cmdbuf_node = dev[core_id].list_manager.head;
		while (1) {
			if (new_cmdbuf_node == NULL)
				break;
			cmdbuf_obj_temp =
			 (struct GB02STR18 *)new_cmdbuf_node->data;
			if (dev[core_id].hwregs && (cmdbuf_obj_temp->filp == filp)) {
				if (cmdbuf_obj_temp->cmdbuf_run_done) {
					cmdbuf_obj_temp->cmdbuf_need_remove = 1;
					retVal =
					 GB02FUNC1621(&dev[core_id].list_manager,
						new_cmdbuf_node);
					if (retVal == 1)
						cmdbuf_obj_temp->GB02STR17
						 = NULL;
				} else if (cmdbuf_obj_temp->cmdbuf_data_linked
				 == 0) {
					cmdbuf_obj_temp->cmdbuf_data_linked = 1;
					cmdbuf_obj_temp->cmdbuf_run_done = 1;
					cmdbuf_obj_temp->cmdbuf_need_remove = 1;
					retVal =
					 GB02FUNC1621(&dev[core_id].list_manager,
							new_cmdbuf_node);
					if (retVal == 1)
						cmdbuf_obj_temp->GB02STR17
						 = NULL;
				} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 1
					&& dev[core_id].working_state
						== GB02MAC145) {
					cmdbuf_obj_temp->cmdbuf_run_done = 1;
					cmdbuf_obj_temp->cmdbuf_need_remove
						= 1;
					retVal =
						GB02FUNC1621(&dev[core_id].list_manager,
						new_cmdbuf_node);
					if (retVal == 1)
						cmdbuf_obj_temp->GB02STR17 = NULL;
				} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 1
					&& dev[core_id].working_state
						== GB02MAC146) {
					bi_list_node *last_cmdbuf_node;
					u32 record_last_cmdbuf_rdy_num;
					//abort the vcmd and wait
					enc_data->software_triger_abort = 1;
					if (wait_event_interruptible(*dev[core_id].wait_abort_queue,
						GB02FUNC1527(&dev[core_id]))) {
						spin_unlock_irqrestore(dev[core_id].spinlock, flags);
						up(&(enc_data->enc_vcmd_reserve_cmdbuf_sem[dev->enc_vcmd_core_cfg.sub_module_type]));
						enc_data->software_triger_abort
							= 0;
						return -ERESTARTSYS;
					}
					enc_data->software_triger_abort = 0;
					cmdbuf_obj_temp->cmdbuf_run_done = 1;
					cmdbuf_obj_temp->cmdbuf_need_remove
						= 1;
					retVal =
						GB02FUNC1621(&dev[core_id].list_manager,
						new_cmdbuf_node);
					if (retVal == 1)
						cmdbuf_obj_temp->GB02STR17
							= NULL;
					//link
					last_cmdbuf_node =
						GB02FUNC260(dev[core_id].list_manager.tail);
					record_last_cmdbuf_rdy_num =
						dev->sw_cmdbuf_rdy_num;
					GB02FUNC1542(dev,
						last_cmdbuf_node);
					//re-run
					if (dev[core_id].sw_cmdbuf_rdy_num)
						GB02FUNC1554(dev,
						 last_cmdbuf_node);
				}
				release_cmdbuf_num++;
				gb_printf(KERN_INFO, "%s: release reserved cmdbuf!\n", __func__);
			}
			new_cmdbuf_node = new_cmdbuf_node->next;
		}
		spin_unlock_irqrestore(dev[core_id].spinlock, flags);
	}
	if (release_cmdbuf_num)
		wake_up_interruptible_all(&enc_vcmd_cmdbuf_memory_wait);
	spin_lock_irqsave(&enc_vcmd_process_manager_lock, flags);
	process_manager_node = enc_data->enc_global_process_manager.head;
	while (1) {
		if (process_manager_node == NULL)
			break;
		GB02STR17 =
			(struct GB02STR17 *)process_manager_node->data;
		if (GB02STR17->filp == filp)
			break;
		process_manager_node = process_manager_node->next;
	}
	//remove node from list
	GB02FUNC225(&enc_data->enc_global_process_manager,
		process_manager_node);
	spin_unlock_irqrestore(&enc_vcmd_process_manager_lock, flags);
	GB02FUNC264(process_manager_node);
	up(&enc_data->enc_vcmd_reserve_cmdbuf_sem[dev->enc_vcmd_core_cfg.sub_module_type]);
	gb_printf(KERN_INFO, "%s: dev close finish!!\n", __func__);
	return 0;
}

static void GB02FUNC1629(bi_list_node *cmdbuf_node)
{
	struct GB02STR18 *GB02STR18 = NULL;

	if (cmdbuf_node == NULL) {
		gb_printf(KERN_ERR, "%s: remove_cmdbuf_node NULL\n", __func__);
		return;
	}
	GB02STR18 = (struct GB02STR18 *)cmdbuf_node->data;

	GB02FUNC1587(GB02STR18->cmdbuf_id);
	GB02FUNC254(GB02STR18);
	GB02FUNC230(cmdbuf_node);
}

long GB02FUNC1630(struct drm_file *filp, u16 cmdbuf_id)
{
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list_node *last_cmdbuf_node = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list *list = NULL;
	u32 module_type;
	unsigned long flags;
	struct GB02STR7 *dev = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	/*get cmdbuf object according to cmdbuf_id*/
	new_cmdbuf_node = enc_data->enc_global_cmdbuf_node[cmdbuf_id];
	if (new_cmdbuf_node == NULL) {
		//should not happen
		gb_printf(KERN_ERR, "%s: ERROR cmdbuf_id !!!!\n", __func__);
		return -1;
	}
	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
	if (GB02STR18->filp != filp) {
		//should not happen
		gb_printf(KERN_ERR, "%s: ERROR cmdbuf_id !!\n", __func__);
		return -1;
	}
	module_type = GB02STR18->module_type;
	//TODO
	if (down_interruptible
		(&enc_data->enc_vcmd_reserve_cmdbuf_sem[module_type])) {
		gb_printf(KERN_ERR, "%s:%d--down interruptible--\n",
			__func__, __LINE__);
		return -ERESTARTSYS;
	}
	dev = &hantrovcmd_enc_data[GB02STR18->core_id];

	list = &dev->list_manager;
	GB02STR18->cmdbuf_need_remove = 1;
	last_cmdbuf_node = new_cmdbuf_node->previous;
	while (1) {
		//remove current node
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if (GB02STR18->cmdbuf_need_remove == 1) {
			new_cmdbuf_node =
				GB02FUNC176(list,
				 new_cmdbuf_node);
			if (new_cmdbuf_node) {
				//free node
				enc_data->enc_global_cmdbuf_node[GB02STR18->cmdbuf_id]
					= NULL;
				if (GB02STR18->GB02STR17) {
					spin_lock_irqsave(&GB02STR18->GB02STR17->spinlock, flags);
					GB02STR18->GB02STR17->total_exe_time -=
						GB02STR18->executing_time;
					spin_unlock_irqrestore(&GB02STR18->GB02STR17->spinlock, flags);
					wake_up_interruptible_all(&GB02STR18->GB02STR17->wait_queue);
				}
				GB02FUNC1629(new_cmdbuf_node);
			}
		}
		if (last_cmdbuf_node == NULL)
			break;
		new_cmdbuf_node = last_cmdbuf_node;
		last_cmdbuf_node = new_cmdbuf_node->previous;
	}
	//spin_unlock_irqrestore(dev->spinlock, flags);
	up(&enc_data->enc_vcmd_reserve_cmdbuf_sem[module_type]);
	return 0;
}

int GB02FUNC1632(struct drm_file *filp, void *data)
{
	int ret = -1;
	struct GB02STR3 vcmd_buf = {0};
	struct drm_gb_cmdbuf_resv *input_para =
		(struct drm_gb_cmdbuf_resv *)data;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	vcmd_buf.offset = enc_data->vcmd_pool->offset;
	vcmd_buf.vir_buff = enc_data->vcmd_pool->vir_buff;

	gb_printf(KERN_INFO, "%s:%d:-vcmd buff offset = 0x%llx, vir_buf = 0x%llx--\n",
		__func__, __LINE__,
		vcmd_buf.offset, (long long)vcmd_buf.vir_buff);
	ret = GB02FUNC1597(filp, input_para,
		&enc_data->enc_global_process_manager,
		enc_data->enc_global_cmdbuf_node,
		enc_data->enc_vcmd_mem_pool, vcmd_buf);
	if (ret == -1)
		gb_printf(KERN_INFO, "%s: enc VCMD Reserve %d\n", __func__,
			input_para->cmdbuf_id);

	return ret;
}


long GB02FUNC1634(struct drm_file *filp, unsigned int cmd,
	void *arg)
{
	int err = 0;
	size_t base_ddr_addr = 0;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	gb_printf(KERN_INFO, "%s: enc ioctl start cmd: 0x%08x\n", __func__, cmd);

	if (_IOC_TYPE(cmd) != GB02MAC2552
	){
		gb_printf(KERN_ERR, "%s:%d--------ioctl fail---\n", __func__, __LINE__);
		return -ENOTTY;
	}
	if ((_IOC_TYPE(cmd) == GB02MAC2552 &&
		_IOC_NR(cmd) > GB02MAC2553)
		) {
		gb_printf(KERN_ERR, "%s:%d--------ioctl fail---\n", __func__, __LINE__);
		return -ENOTTY;
	}
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !gb_access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !gb_access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err) {
		gb_printf(KERN_ERR, "%s:%d--------ioctl fail---\n", __func__, __LINE__);
		return -EFAULT;
	}
	gb_printf(KERN_INFO, "%s:------cmd = %x---\n", __func__, cmd);
	switch (cmd) {
	case HANTRO_IOCH_GET_VCMD_ENABLE:
	{
		__put_user(1, (unsigned long *)arg);
		break;
	}
	case HANTRO_IOCH_GET_CMDBUF_PARAMETER:
	{
		struct GB02STR16 local_cmdbuf_mem_data;
		struct GB02STR14	*vcmd_buf_mem_pool;
		struct GB02STR14	*vcmd_status_buf_mem_pool;

		vcmd_buf_mem_pool = &enc_data->enc_vcmd_mem_pool[0];
		vcmd_status_buf_mem_pool = &enc_data->enc_vcmd_mem_pool[1];
		base_ddr_addr = enc_data->enc_pcie.vram_paddr_base;

		local_cmdbuf_mem_data.cmdbuf_unit_size = GB02MAC156;
		local_cmdbuf_mem_data.status_cmdbuf_unit_size =
		 GB02MAC156;
		local_cmdbuf_mem_data.cmdbuf_total_size =
		 GB02MAC158;
		local_cmdbuf_mem_data.status_cmdbuf_total_size =
			GB02MAC158;
		local_cmdbuf_mem_data.phy_status_cmdbuf_addr =
			vcmd_status_buf_mem_pool->bus_address;
		local_cmdbuf_mem_data.phy_cmdbuf_addr =
			vcmd_buf_mem_pool->bus_address;

		gb_printf(KERN_INFO, "%s:%d----phy status cmdbuf addr=%lx--- \
			phy cmdbuf addr=%lx-base_ddr_addr=%lx-\n",
			__func__, __LINE__,
			local_cmdbuf_mem_data.phy_status_cmdbuf_addr,
			local_cmdbuf_mem_data.phy_cmdbuf_addr, base_ddr_addr);
		local_cmdbuf_mem_data.base_ddr_addr = base_ddr_addr;
		return copy_to_user((struct GB02STR16 *)arg,
		 &local_cmdbuf_mem_data,
		  sizeof(struct GB02STR16));
	}
	case HANTRO_IOCH_GET_VCMD_PARAMETER:
	{
		struct GB02STR184 input_para;
		int ret;

		gb_printf(KERN_INFO, "%s-%d: VCMD get vcmd config parameter\n",
			__func__, __LINE__);
		ret = copy_from_user(&input_para,
		 (struct GB02STR184 *)arg,
		  sizeof(struct GB02STR184));
		if (enc_data->vcmd_type_core_num[input_para.module_type]) {
			input_para.submodule_main_addr =
				enc_vcmd_manager[input_para.module_type][0]->enc_vcmd_core_cfg.submodule_main_addr;
			input_para.submodule_dec400_addr =
				enc_vcmd_manager[input_para.module_type][0]->enc_vcmd_core_cfg.submodule_dec400_addr;
			input_para.submodule_L2Cache_addr =
				enc_vcmd_manager[input_para.module_type][0]->enc_vcmd_core_cfg.submodule_L2Cache_addr;
			input_para.submodule_MMU_addr[0] =
				enc_vcmd_manager[input_para.module_type][0]->enc_vcmd_core_cfg.submodule_MMU_addr[0];
			input_para.submodule_MMU_addr[1] =
				enc_vcmd_manager[input_para.module_type][0]->enc_vcmd_core_cfg.submodule_MMU_addr[1];
			input_para.submodule_axife_addr[0] =
				enc_vcmd_manager[input_para.module_type][0]->enc_vcmd_core_cfg.submodule_axife_addr[0];
			input_para.submodule_axife_addr[1] =
				enc_vcmd_manager[input_para.module_type][0]->enc_vcmd_core_cfg.submodule_axife_addr[1];
			input_para.config_status_cmdbuf_id =
				enc_vcmd_manager[input_para.module_type][0]->status_cmdbuf_id;
			input_para.vcmd_hw_version_id =
			 enc_vcmd_manager[input_para.module_type][0]->hw_version_id;
			input_para.vcmd_core_num =
			enc_data->vcmd_type_core_num[input_para.module_type];
			gb_printf(KERN_INFO, "%s:%d------module type = %x---\n",
				__func__, __LINE__, input_para.module_type);
		} else {
			input_para.submodule_main_addr = 0xffff;
			input_para.submodule_dec400_addr = 0xffff;
			input_para.submodule_L2Cache_addr = 0xffff;
			input_para.submodule_MMU_addr[0] = 0xffff;
			input_para.submodule_MMU_addr[1] = 0xffff;
			input_para.submodule_axife_addr[0] = 0xffff;
			input_para.submodule_axife_addr[1] = 0xffff;
			input_para.config_status_cmdbuf_id = 0;
			input_para.vcmd_core_num = 0;
			input_para.vcmd_hw_version_id = GB02MAC186;
		}
		return copy_to_user((struct GB02STR184 *)arg,
		 &input_para, sizeof(struct GB02STR184));
	}
	case HANTRO_IOCH_GET_HWINFO_FROM_VCMD: {
		struct GB02STR185 *hw_info = &enc_data->hw_info;
		struct GB02STR185 uhw_info;

		memcpy(&uhw_info, hw_info, sizeof(struct GB02STR185));
		gb_printf(KERN_INFO, "%s:ioctl get hw info from vcmd---\n", __func__);
		return copy_to_user((struct GB02STR185 *)arg,
			&uhw_info, sizeof(struct GB02STR185));
	}
	default:
		gb_printf(KERN_INFO, "%s: enc IOCTL default, quit!!!\n", __func__);
	}
	return 0;
}

struct GB02STR186 *GB02FUNC1639(void)
{
	return enc_private_data;
}

struct GB02STR186 *GB02FUNC1640(int total_vcmd_core_num)
{
	struct GB02STR186 *enc_data = NULL;

	hantrovcmd_enc_data =
	 (struct GB02STR7 *)vmalloc(sizeof(struct GB02STR7)
		* total_vcmd_core_num);
	if (hantrovcmd_enc_data == NULL)
		goto err1;
	memset(hantrovcmd_enc_data, 0,
		sizeof(struct GB02STR7) * total_vcmd_core_num);

	enc_data =
	 (struct GB02STR186 *)vmalloc(sizeof(struct GB02STR186));
	if (enc_data == NULL)
		goto err1;
	memset(enc_data, 0, sizeof(struct GB02STR186));
	return enc_data;
err1:
	if (hantrovcmd_enc_data != NULL)
		vfree(hantrovcmd_enc_data);
	if (enc_data != NULL)
		vfree(enc_data);
	gb_printf(KERN_ERR, "%s-%d: err1 module not vmalloc error\n", __func__, __LINE__);
	return NULL;
}

void GB02FUNC1643(struct GB02STR186 *enc_data,
	struct GB02STR69 *pci_bars, struct GB02STR67 *dev_info,
	struct GB02STR126 vpu_offset, struct GB02STR3 *vcmd_pool)
{
	int i = 0, k;

	i = GB02FUNC465(dev_info);
	enc_data->enc_pcie.vpu_paddr_base =
	 pci_bars[i].base + vpu_offset.enc_offset_base;
	enc_data->enc_pcie.vpu_vaddr_base =
	 pci_bars[i].mmio + vpu_offset.enc_offset_base;
	i = GB02FUNC468(dev_info);
	enc_data->enc_pcie.vram_vaddr_base = pci_bars[i].mmio;
	enc_data->enc_pcie.vram_paddr_base = pci_bars[i].base;
	enc_data->vcmd_buff.offset = vcmd_pool->offset;
	enc_data->vcmd_buff.vir_buff = vcmd_pool->vir_buff;
	enc_data->enc_base_ddr_addr = 0;
	enc_data->enc_cmdbuf_used_pos = 0;
	for	(i = 0; i < GB02MAC159; i++) {
		enc_data->enc_cmdbuf_used[i] = 0;
		enc_data->enc_global_cmdbuf_node[i] = NULL;
	}

	enc_data->enc_cmdbuf_used_residual = GB02MAC159;
	enc_data->enc_cmdbuf_used_pos = 1;
	enc_data->enc_cmdbuf_used[0] = 1;
	enc_data->enc_cmdbuf_used_residual -= 1;

	for (k = 0; k < MAX_VCMD_TYPE; k++) {
		enc_data->vcmd_type_core_num[k] = 0;
		enc_data->vcmd_position[k] = 0;
		for (i = 0; i < GB02MAC152; i++)
			enc_vcmd_manager[k][i] = NULL;
	}
}

void GB02FUNC1645(struct GB02STR186 *enc_data,
	struct GB02STR3 *vcmd_pool, struct GB02STR3 *vcmd_reg)
{
	gb_printf(KERN_INFO, "%s:%d--vcmd pool offset =%llx, \
		vaddr = %llx, reg offset=%llx, vaddr=%llx--\n",
		__func__, __LINE__, vcmd_pool->offset,
		(long long)vcmd_pool->vir_buff,
	vcmd_reg->offset, (long long)vcmd_reg->vir_buff);
	enc_data->enc_vcmd_mem_pool[0].bus_address = vcmd_pool->offset;
	enc_data->enc_vcmd_mem_pool[1].bus_address = vcmd_pool->offset
		+ GB02MAC156;
	enc_data->enc_vcmd_mem_pool[2].bus_address = vcmd_reg->offset;

	enc_data->enc_vcmd_mem_pool[0].virtual_address = vcmd_pool->vir_buff;
	enc_data->enc_vcmd_mem_pool[1].virtual_address =
	 vcmd_pool->vir_buff + GB02MAC156;
	enc_data->enc_vcmd_mem_pool[2].virtual_address = vcmd_reg->vir_buff;
}

void GB02FUNC1646(struct GB02STR186 *enc_data)
{
	int i;
	static struct GB02STR14 *enc_vcmd_registers_mem_pool;

	enc_vcmd_registers_mem_pool = &(enc_data->enc_vcmd_mem_pool[2]);
	for (i = 0; i < enc_data->total_vcmd_core_num; i++) {
		if (enc_vcmd_core_array[i].vcmd_irq != 0) {
			gb_printf(KERN_INFO, "%s-%d: enc before irq = %d, should irq=0\n",
			 __func__, __LINE__, enc_vcmd_core_array[i].vcmd_irq);
			enc_vcmd_core_array[i].vcmd_irq = 0;
		}
		hantrovcmd_enc_data[i].enc_vcmd_core_cfg =
		 enc_vcmd_core_array[i];
		hantrovcmd_enc_data[i].hwregs = NULL;
		hantrovcmd_enc_data[i].core_id = i;
		hantrovcmd_enc_data[i].working_state = GB02MAC145;
		hantrovcmd_enc_data[i].sw_cmdbuf_rdy_num = 0;
		hantrovcmd_enc_data[i].spinlock =
			&(enc_data->enc_owner_lock_vcmd[i]);
		spin_lock_init(&(enc_data->enc_owner_lock_vcmd[i]));
		hantrovcmd_enc_data[i].wait_queue =
			&(enc_data->enc_wait_queue_vcmd[i]);
		init_waitqueue_head(&(enc_data->enc_wait_queue_vcmd[i]));
		hantrovcmd_enc_data[i].wait_abort_queue =
			&(enc_data->enc_abort_queue_vcmd[i]);
		init_waitqueue_head(&(enc_data->enc_abort_queue_vcmd[i]));
		GB02FUNC208(&hantrovcmd_enc_data[i].list_manager);
		hantrovcmd_enc_data[i].duration_without_int = 0;
		enc_vcmd_manager[enc_vcmd_core_array[i].sub_module_type][enc_data->vcmd_type_core_num[enc_vcmd_core_array[i].sub_module_type]]
		 = &hantrovcmd_enc_data[i];
		enc_data->vcmd_type_core_num[enc_vcmd_core_array[i].sub_module_type]++;
		hantrovcmd_enc_data[i].vcmd_reg_mem_bus_address =
			enc_vcmd_registers_mem_pool->bus_address +
			i * GB02MAC144;
		hantrovcmd_enc_data[i].vcmd_reg_mem_virtual_address =
			enc_vcmd_registers_mem_pool->virtual_address +
			i * GB02MAC144;
		hantrovcmd_enc_data[i].vcmd_reg_mem_size = GB02MAC144;
#ifndef CONFIG_SW64
		memset(hantrovcmd_enc_data[i].vcmd_reg_mem_virtual_address, 0,
                        GB02MAC144);
#else
		memset_io(hantrovcmd_enc_data[i].vcmd_reg_mem_virtual_address, 0,
                       GB02MAC144);
#endif
	}
}
irqreturn_t GB02FUNC1649(int irq)
{
	irqreturn_t ret = 0;
	ret = enc_hantrovcmd_isr(irq, &hantrovcmd_enc_data[0]);
	return ret;
}

int GB02FUNC28(struct GB02STR7 *gbenc_data, void *pci_bars,
	struct GB02STR67 *dev_info, struct GB02STR3 *vcmd_pool,
	struct GB02STR3 *vcmd_reg, struct GB02STR126 vpu_offset)
{
	int i, total_vcmd_core_num;
	int result = -1;
	bi_list *tmp_enc_manager = NULL;
	struct GB02STR186 *enc_data = NULL;
	u64 enc_paddr;
	struct GB02STR3 *vcmd_pool_tmp = NULL;

	total_vcmd_core_num =
		sizeof(enc_vcmd_core_array) / sizeof(struct GB02STR5);
	enc_data = GB02FUNC1640(total_vcmd_core_num);
	if (enc_data == NULL)
		goto err1;

	vcmd_pool_tmp =
		(struct GB02STR3 *)vmalloc(sizeof(struct GB02STR3));
	if (vcmd_pool_tmp == NULL)
		goto err1;
	memset(vcmd_pool_tmp, 0, sizeof(struct GB02STR3));

	memcpy(vcmd_pool_tmp, vcmd_pool, sizeof(struct GB02STR3));
	enc_data->vcmd_pool = vcmd_pool_tmp;

	gbenc_data = hantrovcmd_enc_data;
	enc_private_data = enc_data;

	enc_data->total_vcmd_core_num = total_vcmd_core_num;
	GB02FUNC1643(enc_data, (struct GB02STR69 *)pci_bars,
	 dev_info, vpu_offset, vcmd_pool);

	GB02FUNC1645(enc_data, vcmd_pool, vcmd_reg);

	enc_paddr = enc_data->enc_pcie.vpu_paddr_base;
	result = GB02FUNC1603(total_vcmd_core_num, enc_paddr);
	if (result)
		goto err1;
	for (i = 0; i < total_vcmd_core_num; i++)
		gb_printf(KERN_INFO, "%s: enc init - vcmdcore[%d] addr =0x%lx\n",
		__func__, i, enc_vcmd_core_array[i].vcmd_base_addr);

	tmp_enc_manager = &(enc_data->enc_global_process_manager);
	GB02FUNC208(tmp_enc_manager);
	result = ConfigAXIFE(1, total_vcmd_core_num);
	if (result < 0) {
		GB02FUNC1488(total_vcmd_core_num);
		goto err1;
	}
	GB02FUNC1646(enc_data);

	result = GB02FUNC1604(hantrovcmd_enc_data, total_vcmd_core_num,
	 (u8 *)enc_data->enc_pcie.vpu_vaddr_base);
	if (result < 0)
		goto err1;
	GB02FUNC1606(hantrovcmd_enc_data, total_vcmd_core_num);

	GB02FUNC247(tmp_enc_manager);
	for (i = 0; i < MAX_VCMD_TYPE; i++) {
		if (enc_data->vcmd_type_core_num[i] == 0)
			continue;
		sema_init(&(enc_data->enc_vcmd_reserve_cmdbuf_sem[i]), 1);
	}

	for (i = 0; i < MAX_VCMD_TYPE; i++) {
		if (enc_data->vcmd_type_core_num[i] == 0)
			continue;
		GB02FUNC1600(i, hantrovcmd_enc_data);
	}
	gb_printf(KERN_INFO, "%s-%d: enc_data[%d].enc_vcmd_core_cfg.vcmd_irq = %d\n",
	 __func__, __LINE__, 0,
	  hantrovcmd_enc_data[0].enc_vcmd_core_cfg.vcmd_irq);
	gb_printf(KERN_INFO, "%s-%d: enc vcmd finish !!!\n", __func__, __LINE__);
	return 0;
err1:
	if (hantrovcmd_enc_data != NULL)
		vfree(hantrovcmd_enc_data);
	if (enc_data != NULL)
		vfree(enc_data);
	if (vcmd_pool_tmp != NULL)
		vfree(vcmd_pool_tmp);
	gb_printf(KERN_ERR, "%s: err1 module not inserted\n", __func__);
	return result;
}

int GB02FUNC1655(int irq_id)
{
	int i, ret = -1;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
		/* get the IRQ line */
	for (i = 0; i < enc_data->total_vcmd_core_num; i++) {
		if (hantrovcmd_enc_data[i].hwregs == NULL)
			continue;
		if (hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_irq != -1) {
			gb_printf(KERN_INFO, "%s-%d: vpu enc irq_id = %d\n",
			 __func__, __LINE__, irq_id);
			ret =
			 request_irq(irq_id,
			  enc_hantrovcmd_isr,
			#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
			SA_INTERRUPT | SA_SHIRQ,
			#else
			IRQF_SHARED,
			#endif
			"vc8000", (void *)&hantrovcmd_enc_data[i]);
			if (ret == -EINVAL) {
				gb_printf(KERN_ERR, "%s: enc vcmd_irq error core_id = %d\n",
					__func__, i);
				GB02FUNC1612(hantrovcmd_enc_data,
				 enc_data->total_vcmd_core_num);
				goto err;
			} else if (ret == -EBUSY) {
				gb_printf(KERN_ERR, "%s: id = %d,IRQ %d busy\n", __func__, i,
				 hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_irq);
				GB02FUNC1612(hantrovcmd_enc_data,
				 enc_data->total_vcmd_core_num);
				goto err;
			} else
				gb_printf(KERN_INFO, "%s-%d: enc irq success, irq = %d",
				 __func__, __LINE__, irq_id);
		} else
			gb_printf(KERN_ERR, "%s: enc IRQ not in use!\n", __func__);
	}
	return 0;
err:
#ifdef HANTROMMU_SUPPORT
	GB02FUNC1614(enc_data->enc_vcmd_mem_pool);
#endif
	if (hantrovcmd_enc_data != NULL)
		vfree(hantrovcmd_enc_data);
	if (enc_data != NULL)
		vfree(enc_data);
	gb_printf(KERN_ERR, "%s: err1 module not inserted\n", __func__);
	return ret;
}

static long GB02FUNC1658(bi_list *list)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	while (1) {
		new_cmdbuf_node = list->head;
		if (new_cmdbuf_node == NULL)
			return 0;
		//remove node from list
		GB02FUNC225(list, new_cmdbuf_node);
		//free node
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		enc_data->enc_global_cmdbuf_node[GB02STR18->cmdbuf_id] = NULL;
		GB02FUNC1629(new_cmdbuf_node);
	}
	return 0;
}

void GB02FUNC1659(void)
{
	int i = 0;
	u32 result;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	for (i = 0; i < enc_data->total_vcmd_core_num; i++) {
		if (hantrovcmd_enc_data[i].hwregs == NULL)
			continue;
		/* disable interrupt at first */
		GB02FUNC318((const void *)hantrovcmd_enc_data[i].hwregs,
			GB02MAC515, 0x0000);
		/* disable HW */
		GB02FUNC318((const void *)hantrovcmd_enc_data[i].hwregs,
			GB02MAC513, 0x0000);
		/* read status register */
		result =
		 GB02FUNC313((const void *)hantrovcmd_enc_data[i].hwregs,
		 GB02MAC514);
		/* clean status register */
		GB02FUNC318((const void *)hantrovcmd_enc_data[i].hwregs,
			GB02MAC514, result);

		/* free the vcmd IRQ */
		if (hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_irq != -1)
			free_irq(hantrovcmd_enc_data[i].enc_vcmd_core_cfg.vcmd_irq,
				(void *)&hantrovcmd_enc_data[i]);
		GB02FUNC1658(&hantrovcmd_enc_data[i].list_manager);
	}

	GB02FUNC277(&enc_data->enc_global_process_manager);
	GB02FUNC1612(hantrovcmd_enc_data, enc_data->total_vcmd_core_num);

	GB02FUNC1614(enc_data->enc_vcmd_mem_pool);
	if (hantrovcmd_enc_data != NULL)
		vfree(hantrovcmd_enc_data);
	if (enc_data != NULL)
		vfree(enc_data);

	gb_printf(KERN_INFO, "%s: module removed finish\n", __func__);
}


