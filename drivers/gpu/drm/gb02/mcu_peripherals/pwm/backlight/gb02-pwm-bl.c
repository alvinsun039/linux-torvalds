// SPDX-License-Identifier: GPL-2.0-only
/*
 * Simple PWM based backlight control, board code has to setup
 * 1) pin configuration so PWM waveforms can output
 * 2) platform_data being correctly configured
 */

#include <linux/version.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include "common/gb-peripherals-common.h"
#include "mcu_peripherals/gpio/pinctrl-gb02-mcu.h"
#include "../gb_pwm.h"
#include "ip/gb_dp_dptx.h"

//#define BL_DEBUG
#ifdef BL_DEBUG
#define GB02MAC1759(arg...) gb_printf(KERN_INFO, "[" GB02_PWM_BL_DEV_NAME "] " arg)
#else
#define GB02MAC1759(arg...)
#endif
#define GB02MAC1761 16000
#define GB02MAC1764 (GB02MAC1761/100)
#define GB02MAC1767 0
#define GB02MAC1770 1
#define GB02MAC1773 2
#define GB02MAC1776 3
#define GB02MAC1778 27000
#define GB02MAC1781 (0x1f << 0)

static struct GB02STR154 *g_pb[GB02MAC658];

struct GB02STR154 {
	struct GB02STR169 *pwm;
	u32 level;
	u32 min;
	u32 max;
	unsigned int period;
	unsigned int lth_brightness;
	unsigned int cur_brightness;
	bool enabled;
	unsigned int scale;
	bool legacy;
	struct dptx *dptx;
	bool is_edp;
	u8 edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE];
	u32 pwm_freq_hz;
};

static int GB02FUNC1189(struct GB02STR154 *pb, int brightness)
{
	unsigned int lth = pb->lth_brightness;
	u64 duty_cycle;

	duty_cycle = brightness * lth;

	return duty_cycle;
}

static u32 scale(u32 source_val,
		 u32 source_min, u32 source_max,
		 u32 target_min, u32 target_max)
{
	u64 target_val;

	WARN_ON(source_min > source_max);
	WARN_ON(target_min > target_max);

	/* defensive */
	source_val = clamp(source_val, source_min, source_max);

	/* avoid overflows */
	target_val = (u64)((source_val - source_min) * (target_max - target_min));
	target_val = DIV_ROUND_CLOSEST_ULL(target_val, source_max - source_min);
	target_val += target_min;

	return target_val;
}

static void GB02FUNC1193(struct GB02STR154 *pb, bool enable)
{
	u8 reg_val = 0;
	struct dptx *dptx = pb->dptx;

	/* Early return when display use other mechanism to enable backlight. */
	if (!(pb->edp_dpcd[1] & DP_EDP_BACKLIGHT_AUX_ENABLE_CAP))
		return;

	if (GB02FUNC1151(dptx, DP_EDP_DISPLAY_CONTROL_REGISTER,
			      &reg_val) < 0) {
		pr_err("Failed to read DPCD register 0x%x\n",
			      DP_EDP_DISPLAY_CONTROL_REGISTER);
		return;
	}
	if (enable)
		reg_val |= DP_EDP_BACKLIGHT_ENABLE;
	else
		reg_val &= ~(DP_EDP_BACKLIGHT_ENABLE);

	if (GB02FUNC1153(dptx, DP_EDP_DISPLAY_CONTROL_REGISTER,
			       reg_val) != 1) {
		pr_err("Failed to %s aux backlight\n",
			      enable ? "enable" : "disable");
	}
	pb->enabled = enable;
}

/*
 * Sends the current backlight level over the aux channel, checking if its using
 * 8-bit or 16 bit value (MSB and LSB)
 */
