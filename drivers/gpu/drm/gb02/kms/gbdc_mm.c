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
#include <linux/io.h>
#include <linux/console.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_atomic.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0)
#include <drm/drm_damage_helper.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 131)
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/gpu_scheduler.h>
#endif
#include <linux/pagemap.h>
#include <generated/uapi/linux/version.h>
#ifdef CONFIG_SKYLIN_OS_V10
#include <asm/machine_t.h>
#endif
#include <drm/drm_gem_framebuffer_helper.h>
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "gbdc_drv.h"
#include "gpu/gb_gem.h"
#include "device/gbdc_device.h"
#include "gpu/gb_ttm.h"
#include "vpu/vpu_mm/gb_vpu_ttm.h"
#include "gbdc_crtc.h"


int GB02FUNC1766(struct drm_device *dev, u32 size, bool iskernel,
		    struct drm_gem_object **obj)
{
	struct GB02STR56 *gbdcbo;
	struct GB02STR59 *gb_bo;
	int ret;

	*obj = NULL;

	size = PAGE_ALIGN(size);
	if (size == 0)
		return -EINVAL;

	gb_bo = kzalloc(sizeof(struct GB02STR59), GFP_KERNEL);
 	if (!gb_bo) {
		DRM_ERROR( "%s %d error: kmalloc GB02STR59 failed!\n",
			__func__, __LINE__);
		return -ENOMEM;
	}

	gb_bo->is_kms_bo = true;
	gb_bo->is_ttm_bo = true;
	gbdcbo = &gb_bo->gb_base;
	gb_bo->gbdev = dev->dev_private;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	ret = GB02FUNC1502(dev, size, 0, TTM_PL_FLAG_VRAM, 0, false, &gbdcbo->ttm_bo);
#else
	ret = GB02FUNC1502(dev, size, 0, TTM_PL_VRAM, 0, false, &gbdcbo->ttm_bo);
#endif
	if (ret) {
		if (ret != -ERESTARTSYS)
			DRM_ERROR("failed to allocate GEM object ret:%d\n", ret);
		kfree(gb_bo);
		return ret;
	}

	gb_bo = gpu_bo_to_gb_bo(gbdcbo);
#if KERNEL_VERSION(5, 3, 0) > LINUX_VERSION_CODE
	*obj = &gb_bo->gb_base.ttm_bo.base;
#else
	*obj = &gb_bo->gb_base.ttm_bo.bo.base;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	gbdcbo->initial_domain = TTM_PL_FLAG_VRAM & TTM_PL_MASK_MEM;
#else
	gbdcbo->initial_domain = TTM_PL_VRAM;
#endif

	return 0;
}

int GB02FUNC1777(struct drm_file *file,
		      struct drm_device *dev, uint32_t handle)
{
	return drm_gem_handle_delete(file, handle);
}

int GB02FUNC1779(struct drm_file *file, struct drm_device *dev,
		     struct drm_mode_create_dumb *args)
{
	struct drm_gem_object *gobj = NULL;
	struct GB02STR56 *bo = NULL;
	u32 handle;
	int ret;

	if (args->width%2 != 0)
		args->width += 1;
	args->pitch = args->width * ((args->bpp + 7) / 8);
	args->size = args->pitch * args->height;
	args->size = ALIGN(args->size, PAGE_SIZE);
	gb_printf(KERN_INFO, "%s: pthread name=%s args->size: %llx\n",
			__func__, current->comm, args->size);

	ret = GB02FUNC1766(dev, args->size, false, &gobj);
	if (ret) {
		gb_printf(KERN_ERR, "%s: GB02FUNC1766 err\n", __func__);
		return ret;
	}

	ret = drm_gem_handle_create(file, gobj, &handle);
#if (defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put_unlocked(gobj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(gobj);
#else
	drm_gem_object_unreference_unlocked(gobj);
#endif

	if (ret)
		return ret;

