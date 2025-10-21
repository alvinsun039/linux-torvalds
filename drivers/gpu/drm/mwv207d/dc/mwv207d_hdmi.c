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
#include <linux/delay.h>
#include <drm/display/drm_scdc_helper.h>
#include <drm/display/drm_hdmi_helper.h>

#include "dw-hdmi.h"
#include "mwv207d_irq.h"
#include "mwv207d_vbios.h"
#include "mwv207d_audio.h"
#include "mwv207d_vi.h"

#define connector_to_hdmi(conn) container_of(conn, struct mwv207d_hdmi, base.connector)
#define encoder_to_hdmi(encoder) container_of(encoder, struct mwv207d_hdmi, base.encoder)

#define output_to_hdmi(output) container_of(output, struct mwv207d_hdmi, base)

struct mwv207d_hdmi_phy_data {
	u32 min_freq_khz;
	u32 max_freq_khz;
	u32 bpp;
	u8  config[0x2F];
	u8  padding;
} __packed;

struct mwv207d_hdmi {
	struct mwv207d_output base;

	void __iomem    *mmio;
	bool            sink_is_hdmi;
	bool            sink_has_audio;
	unsigned int    tmdsclock;
	int             vic;
	int             irq;
	u32             clock;

	u32    last_connector_result;

	struct mutex    clk_lock;

	struct delayed_work hotplug_work;
	struct delayed_work scdc_work;

	u8              mc_clkdis;
	bool            rgb_limited_range;
	struct mwv207d_hdmi_phy_data phy_data[0x13];
};

static inline u8 mwv207d_hdmi_readb(struct mwv207d_hdmi *hdmi, u32 reg)
{
	return readl(hdmi->mmio + reg * 4);
}

static inline void mwv207d_hdmi_writeb(struct mwv207d_hdmi *hdmi, u8 value, u32 reg)
{
	writel_relaxed(value, hdmi->mmio + reg * 4);
}

static inline void mwv207d_hdmi_modb(struct mwv207d_hdmi *hdmi, u8 value, u8 mask, u32 reg)
{
	u8 rvalue = mwv207d_hdmi_readb(hdmi, reg);

	rvalue = (rvalue & ~mask) | (value & mask);
	mwv207d_hdmi_writeb(hdmi, rvalue, reg);
}

static void mwv207d_hdmi_mask_writeb(struct mwv207d_hdmi *hdmi, u8 data, unsigned int reg,
				u8 shift, u8 mask)
{
	mwv207d_hdmi_modb(hdmi, data << shift, mask, reg);
}

static const u16 csc_coeff_default[3][4] = {
	{ 0x2000, 0x0000, 0x0000, 0x0000 },
	{ 0x0000, 0x2000, 0x0000, 0x0000 },
	{ 0x0000, 0x0000, 0x2000, 0x0000 }
};

static const u16 csc_coeff_rgb_full_to_rgb_limited[3][4] = {
	{ 0x1b7c, 0x0000, 0x0000, 0x0020 },
	{ 0x0000, 0x1b7c, 0x0000, 0x0020 },
	{ 0x0000, 0x0000, 0x1b7c, 0x0020 }
};

static int mwv207d_hdmi_phy_data_init(struct mwv207d_hdmi *hdmi, int idx)
{
	const struct mwv207d_vdat *vdat;
	int i;

	vdat = mwv207d_vbios_vdat(hdmi->base.mdev, 0, 0x3E9 + idx);
	if (!vdat && idx != 0)
		vdat = mwv207d_vbios_vdat(hdmi->base.mdev, 0, 0x3E9);

	if (!vdat)
		return -ENOENT;
	if (vdat->len != sizeof(hdmi->phy_data))
		return -EINVAL;
	memcpy(hdmi->phy_data, vdat->dat, vdat->len);
	for (i = 0; i < 0x13; ++i) {
		hdmi->phy_data[i].min_freq_khz =
			le32_to_cpu(hdmi->phy_data[i].min_freq_khz);
		hdmi->phy_data[i].max_freq_khz =
			le32_to_cpu(hdmi->phy_data[i].max_freq_khz);
		hdmi->phy_data[i].bpp =
			le32_to_cpu(hdmi->phy_data[i].bpp);
	}

	return 0;
}

static bool mwv207d_hdmi_support_scdc(struct mwv207d_hdmi *hdmi,
				      const struct drm_display_info *display)
{

	if (!display->hdmi.scdc.supported ||
	    !display->hdmi.scdc.scrambling.supported)
		return false;

	if (!display->hdmi.scdc.scrambling.low_rates &&
	    display->max_tmds_clock <= 340000)
		return false;

	return true;
}

static void mwv207d_hdmi_set_high_tmds_clock_ratio(struct mwv207d_hdmi *hdmi)
{
	struct drm_display_info *display = &hdmi->base.connector.display_info;

	if (mwv207d_hdmi_support_scdc(hdmi, display)) {
		if (hdmi->tmdsclock > 0x1443FD00)
			drm_scdc_set_high_tmds_clock_ratio(&hdmi->base.connector, 1);
		else
			drm_scdc_set_high_tmds_clock_ratio(&hdmi->base.connector, 0);
	}
}

static void mwv207d_hdmi_phy_configure_data(struct mwv207d_hdmi *hdmi, int kfreq, int bpp)
{
	struct mwv207d_output *output = &hdmi->base;
	u32 regbase = (0x6000 + 0x800 * (output->idx));
	int i, cfg, offset;
	u8 bytes;

	for (i = 0; i < 0x13; ++i) {
		if (kfreq >= hdmi->phy_data[i].min_freq_khz
				&& kfreq <= hdmi->phy_data[i].max_freq_khz
				&& bpp == hdmi->phy_data[i].bpp)
			break;
	}
	if (i >= 0x13) {
		pr_warn("mwv207d: no matching hdmi phydata found");
		return;
	}

	if (hdmi->tmdsclock > 0x1443FD00) {
		mwv207d_hdmi_modb(hdmi, HDMI_FC_INVIDCONF_HDCP_KEEPOUT_ACTIVE,
				     HDMI_FC_INVIDCONF_HDCP_KEEPOUT_MASK,
				     0x1000);
		drm_scdc_readb(output->ddc, SCDC_SINK_VERSION, &bytes);
		drm_scdc_writeb(output->ddc, SCDC_SOURCE_VERSION,
				min_t(u8, bytes, 0x1));
		drm_scdc_set_scrambling(&output->connector, 1);
		drm_scdc_set_high_tmds_clock_ratio(&output->connector, 1);
		mwv207d_hdmi_writeb(hdmi, (u8) ~HDMI_MC_SWRSTZ_TMDSSWRST_REQ,
			    0x4002);
		mwv207d_hdmi_writeb(hdmi, 1, 0x10E1);
	} else {
		mwv207d_hdmi_writeb(hdmi, 0, 0x10E1);
		mwv207d_hdmi_writeb(hdmi, (u8) ~HDMI_MC_SWRSTZ_TMDSSWRST_REQ,
			    0x4002);
		drm_scdc_set_scrambling(&output->connector, 0);
		drm_scdc_set_high_tmds_clock_ratio(&output->connector, 0);
	}

	cfg = i;
	mwv207d_output_write(output, regbase + 0x70, 0xc8);
	mwv207d_output_write(output, regbase + 0x74, 0x2);
	for (i = 0; i < 0x2F; i++) {
		offset = 0x4 + i * 4;
		if (offset == 0x84 || offset == 0x88 || offset == 0xB8)
			continue;

		mwv207d_output_write(output, regbase + offset,
				hdmi->phy_data[cfg].config[i]);
	}
}

