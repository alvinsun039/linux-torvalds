/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <drm/drm_fb_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_framebuffer.h>

#include "mwv207d_drv.h"
#include "mwv207d_bo.h"
#include "mwv207d_drm.h"
#include "mwv207d_kms.h"

static struct fb_ops mwv207d_fb_ops = {
	.owner          = THIS_MODULE,
	DRM_FB_HELPER_DEFAULT_OPS,
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit,
};

static int mwv207d_fb_create(struct drm_fb_helper *fb_helper,
			     struct drm_fb_helper_surface_size *sizes)
{
	struct drm_device *dev = fb_helper->dev;
	struct mwv207d_device *mdev = drm_to_mdev(dev);
	struct mwv207d_fbdev *fbdev = mdev->fbdev;
	struct drm_framebuffer *fb = &fbdev->fb;
	struct drm_mode_fb_cmd2 mode_cmd;
	struct mwv207d_bo *mbo;
	struct fb_info *info;
	size_t size, aligned_size;
	void *cpu_addr;
	u64 gpu_addr;
	int ret;

	memset(&mode_cmd, 0, sizeof(mode_cmd));
	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;
	if (sizes->surface_bpp == 24)
		sizes->surface_bpp = 32;

	mode_cmd.pitches[0] = mode_cmd.width * ((sizes->surface_bpp + 7)/8);
	mode_cmd.pixel_format =
		drm_mode_legacy_fb_format(sizes->surface_bpp,
					  sizes->surface_depth);
	size = mode_cmd.pitches[0] * mode_cmd.height;
	aligned_size = ALIGN(size, PAGE_SIZE);

	ret = mwv207d_bo_create(mdev, size, PAGE_SIZE, false,
				0x2,
				0x1,
				NULL, NULL, &mbo);
	if (ret)
		return ret;

	ret = mwv207d_bo_reserve(mbo, false);
	if (ret)
		goto unref_bo;
	ret = mwv207d_bo_pin(mbo, 0x2, &gpu_addr);
	if (ret)
		goto unres_bo;
	ret = mwv207d_bo_kmap(mbo, &cpu_addr);
	if (ret)
		goto unpin_bo;
	fbdev->mbo = mbo;
	mwv207d_bo_unreserve(mbo);

	info = drm_fb_helper_alloc_info(fb_helper);
	if (IS_ERR(info)) {
		ret = PTR_ERR(info);
		goto unmap_bo;
	}
	info->par = fbdev;
	info->skip_vt_switch = true;

	ret = mwv207d_framebuffer_init(dev, fb, &mode_cmd, &mbo->tbo.base);
	if (ret) {
		pr_err("failed to initialize framebuffer %d\n", ret);
		goto cleanup_fbi;
	}

	memset(cpu_addr, 0x0, size);
	fbdev->helper.fb = fb;
	info->fbops = &mwv207d_fb_ops;

	info->fix.smem_start = gpu_addr;
	info->fix.smem_len = aligned_size;
	info->screen_base = cpu_addr;
	info->screen_size = size;

	drm_fb_helper_fill_info(info, fb_helper, sizes);

	if (!info->screen_base) {
		ret = -ENOSPC;
		goto cleanup_fb;
	}

	return 0;

cleanup_fb:
	drm_framebuffer_unregister_private(fb);
	drm_framebuffer_cleanup(fb);
cleanup_fbi:
	drm_fb_helper_unregister_info(fb_helper);
unmap_bo:
	mwv207d_bo_kunmap(mbo);
unpin_bo:
	mwv207d_bo_unpin(mbo);
unres_bo:
	mwv207d_bo_unreserve(mbo);
unref_bo:
	mwv207d_bo_unref(&mbo);
	return ret;
}

static const struct drm_fb_helper_funcs mwv207d_fb_helper = {
	.fb_probe = mwv207d_fb_create,
};

int mwv207d_fbdev_init(struct mwv207d_device *mdev)
{
	struct drm_device *dev = &mdev->base;
	struct mwv207d_fbdev *fbdev;
	int preferred_bpp = 32;
	int ret;

	if (!mdev->hw.is_pf || mdev->renderonly)
		return 0;

	fbdev = devm_kzalloc(mdev->dev, sizeof(*fbdev), GFP_KERNEL);
	if (!fbdev)
		return -ENOMEM;
	drm_fb_helper_prepare(dev, &fbdev->helper, preferred_bpp, &mwv207d_fb_helper);

	ret = drm_fb_helper_init(dev, &fbdev->helper);
	if (ret)
		return ret;
	mdev->fbdev = fbdev;
	fbdev->helper.dev = dev;

	ret = drm_fb_helper_initial_config(&fbdev->helper);
	if (ret)
		goto err_out;

	return 0;
err_out:
	drm_fb_helper_fini(&fbdev->helper);
	return ret;
}

void mwv207d_fbdev_fini(struct mwv207d_device *mdev)
{
	struct mwv207d_fbdev *fbdev = mdev->fbdev;

	if (!fbdev)
		return;

	drm_fb_helper_unregister_info(&fbdev->helper);
	drm_framebuffer_unregister_private(&fbdev->fb);
	drm_framebuffer_cleanup(&fbdev->fb);
	drm_fb_helper_fini(&fbdev->helper);

	if (mwv207d_bo_reserve(fbdev->mbo, false) == 0) {
		mwv207d_bo_kunmap(fbdev->mbo);
		mwv207d_bo_unpin(fbdev->mbo);
		mwv207d_bo_unreserve(fbdev->mbo);
		mwv207d_bo_unref(&fbdev->mbo);
	}
	mdev->fbdev = NULL;
}

void mwv207d_fbdev_resume(struct mwv207d_device *mdev)
{
	struct mwv207d_fbdev *fbdev = mdev->fbdev;

	if (!fbdev)
		return;
	memset(fbdev->mbo->kptr, 0x0, mwv207d_bo_size(fbdev->mbo));
}
