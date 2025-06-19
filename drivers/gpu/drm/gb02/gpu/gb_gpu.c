/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/bitfield.h>
#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/pci.h>

#include "common/gb_uk.h"
#include "gb_device.h"
#include "gb_gpu.h"
#include "gb_regs.h"
#include "gb_gpu_irq.h"


irqreturn_t GB02FUNC831(int irq, void *data)
{
	irqreturn_t ret = IRQ_NONE;
	u32 gpu_status;
	struct GB02STR39 *gbdev = data;

	gpu_status = reg_read(gbdev, GB02MAC1377);
	if (!gpu_status) {
		gb_printf(KERN_INFO,
			"Error: triggerd gpu_status irq once gpu: 0x%x\n",
			gpu_status);
		return IRQ_NONE;
	}

	if (gpu_status)
		ret = GB02FUNC833(gbdev, gpu_status);

	return ret;
}

irqreturn_t GB02FUNC833(struct GB02STR39 *gbdev, u32 state)
{
	u32 fault_status = gpu_read(gbdev, GB02MAC1406);

	if (state & (GB02MAC1380 | GB02MAC1381)) {
		u64 address = (u64) gpu_read(gbdev, GB02MAC1408) << 32;
		address |= gpu_read(gbdev, GB02MAC1407);

		dev_warn(gbdev->dev, "GPU Fault 0x%08x (%s) at 0x%016llx\n",
			 fault_status & 0xFF, GB02FUNC130(gbdev, fault_status),
			 address);

		if (state & GB02MAC1381)
			dev_warn(gbdev->dev, "There were multiple GPU faults - some have not been reported\n");

		gpu_write(gbdev, GB02MAC1376, 0);
	}

	gpu_write(gbdev, GB02MAC1375, state);

	return IRQ_HANDLED;
}

void GB02FUNC837(struct GB02STR39 *gbdev)
{
	gpu_write(gbdev, GB02MAC1375, GB02MAC1390);
	gpu_write(gbdev, GB02MAC1376, 0);
}

int GB02FUNC839(struct GB02STR39 *gbdev)
{
	int ret;
	u32 val;
	u32 stage_status, mmu_status, gpu_status;
	u32 stage_rawstats;
	gpu_write(gbdev, GB02MAC1376, 0);
	gpu_write(gbdev, GB02MAC1375, GB02MAC1383);
	gpu_write(gbdev, GB02MAC1403, GPU_COMMAND_SOFT_RESET);

	gb_printf(KERN_INFO, "===%s start===\n", __func__);

	ret = readl_relaxed_poll_timeout(gbdev->gpu_reg_base + GB02MAC1373,
		val, val & GB02MAC1383, 100, 10000);

	stage_status = reg_read(gbdev, GB02MAC1501);
	mmu_status = reg_read(gbdev, GB02MAC1546);
	gpu_status = reg_read(gbdev, GB02MAC1377);
	stage_rawstats = reg_read(gbdev, GB02MAC1498);

	gb_printf(KERN_INFO, "IRQ info: gpu: 0x%x mmu: %x stage: %x stage_rawstats:%x\n",
		gpu_status, mmu_status, stage_status, stage_rawstats);

	if (ret) {
		dev_err(gbdev->dev, "gpu soft reset timed out\n");
		return ret;
	}

	gpu_write(gbdev, GB02MAC1375, GB02MAC1390);
	gpu_write(gbdev, GB02MAC1376, GB02MAC1390);
	gb_printf(KERN_INFO, "===%s end===\n", __func__);

	return 0;
}

static void GB02FUNC842(struct GB02STR39 *gbdev)
{
	u32 quirks = 0;

	quirks = gpu_read(gbdev, GB02MAC1599);

	gpu_write(gbdev, GB02MAC1599, quirks);

	quirks = gpu_read(gbdev, GB02MAC1600);

	quirks &= ~(GB02MAC1853 |
		    GB02MAC1858);

	gpu_write(gbdev, GB02MAC1600, quirks);
}