	bo = GB02FUNC217(gobj);
	if (bo == NULL) {
		gb_printf(KERN_ERR, "%s: bo == null\n", __func__);
	} else
#if KERNEL_VERSION(5, 0, 0) < LINUX_VERSION_CODE
		gb_printf(KERN_INFO, "%s: bo addr: %pK, vm_node.start: %llx\n",
		       __func__, bo, bo->ttm_bo.bo.base.vma_node.vm_node.start);
#else
		gb_printf(KERN_INFO, "%s: bo addr: %pK, vm_node.start: %llx\n",
		       __func__, bo, bo->ttm_bo.bo.vma_node.vm_node.start);
#endif
	args->handle = handle;
	return 0;
}


int GB02FUNC1783(struct drm_file *file, struct drm_device *dev,
			  uint32_t handle, uint64_t * offset)
{
	struct drm_gem_object *obj;
	struct GB02STR56 *dc_bo;
	struct GB02STR59 *gb_bo;


#if (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS)\
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)

	obj = drm_gem_object_lookup(file, handle);
#else
	obj = drm_gem_object_lookup(dev, file, handle);
#endif
	if (obj == NULL || offset == NULL) {
		gb_printf(KERN_ERR, "%s: obj/offset == null\n", __func__);
		return -ENOENT;
	}

	dc_bo = GB02FUNC217(obj);
	if (dc_bo == NULL) {
		gb_printf(KERN_ERR, "%s: bo == null\n", __func__);
		return -1;
	}

	gb_bo = gpu_bo_to_gb_bo(dc_bo);;
	if (gb_bo == NULL || !gb_bo->is_kms_bo) {
		DRM_ERROR("%s %d: handle %u is not for kms bo!gb_bo:%pK\n",
			__func__, __LINE__, handle, gb_bo);
		goto done;
	}

	*offset = GB02FUNC1394(&dc_bo->ttm_bo);
done:
#if (defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put_unlocked(obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(obj);
#else
	drm_gem_object_unreference_unlocked(obj);
#endif

	return 0;
}

static void GB02FUNC1788(struct drm_framebuffer *fb)
{
	struct GB02STR159 *gbdc_fb = to_gbdc_framebuffer(fb);

#if (defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put_unlocked(gbdc_fb->obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(gbdc_fb->obj);
#else
	drm_gem_object_unreference_unlocked(gbdc_fb->obj);
#endif

	drm_framebuffer_cleanup(fb);
	kfree(fb);
}

static const struct drm_framebuffer_funcs gbdc_fb_funcs = {
	.destroy = GB02FUNC1788,
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0)
	//.dirty = drm_atomic_helper_dirtyfb,
#endif
};

int GB02FUNC1790(struct drm_device *dev,
			  struct GB02STR159 *gfb,
			  const struct drm_mode_fb_cmd2 *mode_cmd,
			  struct drm_gem_object *obj)
{
	int ret;
#if (KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE)
	int i;
#endif

#if (defined CONFIG_X86_64) || (defined CONFIG_CENTOS) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	drm_helper_mode_fill_fb_struct(dev, &gfb->base, mode_cmd);
#else // MIPS & ARM64
	drm_helper_mode_fill_fb_struct(&gfb->base, mode_cmd);
#endif
	gfb->obj = obj;

#if (KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE)
	/*should use fb->format->num_planes, here we can use 1*/
	for (i = 0; i < 1; i++)
		gfb->base.obj[i] = &obj[i];
#endif

	ret = drm_framebuffer_init(dev, &gfb->base, &gbdc_fb_funcs);
	if (ret) {
		DRM_ERROR("drm_framebuffer_init failed: %d\n", ret);
		return ret;
	}

	return 0;
}

int GB02FUNC1794(struct drm_device *dev,
				struct drm_file *filp,
				struct GB02STR159 *gfb,
				const struct drm_mode_fb_cmd2 *mode_cmd)
{
#if (KERNEL_VERSION(5, 8, 0) <= LINUX_VERSION_CODE)
	return drm_gem_fb_init_with_funcs(dev, &gfb->base, filp, mode_cmd, &gbdc_fb_funcs);
#else
	return 0;
#endif
}