static void mwv207d_hdmi_phy_switch(struct mwv207d_hdmi *hdmi, int enable)
{
	struct mwv207d_output *output = &hdmi->base;
	u32 regbase = (0x6000 + 0x800 * (output->idx));

	if (enable) {
		mwv207d_output_write(output, regbase + 0x84, 0x80);
		mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
				      1 << 28, 0 << 28);
		mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
				      1, 1);
	} else {
		mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
				      1, 0);
		usleep_range(1000, 2000);
		mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
				      1 << 28, 1 << 28);
		mwv207d_output_write(output, regbase + 0x84, 0x00);
	}
	mwv207d_hdmi_mask_writeb(hdmi, enable, 0x3000,
			 HDMI_PHY_CONF0_GEN2_TXPWRON_OFFSET,
			 HDMI_PHY_CONF0_GEN2_TXPWRON_MASK);
}

static void mwv207d_hdmi_phy_gen2_pddq(struct mwv207d_hdmi *hdmi, u8 enable)
{
	mwv207d_hdmi_mask_writeb(hdmi, enable, 0x3000,
			 HDMI_PHY_CONF0_GEN2_PDDQ_OFFSET,
			 HDMI_PHY_CONF0_GEN2_PDDQ_MASK);
}

static void mwv207d_hdmi_phy_power_on(struct mwv207d_hdmi *hdmi)
{
	u8 val;
	int i;

	mwv207d_hdmi_phy_switch(hdmi, 1);

	mwv207d_hdmi_phy_gen2_pddq(hdmi, 0);

	for (i = 0; i < 5; ++i) {
		val = mwv207d_hdmi_readb(hdmi, 0x3004) & HDMI_PHY_TX_PHY_LOCK;
		if (val)
			break;
		usleep_range(1000, 2000);
	}

	if (!val)
		pr_warn("mwv207d: hdmi PHY PLL failed to lock\n");
}

static void mwv207d_hdmi_phy_power_off(struct mwv207d_hdmi *hdmi)
{
	u8 val;
	int i;

	mwv207d_hdmi_phy_switch(hdmi, 0);
	for (i = 0; i < 5; ++i) {
		val = mwv207d_hdmi_readb(hdmi, 0x3004);
		if (!(val & HDMI_PHY_TX_PHY_LOCK))
			break;

		usleep_range(1000, 2000);
	}

	if (val & HDMI_PHY_TX_PHY_LOCK)
		pr_warn("mwv207d: PHY failed to power down\n");

	mwv207d_hdmi_phy_gen2_pddq(hdmi, 1);
}

static void mwv207d_hdmi_phy_reset(struct mwv207d_hdmi *hdmi)
{

	mwv207d_hdmi_writeb(hdmi, HDMI_MC_PHYRSTZ_PHYRSTZ, 0x4005);
	mwv207d_hdmi_writeb(hdmi, 0, 0x4005);
}

static void mwv207d_hdmi_phy_enable_svsret(struct mwv207d_hdmi *hdmi, u8 enable)
{
	mwv207d_hdmi_mask_writeb(hdmi, enable, 0x3000,
			 HDMI_PHY_CONF0_SVSRET_OFFSET,
			 HDMI_PHY_CONF0_SVSRET_MASK);
}

static void mwv207d_hdmi_phy_configure(struct mwv207d_hdmi *hdmi,
				       int kfreq, int bpp)
{
	mwv207d_hdmi_phy_power_off(hdmi);

	mwv207d_hdmi_set_high_tmds_clock_ratio(hdmi);

	mwv207d_hdmi_phy_enable_svsret(hdmi, 1);
	mwv207d_hdmi_phy_reset(hdmi);
	mwv207d_hdmi_writeb(hdmi, HDMI_MC_HEACPHY_RST_ASSERT, 0x4007);

	mwv207d_hdmi_phy_configure_data(hdmi, kfreq, bpp);

	if (hdmi->tmdsclock > 0x1443FD00)
		msleep(100);

	mwv207d_hdmi_phy_power_on(hdmi);
}

static void mwv207d_hdmi_phy_sel_data_en_pol(struct mwv207d_hdmi *hdmi, u8 enable)
{
	mwv207d_hdmi_mask_writeb(hdmi, enable, 0x3000,
			 HDMI_PHY_CONF0_SELDATAENPOL_OFFSET,
			 HDMI_PHY_CONF0_SELDATAENPOL_MASK);
}

static void mwv207d_hdmi_phy_sel_interface_control(struct mwv207d_hdmi *hdmi, u8 enable)
{
	mwv207d_hdmi_mask_writeb(hdmi, enable, 0x3000,
			 HDMI_PHY_CONF0_SELDIPIF_OFFSET,
			 HDMI_PHY_CONF0_SELDIPIF_MASK);
}

static void mwv207d_hdmi_phy_init(struct mwv207d_hdmi *hdmi, int kfreq, int bpp)
{
	int i;

	for (i = 0; i < 2; i++) {
		mwv207d_hdmi_phy_sel_data_en_pol(hdmi, 1);
		mwv207d_hdmi_phy_sel_interface_control(hdmi, 0);
		mwv207d_hdmi_phy_configure(hdmi, kfreq, bpp);
	}
}

static void mwv207d_hdmi_clear_overflow(struct mwv207d_hdmi *hdmi)
{
	int i;
	u8 val;

	mwv207d_hdmi_writeb(hdmi, (u8) ~HDMI_MC_SWRSTZ_TMDSSWRST_REQ,
			    0x4002);

	val = mwv207d_hdmi_readb(hdmi, 0x1000);
	for (i = 0; i < 4; i++)
		mwv207d_hdmi_writeb(hdmi, val, 0x1000);
}

static void mwv207d_hdmi_ih_mutes(struct mwv207d_hdmi *hdmi)
{
	u8 ih_mute;

	ih_mute = mwv207d_hdmi_readb(hdmi, 0x1FF) |
		  HDMI_IH_MUTE_MUTE_WAKEUP_INTERRUPT |
		  HDMI_IH_MUTE_MUTE_ALL_INTERRUPT;

	mwv207d_hdmi_writeb(hdmi, ih_mute, 0x1FF);

	mwv207d_hdmi_writeb(hdmi, 0xff, 0x807);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x10D2);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x10D6);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x10DA);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x3006);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x3027);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x3028);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x3102);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x3302);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x3404);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x3505);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x5008);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x7E05);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x7E06);

	mwv207d_hdmi_writeb(hdmi, 0xff, 0x180);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x181);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x182);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x183);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x184);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x185);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x186);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x187);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x188);
	mwv207d_hdmi_writeb(hdmi, 0xff, 0x189);

	ih_mute &= ~(HDMI_IH_MUTE_MUTE_WAKEUP_INTERRUPT |
		    HDMI_IH_MUTE_MUTE_ALL_INTERRUPT);
	mwv207d_hdmi_writeb(hdmi, ih_mute, 0x1FF);
}

