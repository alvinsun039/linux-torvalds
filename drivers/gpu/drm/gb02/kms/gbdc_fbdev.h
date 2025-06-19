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
#include <drm/drm_fb_helper.h>
#ifndef		__GBDC_FBDEV_H__
#define		__GBDC_FBDEV_H__

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#include <drm/drm_framebuffer.h>
#endif

#define	GB02MAC1940	32
struct GB02STR159 {
	struct drm_framebuffer base;
	struct drm_gem_object *obj;
	u32 fb_id;
};

struct GB02STR160 {
	struct drm_fb_helper helper;
	struct GB02STR159 gfb;
	//struct gbgpu_framebuffer rfb;
	struct GB02STR155 *gbdev;
	int size;
	bool initialized;
};
#define to_gbdc_framebuffer(x) container_of(x, struct GB02STR159, base)
int GB02FUNC1223(struct drm_device *drm_dev, struct GB02STR160 *fbInfo, int num_crtc, int num_connector);
void GB02FUNC1227(struct GB02STR160 *fbInfo);

#endif
