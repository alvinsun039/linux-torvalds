#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/console.h>
#include <linux/pagemap.h>
#include <linux/version.h>
#include <drm/drm_gem.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_tt.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
#include <drm/ttm/ttm_range_manager.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#include <drm/drm_gem_ttm_helper.h>
#include <drm/drm_mm.h>
#include <linux/spinlock.h>
#endif
#ifdef CONFIG_SKYLIN_OS_V10
#include <asm/machine_t.h>
#endif
#include "common/gb_bo.h"
#include "common/gb_uk.h"
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "device/gb_dev_res.h"
#include "kms/gbdc_fbdev.h"
#include "kms/gbdc_pm.h"
#include "kms/gb_kms.h"
#include "kms/gbdc_drv.h"
#include "gb_ttm.h"
#include "gb_mmu.h"
#include "gb_regs.h"
#include "vpu/vpu_mm/gb_vpu_ttm.h"
#include "gb_device.h"
#include "common/gb_gpuinfo.h"
#include "common/gb_kernel_ver.h"
#include "ip/gb_edma.h"
#include "ip/edma_reg.h"

extern bool gb_rb_tree;
extern int gb_pg_dump_state;

struct GB02STR166 {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	struct ttm_resource_manager manager;
#endif
	struct drm_mm mm;
	spinlock_t lock;
};

atomic_t alloced_pages = ATOMIC_INIT(0);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static void GB02FUNC1386(struct GB02STR50 *bo, int domain)
{
	unsigned i;
	u32 c = 0;
	bo->placement.placement = bo->placements;
	bo->placement.busy_placement = bo->placements;
	if (domain & TTM_PL_FLAG_VRAM) {
#ifdef CONFIG_SKYLIN_OS_V10
		if (is_ft1500a()) {
			bo->placements[c++].flags =
				TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_VRAM;
		} else {
			bo->placements[c].fpfn = 0;
			bo->placements[c++].flags = TTM_PL_FLAG_WC
				| TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_VRAM;
		}
#else
		bo->placements[c++].flags = TTM_PL_FLAG_WC
			| TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_VRAM;
#endif
	} else if (domain & GB02MAC2678) {
#ifdef CONFIG_SKYLIN_OS_V10
		if (is_ft1500a()) {
			bo->placements[c++].flags =
				TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_PRIV;
		} else {
			bo->placements[c].fpfn = 0;
			bo->placements[c++].flags = TTM_PL_FLAG_WC
				| TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_PRIV;
		}
#else
		bo->placements[c++].flags = TTM_PL_FLAG_WC
			| TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_PRIV;
#endif

	}
	if (domain & TTM_PL_FLAG_SYSTEM) {
		bo->placements[c++].flags = TTM_PL_MASK_CACHING
		    | TTM_PL_FLAG_SYSTEM;
	}
	if (!c) {
		bo->placements[c].fpfn = 0;
		bo->placements[c++].flags = TTM_PL_MASK_CACHING
		    | TTM_PL_FLAG_SYSTEM;
	}
	for (i = 0; i < c; ++i) {
		if (bo->placements[i].flags & TTM_PL_FLAG_PRIV) {
			bo->placements[i].lpfn =
				GB02MAC488 >> PAGE_SHIFT;
		}
		else {
			bo->placements[i].fpfn = 0;
			bo->placements[i].lpfn = 0;
		}
	}
	bo->placement.num_placement = c;
	bo->placement.num_busy_placement = c;
}
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static void GB02FUNC1386(struct GB02STR50 *bo, int domain)
{
	unsigned i;
	u32 c = 0;
	bo->placement.placement = bo->placements;
	bo->placement.busy_placement = bo->placements;
	if (domain == TTM_PL_VRAM) {
#ifdef CONFIG_SKYLIN_OS_V10
		if (is_ft1500a()) {
			bo->placements[c].mem_type = TTM_PL_VRAM;
			bo->placements[c++].flags =
				TTM_PL_FLAG_UNCACHED | TTM_PL_VRAM;
		} else {
			bo->placements[c].fpfn = 0;
			bo->placements[c].mem_type = TTM_PL_VRAM;
			bo->placements[c++].flags = TTM_PL_FLAG_WC
				| TTM_PL_FLAG_UNCACHED | TTM_PL_VRAM;
		}
#else
		bo->placements[c].mem_type = TTM_PL_VRAM;
		bo->placements[c++].flags = TTM_PL_FLAG_WC
			| TTM_PL_FLAG_UNCACHED | TTM_PL_VRAM;
#endif
	} else if (domain == GB02MAC2678) {
#ifdef CONFIG_SKYLIN_OS_V10
		if (is_ft1500a()) {
			bo->placements[c].mem_type = TTM_PL_PRIV;
			bo->placements[c++].flags =
				TTM_PL_FLAG_UNCACHED | TTM_PL_PRIV;
		} else {
			bo->placements[c].fpfn = 0;
			bo->placements[c].mem_type = TTM_PL_PRIV;
			bo->placements[c++].flags = TTM_PL_FLAG_WC
				| TTM_PL_FLAG_UNCACHED | TTM_PL_PRIV;
		}
#else
		bo->placements[c].mem_type = TTM_PL_PRIV;
		bo->placements[c++].flags = TTM_PL_FLAG_WC
			| TTM_PL_FLAG_UNCACHED | TTM_PL_PRIV;
#endif

	}
	if (domain == TTM_PL_SYSTEM) {
		bo->placements[c].mem_type = TTM_PL_SYSTEM;
		bo->placements[c++].flags = TTM_PL_MASK_CACHING
		    | TTM_PL_SYSTEM;
	}
	if (!c) {
		bo->placements[c].fpfn = 0;
		bo->placements[c].mem_type = TTM_PL_SYSTEM;
		bo->placements[c++].flags = TTM_PL_MASK_CACHING
		    | TTM_PL_SYSTEM;
	}
	for (i = 0; i < c; ++i) {
		if (bo->placements[i].flags & TTM_PL_PRIV) {
			bo->placements[i].lpfn =
				GB02MAC488 >> PAGE_SHIFT;
		}
		else {
			bo->placements[i].fpfn = 0;
			bo->placements[i].lpfn = 0;
		}
	}
	bo->placement.num_placement = c;
	bo->placement.num_busy_placement = c;
}
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0) */
static void GB02FUNC1386(struct GB02STR50 *bo, int domain)
{
	unsigned i;
	u32 c = 0;
	bo->placement.placement = bo->placements;
	bo->placement.busy_placement = bo->placements;
	if (domain == TTM_PL_VRAM) {
#ifdef CONFIG_SKYLIN_OS_V10
		if (is_ft1500a()) {
			bo->placements[c].mem_type = TTM_PL_VRAM;
			bo->placements[c].flags = TTM_PL_FLAG_UNCACHED;
			c++;
		} else {
			bo->placements[c].fpfn = 0;
			bo->placements[c].mem_type = TTM_PL_VRAM;
			bo->placements[c].flags = 0;
			c++;
		}
	} else if (domain == GB02MAC2678) {
		if (is_ft1500a()) {
			bo->placements[c].mem_type = TTM_PL_PRIV;
			bo->placements[c].flags = 0;
			c++;
		} else {
			bo->placements[c].fpfn = 0;
			bo->placements[c].mem_type = TTM_PL_PRIV;
			bo->placements[c].flags = 0;
			c++;
		}
#else
		bo->placements[c].mem_type = TTM_PL_VRAM;
		bo->placements[c].flags = 0;
		c++;
	} else if (domain == GB02MAC2678) {
		bo->placements[c].mem_type = TTM_PL_PRIV;
		bo->placements[c].flags = 0;
		c++;
#endif

	}
	if (domain == TTM_PL_SYSTEM) {
		bo->placements[c].mem_type = TTM_PL_SYSTEM;
		bo->placements[c].flags = 0;
		c++;
	}

	if (!c) {
		bo->placements[c].fpfn = 0;
		bo->placements[c].mem_type = TTM_PL_SYSTEM;
		bo->placements[c].flags = 0;
		c++;
	}

	for (i = 0; i < c; i++) {
		if (bo->placements[i].mem_type == TTM_PL_PRIV) {
			bo->placements[i].lpfn =
				GB02MAC488 >> PAGE_SHIFT;
		} else {
			bo->placements[i].fpfn = 0;
			bo->placements[i].lpfn = 0;
		}
	}
	bo->placement.num_placement = c;
	bo->placement.num_busy_placement = c;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */
u64 GB02FUNC1394(struct GB02STR50 *bo)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0))
	return drm_vma_node_offset_addr(&bo->bo.base.vma_node);
#else
	return drm_vma_node_offset_addr(&bo->bo.vma_node);
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
struct GB02STR39 *gb_ttm_gb_bdev(struct ttm_bo_device *bd)
#else
struct GB02STR39 *gb_ttm_gb_bdev(struct ttm_device *bd)
#endif
{
	return container_of(bd, struct GB02STR39, gb_mm.ttm.bdev);
}

#if (!(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	defined SYS_CENTOS7_COMPILE_ENV) \
	|| (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0) && \
	!defined CONFIG_CENTOS_OS)
static int gb_ttm_mem_global_init(struct drm_global_reference *ref)
{
	return ttm_mem_global_init(ref->object);
}

static void gb_ttm_mem_global_release(struct drm_global_reference *ref)
{
	ttm_mem_global_release(ref->object);
}

static int gb_ttm_global_init(struct GB02STR175 *ttm)
{
	int r;
	struct drm_global_reference *global_ref;

	global_ref = &ttm->mem_global_ref;
	global_ref->global_type = DRM_GLOBAL_TTM_MEM;
	global_ref->size = sizeof(struct ttm_mem_global);
	global_ref->init = &gb_ttm_mem_global_init;
	global_ref->release = &gb_ttm_mem_global_release;
	r = drm_global_item_ref(global_ref);
	if (r != 0) {
		DRM_ERROR("Failed setting up TTM memory accounting "
			  "subsystem.\n");
		gb_printf(KERN_ERR, "Failed setting up TTM memory\n");
		return r;
	}

	ttm->bo_global_ref.mem_glob = ttm->mem_global_ref.object;
	global_ref = &ttm->bo_global_ref.ref;
	global_ref->global_type = DRM_GLOBAL_TTM_BO;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	global_ref->size = sizeof(struct ttm_bo_global);
#else
	global_ref->size = sizeof(struct ttm_global);
#endif
	global_ref->init = &ttm_bo_global_init;
	global_ref->release = &ttm_bo_global_release;
	r = drm_global_item_ref(global_ref);
	if (r != 0) {
		DRM_ERROR("Failed setting up TTM BO subsystem.\n");
		drm_global_item_unref(&ttm->mem_global_ref);
		return r;
	}

	return 0;
}