static void mwv207d_hdmi_phy_setup_hpd(struct mwv207d_hdmi *hdmi)
{

	mwv207d_hdmi_writeb(hdmi, HDMI_PHY_HPD, 0x3007);
	mwv207d_hdmi_writeb(hdmi, HDMI_IH_PHY_STAT0_HPD, 0x104);

	mwv207d_hdmi_writeb(hdmi, ~HDMI_PHY_HPD, 0x3006);

	mwv207d_hdmi_writeb(hdmi, HDMI_IH_PHY_STAT0_HPD, 0x104);
	mwv207d_hdmi_writeb(hdmi, ~HDMI_IH_PHY_STAT0_HPD, 0x184);
}

static int mwv207d_hdmi_link_reset(struct mwv207d_hdmi *hdmi,
			struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_crtc *crtc = hdmi->base.connector.state->crtc;
	struct drm_atomic_state *state;
	struct drm_crtc_state *crtc_state;
	int ret;

	state = drm_atomic_state_alloc(crtc->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = ctx;

	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state)) {
		ret = PTR_ERR(crtc_state);
		goto out;
	}

	crtc_state->connectors_changed = true;

	ret = drm_atomic_commit(state);
out:
	drm_atomic_state_put(state);

	return ret;
}

static int mwv207d_hdmi_get_modes(struct drm_connector *connector);
static int mwv207d_hdmi_link_check(struct mwv207d_hdmi *hdmi,
				struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_connector *connector = &hdmi->base.connector;
	struct drm_device *dev = &hdmi->base.mdev->base;
	struct drm_connector_state *conn_state;
	struct drm_crtc *crtc;
	int ret;

	ret = drm_modeset_lock(&dev->mode_config.connection_mutex, ctx);
	if (ret)
		return ret;

	conn_state = connector->state;
	if (!conn_state)
		return -EINVAL;

	crtc = conn_state->crtc;
	if (!crtc)
		return -EINVAL;

	ret = drm_modeset_lock(&crtc->mutex, ctx);
	if (ret)
		return ret;

	if (!crtc->state || !crtc->state->active)
		return -EINVAL;

	if (conn_state->commit &&
		!try_wait_for_completion(&conn_state->commit->hw_done))
		return -EINVAL;

	mwv207d_hdmi_get_modes(connector);

	if (!mwv207d_hdmi_support_scdc(hdmi, &connector->display_info) ||
			hdmi->tmdsclock <= 0x1443FD00)
		return -EINVAL;

	return 0;
}

static void mwv207d_hdmi_link_update(struct mwv207d_hdmi *hdmi)
{
	struct drm_device *dev = &hdmi->base.mdev->base;
	struct drm_modeset_acquire_ctx ctx;
	int ret;

	drm_modeset_acquire_init(&ctx, 0);
retry:
	ret = mwv207d_hdmi_link_check(hdmi, &ctx);

	if (ret == 0) {
		dev_dbg(dev->dev, "hdmi %d received hotplug event, do link reset", hdmi->base.idx);
		ret = mwv207d_hdmi_link_reset(hdmi, &ctx);
	}

	if (ret == -EDEADLK) {
		drm_modeset_backoff(&ctx);
		goto retry;
	}

	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);
}

static int mwv207d_hdmi_hpd_high(struct mwv207d_hdmi *hdmi)
{
	u32 value;

	mutex_lock(&hdmi->clk_lock);
	value = mwv207d_hdmi_readb(hdmi, 0x3004);
	mutex_unlock(&hdmi->clk_lock);

	return value & 0x2;
}

static void mwv207d_hdmi_scdc_work_func(struct work_struct *work)
{
	struct mwv207d_hdmi *hdmi = container_of(work, struct mwv207d_hdmi, scdc_work.work);
	struct drm_device *dev = &hdmi->base.mdev->base;
	u8 status, scdc_flag;

	drm_scdc_readb(hdmi->base.ddc, SCDC_SCRAMBLER_STATUS, &status);
	if (!(status & SCDC_SCRAMBLING_STATUS)) {
		dev_dbg(dev->dev, "hdmi %d scrambling failed\n", hdmi->base.idx);
		return;
	}

	drm_scdc_readb(hdmi->base.ddc, SCDC_STATUS_FLAGS_0, &scdc_flag);
	if (scdc_flag != 0xf) {
		dev_dbg(dev->dev, "hdmi %d scdc lock failed\n", hdmi->base.idx);
		return;
	}

	mwv207d_hdmi_writeb(hdmi, 12, 0x1011);
}

static void mwv207d_hdmi_hotplug_work_func(struct work_struct *work)
{
	struct mwv207d_hdmi *hdmi = container_of(work, struct mwv207d_hdmi, hotplug_work.work);
	struct drm_device *dev = &hdmi->base.mdev->base;
	struct drm_connector *connector = &hdmi->base.connector;
	enum drm_connector_status old_status;
	bool changed;

	mutex_lock(&dev->mode_config.mutex);

	old_status = connector->status;
	connector->status = drm_helper_probe_detect(connector, NULL, false);
	changed = old_status != connector->status;

	dev_dbg(dev->dev, "[CONNECTOR:%d:%s] status updated from %s to %s\n",
			connector->base.id,
			connector->name,
			drm_get_connector_status_name(old_status),
			drm_get_connector_status_name(connector->status));

	if (mwv207d_hdmi_hpd_high(hdmi) && connector->status == connector_status_connected)
		mwv207d_hdmi_link_update(hdmi);

	mutex_unlock(&dev->mode_config.mutex);

	if (changed)
		drm_kms_helper_hotplug_event(dev);
}

static irqreturn_t mwv207d_hdmi_hard_irq(int irq, void *dev_id)
{
	struct mwv207d_hdmi *hdmi = dev_id;
	irqreturn_t ret = IRQ_NONE;
	u8 intr_stat;

	intr_stat = mwv207d_hdmi_readb(hdmi, 0x109);
	if (intr_stat) {
		mwv207d_hdmi_writeb(hdmi, intr_stat, 0x109);

		if (intr_stat & HDMI_IH_AHBDMAAUD_STAT0_DONE)
			mwv207d_audio_isr(hdmi->base.mdev, hdmi->base.idx);

		ret = IRQ_HANDLED;
	}

	intr_stat = mwv207d_hdmi_readb(hdmi, 0x104);
	if (intr_stat) {
		mwv207d_hdmi_writeb(hdmi, ~0, 0x184);
		ret = IRQ_WAKE_THREAD;
	}

	return ret;
}

