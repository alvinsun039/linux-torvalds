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
#include <linux/version.h>
#include <drm/drm_drv.h>
#include <drm/drm_print.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_atomic.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_crtc_helper.h>

#include "mwv207d_gem.h"
#include "mwv207d_kms.h"
#include "mwv207d_va.h"
#include "mwv207d_vi.h"
#include "mwv207d_audio.h"

static const struct drm_framebuffer_funcs mwv207d_framebuffer_funcs = {
	.destroy = drm_gem_fb_destroy,
	.create_handle = drm_gem_fb_create_handle,
};

int mwv207d_framebuffer_init(struct drm_device *dev,
			     struct drm_framebuffer *fb,
			     const struct drm_mode_fb_cmd2 *mode_cmd,
			     struct drm_gem_object *gobj)
{
	int ret;

	fb->obj[0] = gobj;
	drm_helper_mode_fill_fb_struct(dev, fb, mode_cmd);

	ret = drm_framebuffer_init(dev, fb, &mwv207d_framebuffer_funcs);
	if (ret) {
		fb->obj[0] = NULL;
		return ret;
	}

	return 0;
}

static struct drm_framebuffer *
mwv207d_user_framebuffer_create(struct drm_device *dev,
				struct drm_file *filp,
				const struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct drm_gem_object *gobj =
		drm_gem_object_lookup(filp, mode_cmd->handles[0]);
	struct drm_framebuffer *fb;
	int ret;

	if (!gobj) {
		dev_err(dev->dev, "failed to lookup gem object, handle = %d\n",
			mode_cmd->handles[0]);
		return ERR_PTR(-ENOENT);
	}

	if (gobj->import_attach) {
		DRM_DEBUG_KMS("can't create framebuffer from imported dma_buf");
		ret = -EINVAL;
		goto err_unref;
	}

	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if (!fb) {
		ret = -ENOMEM;
		goto err_unref;
	}

	ret = mwv207d_framebuffer_init(dev, fb, mode_cmd, gobj);
	if (ret)
		goto err_free;

	return fb;
err_free:
	kfree(fb);
err_unref:
	drm_gem_object_put(gobj);
	return ERR_PTR(ret);
}

static void mwv207d_atomic_prepare_vblank(struct drm_device *dev,
		struct drm_atomic_state *old_state)
{
	struct drm_crtc_state *new_crtc_state;
	struct drm_crtc *crtc;
	int i;

	for_each_new_crtc_in_state(old_state, crtc, new_crtc_state, i)
		mwv207d_crtc_prepare_vblank(crtc);
}

static void mwv207d_atomic_commit_tail(struct drm_atomic_state *old_state)
{
	struct drm_device *dev = old_state->dev;

	drm_atomic_helper_commit_modeset_disables(dev, old_state);
	drm_atomic_helper_commit_planes(dev, old_state, 0);
	drm_atomic_helper_commit_modeset_enables(dev, old_state);

	mwv207d_atomic_prepare_vblank(dev, old_state);

	drm_atomic_helper_commit_hw_done(old_state);
	drm_atomic_helper_wait_for_vblanks(dev, old_state);
	drm_atomic_helper_cleanup_planes(dev, old_state);
}

static struct drm_mode_config_helper_funcs mwv207d_mode_helper = {
	.atomic_commit_tail = mwv207d_atomic_commit_tail,
};

static const struct drm_mode_config_funcs mwv207d_mode_funcs = {
	.fb_create     = mwv207d_user_framebuffer_create,
	.atomic_check  = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
	.output_poll_changed = drm_fb_helper_output_poll_changed,
};

static void mwv207d_modeset_init(struct drm_device *dev)
{
	struct mwv207d_device *mdev = drm_to_mdev(dev);

	drm_mode_config_init(dev);

	dev->mode_config.async_page_flip = true;
	dev->mode_config.min_width = 20;
	dev->mode_config.min_height = 20;
	dev->mode_config.max_width = 4096 * 4;
	dev->mode_config.max_height = 4096 * 4;

	dev->mode_config.preferred_depth = 24;
	dev->mode_config.prefer_shadow = 1;
	dev->mode_config.fb_modifiers_not_supported = true;

	dev->mode_config.funcs = &mwv207d_mode_funcs;
	dev->mode_config.helper_private = &mwv207d_mode_helper;

	dev->mode_config.cursor_width = 64;
	dev->mode_config.cursor_height = 64;
}

