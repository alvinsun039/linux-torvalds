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

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_fb_cma_helper.h>
#endif
#ifdef CONFIG_SKYLIN_OS_V10
#include <asm/machine_t.h>
#endif
#include "common/gb_uk.h"
#include "common/gb_common.h"
#include "gb_device.h"
#include "gb_gem.h"
#include "gb_mmu.h"
#include "gb_pages_alloc.h"
#include "gb_regs.h"
#include "kms/gbdc_mm.h"
#include "gb_ttm.h"
#include "vpu/vpu_mm/gb_vpu_gem.h"
#include "kms/gb_kms.h"
#include "gpu_test/gb_mem_tool.h"
#include "gb_osl.h"

extern bool gb_rb_tree;
extern int gb_pg_dump_state;
extern bool gb_mmu_mmap;
static void GB02FUNC713(struct GB02STR59 *bo);
void GB02FUNC688(struct drm_gem_object *obj)
{
	struct GB02STR59 *gb_bo = NULL;
	struct GB02STR50 *ttm_bo = NULL;
	struct rb_root *rb_root;
#if 0
	//free bo and clean vram
	struct GB02STR70 *gb_pcie = NULL;
	int bar_id;
	void __iomem *io_remap_addr;

	gb_pcie = GB02FUNC518();
	bar_id = GB02FUNC468(gb_pcie->GB02STR153);

	io_remap_addr = (void __iomem *)((u64)gb_pcie->pci_bars[bar_id].mmio
						+ ttm_bo->bo.offset);
	memset_io(io_remap_addr,
		0, (ttm_bo->bo.mem.num_pages * PAGE_SIZE));
#endif

	ttm_bo = gem_ob_to_ttm_bo(obj);
	if (ttm_bo->sub_bo_flag) {
		GB02FUNC1508(ttm_bo);
		return;
	}
	gb_bo = GB02FUNC212(obj);

	if (!gb_bo) {
		gb_printf(KERN_ERR, "%s: get gb_bo is null %llx\n",
			__func__, (long long int)gb_bo);
		return;
	}

	if (!gb_bo->is_ttm_bo) {
		gb_printf(KERN_ERR, "%s: free bit map gem object %llx\n",
			__func__, (long long int)gb_bo);
		kvfree(gb_bo);
		return;
	}
	gb_printf(KERN_DEBUG, "%s bo:0x%llx\n",
			__func__, (long long)&gb_bo->gb_base.ttm_bo.bo);

	/*
	 *  Every task has 2 rb_tree, cpu_va and gpu_va.
	 *
	 *  Bo can share between tasks and drm_ioctl_gem_close can call
	 *  gb_gem_destroy or just refcount sub. This means not all gem
	 *  close actually call GB02FUNC274 to destory rb_trees.
	 *
	 *  When user task want to close gem, make sure bo node are deleted
	 *  from both cpu_va and gpu_va tree, otherwise this could cause
	 *  use after free bug.
	 *
	 *  For example, task 1 alloc bo1 shared with task 2, rb_node of
	 *  bo1 are inserted into both cpu_va and gpu_va tree for both tasks.
	 *  Then task 2 call gem close, GB02FUNC1508 found refcount is 3,
	 *  then refcount-- and return, bo still exist on task 2 rb_trees.
	 *  Even task 1 call gb_gem_destory delete task 1 rb_node of bo1 and
	 *  kfree bo1 task 2 rb_tree still could visit bo1 address, bang,
	 *  use after free.
	 */
	if (gb_rb_tree) {
		mutex_lock(&gb_bo->gbdev->rb_mutex);
		rb_root = GB02FUNC209(gb_bo->private_data, GB02MAC1130);
		if (rb_root)
			GB02FUNC271(rb_root, gb_bo->base.iovaddr, GB02MAC1130);

		rb_root = GB02FUNC209(gb_bo->private_data, GB02MAC1131);
		if (rb_root)
			GB02FUNC271(rb_root, gb_bo->vm_info.vm_start, GB02MAC1131);
		mutex_unlock(&gb_bo->gbdev->rb_mutex);
	}
	GB02FUNC713(gb_bo);
	GB02FUNC1508(&gb_bo->gb_base.ttm_bo);
}

