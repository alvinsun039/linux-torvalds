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
#include <linux/io.h>
#include <linux/console.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
// #include <drm/drm_encoder.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
#include <drm/ttm/ttm_device.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#include <linux/vga_switcheroo.h>
#include <generated/uapi/linux/version.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_vma_manager.h>
#include "gbdc_mode.h"
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "gbdc_drv.h"
#include "gbdc_planes.h"
#include "gbdc_crtc.h"
#include "gbdc_mode.h"
#include "gbdc_display.h"
#include "gpu/gb_ttm.h"
#include "vpu/vpu_mm/gb_vpu_ttm.h"


/* ---------------------------------------------------------------------- */

static int GB02FUNC1214(struct fb_info *info, struct vm_area_struct *vma)
{
	struct drm_fb_helper *fb_helper = info->par;
	struct GB02STR160 *gb_fbdev = container_of(fb_helper, struct GB02STR160, helper);
	struct GB02STR56 *bo = GB02FUNC217(gb_fbdev->gfb.obj);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
	return ttm_fbdev_mmap(vma, &bo->ttm_bo.bo);
#else
	return ttm_bo_mmap_obj(vma, &bo->ttm_bo.bo);
#endif

}

static struct fb_ops gbdcfb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = drm_fb_helper_check_var,
	.fb_set_par = drm_fb_helper_set_par,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	.fb_fillrect = drm_fb_helper_cfb_fillrect,
	.fb_copyarea = drm_fb_helper_cfb_copyarea,
	.fb_imageblit = drm_fb_helper_cfb_imageblit,
#else
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
#endif
	.fb_pan_display = drm_fb_helper_pan_display,
	.fb_blank = drm_fb_helper_blank,
	.fb_setcmap = drm_fb_helper_setcmap,
	.fb_mmap = GB02FUNC1214,
};

static int GB02FUNC1216(struct drm_device *dev,
				const struct drm_mode_fb_cmd2 *mode_cmd,
				struct drm_gem_object **gobj_p)
{
	struct drm_gem_object *gobj = NULL;
	u32 size;
	int ret = 0;

	size = mode_cmd->pitches[0] * mode_cmd->height;
	gb_printf(KERN_INFO, "inside %s pitches[0]: 0x%x height: 0x%x\n",
			__func__, mode_cmd->pitches[0], mode_cmd->height);
	ret = GB02FUNC1766(dev, size, true, &gobj);
	if (ret)
		return ret;

	*gobj_p = gobj;
	return ret;
}

static int GB02FUNC1217(struct drm_fb_helper *helper,
			 struct drm_fb_helper_surface_size *sizes)
{
	struct fb_info *info;
	struct drm_framebuffer *fb;
	struct drm_mode_fb_cmd2 mode_cmd;
	struct drm_gem_object *gobj = NULL;
	struct GB02STR56 *bo = NULL;
	int size, ret;
	struct GB02STR155 *gbdc =
	    container_of(helper, struct GB02STR155, kms_info.fb_info.helper);
	struct GB02STR160 *fbInfo = &gbdc->kms_info.fb_info;

	if (sizes->surface_bpp != 32)
		return -EINVAL;

	gb_printf(KERN_INFO, "%s: gbdc addr : %llx\n",
		__func__, (long long int)gbdc);
	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;
	mode_cmd.pitches[0] = mode_cmd.width * ((sizes->surface_bpp + 7) / 8);
	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
							  sizes->surface_depth);
	size = mode_cmd.pitches[0] * mode_cmd.height;

	/* alloc, pin & map bo */
	ret = GB02FUNC1216(gbdc->drm_dev, &mode_cmd, &gobj);
	if (ret) {
		DRM_ERROR("failed to create fbcon backing object %d\n", ret);
		return ret;
	}

	bo = GB02FUNC217(gobj);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	ret = GB02FUNC1518(&bo->ttm_bo, true, TTM_PL_FLAG_VRAM);
#else
	ret = GB02FUNC1518(&bo->ttm_bo, true, TTM_PL_VRAM);
#endif

	if (ret) {
		gb_printf(KERN_ERR, "%s ttm bo reserve faild ret:%d\n", __func__, ret);
		return ret;
	}

	/* init fb device */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	info = drm_fb_helper_alloc_fbi(helper);
#else
	info = drm_fb_helper_alloc_info(helper);
#endif
	if (IS_ERR(info))
		return PTR_ERR(info);

	info->par = &fbInfo->helper;

	ret = GB02FUNC1790(gbdc->drm_dev, &fbInfo->gfb, &mode_cmd, gobj);
	if (ret)
		return ret;

	fbInfo->size = size;

	/* setup helper */
	fb = &fbInfo->gfb.base;
	fbInfo->helper.fb = fb;

	strcpy(info->fix.id, "gbdrmfb");

	info->fbops = &gbdcfb_ops;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
	info->skip_vt_switch = false;
