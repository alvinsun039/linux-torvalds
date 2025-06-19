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

#include <linux/io.h>
#include <linux/list.h>
#include <linux/slab.h>

#include <drm/drm_cache.h>
#include <drm/drm_prime.h>
#include <drm/radeon_drm.h>
#include "gb_vpu_ttm.h"
#include "gb_vpu_gem_comm.h"
#include "gb_vpu_gem.h"
#include "gpu/gb_device.h"
#include "gb_vpu_object.h"
/*
 * To exclude mutual BO access we rely on bo_reserve exclusion, as all
 * function are calling it.
 */

__attribute__((unused)) static void gb_update_memory_usage(struct GB02STR201 *bo,
				       unsigned mem_type, int sign)
{
	struct GB02STR39 *gbdev = bo->gbdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
	u64 size = (u64)bo->tbo.num_pages << PAGE_SHIFT;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	u64 size = (u64)bo->tbo.ttm->num_pages << PAGE_SHIFT;
#else
	u64 size = (u64)bo->tbo.base.size;
#endif

	switch (mem_type) {
	case TTM_PL_VRAM:
		if (sign > 0)
			atomic64_add(size, &gbdev->vram_usage);
		else
			atomic64_sub(size, &gbdev->vram_usage);
		break;
	}
}
void GB02FUNC1757(struct ttm_buffer_object *tbo)
{
	if ((tbo) == NULL)
		return;

#if (defined CONFIG_CENTOS && !(defined SYS_CENTOS7_COMPILE_ENV)) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	ttm_bo_put(tbo);
#else
	ttm_bo_unref(&tbo);
#endif
}

int GB02FUNC1758(struct GB02STR56 *bo, void **ptr)
{
	bool is_iomem;
	struct GB02STR50 *ttm_bo;
	int r;

	ttm_bo = &bo->ttm_bo;

	if (bo->kptr) {
		if (ptr)
			*ptr = bo->kptr;

		return 0;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
	r = ttm_bo_kmap(&ttm_bo->bo, 0, ttm_bo->bo.num_pages, &ttm_bo->kmap);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	r = ttm_bo_kmap(&ttm_bo->bo, 0, ttm_bo->bo.resource->num_pages, &ttm_bo->kmap);
#else
	r = ttm_bo_kmap(&ttm_bo->bo, 0, PFN_UP(ttm_bo->bo.base.size), &ttm_bo->kmap);
#endif
	if (r)
		return r;

	bo->kptr = ttm_kmap_obj_virtual(&ttm_bo->kmap, &is_iomem);
	if (ptr)
		*ptr = bo->kptr;

	return 0;
}

void GB02FUNC1760(struct GB02STR201 *bo)
{
	if (bo->kptr == NULL)
		return;
	bo->kptr = NULL;
	ttm_bo_kunmap(&bo->kmap);
}
