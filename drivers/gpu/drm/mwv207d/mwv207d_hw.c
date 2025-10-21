/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/kernel.h>
#include <linux/pci.h>

#include "mwv207d_drv.h"
#include "mwv207d_irq.h"
#include "mwv207d_vbios.h"
#include "mwv207d_db.h"

static inline u32 hwinfo_read(struct mwv207d_device *mdev, u32 reg)
{
	u32 regbase = mdev->hw.is_pf ?
		0x25FC00 : 0xF000;
	return mdev_read(mdev, regbase + reg);
}

static inline void hwinfo_write(struct mwv207d_device *mdev, u32 reg, u32 dat)
{
	u32 regbase = mdev->hw.is_pf ?
		0x25FC00 : 0xF000;
	mdev_write(mdev, regbase + reg, dat);
}

static void mwv207d_hwinfo_verify(struct mwv207d_device *mdev)
{
	struct mwv207d_hwinfo *hw = &mdev->hw;

	BUG_ON(!hw->vram_size || hw->vram_size > (SZ_4G*8));

	BUG_ON(!hw->nr_3d_clusters ||
	       hw->nr_3d_clusters > 0x8);

	if (!hw->is_pf) {
		if (!hw->nr_vconnectors)
			hw->nr_vconnectors = 1;
		if (!hw->vconn_max_height || !hw->vconn_max_width) {
			hw->vconn_max_width = 1920;
			hw->vconn_max_height = 1080;
		}

		dev_info(mdev->dev, "Connectors:    %d (ModeMax: %dx%d)",
			 hw->nr_vconnectors,
			 hw->vconn_max_width,
			 hw->vconn_max_height);
	}

	if (hw->is_emulation)
		return;

	BUG_ON(!hw->nr_2d_clusters ||
	       hw->nr_2d_clusters > 0x8);
	BUG_ON(hw->nr_fus_clusters > 0x8);

	BUG_ON(hw->irq_mode);
}

static void mwv207d_hwinfo_parse(struct mwv207d_device *mdev)
{
	struct mwv207d_hwinfo *hw = &mdev->hw;
	u32 val;

	hw->vram_size = hwinfo_read(mdev, 0x0);
	hw->vram_size <<= 20;

	val = hwinfo_read(mdev, 0x4);
	hw->nr_3d_clusters = val & 0x1f;
	hw->nr_2d_clusters = (val >> 5) & 0x1f;
	hw->nr_fus_clusters = (val >> 10) & 0x1f;
	hw->nr_pipe[0x0] = !!hw->nr_3d_clusters;
	hw->nr_pipe[0x3] = !!hw->nr_2d_clusters;
	hw->nr_pipe[0x5] = !!hw->nr_fus_clusters;

	hw->nr_pipe[0x4] = 0;
	hw->nr_pipe[0x1] = (val >> 19) & 0xf;
	hw->nr_pipe[0x2] = (val >> 23) & 0xf;
	hw->is_emulation = !!((val >> 31) & 1);

	val = hwinfo_read(mdev, 0x8);
	hw->vf_sel = val & 0xf;
	hw->irq_mode = (val >> 4) & 1;
	hw->nr_vconnectors = (val >> 5) & 0xf;
	hw->has_gart = (val >> 9) & 1;

	hw->vram_offset = hwinfo_read(mdev, 0xC);
	hw->vram_offset <<= 20;

	val = hwinfo_read(mdev, 0x10);
	hw->vconn_max_height = val & 0xffff;
	hw->vconn_max_width = (val >> 16) & 0xffff;
}

int mwv207d_hw_vf_reset_3d(struct mwv207d_device *mdev, unsigned long timeout)
{
	BUG_ON(mdev->hw.is_pf);

	hwinfo_write(mdev, 0x14, 0x1);
	return wait_for((hwinfo_read(mdev, 0x14) & 0x1) == 0,
			timeout);
}

static void mwv207d_display_blackout(struct mwv207d_device *mdev)
{
	u32 reg;
	int i;

	for (i = 0; i < 4; i++) {
		reg = 0x370000 + 0x1000 * i;
		mdev_write(mdev, reg + 0x1f0, 0x0);
		mdev_write(mdev, reg + 0x78, 0x0);
		mdev_write(mdev, reg + 0x1ec, 0x0);
		mdev_write(mdev, reg + 0x1f8, 0x0);
		mdev_write(mdev, reg + 0x1f0, 0x1);
	}
}

