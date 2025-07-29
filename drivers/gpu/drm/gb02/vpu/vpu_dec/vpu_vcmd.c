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
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <errno.h>
#include <drm/drm_file.h>
#include "common/gb_uk.h"
#include "vpu_vcmd.h"
#include "vpu_subsys.h"
#include "vpu_dec_irq.h"
#include "vpu/vpu_comm/vpu_list.h"
#include "vpu/vpu_comm/vpu_vcmd_registers.h"
#include "vpu/vpu_comm/vpu_comm.h"
#include "vpu/vpu_comm/vpu_basetype.h"
#include "vpu/vpu_comm/vpu_afe.h"
#include "common/gb_kernel_ver.h"
#include "gpu_test/gb_mem_tool.h"

//#define	GB_TIME_DEBUG
static void GB02FUNC971(bi_list_node *cmdbuf_node);
//hw_queue can be used for reserve cmdbuf memory
DECLARE_WAIT_QUEUE_HEAD(vcmd_cmdbuf_memory_wait);
DEFINE_SPINLOCK(vcmd_cmdbuf_alloc_lock);
DEFINE_SPINLOCK(vcmd_process_manager_lock);

/*for all vcmds, the core info should be listed here for subsequent use*/
struct GB02STR34 vcmd_core_array[GB02MAC9] = {
	//decoder configuration
	{GB02MAC238,
	GB02MAC239,
	GB02MAC241,
	GB02MAC243,
	GB02MAC244,
	GB02MAC246,
	GB02MAC248,
	GB02MAC250},

	{GB02MAC253,
	GB02MAC255,
	GB02MAC257,
	GB02MAC259,
	GB02MAC261,
	GB02MAC263,
	GB02MAC265,
	GB02MAC267},
};

static struct GB02STR37 *vcmd_manager[MAX_VCMD_TYPE][GB02MAC152];

/* dynamic allocation*/
static struct GB02STR37 *hantrovcmd_data;
static struct GB02STR43 *dec_vcmd_private_data;

struct GB02STR43 *GB02FUNC700(void)
{
	return dec_vcmd_private_data;
}

/*
 * Function name   : vcmd_pcie_init
 * Description     : Initialize PCI Hw access
 * Return type     : int
 */
static int GB02FUNC702(int total_vcmd_core_num, u64 dec_offset)
{
	int i = 0;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();
	for (i = 0; i < total_vcmd_core_num; i++) {
		vcmd_core_array[i].vcmd_base_addr  = dec_offset
			+ vcmd_core_array[i].vcmd_base_addr;
	}

	return 0;
}

