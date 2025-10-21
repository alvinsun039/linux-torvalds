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
#include "mwv207d_vi.h"

#define output_to_vga(output) container_of(output, struct mwv207d_vga, base)

static uint mwv207d_vga_load_detect;
module_param(mwv207d_vga_load_detect, uint, 0444);
MODULE_PARM_DESC(mwv207d_vga_load_detect,
		 "Enable vga load detect if set to 1, default is 1");

struct mwv207d_vga {
	struct mwv207d_output base;
	spinlock_t load_detect_lock;
	bool active;
};

static void mwv207d_vga_set_detect_mode(struct mwv207d_output *output,
					bool active)
{
	u32 val, status, load_mode;
	int i;

	mwv207d_output_write(output, 0xA4, 0);

	val = 0x3ff * 7 / 10;
	status = mwv207d_output_read(output, 0xA8);
	mwv207d_output_write(output, 0xA8, status);

	status = val << 20 | val << 10 | val;
	mwv207d_output_write(output, 0xAC, status);

	load_mode = active ? 1 : 0;
	status = load_mode << 20 | load_mode << 16 | load_mode << 12 | 0x1 << 8 | 0x1 << 4 | 0x1;
	mwv207d_output_write(output, 0xAC, status);

	for (i = 0; i < 3; i++) {
		if (active) {
			mwv207d_output_write(output,
					    (0x64 + (i) * 0x14),  256);
			mwv207d_output_write(output,
					    (0x68 + (i) * 0x14)   , 48);
			mwv207d_output_write(output,
					    (0x70 + (i) * 0x14)   , 188);
			mwv207d_output_write(output,
					    (0x74 + (i) * 0x14)   , 164);
		} else {
			mwv207d_output_write(output,
					    (0x68 + (i) * 0x14)   , 1600000);
			mwv207d_output_write(output,
					    (0x6C + (i) * 0x14)   , 798400000);
			mwv207d_output_write(output,
					    (0x70 + (i) * 0x14)   , 1600128);
			mwv207d_output_write(output,
					    (0x74 + (i) * 0x14)   , 8128);
		}
	}

	mwv207d_output_write(output, 0xA4, 0x111111);
}

static void mwv207d_vga_switch(struct mwv207d_output *output, bool on)
{
	struct mwv207d_vga *vga = output_to_vga(output);

	if (!mwv207d_vga_load_detect) {
		mwv207d_output_modify(output, 0x0,
				      1 << 31, (on ? 1 : 0) << 31);
		return;
	}

	spin_lock(&vga->load_detect_lock);
	mwv207d_output_modify(output, 0x0,
			      1 << 31, (on ? 1 : 0) << 31);
	mwv207d_vga_set_detect_mode(output, on);
	vga->active = on;
	spin_unlock(&vga->load_detect_lock);
}

static void mwv207d_vga_config(struct mwv207d_output *output)
{
	struct drm_display_mode *mode = &output->cur_crtc->state->adjusted_mode;
	int hpol, vpol;

	hpol = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 0 : 1;
	vpol = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 0 : 1;

	mwv207d_output_write(output, 0x4C, 0);
	mwv207d_output_modify(output, 0x0, 0x7 << 16, 0x7 << 16);
	mwv207d_output_modify(output, 0x0, 0x1 << 2, hpol << 2);
	mwv207d_output_modify(output, 0x0, 0x1 << 3, vpol << 3);

	mwv207d_output_write(output, 0x4, 0x1666);
	mwv207d_output_write(output, 0x18, 0x1666);
	mwv207d_output_write(output, 0x2C, 0x1666);
}