static void
gb_dp_aux_set_backlight(struct GB02STR154 *pb, u32 level)
{
	u8 vals[2] = { 0x0 };
	struct dptx *dptx = pb->dptx;

	vals[0] = level;

	/* Write the MSB and/or LSB */
	if (pb->edp_dpcd[2] & DP_EDP_BACKLIGHT_BRIGHTNESS_BYTE_COUNT) {
		vals[0] = (level & 0xFF00) >> 8;
		vals[1] = (level & 0xFF);
	}

	if (GB02FUNC1156(dptx, DP_EDP_BACKLIGHT_BRIGHTNESS_MSB,
			      vals, sizeof(vals)) < 0) {
		pr_err("Failed to write aux backlight level\n");
		return;
	}

	pb->level = level;
}

static u32 GB02FUNC1196(struct GB02STR154 *pb, int brightness)
{
	return scale(brightness, 0, 100, pb->min, pb->max);
}

/*
 * Set PWM Frequency divider to match desired frequency in vbt.
 * The PWM Frequency is calculated as 27Mhz / (F x P).
 * - Where F = PWM Frequency Pre-Divider value programmed by field 7:0 of the
 *             EDP_BACKLIGHT_FREQ_SET register (DPCD Address 00728h)
 * - Where P = 2^Pn, where Pn is the value programmed by field 4:0 of the
 *             EDP_PWMGEN_BIT_COUNT register (DPCD Address 00724h)
 */
static bool GB02FUNC1197(struct GB02STR154 *pb)
{
	struct dptx *dptx = pb->dptx;
	int freq, fxp, fxp_min, fxp_max, fxp_actual, f = 1;
	u8 pn, pn_min, pn_max;

	/* Find desired value of (F x P)
	 * Note that, if F x P is out of supported range, the maximum value or
	 * minimum value will applied automatically. So no need to check that.
	 */
	freq = pb->pwm_freq_hz;
	if (!freq) {
		pr_err("Use panel default backlight frequency\n");
		return false;
	}

	fxp = DIV_ROUND_CLOSEST((GB02MAC1778 * 1000), freq);

	/* Use highest possible value of Pn for more granularity of brightness
	 * adjustment while satifying the conditions below.
	 * - Pn is in the range of Pn_min and Pn_max
	 * - F is in the range of 1 and 255
	 * - FxP is within 25% of desired value.
	 *   Note: 25% is arbitrary value and may need some tweak.
	 */
	if (GB02FUNC1151(dptx,
			       DP_EDP_PWMGEN_BIT_COUNT_CAP_MIN, &pn_min) != 1) {
		pr_err("Failed to read pwmgen bit count cap min\n");
		return false;
	}
	if (GB02FUNC1151(dptx,
			       DP_EDP_PWMGEN_BIT_COUNT_CAP_MAX, &pn_max) != 1) {
		pr_err("Failed to read pwmgen bit count cap max\n");
		return false;
	}
	pn_min &= GB02MAC1781;
	pn_max &= GB02MAC1781;

	fxp_min = DIV_ROUND_CLOSEST(fxp * 3, 4);
	fxp_max = DIV_ROUND_CLOSEST(fxp * 5, 4);
	if (fxp_min < (1 << pn_min) || (255 << pn_max) < fxp_max) {
		pr_err("defined backlight frequency out of range\n");
		return false;
	}

	for (pn = pn_max; pn >= pn_min; pn--) {
		f = clamp(DIV_ROUND_CLOSEST(fxp, 1 << pn), 1, 255);
		fxp_actual = f << pn;
		if (fxp_min <= fxp_actual && fxp_actual <= fxp_max)
			break;
	}

	if (GB02FUNC1153(dptx,
			       DP_EDP_PWMGEN_BIT_COUNT, pn) < 0) {
		pr_err("Failed to write aux pwmgen bit count\n");
		return false;
	}
	if (GB02FUNC1153(dptx,
			       DP_EDP_BACKLIGHT_FREQ_SET, (u8) f) < 0) {
		pr_err("Failed to write aux backlight freq\n");
		return false;
	}
	return true;
}

