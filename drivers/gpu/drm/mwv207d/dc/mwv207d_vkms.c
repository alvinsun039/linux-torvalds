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
#include <drm/drm_crtc.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_plane.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_edid.h>

#include "mwv207d_gem.h"
#include "mwv207d_bo.h"
#include "mwv207d_drv.h"
#include "mwv207d_vkms.h"

struct mwv207d_vdisplay {
	struct drm_connector connector;
	struct drm_encoder   encoder;
	struct drm_crtc      crtc;
	struct drm_plane     primary_plane;
	struct drm_plane     cursor_plane;
	struct hrtimer       vblank_timer;
	struct drm_pending_vblank_event *event;
	ktime_t period_ns;
	u32 max_width;
	u32 max_height;
};

static u32 vkms_rgb_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XRGB2101010,
};

static void mwv207d_vkms_finish_vblank(struct drm_crtc *crtc)
{
	struct mwv207d_vdisplay *vdisplay = crtc_to_vdisplay(crtc);
	int put_vblank = false;
	unsigned long flags;

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (vdisplay->event) {
		drm_crtc_send_vblank_event(crtc, vdisplay->event);
		vdisplay->event = NULL;
		put_vblank = true;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

	if (put_vblank)
		drm_crtc_vblank_put(crtc);
}

static enum hrtimer_restart mwv207d_vkms_vblank_simulate(struct hrtimer *timer)
{
	struct mwv207d_vdisplay *vdisplay = container_of(timer, struct mwv207d_vdisplay, vblank_timer);
	struct drm_crtc *crtc = &vdisplay->crtc;
	u64 ret_overrun;
	bool ret;

	ret_overrun = hrtimer_forward_now(&vdisplay->vblank_timer,
					  vdisplay->period_ns);

	ret = drm_crtc_handle_vblank(crtc);

	if (!ret)
		return HRTIMER_NORESTART;

	mwv207d_vkms_finish_vblank(crtc);

	return HRTIMER_RESTART;
}

static int mwv207d_vkms_enable_vblank(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	unsigned int pipe = drm_crtc_index(crtc);
	struct drm_vblank_crtc *vblank = &dev->vblank[pipe];
	struct mwv207d_vdisplay *vdisplay = crtc_to_vdisplay(crtc);

	if (!crtc->state->active)
		return 0;

	drm_calc_timestamping_constants(crtc, &crtc->mode);

	vdisplay->period_ns = ktime_set(0, vblank->framedur_ns);
	hrtimer_start(&vdisplay->vblank_timer, vdisplay->period_ns,
		      HRTIMER_MODE_REL);

	return 0;
}

static void mwv207d_vkms_disable_vblank(struct drm_crtc *crtc)
{
	struct mwv207d_vdisplay *vdisplay = crtc_to_vdisplay(crtc);

	hrtimer_try_to_cancel(&vdisplay->vblank_timer);
}

static const struct drm_crtc_funcs mwv207d_vkms_crtc_funcs = {
	.set_config             = drm_atomic_helper_set_config,
	.destroy                = drm_crtc_cleanup,
	.page_flip              = drm_atomic_helper_page_flip,
	.reset                  = drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_crtc_destroy_state,
	.enable_vblank		= mwv207d_vkms_enable_vblank,
	.disable_vblank		= mwv207d_vkms_disable_vblank,
};

static void mwv207d_vkms_crtc_atomic_enable(struct drm_crtc *crtc,
					   struct drm_atomic_state *state)
{
	drm_crtc_vblank_on(crtc);
}

static void mwv207d_vkms_crtc_atomic_disable(struct drm_crtc *crtc,
					    struct drm_atomic_state *state)
{
	drm_crtc_vblank_off(crtc);
}

static const struct drm_crtc_helper_funcs mwv207d_vkms_crtc_helper_funcs = {
	.atomic_enable	= mwv207d_vkms_crtc_atomic_enable,
	.atomic_disable	= mwv207d_vkms_crtc_atomic_disable,
};

static const struct drm_connector_funcs mwv207d_vkms_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

struct mwv207d_mode_size {
	int w;
	int h;
	int vrefresh;
};

static const struct mwv207d_mode_size common_modes[] = {
	{ 640,  480, 30},
	{ 640,  480, 60},
	{ 720,  480, 30},
	{ 720,  480, 60},
	{ 800,  600, 30},
	{ 800,  600, 60},
	{ 848,  480, 30},
	{ 848,  480, 60},
	{1024,  768, 30},
	{1024,  768, 60},
	{1152,  768, 30},
	{1152,  768, 60},
	{1280,  720, 30},
	{1280,  720, 60},
	{1280,  800, 30},
	{1280,  800, 60},
	{1280,  854, 30},
	{1280,  854, 60},
	{1280,  960, 30},
	{1280,  960, 60},
	{1280, 1024, 30},
	{1280, 1024, 60},
	{1440,  900, 30},
	{1440,  900, 60},
	{1400, 1050, 30},
	{1400, 1050, 60},
	{1680, 1050, 30},
	{1680, 1050, 60},
	{1600, 1200, 30},
	{1600, 1200, 60},
	{1920, 1080, 30},
	{1920, 1080, 60},
	{1920, 1200, 30},
	{1920, 1200, 60},
	{2560, 1440, 30},
	{2560, 1440, 60},
	{4096, 3112, 30},
	{4096, 3112, 60},
	{3656, 2664, 30},
	{3656, 2664, 60},
	{3840, 2160, 30},
	{3840, 2160, 60},
	{4096, 2160, 30},
	{4096, 2160, 60},
	{7680, 4320, 30},
	{7680, 4320, 60},
};

static int mwv207d_vkms_conn_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct mwv207d_vdisplay *vdisplay = connector_to_vdisplay(connector);
	struct drm_display_mode *mode = NULL;
	unsigned int i, prew = 0, preh = 0, count = 0;

	for (i = 0; i < ARRAY_SIZE(common_modes); i++) {
		if (common_modes[i].w * common_modes[i].h >
				vdisplay->max_width * vdisplay->max_height)
			continue;

		mode = drm_cvt_mode(dev, common_modes[i].w,
				    common_modes[i].h, common_modes[i].vrefresh,
				    false, false, false);
		if (!mode)
			continue;

		if (common_modes[i].w * common_modes[i].h > prew * preh) {
			prew = common_modes[i].w;
			preh = common_modes[i].h;
		}

		drm_mode_probed_add(connector, mode);
		count++;
	}

	if (prew * preh > 1920 * 1080)
		drm_set_preferred_mode(connector, 1920, 1080);
	else
		drm_set_preferred_mode(connector, prew, preh);

	return count;
}