static irqreturn_t mwv207d_hdmi_irq(int irq, void *dev_id)
{
	u8 intr_stat, phy_int_pol, phy_pol_mask, phy_stat;
	struct mwv207d_hdmi *hdmi = dev_id;

	intr_stat = mwv207d_hdmi_readb(hdmi, 0x104);
	phy_int_pol = mwv207d_hdmi_readb(hdmi, 0x3007);
	phy_stat = mwv207d_hdmi_readb(hdmi, 0x3004);

	phy_pol_mask = 0;
	if (intr_stat & HDMI_IH_PHY_STAT0_HPD)
		phy_pol_mask |= HDMI_PHY_HPD;
	if (intr_stat & HDMI_IH_PHY_STAT0_RX_SENSE0)
		phy_pol_mask |= HDMI_PHY_RX_SENSE0;
	if (intr_stat & HDMI_IH_PHY_STAT0_RX_SENSE1)
		phy_pol_mask |= HDMI_PHY_RX_SENSE1;
	if (intr_stat & HDMI_IH_PHY_STAT0_RX_SENSE2)
		phy_pol_mask |= HDMI_PHY_RX_SENSE2;
	if (intr_stat & HDMI_IH_PHY_STAT0_RX_SENSE3)
		phy_pol_mask |= HDMI_PHY_RX_SENSE3;

	if (phy_pol_mask)
		mwv207d_hdmi_modb(hdmi, ~phy_int_pol,
				  phy_pol_mask, 0x3007);

	if (intr_stat & HDMI_IH_PHY_STAT0_HPD)
		queue_delayed_work(system_wq, &hdmi->hotplug_work, 0);

	mwv207d_hdmi_writeb(hdmi, intr_stat, 0x104);
	mwv207d_hdmi_writeb(hdmi, ~HDMI_IH_PHY_STAT0_HPD,
			    0x184);

	return IRQ_HANDLED;
}

static void mwv207d_hdmi_switch(struct mwv207d_output *output, bool on)
{

	if (on) {
		mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
				      1 << 28, 0 << 28);
		mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
				      1, 1);
	} else {

		mwv207d_hdmi_writeb(output_to_hdmi(output), 0x2, 0x1018);
		msleep(50);

		mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
				      1, 0);
		usleep_range(1000, 2000);
		mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
				      1 << 28, 1 << 28);

	}
}

static void mwv207d_hdmi_select_crtc(struct mwv207d_output *output)
{

	mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
			      0x3 << 4, drm_crtc_index(output->cur_crtc) << 4);

	mdev_modify(output->mdev, 0x2B0018,
		    0b111 << (8 + output->idx * 4),
		    drm_crtc_index(output->cur_crtc) << (8 + output->idx * 4));

	udelay(150);
}

static void mwv207d_hdmi_disable_overflow_interrupts(struct mwv207d_hdmi *hdmi)
{
	mwv207d_hdmi_writeb(hdmi, HDMI_IH_MUTE_FC_STAT2_OVERFLOW_MASK,
			    0x182);
}

static void mwv207d_hdmi_av_composer(struct mwv207d_hdmi *hdmi,
				     const struct drm_display_mode *mode)
{
	const struct drm_display_info *display = &hdmi->base.connector.display_info;
	const struct drm_hdmi_info *hdmi_info = &display->hdmi;
	int hblank, vblank, h_de_hs, v_de_vs, hsync_len, vsync_len;
	struct mwv207d_output *output = &hdmi->base;
	unsigned int vdisplay, hdisplay;
	u32 tmdsclock, rate;
	u8 inv_val, bytes;

	hdmi->tmdsclock = tmdsclock = mode->clock * 1000;
	hdmi->clock = mode->clock;

	mwv207d_hdmi_writeb(hdmi, 0x1, 0x1018);

	inv_val = (mwv207d_hdmi_support_scdc(hdmi, display) &&
		   (tmdsclock > 0x1443FD00 ||
		   hdmi_info->scdc.scrambling.low_rates) ?
		HDMI_FC_INVIDCONF_HDCP_KEEPOUT_ACTIVE :
		HDMI_FC_INVIDCONF_HDCP_KEEPOUT_INACTIVE);

	inv_val |= mode->flags & DRM_MODE_FLAG_PVSYNC ?
		HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_HIGH :
		HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_LOW;

	inv_val |= mode->flags & DRM_MODE_FLAG_PHSYNC ?
		HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_HIGH :
		HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_LOW;

	inv_val |= HDMI_FC_INVIDCONF_DE_IN_POLARITY_ACTIVE_HIGH;

	if (hdmi->vic == 39)
		inv_val |= HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_HIGH;
	else
		inv_val |= mode->flags & DRM_MODE_FLAG_INTERLACE ?
			HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_HIGH :
			HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_LOW;

	inv_val |= mode->flags & DRM_MODE_FLAG_INTERLACE ?
		HDMI_FC_INVIDCONF_IN_I_P_INTERLACED :
		HDMI_FC_INVIDCONF_IN_I_P_PROGRESSIVE;

	inv_val |= hdmi->sink_is_hdmi ?
		HDMI_FC_INVIDCONF_DVI_MODEZ_HDMI_MODE :
		HDMI_FC_INVIDCONF_DVI_MODEZ_DVI_MODE;

	mwv207d_hdmi_writeb(hdmi, inv_val, 0x1000);

	rate = drm_mode_vrefresh(mode) * 1000;
	mwv207d_hdmi_writeb(hdmi, rate >> 16, 0x1010);
	mwv207d_hdmi_writeb(hdmi, rate >> 8, 0x100F);
	mwv207d_hdmi_writeb(hdmi, rate, 0x100E);

	mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)), 0x1 << 9,
			(mode->flags & DRM_MODE_FLAG_PHSYNC ? 1 : 0) << 9);
	mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)), 0x1 << 8,
			(mode->flags & DRM_MODE_FLAG_PVSYNC ? 1 : 0) << 8);
	mwv207d_output_modify(output, (0x400 + 0x100 * (output->idx)),
			     0x1 << 10, 1 << 10);

	hdisplay = mode->hdisplay;
	hblank = mode->htotal - mode->hdisplay;
	h_de_hs = mode->hsync_start - mode->hdisplay;
	hsync_len = mode->hsync_end - mode->hsync_start;

	vdisplay = mode->vdisplay;
	vblank = mode->vtotal - mode->vdisplay;
	v_de_vs = mode->vsync_start - mode->vdisplay;
	vsync_len = mode->vsync_end - mode->vsync_start;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE) {
		vdisplay /= 2;
		vblank /= 2;
		v_de_vs /= 2;
		vsync_len /= 2;
	}

	if (mwv207d_hdmi_support_scdc(hdmi, display)) {
		if (tmdsclock > 0x1443FD00 ||
		    hdmi_info->scdc.scrambling.low_rates) {
			drm_scdc_readb(hdmi->base.ddc, SCDC_SINK_VERSION,
				       &bytes);
			drm_scdc_writeb(hdmi->base.ddc, SCDC_SOURCE_VERSION,
				min_t(u8, bytes, 0x1));

			drm_scdc_set_scrambling(&hdmi->base.connector, 1);

			mwv207d_hdmi_writeb(hdmi, (u8)~HDMI_MC_SWRSTZ_TMDSSWRST_REQ,
					    0x4002);
			mwv207d_hdmi_writeb(hdmi, 1, 0x10E1);
		} else {
			mwv207d_hdmi_writeb(hdmi, 0, 0x10E1);
			mwv207d_hdmi_writeb(hdmi, (u8)~HDMI_MC_SWRSTZ_TMDSSWRST_REQ,
					    0x4002);
			drm_scdc_set_scrambling(&hdmi->base.connector, 0);
		}
	}

	mwv207d_hdmi_writeb(hdmi, hdisplay >> 8, 0x1002);
	mwv207d_hdmi_writeb(hdmi, hdisplay, 0x1001);

	mwv207d_hdmi_writeb(hdmi, vdisplay >> 8, 0x1006);
	mwv207d_hdmi_writeb(hdmi, vdisplay, 0x1005);

	mwv207d_hdmi_writeb(hdmi, hblank >> 8, 0x1004);
	mwv207d_hdmi_writeb(hdmi, hblank, 0x1003);

	mwv207d_hdmi_writeb(hdmi, vblank >> 8, 0x102E);
	mwv207d_hdmi_writeb(hdmi, vblank, 0x1007);

	mwv207d_hdmi_writeb(hdmi, h_de_hs >> 8, 0x1009);
	mwv207d_hdmi_writeb(hdmi, h_de_hs, 0x1008);

	mwv207d_hdmi_writeb(hdmi, v_de_vs >> 8, 0x102F);
	mwv207d_hdmi_writeb(hdmi, v_de_vs, 0x100C);

	mwv207d_hdmi_writeb(hdmi, hsync_len >> 8, 0x100B);
	mwv207d_hdmi_writeb(hdmi, hsync_len, 0x100A);

	mwv207d_hdmi_writeb(hdmi, vsync_len, 0x100D);
}

