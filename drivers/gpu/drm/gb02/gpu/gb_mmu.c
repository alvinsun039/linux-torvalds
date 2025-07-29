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

#include <linux/atomic.h>
#include <linux/version.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
#include <linux/io-pgtable.h>
#endif
#include <linux/iommu.h>
#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/shmem_fs.h>
#include <linux/sizes.h>

#include "gb_device.h"
#include "gb_mmu.h"
#include "gb_gem.h"
#include "gb_regs.h"
#include "gb_stage.h"
#include "gb_pages_alloc.h"
#include "gb_gpu_irq.h"
#include "common/gb_common.h"

static int GB02FUNC911(struct GB02STR39 *gbdev, u32 as_nr)
{
	int ret;
	u32 val;

	ret = readl_relaxed_poll_timeout_atomic(gbdev->gpu_reg_base +
	 GB02MAC1585(as_nr), val, !(val & GB02MAC1620), 10, 10000);

	if (ret)
		dev_err(gbdev->dev, "%d AS_ACTIVE bit stuck\n", as_nr);

	return ret;
}

static int GB02FUNC914(struct GB02STR39 *gbdev, u32 as_nr, u32 cmd)
{
	int status;

	status = GB02FUNC911(gbdev, as_nr);
	if (!status)
		mmu_write(gbdev, GB02MAC1580(as_nr), cmd);
	else {
		gb_printf(KERN_ERR, "ERROR.......\n");
	}

	return status;
}

static void GB02FUNC916(struct GB02STR39 *gbdev, u32 as_nr,
			u64 iova, size_t size)
{
	u8 region_width;
	u64 region = iova & GB02MAC315;

	/* The size is encoded as ceil(log2) minus(1), which may be calculated
	 * with fls. The size must be clamped to hardware bounds.
	 */
	size = max_t(u64, size, GB02MAC311);
	region_width = fls64(size - 1) - 1;
	region |= region_width;

	mmu_write(gbdev, GB02MAC2813(as_nr), region & 0xFFFFFFFFUL);
	mmu_write(gbdev, GB02MAC2814(as_nr), (region >> 32) & 0xFFFFFFFFUL);
	GB02FUNC914(gbdev, as_nr, GB02MAC1714);
}


static int GB02FUNC918(struct GB02STR39 *gbdev, int as_nr,
				      u64 iova, size_t size, u32 op)
{
	if (as_nr < 0) {
		// dev_WARN(gbdev->dev, "as_nr is a invalid value\n");
		return 0;
	}

	if (op != GB02MAC1717)
		GB02FUNC916(gbdev, as_nr, iova, size);

	/* Run the MMU operation */
	GB02FUNC914(gbdev, as_nr, op);

	/* Wait for the flush to complete */
	return GB02FUNC911(gbdev, as_nr);
}

/* reference to gb_mmu_hw_do_operation */
int GB02FUNC921(struct GB02STR39 *gbdev, int as_nr,
			       u64 iova, size_t size, u32 op)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&gbdev->mmu_hw_lock, flags);
	ret = GB02FUNC918(gbdev, as_nr, iova, size, op);
	spin_unlock_irqrestore(&gbdev->mmu_hw_lock, flags);

	return ret;
}

int GB02FUNC923(struct GB02STR39 *gbdev,
			       u64 iova, size_t size, u32 op)
{
	int i, ret;

	for (i = 0; i < GB02MAC289; i++)
		ret = GB02FUNC921(gbdev, i, iova, size, op);

	return ret;
}

#ifdef GB02MAC286
static u64 GB02FUNC924(void)
{
	u64 memattr = 0;

	memattr = (GB02MAC1199 <<
		  (GB02MAC1204 * 8)) |
		  (GB02MAC1200    <<
		  (GB02MAC1205 * 8)) |
		  (GB02MAC1201           <<
		  (GB02MAC1206 * 8)) |
		  (GB02MAC1202   <<
		  (GB02MAC1207 * 8)) |
		  (GB02MAC1203  << (GB02MAC1208 * 8));

	return memattr;
}
#endif

