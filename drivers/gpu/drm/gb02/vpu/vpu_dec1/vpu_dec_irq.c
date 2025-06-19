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
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <asm/irq.h>

#include "vpu_dec_irq.h"
#include "vpu_subsys.h"
#include "vpu_dec.h"
#include "vpu_vcmd.h"
#include "vpu/vpu_comm/vpu_vcmd_registers.h"
#include "vpu/vpu_comm/vpu_comm.h"
#include "vpu/vpu_comm/vpu_list.h"
//#include "vpu/vpu_comm/vpu_io.h"
#include "vpu/vpu_comm/vpu_basetype.h"

static void GB02FUNC612(struct GB02STR37 *dev)
{
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *base_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;

	/* reset error,all cmdbuf that is not  done will be run again. */
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	/* find the first run_done=0 */
	while (1) {
		if (new_cmdbuf_node == NULL)
			break;
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if ((GB02STR18->cmdbuf_run_done == 0))
			break;
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
	base_cmdbuf_node = new_cmdbuf_node;
	GB02FUNC1191(dev, base_cmdbuf_node);
	GB02FUNC1255(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0)
		GB02FUNC1274(dev, base_cmdbuf_node);
}

static void GB02FUNC615(u32 *cmdbuf_processed_num,
	bi_list_node *new_cmdbuf_node)
{
	u32 tmp_num = *cmdbuf_processed_num;
	struct GB02STR18 *GB02STR18 = NULL;

	if (new_cmdbuf_node == NULL)
		return;

	while (1) {
		if (new_cmdbuf_node == NULL)
			break;
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if ((GB02STR18->cmdbuf_run_done == 0)) {
			GB02STR18->cmdbuf_run_done = 1;
			GB02STR18->executing_status = GB02MAC147;
			tmp_num++;
			*cmdbuf_processed_num = tmp_num;
		} else
			break;
		new_cmdbuf_node = new_cmdbuf_node->previous;
	}
}

static int GB02FUNC619(struct GB02STR37 *dev,
	unsigned long flags, bi_list_node *new_cmdbuf_node,
	 size_t base_ddr_addr)
{
	struct GB02STR18 *GB02STR18 = NULL;
	size_t busAddress;

	busAddress = VCMDGetAddrRegisterValue((const void *)dev->hwregs,
		dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR);
	while (1) {
		if (new_cmdbuf_node == NULL) {
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if ((((GB02STR18->cmdbuf_bus_address - base_ddr_addr)
		 <= busAddress) && (((GB02STR18->cmdbuf_bus_address -
		  base_ddr_addr + GB02STR18->cmdbuf_size) > busAddress)))
		  && (GB02STR18->cmdbuf_run_done == 0))
			break;
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
	return 0;
}

static int GB02FUNC623(struct GB02STR37 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	size_t base_ddr_addr = 0;
	int ret;
	bi_list_node *base_cmdbuf_node = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC1187();
	base_ddr_addr = dec_data->base_ddr_addr;

	//abort error,don't need to reset
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		//find the cmdbuf that triggers ABORT
		ret = GB02FUNC619(dev, flags, new_cmdbuf_node,
			base_ddr_addr);
		if (ret != 0)
			return -1;
	}
	base_cmdbuf_node = new_cmdbuf_node;
	// this cmdbuf and cmdbufs prior to itself, run_done = 1
	GB02FUNC615(cmdbuf_processed_num, new_cmdbuf_node);

	base_cmdbuf_node = base_cmdbuf_node->next;
	GB02FUNC1191(dev, base_cmdbuf_node);

	spin_unlock_irqrestore(dev->spinlock, flags);
	if (cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	//to let high priority cmdbuf be inserted
	wake_up_interruptible_all(dev->wait_abort_queue);
	wake_up_interruptible_all(&dec_data->mc_wait_queue);
	return 0;
}

static int GB02FUNC625(struct GB02STR37 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	size_t base_ddr_addr = 0;
	bi_list_node *base_cmdbuf_node = NULL;
	int ret;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC1187();
	base_ddr_addr = dec_data->base_ddr_addr;

	// bus error, don't need to reset where to record status?
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		// find the buserr cmdbuf
		ret = GB02FUNC619(dev, flags, new_cmdbuf_node,
			base_ddr_addr);
		if (ret != 0)
			return -1;
	}
	base_cmdbuf_node = new_cmdbuf_node;
	// this cmdbuf and cmdbufs prior to itself, run_done = 1
	GB02FUNC615(cmdbuf_processed_num, new_cmdbuf_node);

	new_cmdbuf_node = base_cmdbuf_node;
	if (new_cmdbuf_node != NULL) {
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		GB02STR18->executing_status = GB02MAC149;
	}
	base_cmdbuf_node = base_cmdbuf_node->next;
	GB02FUNC1191(dev, base_cmdbuf_node);
	GB02FUNC1255(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0)
		// restart new command
		GB02FUNC1274(dev, base_cmdbuf_node);

	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	wake_up_interruptible_all(&dec_data->mc_wait_queue);
	return 0;
}

static int GB02FUNC632(struct GB02STR37 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	size_t base_ddr_addr = 0;
	bi_list_node *base_cmdbuf_node = NULL;
	int ret;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC1187();
	base_ddr_addr = dec_data->base_ddr_addr;

	// time out,need to reset
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		// find the timeout cmdbuf
		ret = GB02FUNC619(dev, flags, new_cmdbuf_node,
			base_ddr_addr);
		if (ret != 0)
			return -1;
	}
	base_cmdbuf_node = new_cmdbuf_node;
	new_cmdbuf_node = new_cmdbuf_node->previous;
	// this cmdbuf and cmdbufs prior to itself, run_done = 1
	GB02FUNC615(cmdbuf_processed_num, new_cmdbuf_node);

	GB02FUNC1191(dev, base_cmdbuf_node);
	GB02FUNC1255(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0) {
		// reset
		GB02FUNC1252(dev);
		// restart new command
		GB02FUNC1274(dev, base_cmdbuf_node);
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	wake_up_interruptible_all(&dec_data->mc_wait_queue);
	return 0;
}

static int GB02FUNC639(struct GB02STR37 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	size_t base_ddr_addr = 0;
	bi_list_node *base_cmdbuf_node = NULL;
	int ret;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC1187();
	base_ddr_addr = dec_data->base_ddr_addr;

	//command error,don't need to reset
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		//find the cmderror cmdbuf
		ret = GB02FUNC619(dev, flags, new_cmdbuf_node,
			base_ddr_addr);
		if (ret != 0)
			return -1;
	}
	base_cmdbuf_node = new_cmdbuf_node;
	// this cmdbuf and cmdbufs prior to itself, run_done = 1
	GB02FUNC615(cmdbuf_processed_num, new_cmdbuf_node);

	new_cmdbuf_node = base_cmdbuf_node;
	if (new_cmdbuf_node != NULL) {
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		GB02STR18->executing_status = GB02MAC148;
	}
	base_cmdbuf_node = base_cmdbuf_node->next;
	GB02FUNC1191(dev, base_cmdbuf_node);
	GB02FUNC1255(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0)
		// restart new command
		GB02FUNC1274(dev, base_cmdbuf_node);

	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	wake_up_interruptible_all(&dec_data->mc_wait_queue);
	return 0;
}

static int GB02FUNC647(struct GB02STR37 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *base_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC1187();

	//end command interrupt
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		//find the end cmdbuf
		while (1) {
			if (new_cmdbuf_node == NULL) {
				spin_unlock_irqrestore(dev->spinlock, flags);
				return -1;
			}
			GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
			if ((GB02STR18->has_end_cmdbuf == 1)
				&& (GB02STR18->cmdbuf_run_done == 0))
				break;
			new_cmdbuf_node = new_cmdbuf_node->next;
		}
	}
	base_cmdbuf_node = new_cmdbuf_node;
	// this cmdbuf and cmdbufs prior to itself, run_done = 1
	GB02FUNC615(cmdbuf_processed_num, new_cmdbuf_node);

	base_cmdbuf_node = base_cmdbuf_node->next;
	GB02FUNC1191(dev, base_cmdbuf_node);
	GB02FUNC1255(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0)
		// restart new command
		GB02FUNC1274(dev, base_cmdbuf_node);

	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	wake_up_interruptible_all(&dec_data->mc_wait_queue);
	return 0;
}

