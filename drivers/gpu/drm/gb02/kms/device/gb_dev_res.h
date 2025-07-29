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

#include <linux/module.h>
#ifndef	__GB_DEV_RES_H__
#define	__GB_DEV_RES_H__

enum {
	DE_VIDEO1 = BIT(0),
	DE_GRAPHICS1 = BIT(1),
	DE_GRAPHICS2 = BIT(2),
	DE_VIDEO2 = BIT(3),
	DE_SMART = BIT(4),
};

struct GB02STR75{
	u32		crtc_de_offset;
	u32		crtc_se_offset;
	u32		crtc_dc_offset;
};
struct GB02STR76{
	u32		vram_start_offset;
	u32		crtc_reg_offset;
	u32		crtc_offset_step;
	u32		pcie_conf_offset;
	u32		ddr_conf_offset;
	u32		pll_conf_offset;
	u32		hdmi_reg_offset;
	u32		hdmi_dreg_offset;
};

struct GB02STR77 {
	u16 id;			/* layer ID */
	u16 base;		/* address offset for the register bank */
	u16 ptr;		/* address offset for the pointer register */
	u16 stride_offset;	/* Offset to the first stride register. */
};

struct GB02STR78 {
	u16 id;
	u16	crtc_format;
};

struct GB02STR79 {
	u16 id;
	u16	connect_format;
};

struct GB02STR80{
	u32 format;		/* DRM fourcc */
	u8 layer;		/* bitmask of layers supporting it */
	u8 id;			/* used internally */
};

struct GB02STR81 {
	uint32_t maxX;
	uint32_t maxY;
};

#define GB_MAX_RESOLUTION_DEF(card_type, res_x, res_y) \
static const struct GB02STR81 gb_##card_type##_max_res = { \
	.maxX = res_x, \
	.maxY = res_y, \
}

GB_MAX_RESOLUTION_DEF(FPGA, 4096, 2160);
GB_MAX_RESOLUTION_DEF(GB01, 1920, 1200);
GB_MAX_RESOLUTION_DEF(GB02, 4096, 2160);


struct GB02STR82{
	u32				crtc_num;
	u32				connector_num;
	struct GB02STR79		*connector_res;
	u32				plane_num;
		/* list of supported layers */
	struct GB02STR77 	*plane_res;
	struct GB02STR80 	*plane_cap;
	const struct GB02STR81	*max_res;
	bool	infinity;
	u32	vram_start_offset;

};

#endif
