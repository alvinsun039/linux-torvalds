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

#include <linux/errno.h>
#include <linux/err.h>
#include <linux/reset.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include "gb_device.h"
#include "gb_gpu.h"
#include "gb_stage.h"
#include "gb_mmu.h"
#include "gb_gem.h"
#include "gb_gpu_irq.h"
#include "common/xt.h"
#include "gb_ip_offset.h"

struct GB02STR126 vpu_offset_b = {
	GB02MAC1142, GB02MAC1143, GB02MAC1144
};
struct GB02STR127 audio_offset_b = {
	GB02MAC1145, GB02MAC1146, GB02MAC1147
};
struct GB02STR128 dcdp_offset_b = {

};

static int GB02FUNC66(struct GB02STR39 *gbdev)
{
	int err;

	gbdev->rstc = devm_reset_control_array_get(gbdev->dev, false, true);
	if (IS_ERR(gbdev->rstc)) {
		dev_err(gbdev->dev, "get reset failed %ld\n", PTR_ERR(gbdev->rstc));
		return PTR_ERR(gbdev->rstc);
	}

	err = reset_control_deassert(gbdev->rstc);
	if (err)
		return err;

	return 0;
}

static void GB02FUNC74(struct GB02STR39 *gbdev)
{
	reset_control_assert(gbdev->rstc);
}

int GB02FUNC75(struct GB02STR39 *gbdev, struct GB02STR67 *GB02STR153)
{
	int i;

	spin_lock_init(&gbdev->GB02STR35.ddr_lock);


	gbdev->global_gb_page = (struct GB02STR125 *)vzalloc(sizeof(struct GB02STR125) * GB02MAC323(GB02STR153));
	if (gbdev->global_gb_page == NULL) {
		gb_printf(KERN_ERR, "kmalloc global_gb_page failed!\n");
		return -ENOMEM;
	}

	for (i = 0; i < GB02MAC323(GB02STR153); i++) {
		gbdev->global_gb_page[i].pfn = -1;
		gbdev->global_gb_page[i].gpu_phy = -1;
	}

	gbdev->mmu_top_page = (struct GB02STR125 *)vzalloc(sizeof(struct GB02STR125) * GB02MAC324(GB02STR153));
	if (gbdev->mmu_top_page == NULL) {
		gb_printf(KERN_ERR, "kmalloc mmu top page failed!\n");
		vfree(gbdev->global_gb_page);
		return -ENOMEM;
	}

	for (i = 0; i < GB02MAC324(GB02STR153); i++) {
		gbdev->mmu_top_page[i].pfn = -1;
		gbdev->mmu_top_page[i].gpu_phy = -1;
	}

	atomic_set(&gbdev->GB02STR35.used_pages, 0);
	atomic_set(&gbdev->GB02STR35.used_mmu_pages, 0);

	gbdev->GB02STR35.page_highest_addr =
		GB02MAC319(GB02STR153);
	gbdev->GB02STR35.mmu_lowest_addr =
		GB02MAC328(GB02STR153);
	pr_info("[%s] mmu_low 0x%llx page_highest_addr:0x%llx\n",__func__,
		gbdev->GB02STR35.mmu_lowest_addr, gbdev->GB02STR35.page_highest_addr);

	gbdev->GB02STR35.usage = (volatile u64 *)vzalloc(sizeof(u64) * GB02MAC331(GB02STR153));
	gbdev->GB02STR35.mmu_usage = (volatile u64 *)vzalloc(sizeof(u64) * GB02MAC330(GB02STR153));

	return 0;
}

void GB02FUNC84(struct GB02STR39 *gbdev)
{
	struct GB02STR130 *ip_offset_base;

	ip_offset_base = &gbdev->ipoffset_base;
	ip_offset_base->vpu_offset.dec0_offset_base =
	 vpu_offset_b.dec0_offset_base;
	ip_offset_base->vpu_offset.dec1_offset_base =
	 vpu_offset_b.dec1_offset_base;
	ip_offset_base->vpu_offset.enc_offset_base =
	 vpu_offset_b.enc_offset_base;

	ip_offset_base->audio_offset.GB02STR127 =
	 audio_offset_b.GB02STR127;
	ip_offset_base->audio_offset.audio_interval =
	 audio_offset_b.audio_interval;
	ip_offset_base->audio_offset.audio_num = audio_offset_b.audio_num;
}

