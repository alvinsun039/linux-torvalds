#ifndef __GB_VPU_GEM_COMM_H__
#define __GB_VPU_GEM_COMM_H__
#include <drm/ttm/ttm_placement.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_api.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#include <linux/mutex.h>
/*
 * GEM objects.
 */
struct GB02STR200 {
	struct mutex		mutex;
	struct list_head	objects;
};

struct GB02STR201 {
	/* Protected by gem.mutex */
	struct list_head		list;
	/* Protected by tbo.reserved */
	u32				initial_domain;
	struct ttm_place		placements[4];
	struct ttm_placement		placement;
	struct ttm_buffer_object	tbo;
	struct ttm_bo_kmap_obj		kmap;
	u32				flags;
	unsigned			pin_count;
	void				*kptr;
	u32				tiling_flags;
	u32				pitch;
	int				surface_reg;
	unsigned			prime_shared_count;
	/* Constant after initialization */
	struct GB02STR39		*gbdev;
	struct list_head		va;
};

#define gem_to_gb_bo(gobj) container_of((gobj), struct GB02STR201, tbo.base)
#if 0
static inline struct GB02STR39 *GB02FUNC1748(struct ttm_bo_device *bdev)
{
	return container_of(bdev, struct GB02STR39, mman.bdev);
}
#endif
#endif