static int mwv207d_vkms_conn_detect_ctx(struct drm_connector *connector,
		struct drm_modeset_acquire_ctx *ctx, bool force)
{

	return connector_status_connected;
}

static enum drm_mode_status mwv207d_vkms_mode_valid(
		struct mwv207d_vdisplay *vdisplay,
		const struct drm_display_mode *mode)
{
	if (mode->vdisplay * mode->hdisplay >
			vdisplay->max_width * vdisplay->max_height)
		return MODE_CLOCK_HIGH;
	return MODE_OK;
}

static enum drm_mode_status mwv207d_vkms_conn_mode_valid(
		struct drm_connector *connector,
		struct drm_display_mode *mode)
{
	struct mwv207d_vdisplay *vdisplay = connector_to_vdisplay(connector);

	return mwv207d_vkms_mode_valid(vdisplay, mode);
}

static const struct drm_connector_helper_funcs mwv207d_vkms_conn_helper_funcs = {
	.get_modes  = mwv207d_vkms_conn_get_modes,
	.mode_valid = mwv207d_vkms_conn_mode_valid,
	.detect_ctx = mwv207d_vkms_conn_detect_ctx,
};

static bool mwv207d_vkms_plane_format_mod_supported(
	struct drm_plane *plane, uint32_t format, uint64_t modifier)
{
	if (modifier == DRM_FORMAT_MOD_LINEAR)
		return true;
	return false;
}

static const struct drm_plane_funcs mwv207d_vkms_plane_funcs = {
	.update_plane		= drm_atomic_helper_update_plane,
	.disable_plane		= drm_atomic_helper_disable_plane,
	.destroy		= drm_plane_cleanup,
	.reset			= drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_plane_destroy_state,
	.format_mod_supported   = mwv207d_vkms_plane_format_mod_supported,
};

static void mwv207d_vkms_plane_atomic_update(struct drm_plane *plane,
					     struct drm_atomic_state *state)
{
}

static int mwv207d_vkms_plane_atomic_check(struct drm_plane *plane,
					   struct drm_atomic_state *state)
{

	return 0;
}

static const struct drm_plane_helper_funcs mwv207d_vkms_plane_helper_funcs = {
	.atomic_update = mwv207d_vkms_plane_atomic_update,
	.atomic_check  = mwv207d_vkms_plane_atomic_check,
	.prepare_fb    = drm_gem_plane_helper_prepare_fb,
};

static const struct drm_framebuffer_funcs mwv207d_vkms_framebuffer_funcs = {
	.destroy = drm_gem_fb_destroy,
	.create_handle = drm_gem_fb_create_handle,
};

