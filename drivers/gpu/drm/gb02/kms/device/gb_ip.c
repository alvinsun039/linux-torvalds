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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif

#include "gb_ip.h"
#include "reg_ops.h"
#include "gbdc_regs.h"
#include "gbdc_edid.h"
#include "gbdc_ops.h"
#include "common/gb_common.h"

/* #define PLL_TEST_CONF */

/*
*		PCIE init
*
*/

void GB02FUNC607(void __iomem *cfgbar_mmio,
			struct GB02STR117 *gb_version)
{
	gb_version->mcu_vdata = GB02FUNC730(cfgbar_mmio, GB02MAC781);
	gb_version->mcu_vid = GB02FUNC730(cfgbar_mmio, GB02MAC784);
	gb_printf(KERN_INFO, "mcu version:%x%x", gb_version->mcu_vdata,
					gb_version->mcu_vid);
	gb_version->bios_vdata = GB02FUNC730(cfgbar_mmio, GB02MAC787);
	gb_version->bios_vid = GB02FUNC730(cfgbar_mmio, GB02MAC790);
	gb_printf(KERN_INFO, "bios version:%x%x", gb_version->bios_vdata,
					gb_version->bios_vid);
}

void GB02FUNC608(void __iomem *cfgbar_mmio,
            void __iomem *sysctl_cfg_base, struct GB02STR117 *gb_version)
{
    u32 sysctl_cfg;
    sysctl_cfg = GB02FUNC730(sysctl_cfg_base, GB02MAC734);
    if (sysctl_cfg != GB02MAC740)
        GB02FUNC733(sysctl_cfg_base, GB02MAC734, GB02MAC740);

    GB02FUNC733(cfgbar_mmio, GB02MAC787, gb_version->bios_vdata);
    GB02FUNC733(cfgbar_mmio, GB02MAC790, gb_version->bios_vid);
    GB02FUNC733(sysctl_cfg_base, GB02MAC734, sysctl_cfg);
}

/*
*		HDMI init
*
*/
static bool GB02FUNC610(void __iomem  *hdmi_base, int msec)
{
	u32 val;

	while ((val = GB02FUNC730(hdmi_base, GB02MAC1089(0x0108)) & 0x3) == 0) {
		if (msec-- == 0)
			return false;
		udelay(1000);
	}
	GB02FUNC733(hdmi_base, GB02MAC1089(0x0108), val);

	return true;
}

static void GB02FUNC611(void __iomem  *hdmi_base, u32 addr, u32 data)
{
	GB02FUNC733(hdmi_base, GB02MAC1089(0x0108), 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3021), addr);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3022), (data & 0xFF00) >> 8);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3023), (data & 0xFF));
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3026), 0x10);
	GB02FUNC610(hdmi_base, 10);
}
static void GB02FUNC613(void __iomem  *hdmi_base, u32 addr, u32 *data)
{
	if (NULL == data)
		return ;
	else
		*data = 0;
	GB02FUNC733(hdmi_base, GB02MAC1089(0x0108), 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3021), addr);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3026), 0x01);
	GB02FUNC610(hdmi_base, 10);
	*data = GB02FUNC730(hdmi_base, GB02MAC1089(0x3024)) << 8;
	*data |= GB02FUNC730(hdmi_base, GB02MAC1089(0x3025));
}


__attribute__((unused)) static uint8_t  hdmi_ddc_i2c_read(
				void __iomem *hdmi_base, u32 addr)
{
	u8 data = 0;
	GB02FUNC733(hdmi_base, GB02MAC1089(0x0185), 0x00);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x7e00), 0x50);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x7e01), addr);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x7e04), 0x01);

	msleep(1);
	data = GB02FUNC730(hdmi_base, GB02MAC1089(0x7e03));
	GB02FUNC733(hdmi_base, GB02MAC1089(0x0185), 0x03);
	return data;
}

__attribute__((unused)) static void hdmi_get_edid(void __iomem  *hdmi_base,
							uint8_t *edid)
{
	int i = 0;
	for (i = 0; i < 128; i++)
		edid[i] = hdmi_ddc_i2c_read(hdmi_base, i);
}
/* -----------------------------------------------------------------------------
 * Register access
 */
