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
#include <drm/drm_modes.h>
#ifndef __GB_IP_H__
#define __GB_IP_H__

#define	GB02MAC1089(value)	(value << 2)

#define GB02MAC740 0x60000000
#define GB02MAC734 0xd0c

#define		GB02MAC1090	0
#define		GB02MAC1091	3

#define	GB02MAC1092		(20000)
#define	GB02MAC1093		(25200)

#define GB02MAC1094		0x00
#define GB02MAC1095		0x10
#define GB02MAC1096		0x20
#define GB02MAC1097		0x30
#define GB02MAC1098		0x40
#define GB02MAC1099		0x50
#define	GB02MAC1100		0
#define	GB02MAC1101		1
#define GB02MAC1102			0x08
#define GB02MAC1103		0x8bc

#define GB02MAC1104(x)		((x*GB02MAC1099)+GB02MAC1094)
#define GB02MAC1105(x)		((x*GB02MAC1099)+GB02MAC1095)
#define GB02MAC1106(x)		((x*GB02MAC1099)+GB02MAC1096)
#define GB02MAC1107(x)		((x*GB02MAC1099)+GB02MAC1097)
#define GB02MAC1108(x)		((x*GB02MAC1099)+GB02MAC1098)
enum {
	HDMI_FC_INVIDCONF_HDCP_KEEPOUT_MASK = 0x80,
	HDMI_FC_INVIDCONF_HDCP_KEEPOUT_ACTIVE = 0x80,
	HDMI_FC_INVIDCONF_HDCP_KEEPOUT_INACTIVE = 0x00,
	HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_MASK = 0x40,
	HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_HIGH = 0x40,
	HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_LOW = 0x00,
	HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_MASK = 0x20,
	HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_HIGH = 0x20,
	HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_LOW = 0x00,
	HDMI_FC_INVIDCONF_DE_IN_POLARITY_MASK = 0x10,
	HDMI_FC_INVIDCONF_DE_IN_POLARITY_ACTIVE_HIGH = 0x10,
	HDMI_FC_INVIDCONF_DE_IN_POLARITY_ACTIVE_LOW = 0x00,
	HDMI_FC_INVIDCONF_DVI_MODEZ_MASK = 0x8,
	HDMI_FC_INVIDCONF_DVI_MODEZ_HDMI_MODE = 0x8,
	HDMI_FC_INVIDCONF_DVI_MODEZ_DVI_MODE = 0x0,
	HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK = 0x2,
	HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_HIGH = 0x2,
	HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_LOW = 0x0,
	HDMI_FC_INVIDCONF_IN_I_P_MASK = 0x1,
	HDMI_FC_INVIDCONF_IN_I_P_INTERLACED = 0x1,
	HDMI_FC_INVIDCONF_IN_I_P_PROGRESSIVE = 0x0,
};
typedef enum _pll_module {
	BUS_PLL = 0,
	GPU_PLL = 1,
	DDR_PLL = 2,
	PIX_PLL = 3,
	PIX1_PLL = 4
} pll_module_enum;

struct GB02STR117 {
	int mcu_vdata;
	int mcu_vid;
	int drv_vdata;
	int drv_vid;
	int bios_vdata;
	int bios_vid;
};

struct GB02STR118 {
	unsigned int clk_val;
	unsigned int post_div;
};

void GB02FUNC607(void __iomem *cfgbar_mmio,
			struct GB02STR117 *gb_version);
void GB02FUNC608(void __iomem *cfgbar_mmio,
            void __iomem *sysctl_cfg_base, struct GB02STR117 *gb_version);
void config_gb_vga_ep(void __iomem *cfgbar_mmio);

int GB02FUNC689(void __iomem *ip_base,
			const struct drm_display_mode *ptr, int crtc_id);
int GB02FUNC658(void __iomem *hdmi_base,
			const struct drm_display_mode *ptr);
int GB02FUNC635(void __iomem *hdmi_base);
int GB02FUNC631(void __iomem *hdmi_base);
void GB02FUNC661(void __iomem *ip_base);
int GB02FUNC669(void __iomem *hdmi_base, int vga_cmd);
void GB02FUNC672(void __iomem *ip_base);
void GB02FUNC690(void __iomem  *base, u32 addr, u32 data);
u32 GB02FUNC691(void __iomem  *base, u32 addr);
int GB02FUNC692(void __iomem  *base);
void GB02FUNC696(void __iomem  *base, u32 addr, u32 data);
u32 GB02FUNC697(void __iomem  *base, u32 addr);
u32 GB02FUNC701(void __iomem  *base);
void GB02FUNC704(void __iomem  *base);
void GB02FUNC707(void __iomem  *base, int brightness);
void GB02FUNC709(void __iomem  *base);
int GB02FUNC711(void __iomem  *base);
u32 GB02FUNC714(void __iomem  *base);
#endif
