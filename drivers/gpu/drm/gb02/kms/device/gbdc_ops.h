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
#ifndef	__GBDC_OPS_H__
#define	__GBDC_OPS_H__


#define	GB02MAC548 64

enum overlay_attrbute {
	VOERLAY_ADDR,
	OVERLAY_RESOLUTION
};

struct GB02STR65 {
	void __iomem *dc_base;
	void __iomem *de_base;
};

struct GB02STR66 {
	int	w;
	int h;
	int x;
	int y;
	u64 yaddr;
	u64 cbaddr;
	int format;	//nv12,p010
	int ol_id;	//0,1
	int	attr;	//VOERLAY_ADDR
	int layer_id; //0,2
};

int GB02FUNC394(char *key, char *val, size_t len);
int GB02FUNC238(void __iomem *dc_base, void __iomem *de_base,
	struct GB02STR66 *olinfo);
int GB02FUNC278(u16 base, void __iomem *dc_base, void __iomem *de_base);
int GB02FUNC281(void __iomem *dc_base, void __iomem *de_base);
int GB02FUNC284(void __iomem **dc_base, void __iomem **de_base);
#ifdef GB_ALLSCREEN
#define GB02MAC557 6
int GB02FUNC152(void);
int GB02FUNC148(int *crtc_id_list);
int GB02FUNC144(int *crtc_id_list, int crtc_num,
	struct GB02STR246 *crtc_info);

#endif
#endif