static void GB02FUNC618(void __iomem  *hdmi_base,
		const struct drm_display_mode *ptr)
{
	char sync_flag = 0x0;
	gb_printf(KERN_INFO, "hdmi config:%s\n", ptr->name);

	/* AV_CONFIG */
	if (ptr->flags & DRM_MODE_FLAG_PHSYNC)
		sync_flag |=
			HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_HIGH;

	if (ptr->flags & DRM_MODE_FLAG_PVSYNC)
		sync_flag |=
			HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_HIGH;

	if (GB02FUNC759())
		sync_flag |= HDMI_FC_INVIDCONF_DVI_MODEZ_HDMI_MODE;

	sync_flag |=
		HDMI_FC_INVIDCONF_DE_IN_POLARITY_ACTIVE_HIGH;

	gb_printf(KERN_INFO, "---gb----sync flag = %x---\n", sync_flag);

	GB02FUNC733(hdmi_base, GB02MAC1089(0x1000),
		sync_flag);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1001),
		(ptr->hdisplay) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1002),
		(ptr->hdisplay >> 8) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1003),
		((ptr->htotal-ptr->hdisplay)) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1004),
		((ptr->htotal-ptr->hdisplay) >> 8) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1005),
		(ptr->vdisplay) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1006),
		(ptr->vdisplay >> 8) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1007),
		((ptr->vtotal-ptr->vdisplay)) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1008),
		((ptr->hsync_start-ptr->hdisplay)) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1009),
		((ptr->hsync_start-ptr->hdisplay) >> 8) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x100A),
		((ptr->hsync_end-ptr->hsync_start)) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x100B),
		((ptr->hsync_end-ptr->hsync_start) >> 8) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x100C),
		((ptr->vsync_start-ptr->vdisplay)) & 0xFF);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x100D),
		((ptr->vsync_end-ptr->vsync_start) >> 8) & 0xFF);
}
static int GB02FUNC624(void __iomem  *hdmi_base,
			const struct drm_display_mode *ptr)
{

	u32 divset = 0;
	u32 curset = 0;
	u32 gmpset = 0;

	/* PHY_CONFIG */
	if (ptr->clock <= 27000) {
		divset = 0x5E0;
		curset = 0x91C;
		gmpset = 0x00;
	} else if (ptr->clock <= 59400) {
		divset = 0x540;
		curset = 0x91C;
		gmpset = 0x05;
	} else if (ptr->clock <= 74250) {
		divset = 0x540;
		curset = 0x6DC;
		gmpset = 0x05;
	} else if (ptr->clock <= 144000) {
		divset = 0x4A0;
		curset = 0x91C;
		gmpset = 0x0A;
	} else if (ptr->clock <= 148500) {
		divset = 0x4A0;
		curset = 0x6DC;
		gmpset = 0x0A;
	} else if (ptr->clock <= 216000) {
		divset = 0x000;
		curset = 0x6DC;
		gmpset = 0x0F;
	}

	GB02FUNC611(hdmi_base, 0x06, divset);
	GB02FUNC611(hdmi_base, 0x10, curset);
	GB02FUNC611(hdmi_base, 0x15, gmpset);

	GB02FUNC611(hdmi_base, 0x17, 0x06);
	GB02FUNC611(hdmi_base, 0x19, 0x04);
	GB02FUNC611(hdmi_base, 0x09, 0x0d);
	GB02FUNC611(hdmi_base, 0x0E, 0x318);
	GB02FUNC611(hdmi_base, 0x05, 0x8000);


	GB02FUNC613(hdmi_base, 0x10, &gmpset);
	if (gmpset != curset) {
		gb_printf(KERN_ERR, "%s value set failed!\r\n", __func__);
		return -1;
	}
	return 0;
}

/* -----------------------------------------------------------------------------
 * Register function
 */
