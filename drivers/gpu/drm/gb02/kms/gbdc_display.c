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
#include <asm/div64.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_crtc.h>
#include <linux/pm_runtime.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_fb_helper.h>

#include <linux/io.h>
#include <linux/console.h>
#include <drm/drm_gem.h>
#include <video/videomode.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#include <drm/drm_edid.h>
//#include <drm/drm_atomic_helper.h>
#include <linux/gcd.h>
#include <generated/uapi/linux/version.h>
#include "gbdc_mode.h"
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_mm.h"
#include "gbdc_mode.h"
#include "gbdc_display.h"


void GB02FUNC1160(const struct drm_display_mode *dmode,
									struct videomode *vm)
{
	vm->hactive = dmode->hdisplay;
	vm->hfront_porch = dmode->hsync_start - dmode->hdisplay;
	vm->hsync_len = dmode->hsync_end - dmode->hsync_start;
	vm->hback_porch = dmode->htotal - dmode->hsync_end;

	vm->vactive = dmode->vdisplay;
	vm->vfront_porch = dmode->vsync_start - dmode->vdisplay;
	vm->vsync_len = dmode->vsync_end - dmode->vsync_start;
	vm->vback_porch = dmode->vtotal - dmode->vsync_end;

	vm->pixelclock = dmode->clock * 1000;

	vm->flags = 0;

	if (dmode->flags & DRM_MODE_FLAG_PHSYNC)
		vm->flags |= DISPLAY_FLAGS_HSYNC_HIGH;
	else if (dmode->flags & DRM_MODE_FLAG_NHSYNC)
		vm->flags |= DISPLAY_FLAGS_HSYNC_LOW;
	if (dmode->flags & DRM_MODE_FLAG_PVSYNC)
		vm->flags |= DISPLAY_FLAGS_VSYNC_HIGH;
	else if (dmode->flags & DRM_MODE_FLAG_NVSYNC)
		vm->flags |= DISPLAY_FLAGS_VSYNC_LOW;
	if (dmode->flags & DRM_MODE_FLAG_INTERLACE)
		vm->flags |= DISPLAY_FLAGS_INTERLACED;
	if (dmode->flags & DRM_MODE_FLAG_DBLSCAN)
		vm->flags |= DISPLAY_FLAGS_DOUBLESCAN;
#if 0
	if (dmode->flags & DRM_MODE_FLAG_DBLCLK)
		vm->flags |= DISPLAY_FLAGS_DOUBLECLK;
#endif
}

/* we can also use this function, whitch is the new interface of Linux kernel
 * it's better
#if (KERNEL_VERSION(5, 8, 0) <= LINUX_VERSION_CODE)
struct drm_framebuffer *GB02FUNC1169(
		struct drm_device *dev, struct drm_file *filp,
		const struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct GB02STR159 *gbdc_fb;
	int ret;

	gbdc_fb = kzalloc(sizeof(*gbdc_fb), GFP_KERNEL);
	if (!gbdc_fb)
		return ERR_PTR(-ENOMEM);

	ret = GB02FUNC1794(dev, filp, gbdc_fb, mode_cmd);
	if (ret) {
		kfree(gbdc_fb);
		return ERR_PTR(ret);
	}

	gbdc_fb->obj = gbdc_fb->base.obj[0];
	gbdc_fb->fb_id = gbdc_fb->base.base.id;

	return &gbdc_fb->base;
}
#else
*/
struct drm_framebuffer *GB02FUNC1169(
		struct drm_device *dev, struct drm_file *filp,
		const struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct drm_gem_object *obj;
	struct GB02STR159 *gbdc_fb;
	int ret;

	DRM_DEBUG_DRIVER("%dx%d, format %c%c%c%c\n",
					 mode_cmd->width, mode_cmd->height,
					 (mode_cmd->pixel_format) & 0xff,
					 (mode_cmd->pixel_format >> 8) & 0xff,
					 (mode_cmd->pixel_format >> 16) & 0xff,
					 (mode_cmd->pixel_format >> 24) & 0xff);
	gb_printf(KERN_DEBUG, "mode_cmd->pixel_format: %x\n",
		   mode_cmd->pixel_format);
	gb_printf(KERN_DEBUG, "FORMAT_XRGB8888: %x\n", DRM_FORMAT_XRGB8888);
	if ((mode_cmd->pixel_format != DRM_FORMAT_XRGB8888) &&
	 (mode_cmd->pixel_format != DRM_FORMAT_ARGB8888) &&
	 (mode_cmd->pixel_format != DRM_FORMAT_NV12))
		return ERR_PTR(-ENOENT);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) || (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS)\
	|| (defined CONFIG_SKYLIN_OS_V10) ||(defined CONFIG_CENTOS)
	obj = drm_gem_object_lookup(filp, mode_cmd->handles[0]);
#else
	obj = drm_gem_object_lookup(dev, filp, mode_cmd->handles[0]);
#endif
	if (obj == NULL)
		return ERR_PTR(-ENOENT);

	gbdc_fb = kzalloc(sizeof(*gbdc_fb), GFP_KERNEL);
	if (!gbdc_fb) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)) || \
	(defined CONFIG_CENTOS) || (defined SYS_CENTOS7_9_2009)
		drm_gem_object_put_unlocked(obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
		drm_gem_object_put(obj);
#else
		drm_gem_object_unreference_unlocked(obj);
#endif
		return ERR_PTR(-ENOMEM);
	}

	ret = GB02FUNC1790(dev, gbdc_fb, mode_cmd, obj);
	if (ret) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)) || \
	(defined CONFIG_CENTOS) || (defined SYS_CENTOS7_9_2009)
		drm_gem_object_put_unlocked(obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
		drm_gem_object_put(obj);
#else
		drm_gem_object_unreference_unlocked(obj);
#endif
		kfree(gbdc_fb);
		return ERR_PTR(ret);
	}
	gbdc_fb->fb_id = gbdc_fb->base.base.id;
	return &gbdc_fb->base;
}
/*#endif*/