static void mwv207d_hdmi_enable_video_path(struct mwv207d_hdmi *hdmi)
{

	if (hdmi->tmdsclock > 0x1443FD00)
		mwv207d_hdmi_writeb(hdmi, 255, 0x1011);
	else
		mwv207d_hdmi_writeb(hdmi, 12, 0x1011);
	mwv207d_hdmi_writeb(hdmi, 32, 0x1012);
	mwv207d_hdmi_writeb(hdmi, 1, 0x1013);

	mwv207d_hdmi_writeb(hdmi, 0x0B, 0x1014);
	mwv207d_hdmi_writeb(hdmi, 0x16, 0x1015);
	mwv207d_hdmi_writeb(hdmi, 0x21, 0x1016);

	hdmi->mc_clkdis |= HDMI_MC_CLKDIS_HDCPCLK_DISABLE |
			   HDMI_MC_CLKDIS_AUDCLK_DISABLE |
			   HDMI_MC_CLKDIS_CSCCLK_DISABLE |
			   HDMI_MC_CLKDIS_PREPCLK_DISABLE |
			   HDMI_MC_CLKDIS_TMDSCLK_DISABLE;
	hdmi->mc_clkdis &= ~HDMI_MC_CLKDIS_PIXELCLK_DISABLE;
	mwv207d_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);

	hdmi->mc_clkdis &= ~HDMI_MC_CLKDIS_TMDSCLK_DISABLE;
	mwv207d_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);

	if (hdmi->rgb_limited_range) {
		hdmi->mc_clkdis &= ~HDMI_MC_CLKDIS_CSCCLK_DISABLE;
		mwv207d_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);

		mwv207d_hdmi_writeb(hdmi, HDMI_MC_FLOWCTRL_FEED_THROUGH_OFF_CSC_IN_PATH,
				    0x4004);
	} else {
		hdmi->mc_clkdis |= HDMI_MC_CLKDIS_CSCCLK_DISABLE;
		mwv207d_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);

		mwv207d_hdmi_writeb(hdmi, HDMI_MC_FLOWCTRL_FEED_THROUGH_OFF_CSC_BYPASS,
				    0x4004);
	}
}

static void mwv207d_hdmi_enable_audio_clk(struct mwv207d_hdmi *hdmi, bool enable)
{
	if (enable)
		hdmi->mc_clkdis &= ~HDMI_MC_CLKDIS_AUDCLK_DISABLE;
	else
		hdmi->mc_clkdis |= HDMI_MC_CLKDIS_AUDCLK_DISABLE;
	mwv207d_hdmi_writeb(hdmi, hdmi->mc_clkdis, 0x4001);
}

static void mwv207d_hdmi_config_AVI(struct mwv207d_hdmi *hdmi,
				    struct drm_connector *connector,
				    const struct drm_display_mode *mode)
{
	struct hdmi_avi_infoframe frame;
	u8 val;

	drm_hdmi_avi_infoframe_from_display_mode(&frame, connector, mode);

	drm_hdmi_avi_infoframe_quant_range(&frame, connector, mode,
					   hdmi->rgb_limited_range ?
					   HDMI_QUANTIZATION_RANGE_LIMITED :
					   HDMI_QUANTIZATION_RANGE_FULL);

	frame.colorspace = HDMI_COLORSPACE_RGB;
	frame.colorimetry = HDMI_COLORIMETRY_NONE;
	frame.extended_colorimetry = HDMI_EXTENDED_COLORIMETRY_XV_YCC_601;

	val = (frame.scan_mode & 3) << 4 | (frame.colorspace & 3);
	if (frame.active_aspect & 15)
		val |= HDMI_FC_AVICONF0_ACTIVE_FMT_INFO_PRESENT;
	if (frame.top_bar || frame.bottom_bar)
		val |= HDMI_FC_AVICONF0_BAR_DATA_HORIZ_BAR;
	if (frame.left_bar || frame.right_bar)
		val |= HDMI_FC_AVICONF0_BAR_DATA_VERT_BAR;
	mwv207d_hdmi_writeb(hdmi, val, 0x1019);

	val = ((frame.colorimetry & 0x3) << 6) |
	      ((frame.picture_aspect & 0x3) << 4) |
	      (frame.active_aspect & 0xf);
	mwv207d_hdmi_writeb(hdmi, val, 0x101A);

	val = ((frame.extended_colorimetry & 0x7) << 4) |
	      ((frame.quantization_range & 0x3) << 2) |
	      (frame.nups & 0x3);
	if (frame.itc)
		val |= HDMI_FC_AVICONF2_IT_CONTENT_VALID;
	mwv207d_hdmi_writeb(hdmi, val, 0x101B);

	val = frame.video_code & 0x7f;
	mwv207d_hdmi_writeb(hdmi, val, 0x101C);

	val = ((1 << HDMI_FC_PRCONF_INCOMING_PR_FACTOR_OFFSET) &
		HDMI_FC_PRCONF_INCOMING_PR_FACTOR_MASK);
	mwv207d_hdmi_writeb(hdmi, val, 0x10E0);

	val = ((frame.ycc_quantization_range & 0x3) << 2) |
	      (frame.content_type & 0x3);
	mwv207d_hdmi_writeb(hdmi, val, 0x1017);

	mwv207d_hdmi_writeb(hdmi, frame.top_bar & 0xff, 0x101D);
	mwv207d_hdmi_writeb(hdmi, (frame.top_bar >> 8) & 0xff, 0x101E);
	mwv207d_hdmi_writeb(hdmi, frame.bottom_bar & 0xff, 0x101F);
	mwv207d_hdmi_writeb(hdmi, (frame.bottom_bar >> 8) & 0xff,
			    0x1020);
	mwv207d_hdmi_writeb(hdmi, frame.left_bar & 0xff, 0x1021);
	mwv207d_hdmi_writeb(hdmi, (frame.left_bar >> 8) & 0xff,
			    0x1022);
	mwv207d_hdmi_writeb(hdmi, frame.right_bar & 0xff, 0x1023);
	mwv207d_hdmi_writeb(hdmi, (frame.right_bar >> 8) & 0xff,
			    0x1024);
}