static void GB02FUNC694(struct kref *kref)
{
	unsigned long long vpfn;
	size_t nr_pages;
	struct GB02STR59 *bo = NULL;
	struct GB02STR57 *bo_base = NULL;
	struct GB02STR47 *priv = NULL;
	struct GB02STR39 *gbdev = NULL;
	struct GB02STR138 *mmu_mode = NULL;

	bo_base = container_of(kref, struct GB02STR57, refcount);
	bo = bo_base->bo;
	gbdev = bo->gbdev;
	mmu_mode = gbdev->mmu_mode;

	if (!bo) {
		gb_printf(KERN_ERR, "%s bo pointer is null \n", __func__);
		return;
	}

	if (bo->is_ttm_bo) {
		gb_printf(KERN_DEBUG, "%s %d: bo is ttm bo,unrserve\n", __func__, __LINE__);
		GB02FUNC274(bo, priv);
		if (bo->domain == GB02MAC2678 && !(bo->is_fb_bo) &&
			(bo->vbo_save_flag_old & GB02MAC955)) {
			if (bo->base.pages)
				kvfree(bo->base.pages);
			bo->is_mapped = false;
			return;
		}
#ifndef GB02MAC286
		return;
#endif
	}
	/*Release drm_gem_object and drm_mm*/

	if (bo->is_mapped && !bo->is_heap_growable) {
		vpfn = bo->base.node.start * GB02MAC317;
		nr_pages = bo->base.nr_pages;
#ifndef GB02MAC426
		GB02FUNC1133(gbdev, bo->base.nr_pages, bo->base.pages);
#endif
		GB02FUNC774(mmu_mode, vpfn, nr_pages);

		if (bo->base.pages)
			kvfree(bo->base.pages);
	} else if (bo->is_mapped && bo->is_heap_growable) {
		struct gb_heap_ttm_bo *gb_heap_bo, *tmp;
		list_for_each_entry_safe(gb_heap_bo, tmp, &bo->gb_base.ttm_bo_root, ttm_bo_list) {
			list_del_init(&gb_heap_bo->ttm_bo_list);
			vpfn = gb_heap_bo->gb_va_start >> GB02MAC313;
			nr_pages = gb_heap_bo->grow_nr_pages;
			GB02FUNC774(mmu_mode, vpfn, nr_pages);
			if (gb_heap_bo->pages)
				kvfree(gb_heap_bo->pages);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
			GB02FUNC688(&gb_heap_bo->ttm_bo.bo.base);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
			GB02FUNC688(&gb_heap_bo->ttm_bo.bo.base);
#else
			GB02FUNC688(&gb_heap_bo->ttm_bo.base);
#endif
		}

		if (bo->base.pages)
			kvfree(bo->base.pages);
		} else {
			if (bo->base.pages)
				kvfree(bo->base.pages);
		}

	mutex_lock(&mmu_mode->mm_lock);
	if (drm_mm_node_allocated(&bo_base->node))
		drm_mm_remove_node(&bo_base->node);
	mutex_unlock(&mmu_mode->mm_lock);
	bo->is_mapped = false;
	//gb_printf(KERN_INFO, "%s: bo %pK handle_count %d\n", __func__, bo, kref_read(&obj->refcount)); TODO dante
#ifdef GB_ALLSCREEN
	if (1 == bo->is_fb_bo) {
		struct GB02STR247 *fb_base_info = GB02FUNC161();
		//atomic_set(&fb_base_info->info[NEW_FB].flag, 0);
		memset(&fb_base_info->info[DC_FB], 0, sizeof(struct GB02STR246) * 3);
		GB02FUNC164(bo->gbdev->gbdc_dev, fb_base_info, DC_FB, fb_base_info->expend_flag);
	}
#endif
}

static void GB02FUNC712(struct GB02STR59 *bo)
{
	kref_get(&bo->base.refcount);
}

static void GB02FUNC713(struct GB02STR59 *bo)
{
	if (!bo->is_mapped)
		return;
	kref_put(&bo->base.refcount, GB02FUNC694);
}

