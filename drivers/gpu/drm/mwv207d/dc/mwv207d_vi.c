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
#include "mwv207d_vbios.h"

bool mwv207d_output_probe(struct mwv207d_device *mdev, u32 key_offset)
{
	const struct mwv207d_vdat *vdat;

	vdat = mwv207d_vbios_vdat(mdev, 0,
			0x460 + key_offset + 0x0);

	if (!vdat)
		return false;

	return le32_to_cpu(*(uint32_t *)vdat->dat) != 0x0;
}

static int mwv207d_output_get_cfg_edid(struct mwv207d_output *output, u32 key_offset)
{
	const struct mwv207d_vdat *vdat;
	int edid_extensions = 0, i;
	u8 *edid, *block;

	vdat = mwv207d_vbios_vdat(output->mdev, 0,
			0x460 + key_offset + 0x2);
	if (!vdat)
		return 0;

	edid = devm_kzalloc(output->mdev->dev, 0x100, GFP_KERNEL);
	if (!edid) {
		dev_err(output->mdev->dev, "output edid alloc failed");
		return -ENOMEM;
	}

	BUG_ON(vdat->len > 0x100);
	memcpy(edid, vdat->dat, vdat->len);

	edid_extensions = edid[0x7e];
	BUG_ON(edid_extensions > 1);
	for (i = 0; i < edid_extensions + 1; i++) {
		block = edid + i * EDID_LENGTH;
		if (!drm_edid_block_valid(block, i, false, NULL)) {
			dev_err(output->mdev->dev, "bad edid from hw");
			return -EINVAL;
		}
	}
	output->forced_edid = (struct edid *)edid;

	return 0;
}

int mwv207d_output_ddc_init(struct mwv207d_output *output)
{
	output->ddc = mwv207d_i2c_create(output->mdev, output->i2c_chan);
	if (!output->ddc) {
		DRM_ERROR("Failed to create i2c adapter\n");
		return -ENODEV;
	}
	return 0;
}

void mwv207d_output_ddc_fini(struct i2c_adapter *ddc)
{
	if (ddc)
		mwv207d_i2c_destroy(ddc);
}

int mwv207d_output_get_modes(struct drm_connector *connector)
{
	struct mwv207d_output *output = connector_to_output(connector);
	struct edid *edid;
	int count;

	if (output->forced_edid)
		edid = output->forced_edid;
	else
		edid = drm_get_edid(connector, output->ddc);

	drm_connector_update_edid_property(connector, edid);
	count = drm_add_edid_modes(connector, edid);

	if (!output->forced_edid)
		kfree(edid);

	return count;
}

void mwv207d_output_set_crtc(struct drm_encoder *encoder, struct drm_crtc *crtc)
{
	struct mwv207d_output *output = encoder_to_output(encoder);

	output->cur_crtc = crtc;
}

void *mwv207d_output_probe_and_init(struct mwv207d_device *mdev,
				u32 size, int idx,
				u32 i2c_chan, u32 key_offset)
{
	struct mwv207d_output *output;
	int ret;

	if (!mwv207d_output_probe(mdev, key_offset))
		return NULL;

	output = devm_kzalloc(mdev->dev, size, GFP_KERNEL);
	if (!output)
		return ERR_PTR(-ENOMEM);

	output->mdev = mdev;
	ret = mwv207d_output_get_cfg_edid(output, key_offset);
	if (ret)
		return ERR_PTR(ret);

	output->idx = idx;
	output->mmio = mdev->mmio + 0x2D0000;
	output->i2c_chan = i2c_chan;
	output->key_offset = key_offset;

	ret = mwv207d_output_ddc_init(output);
	if (ret)
		return ERR_PTR(ret);

	ret = devm_add_action_or_reset(mdev->dev,
				       (void(*)(void *))mwv207d_output_ddc_fini,
				       output->ddc);
	if (ret)
		return ERR_PTR(ret);

	return output;
}

int mwv207d_vi_init(struct mwv207d_device *mdev)
{
	int ret;

	mutex_init(&mdev->gpio_lock);

	ret = mwv207d_hdmi_init(mdev);
	if (ret)
		return ret;
	ret = mwv207d_edp_init(mdev);
	if (ret)
		return ret;
	ret = mwv207d_vga_init(mdev);
	if (ret)
		return ret;
	ret = mwv207d_dvo_init(mdev);
	if (ret)
		return ret;
	return 0;
}