int GB02FUNC631(void __iomem *hdmi_base)
{
	u32 val;
	int i = 0;

	/* 0x3000 bit3Åä0£¬poweroff */
	val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3000));
	val &= (~(1 << 3));
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3000), val);

	/*
	 * Wait for TX_PHY_LOCK to be deasserted to indicate that the PHY went
	 * to low power mode.
	 */
	for (i = 0; i < 5; ++i) {
		val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3004));
		if (!(val & 0x1))
			break;

		usleep_range(1000, 2000);
	}

	if (val & 0x1) {
		gb_printf(KERN_ERR, "%s: gb01 hdmi phy power off failed!\n", __func__);
		return -1;
	}

	/* 0x3000 bit4Åä1£¬PDDQ */
	val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3000));
	val |= (1 << 4);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3000), val);

	return 0;
}
int GB02FUNC635(void __iomem  *hdmi_base)
{
	u32 val;
	int i = 0;

	val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3000));
	val |= (1 << 3);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3000), val);

	val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3000));
	val &= (~(1 << 4));
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3000), val);

	/* Wait for PHY PLL lock */
	for (i = 0; i < 5; ++i) {
		val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3004));
		if (val & 0x1)
			break;

		usleep_range(1000, 2000);
	}

	if (!(val & 0x1)) {
		gb_printf(KERN_ERR, "PHY PLL failed to lock\n");
		return -ETIMEDOUT;
	}

	return 0;
}

#ifdef HDMI_OUT_DEBUG
static void GB02FUNC641(void __iomem  *hdmi_dgb_base)
{
	//debug output 1920x1080
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7012), 0x0A);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7001), 0x07);
	/* bit[5:4]00:8bit colordepth 01:10bit colordepth
	*  10:12bit colordepth 11:16bit colordepth */
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7002), 0x00);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7003), 0x80);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7004), 0x07);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7005), 0x18);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7006), 0x01);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7007), 0x58);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7008), 0x00);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7009), 0x2C);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x700A), 0x00);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x700B), 0x38);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x700C), 0x04);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x700D), 0x2D);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x700E), 0x04);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x700F), 0x05);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7011), 0x04);

	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7038), 0x01);
	GB02FUNC733(hdmi_dgb_base, GB02MAC1089(0x7039), 0x01);
}
#endif

static int GB02FUNC643(void __iomem  *hdmi_base)
{
	u32 type = 0;
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3000), 0x04);

	type = GB02FUNC730(hdmi_base, GB02MAC1089(0x4));
	if (type & (1 << 3))
		gb_printf(KERN_INFO, "GB1 support HDMI v1.4\r\n");

	type = GB02FUNC730(hdmi_base, GB02MAC1089(0x5));
	if (type & (1 << 5))
		gb_printf(KERN_INFO, "GB1 support HDMI v2.0\r\n");

	type = GB02FUNC730(hdmi_base, GB02MAC1089(0x6));
	if (type == 0xF2)
		gb_printf(KERN_INFO, "PHY type = 0x%x\n", type);
	else {
		gb_printf(KERN_ERR, "%s TYPE check failed!\r\n", __func__);
		return -1;
	}

	return 0;
}

static void GB02FUNC648(void __iomem  *hdmi_base, u32 addr)
{
	u32 val;

	val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3001));
	val |= (1 << 5);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3001), val);

	GB02FUNC733(hdmi_base, GB02MAC1089(0x3020), addr);

	val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3001));
	val &= (~(1 << 5));
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3001), val);
}

static void GB02FUNC650(void __iomem *hdmi_base)
{
	/* ENV_CONFIG */
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1011), 0x0C);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1012), 0x20);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1013), 0x01);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1014), 0x0B);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1015), 0x16);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x1016), 0x21);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x4001), 0x7C);
}

static int GB02FUNC652(void __iomem  *hdmi_base,
			const struct drm_display_mode *ptr)
{
	u32 val;

	val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3000));
	val |= (1 << 1);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3000), val);

	val = GB02FUNC730(hdmi_base, GB02MAC1089(0x3000));
	val &= (~(1 << 0));
	GB02FUNC733(hdmi_base, GB02MAC1089(0x3000), val);
	/* power off */
	if (GB02FUNC631(hdmi_base))
		return -1;

	/* reset phy */
	GB02FUNC733(hdmi_base, GB02MAC1089(0x4005), 0x01);
	GB02FUNC733(hdmi_base, GB02MAC1089(0x4005), 0x00);

	/* I2C PHY addr 0x69 */
	GB02FUNC648(hdmi_base, 0x69);

	GB02FUNC624(hdmi_base, ptr);

	/* power on */
	return GB02FUNC635(hdmi_base);
}