/* The func is called by each GB02FUNC288 throgh drm_gem_handle_create */
int GB02FUNC715(struct drm_gem_object *obj, struct drm_file *file_priv)
{
	int ret = 0;
	size_t size = obj->size;
	/* the unit of node is page, so we don't need to align it again */
	u64 align = 0;
	struct GB02STR59 *bo = GB02FUNC212(obj);
	struct GB02STR57 *gbmem;
	struct drm_mm *va_mm;
	unsigned long color = bo->noexec ? GB02MAC924 : 0;
	struct GB02STR47 *priv = file_priv->driver_priv;
	struct GB02STR39 *gbdev = priv->gbdev;
	struct GB02STR138 *mmu_mode = gbdev->mmu_mode;
	//struct GB02STR39 *gbdev = priv->gbdev;
	u64 vpfn, nr_pages;
	phys_addr_t *pages;

	if (!bo) {
		gb_printf(KERN_ERR, "%s bo pointer is null \n", __func__);
		return -EINVAL;
	}

	if (bo->is_kms_bo)
		return 0;

	if (bo->is_mapped) {
		gb_printf(KERN_INFO, "[%s]bo 0x%llx bo start: 0x%llx, pa 0x%llx, size %llx is low va %d\n",
			__func__, (u64)bo, bo->base.node.start, bo->base.pages[0],
			bo->base.node.size, bo->is_low_va);
		GB02FUNC712(bo);
		return 0;
	}
	kref_init(&bo->base.refcount);

	if (bo->is_ttm_bo) {
		gb_printf(KERN_DEBUG, "%s %d: bo is ttm bo,do not to reserve %d\n",
				__func__, __LINE__, ret);
		if (bo->domain == GB02MAC2678 && !(bo->is_fb_bo) &&
			(bo->vbo_save_flag_old & GB02MAC955)) {
			if (!bo->is_heap_growable)
				bo->base.iopaddr = bo->base.pages[0];
			bo->is_mapped = true;
			return 0;
		}
#ifndef GB02MAC286
		gb_printf(KERN_INFO, "%s %d: if for mmu enabled return\n",
				__func__, __LINE__);
		return 0;
#endif
	}

	va_mm = bo->is_low_va ? &mmu_mode->mm[GB_LOW_VA_MM] : &mmu_mode->mm[GB_HIGH_VA_MM];
	mutex_lock(&mmu_mode->mm_lock);
	ret = drm_mm_insert_node_generic(va_mm, &bo->base.node,
					 size >> PAGE_SHIFT, align, color, 0);
	mutex_unlock(&mmu_mode->mm_lock);

	gb_printf(KERN_DEBUG, "%s mm 0x%llx bo 0x%llx bo start: 0x%llx, pa 0x%llx, size %llx is low va %d\n",
			__func__, (u64)va_mm, (u64)bo, bo->base.node.start, bo->base.pages[0],
			bo->base.node.size, bo->is_low_va);

	if (ret) {
		gb_printf(KERN_ERR, "%s Error, insert node error, ret:%d\n", __func__, ret);
		return ret;
	}

	bo->base.iovaddr = bo->base.node.start << PAGE_SHIFT;
	gbmem = &bo->base;
	if (!gbmem) {
		gb_printf(KERN_ERR, "%s gbmem pointer is null \n", __func__);
		return -EINVAL;
	}

	pages = gbmem->pages;
	nr_pages = gbmem->nr_pages;
	if (!bo->is_heap_growable) {
		vpfn = bo->base.node.start * GB02MAC317;
		ret = GB02FUNC767(mmu_mode, vpfn, pages, nr_pages, 0,
			             gb_pg_dump_state & GB02MAC1236);
		if (ret) {
			gb_printf(KERN_ERR, "%s GB02FUNC767 faild, ret:%d\n", __func__, ret);
			mutex_lock(&mmu_mode->mm_lock);
			drm_mm_remove_node(&bo->base.node);
			mutex_unlock(&mmu_mode->mm_lock);
			goto out;
		}

		ret = GB02FUNC923(priv->gbdev,
				vpfn << GB02MAC313, nr_pages*GB02MAC311,
				GB02MAC1724);
		if (ret) {
			gb_printf(KERN_ERR, "%s mmu do operation faild, ret:%d\n", __func__, ret);
			goto out;
		}

		bo->base.iopaddr = bo->base.pages[0];
	} else {
		gb_printf(KERN_INFO, "%s HIT GROW HEAP, bo:0x%llx \n",
			__func__, (u64)bo);
	}
	bo->is_mapped = true;
out:
	if (ret) {
		//pr_err("[%s][%s] bo 0x%llx put close refcount %d bo count %d \n",current->comm, __func__, (u64)bo, kref_read(&bo->base.refcount), kref_read(&bo->gb_base.ttm_bo.bo.base.refcount));
		GB02FUNC713(bo);
	}
	return ret;
}

void GB02FUNC723(struct drm_gem_object *obj, struct drm_file *file_priv)
{
	struct GB02STR59 *bo = GB02FUNC212(obj);
	if (bo->is_kms_bo)
		return;
	GB02FUNC713(bo);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) 
const struct drm_gem_object_funcs gb_gem_object_funcs = {
	.free	= GB02FUNC688,
	.open	= GB02FUNC715,
	.close	= GB02FUNC723,
	.export = GB02FUNC1060,
};
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
const struct drm_gem_object_funcs gb_gem_object_funcs = {
	.free	= GB02FUNC688,
	.open	= GB02FUNC715,
	.close	= GB02FUNC723,
	.export = drm_gem_prime_export,
};
#endif

#if KERNEL_VERSION(5, 3, 0) > LINUX_VERSION_CODE
struct GB02STR59 *drm_ttm_bo_to_gb_gem(struct ttm_buffer_object *tbo)
{
	struct GB02STR50 *gb_tbo;
	struct GB02STR59 *bo;

	if (!tbo) {
		gb_printf(KERN_ERR, "%s ttm_bo is null \n", __func__);
		return NULL;
	}

	gb_tbo = ttm_ob_to_ttm_bo(tbo);
	if (!gb_tbo) {
		gb_printf(KERN_ERR, "%s gb_ttm_bo is null \n", __func__);
		return NULL;
	}

