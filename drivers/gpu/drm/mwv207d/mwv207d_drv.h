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
#ifndef MWV207D_DRV_H_VTIQLF2Y
#define MWV207D_DRV_H_VTIQLF2Y

#include <linux/pci.h>
#include <linux/mutex.h>
#include <linux/idr.h>
#include <linux/dmapool.h>
#include <linux/delay.h>
#include <drm/drm_file.h>
#include <drm/drm_device.h>
#include <drm/ttm/ttm_bo.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_framebuffer.h>
#include "mwv207d_drm.h"

#define DRIVER_NAME                  "mwv207d"
#define DRIVER_DESC                  "mwv207d drm driver"
#define DRIVER_DATE                  "20250807"
#define DRIVER_MAJOR                 1
#define DRIVER_MINOR                 0
#define DRIVER_PATCHLEVEL            3

struct mwv207d_db;
struct mwv207d_vm;
struct mwv207d_pta;
struct mwv207d_vbios;

struct mwv207d_fbdev {
	struct drm_fb_helper helper;
	struct drm_framebuffer fb;
	struct mwv207d_bo *mbo;
};

struct mwv207d_hwinfo {
	bool is_pf;
	bool is_emulation;
	bool has_gart;
	bool remap_codec;
	int vf_sel;
	int irq_mode;
	u64 vram_size;
	u64 vram_offset;
	int nr_3d_clusters;
	int nr_2d_clusters;
	int nr_fus_clusters;
	int nr_pipe[0x6];
	int nr_vconnectors;
	u32 vconn_max_width;
	u32 vconn_max_height;
};

struct mwv207d_irq;
struct mwv207d_monitor;
struct mwv207d_vdisplay;

#define drm_to_mdev(dev) container_of(dev, struct mwv207d_device, base)
#define bdev_to_mdev(dev) container_of(dev, struct mwv207d_device, bdev)
struct mwv207d_device {
	struct drm_device base;
	struct ttm_device bdev;
	struct mwv207d_vdisplay *vdisplay;
	struct mwv207d_audio_card *mcard;

	void __iomem *mmio;

	u64 ap_base;
	u64 ap_size;
	void __iomem *ap_vaddr;
	u64 gtt_size;
	u64 visible_vram_size;

	unsigned long *pt_bitmap;
	u32            pt_bits;
	spinlock_t     bitmap_lock;

	struct device *dev;
	struct mwv207d_fbdev *fbdev;

	struct mwv207d_hwinfo hw;

	struct drm_gpu_scheduler
		*sched[0x6][0x8];
	int nr_pipe[0x6];

	struct drm_sched_entity *dma_entity;
	struct drm_sched_entity *blt_2d_entity;
	struct drm_sched_entity *blt_3d_entity;
	struct mutex qlock;

	struct mwv207d_db *db;

	struct mutex gpio_lock;

	struct mwv207d_vm *vm;
	struct mwv207d_pta *pta;
	void *pgtable_segment;
	u64 pgtable_segment_size;

	struct mwv207d_irq *irq;

	u64 vram_pinned_bytes;
	u64 gtt_pinned_bytes;

	atomic64_t moved_bytes;
	atomic64_t vram_used_bytes;
	atomic64_t gtt_used_bytes;
	atomic64_t visible_vram_used_bytes;

	bool isr_poll;
	bool renderonly;

	struct mwv207d_vbios *vbios;
	struct mwv207d_monitor *monitor;

	struct dentry *debugfs_root;

	struct mwv207d_gart *gart;

	bool skip_thaw_kms;
};

struct mwv207d_ctx_mgr {
	struct mutex lock;
	struct idr handle_table;
};

struct mwv207d_fpriv {
	struct mwv207d_ctx_mgr ctx_mgr;
	struct mwv207d_vm *vm;
};

int mwv207d_test(struct mwv207d_device *mdev);

static inline void mdev_write(struct mwv207d_device *mdev, u32 reg, u32 value)
{

	if (!mdev->hw.is_pf && reg >= SZ_2M) {
		dump_stack();
		return;
	}
	writel_relaxed(value, mdev->mmio + reg);
}

static inline u32 mdev_read(struct mwv207d_device *mdev, u32 reg)
{

	if (!mdev->hw.is_pf && reg >= SZ_2M) {
		dump_stack();
		return 0xffffffff;
	}
	return readl_relaxed(mdev->mmio + reg);
}

static inline void mdev_modify(struct mwv207d_device *mdev, u32 reg,
			       u32 mask, u32 value)
{
	u32 rvalue = mdev_read(mdev, reg);

	rvalue = (rvalue & ~mask) | (value & mask);
	mdev_write(mdev, reg, rvalue);
}

#define to_fpriv(drm_file)  ((drm_file)->driver_priv)

#define wait_for(COND, MS) ({                                           \
	unsigned long timeout__ = jiffies + msecs_to_jiffies(MS) + 1;   \
	int ret__ = 0;                                                  \
	while (!(COND)) {                                               \
		if (time_after(jiffies, timeout__)) {                   \
			if (!(COND))                                    \
				ret__ = -ETIMEDOUT;                     \
			break;                                          \
		}                                                       \
		msleep(1);                                              \
	}                                                               \
	ret__;                                                          \
})

int mwv207d_hw_init(struct mwv207d_device *mdev);
void mwv207d_hw_fini(struct mwv207d_device *mdev);
int mwv207d_hw_vf_reset_3d(struct mwv207d_device *mdev, unsigned long timeout);
void mwv207d_hw_suspend(struct mwv207d_device *mdev);
void mwv207d_hw_resume(struct mwv207d_device *mdev);

#endif