static u64 GB02FUNC925(void)
{
	u64 transcfg;

#ifdef GB02MAC286
	transcfg = GB02MAC1216;
#else
	transcfg = GB02MAC1214;
#endif

	return transcfg;
}

static void GB02FUNC927(struct GB02STR39 *gbdev, int as_num)
{
	u32 irq_mask;
	unsigned long flags;

	spin_lock_irqsave(&gbdev->hw_irq_lock, flags);
	irq_mask = mmu_read(gbdev, GB02MAC1544);
	mmu_write(gbdev, GB02MAC1544, irq_mask | BIT(as_num) | BIT(as_num + 16));
	spin_unlock_irqrestore(&gbdev->hw_irq_lock, flags);
	gb_printf(KERN_INFO, "%s: enable %d MMU irq\n", __func__, as_num);
}

static void GB02FUNC929(struct GB02STR39 *gbdev, int as_num)
{
	u32 irq_mask;
	unsigned long flags;

	spin_lock_irqsave(&gbdev->hw_irq_lock, flags);
	irq_mask = mmu_read(gbdev, GB02MAC1544);
	mmu_write(gbdev, GB02MAC1544, irq_mask & ~(BIT(as_num) | BIT(as_num + 16)));
	spin_unlock_irqrestore(&gbdev->hw_irq_lock, flags);
	gb_printf(KERN_INFO, "%s: disable %d MMU irq\n", __func__, as_num);
}

void GB02FUNC930(struct GB02STR39 *gbdev, int as_nr)
{
	u64 transcfg = GB02FUNC925();
#ifdef GB02MAC286
	u64 transtab, memattr;
#endif

#ifdef GB02MAC286
	transtab = (u64)gbdev->mmu_mode->pgd & GB02MAC1221;
	memattr = GB02FUNC924();
	mmu_write(gbdev, GB02MAC2809(as_nr), transtab & 0xffffffffUL);
	mmu_write(gbdev, GB02MAC2810(as_nr), transtab >> 32);

	mmu_write(gbdev, GB02MAC2811(as_nr), memattr & 0xffffffffUL);
	mmu_write(gbdev, GB02MAC2812(as_nr), memattr >> 32);
#endif

	mmu_write(gbdev, GB02MAC2817(as_nr), transcfg & 0xffffffffUL);
	mmu_write(gbdev, GB02MAC2818(as_nr), transcfg >> 32);

	GB02FUNC927(gbdev, as_nr);

	GB02FUNC914(gbdev, as_nr, GB02MAC1711);
}

static void GB02FUNC932(struct GB02STR39 *gbdev, u32 as_nr)
{
	GB02FUNC929(gbdev, as_nr);
	GB02FUNC921(gbdev, as_nr, 0, ~0UL, GB02MAC2821);

	mmu_write(gbdev, GB02MAC2809(as_nr), 0);
	mmu_write(gbdev, GB02MAC2810(as_nr), 0);

	mmu_write(gbdev, GB02MAC2811(as_nr), 0);
	mmu_write(gbdev, GB02MAC2812(as_nr), 0);

	GB02FUNC914(gbdev, as_nr, GB02MAC1711);
}

void GB02FUNC933(struct GB02STR39 *gbdev, int enabled)
{
	int as;

	for (as = 0; as < GB02MAC289; as++) {
		if (enabled)
			GB02FUNC930(gbdev, as);
		else
			GB02FUNC932(gbdev, as);
	}
}

int GB02FUNC935(void)
{
	static int as_nr = 0;
	as_nr++;
	if (as_nr >= GB02MAC289)
		as_nr = 0;
	return as_nr;
}

void GB02FUNC937(struct GB02STR39 *gbdev)
{
	mmu_write(gbdev, GB02MAC1542, ~0);
}