static void GB02FUNC1398(struct GB02STR175 *ttm)
{
	if (ttm->mem_global_ref.release == NULL)
		return;

	drm_global_item_unref(&ttm->bo_global_ref.ref);
	drm_global_item_unref(&ttm->mem_global_ref);
	ttm->mem_global_ref.release = NULL;
}
#endif

static void GB02FUNC1399(struct ttm_buffer_object *tbo)
{
	struct GB02STR50 *ttm_bo = ttm_ob_to_ttm_bo(tbo);
	struct GB02STR59 *gb_bo = NULL;
	struct GB02STR39 *gbdev = NULL;
	struct gb_heap_ttm_bo *gb_heap_bo = NULL;

	gb_printf(KERN_DEBUG, "--%s:%d----bo:0x%llx bo.offset:0x%llx--\n",
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
	__func__, __LINE__, (u64)&ttm_bo->bo, ttm_bo->bo.offset);
#else
	__func__, __LINE__, (u64)&ttm_bo->bo, ttm_bo->offset);
#endif
	if (!ttm_bo->sub_bo_flag) {
		gb_bo = GB02FUNC205(ttm_bo);
		gbdev = gb_bo->gbdev;
	} else {
		gb_heap_bo = container_of(ttm_bo, struct gb_heap_ttm_bo, ttm_bo);
	}	
#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	drm_gem_object_release(&ttm_bo->bo.base);
#else
	drm_gem_object_release(&ttm_bo->base);
#endif
	if (gb_bo != NULL) {
		kvfree(gb_bo);
		gb_bo = NULL;
	}

	if (gb_heap_bo) {
		kvfree(gb_heap_bo);
		gb_heap_bo = NULL;
	}
}

static bool GB02FUNC1401(struct ttm_buffer_object *bo)
{
	if (bo->destroy == &GB02FUNC1399)
		return true;

	return false;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static int gb_ttm_bo_man_init(struct ttm_mem_type_manager *man,
			   unsigned long p_size)
{
	struct GB02STR166 *rman;

	rman = kzalloc(sizeof(*rman), GFP_KERNEL);
	if (!rman)
		return -ENOMEM;

	drm_mm_init(&rman->mm, 0, p_size);
	spin_lock_init(&rman->lock);
	man->priv = rman;
	return 0;
}

static int gb_ttm_bo_man_takedown(struct ttm_mem_type_manager *man)
{
	struct GB02STR166 *rman = (struct GB02STR166 *) man->priv;
	struct drm_mm *mm = &rman->mm;

	spin_lock(&rman->lock);
	if (drm_mm_clean(mm)) {
		drm_mm_takedown(mm);
		spin_unlock(&rman->lock);
		kfree(rman);
		man->priv = NULL;
		return 0;
	}
	spin_unlock(&rman->lock);
	return -EBUSY;
}

static int gb_ttm_bo_man_get_node(struct ttm_mem_type_manager *man,
			       struct ttm_buffer_object *bo,
			       const struct ttm_place *place,
			       struct ttm_mem_reg *mem)
{
	struct GB02STR166 *rman = (struct GB02STR166 *) man->priv;
	struct drm_mm *mm = &rman->mm;
	struct drm_mm_node *node;
	struct gpu_info *gpu_info = GB02FUNC314();
	enum drm_mm_insert_mode mode;
	unsigned long lpfn;
	int ret = 0;

	lpfn = place->lpfn;
	if (!lpfn)
		lpfn = man->size;

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	mode = DRM_MM_INSERT_BEST;
	if (place->flags & TTM_PL_FLAG_TOPDOWN)
		mode = DRM_MM_INSERT_HIGH;

	spin_lock(&rman->lock);
	ret = drm_mm_insert_node_in_range(mm, node,
					  mem->num_pages,
					  mem->page_alignment, 0,
					  place->fpfn, lpfn, mode);
	spin_unlock(&rman->lock);

	if (unlikely(ret)) {
		pr_err("[%s]cant alloc vram %lu pages, total alloc %u pages gpu_used:%ld vpu_used:%ld\n",
				__func__, mem->num_pages,
				(unsigned int)atomic_read(&alloced_pages),
				gpu_info->gpu_used, gpu_info->vpu_used);
		kfree(node);
	} else {
		mem->mm_node = node;
		mem->start = node->start;
		atomic_add(mem->num_pages, &alloced_pages);
	}

	return ret;
}

static void gb_ttm_bo_man_put_node(struct ttm_mem_type_manager *man,
				struct ttm_mem_reg *mem)
{
	struct GB02STR166 *rman = (struct GB02STR166 *) man->priv;

	if (mem->mm_node) {
		spin_lock(&rman->lock);
		drm_mm_remove_node(mem->mm_node);
		atomic_sub(mem->num_pages, &alloced_pages);
		spin_unlock(&rman->lock);

		kfree(mem->mm_node);
		mem->mm_node = NULL;
	}
}

static void gb_ttm_bo_man_debug(struct ttm_mem_type_manager *man,
			     struct drm_printer *printer)
{
	struct GB02STR166 *rman = (struct GB02STR166 *) man->priv;

	spin_lock(&rman->lock);
	drm_mm_print(&rman->mm, printer);
	spin_unlock(&rman->lock);
}

const struct ttm_mem_type_manager_func gb_ttm_bo_manager_func = {
	.init = gb_ttm_bo_man_init,
	.takedown = gb_ttm_bo_man_takedown,
	.get_node = gb_ttm_bo_man_get_node,
	.put_node = gb_ttm_bo_man_put_node,
	.debug = gb_ttm_bo_man_debug
};
#else /* > 5.10 */
static inline struct GB02STR166 *
gb_to_range_manager(struct ttm_resource_manager *man)
{
	return container_of(man, struct GB02STR166, manager);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static int gb_ttm_range_man_alloc(struct ttm_resource_manager *man,
			       struct ttm_buffer_object *bo,
			       const struct ttm_place *place,
			       struct ttm_resource *mem)
{
	struct GB02STR166 *rman = gb_to_range_manager(man);
	struct drm_mm *mm = &rman->mm;
	struct drm_mm_node *node;
	enum drm_mm_insert_mode mode;
	unsigned long lpfn;
	int ret;

	lpfn = place->lpfn;
	if (!lpfn)
		lpfn = man->size;

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	mode = DRM_MM_INSERT_BEST;
	if (place->flags & TTM_PL_FLAG_TOPDOWN)
		mode = DRM_MM_INSERT_HIGH;

	spin_lock(&rman->lock);
	ret = drm_mm_insert_node_in_range(mm, node,
					  mem->num_pages,
					  mem->page_alignment, 0,
					  place->fpfn, lpfn, mode);
	spin_unlock(&rman->lock);

	if (unlikely(ret)) {
		kfree(node);
	} else {
		mem->mm_node = node;
		mem->start = node->start;
	}

	return ret;
}

static void GB02FUNC1409(struct ttm_resource_manager *man,
			       struct ttm_resource *mem)
{
	struct GB02STR166 *rman = gb_to_range_manager(man);

	if (mem->mm_node) {
		spin_lock(&rman->lock);
		drm_mm_remove_node(mem->mm_node);
		spin_unlock(&rman->lock);

		kfree(mem->mm_node);
		mem->mm_node = NULL;
	}
}

static void GB02FUNC1413(struct ttm_resource_manager *man,
				struct drm_printer *printer)
{
	struct GB02STR166 *rman = gb_to_range_manager(man);

	spin_lock(&rman->lock);
	drm_mm_print(&rman->mm, printer);
	spin_unlock(&rman->lock);
}
#else
static int gb_ttm_range_man_alloc(struct ttm_resource_manager *man,
			       struct ttm_buffer_object *bo,
			       const struct ttm_place *place,
			       struct ttm_resource **res)
{
	struct GB02STR166 *rman = gb_to_range_manager(man);
	struct ttm_range_mgr_node *node;
	struct drm_mm *mm = &rman->mm;
	enum drm_mm_insert_mode mode;
	unsigned long lpfn;
	int ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	uint32_t num_pages = 0;
#endif

	lpfn = place->lpfn;
	if (!lpfn)
		lpfn = man->size;

	node = kzalloc(struct_size(node, mm_nodes, 1), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	mode = DRM_MM_INSERT_BEST;
	if (place->flags & TTM_PL_FLAG_TOPDOWN)
		mode = DRM_MM_INSERT_HIGH;

	ttm_resource_init(bo, place, &node->base);

	spin_lock(&rman->lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)	
	ret = drm_mm_insert_node_in_range(mm, &node->mm_nodes[0],
					  node->base.num_pages,
					  bo->page_alignment, 0,
					  place->fpfn, lpfn, mode);
#else
	num_pages = PFN_UP(node->base.size);
	ret = drm_mm_insert_node_in_range(mm, &node->mm_nodes[0],
					  num_pages,
					  bo->page_alignment, 0,
					  place->fpfn, lpfn, mode);
#endif
	spin_unlock(&rman->lock);

	if (unlikely(ret)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)	
		pr_err("[%s]cant alloc vram %lu pages, total alloc %u pages",
				__func__, node->base.num_pages, atomic_read(&alloced_pages));
#else
		pr_err("[%s]cant alloc vram %u pages, total alloc %u pages",
				__func__, num_pages, atomic_read(&alloced_pages));
#endif
		ttm_resource_fini(man, &node->base);
		kfree(node);
		return -ENOMEM;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)	
	atomic_add(node->base.num_pages, &alloced_pages);
#else
	atomic_add(num_pages, &alloced_pages);
#endif
	node->base.start = node->mm_nodes[0].start;
	*res = &node->base;
	return 0;
}

static void GB02FUNC1409(struct ttm_resource_manager *man,
			       struct ttm_resource *res)
{
	struct ttm_range_mgr_node *node = to_ttm_range_mgr_node(res);
	struct GB02STR166 *rman = gb_to_range_manager(man);

	spin_lock(&rman->lock);
	drm_mm_remove_node(&node->mm_nodes[0]);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)	
	atomic_sub(node->base.num_pages, &alloced_pages);
#else
	atomic_sub(node->mm_nodes[0].size, &alloced_pages);
#endif
	spin_unlock(&rman->lock);

	ttm_resource_fini(man, res);
	kfree(node);
}

static void GB02FUNC1413(struct ttm_resource_manager *man,
				struct drm_printer *printer)
{
	struct GB02STR166 *rman = gb_to_range_manager(man);

	spin_lock(&rman->lock);
	drm_mm_print(&rman->mm, printer);
	spin_unlock(&rman->lock);
}
#endif

static const struct ttm_resource_manager_func gb_ttm_range_manager_func = {
	.alloc = gb_ttm_range_man_alloc,
	.free = GB02FUNC1409,
	.debug = GB02FUNC1413
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
int gb_ttm_range_man_init(struct ttm_bo_device *bdev, unsigned type,
		       unsigned long p_size)
#else
int gb_ttm_range_man_init(struct ttm_device *bdev, unsigned type,
		       unsigned long p_size)
#endif
{
	struct ttm_resource_manager *man;
	struct GB02STR166 *rman;

	rman = kzalloc(sizeof(*rman), GFP_KERNEL);
	if (!rman)
		return -ENOMEM;

	man = &rman->manager;
	man->use_tt = false;

	man->func = &gb_ttm_range_manager_func;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	ttm_resource_manager_init(man, p_size);
#else
	ttm_resource_manager_init(man, bdev, p_size);
#endif

	drm_mm_init(&rman->mm, 0, p_size);
	spin_lock_init(&rman->lock);

	ttm_set_driver_manager(bdev, type, &rman->manager);
	ttm_resource_manager_set_used(man, true);
	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
int gb_ttm_range_man_fini(struct ttm_bo_device *bdev,
		       unsigned type)
#else
int gb_ttm_range_man_fini(struct ttm_device *bdev,
		       unsigned type)
#endif
{
	struct ttm_resource_manager *man = ttm_manager_type(bdev, type);
	struct GB02STR166 *rman = gb_to_range_manager(man);
	struct drm_mm *mm = &rman->mm;
	int ret;

	if (!man)
		return 0;

	ttm_resource_manager_set_used(man, false);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
	ret = ttm_resource_manager_force_list_clean(bdev, man);
	if (ret)
		return ret;
#else
	ret = ttm_resource_manager_evict_all(bdev, man);
	if (ret)
		return ret;
#endif
	spin_lock(&rman->lock);
	drm_mm_clean(mm);
	drm_mm_takedown(mm);
	spin_unlock(&rman->lock);

	ttm_resource_manager_cleanup(man);
	ttm_set_driver_manager(bdev, type, NULL);
	kfree(rman);
	return 0;
}
#endif 

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static int gb_ttm_init_mem_type(struct ttm_bo_device *bdev,
		uint32_t type, struct ttm_mem_type_manager *man)
{
	switch (type) {
	case TTM_PL_SYSTEM:
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE;
		man->available_caching = TTM_PL_MASK_CACHING;
		man->default_caching = TTM_PL_FLAG_CACHED;
		break;
	case TTM_PL_VRAM:
		man->func = &gb_ttm_bo_manager_func;
		man->gpu_offset = GB02MAC500;
		man->flags = TTM_MEMTYPE_FLAG_FIXED | TTM_MEMTYPE_FLAG_MAPPABLE;
#ifdef CONFIG_LOONGARCH
		man->available_caching = TTM_PL_FLAG_UNCACHED;
		man->default_caching = TTM_PL_FLAG_UNCACHED;
#else
#ifdef CONFIG_SKYLIN_OS_V10
		if (is_ft1500a()) {
			man->available_caching = TTM_PL_FLAG_UNCACHED;
			man->default_caching = TTM_PL_FLAG_UNCACHED;
		} else {
			man->available_caching = TTM_PL_FLAG_UNCACHED |
				TTM_PL_FLAG_WC;
			man->default_caching = TTM_PL_FLAG_WC;
		}
#else
		man->available_caching = TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
#endif
#endif
		break;
	case GB02MAC2677:
		/* On-chip GDS memory*/
		man->func = &gb_ttm_bo_manager_func;
		man->gpu_offset = GB02MAC499;
		man->flags = TTM_MEMTYPE_FLAG_FIXED | TTM_MEMTYPE_FLAG_MAPPABLE;
#ifdef CONFIG_LOONGARCH
		man->available_caching = TTM_PL_FLAG_UNCACHED;
		man->default_caching = TTM_PL_FLAG_UNCACHED;
#else
#ifdef CONFIG_SKYLIN_OS_V10
		if (is_ft1500a()) {
			man->available_caching = TTM_PL_FLAG_UNCACHED;
			man->default_caching = TTM_PL_FLAG_UNCACHED;
		} else {
			man->available_caching =
				TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
			man->default_caching = TTM_PL_FLAG_WC;
		}
#else
		man->available_caching = TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
#endif
#endif
		break;

	default:
		DRM_ERROR("Unsupported memory type %u\n", (unsigned)type);
		return -EINVAL;
	}
	return 0;
}
#endif
static void
gb_ttm_bo_evict_flags(struct ttm_buffer_object *bo, struct ttm_placement *pl)
{
	struct GB02STR50 *ttm_bo = ttm_ob_to_ttm_bo(bo);
	if (!GB02FUNC1401(bo))
		return;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
	switch (bo->mem.mem_type) {
#else
	switch (bo->resource->mem_type) {
#endif
		case TTM_PL_VRAM:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
			GB02FUNC1386(ttm_bo, TTM_PL_FLAG_SYSTEM);
#else
			GB02FUNC1386(ttm_bo, TTM_PL_SYSTEM);
#endif
			break;
		case TTM_PL_SYSTEM:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
			GB02FUNC1386(ttm_bo, TTM_PL_FLAG_VRAM);
#else
			GB02FUNC1386(ttm_bo, TTM_PL_VRAM);
#endif
			break;
		case TTM_PL_PRIV:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
			GB02FUNC1386(ttm_bo, TTM_PL_FLAG_SYSTEM);
#else
			GB02FUNC1386(ttm_bo, TTM_PL_SYSTEM);
#endif
			break;
		default:
			break;
	}

	*pl = ttm_bo->placement;
}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0))
static int gb_ttm_bo_verify_access(struct ttm_buffer_object *bo,
				 struct file *filp)
{
	struct drm_gem_object *drm_ob = NULL;
	struct GB02STR59 *gb_bo = NULL;
	struct GB02STR50 *ttm_bo = ttm_ob_to_ttm_bo(bo);
	if (!ttm_bo) {
		gb_printf(KERN_ERR, "%s ttm_bo == null\n", __func__);
		return -1;
	}
	gb_bo = GB02FUNC205(ttm_bo);
	if (!gb_bo) {
		gb_printf(KERN_ERR, "%s gb_bo == null\n", __func__);
		return -1;
	}

#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	drm_ob = &gb_bo->gb_base.ttm_bo.bo.base;
#else
	drm_ob = &gb_bo->gb_base.ttm_bo.base;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) || (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS)\
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS)
	return drm_vma_node_verify_access(&drm_ob->vma_node,
			filp->private_data);
#else // ARM64 & MIPS
	return drm_vma_node_verify_access(&drm_ob->vma_node, filp);
#endif

}
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static int gb_ttm_io_mem_reserve(struct ttm_bo_device *bdev,
				   struct ttm_resource *mem)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static int gb_ttm_io_mem_reserve(struct ttm_bo_device *bdev,
				   struct ttm_mem_reg *mem)