	bo = GB02FUNC205(gb_tbo);
	if (!bo) {
		gb_printf(KERN_ERR, "%s gb_gem_bo is null \n", __func__);
		return NULL;
	}
	return bo;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
struct reservation_object *gb_gem_prime_res_obj(struct drm_gem_object *obj)
{
	struct GB02STR50 *ttm_bo = gem_ob_to_ttm_bo(obj);
	return ttm_bo->bo.resv;
}
#endif

int GB02FUNC728(struct file *filp, struct vm_area_struct *vma)
{
#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	struct drm_gem_object *gem_obj;
#else
	struct ttm_buffer_object *gem_obj;
#endif
	struct GB02STR57 *gbmem_obj;
	struct GB02STR59 *bo;
	int ret = 0;
	struct GB02STR39 *gbdev;
	phys_addr_t *page_array;
	int nr_pages, i;
	unsigned long addr = vma->vm_start;
	int ddr_bar_id;
	struct GB02STR47 *gb_priv;
	u64 ddr_bar_base;
	u64 pfn;

	ret = drm_gem_mmap(filp, vma);
	if (ret) {
		gb_printf(KERN_DEBUG, "====%s %d end, ttm_bo, vm_pgoff:0x%lx\n",
			__func__, __LINE__, vma->vm_pgoff);
		ret = GB02FUNC1516(filp, vma);
		if (ret) {
			gb_printf(KERN_ERR, "%s ttm_mmap faild ret:%d\n", __func__, ret);
			return ret;
		}
	}

	gem_obj = vma->vm_private_data;
	if (gem_obj == NULL) {
		gb_printf(KERN_ERR, "%s gem_obj is null vma: 0x%lx 0x%lx\n",
			 __func__, vma->vm_start, vma->vm_end);
		return -EINVAL;
	}
	gb_printf(KERN_DEBUG, "%s get vma: 0x%lx 0x%lx\n",
			__func__, vma->vm_start, vma->vm_end);

#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	bo = GB02FUNC212(gem_obj);
#else
	bo = drm_ttm_bo_to_gb_gem(gem_obj);
#endif
	if (bo->is_kms_bo)
		return 0;

	if (gb_rb_tree) {
		mutex_lock(&bo->gbdev->rb_mutex);
		ret = GB02FUNC259(bo, vma);
		if (ret)
			gb_printf(KERN_ERR, "%s bo rb mmap faild :%d\n", __func__, ret);
		mutex_unlock(&bo->gbdev->rb_mutex);
	}

	if (bo->is_ttm_bo)
		return 0;

	mutex_lock(&bo->base.pages_lock);
	gbdev = bo->gbdev;
	ddr_bar_id = GB02FUNC468(gbdev->gb_pcie->GB02STR153);

	ddr_bar_base = gbdev->gb_pcie->pci_bars[ddr_bar_id].base;

	gb_priv = bo->private_data;

	gbmem_obj = &bo->base;
	nr_pages = (vma->vm_end - vma->vm_start) / PAGE_SIZE;

	gb_printf(KERN_INFO, "inside %s nr_pages: %d %d\n",
		__func__, nr_pages, gbmem_obj->nr_pages);
	if ((nr_pages == 0) || gbmem_obj->nr_pages == 0) {
		gb_printf(KERN_ERR, "inside %s Warning, nr_pages is 0!!!\n", __func__);
		mutex_unlock(&bo->base.pages_lock);
		return -EINVAL;
	}

#if defined(CONFIG_MIPS) || defined(CONFIG_LOONGARCH)
	vma->vm_page_prot = pgprot_device(vma->vm_page_prot);
#else
#ifdef CONFIG_SKYLIN_OS_V10
	if (is_ft1500a())
		vma->vm_page_prot = pgprot_device(vma->vm_page_prot);
	else
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#else
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#endif
#endif
	page_array = gbmem_obj->pages;
	for (i = 0; i < nr_pages; i++) {
		pfn = PFN_DOWN(ddr_bar_base + page_array[i]);
		ret = remap_pfn_range(vma, addr, pfn, PAGE_SIZE, vma->vm_page_prot);
		addr += PAGE_SIZE;
	}

	gb_printf(KERN_INFO, "start to do gpu mmaping... %s gpu_phy:0x%llx\n",
			__func__, bo->base.pages[0]);

	if (ret) {
		gb_printf(KERN_ERR, "Error %s\n", __func__);
		drm_gem_vm_close(vma);
	}
	mutex_unlock(&bo->base.pages_lock);

	return ret;
}

static phys_addr_t GB02FUNC744(struct GB02STR138 *mmu_mode)
{
	u64 *page;
	int i;
	struct GB02STR125 *p;

	p = GB02FUNC1123(mmu_mode->gbdev);
	if (!p)
		goto alloc_free;

	page = GB02FUNC1107(p);
	if (NULL == page)
		goto alloc_free;

	for (i = 0; i < GB02MAC1193; i++)
		mmu_mode->GB02FUNC1009(&page[i]);

	return GB02FUNC1102(p);

alloc_free:
	gb_err(mmu_mode->gbdev->dev, "%s alloc failed\n", __func__);

	return 0;
}

/* This is a debug feature only */
static void GB02FUNC748(struct GB02STR138 *mmu_mode, phys_addr_t pgd)
{
	u64 *page;
	struct GB02STR125 *p;
	int i;

	p = GB02FUNC1104(mmu_mode->gbdev, pgd);
	if (!p) {
		gb_err(mmu_mode->gbdev->dev, "Error: %s GB02STR125 is NULL\n", __func__);
		return;
	}

	page = GB02FUNC1107(p);
	if (page == NULL) {
		gb_err(mmu_mode->gbdev->dev, "Error: page is NULL!\n");

		return;
	}

	for (i = 0; i < GB02MAC1193; i++) {
		if (mmu_mode->GB02FUNC998(page[i]))
			gb_printf(KERN_ERR, "error libe pte 0x%llx", page[i]);
	}
}

static void GB02FUNC750(struct GB02STR138 *mmu_mode, phys_addr_t pgd, int level, int zap, u64 *pgd_page_buffer)
{
	phys_addr_t target_pgd;
	u64 *pgd_page = NULL;
	struct GB02STR125 *p = NULL;
	void *p_ptr = NULL;
	int i;

	lockdep_assert_held(&mmu_mode->mmu_lock);

	p = GB02FUNC1104(mmu_mode->gbdev, pgd);
	if (!p) {
		gb_err(mmu_mode->gbdev->dev, "Error: %s GB02STR125 is NULL\n", __func__);
		return;
	}

	pgd_page = GB02FUNC1107(p);
	// GB_DEBUG_ASSERT(NULL != pgd_page);
	if (pgd_page == NULL) {
		gb_err(mmu_mode->gbdev->dev, "Error: %s pgd_page is NULL\n", __func__);
		return;
	}

	/* Copy the page to our preallocated buffer so that we can minimize kmap_atomic usage */
	/* Don't replace PAGE_SIZE using GB02MAC311 */
	memcpy_fromio(pgd_page_buffer, pgd_page, GB02MAC311);
	pgd_page = pgd_page_buffer;

	for (i = 0; i < GB02MAC1193; i++) {
		target_pgd = mmu_mode->GB02FUNC995(pgd_page[i]);

		if (target_pgd) {
			if (level < (GB02MAC1195 - 1)) {
				/* Don't replace PAGE_SIZE using GB02MAC311 */
				GB02FUNC750(mmu_mode, target_pgd, level + 1, zap, pgd_page_buffer + (GB02MAC311 / sizeof(u64)));
			} else {
				/*
				 * So target_pte is a level-3 page.
				 * As a leaf, it is safe to free it.
				 * Unless we have live pages attached to it!
				 */
				GB02FUNC748(mmu_mode, target_pgd);
			}

			if (zap) {
				p = GB02FUNC1104(mmu_mode->gbdev, target_pgd);
				if (!p) {
					gb_err(mmu_mode->gbdev->dev, "Error: %s zap GB02STR125 is NULL\n", __func__);
					return;
				}

				p_ptr = GB02FUNC1107(p);
				if (p_ptr) {
					//memset_io(p_ptr, 0, GB02MAC311);
				} else {
					gb_err(mmu_mode->gbdev->dev, "Error: bad mmu page table\n");
				}

				GB02FUNC1130(p, mmu_mode->gbdev);
			}
		}
	}
}

void GB02FUNC757(struct GB02STR39 *gbdev)
{
	struct GB02STR138 *mmu_mode = gbdev->mmu_mode;

	gb_printf(KERN_INFO, "%s 1 used_mmu_pages: %d\n", __func__,
		atomic_read(&gbdev->GB02STR35.used_mmu_pages));

	mutex_lock(&mmu_mode->mmu_lock);
	GB02FUNC750(mmu_mode, mmu_mode->pgd, GB02MAC1194, 1, mmu_mode->mmu_teardown_pages);
	mutex_unlock(&mmu_mode->mmu_lock);
	gb_printf(KERN_INFO, "%s 2 used_mmu_pages: %d\n", __func__,
		atomic_read(&mmu_mode->gbdev->GB02STR35.used_mmu_pages));
}

static int GB02FUNC758(struct GB02STR138 *mmu_mode,
		phys_addr_t *pgd, u64 vpfn, int level, int dump)
{
	u64 *page;
	phys_addr_t target_pgd;
	struct GB02STR125 *p;

    /*
     * Architecture spec defines level-0 as being the top-most.
     * This is a bit unfortunate here, but we keep the same convention.
     */
	vpfn >>= (3 - level) * 9;
	vpfn &= 0x1FF;

	p = GB02FUNC1104(mmu_mode->gbdev, *pgd);
	if (!p) {
		gb_err(mmu_mode->gbdev->dev, "Error: %s GB02STR125 is NULL\n", __func__);
		return -EINVAL;
	}

	page = GB02FUNC1107(p);

	if (NULL == page) {
		gb_warn(mmu_mode->gbdev->dev, "GB02FUNC758: GB02FUNC1107 failure\n");
		return -EINVAL;
	}

	if (dump) {
		pr_info("pagetable: page[%lld]: 0x%llx, level: %d\n",
				vpfn, page[vpfn], level);
	} // when debug mode dump page table

	target_pgd = mmu_mode->GB02FUNC995(page[vpfn]);

	if (!target_pgd) {
		target_pgd = GB02FUNC744(mmu_mode);
		if (!target_pgd) {
			gb_err(mmu_mode->gbdev->dev, "GB02FUNC758: GB02FUNC744 failure\n");
			return -ENOMEM;
		}
		// gb_printf(KERN_INFO, "PGD: call GB02FUNC1008, target_pgd: 0x%lx\n", target_pgd);
		mmu_mode->GB02FUNC1008(&page[vpfn], target_pgd);
	}

	if (dump)
		gb_info(mmu_mode->gbdev->dev, "pagetable:0x%llx \n", target_pgd); // when debug mode dump page table

	*pgd = target_pgd;

	if (dump)
		GB02FUNC188(2 * 16, (p->gpu_phy + (vpfn * 8)) & ~0xf, NULL, 1);

	return 0;
}

static int GB02FUNC763(struct GB02STR138 *mm_node, u64 vpfn, phys_addr_t *out_pgd, int dump)
{
	phys_addr_t pgd;
	int l;

	pgd = mm_node->pgd;
	for (l = GB02MAC1194; l < GB02MAC1195; l++) {
		int err = GB02FUNC758(mm_node, &pgd, vpfn, l, dump);
		if (err) {
			gb_err(mm_node->gbdev->dev, "GB02FUNC763: GB02FUNC758 failure\n");
			return err;
		}
	}
	*out_pgd = pgd;

	return 0;
}

int GB02FUNC767(struct GB02STR138 *mmu_mode, u64 vpfn,
			phys_addr_t *phys, size_t nr_pages, unsigned long flags, int dump)
{
	phys_addr_t pgd = 0;
	size_t remain = nr_pages;
	int err;
	struct GB02STR39 *gbdev = mmu_mode->gbdev;
	u64 *pgd_page;
	mutex_lock(&mmu_mode->mmu_lock);
	while (remain) {
		unsigned int i;
		unsigned int index = vpfn & 0x1FF;
		unsigned int count = GB02MAC1193 - index;
		struct GB02STR125 *p;

		gb_printf(KERN_DEBUG, "after remain: 0x%lx %s\n", remain, __func__);
		if (count > remain)
			count = remain;

		do {
			err = GB02FUNC763(mmu_mode, vpfn, &pgd, dump);
			if (err != -ENOMEM)
				break;
		} while (!err);

		if (err) {
			gb_printf(KERN_ERR, "WARNING: %s GB02FUNC763 failed\n", __func__);
			mutex_unlock(&mmu_mode->mmu_lock);
			return err;
		}

		p = GB02FUNC1104(gbdev, pgd);
		if (!p) {
			gb_err(gbdev->dev, "Error: %s GB02STR125 is NULL\n", __func__);
			err = -EINVAL;
			mutex_unlock(&mmu_mode->mmu_lock);
			return err;
		}

		pgd_page = GB02FUNC1107(p);
		if (!pgd_page) {
			gb_warn(gbdev->dev, "gb_mmu_insert_pages: kmap failure\n");
			mutex_unlock(&mmu_mode->mmu_lock);
			return -ENOMEM;
		}

		for (i = 0; i < count; i++) {
			unsigned int ofs = index + i;
			if (gb_mmu_mmap) {
				pr_info("%s p->gpu_phy:0x%llx phys[i]:0x%llx\n",
					__func__, p->gpu_phy + (ofs * 8), phys[i]);
			}
			mmu_mode->GB02FUNC1006(&pgd_page[ofs], phys[i], flags);
		}

		if (dump)
			GB02FUNC188((count + 4) * 8, (p->gpu_phy + (index * 8)) & ~0xf, NULL, 1);

		phys += count;
		vpfn += count;
		remain -= count;
	}
	mutex_unlock(&mmu_mode->mmu_lock);
	return 0;
}

/* reference to gb_mmu_teardown_pages */
int GB02FUNC774(struct GB02STR138 *mmu_mode, u64 vpfn, size_t nr_pages)
{
	phys_addr_t pgd;
	u64 *pgd_page;
	struct GB02STR39 *gbdev = mmu_mode->gbdev;
	int err;
	u64 iova = vpfn << GB02MAC313;
	size_t requested_size = nr_pages * GB02MAC311;

	if (0 == nr_pages) {
		/* early out if nothing to do */
		return 0;
	}

	mutex_lock(&mmu_mode->mmu_lock);

	while (nr_pages) {
		unsigned int i;
		unsigned int index = vpfn & 0x1FF;
		unsigned int count = GB02MAC1193 - index;
		struct GB02STR125 *p;

		if (count > nr_pages)
			count = nr_pages;

		err = GB02FUNC763(mmu_mode, vpfn, &pgd, 0);
		if (err) {
			dev_WARN(gbdev->dev, "gb_mmu_teardown_pages: GB02FUNC763 failure\n");
			err = -EINVAL;
			goto fail_unlock;
		}

		p = GB02FUNC1104(gbdev, pgd);
		if (!p) {
			dev_err(gbdev->dev, "Error: %s GB02STR125 is NULL\n", __func__);
			err = -EINVAL;
			goto fail_unlock;
		}

		pgd_page = GB02FUNC1107(p);
		if (!pgd_page) {
			dev_WARN(gbdev->dev, "gb_mmu_teardown_pages: kmap failure\n");
			err = -ENOMEM;
			goto fail_unlock;
		}

		for (i = 0; i < count; i++) {
			if (gb_mmu_mmap) {
				pr_info("%s p->gpu_phy:0x%llx phys[i]:0x%llx\n",
					__func__, p->gpu_phy + (index + i) * 8,
					pgd_page[index + i]);
			}
			mmu_mode->GB02FUNC1009(&pgd_page[index + i]);
			if (gb_mmu_mmap) {
				pr_info("%s p->gpu_phy:0x%llx phys[i]:0x%llx\n",
					__func__, p->gpu_phy + (index + i) * 8,
					pgd_page[index + i]);
			}
		}

		vpfn += count;
		nr_pages -= count;
	}

	mutex_unlock(&mmu_mode->mmu_lock);
	GB02FUNC923(gbdev, iova, requested_size, GB02MAC2821);

	return 0;

fail_unlock:
	mutex_unlock(&mmu_mode->mmu_lock);
	return err;
}

static struct GB02STR57 *
drm_gpu_gem_gbmem_create(struct drm_device *drm, size_t size, u32 flags)
{
	struct GB02STR57 *gbmem_obj;
	struct drm_gem_object *drm_gem_obj;
	u64 gem_offset;
	int ret, nr_pages;

	size = round_up(size, PAGE_SIZE);

	drm_gem_obj = GB02FUNC797(drm, size);
	if (!drm_gem_obj)
		return ERR_PTR(-ENOMEM);

	gbmem_obj = GB02FUNC218(drm_gem_obj);

	ret = drm_gem_object_init(drm, drm_gem_obj, size);
	if (ret)
		goto error;

	ret = drm_gem_create_mmap_offset(drm_gem_obj);
	if (ret) {
		drm_gem_object_release(drm_gem_obj);
		goto error;
	}

	gem_offset = drm_vma_node_offset_addr(&drm_gem_obj->vma_node);
	gb_printf(KERN_INFO, "vma bo node: start: 0x%llx end: 0x%llx\n",
		gem_offset, gem_offset+size);

	nr_pages = gbmem_obj->nr_pages = size / PAGE_SIZE;

	gbmem_obj->pages = kvzalloc(nr_pages*sizeof(phys_addr_t), GFP_KERNEL);
	if (unlikely(!gbmem_obj->pages)) {
		gb_printf(KERN_ERR, "%s kzalloc %d pages struct %ld failed\n",
				__func__, nr_pages, nr_pages*sizeof(phys_addr_t));
		drm_gem_object_release(drm_gem_obj);
		ret = -ENOMEM;
		goto error;
	}

	return gbmem_obj;

error:
	kvfree(GB02FUNC212(drm_gem_obj));
	return ERR_PTR(ret);
}

/* referenced to vc4_bo_create */
struct GB02STR59 *
gb_gpu_bo_create(struct drm_device *dev, size_t unaligned_size, u32 flags)
{
	size_t size = roundup(unaligned_size, PAGE_SIZE);
	struct GB02STR57 *gbmem_obj;
	struct GB02STR59 *bo;
	struct GB02STR39 *gbdev;
	int ret = 0;

	if (size == 0)
		return ERR_PTR(-EINVAL);

	gbmem_obj = drm_gpu_gem_gbmem_create(dev, size, flags);
	if (IS_ERR_OR_NULL(gbmem_obj)) {
		gb_printf(KERN_ERR, "Out of vram!\n");
		return ERR_PTR(-ENOMEM);
	}

	bo = gpu_mmbo_to_gb_bo(gbmem_obj);
	gb_printf(KERN_INFO, "%s: create bo %pK size 0x%lx\n", __func__, bo, size);
	bo->gbdev = dev->dev_private;  // initialize gb device for bo

	gbdev = dev->dev_private; // get gb device from drm device private.
	bo->noexec = !!(flags & GB02MAC924);
	bo->is_heap = !!(flags & GB02MAC925);
	bo->is_kms_bo = false;
	bo->is_ttm_bo = false;

	mutex_init(&bo->base.pages_lock);

#ifdef GB02MAC286
	if (!bo->is_heap) {  // Don't alloc vram for heap memory.
		ret = GB02FUNC1132(gbdev, gbmem_obj->nr_pages, gbmem_obj->pages);
	} else {
	}
#else
	/* No heap memory in mmu by pass mode */
	ret = GB02FUNC1132(gbdev, gbmem_obj->nr_pages, gbmem_obj->pages);
#endif
	if (ret) {
		return ERR_PTR(ret);
	}
	return bo;
}

struct drm_gem_object *GB02FUNC797(struct drm_device *dev, size_t size)
{
	struct GB02STR59 *bo;