struct GB02STR47 *gl_perf_priv;
struct GB02STR47* GB02FUNC91(void)
{
	return gl_perf_priv;
}

void GB02FUNC95(struct GB02STR39 *gbdev)
{
	u32 i, nr_pages;
	int ret;
	struct GB02STR47 *gb_priv = NULL;
	phys_addr_t *phys = NULL;
	u64 flags = GB02MAC1240 | GB02MAC1260 |
		GB02MAC1241 | GB02MAC1259;

	nr_pages = ALIGN(GB02MAC1224, GB02MAC311) / GB02MAC311;

	phys = devm_kzalloc(gbdev->dev,
			nr_pages * sizeof(phys_addr_t), GFP_KERNEL);
	if (!phys) {
		dev_err(gbdev->dev, "%s alloc phys fail! size:0x%zx\n",
			__func__, nr_pages * sizeof(phys_addr_t));
		goto err_out;
	}
	gb_priv = devm_kzalloc(gbdev->dev,
		sizeof(struct GB02STR47), GFP_KERNEL);
	if (!gb_priv) {
		dev_err(gbdev->dev, "%s alloc gb_priv fail! size:0x%zx\n",
			__func__, sizeof(struct GB02STR47));
		goto err_out;
	}

	/* init vram phys for perf */
	for (i = 0; i < nr_pages; i++)
		phys[i] = GB02MAC504 + i * GB02MAC311;

	/* init gpu mmap for perf */
	ret = GB02FUNC767(gbdev->mmu_mode, GB02MAC1226 >> GB02MAC313,
				phys, nr_pages, flags, 0);
	if (ret) {
		dev_err(gbdev->dev, "%s mmap fail! nr_pages:%u\n",
			__func__, nr_pages);
		goto err_out;
	}
	mutex_init(&gb_priv->perfcnt_lock);
	gl_perf_priv = gb_priv;
	gl_perf_priv->gbdev = gbdev;
	return;
err_out:
	if (phys)
		devm_kfree(gbdev->dev, phys);
	if (gb_priv)
		devm_kfree(gbdev->dev, gb_priv);
}

int GB02FUNC107(struct GB02STR39 *gbdev, struct GB02STR67 *GB02STR153)
{
	int err;
	int gpu_reg_bar_id =
	 GB02FUNC465(gbdev->gb_pcie->GB02STR153);

	mutex_init(&gbdev->gb_file_priv_lock);
	mutex_init(&gbdev->sched_lock);
	mutex_init(&gbdev->vram_mutex);
	mutex_init(&gbdev->v2v_mutex);
	INIT_LIST_HEAD(&gbdev->scheduled_stages);
	INIT_LIST_HEAD(&gbdev->as_lru_list);

	spin_lock_init(&gbdev->as_lock);
	spin_lock_init(&gbdev->mmu_hw_lock);
	spin_lock_init(&gbdev->hw_irq_lock);
	mutex_init(&gbdev->mmu_as_lock);
	mutex_init(&gbdev->rb_mutex);

	err = GB02FUNC66(gbdev);
	if (err) {
		dev_err(gbdev->dev, "reset init failed %d\n", err);
		goto err_out1;
	}

	gbdev->gpu_reg_base =
	 gbdev->gb_pcie->pci_bars[gpu_reg_bar_id].mmio + GB02MAC1050;
	if (IS_ERR(gbdev->gpu_reg_base)) {
		dev_err(gbdev->dev, "failed to ioremap gpu_reg_base\n");
		err = PTR_ERR(gbdev->gpu_reg_base);
		goto err_out2;
	}

	gbdev->jsl.hw_job_limit = GB02MAC290;
	err = GB02FUNC886(gbdev);
	if (err) {
		dev_err(gbdev->dev, "irq init failed %d\n", err);
		goto err_out2;
	}
	err = GB02FUNC863(gbdev);
	if (err)
		goto err_out2;
	err = GB02FUNC1355(gbdev);
	if (err)
		goto err_out3;

	err = GB02FUNC75(gbdev, GB02STR153);
	if (err)
		goto err_out4;

	err = GB02FUNC974(gbdev);
	if (err)
		goto err_out4;

	GB02FUNC95(gbdev);
	return 0;
err_out4:
	GB02FUNC1359(gbdev);
err_out3:
	GB02FUNC867(gbdev);
err_out2:
	GB02FUNC74(gbdev);
err_out1:
	return err;
}