static void GB02FUNC655(void __iomem  *hdmi_base)
{
	GB02FUNC733(hdmi_base, GB02MAC1089(0x182), 0x3);
}

int GB02FUNC658(void __iomem *hdmi_base, const struct drm_display_mode *ptr)
{
	int i = 0;

	gb_printf(KERN_INFO, "%s hdmi status:0x%x!\r\n", __func__,
		GB02FUNC730(hdmi_base, GB02MAC1089(0x3007)));

#ifdef HDMI_OUT_DEBUG
	GB02FUNC641(hdmi_base + GB02MAC690);
#endif
	if (GB02FUNC643(hdmi_base)) {
		gb_printf(KERN_ERR, "%s:gb01 hdmi sink check failed!", __func__);
		return -1;
	}
	GB02FUNC655(hdmi_base);
	GB02FUNC618(hdmi_base, ptr);

	/* HDMI Phy spec says to do the phy initialization sequence twice */
	for (i = 0; i < 2; i++) {
		if (GB02FUNC652(hdmi_base, ptr)) {
			gb_printf(KERN_ERR, "%s: gb01 hdmi phy cofigure failed!\n", __func__);
			return -1;
		}
	}

	GB02FUNC650(hdmi_base);

	return 0;
}

void GB02FUNC661(void __iomem *ip_base)
{
	GB02FUNC733(ip_base + GB02MAC690,
			GB02MAC1089(GB02MAC771), GB02MAC769);
	gb_printf(KERN_INFO, "%s", __func__);
}
static u8 GB02FUNC664(void __iomem *_base)
{
	u8 clock = 0;
	clock = GB02FUNC730(_base + GB02MAC690,
			GB02MAC1089(GB02MAC773));
	gb_printf(KERN_INFO, "%s = %d\n", __func__, clock);
	return clock;
}

static u8 GB02FUNC665(void __iomem *_base)
{
	u8 dat = 0, ret = 0;

	dat = GB02FUNC730(_base + GB02MAC690,
			GB02MAC1089(GB02MAC816));
	if (dat & (1 << GB02MAC820))
		ret = 1;

	return ret;
}

#define		GB02MAC1065	0x03
int GB02FUNC669(void __iomem *hdmi_base, int vga_cmd)
{
	unsigned int vga_status = 0;

	GB02FUNC733(hdmi_base, GB02MAC1089(GB02MAC775), vga_cmd);
#if 0
	vga_status = GB02FUNC730(hdmi_base, GB02MAC1089(GB02MAC778));
	if (vga_status)
		usleep_range(100, 1000);
#endif
	gb_printf(KERN_INFO, "--gb---vga--cmd=%d--\n", vga_cmd);
	return vga_status;
}

void GB02FUNC672(void __iomem *ip_base)
{
	GB02FUNC631(ip_base + GB02MAC690);
	GB02FUNC669(ip_base + GB02MAC690, GB02MAC1065);

}
/*
*		PLL init
*
*/