	bo = kvzalloc(sizeof(*bo), GFP_KERNEL);
	if (unlikely(!bo)) {
		gb_printf(KERN_ERR, "%s: alloc GB02STR59 failed, size 0x%lx\n", __func__, size);
		return NULL;
	}
#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	bo->gb_base.ttm_bo.bo.base.size = size;

	return &bo->gb_base.ttm_bo.bo.base;
#else
	bo->gb_base.ttm_bo.base.size = size;

	return &bo->gb_base.ttm_bo.base;
#endif
}

struct GB02STR59 *GB02FUNC800(struct drm_file *file_priv,
       struct drm_device *dev, size_t size, int init_domain,
	u32 flags, bool kernel, uint32_t *handle)
{
	int ret, i, nr_pages;
	struct GB02STR59 *bo = NULL;

	bo = GB02FUNC824(dev, size, init_domain, flags, kernel);
	if (IS_ERR(bo)) {
		gb_printf(KERN_ERR, "%s:%d----ttm bo create fail---\n",
			__func__, __LINE__);
		return bo;
	}

	bo->private_data = file_priv->driver_priv;  // hold on the gb_priv struct
	bo->noexec = !!(flags & GB02MAC924);
	bo->is_heap = !!(flags & GB02MAC925);
	bo->is_heap_growable = !!(flags & GB02MAC928);
	bo->is_low_va = !!(flags & GB02MAC927);
	bo->domain = init_domain;
	bo->vbo_save_flag = bo->vbo_save_flag_old = (flags & GB02MAC957);
	bo->is_noclear_bo = (flags & GB02MAC942);
	bo->base.nr_pages = nr_pages = PAGE_ALIGN(size) / GB02MAC311;
	bo->base.pages = kvzalloc(nr_pages*sizeof(phys_addr_t), GFP_KERNEL);
	bo->base.bo = bo;

	if (!bo->base.pages) {
		gb_printf(KERN_ERR, "zalloc mem failed!\r\n");
		return ERR_PTR(-ENOMEM);
	}

	if (!bo->is_heap_growable) {
		for (i = 0 ; i < nr_pages; i++) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
			bo->base.pages[i] = bo->gb_base.ttm_bo.bo.offset + i * GB02MAC311;
#else
			bo->base.pages[i] = bo->gb_base.ttm_bo.offset + i * GB02MAC311;
#endif
		}
	}

#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	ret = drm_gem_handle_create(file_priv, &bo->gb_base.ttm_bo.bo.base, handle);
#else
	ret = drm_gem_handle_create(file_priv, &bo->gb_base.ttm_bo.base, handle);
#endif
	if (ret) {
		gb_printf(KERN_ERR, "%s:%d----drm gem handle create-fail--\n",
			__func__, __LINE__);
		kvfree(bo->base.pages);
		return ERR_PTR(ret);
	}
	gb_printf(KERN_DEBUG, "%s bo.offset 0x%llx \n", __func__, bo->base.iopaddr);
	GB02FUNC712(bo);

