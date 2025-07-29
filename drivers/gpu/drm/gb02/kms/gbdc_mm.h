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

#include <linux/delay.h>
#include <linux/version.h>
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gpu/gb_gem.h"
#include "common/gb_bo.h"
#ifndef		__GBDC_MM_H__
#define		__GBDC_MM_H__

int GB02FUNC1779(struct drm_file *file, struct drm_device *dev,
		     struct drm_mode_create_dumb *args);
int GB02FUNC1783(struct drm_file *file, struct drm_device *dev,
			  uint32_t handle, uint64_t * offset);
int GB02FUNC1790(struct drm_device *dev,
			  struct GB02STR159 *gfb,
			  const struct drm_mode_fb_cmd2 *mode_cmd,
			  struct drm_gem_object *obj);
int GB02FUNC1794(struct drm_device *dev,
				struct drm_file *filp,
				struct GB02STR159 *gfb,
				const struct drm_mode_fb_cmd2 *mode_cmd);
int GB02FUNC1766(struct drm_device *dev, u32 size, bool iskernel,
		    struct drm_gem_object **obj);
#endif