#endif

#if (defined CONFIG_CENTOS && !(defined SYS_CENTOS7_COMPILE_ENV)) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
	drm_fb_helper_fill_info(info, &fbInfo->helper, sizes);
#elif defined CONFIG_X86_64 || LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	drm_fb_helper_fill_fix(info, fb->pitches[0], fb->format->depth);
	drm_fb_helper_fill_var(info, &fbInfo->helper, sizes->fb_width,
				sizes->fb_height);
#else // MIPS & ARM64
	drm_fb_helper_fill_fix(info, fb->pitches[0], fb->depth);
	drm_fb_helper_fill_var(info, &fbInfo->helper, sizes->fb_width,
				sizes->fb_height);
#endif

	info->screen_base = bo->ttm_bo.kmap.virtual;
	info->screen_size = size;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 13, 0))
	drm_vma_offset_remove(bo->ttm_bo.bo.bdev->vma_manager, &bo->ttm_bo.bo.base.vma_node);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	drm_vma_offset_remove(&bo->ttm_bo.bo.bdev->vma_manager, &bo->ttm_bo.bo.base.vma_node);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE <= KERNEL_VERSION(5, 13, 0))
	drm_vma_offset_remove(bo->ttm_bo.bo.bdev->vma_manager, &bo->ttm_bo.bo.base.vma_node);
#else
	drm_vma_offset_remove(&bo->ttm_bo.bo.bdev->vma_manager, &bo->ttm_bo.bo.vma_node);
#endif
	info->fix.smem_start = gbdc->pcie_info.vram_config.fb_base;
	info->fix.smem_len = size;
	gb_printf(KERN_INFO, "%s-%d:screen size=%ld smem start=%lx, len=%d\n",
		__func__, __LINE__, info->screen_size, info->fix.smem_start,
		info->fix.smem_len);
	fbInfo->initialized = true;
	return 0;
}

static int GB02FUNC1220(struct GB02STR160 *fbInfo)
{
	struct GB02STR159 *gfb = &fbInfo->gfb;

	DRM_DEBUG_DRIVER("\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	drm_fb_helper_unregister_fbi(&fbInfo->helper);
#else
	drm_fb_helper_release_info(&fbInfo->helper);
#endif
	if (gfb->obj) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)) || \
	(defined CONFIG_CENTOS) || (defined SYS_CENTOS7_9_2009)
		drm_gem_object_put_unlocked(gfb->obj);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(gfb->obj);
#else
		drm_gem_object_unreference_unlocked(gfb->obj);
#endif
		gfb->obj = NULL;
	}

	drm_framebuffer_unregister_private(&gfb->base);
	drm_framebuffer_cleanup(&gfb->base);

	return 0;
}

static const struct drm_fb_helper_funcs gbdc_fb_helper_funcs = {
	.fb_probe = GB02FUNC1217,
};
int GB02FUNC1223(struct drm_device *drm_dev, struct GB02STR160 *fbInfo, int num_crtc, int num_connector)
{
	int ret;
	int bpp_sel = GB02MAC1940;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	drm_fb_helper_prepare(drm_dev, &fbInfo->helper,
			      &gbdc_fb_helper_funcs);
#else
	drm_fb_helper_prepare(drm_dev, &fbInfo->helper, bpp_sel,
			      &gbdc_fb_helper_funcs);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
	ret = drm_fb_helper_init(drm_dev, &fbInfo->helper);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) || (defined CONFIG_CENTOS)
	ret = drm_fb_helper_init(drm_dev, &fbInfo->helper, num_crtc);
#else // MIPS & ARM64
	ret = drm_fb_helper_init(drm_dev, &fbInfo->helper, num_crtc, num_connector);
#endif
	if (ret)
		return ret;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0))
	ret = drm_fb_helper_single_add_all_connectors(&fbInfo->helper);
#endif
	if (ret)
		goto fini;

#ifndef CONFIG_GBDC_ATOMIC_FEATURE
	drm_helper_disable_unused_functions(drm_dev);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	ret = drm_fb_helper_initial_config(&fbInfo->helper, bpp_sel);
#else
	ret = drm_fb_helper_initial_config(&fbInfo->helper);
#endif
	if (ret)
		goto fini;
	return 0;

 fini:
	drm_fb_helper_fini(&fbInfo->helper);
	return ret;
}


void GB02FUNC1227(struct GB02STR160 *fbInfo)
{
	if (fbInfo->initialized)
		GB02FUNC1220(fbInfo);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	if (fbInfo->helper.fbdev)
#else
	if (fbInfo->helper.info)
#endif
		drm_fb_helper_fini(&fbInfo->helper);

	fbInfo->initialized = false;
}