int mwv207d_vkms_framebuffer_init(struct drm_device *dev,
				  struct drm_framebuffer *fb,
				  const struct drm_mode_fb_cmd2 *mode_cmd,
				  struct drm_gem_object *gobj)
{
	int ret;

	fb->obj[0] = gobj;
	drm_helper_mode_fill_fb_struct(dev, fb, mode_cmd);

	ret = drm_framebuffer_init(dev, fb, &mwv207d_vkms_framebuffer_funcs);
	if (ret) {
		fb->obj[0] = NULL;
		return ret;
	}

	return 0;
}

static struct drm_framebuffer *
mwv207d_vkms_framebuffer_create(struct drm_device *dev,
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

	ret = mwv207d_vkms_framebuffer_init(dev, fb, mode_cmd, gobj);
	if (ret)
		goto err_free;

	return fb;
err_free:
	kfree(fb);
err_unref:
	drm_gem_object_put(gobj);
	return ERR_PTR(ret);
}

static const struct drm_mode_config_funcs mwv207d_mode_config_funcs = {
	.fb_create           = mwv207d_vkms_framebuffer_create,
	.atomic_check        = drm_atomic_helper_check,
	.atomic_commit       = drm_atomic_helper_commit,
	.output_poll_changed = drm_fb_helper_output_poll_changed,
};

static void mwv207d_vkms_prepare_vblank(struct drm_crtc *crtc)
{
	struct drm_pending_vblank_event *event = crtc->state->event;
	struct mwv207d_vdisplay *vdisplay = crtc_to_vdisplay(crtc);
	unsigned long flags;

	if (event) {
		if (crtc->state->active) {
			WARN_ON(drm_crtc_vblank_get(crtc) != 0);
			spin_lock_irqsave(&crtc->dev->event_lock, flags);
			vdisplay->event = event;
			spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
		} else {
			spin_lock_irqsave(&crtc->dev->event_lock, flags);
			drm_crtc_send_vblank_event(crtc, crtc->state->event);
			spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
		}
		crtc->state->event = NULL;
	}
}

static void mwv207d_vkms_atomic_prepare_vblank(struct drm_device *dev,
		struct drm_atomic_state *old_state)
{
	struct drm_crtc_state *new_crtc_state;
	struct drm_crtc *crtc;
	int i;

	for_each_new_crtc_in_state(old_state, crtc, new_crtc_state, i)
		mwv207d_vkms_prepare_vblank(crtc);
}

static void mwv207d_atomic_commit_tail(struct drm_atomic_state *old_state)
{
	struct drm_device *dev = old_state->dev;

	drm_atomic_helper_commit_modeset_disables(dev, old_state);
	drm_atomic_helper_commit_planes(dev, old_state, 0);
	drm_atomic_helper_commit_modeset_enables(dev, old_state);
	mwv207d_vkms_atomic_prepare_vblank(dev, old_state);
	drm_atomic_helper_commit_hw_done(old_state);
	drm_atomic_helper_wait_for_vblanks(dev, old_state);
	drm_atomic_helper_cleanup_planes(dev, old_state);
}

static struct drm_mode_config_helper_funcs mwv207d_mode_config_helper_funcs = {
	.atomic_commit_tail = mwv207d_atomic_commit_tail,
};

static void mwv207d_vkms_modeset_init(struct drm_device *dev)
{
	drm_mode_config_init(dev);

	dev->mode_config.min_width = 20;
	dev->mode_config.min_height = 20;

	dev->mode_config.max_width = 4096 * 4;
	dev->mode_config.max_height = 4096 * 4;

	dev->mode_config.preferred_depth = 24;
	dev->mode_config.prefer_shadow = 1;
	dev->mode_config.fb_modifiers_not_supported = true;

	dev->mode_config.async_page_flip = true;

	dev->mode_config.funcs = &mwv207d_mode_config_funcs;
	dev->mode_config.helper_private = &mwv207d_mode_config_helper_funcs;
}

static const struct drm_encoder_funcs mwv207d_vkms_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static enum drm_mode_status mwv207d_vkms_encoder_mode_valid(
		struct drm_encoder *encoder,
		const struct drm_display_mode *mode)
{
	struct mwv207d_vdisplay *vdisplay = encoder_to_vdisplay(encoder);

	return mwv207d_vkms_mode_valid(vdisplay, mode);
}

static const struct drm_encoder_helper_funcs mwv207d_vkms_encoder_helper_funcs = {
	.mode_valid = mwv207d_vkms_encoder_mode_valid,
};

static int mwv207d_vkms_vdisplay_init(struct drm_device *ddev,
				      struct mwv207d_vdisplay *vdisplay, int idx)
{
	int ret;

