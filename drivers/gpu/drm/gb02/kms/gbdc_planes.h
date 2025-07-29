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
#ifndef		__GBDC_PLANE_H__
#define		__GBDC_PLANE_H__
#include "gbdc_mode.h"
#include "gbdc_infinity.h"

#define 	GB02MAC2828		0

struct GB02STR239 {
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
	unsigned int left;
	unsigned int w_total;
	unsigned int h_total;
	unsigned int width;
	unsigned int high;
	int x1;
	int y1;
	int x2;
	int y2;
};

struct gbdc_plane_state {
	struct drm_plane_state base;

	/* size of the required rotation memory if plane is rotated */
	u32 rotmem_size;
	/* internal format ID */
	u8 format;
	u8 n_planes;

	/* infinity */
	struct gbdc_plane *infinity_plane_list;
	struct GB02STR190 order;
    struct GB02STR191 geometry;
    struct GB02STR189 padding;
	struct GB02STR239 gb_border[GB02MAC2558];
};

struct gbdc_plane{
	struct drm_plane 	base;
	uint32_t		crtc_id;
	/* forcc plane num	= 1,,2,3*/
	int			fplane_num;
	uint32_t 		plane_id;
	unsigned long 		cur_fb_offset;
	/* list of supported layers */
	struct GB02STR77  plane_res;

	/* list of supported pixel formats for each layer */
	const struct GB02STR80 *pixel_formats;
	int			num_formats;
	unsigned int zpos;

	/*virt*/
	bool virt;

	struct list_head head;
	unsigned int infinity_row;
	unsigned int infinity_col;
	struct GB02STR190 order;
    struct GB02STR191 geometry;
    struct GB02STR189 padding;
};



struct GB02STR240 {
	unsigned int x;
	unsigned int y;
	bool show;
}; 

enum cusor_pos {
    CURSOE_SHOW_THIS = 0,
    CURSOE_SHOW_RIGHT,
    CURSOE_SHOW_BOTTOM,
    CURSOE_SHOW_BOTTOM_RIGHT,
    CURSOE_SHOW_MAX
};

#define to_gbdc_plane_info(x)   container_of(x, struct gbdc_plane, base)
#define to_gbdc_plane_state(x) container_of(x, struct gbdc_plane_state, base)

int GB02FUNC1860(struct drm_device *dev,
			struct gbdc_plane *gb_plane,
			unsigned long possible_crtcs, u32 type,
			unsigned int zpos);

enum drm_plane_type GB02FUNC1858(unsigned int idx);
void __iomem * get_dc_base(struct GB02STR155 *gb_dev,unsigned int crtc_id);
void __iomem * get_de_base(struct GB02STR155 *gb_dev,unsigned int crtc_id);
void GB02FUNC1844(struct drm_plane *plane);
int GB02FUNC1849(int row, int col, struct drm_plane_state *state);
#endif
