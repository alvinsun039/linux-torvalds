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

static void mwv207d_dvo_switch(struct mwv207d_output *output, bool on)
{
	mwv207d_output_modify(output, 0x200,
			     0x1 << 0, (on ? 1 : 0) << 0);
}

static void mwv207d_dvo_config(struct mwv207d_output *output)
{
	struct drm_display_mode *mode = &output->cur_crtc->state->adjusted_mode;
	int hpol, vpol;

	hpol = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 0 : 1;
	vpol = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 0 : 1;

	mwv207d_output_modify(output, 0x200, 0x1 << 9, hpol << 9);
	mwv207d_output_modify(output, 0x200, 0x1 << 8, vpol << 8);

}

static void mwv207d_dvo_select_crtc(struct mwv207d_output *output)
{

	mwv207d_output_modify(output, 0x200, 0x7 << 16,
			      drm_crtc_index(output->cur_crtc) << 16);

	mdev_modify(output->mdev, 0x2B0018, 0xf,
		    drm_crtc_index(output->cur_crtc));
}
static enum drm_mode_status mwv207d_dvo_mode_valid(struct mwv207d_output *output,
					const struct drm_display_mode *mode)
{

	if (mode->clock > 162000)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static enum drm_mode_status mwv207d_dvo_connector_mode_valid(
	struct drm_connector *connector, struct drm_display_mode *mode)
{
	return mwv207d_dvo_mode_valid(connector_to_output(connector), mode);
}

static int mwv207d_dvo_detect_ctx(struct drm_connector *connector,
				  struct drm_modeset_acquire_ctx *ctx,
				  bool force)
{
	struct mwv207d_output *output = connector_to_output(connector);

	if (connector->force == DRM_FORCE_OFF)
		return connector_status_disconnected;

	if (connector->force == DRM_FORCE_ON)
		return connector_status_connected;

	if (output->forced_edid)
		return connector_status_connected;

	if (mwv207d_i2c_probe(output->ddc))
		return connector_status_connected;
	else
		return connector_status_disconnected;
}

static void mwv207d_dvo_destroy(struct drm_connector *conn)
{
	drm_connector_cleanup(conn);
}

static const struct drm_connector_helper_funcs mwv207d_dvo_connector_helper_funcs = {
	.get_modes  = mwv207d_output_get_modes,
	.mode_valid = mwv207d_dvo_connector_mode_valid,
	.detect_ctx = mwv207d_dvo_detect_ctx
};

static const struct drm_connector_funcs mwv207d_dvo_connector_funcs = {
	.reset                  = drm_atomic_helper_connector_reset,
	.fill_modes             = drm_helper_probe_single_connector_modes,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
	.destroy                = mwv207d_dvo_destroy,
};

static enum drm_mode_status mwv207d_dvo_encoder_mode_valid(
	struct drm_encoder *encoder, const struct drm_display_mode *mode)
{
	struct mwv207d_output *output = encoder_to_output(encoder);

	return mwv207d_dvo_mode_valid(output, mode);
}

static int mwv207d_dvo_encoder_atomic_check(struct drm_encoder *encoder,
		struct drm_crtc_state *crtc_state,
		struct drm_connector_state *conn_state)
{
	return 0;
}

static void mwv207d_dvo_encoder_enable(struct drm_encoder *encoder)
{
	struct mwv207d_output *output = encoder_to_output(encoder);

	mwv207d_dvo_select_crtc(output);

	mwv207d_dvo_config(output);

	mwv207d_dvo_switch(output, true);
}

static void mwv207d_dvo_encoder_disable(struct drm_encoder *encoder)
{
	struct mwv207d_output *output = encoder_to_output(encoder);

	mwv207d_dvo_switch(output, false);
}

static void mwv207d_dvo_encoder_reset(struct drm_encoder *encoder)
{
	mwv207d_dvo_encoder_disable(encoder);
}

static const struct drm_encoder_funcs mwv207d_dvo_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
	.reset   = mwv207d_dvo_encoder_reset,
};

static const struct drm_encoder_helper_funcs mwv207d_dvo_encoder_helper_funcs = {
	.mode_valid      = mwv207d_dvo_encoder_mode_valid,
	.atomic_check    = mwv207d_dvo_encoder_atomic_check,
	.enable          = mwv207d_dvo_encoder_enable,
	.disable         = mwv207d_dvo_encoder_disable,
};

int mwv207d_dvo_init(struct mwv207d_device *mdev)
{
	struct mwv207d_output *output;
	int ret;

	output = mwv207d_output_probe_and_init(mdev, sizeof(struct mwv207d_output), 0,
					MWV207D_DVO_I2C_CHAN, (20*6));

	if (!output) {
		dev_info(mdev->dev, "dvo is disabled by firmware");
		return 0;
	}

	if (IS_ERR(output))
		return PTR_ERR(output);

	output->connector.polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
	ret = drm_connector_init_with_ddc(&mdev->base, &output->connector,
				 &mwv207d_dvo_connector_funcs,
				 DRM_MODE_CONNECTOR_DVII, output->ddc);
	if (ret)
		return ret;
	drm_connector_helper_add(&output->connector,
				 &mwv207d_dvo_connector_helper_funcs);

	output->encoder.possible_crtcs = (1 << mdev->base.mode_config.num_crtc) - 1;
	ret = drm_encoder_init(&mdev->base, &output->encoder,
			       &mwv207d_dvo_encoder_funcs, DRM_MODE_ENCODER_DAC,
			       "dvo-%d", output->idx);
	if (ret)
		return ret;
	drm_encoder_helper_add(&output->encoder,
			       &mwv207d_dvo_encoder_helper_funcs);

	ret = drm_connector_attach_encoder(&output->connector,
					   &output->encoder);
	if (ret)
		return ret;

	return 0;
}
