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
#ifndef		__GBDC_CONNECTER_H__
#define		__GBDC_CONNECTER_H__
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include "gbdc_vga_bridge.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_dp_helper.h>
#else
#include <drm/display/drm_dp.h>
#include <drm/display/drm_dp_helper.h>
#endif
#include "gbdc_infinity.h"


#define GB02MAC1190 GB02MAC2693
#define GB02MAC1191 GB02MAC2693
enum genbu_connector_id {
	CONNECTOR0 = 0,
	CONNECTOR1,
	CONNECTOR_MAX,
};

struct GB02STR134 {
	struct i2c_adapter adapter;
	struct mutex lock; /* used to serialize data transfers */
	u8 stat;
	u8 slave_reg;
	bool is_regaddr;
	bool is_segment;
};
struct GB02STR135 {
	unsigned int vwidth;
	unsigned int vheight;
	unsigned int max_wpad;
	unsigned int max_hpad;
};
struct gbdc_connector {
	struct drm_connector base;
	uint32_t connector_id;
	uint32_t devices;
	struct GB02STR134 *ddc_bus;
	struct i2c_adapter *ddc;
	/* we need to mind the EDID between detect
	   and get modes due to analog/digital/tvencoder */
	struct edid *edid;
	uint16_t connector_object_id;
	u8 	plugged_state;
	enum gb_drm_output_type			type;
	bool sink_is_hdmi;
	struct GB02STR243 vga;
	struct drm_bridge *bridge;
	struct drm_dp_aux aux;

	/*virt*/
	bool virt;
	bool edid_getted;
	bool init_array_done;
	bool fill_modes_done;
	int modes_cnt;

	wait_queue_head_t waitq;
	struct gbdc_connector ***connector_array;
	int grp_id;
	struct list_head node;/*virt node*/
	struct list_head head;/*virt head link all real connector*/
	struct list_head modes;//backup modes

	bool infinity_inuse;
	unsigned int infinity_row;
	unsigned int infinity_col;
	unsigned int last_row,last_col;
	struct drm_property_blob *infinity_info_blob_ptr;
	struct GB02STR190 order;
	struct GB02STR189 padding[GB02MAC2560];
	enum drm_connector_status connected;
	struct mutex gbdc_infinity_atomic_mutex;
	struct mutex gbdc_infinity_mutex;
	int cur_pad_width;
	int cur_pad_height;//current mode pad
	struct drm_display_mode *need_pad_mode;
	struct GB02STR135 vpad[GB02MAC2560];
};

struct gbdc_connector_state {
	struct drm_connector_state base;

	bool is_virtual;
	bool gbdc_infinity_enable;
	bool crtc_need_disable;

	/* infinity */
	struct gbdc_connector *infinity_connector_list;

	struct GB02STR196 infinity_set;
	struct GB02STR195 infinity_adj;
};

#define to_gbdc_connector_state(x)\
	container_of((x), struct gbdc_connector_state, base)
struct gbdc_connector *GB02FUNC936(struct drm_device *dev, int conn_id, bool virt);
#define to_gbdc_connector(x)   container_of(x, struct gbdc_connector, base)
int GB02FUNC951(struct drm_connector *connector);
struct drm_display_mode *GB02FUNC906(struct drm_display_mode *virt_mode,
					struct gbdc_connector *phy_conn);
int GB02FUNC963(
                        struct gbdc_connector *gbdc_connector,
						enum gbdc_infinity_info_type type,
                        unsigned int pos,
						void *data);
void GB02FUNC964(struct drm_connector *connector,
						struct drm_connector_state *state);
enum drm_connector_status GB02FUNC874(struct drm_connector
						  *connector);
enum drm_connector_status GB02FUNC872(int id);
bool GB02FUNC812(struct gbdc_connector *vconn, struct drm_display_mode *mode);
void GB02FUNC894(struct drm_connector *connector);
bool GB02FUNC890(struct drm_connector *connector);
bool GB02FUNC891(struct drm_connector *connector);
#endif