static void GB02FUNC846(struct GB02STR39 *gbdev)
{
	u32 gpu_id, num_ss, major, minor, status, rev;
	int i;

	gbdev->features.l2_features = gpu_read(gbdev, L2_FEATURES);
	gbdev->features.tiler_features = gpu_read(gbdev, TILER_FEATURES);
	gbdev->features.mem_features = gpu_read(gbdev, MEM_FEATURES);
	gbdev->features.mmu_features = gpu_read(gbdev, MMU_FEATURES);
	gbdev->features.thread_features = gpu_read(gbdev, THREAD_FEATURES);
	gbdev->features.max_threads = gpu_read(gbdev, GB02MAC1426);
	gbdev->features.thread_max_workgroup_sz = gpu_read(gbdev, GB02MAC1427);
	gbdev->features.thread_max_barrier_sz = gpu_read(gbdev, GB02MAC1429);
	gbdev->features.coherency_features = gpu_read(gbdev, COHERENCY_FEATURES);
	gbdev->features.texture_features[0] = gpu_read(gbdev, GB02MAC1430);
	gbdev->features.texture_features[1] = gpu_read(gbdev, GB02MAC1431);
	gbdev->features.texture_features[2] = gpu_read(gbdev, GB02MAC1432);

	gbdev->features.as_present = gpu_read(gbdev, AS_PRESENT);
	/* we use as[GB02MAC289] for perf & gpu utilization */
	gbdev->features.as_present &= (~(0x1 << GB02MAC289));

	gbdev->features.ss_present = gpu_read(gbdev, SS_PRESENT);
	num_ss = hweight32(gbdev->features.ss_present);
	for (i = 0; i < num_ss; i++)
		gbdev->features.ss_features[i] = gpu_read(gbdev, GB02MAC1591(i));

	gbdev->features.shader_present = gpu_read(gbdev, SHADER_PRESENT_LO);
	gbdev->features.shader_present |= (u64)gpu_read(gbdev, SHADER_PRESENT_HI) << 32;

	gbdev->features.tiler_present = gpu_read(gbdev, TILER_PRESENT_LO);
	gbdev->features.tiler_present |= (u64)gpu_read(gbdev, TILER_PRESENT_HI) << 32;

	gbdev->features.l2_present = gpu_read(gbdev, GB02MAC1446);
	gbdev->features.l2_present |= (u64)gpu_read(gbdev, GB02MAC1448) << 32;
	gbdev->features.nr_core_groups = hweight64(gbdev->features.l2_present);

	gbdev->features.stack_present = 0;

	gbdev->features.revision = GB02MAC879;//GB02MAC875;
	gbdev->features.id = GB02MAC878;//GB02MAC874;

	major = (gbdev->features.revision >> 12) & 0xf;
	minor = (gbdev->features.revision >> 4) & 0xff;
	status = gbdev->features.revision & 0xf;
	rev = gbdev->features.revision;

	gpu_id = gbdev->features.id;

	dev_info(gbdev->dev, "Features: L2:0x%08x Tiler:0x%08x Mem:0x%0x MMU:0x%08x AS:0x%x JS:0x%x",
		 gbdev->features.l2_features,
		 gbdev->features.tiler_features,
		 gbdev->features.mem_features,
		 gbdev->features.mmu_features,
		 gbdev->features.as_present,
		 gbdev->features.ss_present);

	dev_info(gbdev->dev, "shader_present=0x%0llx l2_present=0x%0llx",
		 gbdev->features.shader_present, gbdev->features.l2_present);
}

void GB02FUNC854(void)
{
	struct GB02STR70 *pcie_dev = GB02FUNC518();
	struct GB02STR39 *gbdev = pcie_dev->gbdev;

	gpu_write(gbdev, GB02MAC1463,
		gbdev->features.l2_present & 0xffffffff);
	gpu_write(gbdev, GB02MAC1464,
		gbdev->features.l2_present & 0xffffffff);
}

void GB02FUNC855(void)
{
	struct GB02STR70 *pcie_dev = GB02FUNC518();
	struct GB02STR39 *gbdev = pcie_dev->gbdev;

	gpu_write(gbdev, GB02MAC1471, 0xffffffff);
	gpu_write(gbdev, GB02MAC1472, 0xffffffff);
}

void GB02FUNC856(struct GB02STR39 *gbdev)
{
	int ret;
	u32 val;

	/* Just turn on everything for now */
	gpu_write(gbdev, GB02MAC1463,
		gbdev->features.l2_present & 0xffffffff);
	/* GB02MAC1455 only support 1, no matter how many l2 are ready */
	ret = readl_relaxed_poll_timeout(gbdev->gpu_reg_base + GB02MAC1455,
		val, val == 1, 100, 1000);
		//val, val == gbdev->features.l2_present, 100, 1000);

	gpu_write(gbdev, STACK_PWRON_LO, gbdev->features.stack_present);
	ret |= readl_relaxed_poll_timeout(gbdev->gpu_reg_base + STACK_READY_LO,
		val, val == gbdev->features.stack_present, 100, 1000);

	gpu_write(gbdev, SHADER_PWRON_LO, gbdev->features.shader_present);
	ret |= readl_relaxed_poll_timeout(gbdev->gpu_reg_base + SHADER_READY_LO,
		val, val == gbdev->features.shader_present, 100, 1000);

	gpu_write(gbdev, TILER_PWRON_LO, gbdev->features.tiler_present);
	ret |= readl_relaxed_poll_timeout(gbdev->gpu_reg_base + TILER_READY_LO,
		val, val == gbdev->features.tiler_present, 100, 1000);

	if (ret)
		dev_err(gbdev->dev, "error powering up gpu");
}

void GB02FUNC860(struct GB02STR39 *gbdev)
{
	gpu_write(gbdev, TILER_PWROFF_LO, 0xffffffff);
	gpu_write(gbdev, SHADER_PWROFF_LO, 0xffffffff);
	gpu_write(gbdev, STACK_PWROFF_LO, 0xffffffff);
	gpu_write(gbdev, GB02MAC1471, 0xffffffff);
}

int GB02FUNC863(struct GB02STR39 *gbdev)
{
	int err;

	err = GB02FUNC839(gbdev);
	if (err)
		return err;

	GB02FUNC846(gbdev);

	dma_set_mask_and_coherent(gbdev->dev,
		DMA_BIT_MASK(FIELD_GET(0xff00, gbdev->features.mmu_features)));

	GB02FUNC842(gbdev);
	GB02FUNC856(gbdev);

	return 0;
}

void GB02FUNC867(struct GB02STR39 *gbdev)
{
	GB02FUNC860(gbdev);
}

u32 GB02FUNC869(struct GB02STR39 *gbdev)
{
	return gpu_read(gbdev, GB02MAC1405);
}
