
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

#include "gb_device.h"
#include "gb_gpu.h"
#include "gb_regs.h"
#include "gb_gpu_irq.h"
#include "gb_stage.h"
#include "common/gb_irq.h"
#include "common/gb_common.h"
#include "gb_pcie_info.h"
#include "vpu/vpu_dec/vpu_vcmd.h"
#include "vpu/vpu_enc/vpu_vc8000E_vcmd.h"

irqreturn_t GB02FUNC877(int irq, void *data)
{
	u32 stage_status, mmu_status, gpu_status;
	struct GB02STR39 *gbdev = data;
	irqreturn_t ret = IRQ_NONE;

	stage_status = reg_read(gbdev, GB02MAC1501);
	mmu_status = reg_read(gbdev, GB02MAC1546);
	gpu_status = reg_read(gbdev, GB02MAC1377);

	// gb_printf(KERN_INFO, "IRQ info: gpu: 0x%x mmu: %x stage: %x\n", gpu_status, mmu_status, stage_status);
	if (gpu_status) {
		ret = GB02FUNC833(gbdev, gpu_status);
	}
	if (mmu_status) {
		ret = GB02FUNC959(irq, data);
	}
	if (stage_status) {
		ret = GB02FUNC1339(irq, data);
	}

	if(!(gpu_status || mmu_status || stage_status)) {
		gb_printf(KERN_INFO,
			"Error: triggerd multiple irq once gpu: 0x%d mmu: 0x%d stage: 0x%d\n",
			gpu_status, mmu_status, stage_status);
		return IRQ_HANDLED;
	}

	return ret;
}

void GB02FUNC882(struct GB02STR39 *gbdev)
{
	synchronize_irq(gbdev->gb_pcie->pdev->irq);
}

int GB02FUNC886(struct GB02STR39 *gbdev)
{
	int ret;
#ifdef GB02MAC287

	int i, tmp;

	for (i = 0; i < GPU_IRQ_TOTAL; i++) {
		tmp = GB02FUNC373(gbdev->gb_pcie, irq_type_gpu, i);
		gb_printf(KERN_INFO, "gpu irq i = %d, irq: %d\n",
		i, gbdev->gb_pcie->irqs[i]);

		switch (i) {
		case GPU_MMU_IRQ:
			ret = devm_request_threaded_irq(gbdev->dev, gbdev->gb_pcie->irqs[tmp],
				GB02FUNC959,
				GB02FUNC961,
				IRQF_SHARED, "GPU_MMU", gbdev);
			break;
		case GPU_JOB_IRQ:
			ret = devm_request_threaded_irq(gbdev->dev,
							gbdev->gb_pcie->irqs[tmp],
							GB02FUNC1336,
							GB02FUNC1334,
							IRQF_SHARED, "GPU_JOB", gbdev);
			break;
		case GPU_GPU_IRQ:
			ret = devm_request_irq(gbdev->dev, gbdev->gb_pcie->irqs[tmp],
				GB02FUNC831, IRQF_SHARED, "GPU_GPU", gbdev);
			break;
		case GPU_EVENT_IRQ:
			ret = devm_request_irq(gbdev->dev, gbdev->gb_pcie->irqs[tmp],
				GB02FUNC877, IRQF_SHARED, "GPU_EVENT", gbdev);
			break;
		default:
			break;
		}

		if (ret) {
			gb_err(gbdev->dev, "Can't request interrupt %d (index %d)\n",
				gbdev->gb_pcie->irqs[i], i);
#ifdef CONFIG_SPARSE_IRQ
			gb_err(gbdev->dev, "You have CONFIG_SPARSE_IRQ support enabled - is the interrupt number correct for this configuration?\n");
#endif /* CONFIG_SPARSE_IRQ */
			goto release;
		}
	}
#else
	ret = devm_request_irq(gbdev->dev, gbdev->gb_pcie->pdev->irq,
	 GB02FUNC877, IRQF_SHARED, "GenBu", gbdev);
	if (ret) {
		dev_err(gbdev->dev, "Failed to request irq handler");
	}
#endif

	return 0;

#ifdef GB02MAC287
release:
	while (i-- > 0)
		devm_free_irq(gbdev->dev, gbdev->gb_pcie->irqs[i], gbdev);

	return ret;
#endif
}

int GB02FUNC893(struct GB02STR39 *gbdev)
{
#ifdef GB02MAC287
	int i;

	for (i = 0; i < gbdev->gb_pcie->irq_vector_num; i++)
		devm_free_irq(gbdev->dev, gbdev->gb_pcie->irqs[i], gbdev);

	return 0;
#endif

	devm_free_irq(gbdev->dev, gbdev->gb_pcie->pdev->irq, gbdev);

	return 0;
}
void GB02FUNC895(struct GB02STR39 *gbdev)
{
	int irq_index = 0;

	#ifdef GB02MAC483
	irq_index = GB02FUNC373(gbdev->gb_pcie, irq_type_dec0, 0);
	GB02FUNC948(gbdev->pcie_info.irqs[irq_index]);
	#endif
	#ifdef GB02MAC484
	irq_index =
	 GB02FUNC373(gbdev->gb_pcie, irq_type_enc, 0);
	GB02FUNC1655(gbdev->pcie_info.irqs[irq_index]);
	#endif
}


