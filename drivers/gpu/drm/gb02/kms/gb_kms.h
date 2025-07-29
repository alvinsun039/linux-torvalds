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
#include <drm/drm_mm.h>
#include <drm/drm_property.h>
//#include <ip/gb_edma.h>
//#include <ip/gb_v2vdma.h>
//#include "kms/device/gb_dev_res.h"
#include "gpu/gb_device.h"
#include "kms/gbdc_mode.h"
#ifndef		__GB_KMS_H__
#define		__GB_KMS_H__

struct GB02STR77;
struct GB02STR244 {
	int (*enter_config)(void __iomem *dc_base, void __iomem *de_base);
	int (*config_mode)(void __iomem *de_base, struct videomode *vmode);
	int (*gamma_set)(void __iomem *dc_base,
			 void __iomem *de_base, struct drm_color_lut *lut);
	int (*config_base)(struct drm_crtc *dcrtc, 	struct GB02STR77 *plane, void __iomem *de_base);
	int (*config_base_atomic)(struct drm_crtc *dcrtc, struct GB02STR77 *plane, struct drm_plane_state *state, void __iomem *de_base);
	int (*get_resolution)(struct drm_display_mode *dmode);
	int (*config_plane_base)(struct drm_plane *dplane, struct GB02STR77 *plane, void __iomem *de_base);
	int (*query_hw)(void __iomem *dc_base);
	int (*save_csr_image)(struct GB02STR77 *plane,
	 void __iomem *dc_base, void __iomem *de_base, uint64_t csr_addr,
	  int width, int height);
	int (*config_csr_pos)(struct GB02STR77 *plane,
	 void __iomem *dc_base, void __iomem *de_base, int x, int y,
	  int width, int height, int stride);
	int (*hide_csr)(struct GB02STR77 *plane, void __iomem *dc_base,
	 void __iomem *de_base);
	int (*leave_config)(void __iomem *dc_base, void __iomem *de_base);
	int (*config_overly_plane)(struct drm_crtc *dcrtc, struct drm_plane *drm_planes,
		struct GB02STR77 *plane, struct drm_framebuffer *fb,
		void __iomem *de_base, int plane_id);
	int (*colse_overly_plane)(u16 base, void __iomem *dc_base,
		void __iomem *de_base);
	int (*config_virt_base_atomic)(struct drm_crtc *dcrtc,
		struct GB02STR77 *plane, struct drm_plane_state *state);
};

struct GB02STR245{
	struct GB02STR160			fb_info;
	struct GB02STR244			*kms_ops;
	struct GB02STR79			*connector_res;
	struct GB02STR77			*plane_res;
	struct GB02STR80 			*plane_cap;
	int					num_plane_cap;
	int					num_crtc;
	int 					num_connector;
	int					num_plane;
	bool 					mode_config_initialized;
	const struct GB02STR81		*max_res;
	bool infinity;
	struct gbdc_crtc *crtcs[GB02MAC2693 + (GB02MAC2693 / 2)];
	struct list_head  virt_crtc_head;
	struct list_head  virt_conn_head;
	int virt_crtcid;

	struct drm_property *infinity_set_property;//phyconn
	struct drm_property *virtual_flag_property;//phyconn

	/*virt*/
	struct drm_property *infinity_enable_property;
	struct drm_property *infinity_info_property;
	struct drm_property *infinity_adjust_property;

	struct list_head infinity_list;
	spinlock_t infinity_lock;
	struct delayed_work infinity_hotplug_work;
	bool connect_status_change;
	struct mutex gbdc_infinity_atomic_mutex;
};

int GB02FUNC1899(struct GB02STR245 *kms_info, struct drm_device *drm_dev, struct GB02STR253 *vram_config);
void GB02FUNC1900(struct GB02STR245 *kms_info, struct drm_device *drm_dev);
int GB02FUNC1890(struct drm_device *drm_dev, void *data,
	struct drm_file *filp);
int GB02FUNC1885(struct drm_device *drm_dev, void *data,
	struct drm_file *filp);
int GB02FUNC1894(struct drm_device *drm_dev, void *data,
	struct drm_file *filp);
int GB02FUNC1891(struct drm_device *drm_dev, void *data,
	struct drm_file *filp);

#define GB02MAC2830 0
#define GB02MAC2831 1
#define GB02MAC2832 2
#define GB02MAC2833 3
#define GB02MAC2834 4
#define GB02MAC2835 5
#define GB02MAC2836 6
#define GB02MAC2837 7
#define GB02MAC2838 8
#define GB02MAC2839 9
int GB02FUNC1904(enum gb_board_type gb_type, int id);
int GB02FUNC1903(enum gb_board_type gb_type, int vidx);
int GB02FUNC1902(enum gb_board_type gb_type, int id);
struct GB02STR245 *GB02FUNC1901(void);

#ifdef GB_ALLSCREEN
struct GB02STR246 {
		atomic_t flag;
		phys_addr_t fb_addr;
		u64 height;
		u64 width;
		u64 size;
		struct drm_mm_node node;
};
#define DC_FB 0
#define GB02MAC2840 1
#define GB02MAC2841 2
#define GB02MAC2842 3
#define GB02MAC2843 6

struct GB02STR247 {
		int expend_flag;
		struct GB02STR77 *plane_res;
		struct GB02STR246 crtc_plane[GB02MAC2843];
		struct GB02STR246 info[GB02MAC2842];
};

struct GB02STR247 *GB02FUNC161(void);
int GB02FUNC1892(struct drm_device *drm_dev, void *data,
	struct drm_file *filp);
int GB02FUNC1893(struct drm_device *drm_dev, void *data,
	struct drm_file *filp);
int GB02FUNC164(struct GB02STR155 *gbdc_dev, struct GB02STR247 *fb_info,
	int fb_type, int expend_flag);
#endif
int GB02FUNC1884(struct drm_device *drm_dev, void *data,
	struct drm_file *filp);
#endif