static void mwv207d_hdmi_config_vendor_specific_infoframe(
		struct mwv207d_hdmi *hdmi,
		struct drm_connector *connector,
		const struct drm_display_mode *mode)
{
	struct hdmi_vendor_infoframe frame;
	u8 buffer[10];
	ssize_t err;

	err = drm_hdmi_vendor_infoframe_from_display_mode(&frame, connector,
							  mode);
	if (err < 0)
		return;

	err = hdmi_vendor_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0) {
		DRM_ERROR("Failed to pack vendor infoframe: %zd\n", err);
		return;
	}
	mwv207d_hdmi_mask_writeb(hdmi, 0, 0x10B3,
				 HDMI_FC_DATAUTO0_VSD_OFFSET,
				 HDMI_FC_DATAUTO0_VSD_MASK);

	mwv207d_hdmi_writeb(hdmi, buffer[2], 0x102A);

	mwv207d_hdmi_writeb(hdmi, buffer[4], 0x1029);
	mwv207d_hdmi_writeb(hdmi, buffer[5], 0x1030);
	mwv207d_hdmi_writeb(hdmi, buffer[6], 0x1031);

	mwv207d_hdmi_writeb(hdmi, buffer[7], 0x1032);
	mwv207d_hdmi_writeb(hdmi, buffer[8], 0x1033);

	if (frame.s3d_struct >= HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF)
		mwv207d_hdmi_writeb(hdmi, buffer[9], 0x1034);

	mwv207d_hdmi_writeb(hdmi, 1, 0x10B4);

	mwv207d_hdmi_writeb(hdmi, 0x11, 0x10B5);

	mwv207d_hdmi_mask_writeb(hdmi, 1, 0x10B3,
				 HDMI_FC_DATAUTO0_VSD_OFFSET,
				 HDMI_FC_DATAUTO0_VSD_MASK);
}

static void mwv207d_hdmi_config_drm_infoframe(struct mwv207d_hdmi *hdmi,
					const struct drm_connector *connector)
{
	const struct drm_connector_state *conn_state = connector->state;
	struct hdmi_drm_infoframe frame;
	u8 buffer[30];
	ssize_t err;
	int i;

	mwv207d_hdmi_modb(hdmi, HDMI_FC_PACKET_TX_EN_DRM_DISABLE,
			  HDMI_FC_PACKET_TX_EN_DRM_MASK, 0x10E3);

	err = drm_hdmi_infoframe_set_hdr_metadata(&frame, conn_state);
	if (err < 0)
		return;

	err = hdmi_drm_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0)
		return;

	mwv207d_hdmi_writeb(hdmi, frame.version, 0x1168);
	mwv207d_hdmi_writeb(hdmi, frame.length, 0x1169);

	for (i = 0; i < frame.length; i++)
		mwv207d_hdmi_writeb(hdmi, buffer[4 + i], 0x116A + i);

	mwv207d_hdmi_writeb(hdmi, 1, 0x1167);
	mwv207d_hdmi_modb(hdmi, HDMI_FC_PACKET_TX_EN_DRM_ENABLE,
			  HDMI_FC_PACKET_TX_EN_DRM_MASK, 0x10E3);
}

static void mwv207d_hdmi_video_packetize(struct mwv207d_hdmi *hdmi)
{
	unsigned int output_select = HDMI_VP_CONF_OUTPUT_SELECTOR_BYPASS;
	unsigned int remap_size = HDMI_VP_REMAP_YCC422_16bit;
	unsigned int color_depth = 4;
	u8 val, vp_conf;

	val = ((color_depth << HDMI_VP_PR_CD_COLOR_DEPTH_OFFSET) &
		HDMI_VP_PR_CD_COLOR_DEPTH_MASK);
	mwv207d_hdmi_writeb(hdmi, val, 0x801);

	mwv207d_hdmi_modb(hdmi, HDMI_VP_STUFF_PR_STUFFING_STUFFING_MODE,
			  HDMI_VP_STUFF_PR_STUFFING_MASK, 0x802);

	vp_conf = HDMI_VP_CONF_PR_EN_DISABLE |
		  HDMI_VP_CONF_BYPASS_SELECT_VID_PACKETIZER;

	mwv207d_hdmi_modb(hdmi, vp_conf,
		  HDMI_VP_CONF_PR_EN_MASK |
		  HDMI_VP_CONF_BYPASS_SELECT_MASK, 0x804);

	mwv207d_hdmi_modb(hdmi, 1 << HDMI_VP_STUFF_IDEFAULT_PHASE_OFFSET,
			  HDMI_VP_STUFF_IDEFAULT_PHASE_MASK, 0x802);

	mwv207d_hdmi_writeb(hdmi, remap_size, 0x803);

	vp_conf = HDMI_VP_CONF_BYPASS_EN_ENABLE |
		  HDMI_VP_CONF_PP_EN_DISABLE |
		  HDMI_VP_CONF_YCC422_EN_DISABLE;

	mwv207d_hdmi_modb(hdmi, vp_conf,
		  HDMI_VP_CONF_BYPASS_EN_MASK | HDMI_VP_CONF_PP_EN_ENMASK |
		  HDMI_VP_CONF_YCC422_EN_MASK, 0x804);

	mwv207d_hdmi_modb(hdmi, HDMI_VP_STUFF_PP_STUFFING_STUFFING_MODE |
			  HDMI_VP_STUFF_YCC422_STUFFING_STUFFING_MODE,
			  HDMI_VP_STUFF_PP_STUFFING_MASK |
			  HDMI_VP_STUFF_YCC422_STUFFING_MASK, 0x802);

	mwv207d_hdmi_modb(hdmi, output_select, HDMI_VP_CONF_OUTPUT_SELECTOR_MASK,
			  0x804);
}

static void mwv207d_hdmi_update_csc_coeffs(struct mwv207d_hdmi *hdmi)
{
	const u16 (*csc_coeff)[3][4] = &csc_coeff_default;
	unsigned int i;
	u32 csc_scale = 1;

	if (hdmi->rgb_limited_range)
		csc_coeff = &csc_coeff_rgb_full_to_rgb_limited;
	else
		csc_coeff  = &csc_coeff_default;

	for (i = 0; i < ARRAY_SIZE(csc_coeff_default[0]); i++) {
		u16 coeff_a = (*csc_coeff)[0][i];
		u16 coeff_b = (*csc_coeff)[1][i];
		u16 coeff_c = (*csc_coeff)[2][i];

		mwv207d_hdmi_writeb(hdmi, coeff_a & 0xff,
				    0x4103 + i * 2);
		mwv207d_hdmi_writeb(hdmi, coeff_a >> 8,
				    0x4102 + i * 2);
		mwv207d_hdmi_writeb(hdmi, coeff_b & 0xff,
				    0x410B + i * 2);
		mwv207d_hdmi_writeb(hdmi, coeff_b >> 8,
				    0x410A + i * 2);
		mwv207d_hdmi_writeb(hdmi, coeff_c & 0xff,
				    0x4113 + i * 2);
		mwv207d_hdmi_writeb(hdmi, coeff_c >> 8,
				    0x4112 + i * 2);
	}

	mwv207d_hdmi_modb(hdmi, csc_scale, HDMI_CSC_SCALE_CSCSCALE_MASK,
			  0x4101);
}

