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
#include <linux/errno.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include "common/gb_common.h"
#include "vpu_vc8000E_irq.h"
#include "vpu_vc8000E_vcmd.h"
#include "vpu/vpu_comm/vpu_list.h"
#include "vpu/vpu_comm/vpu_vcmd_registers.h"
#include "vpu/vpu_comm/vpu_comm.h"
#include "vpu/vpu_comm/vpu_basetype.h"

static void GB02FUNC1449(struct GB02STR7 *dev)
{
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *base_cmdbuf_node = NULL;
	struct GB02STR18 *GB02STR18 = NULL;

	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	while (1) {
		if (new_cmdbuf_node == NULL)
			break;
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if ((GB02STR18->cmdbuf_run_done == 0))
			break;
		new_cmdbuf_node = new_cmdbuf_node->next;
	}
	base_cmdbuf_node = new_cmdbuf_node;
	GB02FUNC1616(dev, base_cmdbuf_node);
	GB02FUNC1542(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0)
		GB02FUNC1554(dev, base_cmdbuf_node); /*restart new command*/
}

static int GB02FUNC1453(struct GB02STR7 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	size_t base_ddr_addr = 0;
	size_t exe_cmdbuf_busAddress;
	bi_list_node *base_cmdbuf_node = NULL;
	u32 tmp_num = *cmdbuf_processed_num;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	base_ddr_addr = enc_data->enc_base_ddr_addr;

	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = enc_data->enc_global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		exe_cmdbuf_busAddress =
			VCMDGetAddrRegisterValue((const void *)dev->hwregs,
			dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR);
		/* find the cmderror cmdbuf */
		while (1) {
			if (new_cmdbuf_node == NULL) {
				spin_unlock_irqrestore(dev->spinlock, flags);
				return -1;
			}
			GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
			if ((((GB02STR18->cmdbuf_bus_address -
			 base_ddr_addr) <= exe_cmdbuf_busAddress) &&
			  (((GB02STR18->cmdbuf_bus_address -
			   base_ddr_addr + GB02STR18->cmdbuf_size)
			    > exe_cmdbuf_busAddress)))
				&& (GB02STR18->cmdbuf_run_done == 0))
				break;
			new_cmdbuf_node = new_cmdbuf_node->next;
		}
	}
	base_cmdbuf_node = new_cmdbuf_node;

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
	base_cmdbuf_node = base_cmdbuf_node->next;
	GB02FUNC1616(dev, base_cmdbuf_node);
	if (enc_data->software_triger_abort == 0) {
		GB02FUNC1542(dev, base_cmdbuf_node);
		if (dev->sw_cmdbuf_rdy_num != 0)
			GB02FUNC1554(dev, base_cmdbuf_node);
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	wake_up_interruptible_all(dev->wait_abort_queue);
	return 0;
}

static int GB02FUNC1461(struct GB02STR7 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	size_t base_ddr_addr = 0;
	size_t exe_cmdbuf_busAddress;
	bi_list_node *base_cmdbuf_node = NULL;
	u32 tmp_num = *cmdbuf_processed_num;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	base_ddr_addr = enc_data->enc_base_ddr_addr;

	/* bus error, don't need to reset where to record status? */
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = enc_data->enc_global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		exe_cmdbuf_busAddress =
			VCMDGetAddrRegisterValue((const void *)dev->hwregs,
				dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR);
		/* find the cmderror cmdbuf */
		while (1) {
			if (new_cmdbuf_node == NULL) {
				spin_unlock_irqrestore(dev->spinlock, flags);
				return -1;
			}
			GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
			if ((((GB02STR18->cmdbuf_bus_address -
			 base_ddr_addr) <= exe_cmdbuf_busAddress) &&
			  (((GB02STR18->cmdbuf_bus_address -
			   base_ddr_addr + GB02STR18->cmdbuf_size)
			    > exe_cmdbuf_busAddress)))
				&& (GB02STR18->cmdbuf_run_done == 0))
				break;
			new_cmdbuf_node = new_cmdbuf_node->next;
		}
	}
	base_cmdbuf_node = new_cmdbuf_node;

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
	new_cmdbuf_node = base_cmdbuf_node;
	if (new_cmdbuf_node != NULL) {
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		GB02STR18->executing_status = GB02MAC149;
	}
	base_cmdbuf_node = base_cmdbuf_node->next;
	GB02FUNC1616(dev, base_cmdbuf_node);
	GB02FUNC1542(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0)
		GB02FUNC1554(dev, base_cmdbuf_node); /*restart new command*/
	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	return 0;
}

static void GB02FUNC1464(struct GB02STR7 *dev)
{
	u32 result;

	if (dev->hwregs != NULL) {
		/* disable interrupt at first */
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC515, 0x0000);
		/* reset all	*/
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC513, 0x0002);
		/* read status register */
		result = GB02FUNC313((const void *)dev->hwregs,
			GB02MAC514);
		/* clean status register */
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC514, result);
	}
}