static void mwv207d_mode_config_reset(struct mwv207d_device *mdev)
{
	mdev_write(mdev, 0x2B0000, 0xbc3fffff);
	mdev_write(mdev, 0x2B0008, 0x0000ffff);

	mdev_modify(mdev, 0x348004, (1 << 31) | (1 << 29), 0);
	mdev_modify(mdev, 0x348008, (1 << 31) | (1 << 29), 0);
	mdev_modify(mdev, 0x34800C, (1 << 31) | (1 << 29), 0);
	mdev_modify(mdev, 0x348010, (1 << 31) | (1 << 29), 0);

	mdev_modify(mdev, 0x348620, 1, 0);
	mdev_modify(mdev, 0x2B0064, 1, 0);

	mdev_write(mdev, 0x2B0000, 0x003fffff);
	mdev_write(mdev, 0x2B0000, 0xbc3fffff);

	mdev_modify(mdev, 0x2B0064, 1, 1);
	mdev_modify(mdev, 0x348620, 1, 1);

	mdev_modify(mdev, 0x348004,
		    (1 << 31) | (1 << 29), (1 << 31) | (1 << 29));
	mdev_modify(mdev, 0x348008,
		    (1 << 31) | (1 << 29), (1 << 31) | (1 << 29));
	mdev_modify(mdev, 0x34800C,
		    (1 << 31) | (1 << 29), (1 << 31) | (1 << 29));
	mdev_modify(mdev, 0x348010,
		    (1 << 31) | (1 << 29), (1 << 31) | (1 << 29));
	mdev_write(mdev, 0x2B0000, 0xffffffff);
	mdev_write(mdev, 0x2B0008, 0xffffffff);

	drm_mode_config_reset(&mdev->base);
}

int mwv207d_kms_init(struct mwv207d_device *mdev)
{
	int ret;

	mdev->mcard = mwv207d_audio_create(mdev);

	mwv207d_modeset_init(&mdev->base);

	ret = mwv207d_va_init(mdev);
	if (ret)
		goto cleanup;

	ret = mwv207d_vi_init(mdev);
	if (ret)
		goto cleanup;

	ret = drm_vblank_init(&mdev->base, mdev->base.mode_config.num_crtc);
	if (ret)
		goto cleanup;

	mwv207d_mode_config_reset(mdev);

	DRM_INFO("number of crtc:      %d\n", mdev->base.mode_config.num_crtc);
	DRM_INFO("number of encoder:   %d\n", mdev->base.mode_config.num_encoder);
	DRM_INFO("number of connector: %d\n", mdev->base.mode_config.num_connector);

	drm_kms_helper_poll_init(&mdev->base);

	return 0;

cleanup:
	drm_mode_config_cleanup(&mdev->base);
	return ret;
}

void mwv207d_kms_fini(struct mwv207d_device *mdev)
{
	drm_kms_helper_poll_fini(&mdev->base);
	drm_atomic_helper_shutdown(&mdev->base);
	mwv207d_fbdev_fini(mdev);
	drm_mode_config_cleanup(&mdev->base);
	mwv207d_audio_destroy(mdev);
}

int mwv207d_kms_suspend(struct mwv207d_device *mdev)
{
	struct drm_crtc *crtc;
	int ret;

	if (mdev->renderonly)
		return 0;

	ret = drm_mode_config_helper_suspend(&mdev->base);
	if (ret)
		return ret;

	drm_for_each_crtc(crtc, &mdev->base) {
		ret = mwv207d_va_lut_suspend(crtc);
		if (ret)
			return ret;
	}

	return 0;
}

int mwv207d_kms_resume(struct mwv207d_device *mdev)
{
	struct drm_crtc *crtc;
	int ret;

	if (mdev->renderonly)
		return 0;

	mwv207d_fbdev_resume(mdev);

	drm_for_each_crtc(crtc, &mdev->base) {
		ret = mwv207d_va_lut_resume(crtc);
		if (ret)
			return ret;
	}

	mwv207d_mode_config_reset(mdev);

	if (mdev->skip_thaw_kms)
		return 0;

	ret = drm_mode_config_helper_resume(&mdev->base);
	if (ret)
		dev_err(mdev->dev, "failed to resume kms, %d", ret);
	drm_kms_helper_hotplug_event(&mdev->base);

	return ret;
}