typedef enum {
	CLK_TYPE_FOR_252 = 0, /* 25.2MHz */
	CLK_TYPE_for_200, /* 20MHz */
	CLK_TYPE_for_200_VGA, /* 20MHz, for VGA */
	CLK_TYPE_MAX
} clk_type_e;
#ifdef PLL_TEST_CONF
static int GB02FUNC674(char *cmd)
{
	char val[64] = {0};
	int v = 0;

	if (!cmd)
		return -1;
	GB02FUNC394(cmd, val, sizeof(val));
	/* fixme: WARNING: simple_strtol is obsolete, use kstrtol instead */
	v = simple_strtol(val, NULL, 10);
	gb_printf(KERN_INFO, "%s = %d\n", cmd, v);

	return v;
}
static int GB02FUNC676(void __iomem  *ppll_base,
	pll_module_enum xpll, u32 clk, u32 fin, u32 p)
{
	u32 fvco = 0;
	u32 fclk = 0;
	u32 pll_mi = 0;
	u32 pll_m = 0;
	u32 pll_n = 0x13;
	u32 pll_p = 0;
	u32 pll_vdiv = 0;
	u32 pll_pdiv = 0;
	int delta_clk = 0;
	u32 mfrac = 0;
	u32 pll_mfrac = 0;

#if 0
	if (pll_m * pll_n * pll_p < 1) {
		gb_printf(KERN_ERR, "input Invalid argument");
		return -EINVAL;
	}
#endif
	gb_printf(KERN_INFO, "---------------/etc/sietium.conf------------------");
	pll_mi = GB02FUNC674("pll_mi");
	pll_p = GB02FUNC674("pll_p");
	if (pll_p < 3)
		pll_p = p;
	gb_printf(KERN_INFO, "fin = %u\n", fin);
	pll_m = 100 * (clk * (pll_p + 1) * (pll_vdiv + 1) * (pll_pdiv + 1) *
			      (pll_n + 1)) / fin;
	gb_printf(KERN_INFO, "M = %u\n", pll_m);
	pll_m += pll_mi;
	fvco = (((pll_m / 100) * fin) / (pll_n + 1));
	fclk = fvco / (p + 1);
	gb_printf(KERN_INFO, "fvco = %u\n", fvco);
	gb_printf(KERN_INFO, "fclk = %u\n", fclk);
	delta_clk = fclk - clk;
	if ((delta_clk > 1000) || (delta_clk < -1000)) {
		gb_printf(KERN_ERR, "clk exceeds the threshold\n");
		/*
		 * return -EINVAL;
		 */
	}

	/*
	 * The mfrac coefficient value corresponding to each 0.01 is 10485.
	 *
	 * 2^-20 : 0x1
	 * 2^-1  : 0x80000 524288
	 * 0.5   : 0x80000 524288
	 * 0.01  : 0x28f5 10485
	 *
	 * Reference doc: 19602tqlp_A753-0_1G5-DPLL_databook_0v7.pdf
	 */
	mfrac = pll_m % 100;
	gb_printf(KERN_INFO, "mfrac = %u\n", mfrac);
	pll_mfrac = (mfrac * 10485);
	gb_printf(KERN_INFO, "pll_mfrac = %u\n", pll_mfrac);

	gb_printf(KERN_INFO, "pll_m = %u, pll_n = %u, pll_p = %u\n",
	       pll_m / 100, pll_n, pll_p);

	GB02FUNC733(ppll_base, GB02MAC1108(xpll), 0x00001000);
	GB02FUNC733(ppll_base, GB02MAC1104(xpll), 0x04000000 | (pll_n & 0xFF));
	GB02FUNC733(ppll_base, GB02MAC1105(xpll),
		      (((pll_m / 100) & 0xFFF) << 20) | (pll_mfrac & 0xFFFFF));
	GB02FUNC733(ppll_base, GB02MAC1106(xpll),
		      0x02000000 | (pll_p & 0x3F) << 16);
	GB02FUNC733(ppll_base, GB02MAC1107(xpll), 0x00000000);
	GB02FUNC733(ppll_base, GB02MAC1108(xpll), 0x00011000);
	gb_printf(KERN_INFO, "------------/etc/sietium.conf---end---------------");

	if (!(GB02FUNC730(ppll_base, GB02MAC1108(xpll)) & 0x01))
		return -1;
	return 0;
}
#endif

