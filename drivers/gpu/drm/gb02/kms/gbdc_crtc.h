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
#ifndef		__GBDC_CRTC_H__
#define		__GBDC_CRTC_H__

#include <drm/drm_flip_work.h>
#include "device/gb_dev_res.h"
#include "gbdc_planes.h"
#include "kms/gb_kms.h"
#include "gbdc_infinity.h"

#define	GB02MAC1419	2
#define GB02MAC1420	12
#define GB02MAC1422	64

#define GB02MAC1424		256
#define GB02MAC1425		0x19

enum {
	DC_PLANE_VIDEO_1 = 0,
	DC_PLANE_GRAPHIC = 1,
	DC_PLANE_VIDEO_2 = 2,
	DC_PLANE_SMART = 3,
	GB_MAX_PLANES,
};

struct GB02STR151 {
	u8 scale_enable:1;
	u8 enhancer_enable:1;
	u8 hcoeff:3;
	u8 vcoeff:3;
	u8 plane_src_id;
	u16 input_w, input_h;
	u16 output_w, output_h;
	u32 h_init_phase, h_delta_phase;
	u32 v_init_phase, v_delta_phase;
};

enum mode_set_flag {
	MODE_SET_NONE,
	MODE_SET_DONE,
};
struct gbdc_crtc_state {
	struct drm_crtc_state base;
	u32 gamma_coeffs[GB02MAC1422];
	u32 coloradj_coeffs[GB02MAC1420];
	struct GB02STR151 scaler_config;
	/* Bitfield of all the planes that have requested a scaled output. */
	u8 scaled_planes_mask;

	/* infinity */
	struct gbdc_crtc *infinity_crtc_list;
	int vblank_id;
	bool need_disable_changed;
	bool need_disable;
};

struct gbdc_crtc {
	struct drm_crtc base;
	struct gbdc_plane cplanes[GB_MAX_PLANES];
	/* list of supported layers */
	struct GB02STR77 plane_res;
	struct tasklet_struct	task_vblank;

	/* list of supported pixel formats for each layer */
	const struct GB02STR80 *pixel_formats;
	int num_formats;
	int crtc_id;
	int cur_x;
	int cur_y;
	int mod_flag;	/**< mode set flag; =0 not set mode; =1*/

	int cursor_width;
	int cursor_height;
	int cursor_hot_x;
	int cursor_hot_y;
	int cursor_x;
	int cursor_y;
	u64 cursor_addr;
	u64 static_cursor_addr;
	int max_cursor_width;
	int max_cursor_height;
	int cursor_resume; //
	struct drm_gem_object *cursor_bo;
	int drm_file_handle;//temp add
	struct drm_file *drmfile;//add
	struct mutex gbdc_crtc_lock;
	struct drm_pending_vblank_event *event;
	bool is_enable;
	bool is_hide;
	struct drm_flip_work gbdc_fb_unref_work;
	unsigned long pending;

	/*virt*/
	int	grp_id;
	bool	virt;
	struct list_head node;//virt list node
	struct list_head head;//link real crtc of this grp
	struct drm_display_mode adjusted_mode;
	struct drm_display_mode old_adjusted_mode;
	int vblank_id;
	unsigned int infinity_row;
	unsigned int infinity_col;
	struct GB02STR190 order;
	struct GB02STR191 geometry;
	struct GB02STR189 padding;
	bool virt_need_vsync;
	bool need_disable;
};

enum gbdc_pending {
	GBDC_PENDING_FB_UNREF,
};

#define GB02FUNC1573(x)   container_of(x, struct gbdc_crtc, base)
#define to_gbdc_crtc_state(x) container_of(x, struct gbdc_crtc_state, base)
void GB02FUNC1054(struct drm_crtc *crtc);
int GB02FUNC1139(struct drm_device *drm_dev, struct GB02STR245 *kms_info);
int GB02FUNC1134(struct drm_device *drm_dev, int crtc_id);
int GB02FUNC970(struct drm_crtc *crtc, int x, int y,struct drm_framebuffer *old_fb);

int GB02FUNC1032(struct drm_crtc *crtc,int x, int y);
int GB02FUNC1045(struct drm_crtc *crtc,int x, int y);
void GB02FUNC1026(struct drm_crtc *crtc);
int GB02FUNC1028(struct drm_plane *plane, struct drm_crtc *crtc);

int GB02FUNC1140(struct drm_device *drm_dev, struct GB02STR245 *kms_info, int vidx);
struct drm_vblank_crtc *GB02FUNC1011(struct drm_crtc *crtc);
#endif