void GB02FUNC938(struct GB02STR39 *gbdev, int as,
				     u64 iova, size_t size, bool sync)
{
	u32 op = 0;

	pm_runtime_get_noresume(gbdev->dev);

	/* Flush the PTs only if we're already awake */
	if (pm_runtime_active(gbdev->dev)) {
	}
	if (sync)
		op = GB02MAC2821;
	else
		op = GB02MAC1724;
	GB02FUNC921(gbdev, as, iova, size, op);

	pm_runtime_put_sync_autosuspend(gbdev->dev);
}

int GB02FUNC939(struct GB02STR39 *gbdev)
{
	struct GB02STR125 *p_pgd;
	struct GB02STR138 *mmu_mode = gbdev->mmu_mode;
	u64 *p_vaddr;
	int i;

	mmu_mode->mmu_teardown_pages = kmalloc(PAGE_SIZE * 4, GFP_KERNEL);
	if (NULL == mmu_mode->mmu_teardown_pages)
		return -ENOMEM;

	mutex_lock(&mmu_mode->mmu_lock);
	p_pgd = GB02FUNC1123(gbdev);
	if (!p_pgd) {
		mutex_unlock(&mmu_mode->mmu_lock);
		goto alloc_free;
	}
	mutex_unlock(&mmu_mode->mmu_lock);

	p_vaddr = GB02FUNC1107(p_pgd);
	if (NULL == p_vaddr) {
		goto alloc_free;
	}

	for (i = 0; i < GB02MAC1193; i++) {
		mmu_mode->GB02FUNC1009(&p_vaddr[i]);
	}

	mmu_mode->pgd = GB02FUNC1102(p_pgd);
	//gb_printf(KERN_ERR, "%s alloc pgd 0x%llx\n", __func__, mmu_mode->pgd);

	return 0;

alloc_free:
	gb_err(gbdev->dev, "%s alloc failed\n", __func__);
	if (p_pgd)
		GB02FUNC1130(p_pgd, gbdev);

	kfree(mmu_mode->mmu_teardown_pages);

	return -ENOMEM;
}

void GB02FUNC945(struct GB02STR39 *gbdev)
{
	struct GB02STR138 *mmu_mode;
	mmu_mode = gbdev->mmu_mode;

	pm_runtime_get_noresume(gbdev->dev);

	if (pm_runtime_active(gbdev->dev))
		mmu_mode->mmu_controll(gbdev, GB02MAC1275);

	pm_runtime_put_autosuspend(gbdev->dev);
	GB02FUNC757(gbdev);
	gb_printf(KERN_INFO, "%s x used_mmu_pages: %d\n", __func__, atomic_read(&gbdev->GB02STR35.used_mmu_pages));
}

struct GB02STR59 *GB02FUNC949(struct GB02STR39 *gbdev, u64 addr)
{
	int i;
	u64 offset;
	struct GB02STR59 *bo = NULL;
	struct GB02STR138 *mmu_mode;
	struct drm_mm_node *node;
	mmu_mode = gbdev->mmu_mode;
	offset = addr >> PAGE_SHIFT;

	mutex_lock(&mmu_mode->mm_lock);
	for (i = 0; i < GB_MAX_VA_MM; i++) {
		drm_mm_for_each_node(node, &mmu_mode->mm[i]) {
			if (offset >= node->start &&
		    	offset < (node->start + node->size)) {
				bo = GB02FUNC821(node);
				mutex_unlock(&mmu_mode->mm_lock);
				goto out;
			}
		}
	}
	mutex_unlock(&mmu_mode->mm_lock);
out:
	return bo;
}