static int GB02FUNC681(void __iomem  *ppll_base,
	pll_module_enum xpll, u32 clk, u32 fin, u32 p, int high_speed)
{
	u32 M = 0;
	u32 N = 0x0;
	u32 vdiv = 0;
	u32 pdiv = 0;
	u32 mfrac = 0;

	if (high_speed)
		vdiv = 1;
	/*
	 * fvco = (M / N) * fin;
	 * clk = (1 / (p * enp * prop)) * ((M / N) * fin);
	 * M  = (clk * p * vdivp * pdivp * N) / fin;
	 */
	M  = 100 * (clk * (p + 1) * (vdiv + 1) * (pdiv + 1) * (N + 1)) / fin;
	gb_printf(KERN_INFO, "M = %u, N = %u, p = %u, vdiv = %u, pdiv = %u, high_speed = %d\n",
	       M, N, p, vdiv, pdiv, high_speed);
	/*
	 * The mfrac coefficient value corresponding to each 0.01 is 10485.
	 *
	 * 2^-20 : 0x1
	 * 2^-1  : 0x80000 524288
	 * 0.5   : 0x80000 524288
	 * 0.01  : 0x28f5 10485
	 *
	 * Reference doc: 19602tqlp_A753-0_1G5-DPLL_databook_0v7.pdf
	 */
	mfrac = (M % 100) * 10485;
	gb_printf(KERN_INFO, "mfrac = %u\n", mfrac);

	GB02FUNC733(ppll_base, GB02MAC1108(xpll), 0x00001000);
	GB02FUNC733(ppll_base, GB02MAC1104(xpll),
		      0x04000000 | (N & 0xFF) | (high_speed << 9));
	GB02FUNC733(ppll_base, GB02MAC1105(xpll),
		      (((M / 100) & 0xFFF) << 20) | (mfrac & 0xFFFFF));
	GB02FUNC733(ppll_base, GB02MAC1106(xpll), 0x02000000
			| (p & 0x3F) << 16 | ((vdiv) << 24) | ((pdiv) << 22));
	GB02FUNC733(ppll_base, GB02MAC1107(xpll), 0x00000000);
	GB02FUNC733(ppll_base, GB02MAC1108(xpll), 0x00011000);

	if (!(GB02FUNC730(ppll_base, GB02MAC1108(xpll)) & 0x01))
		return -1;

	return 0;
}

#define GB02MAC1072 1500000
#define GB02MAC1073 1000000
#define GB02MAC1074 500000
#define GB02MAC1075 123500

static int GB02FUNC686(void __iomem  *ppll_base,
	const struct drm_display_mode *ptr, pll_module_enum xpll)
{
	int clock_temp = 0;
	int clkp_temp = 0;
	int clkp = 0;
	int clkin = 0;
	int high_speed = 0;
	u32 max_fvco = GB02MAC1072;

	clk_type_e clk_type = CLK_TYPE_for_200;
	clock_temp = ptr->clock;
	if (xpll == PIX1_PLL && ptr->clock >= GB02MAC1075)
		clock_temp = GB02MAC1075;

	clkin = GB02FUNC664(ppll_base - GB02MAC689);
	if (!clkin) {
		clkin = 252;
		clk_type = CLK_TYPE_FOR_252;
	}

	if ((CLK_TYPE_for_200 == clk_type) &&
		(GB02FUNC665(ppll_base - GB02MAC689)))
		clk_type = CLK_TYPE_for_200_VGA;

find_clkp:
	if (clock_temp != 0)
		clkp = max_fvco / clock_temp;
	else {
		gb_printf(KERN_ERR, "%s %d clock freq is 0!\n",
				__func__, __LINE__);
		return -1;
	}

	clkp_temp = clkp;
	while ((clkp_temp * clock_temp) % 1000) {
		clkp_temp--;
		if (clkp_temp * clock_temp < GB02MAC1074) {
			clkp_temp = clkp;
			break;
		}
	}

	if (max_fvco == GB02MAC1072) {
		if (clkp_temp % 2) {
			if ((clkp_temp - 1) * clock_temp > GB02MAC1073)
				high_speed = 1;
		} else {
			if (clkp_temp * clock_temp > GB02MAC1073)
				high_speed = 1;
		}
	}

	if (high_speed)
		clkp = (clkp_temp / 2) - 1;
	else {
		if (max_fvco == GB02MAC1072) {
			max_fvco = GB02MAC1073;
			high_speed = 0;
			goto find_clkp;
		}
		clkp = clkp_temp - 1;
	}

	GB02FUNC681(ppll_base, xpll, clock_temp,
		clkin * 100, clkp, high_speed);
#ifdef PLL_TEST_CONF
	GB02FUNC676(ppll_base, xpll, clock_temp,
		clkin * 100, clkp);
#endif
	mdelay(50);
	if (!(GB02FUNC730(ppll_base, GB02MAC1108(xpll)) & 0x01)) {
		gb_printf(KERN_ERR, "PLL%d set pll %dx%d %dkHz failed!\r\n",
				xpll, ptr->hdisplay, ptr->vdisplay, ptr->clock);
		return -1;
	}

	mdelay(50);
	gb_printf(KERN_INFO, "PLL%d set pll %dx%d %dkHz succeed!\r\n",
				xpll, ptr->hdisplay, ptr->vdisplay, ptr->clock);
	return 0;
}