static void GB02FUNC1203(struct GB02STR154 *pb,
					  int brightness)
{
	u8 dpcd_buf, new_dpcd_buf, edp_backlight_mode;
	struct dptx *dptx = pb->dptx;
	u32 level;

	if (GB02FUNC1151(dptx,
			DP_EDP_BACKLIGHT_MODE_SET_REGISTER, &dpcd_buf) != 1) {
		pr_err("Failed to read DPCD register 0x%x\n",
			      DP_EDP_BACKLIGHT_MODE_SET_REGISTER);
		return;
	}

	new_dpcd_buf = dpcd_buf;
	edp_backlight_mode = dpcd_buf & DP_EDP_BACKLIGHT_CONTROL_MODE_MASK;

	switch (edp_backlight_mode) {
	case DP_EDP_BACKLIGHT_CONTROL_MODE_PWM:
	case DP_EDP_BACKLIGHT_CONTROL_MODE_PRESET:
	case DP_EDP_BACKLIGHT_CONTROL_MODE_PRODUCT:
		new_dpcd_buf &= ~DP_EDP_BACKLIGHT_CONTROL_MODE_MASK;
		new_dpcd_buf |= DP_EDP_BACKLIGHT_CONTROL_MODE_DPCD;
		break;

	/* Do nothing when it is already DPCD mode */
	case DP_EDP_BACKLIGHT_CONTROL_MODE_DPCD:
	default:
		break;
	}

	if (pb->edp_dpcd[2] & DP_EDP_BACKLIGHT_FREQ_AUX_SET_CAP)
		if (GB02FUNC1197(pb))
			new_dpcd_buf |= DP_EDP_BACKLIGHT_FREQ_AUX_SET_ENABLE;

	if (new_dpcd_buf != dpcd_buf) {
		if (GB02FUNC1153(dptx,
			DP_EDP_BACKLIGHT_MODE_SET_REGISTER, new_dpcd_buf) < 0) {
			pr_err("Failed to write aux backlight mode\n");
		}
	}

	level = GB02FUNC1196(pb, brightness);
	GB02FUNC1193(pb, true);
	gb_dp_aux_set_backlight(pb, level);
}

int GB02FUNC8(struct GB02STR154 *pb, int brightness)
{
	int duty_cycle = 0;

	if (pb->is_edp) {
		GB02FUNC1203(pb, brightness);
	} else {
		if (brightness > 0) {
			duty_cycle = GB02FUNC1189(pb, brightness);
			GB02FUNC1379(pb->pwm, duty_cycle, pb->period);
			pb->enabled = true;
		}
	}

	pb->cur_brightness = brightness;
	GB02MAC1759("update brightness %d\n", duty_cycle);
	return 0;
}

int GB02FUNC1208(struct GB02STR154 *pb)
{
	return pb->cur_brightness;
}

static void GB02FUNC1209(struct GB02STR154 *pb)
{
	if (pb->is_edp)
		GB02FUNC1193(pb, false);
}

static struct GB02STR154 *GB02FUNC1210(struct GB02STR39 *gbdev)
{
	struct GB02STR154 *pb;

	pb = devm_kzalloc(gbdev->dev, sizeof(*pb), GFP_KERNEL);
	if (!pb) {
		return NULL;
	}
	pb->period = GB02MAC1761;
	pb->enabled = false;
	pb->scale = 100;
	pb->lth_brightness = GB02MAC1764;

	return pb;
}