int mwv207d_hw_init(struct mwv207d_device *mdev)
{
	struct pci_dev *pdev = to_pci_dev(mdev->dev);
	struct mwv207d_hwinfo *hw = &mdev->hw;
	resource_size_t base, size;
	int ret;

	base = pci_resource_start(pdev, 0x2);
	size = pci_resource_len(pdev, 0x2);
	if (!base || !size) {
		dev_err(mdev->dev, "error, pci <%04x:%04x> bar%d no resource!",
			pdev->vendor, pdev->device, 0x2);
		return -ENODEV;
	}
	mdev->mmio = devm_ioremap(mdev->dev, base, size);
	if (!mdev->mmio) {
		dev_err(mdev->dev, "failed to map mmio");
		return -ENOMEM;
	}

	hw->is_pf = size > SZ_2M;
	hw->remap_codec = size < SZ_2M;

	mdev->gtt_size = SZ_2G;
	mdev->ap_base = pci_resource_start(pdev, 0x0);
	mdev->ap_size = pci_resource_len(pdev, 0x0);
	if (!mdev->ap_base || !mdev->ap_size) {
		dev_err(mdev->dev, "error, pci <%04x:%04x> bar%d no resource!",
			pdev->vendor, pdev->device, 0x0);
		return -ENODEV;
	}
	mdev->pgtable_segment_size = min_t(u64,
			mdev->pgtable_segment_size, mdev->ap_size);

	mdev->pt_bits = mdev->pgtable_segment_size / SZ_4K;
	mdev->pt_bitmap = devm_kcalloc(mdev->dev,
			BITS_TO_LONGS(mdev->pt_bits),
			sizeof(unsigned long), GFP_KERNEL);
	if (!mdev->pt_bitmap)
		return -ENOMEM;
	spin_lock_init(&mdev->bitmap_lock);

	mwv207d_hwinfo_parse(mdev);

	mdev->visible_vram_size = min(hw->vram_size, mdev->ap_size);

	dev_info(mdev->dev, "running on:    %s", hw->is_pf ? "pf" : "vf");
	dev_info(mdev->dev, "3D clusters:   %d", hw->nr_3d_clusters);
	dev_info(mdev->dev, "2D clusters:   %d", hw->nr_2d_clusters);
	dev_info(mdev->dev, "FUS clusters:  %d", hw->nr_fus_clusters);
	dev_info(mdev->dev, "Decoders:      %d", hw->nr_pipe[0x1]);
	dev_info(mdev->dev, "Encoders:      %d", hw->nr_pipe[0x2]);
	dev_info(mdev->dev, "GART:          %s", hw->has_gart ? "on" : "off");
	dev_info(mdev->dev, "VRAM:          %lluM (%lluM visible)",
		 hw->vram_size >> 20, mdev->visible_vram_size >> 20);
	dev_info(&pdev->dev, "pgtable seg:   %lluMB",
			mdev->pgtable_segment_size >> 20);

	mwv207d_hwinfo_verify(mdev);

	if (mdev->hw.is_emulation) {
		dev_info(mdev->dev, "running on cmodel, force polling mode");
		mdev->isr_poll = 1;
	}

	mdev->ap_vaddr = devm_ioremap_wc(mdev->dev, mdev->ap_base,
					 mdev->visible_vram_size);
	if (!mdev->ap_vaddr) {
		dev_err(mdev->dev, "failed to mmap aperture vram");
		return -ENOMEM;
	}
	atomic64_add(mdev->pgtable_segment_size, &mdev->vram_used_bytes);
	atomic64_add(mdev->pgtable_segment_size, &mdev->visible_vram_used_bytes);

	if (mdev->hw.is_pf)
		wait_for(mdev_read(mdev, 0x34C120) &
			 0x4,
			 10 * 1000);

	if (mdev->hw.is_pf)
		mwv207d_display_blackout(mdev);

	ret = mwv207d_vbios_init(mdev);
	if (ret) {
		dev_err(mdev->dev, "failed to init vbios, %d", ret);
		return ret;
	}

	ret = mwv207d_irq_init(mdev);
	if (ret) {
		dev_err(mdev->dev, "failed to init irq, %d", ret);
		mwv207d_vbios_fini(mdev);
		return ret;

	}

	mwv207d_db_add(mdev, DRM_MWV207D_3D_CLUSTER_NR, hw->nr_3d_clusters);
	mwv207d_db_add(mdev, DRM_MWV207D_2D_CLUSTER_NR, hw->nr_2d_clusters);
	mwv207d_db_add(mdev, DRM_MWV207D_FUS_CLUSTER_NR, hw->nr_fus_clusters);
	mwv207d_db_add(mdev, DRM_MWV207D_VRAM_SIZE, hw->vram_size >> 20);
	mwv207d_db_add(mdev, DRM_MWV207D_GTT_SIZE, mdev->gtt_size >> 20);
	mwv207d_db_add(mdev, DRM_MWV207D_VISIBLE_VRAM_SIZE,
		       (mdev->visible_vram_size - mdev->pgtable_segment_size) >> 20);
	mwv207d_db_add(mdev, DRM_MWV207D_FAMILY, 0x1100);

	return 0;
}

void mwv207d_hw_fini(struct mwv207d_device *mdev)
{
	mwv207d_irq_fini(mdev);
	mwv207d_vbios_fini(mdev);
}

void mwv207d_hw_suspend(struct mwv207d_device *mdev)
{
}

void mwv207d_hw_resume(struct mwv207d_device *mdev)
{
	if (mdev->hw.is_pf)
		mwv207d_display_blackout(mdev);
}