	ret = drm_universal_plane_init(ddev, &vdisplay->primary_plane, 1 << idx,
				       &mwv207d_vkms_plane_funcs, vkms_rgb_formats,
				       ARRAY_SIZE(vkms_rgb_formats),
				       NULL, DRM_PLANE_TYPE_PRIMARY, NULL);
	if (ret)
		return ret;
	drm_plane_helper_add(&vdisplay->primary_plane,
			     &mwv207d_vkms_plane_helper_funcs);

	ret = drm_universal_plane_init(ddev, &vdisplay->cursor_plane, 1 << idx,
				       &mwv207d_vkms_plane_funcs, vkms_rgb_formats,
				       ARRAY_SIZE(vkms_rgb_formats),
				       NULL, DRM_PLANE_TYPE_CURSOR, NULL);
	if (ret)
		return ret;
	drm_plane_helper_add(&vdisplay->cursor_plane,
			     &mwv207d_vkms_plane_helper_funcs);

	ret = drm_crtc_init_with_planes(ddev, &vdisplay->crtc,
					&vdisplay->primary_plane, &vdisplay->cursor_plane,
					&mwv207d_vkms_crtc_funcs, NULL);
	if (ret)
		return ret;
	drm_crtc_helper_add(&vdisplay->crtc, &mwv207d_vkms_crtc_helper_funcs);

	hrtimer_init(&vdisplay->vblank_timer,
		     CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vdisplay->vblank_timer.function = &mwv207d_vkms_vblank_simulate;

	vdisplay->connector.polled = DRM_CONNECTOR_POLL_CONNECT |
				     DRM_CONNECTOR_POLL_DISCONNECT;
	ret = drm_connector_init(ddev, &vdisplay->connector,
				 &mwv207d_vkms_connector_funcs,
				 DRM_MODE_CONNECTOR_VIRTUAL);
	if (ret)
		return ret;
	drm_connector_helper_add(&vdisplay->connector,
				 &mwv207d_vkms_conn_helper_funcs);

	ret = drm_encoder_init(ddev, &vdisplay->encoder,
				&mwv207d_vkms_encoder_funcs,
				DRM_MODE_ENCODER_VIRTUAL, "virtual");
	if (ret)
		return ret;
	drm_encoder_helper_add(&vdisplay->encoder,
			       &mwv207d_vkms_encoder_helper_funcs);
	vdisplay->encoder.possible_crtcs = (1 << ddev->mode_config.num_crtc) - 1;

	ret = drm_connector_attach_encoder(&vdisplay->connector,
					   &vdisplay->encoder);
	if (ret)
		return ret;

	return 0;
}

int mwv207d_vkms_init(struct mwv207d_device *mdev)
{
	struct drm_device *ddev = &mdev->base;
	struct mwv207d_vdisplay *vdisplay;
	struct mwv207d_hwinfo *hw = &mdev->hw;
	int ret, i;

	vdisplay = devm_kcalloc(mdev->dev, hw->nr_vconnectors,
				sizeof(*vdisplay), GFP_KERNEL);
	if (!vdisplay)
		return -ENOMEM;

	mdev->vdisplay = vdisplay;

	mwv207d_vkms_modeset_init(ddev);

	for (i = 0; i < hw->nr_vconnectors; i++) {
		vdisplay[i].max_width = hw->vconn_max_width;
		vdisplay[i].max_height = hw->vconn_max_height;
		ret = mwv207d_vkms_vdisplay_init(ddev, &vdisplay[i], i);
		if (ret)
			goto cleanup;
	}

	ret = drm_vblank_init(ddev, ddev->mode_config.num_crtc);
	if (ret)
		goto cleanup;

	drm_mode_config_reset(ddev);

	drm_kms_helper_poll_init(ddev);

	DRM_INFO("number of crtc:      %d\n", ddev->mode_config.num_crtc);
	DRM_INFO("number of encoder:   %d\n", ddev->mode_config.num_encoder);
	DRM_INFO("number of connector: %d\n", ddev->mode_config.num_connector);

	return 0;
cleanup:
	drm_mode_config_cleanup(ddev);
	return ret;
}

void mwv207d_vkms_fini(struct mwv207d_device *mdev)
{
	struct mwv207d_vdisplay *vdisplay = mdev->vdisplay;
	struct drm_device *ddev = &mdev->base;
	int i;

	for (i = 0; i < ddev->mode_config.num_crtc; i++)
		hrtimer_cancel(&vdisplay[i].vblank_timer);

	drm_kms_helper_poll_fini(ddev);
	drm_atomic_helper_shutdown(ddev);
	drm_mode_config_cleanup(ddev);
}