static bool GB02FUNC1211(struct dptx *dptx, struct GB02STR154 *pb)
{
	GB02FUNC1154(dptx, DP_EDP_DPCD_REV, pb->edp_dpcd, sizeof(pb->edp_dpcd));
	/* Check the eDP Display control capabilities registers to determine if
	 * the panel can support backlight control over the aux channel
	 */
	pr_err("edp_dpcd[0] = 0x%x edp_dpcd[1] = 0x%x edp_dpcd[2] = 0x%x\n", pb->edp_dpcd[0], pb->edp_dpcd[1], pb->edp_dpcd[2]);
	if (pb->edp_dpcd[1] & DP_EDP_TCON_BACKLIGHT_ADJUSTMENT_CAP &&
	    (pb->edp_dpcd[2] & DP_EDP_BACKLIGHT_BRIGHTNESS_AUX_SET_CAP) &&
	    !(pb->edp_dpcd[2] & DP_EDP_BACKLIGHT_BRIGHTNESS_PWM_PIN_CAP)) {
		pr_err("AUX Backlight Control Supported!\n");
		return true;
	}
	return false;
}

/*
 * Read the current backlight value from DPCD register(s) based
 * on if 8-bit(MSB) or 16-bit(MSB and LSB) values are supported
 */
static u32 GB02FUNC1212(struct dptx *dptx,
					struct GB02STR154 *pb)
{
	u8 read_val[2] = { 0x0 };
	u16 level = 0;

	if (GB02FUNC1154(dptx, DP_EDP_BACKLIGHT_BRIGHTNESS_MSB, read_val, sizeof(read_val))) {
		pr_err("%s read failed\n", __func__);
		return 0;
	}

	level = read_val[0];
	if (pb->edp_dpcd[2] & DP_EDP_BACKLIGHT_BRIGHTNESS_BYTE_COUNT)
		level = (read_val[0] << 8 | read_val[1]);

	return level;
}

static int GB02FUNC1213(struct dptx *dptx,
					struct GB02STR154 *pb)
{

	if (pb->edp_dpcd[2] & DP_EDP_BACKLIGHT_BRIGHTNESS_BYTE_COUNT)
		pb->max = 0xFFFF;
	else
		pb->max = 0xFF;

	pb->min = 0;
	pb->level = GB02FUNC1212(dptx, pb);

	pb->enabled = pb->level != 0;

	return 0;
}

struct GB02STR154 *GB02FUNC2(int index)
{
	return g_pb[index];
}

int GB02FUNC1215(struct GB02STR39 *gbdev,
			struct GB02STR72 *peri_info, struct GB02STR74 *pwm_dev)
{
	struct GB02STR154 *pb;
	struct GB02STR172 *gb02_bl_pwm;
	int num_connector;
	bool is_edp = false;
	enum gb_board_type board_type;
	int ret = 0;

	gb02_bl_pwm = (struct GB02STR172 *)peri_info->priv;
	num_connector = pwm_dev->num_connector;

	pb = GB02FUNC1210(gbdev);
	if (!pb)
		return -ENOMEM;

	board_type = GB02FUNC503(gbdev->gb_pcie);
	if (gbdev->gb_pcie->dptx[num_connector])
		is_edp = gbdev->gb_pcie->dptx[num_connector]->is_edp;

	if (is_edp && board_type == PCIE_FULL_FUNC_DDR4) {
		pb->dptx = gbdev->gb_pcie->dptx[num_connector];
		if (!GB02FUNC1211(pb->dptx, pb))
			return ret;
		GB02FUNC1213(pb->dptx, pb);
		pb->is_edp = true;
	} else {
		pb->is_edp = false;
		/* use mcu power ctrl*/
		pb->pwm = (struct GB02STR169 *)&gb02_bl_pwm->pwms[GB02MAC2033];
		if (IS_ERR(pb->pwm)) {
			ret = PTR_ERR(pb->pwm);
			if (ret != -EPROBE_DEFER)
				dev_err(gbdev->dev, "unable to request PWM\n");
			return ret;
		}
	}
	g_pb[num_connector] = pb;
	GB02FUNC8(pb, 50);
	return ret;
}

void GB02FUNC1219(struct GB02STR72 *peri_info)
{
	int i;
	struct GB02STR154 *pb;
	for (i = 0; i < GB02MAC658; i++) {
		pb = g_pb[i];
		if (pb)
			GB02FUNC1209(pb);
	}
}