	if (bo->is_heap_growable) {
		ret = GB02FUNC1513(bo, bo->gbdev, bo->private_data, bo->gb_base.iovaddr, 0 , 0);
		if (ret)
			gb_printf(KERN_INFO, "[%s] init first growable bo failed skip\n", __func__);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
	gb_printf(KERN_DEBUG, "%s bo.offset 0x%llx \n", __func__, bo->gb_base.ttm_bo.bo.offset);
#else
	gb_printf(KERN_DEBUG, "%s bo.offset 0x%llx \n", __func__, bo->gb_base.ttm_bo.offset);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	drm_gem_object_put(&bo->gb_base.ttm_bo.bo.base);
#elif KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	drm_gem_object_put_unlocked(&bo->gb_base.ttm_bo.bo.base);
#else
	drm_gem_object_put_unlocked(&bo->gb_base.ttm_bo.base);
#endif

	return bo;

}


struct GB02STR59 *
gb_gpu_gem_create_with_handle(struct drm_file *file_priv,
       struct drm_device *dev, size_t size,
       u32 flags, uint32_t *handle)
{
	int ret;
	struct GB02STR59 *bo;

	gb_printf(KERN_INFO, "%s size: %ld\n", __func__, size);

	bo = gb_gpu_bo_create(dev, size, flags);

	if (IS_ERR(bo))
		return bo;
	bo->private_data = file_priv->driver_priv;  // hold on the gb_priv struct

#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	ret = drm_gem_handle_create(file_priv, &bo->gb_base.ttm_bo.bo.base, handle);
#else
	ret = drm_gem_handle_create(file_priv, &bo->gb_base.ttm_bo.base, handle);
#endif
	if (ret)
		return ERR_PTR(ret);

#ifdef GB02MAC286
//	bo->base.iovaddr = bo->node.start << PAGE_SHIFT;
#else
//	bo->base.iovaddr = bo->base.pages[0]; // mmu by pass, set gpu physical address.
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	drm_gem_object_put(&bo->gb_base.ttm_bo.bo.base);
#elif KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	drm_gem_object_put_unlocked(&bo->gb_base.ttm_bo.bo.base);
#else
	drm_gem_object_put_unlocked(&bo->gb_base.ttm_bo.base);
#endif

	return bo;
}

struct GB02STR59 *
GB02FUNC824(struct drm_device *dev,
	size_t size, int init_domain, u32 flags, bool kernel)
{
	struct GB02STR59 *gb_bo = NULL;
	struct GB02STR56 *gbgpubo = NULL;
	int ret;

	size = PAGE_ALIGN(size);
	if (size == 0) {
		gb_printf(KERN_ERR, "%s:%d--size = 0-\n", __func__, __LINE__);
		return ERR_PTR(-EINVAL);
	}
	gb_bo = kzalloc(sizeof(struct GB02STR59), GFP_KERNEL);
	if (!gb_bo) {
		gb_printf(KERN_ERR, "%s %d error: kmalloc GB02STR59 failed!\n",
			__func__, __LINE__);
		return ERR_PTR(-ENOMEM);
	}

	gbgpubo = &gb_bo->gb_base;

	gb_printf(KERN_DEBUG, "%s:%d---cmdbuf type = %x--\n",
		__func__, __LINE__, gb_bo->cmdbuf_type);
	gbgpubo->flags = flags;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	gbgpubo->initial_domain = init_domain & TTM_PL_MASK_MEM;
#else
	gbgpubo->initial_domain = init_domain;
#endif

	//gbgpubo->ttm_bo.gb_bo_flag = true;
	ret = GB02FUNC1502(dev, size, 0, init_domain, flags, kernel, &gbgpubo->ttm_bo);
	if (ret) {
		if (ret != -ERESTARTSYS)
			gb_printf(KERN_ERR, "%s:%d--failed to create ttm bo--ret:%d\n",
				__func__, __LINE__, ret);
		return ERR_PTR(ret);
	}

	INIT_LIST_HEAD(&gbgpubo->ttm_bo_root);
	gb_bo->gbdev = dev->dev_private;
	gb_bo->is_kms_bo = false;
	gb_bo->is_ttm_bo = true;
	gb_bo->is_fb_bo = !!(flags & GB02MAC926);
	mutex_init(&gb_bo->base.pages_lock);

	return gb_bo;
}