static void mwv207d_hdmi_video_csc(struct mwv207d_hdmi *hdmi)
{
	int color_depth = HDMI_CSC_SCALE_CSC_COLORDE_PTH_24BPP;

	mwv207d_hdmi_writeb(hdmi, 0, 0x4100);
	mwv207d_hdmi_modb(hdmi, color_depth,
			  HDMI_CSC_SCALE_CSC_COLORDE_PTH_MASK, 0x4101);

	mwv207d_hdmi_update_csc_coeffs(hdmi);
}

static void mwv207d_hdmi_video_sample(struct mwv207d_hdmi *hdmi)
{
	int color_format = 1;
	u8 val;

	val = HDMI_TX_INVID0_INTERNAL_DE_GENERATOR_DISABLE |
		((color_format << HDMI_TX_INVID0_VIDEO_MAPPING_OFFSET) &
		HDMI_TX_INVID0_VIDEO_MAPPING_MASK);
	mwv207d_hdmi_writeb(hdmi, val, 0x200);

	val = HDMI_TX_INSTUFFING_BDBDATA_STUFFING_ENABLE |
	      HDMI_TX_INSTUFFING_RCRDATA_STUFFING_ENABLE |
	      HDMI_TX_INSTUFFING_GYDATA_STUFFING_ENABLE;
	mwv207d_hdmi_writeb(hdmi, val, 0x201);
	mwv207d_hdmi_writeb(hdmi, 0x0, 0x202);
	mwv207d_hdmi_writeb(hdmi, 0x0, 0x203);
	mwv207d_hdmi_writeb(hdmi, 0x0, 0x204);
	mwv207d_hdmi_writeb(hdmi, 0x0, 0x205);
	mwv207d_hdmi_writeb(hdmi, 0x0, 0x206);
	mwv207d_hdmi_writeb(hdmi, 0x0, 0x207);
}

static void mwv207d_hdmi_tx_hdcp_config(struct mwv207d_hdmi *hdmi)
{
	u8 de = HDMI_A_VIDPOLCFG_DATAENPOL_ACTIVE_HIGH;

	mwv207d_hdmi_modb(hdmi, HDMI_A_HDCPCFG0_RXDETECT_DISABLE,
			  HDMI_A_HDCPCFG0_RXDETECT_MASK, 0x5000);

	mwv207d_hdmi_modb(hdmi, de, HDMI_A_VIDPOLCFG_DATAENPOL_MASK,
			  0x5009);

	mwv207d_hdmi_modb(hdmi, HDMI_A_HDCPCFG1_ENCRYPTIONDISABLE_DISABLE,
			  HDMI_A_HDCPCFG1_ENCRYPTIONDISABLE_MASK,
			  0x5001);
}

static void mwv207d_hdmi_set_timing(struct mwv207d_hdmi *hdmi)
{
	struct drm_display_mode *mode;
	struct drm_connector *conn = &hdmi->base.connector;

	mwv207d_hdmi_disable_overflow_interrupts(hdmi);

	mode = &hdmi->base.cur_crtc->state->adjusted_mode;
	hdmi->vic = drm_match_cea_mode(mode);

	hdmi->rgb_limited_range = hdmi->sink_is_hdmi &&
				  drm_default_rgb_quant_range(mode) ==
				  HDMI_QUANTIZATION_RANGE_LIMITED;

	mwv207d_hdmi_av_composer(hdmi, mode);

	mwv207d_hdmi_switch(&hdmi->base, true);

	mwv207d_hdmi_phy_init(hdmi, mode->clock, 8);

	mwv207d_hdmi_enable_video_path(hdmi);

	if (hdmi->sink_is_hdmi) {
		struct drm_connector *conn = &hdmi->base.connector;

		mwv207d_hdmi_config_AVI(hdmi, conn, mode);
		mwv207d_hdmi_config_vendor_specific_infoframe(hdmi, conn, mode);
		mwv207d_hdmi_config_drm_infoframe(hdmi, conn);
	}

	mwv207d_hdmi_video_packetize(hdmi);
	mwv207d_hdmi_video_csc(hdmi);
	mwv207d_hdmi_video_sample(hdmi);
	mwv207d_hdmi_tx_hdcp_config(hdmi);

	mwv207d_hdmi_clear_overflow(hdmi);

	if (hdmi->sink_has_audio) {

		mwv207d_hdmi_enable_audio_clk(hdmi, 1);
		mwv207d_audio_switch(hdmi->base.mdev, hdmi->base.idx, conn->eld,
				hdmi->clock, true, true);
	}

	if (hdmi->tmdsclock > 0x1443FD00)
		queue_delayed_work(system_wq, &hdmi->scdc_work,	msecs_to_jiffies(3000));
}

static int mwv207d_hdmi_get_modes(struct drm_connector *connector)
{
	struct mwv207d_hdmi *hdmi = connector_to_hdmi(connector);
	int count;

	count = mwv207d_output_get_modes(connector);

	if (connector->edid_blob_ptr) {
		struct edid *edid;

		edid = (struct edid *)connector->edid_blob_ptr->data;
		hdmi->sink_is_hdmi = drm_detect_hdmi_monitor(edid);
		hdmi->sink_has_audio = drm_detect_monitor_audio(edid);
	} else {
		hdmi->sink_is_hdmi = false;
		hdmi->sink_has_audio = false;
	}

	return count;
}