static int GB02FUNC952(struct GB02STR39 *gbdev, int as, u64 addr)
{
	int i, ret = 0;
	unsigned long flags;
	struct GB02STR59 *bo;
	struct GB02STR47 *gb_priv;

	bo = GB02FUNC949(gbdev, addr);
	if (!bo) {
		gb_printf(KERN_ERR, "%s: APP in %d AS can't found bo by addr 0x%llx\n",
			__func__, as, addr);
		ret = -ENOENT;
		goto fail;
	}

	if (!bo->is_heap_growable) {
		dev_WARN(gbdev->dev, "matching BO 0x%llx is not heap type (GPU VA = %llx)",
				(long long )bo, bo->base.node.start << PAGE_SHIFT);
		gb_printf(KERN_ERR, "fault addr: 0x%llx bo start: 0x%llx nr_pages: 0x%x bo end: 0x%llx\n",
			  addr, bo->base.node.start<<PAGE_SHIFT, bo->base.nr_pages,
			  (bo->base.node.start << PAGE_SHIFT) + bo->base.nr_pages * PAGE_SIZE);
		ret = -EINVAL;
		goto fail;
	}
#if 0
	list_for_each_entry(gb_heap_bo, &bo->gb_base.ttm_bo_root, ttm_bo_list) {
		if (addr >= gb_heap_bo->gb_va_start && addr <
			gb_heap_bo->gb_va_start + gb_heap_bo->grow_nr_pages * GB02MAC311) {
			printk("[%s] found heapbo already mappped addr 0x%llx as:%d\n", __func__, addr, as);
			return ret;
		}
	}
#endif

	gb_priv = bo->private_data;
	ret = GB02FUNC1513(bo, gbdev, gb_priv, addr, 1, as);
	return ret;

fail:
	for (i = 0; i < GB02MAC288; i++) {
		spin_lock_irqsave(&gbdev->ss->stages_lock, flags);
		if (gbdev->stages[i] && (gbdev->stages[i]->as == as)) {
			/* TODO for fault job*/
			gbdev->stages[i]->reset_time = GB02MAC1958 + 1;
			dma_fence_signal_locked(gbdev->stages[i]->irq_done_fence);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 131)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#if (defined(GB_KYLIN_SERV_TERCEL)) && (LINUX_VERSION_CODE == KERNEL_VERSION(4, 19, 90))
			mod_delayed_work(system_wq,
					&gbdev->ss->queue[i].sched.work_tdr, 0);
#else
			mod_delayed_work(system_wq,
					&gbdev->stages[i]->base.work_tdr, 0);
#endif
#else
			mod_delayed_work(system_wq,
					&gbdev->ss->queue[i].sched.work_tdr, 0);
#endif
			gbdev->stages[i] = NULL;
		}
		spin_unlock_irqrestore(&gbdev->ss->stages_lock, flags);
	}

	return ret;
}

static const char *GB02FUNC957(struct GB02STR39 *gbdev,
		u32 fault_status)
{
	switch (fault_status & GB02MAC1644) {
	case GB02MAC1646:
		return "ATOMIC";
	case GB02MAC1650:
		return "READ";
	case GB02MAC1652:
		return "WRITE";
	case GB02MAC1648:
		return "EXECUTE";
	default:
		WARN_ON(1);
		return NULL;
	}
}

irqreturn_t GB02FUNC959(int irq, void *data)
{
	struct GB02STR39 *gbdev = data;

	if (!reg_read(gbdev, GB02MAC1546))
		return IRQ_NONE;

	mmu_write(gbdev, GB02MAC1544, 0);
	return IRQ_WAKE_THREAD;
}