#else
static int gb_ttm_io_mem_reserve(struct ttm_device *bdev,
				   struct ttm_resource *mem)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct ttm_mem_type_manager *man = &bdev->man[mem->mem_type];
#endif
	struct GB02STR39 *gb_dev = gb_ttm_gb_bdev(bdev);
	struct GB02STR253 *vram_config = &gb_dev->gbdc_dev->pcie_info.vram_config;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	mem->bus.addr = NULL;
	mem->bus.offset = 0;
	mem->bus.size = mem->num_pages << PAGE_SHIFT;
	mem->bus.base = 0;
	mem->bus.is_iomem = false;

	if (!(man->flags & TTM_MEMTYPE_FLAG_MAPPABLE))
		return -EINVAL;
#endif

	switch (mem->mem_type) {
	case TTM_PL_SYSTEM:
		gb_printf(KERN_DEBUG, "inside %s TTM_PL_SYSTEM... mem->start: 0x%lx\n",
			__func__, mem->start);
		/* system memory */
		return 0;
	case TTM_PL_VRAM:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
		mem->bus.offset = mem->start << PAGE_SHIFT;
		mem->bus.base = vram_config->fb_base + GB02MAC500;
#else
		mem->bus.offset = (mem->start << PAGE_SHIFT) +
			vram_config->fb_base + GB02MAC500;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
		mem->bus.caching = ttm_write_combined;
#endif
#endif
		mem->bus.is_iomem = true;
		gb_printf(KERN_DEBUG, "%s:%d +++++++++++ TTM_PL_VRAM... mem->start: 0x%lx\n",
			__func__, __LINE__,  mem->start);
		break;
	case GB02MAC2677:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
		mem->bus.offset = mem->start << PAGE_SHIFT;
		mem->bus.base = vram_config->fb_base + GB02MAC499;
#else
		mem->bus.offset = (mem->start << PAGE_SHIFT) +
			vram_config->fb_base + GB02MAC499;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
		mem->bus.caching = ttm_write_combined;
#endif
#endif
		mem->bus.is_iomem = true;
		gb_printf(KERN_DEBUG, "%s-%d: ------VPU mem->start: 0x%lx, bus.base = 0x%llx\n",
			__func__, __LINE__, mem->start, (u64)mem->bus.addr);

		break;
	default:
		return -EINVAL;
		break;
	}

	gb_printf(KERN_DEBUG, "inside %s finished...\n", __func__);
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static void gb_ttm_io_mem_free(struct ttm_bo_device *bdev,
				 struct ttm_resource *mem)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static void gb_ttm_io_mem_free(struct ttm_bo_device *bdev,
				 struct ttm_mem_reg *mem)
#else
static void gb_ttm_io_mem_free(struct ttm_device *bdev,
				 struct ttm_resource *mem)