static int GB02FUNC1466(struct GB02STR7 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	size_t base_ddr_addr = 0;
	size_t exe_cmdbuf_busAddress;
	bi_list_node *base_cmdbuf_node = NULL;
	u32 tmp_num = *cmdbuf_processed_num;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	base_ddr_addr = enc_data->enc_base_ddr_addr;

	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = enc_data->enc_global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		exe_cmdbuf_busAddress =
			VCMDGetAddrRegisterValue((const void *)dev->hwregs,
				dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR);
		/* find the timeout cmdbuf */
		while (1) {
			if (new_cmdbuf_node == NULL) {
				spin_unlock_irqrestore(dev->spinlock, flags);
				return -1;
			}
			GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
			if ((((GB02STR18->cmdbuf_bus_address -
			 base_ddr_addr) <= exe_cmdbuf_busAddress) &&
			  (((GB02STR18->cmdbuf_bus_address -
			   base_ddr_addr + GB02STR18->cmdbuf_size) >
			    exe_cmdbuf_busAddress)))
				 && (GB02STR18->cmdbuf_run_done == 0))
				break;
			new_cmdbuf_node = new_cmdbuf_node->next;
		}
	}
	base_cmdbuf_node = new_cmdbuf_node;
	new_cmdbuf_node = new_cmdbuf_node->previous;

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
	GB02FUNC1616(dev, base_cmdbuf_node);
	GB02FUNC1542(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0) {
		GB02FUNC1464(dev);
		GB02FUNC1554(dev, base_cmdbuf_node); /*restart new command*/
	}
	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	return 0;
}

static int GB02FUNC1470(struct GB02STR7 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	size_t base_ddr_addr = 0;
	size_t exe_cmdbuf_busAddress;
	bi_list_node *base_cmdbuf_node = NULL;
	u32 tmp_num = *cmdbuf_processed_num;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();
	base_ddr_addr = enc_data->enc_base_ddr_addr;

	/* command error,don't need to reset */
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = enc_data->enc_global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		exe_cmdbuf_busAddress =
			VCMDGetAddrRegisterValue((const void *)dev->hwregs,
				dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR);
		/* find the cmderror cmdbuf */
		while (1) {
			if (new_cmdbuf_node == NULL) {
				spin_unlock_irqrestore(dev->spinlock, flags);
				return -1;
			}
			GB02STR18 =
			 (struct GB02STR18 *)new_cmdbuf_node->data;
			if ((((GB02STR18->cmdbuf_bus_address -
			 base_ddr_addr) <= exe_cmdbuf_busAddress)
			  && (((GB02STR18->cmdbuf_bus_address -
			   base_ddr_addr + GB02STR18->cmdbuf_size)
			    > exe_cmdbuf_busAddress)))
				 && (GB02STR18->cmdbuf_run_done == 0))
				break;
			new_cmdbuf_node = new_cmdbuf_node->next;
		}
	}
	base_cmdbuf_node = new_cmdbuf_node;
	/* this cmdbuf and cmdbufs prior to itself, run_done = 1 */
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
	new_cmdbuf_node = base_cmdbuf_node;
	if (new_cmdbuf_node != NULL) {
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		GB02STR18->executing_status = GB02MAC148;
	}
	base_cmdbuf_node = base_cmdbuf_node->next;
	GB02FUNC1616(dev, base_cmdbuf_node);
	GB02FUNC1542(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0)
		GB02FUNC1554(dev, base_cmdbuf_node); /*restart new command*/
	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	return 0;
}