void GB02FUNC705(struct GB02STR37 *dev,
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

void GB02FUNC710(struct GB02STR37  *dev, int total_vcmd_core_num)
{
	int i, n;
	u32 ret;

	for (n = 0; n < total_vcmd_core_num; n++) {
		if (dev[n].hwregs == NULL)
			continue;

		/*disable interrupt at first*/
		GB02FUNC318((const void *)dev[n].hwregs,
		 GB02MAC515, 0x0000);
		/*reset all*/
		GB02FUNC318((const void *)dev[n].hwregs,
		 GB02MAC513, 0x0002);
		/*read status register*/
		ret = GB02FUNC313((const void *)dev[n].hwregs,
		 GB02MAC514);
		/*clean status register*/
		GB02FUNC318((const void *)dev[n].hwregs,
		 GB02MAC514, ret);
		for (i = GB02MAC513; i <
			dev[n].vcmd_core_cfg.vcmd_iosize; i += 4) {
			GB02FUNC318((const void *)dev[n].hwregs, i, 0x0000);
		}
		/*enable all interrupt*/
		GB02FUNC318((const void *)dev[n].hwregs,
		 GB02MAC515, 0xffffffff);
		/* gate all external interrupts*/
		GB02FUNC318((const void *)dev[n].hwregs,
		 GB02MAC516, 0xffffffff);
	}
}

void GB02FUNC717(struct GB02STR37 *hantrovcmd_data,
	int total_vcmd_core_num)
{
	u32 i;
	unsigned long base_addr;
	u32 vcmd_iosize;

	for (i = 0; i < total_vcmd_core_num; i++) {
		if (hantrovcmd_data[i].hwregs) {
			base_addr =
			 hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr;
			vcmd_iosize =
			 hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize;
			iounmap((void *) hantrovcmd_data[i].hwregs);
			release_mem_region(base_addr, vcmd_iosize);
			hantrovcmd_data[i].hwregs = NULL;
		}
	}
}

int GB02FUNC718(struct GB02STR37 *hantrovcmd_data,
	int total_vcmd_core_num, u8 *vpu_vcmd_base)
{
	u32 hwid;
	int i;
	u32 found_hw = 0;
	unsigned long vcmd_base_addr;
	u32 vcmd_iosize;

	for (i = 0; i < total_vcmd_core_num; i++) {
		hantrovcmd_data[i].hwregs = NULL;

		vcmd_base_addr =
		 hantrovcmd_data[i].vcmd_core_cfg.vcmd_base_addr;
		vcmd_iosize = hantrovcmd_data[i].vcmd_core_cfg.vcmd_iosize;
		hantrovcmd_data[i].hwregs = vpu_vcmd_base;

		if (hantrovcmd_data[i].hwregs == NULL) {
			gb_printf(KERN_ERR, "%s: failed to ioremap HW regs is NULL\n",
			 __func__);
			continue;
		}
		/*read hwid and check validness and store it*/
		hwid = (u32)ioread32((void *)hantrovcmd_data[i].hwregs);
		gb_printf(KERN_INFO, "%s: hwid = 0x%08x\n", __func__, hwid);
		hantrovcmd_data[i].hw_version_id = hwid;

		/* check for vcmd HW ID */
		if (((hwid >> 16) & 0xFFFF) != GB02MAC153) {
			gb_printf(KERN_ERR, "%s: HW not found at 0x%lx\n",
			 __func__, vcmd_base_addr);
			hantrovcmd_data[i].hwregs = NULL;
			continue;
		}

		found_hw = 1;
		gb_printf(KERN_INFO, "%s: HW base <0x%lx> with ID <0x%08x>\n", __func__,
		 vcmd_base_addr, hwid);
	}

	if (found_hw == 0) {
		gb_printf(KERN_ERR, "%s: NO ANY HW found!!\n", __func__);
		return -1;
	}
	gb_printf(KERN_INFO, "%s-%d: dec vcmd reserve_IO finish!!!\n", __func__, __LINE__);
	return 0;
}

/* cmdbuf pool management */
static int GB02FUNC721(struct GB02STR14 *new_cmdbuf_addr,
	struct GB02STR14 *new_status_cmdbuf_addr, struct GB02STR3 vcmd_buff,
	struct drm_gb_cmdbuf_resv *input_para)
{
	unsigned long flags;
	int cmdbuf_index, status_cmdbuf_index;
	struct GB02STR14 *vcmd_buf_mem_pool = NULL;
	struct GB02STR14 *vcmd_status_buf_mem_pool = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	spin_lock_irqsave(&vcmd_cmdbuf_alloc_lock, flags);
	if (dec_data->cmdbuf_used_residual == 0) {
		spin_unlock_irqrestore(&vcmd_cmdbuf_alloc_lock, flags);
		return 0;
	}

	vcmd_buf_mem_pool = &dec_data->dec_vcmd_mem_pool[0];
	vcmd_status_buf_mem_pool = &dec_data->dec_vcmd_mem_pool[1];
	/* there is one cmdbuf at least */
	while (1) {
		if (dec_data->cmdbuf_used[dec_data->cmdbuf_used_pos] == 0 &&
		 (dec_data->global_cmdbuf_node[dec_data->cmdbuf_used_pos]
		  == NULL)) {
			dec_data->cmdbuf_used[dec_data->cmdbuf_used_pos] = 1;
			dec_data->cmdbuf_used_residual -= 1;
			cmdbuf_index = (dec_data->cmdbuf_used_pos - 1) * 2;
			status_cmdbuf_index = (dec_data->cmdbuf_used_pos - 1) * 2 + 1;
			new_cmdbuf_addr->virtual_address =
				vcmd_buff.vir_buff + cmdbuf_index * GB02MAC156;
			new_cmdbuf_addr->bus_address =
				vcmd_buff.offset + cmdbuf_index * GB02MAC156;
			new_cmdbuf_addr->size = GB02MAC156;
			new_cmdbuf_addr->cmdbuf_id = dec_data->cmdbuf_used_pos;

			new_status_cmdbuf_addr->virtual_address =
			 	vcmd_buff.vir_buff + status_cmdbuf_index * GB02MAC156;
			new_status_cmdbuf_addr->bus_address =
			 	vcmd_buff.offset + status_cmdbuf_index * GB02MAC156;
			new_status_cmdbuf_addr->size = GB02MAC156;
			new_status_cmdbuf_addr->cmdbuf_id = dec_data->cmdbuf_used_pos;

			(dec_data->cmdbuf_used_pos)++;
			if (dec_data->cmdbuf_used_pos >= GB02MAC159)
				dec_data->cmdbuf_used_pos = 0;

			input_para->cmdbuf_offset = cmdbuf_index * GB02MAC156;
			input_para->status_offset = status_cmdbuf_index * GB02MAC156;
			spin_unlock_irqrestore(&vcmd_cmdbuf_alloc_lock, flags);
			return 1;
		}
		(dec_data->cmdbuf_used_pos)++;
		if (dec_data->cmdbuf_used_pos >= GB02MAC159)
			dec_data->cmdbuf_used_pos = 0;
	}
	return 0;
}


static void GB02FUNC727(u16 cmdbuf_id)
{
	unsigned long flags;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	spin_lock_irqsave(&vcmd_cmdbuf_alloc_lock, flags);
	dec_data->cmdbuf_used[cmdbuf_id] = 0;
	dec_data->cmdbuf_used_residual += 1;
	spin_unlock_irqrestore(&vcmd_cmdbuf_alloc_lock, flags);
	wake_up_interruptible_all(&vcmd_cmdbuf_memory_wait);
}

static bi_list_node *GB02FUNC729(size_t  cmdbuf_addr,
	bi_list *list)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();
	new_cmdbuf_node = list->head;
	while (1) {
		if (new_cmdbuf_node == NULL)
			return NULL;
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if (((GB02STR18->cmdbuf_bus_address - dec_data->base_ddr_addr)
		 <= cmdbuf_addr) && (((GB02STR18->cmdbuf_bus_address -
		  dec_data->base_ddr_addr + GB02STR18->cmdbuf_size)
		   > cmdbuf_addr)))
			return new_cmdbuf_node;
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
	return NULL;
}

static int GB02FUNC732(struct GB02STR37 *dev, bi_list *list,
	bi_list_node *new_cmdbuf_node, struct GB02STR18 *GB02STR18)
{
	unsigned long flags = 0;
	int counter = 0;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	while (1) {
		dev =
		 vcmd_manager[GB02STR18->module_type][dec_data->vcmd_position[GB02STR18->module_type]];
		list = &dev->list_manager;
		spin_lock_irqsave(dev->spinlock, flags);
		if (list->tail == NULL) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			dec_data->vcmd_position[GB02STR18->module_type]++;
			if (dec_data->vcmd_position[GB02STR18->module_type] >=
			 dec_data->vcmd_type_core_num[GB02STR18->module_type])
				dec_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return 0;
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		dec_data->vcmd_position[GB02STR18->module_type]++;
		if (dec_data->vcmd_position[GB02STR18->module_type] >=
			dec_data->vcmd_type_core_num[GB02STR18->module_type])
			dec_data->vcmd_position[GB02STR18->module_type] = 0;
		counter++;

		if (counter >=
		 dec_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	return -1;
}

static int GB02FUNC738(struct GB02STR37 *dev, bi_list *list,
	bi_list_node *new_cmdbuf_node, struct GB02STR18 *GB02STR18)
{
	int counter = 0;
	bi_list_node *curr_cmdbuf_node = NULL;
	unsigned long flags = 0;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	while (1) {
		dev =
		 vcmd_manager[GB02STR18->module_type][dec_data->vcmd_position[GB02STR18->module_type]];
		list = &dev->list_manager;
		spin_lock_irqsave(dev->spinlock, flags);
		curr_cmdbuf_node = list->tail;
		if (curr_cmdbuf_node == NULL) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			dec_data->vcmd_position[GB02STR18->module_type]++;
			if (dec_data->vcmd_position[GB02STR18->module_type] >=
			 dec_data->vcmd_type_core_num[GB02STR18->module_type])
				dec_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return 0;
		}
		cmdbuf_obj_temp = (struct GB02STR18 *)curr_cmdbuf_node->data;
		if (cmdbuf_obj_temp->cmdbuf_run_done == 1) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			dec_data->vcmd_position[GB02STR18->module_type]++;
			if (dec_data->vcmd_position[GB02STR18->module_type] >=
			 dec_data->vcmd_type_core_num[GB02STR18->module_type])
				dec_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return 0;
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		dec_data->vcmd_position[GB02STR18->module_type]++;
		if (dec_data->vcmd_position[GB02STR18->module_type] >=
			dec_data->vcmd_type_core_num[GB02STR18->module_type])
			dec_data->vcmd_position[GB02STR18->module_type]
				= 0;
		counter++;

		if (counter >=
		 dec_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	return -1;
}

static int GB02FUNC747(struct GB02STR37 *dev, bi_list *list,
	bi_list_node *new_cmdbuf_node, struct GB02STR18 *GB02STR18)
{
	u32 hw_rdy_cmdbuf_num = 0;
	unsigned long flags = 0;
	int counter = 0;
	bi_list_node *curr_cmdbuf_node = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	while (1) {
		dev =
		 vcmd_manager[GB02STR18->module_type][dec_data->vcmd_position[GB02STR18->module_type]];
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
			dec_data->vcmd_position[GB02STR18->module_type]++;
			if (dec_data->vcmd_position[GB02STR18->module_type] >=
			 dec_data->vcmd_type_core_num[GB02STR18->module_type])
				dec_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return 0;
		}

		if ((hw_rdy_cmdbuf_num == dev->sw_cmdbuf_rdy_num)) {
			GB02FUNC210(list, new_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
			dec_data->vcmd_position[GB02STR18->module_type]++;
			if (dec_data->vcmd_position[GB02STR18->module_type] >=
			 dec_data->vcmd_type_core_num[GB02STR18->module_type])
				dec_data->vcmd_position[GB02STR18->module_type]
				 = 0;
			GB02STR18->core_id = dev->core_id;
			return 0;
		}
		spin_unlock_irqrestore(dev->spinlock, flags);
		dec_data->vcmd_position[GB02STR18->module_type]++;
		if (dec_data->vcmd_position[GB02STR18->module_type] >=
			dec_data->vcmd_type_core_num[GB02STR18->module_type])
			dec_data->vcmd_position[GB02STR18->module_type]
				= 0;
		counter++;

		if (counter >=
		 dec_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	return -1;
}

static int GB02FUNC753(struct GB02STR37 *dev,
	bi_list *list, bi_list_node *new_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	int counter = 0;
	size_t exe_cmdbuf_addr = 0;
	unsigned long flags = 0;
	bi_list_node *curr_cmdbuf_node = NULL;
	u32 cmdbuf_id = 0;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	while (1) {
		dev =
		 vcmd_manager[GB02STR18->module_type][dec_data->vcmd_position[GB02STR18->module_type]];
		//read executing cmdbuf address
		if (dev->hw_version_id <= GB02MAC186) {
			exe_cmdbuf_addr =
			 VCMDGetAddrRegisterValue((const void *)dev->hwregs,
			  dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR);
			list = &dev->list_manager;
			spin_lock_irqsave(dev->spinlock, flags);
			//get the executing cmdbuf node.
			curr_cmdbuf_node =
			 GB02FUNC729(exe_cmdbuf_addr,
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
			else	//GB02MAC151
				cmdbuf_id =
				 *(dev->vcmd_reg_mem_virtual_address +
					GB02MAC143);
			spin_lock_irqsave(dev->spinlock, flags);
			if (cmdbuf_id >= GB02MAC159
			 || cmdbuf_id == 0) {
				gb_printf(KERN_ERR, "%s:%d--cmdbuf addr =%lx, priotiry=%d, cmdbuf_id %d greater than the ceiling !!\n",
						__func__, __LINE__, dev->vcmd_reg_mem_bus_address, GB02STR18->priority, cmdbuf_id);
				spin_unlock_irqrestore(dev->spinlock, flags);
				return -2;
			}
			//get the executing cmdbuf node.
			curr_cmdbuf_node =
			 dec_data->global_cmdbuf_node[cmdbuf_id];
			if (curr_cmdbuf_node == NULL) {
				list = &dev->list_manager;
				curr_cmdbuf_node = list->head;
				while (1) {
					if (curr_cmdbuf_node == NULL)
						break;
					cmdbuf_obj_temp =
					 (struct GB02STR18 *)curr_cmdbuf_node->data;
					if (cmdbuf_obj_temp->cmdbuf_data_linked
					 && cmdbuf_obj_temp->cmdbuf_run_done
					  == 0)
						break;
					curr_cmdbuf_node =
					 curr_cmdbuf_node->next;
				}
			}
			//calculate total execute time of this device
			dev->total_exe_time =
			 GB02FUNC272(curr_cmdbuf_node);
			spin_unlock_irqrestore(dev->spinlock, flags);
		}
		dec_data->vcmd_position[GB02STR18->module_type]++;
		if (dec_data->vcmd_position[GB02STR18->module_type] >=
			dec_data->vcmd_type_core_num[GB02STR18->module_type])
			dec_data->vcmd_position[GB02STR18->module_type] = 0;
		counter++;
		if (counter >=
		 dec_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	return -1;
}

void GB02FUNC761(struct GB02STR37 *dev,
	bi_list *list, bi_list_node *new_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	int counter = 0;
	unsigned long flags = 0;
	u64 executing_time = 0xffffffffffffffff;
	struct GB02STR37 *smallest_dev = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	while (1) {
		dev =
		 vcmd_manager[GB02STR18->module_type][dec_data->vcmd_position[GB02STR18->module_type]];
		if (dev->total_exe_time <= executing_time) {
			executing_time = dev->total_exe_time;
			smallest_dev = dev;
		}
		dec_data->vcmd_position[GB02STR18->module_type]++;
		if (dec_data->vcmd_position[GB02STR18->module_type] >=
			dec_data->vcmd_type_core_num[GB02STR18->module_type])
			dec_data->vcmd_position[GB02STR18->module_type] = 0;
		counter++;
		if (counter >=
		 dec_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
	//insert list
	list = &smallest_dev->list_manager;
	spin_lock_irqsave(smallest_dev->spinlock, flags);
	GB02FUNC210(list, new_cmdbuf_node);
	spin_unlock_irqrestore(smallest_dev->spinlock, flags);
	GB02STR18->core_id = smallest_dev->core_id;
}

static int GB02FUNC766(struct GB02STR37 *dev)
{
	return dev->working_state == GB02MAC145;
}

int GB02FUNC768(struct GB02STR37 *dev,
	bi_list *list, bi_list_node *new_cmdbuf_node,
	struct GB02STR18 *GB02STR18)
{
	int counter = 0;
	unsigned long flags = 0;
	bi_list_node *curr_cmdbuf_node = NULL;
	u64 executing_time = 0xffffffffffffffff;
	struct GB02STR37 *smallest_dev = NULL;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	while (1) {
		dev =
		 vcmd_manager[GB02STR18->module_type][dec_data->vcmd_position[GB02STR18->module_type]];
		if (dev->total_exe_time <= executing_time) {
			executing_time = dev->total_exe_time;
			smallest_dev = dev;
		}
		dec_data->vcmd_position[GB02STR18->module_type]++;
		if (dec_data->vcmd_position[GB02STR18->module_type] >=
			dec_data->vcmd_type_core_num[GB02STR18->module_type])
			dec_data->vcmd_position[GB02STR18->module_type] = 0;
		counter++;
		if (counter >=
		 dec_data->vcmd_type_core_num[GB02STR18->module_type])
			break;
	}
    //abort the vcmd and wait
	GB02FUNC321((const void *)smallest_dev->hwregs,
	 smallest_dev->reg_mirror, HWIF_VCMD_START_TRIGGER, 0);
	if (wait_event_interruptible(*smallest_dev->wait_abort_queue,
		GB02FUNC766(smallest_dev)))
		return -ERESTARTSYS;
	//GB02MAC151
	spin_lock_irqsave(smallest_dev->spinlock, flags);
	curr_cmdbuf_node = smallest_dev->list_manager.head;
	while (1) {
		//if list is empty or tail,insert to tail
		if	(curr_cmdbuf_node == NULL)
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

static int GB02FUNC773(bi_list_node *new_cmdbuf_node)
{
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list *list = NULL;
	int ret = -1;
	struct GB02STR37 *dev = NULL;

	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;

	//there is an empty vcmd to be used
	ret = GB02FUNC732(dev, list, new_cmdbuf_node, GB02STR18);
	if (-1 != ret) {
		gb_printf(KERN_INFO, "%s-%d: find empty vcmd, ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = GB02FUNC738(dev, list, new_cmdbuf_node, GB02STR18);
	if (-1 != ret) {
		gb_printf(KERN_INFO, "%s-%d: select use vcmd, ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = GB02FUNC747(dev, list, new_cmdbuf_node, GB02STR18);
	if (-1 != ret) {
		gb_printf(KERN_INFO, "%s-%d: vcmd pend handle, ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}

	if (GB02STR18->priority == GB02MAC150) {
		//calculate total execute time of all devices
		ret = GB02FUNC753(dev, list,
		 new_cmdbuf_node, GB02STR18);
		if (-1 != ret) {
			gb_printf(KERN_ERR, "%s-%d: error, ret = %d\n",
			 __func__, __LINE__, ret);
			return ret;
		}

		//find the device with the least total_exe_time.
		GB02FUNC761(dev, list,
		 new_cmdbuf_node, GB02STR18);
		return 0;
	}
	//calculate total execute time of all devices
	ret = GB02FUNC753(dev, list,
		new_cmdbuf_node, GB02STR18);
	if (-1 != ret) {
		gb_printf(KERN_ERR, "%s-%d: error, ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}

	//find the smallest device.
	GB02FUNC768(dev, list, new_cmdbuf_node, GB02STR18);
	return 0;
}

static void GB02FUNC778(struct GB02STR37 *dev,
	bi_list_node *new_cmdbuf_node)
{
	u32  *jmp_addr = NULL;
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
					 GB02STR18->cmdbuf_virtual_address +
					  (GB02STR18->cmdbuf_size / 4);
					operation_code = *(jmp_addr - 4);
					operation_code =
					 GB02MAC184 | operation_code;
					*(jmp_addr - 4) = operation_code;
					dev->duration_without_int = 0;
				}
			}
		}
	}
}

static void GB02FUNC782(struct GB02STR37 *dev,
	struct GB02STR18 *GB02STR18, struct GB02STR18 *next_cmdbuf_obj)
{
	u32  *jmp_addr = NULL;
	u32 operation_code;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	gb_printf(KERN_INFO, "%s: Link cmdbuf %d to cmdbuf %d", __func__,
		GB02STR18->cmdbuf_id, next_cmdbuf_obj->cmdbuf_id);
	jmp_addr = GB02STR18->cmdbuf_virtual_address +
		(GB02STR18->cmdbuf_size / 4);
	if (dev->hw_version_id > GB02MAC186) {
		//set next cmdbuf id
		*(jmp_addr - 1) = next_cmdbuf_obj->cmdbuf_id;
	}
	if (sizeof(size_t) == 8)
		*(jmp_addr - 2) =
		 (u32)((u64)(next_cmdbuf_obj->cmdbuf_bus_address
		  - dec_data->base_ddr_addr) >> 32);
	else
		*(jmp_addr - 2) = 0;

	*(jmp_addr - 3) = (u32)(next_cmdbuf_obj->cmdbuf_bus_address
		- dec_data->base_ddr_addr);
	operation_code = *(jmp_addr - 4);
	operation_code >>= 16;
	operation_code <<= 16;
	*(jmp_addr - 4) = (u32)(operation_code | GB02MAC185
		| ((next_cmdbuf_obj->cmdbuf_size + 7) / 8));
	next_cmdbuf_obj->cmdbuf_data_linked = 1;
	(dev->sw_cmdbuf_rdy_num)++;
}

static void GB02FUNC785(struct GB02STR37 *dev,
	struct GB02STR18 *next_cmdbuf_obj)
{
	u32  *jmp_addr = NULL;
	u32 operation_code;

	if (next_cmdbuf_obj->has_end_cmdbuf == 0) {
		if (next_cmdbuf_obj->no_normal_int_cmdbuf == 1) {
			dev->duration_without_int +=
			 next_cmdbuf_obj->executing_time;

	//maybe we see the modified nop before abort, so need to write back.
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

void GB02FUNC790(struct GB02STR37  *dev)
{
	u32 result;

	if (dev->hwregs != NULL) {
		//disable interrupt at first
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC515, 0x0000);
		//reset all
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC513, 0x0002);
		//read status register
		result = GB02FUNC313((const void *)dev->hwregs,
			GB02MAC514);
		//clean status register
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC514, result);
	}
}

void GB02FUNC793(struct GB02STR37 *dev,
	bi_list_node *last_linked_cmdbuf_node)
{
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *next_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR18 *next_cmdbuf_obj = NULL;

	new_cmdbuf_node = last_linked_cmdbuf_node;
	//for the first cmdbuf.
	GB02FUNC778(dev, new_cmdbuf_node);

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
			GB02FUNC782(dev,
			 GB02STR18, next_cmdbuf_obj);

			//modify nop code of next cmdbuf
			GB02FUNC785(dev, next_cmdbuf_obj);
		}
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
}

static void GB02FUNC799(const void *hwregs, char *info)
{
#ifdef HANTRO_VCMD_DRIVER_DEBUG
	u32 i, fordebug;

	for (i = 0; i < GB02MAC4; i++) {
		fordebug = GB02FUNC313((const void *)hwregs, i * 4);
		gb_printf(KERN_INFO, "%s: vcmd register %d:0x%x\n", info, i, fordebug);
	}
#endif
}

#ifdef HANTRO_VCMD_DRIVER_DEBUG
static void GB02FUNC801(struct GB02STR37 **device,
	u32 *mirror_index, u32 write_command)
{
	u32 register_index, tmp_mirror_index, register_value;
	struct GB02STR37 *dev;

	dev = *device;
	tmp_mirror_index = *mirror_index;
	if	(dev->vcmd_core_cfg.submodule_axife_addr != 0xffff) {
		register_index = GB02MAC96;
		register_value = 0x02;
		dev->reg_mirror[tmp_mirror_index++] = write_command
		 | (dev->vcmd_core_cfg.submodule_axife_addr
		  + register_index);
		dev->reg_mirror[tmp_mirror_index++] = register_value;

		register_index =  GB02MAC98;
		register_value = 0x00;
		dev->reg_mirror[tmp_mirror_index++] = write_command
		 | (dev->vcmd_core_cfg.submodule_axife_addr
		  + register_index);
		dev->reg_mirror[tmp_mirror_index++] = register_value;
	}
	*mirror_index = tmp_mirror_index;
}

static void GB02FUNC802(struct GB02STR37 **device,
	u32 *mirror_index, u32 write_command)
{
	u64 address = 0;
	u16 mmu_offset_address = 0;
	u32 register_index, tmp_mirror_index, register_value, i;
	struct GB02STR37 *dev;
	u64 address_ext = 0;

	dev = *device;
	tmp_mirror_index = *mirror_index;
	address = GetMMUAddress();
	gb_printf(KERN_INFO, "%s: address = 0x%llx", __func__, address);
	for (i = 0; i < 2; i++) {
		mmu_offset_address = (i == 0) ?
		 dev->vcmd_core_cfg.submodule_MMU_addr
			: dev->vcmd_core_cfg.submodule_MMUWrite_addr;
		if (mmu_offset_address != 0xffff) {
			register_index = GB02MAC172;
			register_value = address;
			dev->reg_mirror[tmp_mirror_index++] = write_command
				| (mmu_offset_address+register_index);
			dev->reg_mirror[tmp_mirror_index++] = register_value;

			register_index = GB02MAC173;
			register_value = address_ext;
			dev->reg_mirror[tmp_mirror_index++] = write_command
				| (mmu_offset_address+register_index);
			dev->reg_mirror[tmp_mirror_index++] = register_value;

			register_index =  GB02MAC170;
			register_value = 0x10000;
			dev->reg_mirror[tmp_mirror_index++] = write_command
				| (mmu_offset_address+register_index);
			dev->reg_mirror[tmp_mirror_index++] = register_value;

			register_index =  GB02MAC170;
			register_value = 0x00000;
			dev->reg_mirror[tmp_mirror_index++] = write_command
				| (mmu_offset_address+register_index);
			dev->reg_mirror[tmp_mirror_index++] = register_value;

			register_index =  GB02MAC171;
			register_value = 1;
			dev->reg_mirror[tmp_mirror_index++] = write_command
				| (mmu_offset_address+register_index);
			dev->reg_mirror[tmp_mirror_index++] = register_value;
		}
	}
	*mirror_index = tmp_mirror_index;
}
#endif

static void ConfigAIIXFE_MMU_BYVCMD(struct GB02STR37 **device)
{
#ifdef HANTROVCMD_ENABLE_IP_SUPPORT
	u32 i = 0;

	u32 mirror_index, register_index, register_value;
	u32 write_command = 0;
	struct GB02STR37 *dev;

	if (!device)
		return;
	dev = *device;
	mirror_index = GB02MAC2;
	write_command = GB02MAC174|(1<<26)|(1<<16);
	//enable AXIFE by VCMD
	GB02FUNC801(device, &mirror_index, write_command);

	//enable MMU by VCMD
	GB02FUNC802(device, &mirror_index, write_command);

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

void GB02FUNC813(struct GB02STR37 *dev,
	bi_list_node *first_linked_cmdbuf_node)
{
	struct GB02STR18 *GB02STR18 = NULL;

	gb_printf(KERN_INFO, "%s:%d, dev = 0x%px, cmdbuf_id = %d \n", __func__, __LINE__, dev,
		((struct GB02STR18 *)first_linked_cmdbuf_node->data)->cmdbuf_id);
	if (dev->working_state == GB02MAC145) {
		if ((first_linked_cmdbuf_node != NULL)
		 && dev->sw_cmdbuf_rdy_num) {
			GB02STR18 =
			 (struct GB02STR18 *)first_linked_cmdbuf_node->data;
			GB02FUNC799((const void *)dev->hwregs,
			 "func vcmd start exits");
			//0x40
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
			//0x48
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
			if (sizeof(size_t) == 8)
				GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
				(u32)((u64)(GB02STR18->cmdbuf_bus_address)
				 >> 32));
			else
				GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_EXECUTING_CMD_ADDR_MSB, 0);

			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_EXE_CMDBUF_LENGTH,
				(u32)((GB02STR18->cmdbuf_size + 7) / 8));
			GB02FUNC342(dev->reg_mirror,
			 HWIF_VCMD_RDY_CMDBUF_COUNT, dev->sw_cmdbuf_rdy_num);
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
				0xffffffff);//not interrupt cpu
			dev->working_state = GB02MAC146;

			ConfigAIIXFE_MMU_BYVCMD(&dev);

			//start
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_MASTER_OUT_CLK_GATE_DISABLE, 0);
			GB02FUNC342(dev->reg_mirror,
				HWIF_VCMD_START_TRIGGER, 1);
			GB02FUNC318((const void *)dev->hwregs, 0x40,
				dev->reg_mirror[0x40 / 4]);

			GB02FUNC799((const void *)dev->hwregs,
			 "func vcmd start exits");
		}
	}
}


static bi_list_node *GB02FUNC827(struct GB02STR3 vcmd_buff,
	struct drm_gb_cmdbuf_resv *input_para)
{
	bi_list_node *current_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR14  new_cmdbuf_addr;
	struct GB02STR14  new_status_cmdbuf_addr;

	if (wait_event_interruptible(vcmd_cmdbuf_memory_wait,
	 GB02FUNC721(&new_cmdbuf_addr, &new_status_cmdbuf_addr, vcmd_buff, input_para)))
		return NULL;
	GB02STR18 = GB02FUNC249();
	if (GB02STR18 == NULL) {
		gb_printf(KERN_ERR, "%s: GB02FUNC249 fail!\n", __func__);
		GB02FUNC727(new_cmdbuf_addr.cmdbuf_id);
		return NULL;
	}
	GB02STR18->cmdbuf_bus_address = new_cmdbuf_addr.bus_address;
	GB02STR18->cmdbuf_virtual_address = new_cmdbuf_addr.virtual_address;
	GB02STR18->cmdbuf_size = new_cmdbuf_addr.size;
	GB02STR18->cmdbuf_id = new_cmdbuf_addr.cmdbuf_id;

	GB02STR18->status_bus_address = new_status_cmdbuf_addr.bus_address;
	GB02STR18->status_virtual_address =
		new_status_cmdbuf_addr.virtual_address;
	GB02STR18->status_size = new_status_cmdbuf_addr.size;
	current_node = GB02FUNC215();
	if (current_node == NULL) {
		gb_printf(KERN_ERR, "%s: GB02FUNC215 fail!\n", __func__);
		GB02FUNC727(new_cmdbuf_addr.cmdbuf_id);
		GB02FUNC254(GB02STR18);
		return NULL;
	}
	current_node->data = (void *)GB02STR18;
	current_node->next = NULL;
	current_node->previous = NULL;
	return current_node;
}


long GB02FUNC832(struct drm_file *filp,
	struct drm_gb_cmdbuf_resv *input_para, bi_list *global_process_manager,
	bi_list_node **global_cmdbuf_node, struct GB02STR3 vcmd_buff)
{
	u32 module_type;
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list_node *process_manager_node = NULL;
	struct GB02STR43 *dec_data = NULL;
	struct GB02STR17 *GB02STR17 = NULL;
	unsigned long flags;

	input_para->cmdbuf_id = 0;
	dec_data = GB02FUNC700();

	if (input_para->cmdbuf_size > 2 * GB02MAC156)
		return -1;

	spin_lock_irqsave(&vcmd_process_manager_lock, flags);
	process_manager_node = global_process_manager->head;
	while (1) {
		if (process_manager_node == NULL) {
			//should not happen
			gb_printf(KERN_ERR, "hantrovcmd: ERROR process_manager_node !!\n");
			spin_unlock_irqrestore(&vcmd_process_manager_lock,
			 flags);
			return -1;
		}
		GB02STR17 =
		 (struct GB02STR17 *)process_manager_node->data;
		gb_printf(KERN_INFO, "%s: node %p, filp %p\n", __func__,
		 (void *)process_manager_node,
		  (void *)GB02STR17->filp);
		if (filp == GB02STR17->filp)
			break;
		process_manager_node = process_manager_node->next;
	}
	spin_unlock_irqrestore(&vcmd_process_manager_lock, flags);

	spin_lock_irqsave(&GB02STR17->spinlock, flags);
	GB02STR17->total_exe_time += input_para->executing_time;
	spin_unlock_irqrestore(&GB02STR17->spinlock, flags);
	if (wait_event_interruptible(GB02STR17->wait_queue,
		GB02FUNC256(GB02STR17))) {
		gb_printf(KERN_ERR, "%s:%d wait resource rdy fail----\n", __func__, __LINE__);
		return -1;
	}

	module_type = input_para->module_type;
	if (down_interruptible(&dec_data->vcmd_reserve_cmdbuf_sem[module_type]))
		return -ERESTARTSYS;
	new_cmdbuf_node = GB02FUNC827(vcmd_buff, input_para);
	if (new_cmdbuf_node == NULL) {
		gb_printf(KERN_ERR, "%s:%d create cmdbuf node fail----\n", __func__, __LINE__);
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
	up(&dec_data->vcmd_reserve_cmdbuf_sem[module_type]);

	return 0;
}

void GB02FUNC841(struct GB02STR37 *dev, u32 *jmp_addr,
	 struct GB02STR18 *GB02STR18)
{
	if (dev->hw_version_id > GB02MAC186) {
		jmp_addr = GB02STR18->cmdbuf_virtual_address;

		if (sizeof(size_t) == 8)
			*(jmp_addr + 2) =
			 (u32)((u64)(dev->vcmd_reg_mem_bus_address +
			  (GB02MAC143 + 1) * 4) >> 32);
		else
			*(jmp_addr + 2) = 0;

		*(jmp_addr + 1) = (u32)((dev->vcmd_reg_mem_bus_address +
			(GB02MAC143 + 1) * 4));

		jmp_addr = GB02STR18->cmdbuf_virtual_address +
			(GB02STR18->cmdbuf_size / 4);

		if (sizeof(size_t) == 8)
			*(jmp_addr - 6) =
			 (u32)((u64)dev->vcmd_reg_mem_bus_address >> 32);
		else
			*(jmp_addr - 6) = 0;

		*(jmp_addr - 7) = (u32)(dev->vcmd_reg_mem_bus_address);
	}
}


long GB02FUNC845(struct drm_file *filp,
	int cmdbuf_id, int cmdbuf_size, int *core_id)
{
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *last_cmdbuf_node;
	u32 *jmp_addr = NULL;
	u32 opCode, opcode_bak;
	u32 tempOpcode;
	u32 record_last_cmdbuf_rdy_num;
	struct GB02STR37 *dev = NULL;
	unsigned long flags;
	int return_value;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
	if (new_cmdbuf_node == NULL) {
		//should not happen
		gb_printf(KERN_ERR, "hantrovcmd: ERROR cmdbuf_id !!\n");
		return -1;
	}
	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
	if (GB02STR18->filp != filp) {
		//should not happen
		gb_printf(KERN_ERR, "hantrovcmd: ERROR cmdbuf_id !!\n");
		return -1;
	}
	GB02STR18->cmdbuf_data_loaded = 1;
	GB02STR18->cmdbuf_size = cmdbuf_size;
	GB02STR18->waited = 0;
	gb_printf(KERN_INFO, "%s:%d--cmdbuf size=0x%x, cmdbuf obj = 0x%llx-\n",
		__func__, __LINE__, cmdbuf_size, (u64)GB02STR18);
	//test nop and end opcode, then assign value.
	GB02STR18->has_end_cmdbuf = 0; //0: has jmp opcode,1 has end code
	GB02STR18->no_normal_int_cmdbuf = 0;
	jmp_addr = GB02STR18->cmdbuf_virtual_address +
		(GB02STR18->cmdbuf_size / 4);
	opcode_bak = opCode = tempOpcode = *(jmp_addr - 4);
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
		gb_printf(KERN_ERR, "%s:%d--not support other opcode = 0x%x \n",
			  __func__, __LINE__, opCode);
		/* not support other opcode */
		return -1;
	}

	if (down_interruptible(&dec_data->vcmd_reserve_cmdbuf_sem[GB02STR18->module_type]))
		return -ERESTARTSYS;

	return_value = GB02FUNC773(new_cmdbuf_node);
	if (return_value) {
		up(&dec_data->vcmd_reserve_cmdbuf_sem[GB02STR18->module_type]);
		return return_value;
	}
	dev = &hantrovcmd_data[GB02STR18->core_id];
	*core_id = GB02STR18->core_id;
	gb_printf(KERN_INFO, "Allocate cmd buffer [%d]\n", cmdbuf_id);

	//set ddr address for vcmd registers copy.
	GB02FUNC841(dev, jmp_addr, GB02STR18);

	//start to link and/or run
	spin_lock_irqsave(dev->spinlock, flags);
	last_cmdbuf_node = GB02FUNC260(new_cmdbuf_node);
	record_last_cmdbuf_rdy_num = dev->sw_cmdbuf_rdy_num;
	GB02FUNC793(dev, last_cmdbuf_node);
	tempOpcode = *(jmp_addr - 4);
	*(jmp_addr - 4) = opcode_bak | (tempOpcode & 0x2000000);

	if (dev->working_state == GB02MAC145) {
		//run
		if (filp == NULL)
			((struct GB02STR18 *)last_cmdbuf_node->data)->\
				cmdbuf_run_done = 0;
		while (last_cmdbuf_node &&
		 ((struct GB02STR18 *)last_cmdbuf_node->data)->cmdbuf_run_done) {
			gb_printf(KERN_INFO, "%s:%d-----while---\n", __func__, __LINE__);
			last_cmdbuf_node = last_cmdbuf_node->next;
		}

		if (last_cmdbuf_node && last_cmdbuf_node->data) {
			gb_printf(KERN_INFO, "%s:%d---vcmd start for cmdbuf id %d,\
				cmdbuf_run_done = %d\n",
			__func__, __LINE__,
			 ((struct GB02STR18 *)last_cmdbuf_node->data)->cmdbuf_id,
			  ((struct GB02STR18 *)last_cmdbuf_node->data)->cmdbuf_run_done);
		}
		GB02FUNC813(dev, last_cmdbuf_node);
	} else {
		//just update cmdbuf ready number
		if	(record_last_cmdbuf_rdy_num != dev->sw_cmdbuf_rdy_num)
			GB02FUNC321((const void *)dev->hwregs,
			dev->reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT,
			dev->sw_cmdbuf_rdy_num);
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	up(&dec_data->vcmd_reserve_cmdbuf_sem[GB02STR18->module_type]);

	return 0;
}

/******************************************************************************/
static int GB02FUNC858(struct GB02STR37 *dev,
	struct GB02STR18 *GB02STR18, u32 *irq_status_ret)
{
	int rdy = 0;
	unsigned long flags;

	spin_lock_irqsave(dev->spinlock, flags);
	if (GB02STR18->cmdbuf_run_done) {
		rdy = 1;
		*irq_status_ret = GB02STR18->executing_status;
		gb_printf(KERN_INFO, "%s: cmdbuf_id = %d, rdy = 1\n",
			  __func__, GB02STR18->cmdbuf_id);
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	return rdy;
}

/******************************************************************************/
static int GB02FUNC861(struct drm_file *filp,
	struct GB02STR18 *GB02STR18, u32 *irq_status_ret)
{
	int k;
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR37 *dev = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	for (k = 0; k < GB02MAC159; k++) {
		new_cmdbuf_node = dec_data->global_cmdbuf_node[k];
		if (new_cmdbuf_node == NULL)
			continue;

		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if (!GB02STR18 || GB02STR18->filp != filp)
			continue;

		dev = &hantrovcmd_data[GB02STR18->core_id];
		if (GB02FUNC858(dev, GB02STR18, irq_status_ret) == 1) {
			/* Return cmdbuf_id when GB02MAC235 is used. */
			if (!GB02STR18->waited) {
				*irq_status_ret = GB02STR18->cmdbuf_id;
				GB02STR18->waited = 1;
				return 1;
			}
		}
	}

	return 0;
}

unsigned int GB02FUNC868(struct drm_file *filp,
	u32 cmdbuf_id, u32 *irq_status_ret)
{
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR37 *dev = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	if (cmdbuf_id != GB02MAC235) {
		new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			/* should not happen */
			gb_printf(KERN_ERR, "%s: ERROR cmdbuf_id !!\n", __func__);
			return -1;
		}
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if (GB02STR18->filp != filp) {
			/* should not happen */
			gb_printf(KERN_ERR, "%s: ERROR cmdbuf_id !!\n", __func__);
			return -1;
		}
		dev = &hantrovcmd_data[GB02STR18->core_id];
		if (wait_event_interruptible(*dev->wait_queue,
			GB02FUNC858(dev, GB02STR18, irq_status_ret))) {
			gb_printf(KERN_ERR, "%s: vcmd_wait error interrupted\n", __func__);
			return -ERESTARTSYS;
		}
		return 0;
	}
	if (GB02FUNC861(filp, GB02STR18, irq_status_ret)) {
		gb_printf(KERN_INFO, "%s:%d--1-end--wait event interrput-cmdbuf id=%d----\n", __func__, __LINE__, cmdbuf_id);
		return 0;
	}
	if (wait_event_interruptible(dec_data->mc_wait_queue,
		GB02FUNC861(filp, GB02STR18, irq_status_ret))) {
		gb_printf(KERN_ERR, "%s: multicore error interrupt\n", __func__);
		return -ERESTARTSYS;
	}
	return 0;
}

static void create_read_all_registers_cmdbuf
(struct drm_gb_cmdbuf_resv *input_para, bi_list_node **global_cmdbuf_node)
{
	u32 register_range[] = { GB02MAC189,
	 GB02MAC190,
	  GB02MAC192, GB02MAC193};
	u32 counter_cmdbuf_size = 0;
	u32  *set_base_addr;
	ptr_t status_base_phy_addr;
	u32 offset_inc = 0;
	u16 submodule_main_addr =
	 vcmd_manager[input_para->module_type][0]->vcmd_core_cfg.submodule_main_addr;
	u16 submodule_L2Cache_addr =
	 vcmd_manager[input_para->module_type][0]->vcmd_core_cfg.submodule_L2Cache_addr;
	struct GB02STR14 *vcmd_buf_mem_pool = NULL;
	struct GB02STR14 *vcmd_status_buf_mem_pool = NULL;
	struct GB02STR43 *dec_data = NULL;
	struct GB02STR18 *cmdbuf =
	 (struct GB02STR18 *)(global_cmdbuf_node[input_para->cmdbuf_id]->data);

	dec_data = GB02FUNC700();
	vcmd_buf_mem_pool = &dec_data->dec_vcmd_mem_pool[0];
	vcmd_status_buf_mem_pool = &dec_data->dec_vcmd_mem_pool[1];

	set_base_addr = cmdbuf->cmdbuf_virtual_address;
	gb_printf(KERN_INFO, "%s:%d----cmdbuf bus addr=%lx-vaddr=%llx-\n",
		__func__, __LINE__, cmdbuf->cmdbuf_bus_address, (u64)set_base_addr);
	status_base_phy_addr = cmdbuf->status_bus_address +
	(submodule_main_addr / 2 + 0);
	if (vcmd_manager[input_para->module_type][0]->hw_version_id >
	 GB02MAC186) {
		gb_printf(KERN_INFO, "%s:create cmdbuf data when hw_version_id = 0x%x\n",
		 __func__,
		  vcmd_manager[input_para->module_type][0]->hw_version_id);

		*(set_base_addr + 0) = (GB02MAC177) | (1 << 16)
			| (GB02MAC143 * 4);
		counter_cmdbuf_size += 4;
		*(set_base_addr + 1) = (u32)0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 2) = (u32)0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 3) = 0;		//alignment
		counter_cmdbuf_size += 4;

		//read main IP all registers
		*(set_base_addr + 4) = (GB02MAC177)
			| ((register_range[input_para->module_type] / 4) << 16)
			| (submodule_main_addr + 0);
		counter_cmdbuf_size += 4;

		*(set_base_addr + 5) = (u32)(status_base_phy_addr);
		counter_cmdbuf_size += 4;

		if (sizeof(size_t) == 8)
			*(set_base_addr + 6) =
			 (u32)((u64)(status_base_phy_addr) >> 32);
		else
			*(set_base_addr + 6) = 0;

		counter_cmdbuf_size += 4;

		//alignment
		*(set_base_addr + 7) = 0;
		counter_cmdbuf_size += 4;
		if (submodule_L2Cache_addr != 0xffff) {
			offset_inc = 4;
			status_base_phy_addr =
			 cmdbuf->status_bus_address +
			   (submodule_L2Cache_addr / 2 + 0);
			/* read L2cache IP first register */
			*(set_base_addr + 8) = (GB02MAC177) | (1 << 16)
						| (submodule_L2Cache_addr + 0);
			counter_cmdbuf_size += 4;
			*(set_base_addr + 9) = (u32)(status_base_phy_addr -
				dec_data->base_ddr_addr);

			counter_cmdbuf_size += 4;
			if (sizeof(size_t) == 8) {
				*(set_base_addr + 10) =
				 (u32)((u64)(status_base_phy_addr -
				  dec_data->base_ddr_addr) >> 32);
			} else {
				*(set_base_addr + 10) = 0;
			}

			counter_cmdbuf_size += 4;
			//alignment
			*(set_base_addr + 11) = 0;
			counter_cmdbuf_size += 4;
		}

		/* read vcmd registers to ddr */
		*(set_base_addr + 8 + offset_inc) =
		 (GB02MAC177) | (27 << 16) | (0);
		counter_cmdbuf_size += 4;
		*(set_base_addr + 9 + offset_inc) = (u32)0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 10 + offset_inc) = (u32)0;
		counter_cmdbuf_size += 4;
		//alignment
		*(set_base_addr + 11 + offset_inc) = 0;
		counter_cmdbuf_size += 4;
		//JMP RDY = 0
		*(set_base_addr + 12 + offset_inc) =
		 (GB02MAC182) | 0 | GB02MAC184 | 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 13 + offset_inc) = 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 14 + offset_inc) = 0;
		counter_cmdbuf_size += 4;
		*(set_base_addr + 15 + offset_inc) = input_para->cmdbuf_id;
		/*
		 * don't add the last alignment DWORD in order to
		 * identify END command or JMP command.
		 */
		input_para->cmdbuf_size = (16 + offset_inc) * 4;
	} else {
		gb_printf(KERN_INFO, "%s: create cmdbuf when hw_version_id = 0x%x\n",
		 __func__,
		  vcmd_manager[input_para->module_type][0]->hw_version_id);
		//read all registers
		*(set_base_addr + 0) = (GB02MAC177)
			| ((register_range[input_para->module_type] / 4) << 16)
			| (submodule_main_addr + 0);
		counter_cmdbuf_size += 4;
		*(set_base_addr + 1) =
		 (u32)(status_base_phy_addr - dec_data->base_ddr_addr);
		counter_cmdbuf_size += 4;

		if (sizeof(size_t) == 8)
			*(set_base_addr + 2) =
			 (u32)((u64)(status_base_phy_addr -
			  dec_data->base_ddr_addr) >> 32);
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
		/*
		 * don't add the last alignment DWORD in order to
		 * identify END command or JMP command.
		 */
		input_para->cmdbuf_size = 8 * 4;
	}
}
static void GB02FUNC888(u32 main_module_type,
	struct GB02STR37 *hantrovcmd_data)
{
	int ret;
	int core_id;
	struct drm_gb_cmdbuf_resv input_para;
	u32 irq_status_ret = 0;
	u32 *status_base_virt_addr;
	struct GB02STR14 *vcmd_status_buf_mem_pool = NULL;
	u16 submodule_main_addr;
	struct GB02STR41 *hw_info;
	struct GB02STR43 *dec_data = NULL;
	dec_data = GB02FUNC700();
	vcmd_status_buf_mem_pool = &dec_data->dec_vcmd_mem_pool[1];
	hw_info = &dec_data->hw_info;

	input_para.executing_time = 0;
	input_para.module_type = main_module_type;
	input_para.cmdbuf_size = 0;
	input_para.priority = GB02MAC150;
	submodule_main_addr =
	 vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_main_addr;
	ret = GB02FUNC832(NULL, &input_para,
		&dec_data->global_process_manager,
		 dec_data->global_cmdbuf_node, dec_data->vcmd_buff);
	vcmd_manager[main_module_type][0]->status_cmdbuf_id
	 = input_para.cmdbuf_id;
	create_read_all_registers_cmdbuf(&input_para,
			dec_data->global_cmdbuf_node);
	GB02FUNC845(NULL, input_para.cmdbuf_id,
		input_para.cmdbuf_size, &core_id);
	if (hw_info->hw_id != 0)
		return;
	GB02FUNC868(NULL, input_para.cmdbuf_id, &irq_status_ret);
	status_base_virt_addr = dec_data->vcmd_buff.vir_buff +
		input_para.cmdbuf_id * GB02MAC156 +
		(submodule_main_addr / 2 + 0);
	hw_info->hw_id = *status_base_virt_addr;
	hw_info->build_id = *(status_base_virt_addr + 309);
	hw_info->synth_cfg = *(status_base_virt_addr + 50);
	hw_info->synth_cfg_2 = *(status_base_virt_addr + 54);
	hw_info->synth_cfg_3 = *(status_base_virt_addr + 56);
	hw_info->pp_synth_cfg = *(status_base_virt_addr + 60);
	hw_info->fuse_cfg = *(status_base_virt_addr + 57);
	hw_info->pp_cfg_stat = *(status_base_virt_addr + 260);

	gb_printf(KERN_INFO, "%s: dec reg 0:0x%x\n", __func__, *status_base_virt_addr);
	gb_printf(KERN_INFO, "%s: reg 50:0x%08x\n", __func__, *(status_base_virt_addr + 50));
	gb_printf(KERN_INFO, "%s: reg 54:0x%08x\n", __func__, *(status_base_virt_addr + 54));
	gb_printf(KERN_INFO, "%s: reg 56:0x%08x\n", __func__, *(status_base_virt_addr + 56));
	gb_printf(KERN_INFO, "%s: reg 309:0x%08x\n", __func__,
	 *(status_base_virt_addr + 309));

	gb_printf(KERN_INFO, "%s: hw id 0:0x%x\n", __func__, *status_base_virt_addr);
	gb_printf(KERN_INFO, "%s: synth cfg:0x%08x\n", __func__, *(status_base_virt_addr + 50));
	gb_printf(KERN_INFO, "%s: synth cfg2:0x%08x\n", __func__, *(status_base_virt_addr + 54));
	gb_printf(KERN_INFO, "%s: synth cfg3:0x%08x\n", __func__, *(status_base_virt_addr + 56));
	gb_printf(KERN_INFO, "%s: pp synth cfg:0x%08x\n", __func__, *(status_base_virt_addr + 60));
	gb_printf(KERN_INFO, "%s: fuse cfg:0x%08x\n", __func__, *(status_base_virt_addr + 57));
	gb_printf(KERN_INFO, "%s: build id:0x%08x\n", __func__,
	 *(status_base_virt_addr + 309));
}

int GB02FUNC898(struct drm_file *filp, void **dec_priv, void *data)
{
	int result = 0;
	unsigned long flags;
	struct drm_gem_object *vcmd_obj = NULL;
	struct GB02STR37 *dev = hantrovcmd_data;
	bi_list_node *process_manager_node;
	struct GB02STR17 *GB02STR17 = NULL;
	struct GB02STR43 *dec_data = NULL;
	struct GB02STR97 *args = (struct GB02STR97 *)data;

	dec_data = GB02FUNC700();
	vcmd_obj = dec_data->vcmd_pool->obj;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	result = drm_vma_node_allow(&vcmd_obj->vma_node, filp);
#else
	result = drm_vma_node_allow(&vcmd_obj->vma_node, filp->filp);
#endif
	args->offset_va = dec_data->vcmd_pool->flp_offset;
	args->offset_pa = dec_data->vcmd_pool->offset;
	gb_printf(KERN_INFO, "%s:%d vcmdbuf offset_va=0x%llx offset_pa=0x%llx\n",
		__func__, __LINE__, args->offset_va, args->offset_pa);

/*
	if (filp->driver_priv != NULL)
		kfree(filp->driver_priv);
	filp->driver_priv = (void *) dev;
*/
	*dec_priv = (void *)dev;

	process_manager_node = GB02FUNC243();
	if (process_manager_node == NULL)
		return -1;
	GB02STR17 =
		(struct GB02STR17 *)process_manager_node->data;
	GB02STR17->filp = filp;
	spin_lock_irqsave(&vcmd_process_manager_lock, flags);
	GB02FUNC210(&dec_data->global_process_manager,
		process_manager_node);
	spin_unlock_irqrestore(&vcmd_process_manager_lock, flags);

	gb_printf(KERN_INFO, "%s: finish process node %p for filp opened %p\n",
		__func__, (void *)process_manager_node, (void *)filp);
	return result;
}

void GB02FUNC904(struct GB02STR37 *dev,
	struct GB02STR18 *restart_cmdbuf, u32 core_id)
{
	if (restart_cmdbuf) {
		u32 irq_status1, irq_status2;
		struct GB02STR43 *dec_data = NULL;

		dec_data = GB02FUNC700();

		gb_printf(KERN_INFO, "%s: Restart from cmdbuf [%d] after aborting.\n",
			__func__, restart_cmdbuf->cmdbuf_id);
		irq_status1 = GB02FUNC313((const void *)dev->hwregs,
			GB02MAC514);
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC514, irq_status1);
		irq_status2 = GB02FUNC313((const void *)dev->hwregs,
			GB02MAC514);
		gb_printf(KERN_INFO, "%s: Clear irq status from 0x%0x -> 0x%0x\n",
			__func__, irq_status1, irq_status2);

		GB02FUNC321((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR,
			(u32)(restart_cmdbuf->cmdbuf_bus_address
			 - dec_data->base_ddr_addr));
		if (sizeof(size_t) == 8)
			GB02FUNC321((const void *)dev[core_id].hwregs,
			 dev[core_id].reg_mirror,
			  HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
			(u32)((u64)(restart_cmdbuf->cmdbuf_bus_address -
			dec_data->base_ddr_addr) >> 32));
		else
			GB02FUNC321((const void *)dev[core_id].hwregs,
			 dev[core_id].reg_mirror,
			  HWIF_VCMD_EXECUTING_CMD_ADDR_MSB, 0);

		GB02FUNC321((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_EXE_CMDBUF_COUNT, 0);
		GB02FUNC321((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_EXE_CMDBUF_LENGTH,
			(u32)((restart_cmdbuf->cmdbuf_size + 7) / 8));
		GB02FUNC321((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID,
			restart_cmdbuf->cmdbuf_id);
		gb_printf(KERN_INFO, "%s: dev->sw_cmdbuf_rdy_num is %d\n", __func__,
			dev[core_id].sw_cmdbuf_rdy_num);
		GB02FUNC321((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT,
			dev[core_id].sw_cmdbuf_rdy_num);
		GB02FUNC799((const void *)dev->hwregs,
			"before restart");
		GB02FUNC321((const void *)dev[core_id].hwregs,
		 dev[core_id].reg_mirror, HWIF_VCMD_START_TRIGGER, 1);

		gb_printf(KERN_INFO, "%s: Restart from cmdbuf [%d] : start trigger = %d.\n",
		 __func__, restart_cmdbuf->cmdbuf_id,
		  GB02FUNC325((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_START_TRIGGER));
		gb_printf(KERN_INFO, "%s: dev state from %d -> WORKING.\n",
			__func__, dev[core_id].working_state);
		dev[core_id].working_state = GB02MAC146;
		GB02FUNC799((const void *)dev->hwregs,
			"after restart");
	} else {
		gb_printf(KERN_INFO, "%s: No more command buffer to be restarted!\n",
		 __func__);
	}
}

/* Update the last JMP cmd in cmdbuf_ojb in order to jump to next_cmdbuf_obj. */
static void GB02FUNC908(int hw_version_id,
	struct GB02STR18 *GB02STR18, struct GB02STR18 *next_cmdbuf_obj,
	int jmp_IE_1)
{
	u32 *jmp_addr;
	u32 operation_code;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	if (!GB02STR18)
		return;

	if (GB02STR18->has_end_cmdbuf == 0) {
		//need to link, current cmdbuf link to next cmdbuf
		jmp_addr = GB02STR18->cmdbuf_virtual_address +
			(GB02STR18->cmdbuf_size / 4);
		if (!next_cmdbuf_obj) {
			// If next cmdbuf is not available, set the RDY to 0.
			operation_code = *(jmp_addr-4);
			operation_code >>= 16;
			operation_code <<= 16;
			*(jmp_addr - 4) = (u32)(operation_code & ~GB02MAC185);
		} else {
			if (hw_version_id > GB02MAC186) {
				//set next cmdbuf id
				*(jmp_addr - 1) = next_cmdbuf_obj->cmdbuf_id;
			}

			if (sizeof(size_t) == 8) {
				*(jmp_addr - 2) =
				 (u32)((u64)(next_cmdbuf_obj->cmdbuf_bus_address
				  - dec_data->base_ddr_addr) >> 32);
			} else {
				*(jmp_addr - 2) = 0;
			}
			*(jmp_addr - 3) =
			 (u32)(next_cmdbuf_obj->cmdbuf_bus_address -
			  dec_data->base_ddr_addr);

			operation_code = *(jmp_addr - 4);
			operation_code >>= 16;
			operation_code <<= 16;
			*(jmp_addr - 4) = (u32)(operation_code | GB02MAC185
				| jmp_IE_1 |
				 ((next_cmdbuf_obj->cmdbuf_size + 7) / 8));
		}
	}
}

void GB02FUNC912(struct GB02STR37 *dev,
	bi_list_node *cmdbuf_node)
{
	bi_list *list = &dev->list_manager;
	struct GB02STR18 *GB02STR18 =
		(struct GB02STR18 *)cmdbuf_node->data;
	bi_list_node *prev = cmdbuf_node->previous;
	bi_list_node *next = cmdbuf_node->next;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	gb_printf(KERN_INFO, "%s: Delink and remove cmdbuf [%d] from vcmd list.\n",
		__func__, GB02STR18->cmdbuf_id);
	GB02FUNC225(list, cmdbuf_node);
	dec_data->global_cmdbuf_node[GB02STR18->cmdbuf_id] = NULL;
	GB02FUNC971(cmdbuf_node);
	GB02FUNC908(dev->hw_version_id, prev ? prev->data : NULL,
		next ? next->data : NULL,
		dev->duration_without_int > GB02MAC154);
}

static long GB02FUNC915(bi_list *list, bi_list_node *cmdbuf_node)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	/*get cmdbuf object according to cmdbuf_id*/
	new_cmdbuf_node = cmdbuf_node;
	if (new_cmdbuf_node == NULL)
		return -1;
	//remove node from list
	new_cmdbuf_node = GB02FUNC176(list, new_cmdbuf_node);
	if (new_cmdbuf_node) {
		//free node
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		dec_data->global_cmdbuf_node[GB02STR18->cmdbuf_id] = NULL;
		GB02FUNC971(new_cmdbuf_node);
		return 0;
	}
	return 1;
}

int GB02FUNC919(struct GB02STR37 *dev,
	bi_list_node *new_cmdbuf_node, struct GB02STR18 *cmdbuf_obj_temp,
	struct GB02STR18 *restart_cmdbuf, u32 core_id)
{
	int vcmd_aborted = 0;
	long retVal = 0;
	unsigned long flags = 0;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	if (cmdbuf_obj_temp->cmdbuf_run_done) {
		cmdbuf_obj_temp->cmdbuf_need_remove = 1;
		retVal = GB02FUNC915(&dev[core_id].list_manager,
			new_cmdbuf_node);
		if (retVal == 1)
			cmdbuf_obj_temp->GB02STR17 = NULL;
	} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 0) {
		cmdbuf_obj_temp->cmdbuf_data_linked = 1;
		cmdbuf_obj_temp->cmdbuf_run_done = 1;
		cmdbuf_obj_temp->cmdbuf_need_remove = 1;
		retVal = GB02FUNC915(&dev[core_id].list_manager,
			new_cmdbuf_node);
		if (retVal == 1)
			cmdbuf_obj_temp->GB02STR17 = NULL;
	} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 1
		&& dev[core_id].working_state == GB02MAC145) {
		GB02FUNC912(&dev[core_id], new_cmdbuf_node);
		if (restart_cmdbuf == cmdbuf_obj_temp)
			restart_cmdbuf =
				new_cmdbuf_node->next ?
				 new_cmdbuf_node->next->data : NULL;
		if (restart_cmdbuf) {
			gb_printf(KERN_INFO, "%s: Set restart cmdbuf [%d].\n",
				__func__, restart_cmdbuf->cmdbuf_id);
		} else {
			gb_printf(KERN_INFO, "%s: Set restart cmdbuf to NULL.\n", __func__);
		}
	} else if (cmdbuf_obj_temp->cmdbuf_data_linked == 1
		&& dev[core_id].working_state == GB02MAC146) {
		bi_list_node *last_cmdbuf_node = NULL;
		bi_list_node *done_cmdbuf_node = NULL;
		int abort_cmdbuf_id;
		int loop_count = 0;

		//abort the vcmd and wait
		gb_printf(KERN_INFO, "%s: Abort due linked cmdbuf %d of current process.\n",
			__func__, cmdbuf_obj_temp->cmdbuf_id);
		GB02FUNC799((const void *)dev->hwregs,
			"Before trigger to 0");

		// disable abort interrupt
		GB02FUNC321((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_START_TRIGGER, 0);
		vcmd_aborted = 1;

		GB02FUNC799((const void *)dev->hwregs,
			"After trigger to 0");
		// Wait vcmd core aborted and vcmd enters IDLE mode.
		while (GB02FUNC325((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_WORK_STATE)) {
			loop_count++;
			if (!(loop_count % 10)) {
				u32 irq_status =
				 GB02FUNC313((const void *)dev->hwregs,
				  GB02MAC514);

				gb_printf(KERN_INFO, "%s: expected idle state, irq status = 0x%0x\n",
					__func__, irq_status);
				gb_printf(KERN_INFO, "%s: vcmd current status is %d\n",
				 __func__,
				  GB02FUNC325((const void *)dev->hwregs,
					dev[core_id].reg_mirror,
					 HWIF_VCMD_WORK_STATE));
			}
			mdelay(10);  // wait 10ms
			if (loop_count > 100) { // too long
				gb_printf(KERN_ERR, "%s: long before vcmd core IDLE state\n",
				 __func__);
				spin_unlock_irqrestore(dev[core_id].spinlock,
				 flags);
				up(&dec_data->vcmd_reserve_cmdbuf_sem[dev->vcmd_core_cfg.sub_module_type]);
				return -ERESTARTSYS;
			}
		}
		dev[core_id].working_state = GB02MAC145;
		// clear interrupt & restore abort_e
		if (GB02FUNC325((const void *)dev[core_id].hwregs,
				dev[core_id].reg_mirror, HWIF_VCMD_IRQ_ABORT)) {
			gb_printf(KERN_INFO, "%s: Abort interrupt triggered, clear it\n",
			 __func__);
			GB02FUNC318((const void *)dev->hwregs,
				GB02MAC514, 0x1 << 4);
			gb_printf(KERN_INFO, "%s: Now irq status = 0x%0x.\n", __func__,
				GB02FUNC313((const void *)dev->hwregs,
					GB02MAC514));
		}

		abort_cmdbuf_id =
		 GB02FUNC325((const void *)dev[core_id].hwregs,
		  dev[core_id].reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID);
		gb_printf(KERN_INFO, "%s: Abort when executing cmd buf %d.\n",
			__func__, abort_cmdbuf_id);
		dev[core_id].sw_cmdbuf_rdy_num = 0;
		dev[core_id].duration_without_int = 0;
		GB02FUNC321((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_EXE_CMDBUF_COUNT, 0);
		GB02FUNC321((const void *)dev[core_id].hwregs,
			dev[core_id].reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT, 0);

		/* Mark cmdbuf_run_done to 1 for all the cmd buf executed. */
		done_cmdbuf_node = dev[core_id].list_manager.head;
		while (done_cmdbuf_node) {
			if (!((struct GB02STR18 *)done_cmdbuf_node->data)->cmdbuf_run_done) {
				((struct GB02STR18 *)done_cmdbuf_node->data)->cmdbuf_run_done = 1;
				((struct GB02STR18 *)done_cmdbuf_node->data)->cmdbuf_data_linked = 0;
				gb_printf(KERN_INFO, "%s: Set cmdbuf [%d] run_done to 1.\n", __func__,
					((struct GB02STR18 *)done_cmdbuf_node->data)->cmdbuf_id);
			}
			if (((struct GB02STR18 *)done_cmdbuf_node->data)->cmdbuf_id
					== abort_cmdbuf_id)
				break;
			done_cmdbuf_node = done_cmdbuf_node->next;
		}
		if (cmdbuf_obj_temp->cmdbuf_run_done) {
			if (done_cmdbuf_node && done_cmdbuf_node->data)
				gb_printf(KERN_INFO, "%s: done_cmdbuf_node is cmdbuf [%d].\n",
				 __func__,
				  ((struct GB02STR18 *)done_cmdbuf_node->data)->cmdbuf_id);
			if (done_cmdbuf_node) {
				done_cmdbuf_node = done_cmdbuf_node->next;
				if (done_cmdbuf_node)
					restart_cmdbuf = (struct GB02STR18 *)done_cmdbuf_node->data;
				if (restart_cmdbuf) {
					gb_printf(KERN_INFO, "%s: Set restart cmdbuf [%d] via if.\n",
				 	__func__, restart_cmdbuf->cmdbuf_id);
				}
			}
		} else {
			last_cmdbuf_node = new_cmdbuf_node;
			/* cmd buf num from aborted cmd buf to current cmdbuf_obj_temp */
			if (cmdbuf_obj_temp->cmdbuf_id != abort_cmdbuf_id) {
				last_cmdbuf_node  = new_cmdbuf_node->previous;
				while (last_cmdbuf_node &&
					((struct GB02STR18 *)last_cmdbuf_node->data)->cmdbuf_id
						!= abort_cmdbuf_id) {
					restart_cmdbuf =
					 (struct GB02STR18 *)last_cmdbuf_node->data;
					last_cmdbuf_node
					 = last_cmdbuf_node->previous;
					dev[core_id].sw_cmdbuf_rdy_num++;
					dev[core_id].duration_without_int +=
						restart_cmdbuf->executing_time;
					gb_printf(KERN_INFO, "%s: Keep valid cmdbuf [%d] in the list.\n",
					 __func__, restart_cmdbuf->cmdbuf_id);
				}
			}
			if (restart_cmdbuf) {
				gb_printf(KERN_INFO, "%s: Set restart cmdbuf [%d] via else.\n",
					__func__, restart_cmdbuf->cmdbuf_id);
			}
		}
		// remove first linked cmdbuf from list
		GB02FUNC912(&dev[core_id], new_cmdbuf_node);
	}
	return vcmd_aborted;
}

int GB02FUNC934(struct drm_file *filp, void *dec_priv)
{
	struct GB02STR37 *dev =
		(struct GB02STR37 *) dec_priv;
		//(struct GB02STR37 *) filp->driver_priv;
	u32 core_id = 0;
	unsigned long flags;
	u32 release_cmdbuf_num = 0;
	bi_list_node *new_cmdbuf_node = NULL;
	struct drm_gem_object *vcmd_obj = NULL;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;
	bi_list_node *process_manager_node;
	struct GB02STR17 *GB02STR17 = NULL;
	int vcmd_aborted = 0;  /* vcmd is aborted in this function */
	struct GB02STR18 *restart_cmdbuf = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();
	vcmd_obj = dec_data->vcmd_pool->obj;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	drm_vma_node_revoke(&vcmd_obj->vma_node, filp);
#else
	drm_vma_node_revoke(&vcmd_obj->vma_node, filp->filp);
#endif

	gb_printf(KERN_INFO, "%s: dev closed for process %p via\n",
	 	__func__, (void *)filp);
	if (down_interruptible(&dec_data->vcmd_reserve_cmdbuf_sem[dev->vcmd_core_cfg.sub_module_type]))
		return -ERESTARTSYS;

	for (core_id = 0; core_id < dec_data->total_vcmd_core_num; core_id++) {
		if ((&dev[core_id]) == NULL)
			continue;
		spin_lock_irqsave(dev[core_id].spinlock, flags);
		new_cmdbuf_node = dev[core_id].list_manager.head;
		while (1) {
			if (new_cmdbuf_node == NULL)
				break;
			cmdbuf_obj_temp
			 = (struct GB02STR18 *)new_cmdbuf_node->data;
			gb_printf(KERN_INFO, "%s: Process %p is releas: check cmdbuf %d\n",
				__func__, filp, cmdbuf_obj_temp->cmdbuf_id);
			if (dev[core_id].hwregs
			 && (cmdbuf_obj_temp->filp == filp)) {
				vcmd_aborted =
				 GB02FUNC919(dev, new_cmdbuf_node,
				  cmdbuf_obj_temp, restart_cmdbuf, core_id);
				release_cmdbuf_num++;
				gb_printf(KERN_INFO, "%s: release reserved cmdbuf\n",
				 __func__);
			} else if (vcmd_aborted
			 && !cmdbuf_obj_temp->cmdbuf_run_done) {
				if (!restart_cmdbuf)
					restart_cmdbuf = cmdbuf_obj_temp;
				dev[core_id].duration_without_int +=
					cmdbuf_obj_temp->executing_time;
				dev[core_id].sw_cmdbuf_rdy_num++;
			}

			if (new_cmdbuf_node == NULL)
				break;

			new_cmdbuf_node = new_cmdbuf_node->next;
		}

		GB02FUNC904(dev, restart_cmdbuf, core_id);
		spin_unlock_irqrestore(dev[core_id].spinlock, flags);
		// VCMD aborted but not restarted, nedd to wake up
		if (vcmd_aborted && !restart_cmdbuf)
			wake_up_interruptible_all(dev[core_id].wait_queue);
	}

	if (release_cmdbuf_num)
		wake_up_interruptible_all(&vcmd_cmdbuf_memory_wait);
	spin_lock_irqsave(&vcmd_process_manager_lock, flags);
	process_manager_node = dec_data->global_process_manager.head;
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
	gb_printf(KERN_INFO, "%s: process node %p for filp to be removed: %p\n",
	 __func__, (void *)process_manager_node,
	  (void *)GB02STR17->filp);
	GB02FUNC225(&dec_data->global_process_manager,
		process_manager_node);
	spin_unlock_irqrestore(&vcmd_process_manager_lock, flags);
	GB02FUNC264(process_manager_node);
	up(&dec_data->vcmd_reserve_cmdbuf_sem[dev->vcmd_core_cfg.sub_module_type]);
	return 0;
}

void GB02FUNC944(void)
{

}

irqreturn_t GB02FUNC947(int irq)
{
	irqreturn_t ret = 0;
	ret = hantrovcmd_disr(irq, (void *)&hantrovcmd_data[0]);
	return ret;
}
int GB02FUNC948(int irq_id)
{
	int i, ret = 0;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	/* get the IRQ line */
	for (i = 0; i < dec_data->total_vcmd_core_num; i++) {
		if (hantrovcmd_data[i].hwregs == NULL)
			continue;
		gb_printf(KERN_INFO, "%s-%d: dec vcmd_irq = %d\n", __func__,
		 __LINE__, hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq);
		if (hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq != -1) {
			ret = request_irq(irq_id, hantrovcmd_disr,
			#if (KERNEL_VERSION(2, 6, 18) > LINUX_VERSION_CODE)
				SA_INTERRUPT | SA_SHIRQ,
			#else
				IRQF_SHARED,
			#endif
				"vc8000d_dec_vcmd",
				 (void *)&hantrovcmd_data[i]);
			if (ret == -EINVAL) {
				gb_printf(KERN_ERR, "%s: Bad vcmd_irq handler i = %d\n",
				 __func__, i);
				GB02FUNC717(hantrovcmd_data,
				 dec_data->total_vcmd_core_num);
				goto err;
			} else if (ret == -EBUSY) {
				gb_printf(KERN_ERR, "%s: i = %d, IRQ <%d> busy, change config\n",
				 __func__, i,
				  hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq);
				GB02FUNC717(hantrovcmd_data,
				 dec_data->total_vcmd_core_num);
				goto err;
			} else {
				gb_printf(KERN_INFO, "hantro_vcmd: request IRQ <%d> successfully for subsystem %d\n",
				hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq, i);
			}
		} else
			gb_printf(KERN_INFO, "%s: hantro_vcmd: IRQ not in use!\n", __func__);
	}
	return ret;
err:
	GB02FUNC944();
	if (hantrovcmd_data != NULL)
		vfree(hantrovcmd_data);
	if (dec_data != NULL)
		vfree(dec_data);
	return -1;
}

void GB02FUNC953(struct GB02STR43 *dec_data,
	struct GB02STR20 dec_pcie, struct GB02STR3 *vcmd_pool,
	struct GB02STR3 *vcmd_reg)
{
	dec_data->dec_vcmd_mem_pool[0].bus_address = vcmd_pool->flp_offset;
	dec_data->dec_vcmd_mem_pool[1].bus_address = vcmd_pool->flp_offset + GB02MAC156;
	dec_data->dec_vcmd_mem_pool[2].bus_address = vcmd_reg->flp_offset;

	dec_data->dec_vcmd_mem_pool[0].virtual_address = vcmd_pool->vir_buff;
	dec_data->dec_vcmd_mem_pool[1].virtual_address = vcmd_pool->vir_buff + GB02MAC156;
	dec_data->dec_vcmd_mem_pool[2].virtual_address = vcmd_reg->vir_buff;
	dec_data->base_ddr_addr = 0;
}
int GB02FUNC955(struct GB02STR20 dec_pcie, int total_vcmd_core_num,
	struct GB02STR3 *vcmd_pool, struct GB02STR3 *vcmd_reg,
	u64 dec_offset)
{
	int i, k, result;
	struct GB02STR3 *vcmd_pool_tmp = NULL;
	struct GB02STR43 *dec_data = NULL;

	gb_printf(KERN_INFO, "%s-%d: dec vcmd start\n", __func__, __LINE__);
	result = GB02FUNC702(total_vcmd_core_num, dec_offset);
	if (result)
		goto err;

	for (i = 0; i < total_vcmd_core_num; i++)
		gb_printf(KERN_INFO, "%s: module init - vcmdcore[%d] addr =0x%lx\n",
		 __func__, i, vcmd_core_array[i].vcmd_base_addr);
	hantrovcmd_data =
		(struct GB02STR37 *)vmalloc(sizeof(struct GB02STR37)
	  	* total_vcmd_core_num);
	if (hantrovcmd_data == NULL)
		goto err;
	memset(hantrovcmd_data, 0,
		sizeof(struct GB02STR37) * total_vcmd_core_num);

	vcmd_pool_tmp =
		(struct GB02STR3 *)vmalloc(sizeof(struct GB02STR3));
	if (vcmd_pool_tmp == NULL)
		goto err;
	memset(vcmd_pool_tmp, 0, sizeof(struct GB02STR3));

	dec_data = (struct GB02STR43 *)
		vmalloc(sizeof(struct GB02STR43));
	if (dec_data == NULL)
		goto err;
	memset(dec_data, 0, sizeof(struct GB02STR43));

	memcpy(vcmd_pool_tmp, vcmd_pool, sizeof(struct GB02STR3));
	dec_data->vcmd_pool = vcmd_pool_tmp;

	dec_vcmd_private_data = dec_data;
	for (k = 0; k < MAX_VCMD_TYPE; k++) {
		dec_data->vcmd_type_core_num[k] = 0;
		dec_data->vcmd_position[k] = 0;
		for (i = 0; i < GB02MAC152; i++)
			vcmd_manager[k][i] = NULL;
	}
	dec_data->vcmd_buff.offset = vcmd_pool->offset;
	dec_data->vcmd_buff.vir_buff = vcmd_pool->vir_buff;
	dec_data->total_vcmd_core_num = total_vcmd_core_num;
	GB02FUNC953(dec_data, dec_pcie, vcmd_pool, vcmd_reg);
	GB02FUNC208(&dec_data->global_process_manager);
	for (i = 0; i < total_vcmd_core_num; i++) {
		hantrovcmd_data[i].vcmd_core_cfg = vcmd_core_array[i];
		hantrovcmd_data[i].hwregs = NULL;
		hantrovcmd_data[i].core_id = i;
		hantrovcmd_data[i].working_state = GB02MAC145;
		hantrovcmd_data[i].sw_cmdbuf_rdy_num = 0;
		hantrovcmd_data[i].spinlock = &dec_data->owner_lock_vcmd[i];
		spin_lock_init(&dec_data->owner_lock_vcmd[i]);
		hantrovcmd_data[i].wait_queue = &dec_data->wait_queue_vcmd[i];
		init_waitqueue_head(&dec_data->wait_queue_vcmd[i]);
		hantrovcmd_data[i].wait_abort_queue
		 = &dec_data->abort_queue_vcmd[i];
		init_waitqueue_head(&dec_data->abort_queue_vcmd[i]);
		GB02FUNC208(&hantrovcmd_data[i].list_manager);
		hantrovcmd_data[i].duration_without_int = 0;
		vcmd_manager[vcmd_core_array[i].sub_module_type][dec_data->vcmd_type_core_num[vcmd_core_array[i].sub_module_type]] =
		&hantrovcmd_data[i];
		dec_data->vcmd_type_core_num[vcmd_core_array[i].sub_module_type]++;

		hantrovcmd_data[i].vcmd_reg_mem_bus_address =
			vcmd_reg->offset + i * GB02MAC144;

		hantrovcmd_data[i].vcmd_reg_mem_virtual_address =
		vcmd_reg->vir_buff + i * GB02MAC144;

		hantrovcmd_data[i].vcmd_reg_mem_size = GB02MAC144;
#ifndef CONFIG_SW64
		memset(hantrovcmd_data[i].vcmd_reg_mem_virtual_address, 0,
                        GB02MAC144);
#else
		memset_io(hantrovcmd_data[i].vcmd_reg_mem_virtual_address, 0,
                       GB02MAC144);
#endif
	}
	init_waitqueue_head(&dec_data->mc_wait_queue);
	result = GB02FUNC718(hantrovcmd_data, total_vcmd_core_num,
	 (u8 *)dec_pcie.vpu_vaddr_base);
	if (result < 0)
		goto err;
	GB02FUNC710(hantrovcmd_data, total_vcmd_core_num);

	dec_data->cmdbuf_used_pos = 0;
	for (k = 0; k < GB02MAC159; k++) {
		dec_data->cmdbuf_used[k] = 0;
		dec_data->global_cmdbuf_node[k] = NULL;
	}
	//cmdbuf_used[0] not be used, because int vector must non-zero
	dec_data->cmdbuf_used_residual = GB02MAC159;
	dec_data->cmdbuf_used_pos = 1;
	dec_data->cmdbuf_used[0] = 1;
	dec_data->cmdbuf_used_residual -= 1;

	GB02FUNC247(&dec_data->global_process_manager);
	for (i = 0; i < MAX_VCMD_TYPE; i++) {
		if (dec_data->vcmd_type_core_num[i] == 0)
			continue;
		sema_init(&dec_data->vcmd_reserve_cmdbuf_sem[i], 1);
	}

	/*
	 * read all registers for each type of module for analyzing
	 * configuration in cwl
	 */
	for (i = 0; i < MAX_VCMD_TYPE; i++) {
		if (dec_data->vcmd_type_core_num[i] == 0)
			continue;
		GB02FUNC888(i, hantrovcmd_data);
	}

	return 0;
err:
	GB02FUNC944();
	if (hantrovcmd_data != NULL)
		vfree(hantrovcmd_data);
	if (dec_data != NULL)
		vfree(dec_data);
	if (vcmd_pool_tmp != NULL)
		vfree(vcmd_pool_tmp);
	return -1;
}

void GB02FUNC966(void)
{
	bi_list_node *cmdbuf_node = NULL;
	gb_printf(KERN_INFO, "%s:%d, dec0 resume in\n", __func__, __LINE__);

	cmdbuf_node = hantrovcmd_data[0].list_manager.tail;
	if (cmdbuf_node != NULL)
		((struct GB02STR18 *)cmdbuf_node->data)->cmdbuf_run_done = 1;

	hantrovcmd_data[0].working_state = GB02MAC145;
	hantrovcmd_data[0].sw_cmdbuf_rdy_num = 0;

	GB02FUNC710(hantrovcmd_data, 1);

	gb_printf(KERN_INFO, "%s:%d, dec0 resume out\n", __func__, __LINE__);
}

void GB02FUNC969(void)
{
	gb_printf(KERN_INFO, "%s:%d, dec0 suspend in\n", __func__, __LINE__);

	for(;;) {
		if (list_empty(&hantrovcmd_data[0].wait_queue->head))
			break;
	}

	gb_printf(KERN_INFO, "%s:%d, dec1 suspend out\n", __func__, __LINE__);
}

static void GB02FUNC971(bi_list_node *cmdbuf_node)
{
	struct GB02STR18 *GB02STR18 = NULL;

	if (cmdbuf_node == NULL) {
		gb_printf(KERN_ERR, "%s: remove_cmdbuf_node NULL\n", __func__);
		return;
	}
	GB02STR18 = (struct GB02STR18 *)cmdbuf_node->data;
	//free cmdbuf mem in pool
	GB02FUNC727(GB02STR18->cmdbuf_id);
	//free struct GB02STR18
	GB02FUNC254(GB02STR18);
	//free current cmdbuf_node entity.
	GB02FUNC230(cmdbuf_node);
}

static long GB02FUNC973(bi_list *list)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	while (1) {
		new_cmdbuf_node = list->head;
		if (new_cmdbuf_node == NULL)
			return 0;
		/* remove node from list */
		GB02FUNC225(list, new_cmdbuf_node);
		/* free node */
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		dec_data->global_cmdbuf_node[GB02STR18->cmdbuf_id] = NULL;
		GB02FUNC971(new_cmdbuf_node);
	}
	return 0;
}

void GB02FUNC976(int total_vcmd_core_num)
{
	int i = 0;
	u32 result;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	for (i = 0; i < total_vcmd_core_num; i++) {
		if (hantrovcmd_data[i].hwregs == NULL)
			continue;
		//disable interrupt at first
		GB02FUNC318((const void *) hantrovcmd_data[i].hwregs,
			GB02MAC515, 0x0000);
		//disable HW
		GB02FUNC318((const void *) hantrovcmd_data[i].hwregs,
			GB02MAC513, 0x0000);
		//read status register
		result = GB02FUNC313((const void *) hantrovcmd_data[i].hwregs,
			GB02MAC514);
		//clean status register
		GB02FUNC318((const void *) hantrovcmd_data[i].hwregs,
			GB02MAC514, result);

		/* free the vcmd IRQ */
		if (hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq != -1) {
			free_irq(hantrovcmd_data[i].vcmd_core_cfg.vcmd_irq,
				(void *)&hantrovcmd_data[i]);
		}
		GB02FUNC973(&hantrovcmd_data[i].list_manager);
	}
	GB02FUNC277(&dec_data->global_process_manager);

	GB02FUNC717(hantrovcmd_data, total_vcmd_core_num);
	vfree(hantrovcmd_data);

	GB02FUNC944();
	vfree(dec_data);
	gb_printf(KERN_INFO, "%s: module removed finish!!\n", __func__);
}

long GB02FUNC980(struct drm_file *filp, u16 cmdbuf_id)
{
	struct GB02STR18 *GB02STR18 = NULL;
	bi_list_node *last_cmdbuf_node = NULL;
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list *list = NULL;
	u32 module_type;
	unsigned long flags;
	struct GB02STR37 *dev = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	/*get cmdbuf object according to cmdbuf_id*/
	new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
	if (new_cmdbuf_node == NULL) {
		//should not happen
		gb_printf(KERN_ERR, "%s: ERROR new_cmdbuf_node = NULL error!!\n", __func__);
		return -1;
	}
	GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
	if (GB02STR18->filp != filp) {
		//should not happen
		gb_printf(KERN_ERR, "%s: ERROR GB02STR18->filp != filp error!!\n",
		 __func__);
		return -1;
	}
	module_type = GB02STR18->module_type;
	//TODO
	if (down_interruptible(&dec_data->vcmd_reserve_cmdbuf_sem[module_type]))
		return -ERESTARTSYS;
	dev = &hantrovcmd_data[GB02STR18->core_id];

	list = &dev->list_manager;
	GB02STR18->cmdbuf_need_remove = 1;
	last_cmdbuf_node = new_cmdbuf_node->previous;
	while (1) {
		//remove current node
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if (GB02STR18->cmdbuf_need_remove == 1) {
			new_cmdbuf_node = GB02FUNC176(list,
				new_cmdbuf_node);
			if (new_cmdbuf_node) {
				//free node
				dec_data->global_cmdbuf_node[GB02STR18->cmdbuf_id]
				 = NULL;
				if (GB02STR18->GB02STR17) {
					spin_lock_irqsave(&GB02STR18->GB02STR17->spinlock, flags);
					GB02STR18->GB02STR17->total_exe_time
					 -= GB02STR18->executing_time;
					spin_unlock_irqrestore(&GB02STR18->GB02STR17->spinlock, flags);
					wake_up_interruptible_all(&GB02STR18->GB02STR17->wait_queue);
				}
				GB02FUNC971(new_cmdbuf_node);
			}
		}
		if (last_cmdbuf_node == NULL)
			break;
		new_cmdbuf_node = last_cmdbuf_node;
		last_cmdbuf_node = new_cmdbuf_node->previous;
	}
	up(&dec_data->vcmd_reserve_cmdbuf_sem[module_type]);
	return 0;
}

int GB02FUNC985(struct drm_file *filp, void *data)
{
	int ret = -1;
	struct GB02STR3 vcmd_buff = {0};
	struct drm_gb_cmdbuf_resv *input_para =
		(struct drm_gb_cmdbuf_resv *)data;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();
	vcmd_buff.offset = dec_data->vcmd_pool->offset;
	vcmd_buff.vir_buff = dec_data->vcmd_pool->vir_buff;

	gb_printf(KERN_INFO, "%s:%d:-vcmd buff offset = 0x%llx,\
		vir_buf = 0x%llx--\n", __func__, __LINE__,
		vcmd_buff.offset, (long long)vcmd_buff.vir_buff);

	ret = GB02FUNC832(filp, input_para,
		&dec_data->global_process_manager,
		dec_data->global_cmdbuf_node, vcmd_buff);
	if (ret == -1)
		gb_printf(KERN_INFO, "%s: dec VCMD Reserve %d\n", __func__,
		 	input_para->cmdbuf_id);

	return ret;
}

long GB02FUNC987(struct drm_file *filp,
		unsigned int cmd, void *arg)
{
	int err = 0;
	static int last_polling_cmd;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC700();

	if (cmd != HANTRO_VCMD_IOCH_POLLING_CMDBUF) {
		last_polling_cmd = 0;
		gb_printf(KERN_INFO, "%s: if != ioctl cmd 0x%08x\n", __func__, cmd);
	} else {
		if (!last_polling_cmd)
			gb_printf(KERN_INFO, "%s: else == ioctl cmd 0x%08x\n",
			 __func__, cmd);
		last_polling_cmd = 1;
	}
	if (_IOC_TYPE(cmd) != GB02MAC236)
		return -ENOTTY;
	if ((_IOC_TYPE(cmd) == GB02MAC236
	 && _IOC_NR(cmd) > GB02MAC237))
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !gb_access_ok(VERIFY_WRITE, (void *) arg,
			_IOC_SIZE(cmd));
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !gb_access_ok(VERIFY_READ, (void *) arg,
			_IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {
	case HANTRO_VCMD_IOCH_GET_CMDBUF_PARAMETER: {
		struct GB02STR16 local_cmdbuf_mem_data;
		struct GB02STR14 *vcmd_buf_mem_pool =
			&dec_data->dec_vcmd_mem_pool[0];
		struct GB02STR14 *vcmd_status_buf_mem_pool =
			&dec_data->dec_vcmd_mem_pool[1];

		gb_printf(KERN_INFO, "%s: dec VCMD Reserve, cmd = %x\n", __func__, cmd);
		local_cmdbuf_mem_data.cmdbuf_unit_size = GB02MAC156;
		local_cmdbuf_mem_data.status_cmdbuf_unit_size
		 = GB02MAC156;
		local_cmdbuf_mem_data.cmdbuf_total_size
		 = GB02MAC158;
		local_cmdbuf_mem_data.status_cmdbuf_total_size
		 = GB02MAC158;
		local_cmdbuf_mem_data.phy_status_cmdbuf_addr =
			//vcmd_status_buf_mem_pool->bus_address;
			vcmd_buf_mem_pool->bus_address;
		gb_printf(KERN_INFO, "-------vcmd status pool 1 = %llx---\n", vcmd_status_buf_mem_pool->bus_address);
		local_cmdbuf_mem_data.virt_status_cmdbuf_addr =
			vcmd_status_buf_mem_pool->virtual_address;

		local_cmdbuf_mem_data.phy_cmdbuf_addr =
			vcmd_buf_mem_pool->bus_address;


		local_cmdbuf_mem_data.base_ddr_addr = dec_data->base_ddr_addr;
		return copy_to_user((struct GB02STR16 *)arg,
		 &local_cmdbuf_mem_data, sizeof(struct GB02STR16));
	}
	case HANTRO_VCMD_IOCH_GET_VCMD_PARAMETER: {
		struct GB02STR45 input_para;
		int ret;

		gb_printf(KERN_INFO, "%s: dec vcmd config cmd = %x\n", __func__, cmd);
		ret = copy_from_user(&input_para,
		 (struct GB02STR45 *)arg,
		  sizeof(struct GB02STR45));
		if (ret != 0)
			return -1;
		if (dec_data->vcmd_type_core_num[input_para.module_type]) {
			input_para.submodule_main_addr =
			 vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_main_addr;
			input_para.submodule_dec400_addr =
			 vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_dec400_addr;
			input_para.submodule_L2Cache_addr =
			 vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_L2Cache_addr;
			input_para.submodule_MMU_addr =
			 vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_MMU_addr;
			input_para.submodule_MMUWrite_addr =
			 vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_MMUWrite_addr;
			input_para.submodule_axife_addr =
			 vcmd_manager[input_para.module_type][0]->vcmd_core_cfg.submodule_axife_addr;
			input_para.config_status_cmdbuf_id =
			 vcmd_manager[input_para.module_type][0]->status_cmdbuf_id;
			input_para.vcmd_hw_version_id =
			 vcmd_manager[input_para.module_type][0]->hw_version_id;
			input_para.vcmd_core_num =
			 dec_data->vcmd_type_core_num[input_para.module_type];
		} else {
			input_para.submodule_main_addr = 0xffff;
			input_para.submodule_dec400_addr = 0xffff;
			input_para.submodule_L2Cache_addr = 0xffff;
			input_para.submodule_MMU_addr = 0xffff;
			input_para.submodule_MMUWrite_addr = 0xffff;
			input_para.submodule_axife_addr = 0xffff;
			input_para.config_status_cmdbuf_id = 0;
			input_para.vcmd_core_num = 0;
			input_para.vcmd_hw_version_id = GB02MAC186;
		}
		return copy_to_user((struct GB02STR45 *)arg,
			&input_para, sizeof(struct GB02STR45));
	}
	case HANTRO_VCMD_IOCH_GET_HWINFO_FROM_VCMD: {
		struct GB02STR41 *hw_info  = &dec_data->hw_info;
		struct GB02STR41 uhw_info;

		memcpy(&uhw_info, hw_info, sizeof(struct GB02STR41));

		gb_printf(KERN_INFO, "%s: dec VCMD get hw info from vcmd, cmd = %x\n", __func__, cmd);
		return copy_to_user((struct GB02STR41 *)arg,
		 &uhw_info, sizeof(struct GB02STR41));
	}
	default:
		break;
	}
	return 0;
}


