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

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pagemap.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#include <drm/drm_drv.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_syncobj.h>
#include <drm/drm_utils.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
#include <drm/drm_pci.h>
#endif
#include <drm/drm_auth.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_api.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#include <drm/ttm/ttm_placement.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_module.h>
#include <drm/ttm/ttm_page_alloc.h>
#else
#include <drm/ttm/ttm_range_manager.h>
#endif
#include <drm/gpu_scheduler.h>

#include "common/gb_uk.h"
#include "common/gb_bo.h"
#include "vpu/vpu_mm/gb_vpu_gem_comm.h"
#include "gpu/gb_ttm.h"
#include "gb_vpu_ttm.h"
#include "common/gb_common.h"

int GB02FUNC1771(struct GB02STR175 *ttm, unsigned long p_size)
{
	int r = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	/* Initialize various on-chip memory pools */
	r = ttm_bo_init_mm(&ttm->bdev, GB02MAC2677, p_size);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	r = ttm_range_man_init(&ttm->bdev, GB02MAC2677, false, p_size);
#else
	r = ttm_range_man_init(&ttm->bdev, GB02MAC2677, true, p_size);
#endif
	if (r)
		DRM_ERROR("Failed initializing GDS heap.\n");
	return r;
}