int GB02FUNC689(void __iomem  *ppll_base, const struct drm_display_mode *ptr,
			int crtc_id)
{
	if (crtc_id == GB02MAC1100)
		return GB02FUNC686(ppll_base, ptr, PIX_PLL);
	else
		return GB02FUNC686(ppll_base, ptr, PIX1_PLL);
}

void GB02FUNC690(void __iomem  *base, u32 addr, u32 data)
{
	u32 val = data;

	GB02FUNC733(base, GB02MAC1089(addr), val);
}

u32 GB02FUNC691(void __iomem  *base, u32 addr)
{
	u32 val = 0;

	val = GB02FUNC730(base, GB02MAC1089(addr));
	return val;
}

#define GB02MAC1086    5
#define GB02MAC1087   0x6066
int GB02FUNC692(void __iomem  *base)
{
	int val = 0, i = 0;

    /* 1st : select DDC channel of LT86102's TX downstream to be accessed */
	for (i = 0; i < GB02MAC1086; i++) {
		val = GB02FUNC691(base, GB02MAC794);
		if ((val & BIT(7)) == 0)
			break;
		mdelay(2);
	}
	if (i >= GB02MAC1086)
		return -1;

	GB02FUNC690(base, GB02MAC803,
			(GB02MAC1087 >> 8) & 0xff);
	GB02FUNC690(base, GB02MAC805,
			GB02MAC1087 & 0xff);
	GB02FUNC690(base, GB02MAC800, 0x4);
	GB02FUNC690(base, GB02MAC794,
			1 << GB02MAC797);

	/* waiting for MCU to configure LT86102 for accessing EDID shadow */
	for (i = 0; i < GB02MAC1086; i++) {
		val = GB02FUNC691(base, GB02MAC794);
		if ((val & BIT(7)) == 0)
			break;
		mdelay(10);
	}
	if (i >= GB02MAC1086)
		return -2;

	return 0;
}

void GB02FUNC696(void __iomem  *base, u32 addr, u32 data)
{
	u32 val = data;

	GB02FUNC733(base, GB02MAC1089(addr), val);
}

u32 GB02FUNC697(void __iomem  *base, u32 addr)
{
	u32 val = 0;

	val = GB02FUNC730(base, GB02MAC1089(addr));
	return val;
}

u32 GB02FUNC701(void __iomem  *base)
{
	u32 val = 0;

	val = GB02FUNC697(base, GB02MAC813);
	val &= 0xff;

	return val;
}

void GB02FUNC704(void __iomem  *base)
{
	GB02FUNC696(base, GB02MAC807,
		(1 << GB02MAC809)
		 | (1 << GB02MAC811));
}

void GB02FUNC707(void __iomem  *base, int brightness)
{
	GB02FUNC696(base, GB02MAC813, brightness);
}

void GB02FUNC709(void __iomem  *base)
{
	GB02FUNC696(base, GB02MAC807,
		(1 << GB02MAC809));
}

#define GB02MAC1088  5
int GB02FUNC711(void __iomem  *base)
{
	int i = 0;
	u32 val = 0;

	for (i = 0; i < GB02MAC1088; i++) {
		val = GB02FUNC697(base, GB02MAC807);
		if ((val & BIT(7)) == 0)
			break;
		mdelay(5);
	}

	if (i >= GB02MAC1088)
		return -1;
	else
		return 0;
}

/*
 * detect the board type according to the value of
 * register GB02MAC816, as follows:
 * val = 1   ===> MXM BOARD
 * otherwise ===> PCIE BOARD
 */
u32 GB02FUNC714(void __iomem  *base)
{
	u32 val = 0;
#if 0
	val = GB02FUNC730(base, GB02MAC1089(GB02MAC816));
	val &= GB02MAC818;
#endif
	return val;
}

/************************ (C) COPYRIGHT SIETIUM *****END OF FILE****/

