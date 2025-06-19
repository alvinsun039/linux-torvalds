/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * jessy 20221119 Sietium
 */
#ifndef __GB_VPU_TTM_H__
#define __GB_VPU_TTM_H__
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#include <drm/gpu_scheduler.h>
#include "kms/gbdc_mm.h"
#include "gpu/gb_ttm.h"
#define	GB02MAC2677	(TTM_PL_PRIV + 0)
#define	GB02MAC2678	(1 << GB02MAC2677)
#define	GB02MAC2679	(1 << GB02MAC2677)

struct GB02STR204 {
#if (!(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	defined SYS_CENTOS7_COMPILE_ENV) \
	|| (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0) && \
	!defined CONFIG_CENTOS_OS)
	struct drm_global_reference mem_global_ref;
	struct ttm_bo_global_ref bo_global_ref;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	struct ttm_bo_device bdev;
#else
	struct ttm_device bdev;
#endif
	bool initialized;
};

//#define GB02MAC499	(22*GB)
//#define VPU_VRM_LEN		(8 * 1024*MB)

int GB02FUNC1771(struct GB02STR175 *ttm,
		unsigned long p_size);


#endif