irqreturn_t GB02FUNC961(int irq, void *data)
{
	struct GB02STR39 *gbdev = data;
	u32 status = reg_read(gbdev, GB02MAC1540);
	int i, ret;

	/*TODO save IRQ_MASK value for restore instead of ALL slot*/
	while (status) {
		for (i = 0; status && i < GB02MAC289; i++) {
			u32 mask = BIT(i) | BIT(i + 16);
				u64 addr;
			u32 fault_status;
			u32 exception_type;
			u32 access_type;
			u32 source_id;

			if (!(status & mask))
				continue;

			fault_status = mmu_read(gbdev, GB02MAC1582(i));
			addr = mmu_read(gbdev, GB02MAC2815(i));
			addr |= (u64)mmu_read(gbdev, GB02MAC2816(i)) << 32;
			gb_printf(KERN_INFO, "IRQ mmu fault_status: 0x%x, addr: 0x%llx status 0x%x\n", fault_status, addr, status);

			exception_type = fault_status & 0xFF;
			access_type = (fault_status >> 8) & 0x3;
			source_id = (fault_status >> 16);

		/* Page fault only */
			if ((status & mask) == BIT(i)) {
				WARN_ON(exception_type < 0xC1 || exception_type > 0xC4);
				ret = GB02FUNC952(gbdev, i, addr);
				if (!ret) {
					status &= ~mask;
					continue;
				}
			}

			dev_err(gbdev->dev,
				"Unhandled Page fault in AS%d at VA 0x%016llX\n"
				"raw fault status: 0x%X\n"
				"decoded fault status: %s\n"
				"exception type 0x%X: %s\n"
				"access type 0x%X: %s\n"
				"source id 0x%X\n",
				i, addr,
				fault_status,
				(fault_status & (1 << 10) ? "DECODER FAULT" : "SLAVE FAULT"),
				exception_type, GB02FUNC130(gbdev, exception_type),
				access_type, GB02FUNC957(gbdev, fault_status),
				source_id);

			mmu_write(gbdev, GB02MAC1542, mask);
			status &= ~mask;
		}
		status = reg_read(gbdev, GB02MAC1540);
	}
	mmu_write(gbdev, GB02MAC1542, (0xFF << 16 || 0xFF));
	mmu_write(gbdev, GB02MAC1544, ~0);
	return IRQ_HANDLED;
};

void GB02FUNC968(struct GB02STR39 *gbdev)
{
	struct GB02STR125 *p;
	struct GB02STR138 *mmu_mode = gbdev->mmu_mode;

	if (mmu_mode->mmu_teardown_pages) {
		kfree(mmu_mode->mmu_teardown_pages);
		mmu_mode->mmu_teardown_pages = NULL;
	}

	p = GB02FUNC1104(gbdev, mmu_mode->pgd);

	if (!p) {
		gb_err(gbdev->dev, "Error: %s GB02STR125 is NULL\n", __func__);
		return;
	}
	GB02FUNC1130(p, gbdev);

}

void GB02FUNC972(struct GB02STR39 *gbdev)
{
	unsigned long flags;
	GB02FUNC945(gbdev);
	GB02FUNC968(gbdev);

	spin_lock_irqsave(&gbdev->hw_irq_lock, flags);
	mmu_write(gbdev, GB02MAC1544, 0);
	spin_unlock_irqrestore(&gbdev->hw_irq_lock, flags);

	drm_mm_takedown(&gbdev->mmu_mode->mm[GB_LOW_VA_MM]);
	drm_mm_takedown(&gbdev->mmu_mode->mm[GB_HIGH_VA_MM]);
}

int GB02FUNC974(struct GB02STR39 *gbdev)
{
	int ret;
	gbdev->mmu_mode = GB02FUNC1010();
	gbdev->mmu_mode->gbdev = gbdev;
	mutex_init(&gbdev->mmu_mode->mm_lock);
	mutex_init(&gbdev->mmu_mode->mmu_lock);

	drm_mm_init(&gbdev->mmu_mode->mm[GB_LOW_VA_MM], GB02MAC1227 >> PAGE_SHIFT,
		    GB02MAC1229 >> PAGE_SHIFT);
	drm_mm_init(&gbdev->mmu_mode->mm[GB_HIGH_VA_MM], GB02MAC1231 >> PAGE_SHIFT,
	            GB02MAC1233 >> PAGE_SHIFT);
	//pr_err("[%s] start init\n", __func__);

	ret = GB02FUNC939(gbdev);
	if (ret) {
		goto pgd_init_fail;
	}

	gbdev->mmu_mode->mmu_controll(gbdev, GB02MAC1274);
	return ret;

pgd_init_fail:
	drm_mm_takedown(&gbdev->mmu_mode->mm[GB_LOW_VA_MM]);
	drm_mm_takedown(&gbdev->mmu_mode->mm[GB_HIGH_VA_MM]);
	return ret;
}
