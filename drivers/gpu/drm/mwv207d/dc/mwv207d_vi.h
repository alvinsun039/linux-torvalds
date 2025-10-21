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
#ifndef MWV207D_VI_H_OUVHEJAX
#define MWV207D_VI_H_OUVHEJAX

#include <linux/i2c.h>
#include <drm/drm_edid.h>
#include <drm/drm_atomic.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_helper.h>

#include "mwv207d_drv.h"

struct i2c_adapter;
enum mwv207d_i2c_chan{
	MWV207D_HDMI0_I2C_CHAN = 0,
	MWV207D_HDMI1_I2C_CHAN,
	MWV207D_HDMI2_I2C_CHAN,
	MWV207D_HDMI3_I2C_CHAN,
	MWV207D_VGA_I2C_CHAN,
	MWV207D_DVO_I2C_CHAN,
	MWV207D_I2C_CHAN_COUNT
};

struct mwv207d_output {
	struct drm_connector connector;
	struct drm_encoder   encoder;

	struct drm_crtc      *cur_crtc;

	void __iomem          *mmio;
	struct mwv207d_device *mdev;

	struct i2c_adapter   *ddc;
	struct edid          *forced_edid;
	u32                  key_offset;
	int                  i2c_chan;

	int                  idx;
};
#define connector_to_output(conn) container_of(conn, struct mwv207d_output, connector)
#define encoder_to_output(encoder) container_of(encoder, struct mwv207d_output, encoder)

int mwv207d_vi_init(struct mwv207d_device *mdev);
int mwv207d_edp_init(struct mwv207d_device *mdev);
int mwv207d_hdmi_init(struct mwv207d_device *mdev);
int mwv207d_vga_init(struct mwv207d_device *mdev);
int mwv207d_dvo_init(struct mwv207d_device *mdev);

struct i2c_adapter *mwv207d_i2c_create(struct mwv207d_device *mdev,
				       int i2c_chan);
void mwv207d_i2c_destroy(struct i2c_adapter *adapter);
bool mwv207d_i2c_probe(struct i2c_adapter *i2c_bus);

int mwv207d_output_ddc_init(struct mwv207d_output *output);
void mwv207d_output_ddc_fini(struct i2c_adapter *ddc);

int mwv207d_output_get_modes(struct drm_connector *connector);
void mwv207d_output_set_crtc(struct drm_encoder *encoder,
			     struct drm_crtc *crtc);
void *mwv207d_output_probe_and_init(struct mwv207d_device *mdev,
				u32 size, int idx,
				u32 i2c_chan, u32 key_offset);

static inline u32 mwv207d_output_read(struct mwv207d_output *output, u32 reg)
{
	BUG_ON(reg >= 0x8000);
	return readl(output->mmio + reg);
}

static inline void mwv207d_output_write(struct mwv207d_output *output,
					u32 reg, u32 value)
{
	BUG_ON(reg >= 0x8000);
	writel(value, output->mmio + reg);
}

static inline void mwv207d_output_modify(struct mwv207d_output *output,
					 u32 reg, u32 mask, u32 value)
{
	u32 rvalue;

	BUG_ON(reg >= 0x8000);
	rvalue = mwv207d_output_read(output, reg);
	rvalue = (rvalue & ~mask) | (value & mask);
	mwv207d_output_write(output, reg, rvalue);
}

#endif