static int GB02FUNC653(struct GB02STR37 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC1187();

	if (dev->hw_version_id <= GB02MAC186) {
		if (cmdbuf_id >= GB02MAC159) {
			gb_printf(KERN_ERR, "%s: error, cmdbuf_id %d greater than ceiling\n",
			 __func__, cmdbuf_id);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	}
	new_cmdbuf_node = dec_data->global_cmdbuf_node[cmdbuf_id];
	if (new_cmdbuf_node == NULL) {
		gb_printf(KERN_ERR, "%s: error cmdbuf_id !!\n", __func__);
		spin_unlock_irqrestore(dev->spinlock, flags);
		return -1;
	}
	// interrupt cmdbuf and cmdbufs prior to itself, run_done = 1
	GB02FUNC615(cmdbuf_processed_num, new_cmdbuf_node);
	return 0;
}

#if (KERNEL_VERSION(2, 6, 18) > LINUX_VERSION_CODE)
irqreturn_t hantrovcmd_d1isr(int irq, void *dev_id, struct pt_regs *regs)
#else
irqreturn_t hantrovcmd_d1isr(int irq, void *dev_id)
#endif
{
	unsigned int handled = 0;
	struct GB02STR37 *dev = (struct GB02STR37 *) dev_id;
	u32 irq_status = 0;
	unsigned long flags;
	u32 cmdbuf_processed_num = 0;
	u32 cmdbuf_id = 0;
	int ret_val;
	struct GB02STR43 *dec_data = NULL;

	dec_data = GB02FUNC1187();

	spin_lock_irqsave(dev->spinlock, flags);
	if (dev->list_manager.head == NULL) {
		gb_printf(KERN_ERR, "%s: received IRQ but core has nothing to do\n",
		 __func__);
		irq_status = GB02FUNC313((const void *)dev->hwregs,
			GB02MAC514);
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC514, irq_status);
		spin_unlock_irqrestore(dev->spinlock, flags);
		return IRQ_HANDLED;
	}

	irq_status = GB02FUNC313((const void *)dev->hwregs,
	 GB02MAC514);
	if (!irq_status) {
		gb_printf(KERN_ERR, "%s:error,irq_status :0x%x\n", __func__, irq_status);
		spin_unlock_irqrestore(dev->spinlock, flags);
		return IRQ_HANDLED;
	}

	gb_printf(KERN_INFO, "%s: dev coreid = %d, irq_status = 0x%x\n",
		  __func__, dev->core_id, irq_status);
	GB02FUNC318((const void *)dev->hwregs,
	 GB02MAC514, irq_status);
	dev->reg_mirror[GB02MAC514 / 4] = irq_status;
	if ((dev->hw_version_id > GB02MAC186) && (irq_status & 0x3f)) {
		cmdbuf_id = GB02FUNC325((const void *)dev->hwregs,
			dev->reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID);
		if (cmdbuf_id >= GB02MAC159) {
			gb_printf(KERN_ERR, "%s: error cmdbuf_id greater than ceil\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return IRQ_HANDLED;
		}
	} else if (dev->hw_version_id > GB02MAC186) {
		//read cmdbuf id from ddr
		cmdbuf_id = *(dev->vcmd_reg_mem_virtual_address
			+ GB02MAC143);
		if (cmdbuf_id >= GB02MAC159) {
			gb_printf(KERN_ERR, "%s: error cmdbuf_id great than ceil\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return IRQ_HANDLED;
		}
	}
	if (GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_RESET)) {
		GB02FUNC612(dev);
		handled++;
		spin_unlock_irqrestore(dev->spinlock, flags);
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_ABORT)) {
		ret_val = GB02FUNC623(dev, cmdbuf_id, flags,
			&cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_BUSERR)) {
		ret_val = GB02FUNC625(dev, cmdbuf_id, flags,
			&cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
		HWIF_VCMD_IRQ_TIMEOUT)) {
		ret_val = GB02FUNC632(dev, cmdbuf_id, flags,
			&cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
		HWIF_VCMD_IRQ_CMDERR)) {
		ret_val = GB02FUNC639(dev, cmdbuf_id, flags,
		 &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}

	if (GB02FUNC344(dev->reg_mirror,
		HWIF_VCMD_IRQ_ENDCMD)) {
		ret_val = GB02FUNC647(dev, cmdbuf_id, flags,
		 &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (dev->hw_version_id <= GB02MAC186)
		cmdbuf_id = GB02FUNC344(dev->reg_mirror,
		 HWIF_VCMD_IRQ_INTCMD);
	if (cmdbuf_id) {
		ret_val = GB02FUNC653(dev, cmdbuf_id, flags,
		 &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
	}

	spin_unlock_irqrestore(dev->spinlock, flags);
	if (cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	if (!handled)
		gb_printf(KERN_ERR, "%s: IRQ received, but not hantro's!\n", __func__);
	wake_up_interruptible_all(&dec_data->mc_wait_queue);
	return IRQ_HANDLED;
}