void GB02FUNC128(struct GB02STR39 *gbdev)
{
	GB02FUNC1359(gbdev);
	GB02FUNC867(gbdev);
	GB02FUNC972(gbdev);
	GB02FUNC74(gbdev);

	GB02FUNC893(gbdev);
}

const char *GB02FUNC130(struct GB02STR39 *gbdev, u32 exception_code)
{
	switch (exception_code) {
		/* Non-Fault Status code */
	case 0x00: return "NOT_STARTED/IDLE/OK";
	case 0x01: return "DONE";
	case 0x02: return "INTERRUPTED";
	case 0x03: return "STOPPED";
	case 0x04: return "TERMINATED";
	case 0x08: return "ACTIVE";
		/* Job exceptions */
	case 0x40: return "JOB_CONFIG_FAULT";
	case 0x41: return "JOB_POWER_FAULT";
	case 0x42: return "JOB_READ_FAULT";
	case 0x43: return "JOB_WRITE_FAULT";
	case 0x44: return "JOB_AFFINITY_FAULT";
	case 0x48: return "JOB_BUS_FAULT";
	case 0x50: return "INSTR_INVALID_PC";
	case 0x51: return "INSTR_INVALID_ENC";
	case 0x52: return "INSTR_TYPE_MISMATCH";
	case 0x53: return "INSTR_OPERAND_FAULT";
	case 0x54: return "INSTR_TLS_FAULT";
	case 0x55: return "INSTR_BARRIER_FAULT";
	case 0x56: return "INSTR_ALIGN_FAULT";
	case 0x58: return "DATA_INVALID_FAULT";
	case 0x59: return "TILE_RANGE_FAULT";
	case 0x5A: return "ADDR_RANGE_FAULT";
	case 0x60: return "OUT_OF_MEMORY";
		/* GPU exceptions */
	case 0x80: return "DELAYED_BUS_FAULT";
	case 0x88: return "SHAREABILITY_FAULT";
		/* MMU exceptions */
	case 0xC1: return "TRANSLATION_FAULT_LEVEL1";
	case 0xC2: return "TRANSLATION_FAULT_LEVEL2";
	case 0xC3: return "TRANSLATION_FAULT_LEVEL3";
	case 0xC4: return "TRANSLATION_FAULT_LEVEL4";
	case 0xC8: return "PERMISSION_FAULT";
	case 0xC9 ... 0xCF: return "PERMISSION_FAULT";
	case 0xD1: return "TRANSTAB_BUS_FAULT_LEVEL1";
	case 0xD2: return "TRANSTAB_BUS_FAULT_LEVEL2";
	case 0xD3: return "TRANSTAB_BUS_FAULT_LEVEL3";
	case 0xD4: return "TRANSTAB_BUS_FAULT_LEVEL4";
	case 0xD8: return "ACCESS_FLAG";
	case 0xD9 ... 0xDF: return "ACCESS_FLAG";
	case 0xE0 ... 0xE7: return "ADDRESS_SIZE_FAULT";
	case 0xE8 ... 0xEF: return "MEMORY_ATTRIBUTES_FAULT";
	}

	return "UNKNOWN";
}

void GB02FUNC136(struct GB02STR39 *gbdev)
{
	GB02FUNC839(gbdev);

	GB02FUNC856(gbdev);
	GB02FUNC937(gbdev);
	GB02FUNC1307(gbdev);
	GB02FUNC1305(gbdev);
}

#ifdef CONFIG_PM
int GB02FUNC137(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct GB02STR39 *gbdev = platform_get_drvdata(pdev);

	GB02FUNC136(gbdev);

	return 0;
}

int GB02FUNC138(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct GB02STR39 *gbdev = platform_get_drvdata(pdev);

	if (!GB02FUNC1368(gbdev))
		return -EBUSY;

	GB02FUNC860(gbdev);

	return 0;
}
#endif