#endif
{
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static void gb_ttm_backend_destroy(struct ttm_tt *tt)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static void gb_ttm_backend_destroy(struct ttm_bo_device *bdev,
	struct ttm_tt *tt)
#else
static void gb_ttm_backend_destroy(struct ttm_device *bdev,
	struct ttm_tt *tt)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	ttm_tt_destroy_common(bdev, tt);
#endif
	ttm_tt_fini(tt);
	kfree(tt);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static struct ttm_backend_func gb_ttm_tt_backend_func = {
	.destroy = &gb_ttm_backend_destroy,
};
#endif

#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
static struct ttm_tt *gb_ttm_tt_create(struct ttm_buffer_object *bo,
		uint32_t page_flags)
#else
static struct ttm_tt *gb_ttm_tt_create(struct ttm_bo_device *bdev,
		unsigned long size, uint32_t page_flags,
		struct page *dummy_read_page)
#endif
{
	struct ttm_tt *tt;

	tt = kzalloc(sizeof(struct ttm_tt), GFP_KERNEL);
	if (tt == NULL)
		return NULL;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	tt->func = &gb_ttm_tt_backend_func;
#endif
#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
	if (ttm_tt_init(tt, bo, page_flags)) {
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	if (ttm_tt_init(tt, bo, page_flags, ttm_write_combined, 0)) {
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	if (ttm_tt_init(tt, bo, page_flags, ttm_write_combined)) {
#else
	if (ttm_tt_init(tt, bdev, size, page_flags, dummy_read_page)) {
#endif
		kfree(tt);
		return NULL;
	}
	return tt;
}
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 19, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
int gb_ttm_pool_populate(struct ttm_tt *ttm, struct ttm_operation_ctx *ctx)
{
	return ttm_pool_populate(ttm, ctx);
}

void gb_ttm_pool_unpopulate(struct ttm_tt *ttm)
{
	ttm_pool_unpopulate(ttm);
}

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
int gb_ttm_pool_populate(struct ttm_bo_device *bdev,
			struct ttm_tt *ttm,
			struct ttm_operation_ctx *ctx)
{
	return ttm_pool_populate(ttm, ctx);
}

void gb_ttm_pool_unpopulate(struct ttm_bo_device *bdev,
			struct ttm_tt *ttm)
{
	ttm_pool_unpopulate(ttm);
}

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
int gb_ttm_pool_populate(struct ttm_device *bdev,
			struct ttm_tt *ttm,
			struct ttm_operation_ctx *ctx)
{
	return ttm_pool_alloc(&bdev->pool, ttm, ctx);
}

void gb_ttm_pool_unpopulate(struct ttm_device *bdev,
			struct ttm_tt *ttm)
{
	ttm_pool_free(&bdev->pool, ttm);
}
#else
int gb_ttm_pool_populate(struct ttm_tt *ttm)
{
	return ttm_pool_populate(ttm);
}

void gb_ttm_pool_unpopulate(struct ttm_tt *ttm)
{
	ttm_pool_unpopulate(ttm);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static void gbgpu_move_null(struct ttm_buffer_object *bo,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
				 struct ttm_mem_reg *new_mem)
#else
				 struct ttm_resource *new_mem)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct ttm_mem_reg *old_mem = &bo->mem;
#else
	struct ttm_resource *old_mem = bo->resource;
#endif
	BUG_ON(old_mem->mm_node != NULL);
	*old_mem = *new_mem;
	new_mem->mm_node = NULL;
}
#endif

#ifdef GB_VRAM_MOVE_API
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 13, 0)
int ttm_mem_io_reserve(struct ttm_device *bdev,
		       struct ttm_resource *mem)
{
	if (mem->bus.offset || mem->bus.addr)
		return 0;

	mem->bus.is_iomem = false;
	if (!bdev->funcs->io_mem_reserve)
		return 0;

	return bdev->funcs->io_mem_reserve(bdev, mem);
}

void ttm_mem_io_free(struct ttm_device *bdev,
		     struct ttm_resource *mem)
{
	if (!mem)
		return;

	if (!mem->bus.offset && !mem->bus.addr)
		return;

	if (bdev->funcs->io_mem_free)
		bdev->funcs->io_mem_free(bdev, mem);

	mem->bus.offset = 0;
	mem->bus.addr = NULL;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static int gb_ttm_mem_reg_ioremap(struct ttm_bo_device *bdev, struct ttm_mem_reg *mem,
			void **virtual)
{
	struct ttm_mem_type_manager *man = &bdev->man[mem->mem_type];

	int ret;
	void *addr;
	*virtual = NULL;
	(void) ttm_mem_io_lock(man, false);
	ret = ttm_mem_io_reserve(bdev, mem);
	ttm_mem_io_unlock(man);

	if (ret || !mem->bus.is_iomem)
		return ret;

	if (mem->bus.addr) {
		addr = mem->bus.addr;
	} else {
		if (mem->placement & TTM_PL_FLAG_WC)
			addr = ioremap_wc(mem->bus.base + mem->bus.offset, mem->bus.size);
		else
			addr = ioremap_nocache(mem->bus.base + mem->bus.offset, mem->bus.size);
		if (!addr) {
			(void) ttm_mem_io_lock(man, false);
			ttm_mem_io_free(bdev, mem);
			ttm_mem_io_unlock(man);
			return -ENOMEM;
		}
	}
	*virtual = addr;
	return 0;
}
#else
static int gb_ttm_mem_reg_ioremap(struct ttm_device *bdev, struct ttm_resource *mem,
			void **virtual)
{
	int ret;
	void *addr;
	*virtual = NULL;

	ret = ttm_mem_io_reserve(bdev, mem);

	if (ret || !mem->bus.is_iomem)
		return ret;

	if (mem->bus.addr) {
		addr = mem->bus.addr;
	} else {
		if (mem->bus.caching & ttm_write_combined)
			addr = ioremap_wc(0 + mem->bus.offset, mem->num_pages * PAGE_SIZE);
		else
			addr = ioremap_cache(0 + mem->bus.offset, mem->num_pages * PAGE_SIZE);
		if (!addr) {
			ttm_mem_io_free(bdev, mem);
			return -ENOMEM;
		}
	}
	*virtual = addr;
	return 0;
}
#endif

static int GB02FUNC1444(struct GB02STR39 *gbdev,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
							struct ttm_mem_reg *reg_mem,
#else
							struct ttm_resource *reg_mem,
#endif
							struct ttm_tt *ttm, int nr_pages, int write)
{
	int i, ret;
	phys_addr_t *gpu_phys = NULL;
	u64 src_addr;
	struct GB02STR188 dma_info = {0};
	struct page **user_pages = ttm->pages;//cpu_va
	unsigned long num_dma_pages = nr_pages / GB02MAC317;
	unsigned long gpu_num_dma_pages = nr_pages;
	u64 size = num_dma_pages * PAGE_SIZE;

	src_addr = (reg_mem->start << GB02MAC313) + GB02MAC500;

	mutex_lock(&gbdev->mmu_as_lock);
	gpu_phys = kzalloc(gpu_num_dma_pages * sizeof(phys_addr_t*), GFP_KERNEL);
	for (i = 0; i < gpu_num_dma_pages; i++)
		gpu_phys[i] = src_addr + i * GB02MAC311;

	dma_info.cpu_pages = user_pages;
	dma_info.gpu_phys = gpu_phys;
	dma_info.num_dma_pages = num_dma_pages;
	dma_info.dma_dir = write;
	dma_info.last_page_size = size % PAGE_SIZE;
	ret = GB02FUNC1688(gbdev->dev, &dma_info, gbdev->gb_pcie);
	if (ret) {
		printk("%s %d dma failed! size:0x%llx,dir:%d\n",
			__func__, __LINE__,
			(long long)size, write);
		ret = -1;
		return ret;
	}
	mutex_unlock(&gbdev->mmu_as_lock);
	return 0;
}

#ifndef GB02MAC2039
#ifdef CONFIG_X86
#define __ttm_kmap_atomic_prot(__page, __prot) kmap_atomic_prot(__page, __prot)
#define __ttm_kunmap_atomic(__addr) kunmap_atomic(__addr)
#else
#define __ttm_kmap_atomic_prot(__page, __prot) vmap(&__page, 1, 0,  __prot)
#define __ttm_kunmap_atomic(__addr) vunmap(__addr)
#endif

void *GB02FUNC1448(struct page *page, pgprot_t prot)
{
	if (pgprot_val(prot) == pgprot_val(PAGE_KERNEL))
		return kmap_atomic(page);
	else
		return __ttm_kmap_atomic_prot(page, prot);
}

void GB02FUNC1451(void *addr, pgprot_t prot)
{
	if (pgprot_val(prot) == pgprot_val(PAGE_KERNEL))
		kunmap_atomic(addr);
	else
		__ttm_kunmap_atomic(addr);
}

static int GB02FUNC1452(struct ttm_tt *ttm, void *dst,
				unsigned long page,
				pgprot_t prot)
{
	struct page *s = ttm->pages[page];
	void *src;

	if (!s)
		return -ENOMEM;

	dst = (void *)((unsigned long)dst + (page << PAGE_SHIFT));
	src = GB02FUNC1448(s, prot);
	if (!src)
		return -ENOMEM;

	memcpy_toio(dst, src, PAGE_SIZE);
	GB02FUNC1451(src, prot);

	return 0;
}

static int GB02FUNC1457(struct ttm_tt *ttm, void *src,
				unsigned long page,
				pgprot_t prot)
{
	struct page *d = ttm->pages[page];
	void *dst;

	if (!d)
		return -ENOMEM;

	src = (void *)((unsigned long)src + (page << PAGE_SHIFT));

	dst = GB02FUNC1448(d, prot);

	if (!dst)
		return -ENOMEM;

	memcpy_fromio(dst, src, PAGE_SIZE);

	GB02FUNC1451(dst, prot);

	return 0;
}
#endif

static int GB02FUNC1459(void *dst, void *src, unsigned long page)
{
	uint32_t *dstP =
	    (uint32_t *) ((unsigned long)dst + (page << PAGE_SHIFT));
	uint32_t *srcP =
	    (uint32_t *) ((unsigned long)src + (page << PAGE_SHIFT));

	int i;
	for (i = 0; i < PAGE_SIZE / sizeof(uint32_t); ++i)
		iowrite32(ioread32(srcP++), dstP++);
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static void ttm_mem_reg_iounmap(struct ttm_bo_device *bdev, struct ttm_mem_reg *mem,
			 void *virtual)
#else
static void ttm_mem_reg_iounmap(struct ttm_device *bdev, struct ttm_resource *mem,
			 void *virtual)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	struct ttm_mem_type_manager *man;
	man = &bdev->man[mem->mem_type];
#else
	struct ttm_resource_manager *man;
	man = bdev->man_drv[mem->mem_type];
#endif

	if (virtual && mem->bus.addr == NULL)
		iounmap(virtual);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	(void) ttm_mem_io_lock(man, false);
#endif
	ttm_mem_io_free(bdev, mem);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	ttm_mem_io_unlock(man);
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
static void gb_ttm_tt_add_mapping(struct ttm_bo_device *bdev, struct ttm_tt *ttm)
#else
static void gb_ttm_tt_add_mapping(struct ttm_device *bdev, struct ttm_tt *ttm)
#endif
{
	pgoff_t i;

	if (ttm->page_flags & TTM_PAGE_FLAG_SG)
		return;

	for (i = 0; i < ttm->num_pages; ++i)
		ttm->pages[i]->mapping = bdev->dev_mapping;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 131)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
int gb_ttm_tt_populate(struct ttm_tt *ttm, struct ttm_operation_ctx *ctx)
#else
int gb_ttm_tt_populate(struct ttm_device *bdev, struct ttm_tt *ttm, struct ttm_operation_ctx *ctx)
#endif
{
	int ret;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	if (ttm->state != tt_unpopulated)
		return 0;
#else
	if  (ttm_tt_is_populated(ttm))
		return 0;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	if (ttm->bdev->driver->ttm_tt_populate)
		ret = ttm->bdev->driver->ttm_tt_populate(ttm, ctx);
	else
		ret = ttm_pool_populate(ttm, ctx);
#else
	if (bdev->funcs->ttm_tt_populate)
		ret = bdev->funcs->ttm_tt_populate(bdev, ttm, ctx);
	else
		ret = ttm_pool_alloc(&bdev->pool, ttm, ctx);
#endif

	if (!ret)
		gb_ttm_tt_add_mapping(bdev, ttm);

	return ret;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
void gbgpu_ttm_tt_unbind(struct ttm_tt *ttm)
{
	int ret;

	if (ttm->state == tt_bound) {
		ret = ttm->func->unbind(ttm);
		BUG_ON(ret);
		ttm->state = tt_unbound;
	}
}
#endif

static void GB02FUNC1463(struct ttm_tt *ttm)
{
	pgoff_t i;
	struct page **page = ttm->pages;

	if (ttm->page_flags & TTM_PAGE_FLAG_SG)
		return;

	for (i = 0; i < ttm->num_pages; ++i) {
		(*page)->mapping = NULL;
		(*page++)->index = 0;
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
void gb_ttm_tt_unpopulate(struct ttm_tt *ttm)
{
	if (ttm->state == tt_unpopulated)
		return;

	GB02FUNC1463(ttm);
	if (ttm->bdev->driver->ttm_tt_unpopulate)
		ttm->bdev->driver->ttm_tt_unpopulate(ttm);
	else
		ttm_pool_unpopulate(ttm);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
void ttm_tt_destroy(struct ttm_tt *ttm)
{
	if (ttm == NULL)
		return;

	gbgpu_ttm_tt_unbind(ttm);

	if (ttm->state == tt_unbound)
		gb_ttm_tt_unpopulate(ttm);

	if (!(ttm->page_flags & TTM_PAGE_FLAG_PERSISTENT_SWAP) &&
	    ttm->swap_storage)
		fput(ttm->swap_storage);

	ttm->swap_storage = NULL;
	ttm->func->destroy(ttm);
}
#else
void ttm_tt_destroy(struct ttm_device *bdev, struct ttm_tt *ttm)
{
	bdev->funcs->ttm_tt_destroy(bdev, ttm);
}
#endif

#if LINUX_VERSION_CODE == KERNEL_VERSION(4, 4, 131)
int GB02FUNC1467(struct ttm_buffer_object *bo,
#if (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_LOONGSON_OS)
			bool interruptible,
#else
			bool evict, bool interruptible,
#endif
			bool no_wait_gpu,
			struct ttm_mem_reg *new_mem)
#else
int GB02FUNC1467(struct ttm_buffer_object *bo,
			   struct ttm_operation_ctx *ctx,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
			   struct ttm_mem_reg *new_mem)
#else
			   struct ttm_resource *new_mem)
#endif
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	struct ttm_bo_device *bdev = bo->bdev;
	struct ttm_mem_type_manager *man = &bdev->man[new_mem->mem_type];
	struct ttm_tt *ttm = bo->ttm;
	struct ttm_mem_reg *old_mem = &bo->mem;
	struct ttm_mem_reg *old_copy = old_mem;
#else
	struct ttm_device *bdev = bo->bdev;
	struct ttm_tt *ttm = bo->ttm;
	struct ttm_resource *old_mem = bo->resource;
	struct ttm_resource *old_copy = old_mem;
	struct ttm_resource_manager *dst_man =
		ttm_manager_type(bo->bdev, new_mem->mem_type);
#endif

#ifdef GB02MAC2039
	struct GB02STR50 *ttm_bo = ttm_ob_to_ttm_bo(bo);
	struct GB02STR59 *gb_bo = GB02FUNC205(ttm_bo);
	struct GB02STR39 *gbdev = gb_bo->gbdev;
#endif
	void *old_iomap;
	void *new_iomap;
	int ret;
	unsigned long i;
	unsigned long page;
	unsigned long add = 0;
	int dir;

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
	if (ttm && ((ttm->page_flags & TTM_PAGE_FLAG_SWAPPED) ||
		    dst_man->use_tt)) {
		ret = ttm_tt_populate(bdev, ttm, ctx);
		if (ret)
			return ret;
	}
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
	ret = ttm_bo_wait(bo, ctx->interruptible, ctx->no_wait_gpu);
	if (ret)
		return ret;
#endif
	ret = gb_ttm_mem_reg_ioremap(bdev, old_mem, &old_iomap);
	if (ret)
		return ret;
	ret = gb_ttm_mem_reg_ioremap(bdev, new_mem, &new_iomap);
	if (ret)
		goto out;

	/*
	 * Single TTM move. NOP.
	 */
	if (old_iomap == NULL && new_iomap == NULL)
		goto out2;

	/*
	 * Don't move nonexistent data. Clear destination instead.
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	if (old_iomap == NULL &&
		(ttm == NULL || (ttm->state == tt_unpopulated &&
				 !(ttm->page_flags & TTM_PAGE_FLAG_SWAPPED)))) {
		memset_io(new_iomap, 0, new_mem->num_pages*PAGE_SIZE);
		goto out2;
	}
#endif
	/*
	 * TTM might be null for moves within the same region.
	 */
	if (ttm) {
#if LINUX_VERSION_CODE == KERNEL_VERSION(4, 4, 131)
		ret = ttm_pool_populate(ttm);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
		ret = gb_ttm_tt_populate(bdev, ttm, ctx);
#else
		ret = gb_ttm_tt_populate(ttm, ctx);
#endif
		if (ret)
			goto out1;
	}

	add = 0;
	dir = 1;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	if ((old_mem->mem_type == new_mem->mem_type) &&
		(new_mem->start < old_mem->start + old_mem->size)) {
		dir = -1;
		add = new_mem->num_pages - 1;
	}
#else
	if ((old_mem->mem_type == new_mem->mem_type) &&
		(new_mem->start < old_mem->start + (old_mem->num_pages * PAGE_SIZE))) {
		dir = -1;
		add = new_mem->num_pages - 1;
	}
#endif

#ifdef GB02MAC2039
	if (old_iomap == NULL)
		ret = GB02FUNC1444(gbdev, new_mem, ttm, new_mem->num_pages, 0);
	else if (new_iomap == NULL)
		ret = GB02FUNC1444(gbdev, old_mem, ttm, new_mem->num_pages, 1);
	else {
		for (i = 0; i < new_mem->num_pages; ++i) {
			page = i * dir + add;
			ret = GB02FUNC1459(new_iomap, old_iomap, page);
		}
	}
	if (ret)
		goto out1;
#else
	for (i = 0; i < new_mem->num_pages; ++i) {
		page = i * dir + add;
		if (old_iomap == NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
			pgprot_t prot = ttm_io_prot(old_mem->placement,
							PAGE_KERNEL);
#else
			pgprot_t prot = ttm_io_prot(bo, bo->resource, PAGE_KERNEL);
#endif
			ret = GB02FUNC1452(ttm, new_iomap, page,
						   prot);
		} else if (new_iomap == NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
			pgprot_t prot = ttm_io_prot(new_mem->placement,
							PAGE_KERNEL);
#else
			pgprot_t prot = ttm_io_prot(bo, bo->resource, PAGE_KERNEL);
#endif
			ret = GB02FUNC1457(ttm, old_iomap, page,
						   prot);
		} else {
			ret = GB02FUNC1459(new_iomap, old_iomap, page);
		}
		if (ret)
			goto out1;
	}
#endif
	mb();
out2:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	*old_copy = *old_mem;
	*old_mem = *new_mem;
	new_mem->mm_node = NULL;
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	if (man->flags & TTM_MEMTYPE_FLAG_FIXED) {
		ttm_tt_destroy(ttm);
		bo->ttm = NULL;
	}
#endif

out1:
	ttm_mem_reg_iounmap(bdev, old_mem, new_iomap);
out:
	ttm_mem_reg_iounmap(bdev, old_copy, old_iomap);

	/*
	 * On error, keep the mm node!
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	if (!ret)
		ttm_bo_mem_put(bo, old_copy);
#else
	if (!ret)
		ttm_bo_move_sync_cleanup(bo, new_mem);
#endif
	return ret;
}
#endif

#if (defined CONFIG_CENTOS && !(defined SYS_CENTOS7_COMPILE_ENV)) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
static int GB02FUNC1475(struct ttm_buffer_object *bo,
		bool evict,
		struct ttm_operation_ctx *ctx,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
		struct ttm_mem_reg *new_mem)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
		struct ttm_resource *new_mem)
#else
		struct ttm_resource *new_mem, struct ttm_place *hop)
#endif
{
	int ret = 0;
	struct GB02STR50 *ttm_bo = ttm_ob_to_ttm_bo(bo);
	struct GB02STR59 *gb_bo= ttm_bo->gb_bo;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct ttm_mem_reg *old_mem = &bo->mem;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	struct ttm_resource *old_mem = &bo->mem;
#else
	struct ttm_resource *old_mem = bo->resource;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	if (!old_mem || (old_mem->mem_type == TTM_PL_SYSTEM && !bo->ttm)) {
		ttm_bo_move_null(bo, new_mem);
		return 0;
	}
#endif

	if (gb_bo->gb_base.flags & GB02MAC942) {
		if (old_mem->mem_type == TTM_PL_SYSTEM && bo->ttm == NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
			gbgpu_move_null(bo, new_mem);
#else
			ttm_bo_move_null(bo, new_mem);
#endif
			return 0;
		}
	}

#ifdef GB_VRAM_MOVE_API
	ret = GB02FUNC1467(bo, ctx, new_mem);
#else
	ret = ttm_bo_move_memcpy(bo, ctx, new_mem);
#endif
	if (ret)
		return ret;

	return 0;
}
#else
static int GB02FUNC1475(struct ttm_buffer_object *bo,
		bool evict, bool interruptible,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
		bool no_wait_gpu, struct ttm_mem_reg *new_mem)
#else
		bool no_wait_gpu, struct resource *new_mem)
#endif
{
	int ret = 0;
	struct GB02STR50 *ttm_bo = ttm_ob_to_ttm_bo(bo);
	struct GB02STR59 *gb_bo= ttm_bo->gb_bo;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct ttm_mem_reg *old_mem = &bo->mem;
#else
	struct ttm_resource *old_mem = bo->resource;
#endif
	if (gb_bo->gb_base.flags & GB02MAC942) {
		if (old_mem->mem_type == TTM_PL_SYSTEM && bo->ttm == NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
			gbgpu_move_null(bo, new_mem);
#else
			ttm_bo_move_null(bo, new_mem);
#endif
			return 0;
		}
	}

#if (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_LOONGSON_OS)
#ifdef GB_VRAM_MOVE_API
		return GB02FUNC1467(bo, interruptible, no_wait_gpu, new_mem);
#else
		return ttm_bo_move_memcpy(bo, interruptible, no_wait_gpu, new_mem);
#endif
#else
#ifdef GB_VRAM_MOVE_API
		return GB02FUNC1467(bo, evict, interruptible,
			no_wait_gpu, new_mem);
#else
		return ttm_bo_move_memcpy(bo, evict, interruptible,
			no_wait_gpu, new_mem);
#endif
#endif
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static int gb_ttm_invalidate_caches(struct ttm_bo_device *bdev, uint32_t flags)
{
	gb_printf(KERN_INFO, "%s success\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_X86_64
static unsigned long GB02FUNC1477(struct ttm_buffer_object *bo,
					unsigned long page_offset)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	return ((bo->mem.bus.base + bo->mem.bus.offset) >> PAGE_SHIFT)
		+ page_offset;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
	return (bo->mem.bus.offset >> PAGE_SHIFT) + page_offset;
#else
	return (bo->resource->bus.offset >> PAGE_SHIFT) + page_offset;
#endif
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0))
int gb_ttm_tt_bind(struct ttm_bo_device *bdev,
		struct ttm_tt *ttm,
		struct ttm_resource *bo_mem)
{
	return 0;
}

void gb_ttm_tt_unbind(struct ttm_bo_device *bdev,
		struct ttm_tt *ttm)
{
	return;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
struct ttm_bo_driver gb_ttm_bo_driver = {
#else
struct ttm_device_funcs gb_ttm_bo_driver = {
#endif

	.ttm_tt_create = &gb_ttm_tt_create,
	.ttm_tt_populate = &gb_ttm_pool_populate,
	.ttm_tt_unpopulate = &gb_ttm_pool_unpopulate,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	.invalidate_caches = &gb_ttm_invalidate_caches,
	.init_mem_type = &gb_ttm_init_mem_type,
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	.ttm_tt_destroy	= &gb_ttm_backend_destroy,
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0))
	.ttm_tt_bind = &gb_ttm_tt_bind,
	.ttm_tt_unbind = &gb_ttm_tt_unbind,
#endif

#if (KERNEL_VERSION(4, 10, 0) <= LINUX_VERSION_CODE)
	.eviction_valuable = &ttm_bo_eviction_valuable,
#endif

	.evict_flags = &gb_ttm_bo_evict_flags,
	.move = &GB02FUNC1475,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0))
	.verify_access = &gb_ttm_bo_verify_access,
#endif

	.io_mem_reserve = &gb_ttm_io_mem_reserve,
	.io_mem_free = &gb_ttm_io_mem_free,
#ifdef CONFIG_X86_64
	.io_mem_pfn = &GB02FUNC1477,
#else
//	.io_mem_pfn = GB02FUNC1477,
#endif

#if (defined CONFIG_CENTOS) || (defined SYS_CENTOS7_9_2009) || \
	(defined SYS_CENTOS7_COMPILE_ENV)
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	.lru_tail = &ttm_bo_default_lru_tail,
	.swap_lru_tail = &ttm_bo_default_swap_lru_tail,
#endif
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static int gb_ttm_init_vram(struct GB02STR175 *ttm,
		struct GB02STR253 *vram_config)
{
	int ret = 0;

	ret = gb_ttm_range_man_init(&ttm->bdev, TTM_PL_VRAM,
			(vram_config->vram_size - GB02MAC496) >> PAGE_SHIFT);
	return ret;

}
#endif

int GB02FUNC1480(struct drm_device *drm_dev,
	struct GB02STR253 *vram_config, struct GB02STR175 *ttm)
{
	int ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	bool need_dma32 = false;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	bool need_dma32 = true;
#endif

#if (!(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	defined SYS_CENTOS7_COMPILE_ENV) \
	|| (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0) && \
	!defined CONFIG_CENTOS_OS)
	ret = gb_ttm_global_init(ttm);
	if (ret)
		return ret;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	ret = ttm_bo_device_init(&ttm->bdev,
#if (!(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	defined SYS_CENTOS7_COMPILE_ENV) \
	|| (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0) && \
	!defined CONFIG_CENTOS_OS)
				 ttm->bo_global_ref.ref.object,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
				 drm_dev->dev,
#endif
				 &gb_ttm_bo_driver,
				 drm_dev->anon_inode->i_mapping,
#if !(defined CONFIG_CENTOS && !defined SYS_CENTOS7_COMPILE_ENV) \
	&& LINUX_VERSION_CODE <= KERNEL_VERSION(5, 1, 21)
				 GB02MAC2038,
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
				 drm_dev->vma_offset_manager,
#endif
				 need_dma32);
#else	/* linux_version_code >= 5.13.0 */
	ret = ttm_device_init(&ttm->bdev,
				 &gb_ttm_bo_driver, drm_dev->dev,
				 drm_dev->anon_inode->i_mapping,
				 drm_dev->vma_offset_manager, false,
				 false);
#endif
	if (ret) {
		DRM_ERROR("Error initialising bo driver; %d\n", ret);
		return ret;
	}
	gb_printf(KERN_INFO, "%s vram_size: 0x%llx reserve_size:0x%llx gpu_start:0x%llx\n",
		__func__, (u64)vram_config->vram_size,
		(u64)GB02MAC496,
		(u64)GB02MAC500);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	ret = ttm_bo_init_mm(&ttm->bdev, TTM_PL_VRAM,
			(vram_config->vram_size - GB02MAC496) >> PAGE_SHIFT);
#else
	ret = gb_ttm_init_vram(ttm, vram_config);
#endif
	if (ret) {
		DRM_ERROR("Failed ttm VRAM init: %d\n", ret);
		return ret;
	}
//	GB02FUNC1771(ttm, GB02MAC488 >> PAGE_SHIFT);
	ttm->initialized = true;
	return 0;
}

void GB02FUNC1482(struct GB02STR176	*gb_mm)
{
	struct GB02STR175 *ttm = &gb_mm->ttm;

	if (!ttm->initialized)
		return;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	ttm_bo_device_release(&ttm->bdev);
#else
	ttm_device_fini(&ttm->bdev);
#endif
#if (!(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	defined SYS_CENTOS7_COMPILE_ENV) \
	|| (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0) && \
	!defined CONFIG_CENTOS_OS)
	GB02FUNC1398(ttm);
#endif
	ttm->initialized = false;
}

u64 GB02FUNC1484(struct GB02STR50 *bo)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
	return bo->bo.offset;
#else
	return bo->offset;
#endif
}

int GB02FUNC1485(struct GB02STR50 *bo, u32 pl_flag, u64 *gpu_addr)
{
	int ret;
#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	struct ttm_operation_ctx ctx = { false, false };
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
	if (bo->pin_count) {
		bo->pin_count++;
#else
	if (bo->bo.pin_count) {
		ttm_bo_pin(&bo->bo);
#endif
		if (gpu_addr)
			*gpu_addr = GB02FUNC1484(bo);

		return 0;
	}

	GB02FUNC1386(bo, pl_flag);

#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	ret = ttm_bo_validate(&bo->bo, &bo->placement, &ctx);
#else
	ret = ttm_bo_validate(&bo->bo, &bo->placement, false, false);
#endif
	if (ret)
		return ret;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
	bo->pin_count = 1;
#else
	ttm_bo_pin(&bo->bo);
#endif
	if (gpu_addr)
		*gpu_addr = GB02FUNC1484(bo);
	gb_printf(KERN_DEBUG, "---inside %s finish\n", __func__);

	return 0;
}

int GB02FUNC1487(struct GB02STR50 *bo)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
	int ret;
#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	struct ttm_operation_ctx ctx = { false, false };
#endif

	if (!bo->pin_count) {
		DRM_ERROR("unpin bad %p\n", bo);
		return 0;
	}
	bo->pin_count--;
	if (bo->pin_count)
		return 0;

#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	ret = ttm_bo_validate(&bo->bo, &bo->placement, &ctx);
#else
	ret = ttm_bo_validate(&bo->bo, &bo->placement, false, false);
#endif
	if (ret)
		return ret;

#else	/* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0) */
	ttm_bo_unpin(&bo->bo);
#endif
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) 
static size_t ttm_bo_acc_size(struct ttm_bo_device *bdev, unsigned long bo_size, unsigned struct_size)
{
	unsigned npages = (PAGE_ALIGN(bo_size)) >> PAGE_SHIFT;
	size_t size = 0;

	size += ttm_round_pot(struct_size);
	size += ttm_round_pot(npages * sizeof(void *));
	size += ttm_round_pot(sizeof(struct ttm_tt));
	
	return size;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
size_t gb_ttm_bo_acc_size(struct GB02STR50 *ttm_bo, size_t size)
{
	size_t acc_size;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
	acc_size = ttm_bo_acc_size(ttm_bo->bo.bdev, size,
			sizeof(struct GB02STR56));
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	acc_size = ttm_bo_dma_acc_size(ttm_bo->bo.bdev, size,
			sizeof(struct GB02STR56));
#endif

	if (acc_size)
		return acc_size;

	return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
uint64_t gb_ttm_domain_start(uint32_t type)
{
	switch (type) {
	case TTM_PL_VRAM:
		return GB02MAC500;
	case GB02MAC2677:
		return GB02MAC499;
	}
	return 0;
}
#endif
static u64 GB02FUNC1491(struct drm_printer *p, const struct drm_mm_node *entry)
{
	u64 start, size;

	size = entry->hole_size;
	if (size) {
		start = drm_mm_hole_node_start(entry);
		/*
		drm_printf(p, "%#018llx-%#018llx: %llu: free\n",
			   start, start + size, size);
			   */
	}

	return size;
}
void GB02FUNC1492(const struct drm_mm *mm, struct drm_printer *p,int mem_type)
{

	struct gpu_info *gpu_info = GB02FUNC314();
	const struct drm_mm_node *entry;
	u64 total_used = 0, total_free = 0, total = 0;

	total_free += GB02FUNC1491(p, &mm->head_node);

	drm_mm_for_each_node(entry, mm) {
		/*
		drm_printf(p, "%#018llx-%#018llx: %llu: used\n", entry->start,
			   entry->start + entry->size, entry->size);
		*/
		total_used += entry->size;
		total_free += GB02FUNC1491(p, entry);
	}
	total = total_free + total_used;
	if (mem_type == TTM_PL_VRAM) {
		gpu_info->gpu_used =
			(total_used * PAGE_SIZE) / (1024 * 1024);
		gpu_info->gpu_total =
			(total * PAGE_SIZE) / (1024 * 1024);
	} else if (mem_type == TTM_PL_PRIV) {
		gpu_info->vpu_used =
			(total_used * PAGE_SIZE) / (1024 * 1024);
		gpu_info->vpu_total =
			(total * PAGE_SIZE) / (1024 * 1024);
	}

	/*
	drm_printf(p, "total: %llu, used %llu free %llu\n", total,
		   total_used, total_free);
	*/
}
int GB02FUNC1495(int mem_type)
{
	struct drm_printer p = drm_debug_printer("gb_mem_info:");
	struct GB02STR70 *pcie_dev = GB02FUNC518();
	struct GB02STR39 *gl_gbdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct ttm_mem_type_manager *man;
#else
	struct ttm_resource_manager *man;
#endif
	struct GB02STR177 *rman;

	unsigned long flags;
	gl_gbdev = pcie_dev->gbdev;
	if (!gl_gbdev) {
		gb_printf(KERN_ERR, "%s:gl_gbdev is NULL\n",__func__);
		return -EINVAL;
	}
	mdelay(10);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	man = &gl_gbdev->gb_mm.ttm.bdev.man[mem_type];
#else
	man = gl_gbdev->gb_mm.ttm.bdev.man_drv[mem_type];
#endif
	if (mem_type != TTM_PL_SYSTEM) {
		if(man == NULL) {
			gb_printf(KERN_ERR, "%s:man is NULL\n",__func__);
			return -EINVAL;
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
		rman = (struct GB02STR177 *)man->priv;
#else
		rman = to_range_manager(man);
#endif
		spin_lock_irqsave(&rman->lock, flags);
		GB02FUNC1492(&rman->mm, &p, mem_type);
		spin_unlock_irqrestore(&rman->lock, flags);
	}
	return 0;
}

u64 GB02FUNC1498(int init_domain)
{
	int mem_type;
	u64 total_free = 0;
	unsigned long flags;
	struct drm_printer p = drm_debug_printer("gb_mem_info:");
	struct GB02STR70 *pcie_dev = GB02FUNC518();
	struct GB02STR39 *gl_gbdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct ttm_mem_type_manager *man;
#else
	struct ttm_resource_manager *man;
#endif
	struct GB02STR177 *rman;
	const struct drm_mm_node *entry;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	if (init_domain & TTM_PL_FLAG_VRAM) {
		mem_type = TTM_PL_VRAM;
	} else if (init_domain & GB02MAC2678) {
		mem_type = GB02MAC2677;
	} else {
		mem_type = TTM_PL_SYSTEM;
	}
#else
	mem_type = init_domain;
	if (init_domain == GB02MAC2678)
		mem_type = GB02MAC2677;
#endif


	gl_gbdev = pcie_dev->gbdev;

	if (!gl_gbdev) {
		gb_printf(KERN_ERR, "%s:gl_gbdev is NULL\n", __func__);
		return -EINVAL;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	man = &gl_gbdev->gb_mm.ttm.bdev.man[mem_type];
#else
	man = gl_gbdev->gb_mm.ttm.bdev.man_drv[mem_type];
#endif

	if (mem_type != TTM_PL_SYSTEM) {
		if(man == NULL) {
			gb_printf(KERN_ERR, "%s:man is NULL\n", __func__);
			return -EINVAL;
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
		rman = (struct GB02STR177 *)man->priv;
#else
		rman = to_range_manager(man);
#endif
		spin_lock_irqsave(&rman->lock, flags);
		total_free += (GB02FUNC1491(&p, &rman->mm.head_node) * PAGE_SIZE);
		drm_mm_for_each_node(entry, &rman->mm) {
			total_free += (GB02FUNC1491(&p, entry) * PAGE_SIZE);
		}
		spin_unlock_irqrestore(&rman->lock, flags);
	}
	return total_free;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0) 
static int gb_ttm_bo_init(struct ttm_device *bdev,
		struct ttm_buffer_object *bo,
		unsigned long size,
		enum ttm_bo_type type,
		struct ttm_placement *placement,
		uint32_t page_alignment,
		bool interruptible,
		struct sg_table *sg,
		struct dma_resv *resv,
		void (*destroy) (struct ttm_buffer_object *))
{
	struct ttm_operation_ctx ctx = { interruptible, false };
	int ret;

	ret = ttm_bo_init_reserved(bdev, bo, type, placement,
				   page_alignment, &ctx,
				   sg, resv, destroy);
	if (ret)
		return ret;

	if(!resv)
		ttm_bo_unreserve(bo);

	return 0;
}
#endif

int GB02FUNC1502(struct drm_device *dev, size_t size, int align,
			  int init_domain, uint32_t flags,
			  bool kernel, struct GB02STR50 *ttm_bo)
{
	struct GB02STR39 *gb_dev = (struct GB02STR39 *)dev->dev_private;
	struct GB02STR59 *gb_bo = NULL;
	struct GB02STR175 *ttm = &gb_dev->gb_mm.ttm;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
	struct ttm_mem_global *mem_glob = NULL;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	size_t acc_size;
#endif
	enum ttm_bo_type type;
	int sub_bo_flag;
	int ret = 0;
	//unsigned long page_align = roundup(align, PAGE_SIZE) >> PAGE_SHIFT;
	size = ALIGN(size, PAGE_SIZE);
	if (kernel)
		type = ttm_bo_type_kernel;
	else
		type = ttm_bo_type_device;

	sub_bo_flag = ttm_bo->sub_bo_flag;
	if (!sub_bo_flag) {
		gb_bo = GB02FUNC205(ttm_bo);
		ttm_bo->gb_bo = gb_bo;
	}

#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	drm_gem_private_object_init(dev, &ttm_bo->bo.base, size);
#else
	drm_gem_private_object_init(dev, &ttm_bo->base, size);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	ttm_bo->bo.base.funcs = &gb_gem_object_funcs;
#endif
	ttm_bo->bo.bdev = &ttm->bdev;
	ttm_bo->bo.bdev->dev_mapping = dev->anon_inode->i_mapping;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	GB02FUNC1386(ttm_bo, TTM_PL_FLAG_VRAM);
#else
	GB02FUNC1386(ttm_bo, TTM_PL_VRAM);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
	if (ttm->bdev.glob == NULL) {
		gb_printf(KERN_ERR, "------------bdev.glob == null\n");
		ret = -1;
		goto init_fail;
	}
#endif
	if ((flags & GB02MAC925) && (flags & GB02MAC928)) {
#if KERNEL_VERSION(5, 3, 0) > LINUX_VERSION_CODE
		ttm_bo->bo.resv = &ttm_bo->bo.ttm_resv;
		reservation_object_init(&ttm_bo->bo.ttm_resv);
#endif
		return ret;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	acc_size = gb_ttm_bo_acc_size(ttm_bo, size);
	if (!acc_size) {
		gb_printf(KERN_ERR, "%s gb get accsize faild 0x%lx\n",
				__func__, acc_size);
		ret = -1;
		goto init_fail;
	}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
	mem_glob = ttm->bdev.glob->mem_glob;
	if (mem_glob == NULL) {
		gb_printf(KERN_ERR, "mem_glob == null\n");
		ret = -1;
		goto init_fail;
	}
#endif

#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	ret = ttm_bo_init(&ttm->bdev, &ttm_bo->bo, size,
			  type, &ttm_bo->placement,
			  align >> PAGE_SHIFT, kernel, acc_size,
			  NULL, NULL, GB02FUNC1399);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0) 
	ret = gb_ttm_bo_init(&ttm->bdev, &ttm_bo->bo, size, type,
			     &ttm_bo->placement, align >> PAGE_SHIFT,
			     kernel, NULL, NULL, GB02FUNC1399);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
	ret = ttm_bo_init(&ttm->bdev, &ttm_bo->bo, size,
			  type, &ttm_bo->placement,
			  align >> PAGE_SHIFT, kernel, NULL, NULL,
			  GB02FUNC1399);
#else
	ret = ttm_bo_init(&ttm->bdev, &ttm_bo->bo, size,
			  type, &ttm_bo->placement,
			  align >> PAGE_SHIFT, kernel, NULL, acc_size,
			  NULL, NULL, GB02FUNC1399);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	if (ttm_bo->bo.resource)
		ttm_bo->offset = (ttm_bo->bo.resource->start << PAGE_SHIFT) +
			gb_ttm_domain_start(ttm_bo->bo.resource->mem_type);
	else
		ttm_bo->offset = 0;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	ttm_bo->offset = (ttm_bo->bo.mem.start << PAGE_SHIFT) +
			gb_ttm_domain_start(ttm_bo->bo.mem.mem_type);
#endif
	if (ret) {
		gb_printf(KERN_ERR, "%s:%d-ttm_bo_init fail--ret:%d\n",
			__func__, __LINE__,ret);
		return ret;
	}

	gb_printf(KERN_DEBUG, "%s create bo:0x%llx phy:0x%llx size:0x%llx init_domain:0x%llx success\n",
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
			__func__, (u64)&ttm_bo->bo, (u64)ttm_bo->bo.offset, (u64)size, (u64)init_domain);
#else
			__func__, (u64)&ttm_bo->bo, (u64)ttm_bo->offset, (u64)size, (u64)init_domain);
#endif
	return ret;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
init_fail:
	if (gb_bo && !sub_bo_flag) {
		kvfree(gb_bo);
		gb_bo = NULL;
	}
	return ret;
#endif
}

void GB02FUNC1508(struct GB02STR50 *ttm_bo)
{
	struct ttm_buffer_object *tbo;
	struct GB02STR59 *gb_bo = NULL;

	if (ttm_bo == NULL)
		return;

	if (!ttm_bo->sub_bo_flag) {
		gb_bo = GB02FUNC205(ttm_bo);
	}

	tbo = &(ttm_bo->bo);

	gb_printf(KERN_DEBUG, "%s---bo:%llx, bo ref:%d gem_ref:%d, list kref =%d",
	__func__, (long long int)tbo, kref_read(&tbo->kref),
#if KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE && KERNEL_VERSION(5, 9, 0) > LINUX_VERSION_CODE
	kref_read(&tbo->base.refcount), kref_read(&tbo->list_kref));
#elif KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE
	kref_read(&tbo->base.refcount), kref_read(&tbo->kref));
#else
	kref_read(&ttm_bo->base.refcount), kref_read(&tbo->list_kref));
#endif
	if (gb_bo && gb_bo->is_heap_growable) {
#if KERNEL_VERSION(5, 3, 0) > LINUX_VERSION_CODE
		reservation_object_fini(&tbo->ttm_resv);
#endif
		GB02FUNC1399(tbo);
		return;
	}
#if (defined CONFIG_CENTOS && !(defined SYS_CENTOS7_COMPILE_ENV)) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	if (kref_read(&tbo->kref) == 1)
		tbo->pin_count = 0;
#endif
	ttm_bo_put(tbo);
#else
	ttm_bo_unref(&tbo);
#endif
	ttm_bo = NULL;
}

int GB02FUNC1513(struct GB02STR59 *bo, struct GB02STR39 *gbdev,
	struct GB02STR47 *gb_priv, u64 addr, int clear_intr, int as)
{
	u64 gpu_va = 0, vpfn;
	int ret;
	int nr_pages;
	int i;
	u64 bo_start, bo_end;
	u64 size = 0;
	phys_addr_t *pages;
	struct gb_heap_ttm_bo *gb_heap_bo = NULL;

	//需要考虑第一次和最后一次虚拟空间是否满足64m空间;
	bo_start = bo->base.iovaddr;
	bo_end = bo->base.iovaddr + bo->base.nr_pages * GB02MAC311;
	gpu_va = addr & ~(GB02MAC1192 - 1);
	size = GB02MAC1192;

	// when first addr not algin with 2M
	if (gpu_va <= bo_start) {
		gpu_va = bo_start;
		size = GB02MAC1192 - (gpu_va & (GB02MAC1192 - 1));
	}
	// when gpu_va to end addr not enough 2M
	if ((gpu_va + size) > bo_end)
		size = bo_end - gpu_va;
	vpfn = gpu_va >> GB02MAC313;
	nr_pages = size >> GB02MAC313;
	gb_heap_bo = kvzalloc(sizeof(struct gb_heap_ttm_bo), GFP_KERNEL);
	gb_heap_bo->ttm_bo.sub_bo_flag = 1;
	gb_heap_bo->ttm_bo.gb_bo = bo;

	//根据动态size创建物理空间
	ret = GB02FUNC1502(gbdev->ddev, size, 0, 4, 0, 0, &gb_heap_bo->ttm_bo);
	if (ret) {
		// 分配失败需要释放gb_heap_bo
		kvfree(gb_heap_bo);
		goto err_bo;
	}

	nr_pages = size / GB02MAC311;
	pages = kvzalloc(sizeof(phys_addr_t) * nr_pages, GFP_KERNEL);

	//记录物理地址到page
	for(i = 0; i < nr_pages; i++) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
		pages[i] = gb_heap_bo->ttm_bo.bo.offset + i * GB02MAC311;
#else
		pages[i] = gb_heap_bo->ttm_bo.offset + i * GB02MAC311;
#endif
	}

	ret = GB02FUNC767(gbdev->mmu_mode, vpfn, pages, nr_pages, 0,
				      gb_pg_dump_state & GB02MAC1234);
	if (ret) {
		gb_printf(KERN_ERR, "[%s] bo mmap pa 0x%llx to vaddr 0x%llx failed\n", __func__, pages[0], addr);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
		drm_gem_object_put_unlocked(&gb_heap_bo->ttm_bo.bo.base);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(&gb_heap_bo->ttm_bo.bo.base);
#else
		drm_gem_object_unreference_unlocked(&gb_heap_bo->ttm_bo.base);
#endif
		kvfree(pages);
		kvfree(gb_heap_bo);
		goto err_bo;
	}

	//一切成功，添加节点 并设置一些属性
	mutex_lock(&bo->base.pages_lock);
	gb_heap_bo->gb_va_start = gpu_va;
	gb_heap_bo->grow_nr_pages = nr_pages;
	gb_heap_bo->pages = pages;
	list_add(&gb_heap_bo->ttm_bo_list, &bo->gb_base.ttm_bo_root);
	mutex_unlock(&bo->base.pages_lock);

	if (clear_intr)
		mmu_write(gbdev, GB02MAC1542, BIT(as));

	ret = GB02FUNC923(gbdev, vpfn << GB02MAC313,
		nr_pages * GB02MAC311, GB02MAC1724);
	if (ret)
		gb_printf(KERN_ERR, "%s fail for mmu operation \n", __func__);

err_bo:
	return ret;
}


int GB02FUNC1516(struct file *filp, struct vm_area_struct *vma)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	struct drm_file *file_priv;
	struct GB02STR39 *gb_dev;
	struct GB02STR175 *ttm;
#else
	struct drm_gem_object *gem_obj;
#endif
	int ret = 0;

	gb_printf(KERN_DEBUG, "%s: vm_pgoff %lu\n", __func__, vma->vm_pgoff);

	if (unlikely(vma->vm_pgoff < GB02MAC2038)) {
		gb_printf(KERN_ERR, "%s: vm_pgoff %lu DRI_FILE_PAGE_OFFSET %llx\n",
		       __func__, vma->vm_pgoff, GB02MAC2038);
		return -EINVAL;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	file_priv = filp->private_data;
	gb_dev = (struct GB02STR39 *)file_priv->minor->dev->dev_private;
	ttm = &gb_dev->gb_mm.ttm;

	ret = ttm_bo_mmap(filp, vma, &ttm->bdev);
	if (ret) {
		gb_printf(KERN_ERR, "%s ttm_bo_mmap failed error: %d\n", __func__,
		       ret);
	}

	return ret;
#else
	gem_obj = vma->vm_private_data;

	drm_gem_object_get(gem_obj);

	ret = drm_gem_ttm_mmap(gem_obj, vma);
	if (ret) {
		gb_printf(KERN_ERR, "%s drm_gem_ttm_mmap failed error: %d\n", __func__,
		       ret);
	}

	return ret;
#endif
}

int GB02FUNC1518(struct GB02STR50 *ttm_bo, bool no_intr, int domain)
{
	int ret;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) || (defined CONFIG_X86_64) ||\
	(defined CONFIG_LOONGSON_OS) || (defined CONFIG_SKYLIN_OS_V10) ||\
	(defined CONFIG_CENTOS)
	ret = ttm_bo_reserve(&ttm_bo->bo, no_intr, false, NULL);
#else
	ret = ttm_bo_reserve(&ttm_bo->bo, no_intr, false, false, NULL);
#endif
	if (ret)
		return ret;

	ret = GB02FUNC1485(ttm_bo, domain, NULL);
	if (ret) {
		gb_printf(KERN_ERR, "failed to pin gb gem ttm bo\n");
		ttm_bo_unreserve(&ttm_bo->bo);
		return ret;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
	ret = ttm_bo_kmap(&ttm_bo->bo, 0, ttm_bo->bo.num_pages, &ttm_bo->kmap);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	ret = ttm_bo_kmap(&ttm_bo->bo, 0, ttm_bo->bo.resource->num_pages, &ttm_bo->kmap);
#else
	ret = ttm_bo_kmap(&ttm_bo->bo, 0, PFN_UP(ttm_bo->bo.base.size), &ttm_bo->kmap);
#endif
	if (ret) {
		gb_printf(KERN_ERR, "failed to kmap gb gem ttm bo\n");
		ttm_bo_unreserve(&ttm_bo->bo);
		return ret;
	}

	gb_printf(KERN_INFO, "%s gb ttm bo reserve \n", __func__);
	ttm_bo_unreserve(&ttm_bo->bo);

	return 0;

}

void GB02FUNC1522(struct GB02STR50 *ttm_bo)
{
	ttm_bo_unreserve(&ttm_bo->bo);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
struct ttm_resource *
genbu_ttm_resource_manager_first(struct ttm_resource_manager *man,
			   struct ttm_resource_cursor *cursor)
{
	struct ttm_resource *res;

	lockdep_assert_held(&man->bdev->lru_lock);

	for (cursor->priority = 0; cursor->priority < TTM_MAX_BO_PRIORITY;
	     ++cursor->priority)
		list_for_each_entry(res, &man->lru[cursor->priority], lru)
			return res;

	return NULL;
}

struct ttm_resource *
genbu_ttm_resource_manager_next(struct ttm_resource_manager *man,
			  struct ttm_resource_cursor *cursor,
			  struct ttm_resource *res)
{
	lockdep_assert_held(&man->bdev->lru_lock);

	list_for_each_entry_continue(res, &man->lru[cursor->priority], lru)
		return res;

	for (++cursor->priority; cursor->priority < TTM_MAX_BO_PRIORITY;
	     ++cursor->priority)
		list_for_each_entry(res, &man->lru[cursor->priority], lru)
			return res;

	return NULL;
}
#endif