static int GB02FUNC1473(struct GB02STR7 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	bi_list_node *base_cmdbuf_node = NULL;
	u32 tmp_num = *cmdbuf_processed_num;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	/* end command interrupt */
	new_cmdbuf_node = dev->list_manager.head;
	dev->working_state = GB02MAC145;
	if (dev->hw_version_id > GB02MAC186) {
		new_cmdbuf_node = enc_data->enc_global_cmdbuf_node[cmdbuf_id];
		if (new_cmdbuf_node == NULL) {
			gb_printf(KERN_ERR, "%s: error, new_cmdbuf_node == NULL\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	} else {
		/* find the end cmdbuf */
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
	/* this cmdbuf and cmdbufs prior to itself, run_done = 1 */
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
	base_cmdbuf_node = base_cmdbuf_node->next;
	GB02FUNC1616(dev, base_cmdbuf_node);
	GB02FUNC1542(dev, base_cmdbuf_node);
	if (dev->sw_cmdbuf_rdy_num != 0)
		GB02FUNC1554(dev, base_cmdbuf_node); /*restart new command*/

	spin_unlock_irqrestore(dev->spinlock, flags);
	if (*cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	return 0;
}

static int GB02FUNC1476(struct GB02STR7 *dev,
	u32 cmdbuf_id, unsigned long flags, u32 *cmdbuf_processed_num)
{
	bi_list_node *new_cmdbuf_node = NULL;
	u32 tmp_num = *cmdbuf_processed_num;
	struct GB02STR18 *GB02STR18 = NULL;
	struct GB02STR186 *enc_data = NULL;

	enc_data = GB02FUNC1639();

	if (dev->hw_version_id <= GB02MAC186) {
		if (cmdbuf_id >= GB02MAC159) {
			gb_printf(KERN_ERR, "%s: error, cmdbuf_id greater than ceiling\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return -1;
		}
	}
	new_cmdbuf_node = enc_data->enc_global_cmdbuf_node[cmdbuf_id];
	if (new_cmdbuf_node == NULL) {
		gb_printf(KERN_ERR, "%s: error cmdbuf_id !!\n", __func__);
		spin_unlock_irqrestore(dev->spinlock, flags);
		return -1;
	}
	// interrupt cmdbuf and cmdbufs prior to itself, run_done = 1
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
	return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
irqreturn_t enc_hantrovcmd_isr(int irq, void *dev_id,
	struct pt_regs *regs)
#else
irqreturn_t enc_hantrovcmd_isr(int irq, void *dev_id)
#endif
{
	unsigned int handled = 0;
	struct GB02STR7 *dev = (struct GB02STR7 *)dev_id;
	u32 irq_status = 0;
	int ret_val;
	unsigned long flags;
	u32 cmdbuf_processed_num = 0;
	u32 cmdbuf_id = 0;

/*
 * If core is not reserved by any user,
 * but irq is received, just clean it
 */
	spin_lock_irqsave(dev->spinlock, flags);
	if (dev->list_manager.head == NULL) {
		gb_printf(KERN_INFO, "%s: received IRQ but core has nothing to do.\n",
		 __func__);
		irq_status = GB02FUNC313((const void *)dev->hwregs,
			GB02MAC514);
		GB02FUNC318((const void *)dev->hwregs,
			GB02MAC514, irq_status);
		spin_unlock_irqrestore(dev->spinlock, flags);
		return IRQ_HANDLED;
	}

	//gb_printf(KERN_INFO, "%s: received IRQ to do something!!\n", __func__);
	irq_status = GB02FUNC313((const void *)dev->hwregs,
		GB02MAC514);
	if (!irq_status) {
		spin_unlock_irqrestore(dev->spinlock, flags);
		return IRQ_HANDLED;
	}

	//gb_printf(KERN_INFO, "%s: dev->core_id : %d, irq_status:%x\n",
	//	__func__, dev->core_id, irq_status);
	GB02FUNC318((const void *)dev->hwregs,
		GB02MAC514, irq_status);
	dev->reg_mirror[GB02MAC514 / 4] = irq_status;
	if ((dev->hw_version_id > GB02MAC186) && (irq_status & 0x3f)) {
		/* if error,read from register directly. */
		cmdbuf_id = GB02FUNC325((const void *)dev->hwregs,
			dev->reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID);
		if (cmdbuf_id >= GB02MAC159) {
			gb_printf(KERN_ERR, "%s: cmdbuf_id great than ceiling error\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return IRQ_HANDLED;
		}
	} else if (dev->hw_version_id > GB02MAC186) {
		/* read cmdbuf id from ddr */
		cmdbuf_id = *(dev->vcmd_reg_mem_virtual_address +
			GB02MAC143);
		//gb_printf(KERN_INFO, "%s: cmdbuf_id %d from virtual!!\n",
		// __func__, cmdbuf_id);
		if (cmdbuf_id >= GB02MAC159) {
			gb_printf(KERN_ERR, "%s: error cmdbuf_id greater than ceiling\n",
			 __func__);
			spin_unlock_irqrestore(dev->spinlock, flags);
			return IRQ_HANDLED;
		}
	}

	if (GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_RESET)) {
		GB02FUNC1449(dev);
		handled++;
		spin_unlock_irqrestore(dev->spinlock, flags);
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_ABORT)) {
		ret_val = GB02FUNC1453(dev, cmdbuf_id,
			flags, &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_BUSERR)) {
		ret_val = GB02FUNC1461(dev, cmdbuf_id,
			flags, &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_TIMEOUT)) {
		ret_val = GB02FUNC1466(dev, cmdbuf_id,
			flags, &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
		HWIF_VCMD_IRQ_CMDERR)) {
		ret_val = GB02FUNC1470(dev, cmdbuf_id,
			flags, &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_ENDCMD)) {
		ret_val = GB02FUNC1473(dev, cmdbuf_id,
			flags, &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
		return IRQ_HANDLED;
	}
	if (dev->hw_version_id <= GB02MAC186)
		cmdbuf_id = GB02FUNC344(dev->reg_mirror,
			HWIF_VCMD_IRQ_INTCMD);
	if (cmdbuf_id) {
		ret_val = GB02FUNC1476(dev, cmdbuf_id,
			flags, &cmdbuf_processed_num);
		if (ret_val != 0)
			return IRQ_HANDLED;
		handled++;
	}

	spin_unlock_irqrestore(dev->spinlock, flags);
	if (cmdbuf_processed_num)
		wake_up_interruptible_all(dev->wait_queue);
	if (!handled)
		gb_printf(KERN_INFO, "%s: IRQ received, but not hantro's!\n", __func__);
	return IRQ_HANDLED;
}