static enum drm_mode_status mwv207d_hdmi_mode_valid(struct mwv207d_hdmi *hdmi,
		const struct drm_display_mode *mode)
{

	if (mode->clock > 594000)
		return MODE_CLOCK_HIGH;

	if (!hdmi->sink_is_hdmi && mode->clock >= 165000)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static enum drm_mode_status mwv207d_hdmi_connector_mode_valid(
		struct drm_connector *connector,
		struct drm_display_mode *mode)
{
	struct mwv207d_hdmi *hdmi = connector_to_hdmi(connector);

	return mwv207d_hdmi_mode_valid(hdmi, mode);
}

static int mwv207d_hdmi_detect_ctx(struct drm_connector *connector,
				   struct drm_modeset_acquire_ctx *ctx,
				   bool force)
{
	struct mwv207d_hdmi *hdmi = connector_to_hdmi(connector);
	struct mwv207d_output *output = connector_to_output(connector);
	u32 value, status;

	if (connector->force == DRM_FORCE_OFF)
		status = connector_status_disconnected;

	if (connector->force == DRM_FORCE_ON)
		status = connector_status_connected;

	if (output->forced_edid)
		status = connector_status_connected;

	if (mwv207d_i2c_probe(hdmi->base.ddc)) {
		status = connector_status_connected;
		goto out;
	}

	mutex_lock(&hdmi->clk_lock);
	value = mwv207d_hdmi_readb(hdmi, 0x3004);
	mutex_unlock(&hdmi->clk_lock);
	if (value & 0x2)
		status = connector_status_connected;
	else
		status = connector_status_disconnected;

out:
	hdmi->last_connector_result = status;

	return status;
}

static void mwv207d_hdmi_destroy(struct drm_connector *conn)
{
	struct mwv207d_hdmi *hdmi = connector_to_hdmi(conn);

	if (!hdmi->base.mdev->isr_poll)
		free_irq(hdmi->irq, hdmi);
	cancel_delayed_work_sync(&hdmi->hotplug_work);
	cancel_delayed_work_sync(&hdmi->scdc_work);
	drm_connector_cleanup(conn);
}

static const struct drm_connector_helper_funcs mwv207d_hdmi_connector_helper_funcs = {
	.get_modes  = mwv207d_hdmi_get_modes,
	.mode_valid = mwv207d_hdmi_connector_mode_valid,
	.detect_ctx = mwv207d_hdmi_detect_ctx
};

static const struct drm_connector_funcs mwv207d_hdmi_connector_funcs = {
	.reset                  = drm_atomic_helper_connector_reset,
	.fill_modes             = drm_helper_probe_single_connector_modes,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
	.destroy                = mwv207d_hdmi_destroy,
};

static enum drm_mode_status mwv207d_hdmi_encoder_mode_valid(
		struct drm_encoder *encoder,
		const struct drm_display_mode *mode)
{
	struct mwv207d_hdmi *hdmi = encoder_to_hdmi(encoder);

	return mwv207d_hdmi_mode_valid(hdmi, mode);
}

static int mwv207d_hdmi_encoder_atomic_check(
		struct drm_encoder *encoder,
		struct drm_crtc_state *crtc_state,
		struct drm_connector_state *conn_state)
{
	return 0;
}

static void mwv207d_hdmi_encoder_enable(struct drm_encoder *encoder)
{
	struct mwv207d_output *output = encoder_to_output(encoder);

	mwv207d_hdmi_select_crtc(output);

	mwv207d_hdmi_set_timing(encoder_to_hdmi(encoder));
}

static void mwv207d_hdmi_encoder_disable(struct drm_encoder *encoder)
{
	struct mwv207d_output *output = encoder_to_output(encoder);
	struct mwv207d_hdmi *hdmi = encoder_to_hdmi(encoder);

	cancel_delayed_work_sync(&hdmi->scdc_work);

	if (hdmi->sink_has_audio)
		mwv207d_audio_switch(output->mdev, output->idx, output->connector.eld,
				hdmi->clock, false, hdmi->last_connector_result);

	mwv207d_hdmi_switch(output, false);
}

static void mwv207d_hdmi_reset_clk(struct mwv207d_hdmi *hdmi)
{
	struct mwv207d_output *output = &hdmi->base;

	mutex_lock(&hdmi->clk_lock);
	mdev_modify(output->mdev, 0x2B0014,
			     0x10 << output->idx, 0);
	mdev_modify(output->mdev, 0x2B0010,
			     0x10 << output->idx, 0);
	msleep(20);
	mdev_modify(output->mdev, 0x2B0010,
			     0x10 << output->idx, 0x10 << output->idx);
	mdev_modify(output->mdev, 0x2B0014,
			     0x10 << output->idx, 0x10 << output->idx);
	mutex_unlock(&hdmi->clk_lock);
}

static void mwv207d_hdmi_encoder_reset(struct drm_encoder *encoder)
{
	struct mwv207d_output *output = encoder_to_output(encoder);
	struct mwv207d_hdmi *hdmi = encoder_to_hdmi(encoder);
	int ret = 0;

	if (!hdmi->base.mdev->isr_poll)
		free_irq(hdmi->irq, hdmi);
	mwv207d_hdmi_reset_clk(hdmi);
	mwv207d_hdmi_encoder_disable(encoder);
	mwv207d_hdmi_ih_mutes(hdmi);
	mwv207d_hdmi_phy_setup_hpd(hdmi);
	if (!hdmi->base.mdev->isr_poll)
		ret = request_threaded_irq(hdmi->irq,
				mwv207d_hdmi_hard_irq, mwv207d_hdmi_irq,
				IRQF_SHARED, output->encoder.name, hdmi);
	if (ret)
		dev_err(encoder->dev->dev, "hdmi request irq failed :%d", ret);
}

static const struct drm_encoder_funcs mwv207d_hdmi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
	.reset   = mwv207d_hdmi_encoder_reset,
};

static const struct drm_encoder_helper_funcs mwv207d_hdmi_encoder_helper_funcs = {
	.mode_valid      = mwv207d_hdmi_encoder_mode_valid,
	.atomic_check    = mwv207d_hdmi_encoder_atomic_check,
	.enable          = mwv207d_hdmi_encoder_enable,
	.disable         = mwv207d_hdmi_encoder_disable,
};

static int mwv207d_hdmi_init_single(struct mwv207d_device *mdev, int idx)
{
	struct mwv207d_output *output;
	struct mwv207d_hdmi *hdmi;
	int ret;

	hdmi = mwv207d_output_probe_and_init(mdev, sizeof(struct mwv207d_hdmi), idx,
					MWV207D_HDMI0_I2C_CHAN + idx,
					(20*0) + idx * 20);

	if (!hdmi) {
		dev_info(mdev->dev, "hdmi%d is disabled by firmware", idx);
		return 0;
	}

	if (IS_ERR(hdmi))
		return PTR_ERR(hdmi);

	output = &hdmi->base;
	hdmi->mmio = mdev->mmio + (0x400000 + 0x40000 * (idx));
	hdmi->mc_clkdis = 0x7f;
	hdmi->last_connector_result = connector_status_unknown;
	mutex_init(&hdmi->clk_lock);

	ret = mwv207d_hdmi_phy_data_init(hdmi, idx);
	if (ret)
		return ret;

	if (!mdev->isr_poll)
		output->connector.polled = DRM_CONNECTOR_POLL_HPD;
	else
		output->connector.polled = DRM_CONNECTOR_POLL_DISCONNECT | DRM_CONNECTOR_POLL_CONNECT;
	ret = drm_connector_init_with_ddc(&mdev->base, &output->connector,
				 &mwv207d_hdmi_connector_funcs,
				 DRM_MODE_CONNECTOR_HDMIA, output->ddc);
	if (ret)
		return ret;
	drm_connector_helper_add(&output->connector,
				 &mwv207d_hdmi_connector_helper_funcs);

	output->encoder.possible_crtcs = (1 << mdev->base.mode_config.num_crtc) - 1;
	ret = drm_encoder_init(&mdev->base, &output->encoder,
			       &mwv207d_hdmi_encoder_funcs,
			       DRM_MODE_ENCODER_TMDS,
			       "hdmi-%d", output->idx);
	if (ret)
		return ret;
	drm_encoder_helper_add(&output->encoder,
			       &mwv207d_hdmi_encoder_helper_funcs);

	ret = drm_connector_attach_encoder(&output->connector,
					   &output->encoder);
	if (ret)
		return ret;

	INIT_DELAYED_WORK(&hdmi->hotplug_work, mwv207d_hdmi_hotplug_work_func);
	INIT_DELAYED_WORK(&hdmi->scdc_work, mwv207d_hdmi_scdc_work_func);

	if (!mdev->isr_poll) {
		hdmi->irq = mwv207d_irq_find(mdev, 0x18 + output->idx, 0);
		BUG_ON(hdmi->irq == 0);

		ret = request_threaded_irq(hdmi->irq,
			       mwv207d_hdmi_hard_irq, mwv207d_hdmi_irq,
			       IRQF_SHARED, output->encoder.name, hdmi);
		if (ret)
		    return ret;
	}

	return 0;
}

int mwv207d_hdmi_init(struct mwv207d_device *mdev)
{
	int i, ret;

	for (i = 0; i < 0x4; i++) {
		ret = mwv207d_hdmi_init_single(mdev, i);
		if (ret)
			return ret;
	}
	return 0;
}