static void mwv207d_vga_select_crtc(struct mwv207d_output *output)
{

	mwv207d_output_modify(output, 0x0, 0x3 << 24,
			      drm_crtc_index(output->cur_crtc) << 24);

	mdev_modify(output->mdev, 0x2B0014, 0xf << 16,
		    drm_crtc_index(output->cur_crtc) << 16);
}
static enum drm_mode_status mwv207d_vga_mode_valid(struct mwv207d_output *output,
		const struct drm_display_mode *mode)
{

	if (mode->clock > 193250)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static enum drm_mode_status mwv207d_vga_connector_mode_valid(
	struct drm_connector *connector, struct drm_display_mode *mode)
{
	return mwv207d_vga_mode_valid(connector_to_output(connector), mode);
}

static int mwv207d_vga_detect_load(struct mwv207d_output *output)
{
	struct mwv207d_vga *vga = output_to_vga(output);
	u32 r_state, g_state, b_state, value;
	bool active;

	if (mwv207d_i2c_probe(output->ddc))
		return connector_status_connected;

	spin_lock(&vga->load_detect_lock);
	value = mwv207d_output_read(output, 0xA8);
	active = vga->active;
	spin_unlock(&vga->load_detect_lock);

	r_state = (value >> 8) & 0x1;
	g_state = (value >> 4) & 0x1;
	b_state = (value >> 0) & 0x1;
	DRM_DEBUG_DRIVER("active %d r_state = 0x%x, g_state = 0x%x, b_state = 0x%x",
			 active, r_state, g_state, b_state);

	if (active)
		return (r_state && g_state && b_state) ?
			connector_status_disconnected : connector_status_connected;

	return (r_state || g_state || b_state) ?
		connector_status_connected : connector_status_disconnected;
}

static int mwv207d_vga_detect_ctx(struct drm_connector *connector,
				  struct drm_modeset_acquire_ctx *ctx,
				  bool force)
{
	struct mwv207d_output *output = connector_to_output(connector);

	return connector_status_connected;

	if (connector->force == DRM_FORCE_OFF)
		return connector_status_disconnected;

	if (connector->force == DRM_FORCE_ON)
		return connector_status_connected;

	if (output->forced_edid)
		return connector_status_connected;

	if (!mwv207d_vga_load_detect)
		return mwv207d_i2c_probe(output->ddc) ?
				connector_status_connected : connector_status_disconnected;

	return mwv207d_vga_detect_load(output);
}

static void mwv207d_vga_destroy(struct drm_connector *conn)
{
	drm_connector_cleanup(conn);
}

static const struct drm_connector_helper_funcs vga_connector_helper = {
	.get_modes  = mwv207d_output_get_modes,
	.mode_valid = mwv207d_vga_connector_mode_valid,
	.detect_ctx = mwv207d_vga_detect_ctx
};

static const struct drm_connector_funcs vga_connector_funcs = {
	.reset                  = drm_atomic_helper_connector_reset,
	.fill_modes             = drm_helper_probe_single_connector_modes,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
	.destroy                = mwv207d_vga_destroy,
};

static enum drm_mode_status mwv207d_vga_encoder_mode_valid(
	struct drm_encoder *encoder, const struct drm_display_mode *mode)
{
	struct mwv207d_output *output = encoder_to_output(encoder);

	return mwv207d_vga_mode_valid(output, mode);
}

static int mwv207d_vga_encoder_atomic_check(struct drm_encoder *encoder,
		struct drm_crtc_state *crtc_state,
		struct drm_connector_state *conn_state)
{
	return 0;
}

static void mwv207d_vga_encoder_enable(struct drm_encoder *encoder)
{
	struct mwv207d_output *output = encoder_to_output(encoder);

	mwv207d_vga_select_crtc(output);
	mwv207d_vga_config(output);
	mwv207d_vga_switch(output, true);
}

static void mwv207d_vga_encoder_disable(struct drm_encoder *encoder)
{
	struct mwv207d_output *output = encoder_to_output(encoder);

	mwv207d_vga_switch(output, false);
}

static void mwv207d_vga_encoder_reset(struct drm_encoder *encoder)
{
	mwv207d_vga_encoder_disable(encoder);
}

static const struct drm_encoder_funcs mwv207d_vga_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
	.reset   = mwv207d_vga_encoder_reset,
};

static const struct drm_encoder_helper_funcs mwv207d_vga_encoder_helper_funcs = {
	.mode_valid      = mwv207d_vga_encoder_mode_valid,
	.atomic_check    = mwv207d_vga_encoder_atomic_check,
	.enable          = mwv207d_vga_encoder_enable,
	.disable         = mwv207d_vga_encoder_disable,
};

int mwv207d_vga_init(struct mwv207d_device *mdev)
{
	struct mwv207d_output *output;
	struct mwv207d_vga *vga;
	int ret;

	vga = mwv207d_output_probe_and_init(mdev, sizeof(struct mwv207d_vga), 0,
					MWV207D_VGA_I2C_CHAN, (20*7));

	if (!vga) {
		dev_info(mdev->dev, "vga is disabled by firmware");
		return 0;
	}

	if (IS_ERR(vga))
		return PTR_ERR(vga);

	spin_lock_init(&vga->load_detect_lock);
	output = &vga->base;

	output->connector.polled = DRM_CONNECTOR_POLL_CONNECT |
				   DRM_CONNECTOR_POLL_DISCONNECT;
	ret = drm_connector_init_with_ddc(&mdev->base, &output->connector,
				 &vga_connector_funcs, DRM_MODE_CONNECTOR_VGA, output->ddc);
	if (ret)
		return ret;
	drm_connector_helper_add(&output->connector, &vga_connector_helper);

	output->encoder.possible_crtcs = (1 << mdev->base.mode_config.num_crtc) - 1;
	ret = drm_encoder_init(&mdev->base, &output->encoder,
			       &mwv207d_vga_encoder_funcs, DRM_MODE_ENCODER_DAC,
			       "vga-%d", output->idx);
	if (ret)
		return ret;
	drm_encoder_helper_add(&output->encoder,
			       &mwv207d_vga_encoder_helper_funcs);

	ret = drm_connector_attach_encoder(&output->connector,
					   &output->encoder);
	if (ret)
		return ret;

	return 0;
}
