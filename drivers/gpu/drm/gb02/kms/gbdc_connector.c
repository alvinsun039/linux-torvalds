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
#include <generated/uapi/linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>

#include <drm/drm_gem.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#include <drm/drm_atomic_helper.h>
#include <linux/io.h>
#include <linux/console.h>
#if (defined CONFIG_CENTOS && !defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#include <drm/drm_probe_helper.h>
#endif

#include "common/gb_common.h"
#include "ip/gb_dp.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "gbdc_drv.h"
#include "device/gbdc_edid.h"
#include "gbdc_vga_bridge.h"
#include "gbdc_hdmi_ddc.h"
#include "gb_device.h"

static int defx = 1920;
static int defy = 1080;
bool hdmi_conn_sink_is_hdmi = true;

#define GB02MAC1112	(600000) /* 162MHZ */
#define GB02MAC1113	(600000) /* 85.5MHZ */

bool GB02FUNC759(void)
{
	return hdmi_conn_sink_is_hdmi;
}

int GB02FUNC760(struct drm_connector *connector,
			  int hdisplay, int vdisplay)
{
	int i, count, num_modes = 0;
	struct drm_display_mode *mode;
	struct drm_device *dev = connector->dev;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
	if (hdisplay < 0)
		hdisplay = 0;
	if (vdisplay < 0)
		vdisplay = 0;

	count = GB02FUNC117(gbdc_conn->connector_id);
	for (i = 0; i < count; i++) {
		const struct drm_display_mode *ptr =
		 GB02FUNC119(gbdc_conn->connector_id, i);

		if (hdisplay && vdisplay) {
			/*
			 * Only when two are valid, they will be used to check
			 * whether the mode should be added to the mode list of
			 * the connector.
			 */
			if (ptr->hdisplay > hdisplay ||
			    ptr->vdisplay > vdisplay)
				continue;
		}
		if (drm_mode_vrefresh(ptr) > 61)
			continue;
		mode = drm_mode_duplicate(dev, ptr);
		if (mode) {
			drm_mode_probed_add(connector, mode);
			num_modes++;
		}
	}
	return num_modes;
}

static int GB02FUNC764(struct drm_connector *connector)
{
	struct edid *edid;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
	int ret = 0;

	edid = (struct edid *)GB02FUNC1504(gbdc_conn->connector_id);
#if (!(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	defined SYS_CENTOS7_COMPILE_ENV) \
	|| (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0) &&\
		!defined CONFIG_CENTOS_OS)
	ret = drm_mode_connector_update_edid_property(connector, edid);
#else
	ret = drm_connector_update_edid_property(connector, edid);
#endif

	if (ret) {
		gb_printf(KERN_ERR, "%s %d connector update edid fail!ret:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}
	ret = drm_add_edid_modes(connector, edid);
	if (ret == 0)
		gb_printf(KERN_ERR, "%s %d add edid modes err id=%x!ret:%d\n",
			__func__, __LINE__, gbdc_conn->connector_id, ret);

	return ret;
}

struct gbdc_connector *** GB02FUNC769(int rows, int cols)
{
	struct gbdc_connector ***connector_array = NULL;
	int i, j;

	if (rows > GB02MAC1190 || cols > GB02MAC1191)
		return NULL;

	connector_array = kzalloc(rows * sizeof(*connector_array), GFP_KERNEL);
	if (!connector_array) {
			printk(KERN_ERR "Failed to alloc memory for conn array\n");
			return NULL;
	}

	pr_debug("%s: rows %d,cols %d\n",__func__,rows,cols);
	for (i = 0; i < rows; i++) {
		connector_array[i] = kzalloc(cols * sizeof(*connector_array[i]), GFP_KERNEL);
		if (!connector_array[i]) {
			for (j = 0; j < i; j++) {
				kfree(connector_array[j]);
			}
			kfree(connector_array);
			printk(KERN_ERR "Failed to alloc memory for row %d of connec array\n", i);
			return NULL;
		}

		memset(connector_array[i], 0, cols * sizeof(*connector_array[i]));
	}

	return connector_array;
}
void  GB02FUNC771(struct gbdc_connector *vconn ,int rows, int cols)
{
	int row = 0;

	if (!vconn->connector_array)
		return;
	for(row = 0; row < rows; row++)
		kfree(vconn->connector_array[row]);

	kfree(vconn->connector_array);

}
struct gbdc_connector ***init_connector_array(struct gbdc_connector *virt_conn, int rows, int cols)
{
	//struct gbdc_connector ***connector_array;
	struct gbdc_connector *gbdc_connector;
	struct gbdc_connector_state *gbdc_connector_state;
	int i, j;

	gbdc_connector_state = to_gbdc_connector_state(virt_conn->base.state);
	if (!gbdc_connector_state) {
		gb_printf(KERN_ERR, "%s:gbdc_connector_state null\n",__func__);
		return NULL;
	}

	if (!virt_conn->connector_array) {
		virt_conn->connector_array = GB02FUNC769(rows, cols);//create once
		virt_conn->last_row = rows;
		virt_conn->last_col = cols;
	}
	else {
		if(virt_conn->last_row != rows || virt_conn->last_col !=cols) {
			GB02FUNC771(virt_conn, virt_conn->last_row,
						virt_conn->last_col);//free prev
			virt_conn->connector_array = GB02FUNC769(rows, cols);//alloc new
			virt_conn->last_row = rows;
			virt_conn->last_col = cols;//update
		}
	}

	i = 0;
	j = 0;

	gbdc_for_each_gbdc_obj(connector) {
		if(!gbdc_connector) {
				continue;
		}
		i = gbdc_connector->order.x;
		j = gbdc_connector->order.y;
		if (i >= rows || j >= cols)
			continue;
		pr_debug("phyconn arr[%d][%d] is phyconn %d\n",i ,j ,gbdc_connector->connector_id);
		if (virt_conn->connector_array[i][j] != gbdc_connector)
			virt_conn->connector_array[i][j] = gbdc_connector;
	}
	//virt_conn->connector_array = connector_array;
	pr_debug("%s done\n",__func__);

	return virt_conn->connector_array;
}
static bool GB02FUNC777(struct gbdc_connector *vconn, struct drm_display_mode *vmode,
			int *wpad,int *hpad)
{
	//int cur_rows,cur_cols;
	int idx = 0;
	int rows,cols;
	bool is_pad_mode = false;

	rows = vconn->infinity_row;
	cols = vconn->infinity_col;

	for(idx = 0; idx < ARRAY_SIZE(vconn->vpad); idx++) {
		if(!vconn->vpad[idx].vwidth || !vconn->vpad[idx].vheight)
			continue;
		if(((vconn->vpad[idx].vwidth + vconn->vpad[idx].max_wpad) == vmode->hdisplay)
			&& ((vconn->vpad[idx].vheight + vconn->vpad[idx].max_hpad) == vmode->vdisplay)) {
				*wpad = vconn->vpad[idx].max_wpad;
				*hpad = vconn->vpad[idx].max_hpad;
				is_pad_mode = true;
				break;
			}
	}
	//if((*wpad) || (*hpad))
	//	dump_func_param("wpad(%d),hpad(%d)\n",*wpad,*hpad);
	return is_pad_mode;

}
static bool GB02FUNC780(struct gbdc_connector *vconn,
				struct drm_display_mode *vmode1, struct drm_display_mode *mode2)
{
	u16 m1_hdisplay, m1_vdisplay;
	int wpad = 0 ,hpad = 0;
	int vrefresh1, vrefresh2;

	if (vmode1->status != MODE_OK )
		pr_warn("%s: mode1 status not ok\n",__func__);
	if (vmode1->status != MODE_OK)
		pr_warn("%s: mode2 status not ok\n",__func__);

	m1_hdisplay = vmode1->hdisplay;
	m1_vdisplay = vmode1->vdisplay;
	vrefresh1 = drm_mode_vrefresh(vmode1);
	vrefresh2 = drm_mode_vrefresh(mode2);

	if(GB02FUNC777(vconn, vmode1, &wpad,&hpad)) {
		//dump_mode(vmode1,true);
		m1_hdisplay -= wpad;//GB02MAC2689(vmode1->private_flags);
		m1_vdisplay -= hpad;//GB02MAC2688(vmode1->private_flags);
		pr_debug("%s 2nd: m1_hdisplay %d,m1_vdisplay %d\n",__func__,m1_hdisplay,m1_vdisplay);
	}
	return ((m1_hdisplay % mode2->hdisplay == 0)
		&& (m1_vdisplay % mode2->vdisplay == 0)
		&& (vrefresh1 == vrefresh2));
}


static bool GB02FUNC783(struct drm_display_mode *mode, struct drm_connector *connector,
					struct drm_display_mode **ret_mode)
{
	struct drm_display_mode *entry;
	struct list_head *modes_head;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
	bool is_empty = list_empty(&connector->modes);

	modes_head = is_empty ? &gbdc_conn->modes : &connector->modes;
	pr_debug("%s: conn%d mode empty %d\n", __func__, gbdc_conn->connector_id, is_empty);
	list_for_each_entry(entry, modes_head, head) {
		if (drm_mode_match(mode, entry, DRM_MODE_MATCH_TIMINGS
			| DRM_MODE_MATCH_CLOCK)) {
		*ret_mode = entry;
		return true;
		}
	}
	return false;
}
static bool GB02FUNC786(struct GB02STR189 *mode, struct GB02STR189 *other)
{
	if(!mode || !other)
		return 0;
	return (mode->width == other->width &&
		mode->height == other->height);
}
static bool GB02FUNC787(struct GB02STR189 *mode, struct gbdc_connector *connector,
					struct GB02STR189 **ret_mode)
{
	struct GB02STR189 *entry;
	int idx;

	for(idx = 0 ;idx < ARRAY_SIZE(connector->padding); idx++){
		entry = &connector->padding[idx];
		if(GB02FUNC786(mode, entry)) {
			*ret_mode = entry;
			pr_debug("%s:match entry width %d height %d\n",__func__, entry->width, entry->height);
			//dump_mode_pad(connector->padding, idx);
			return true;
		}
	}

	return false;
}
static int GB02FUNC791(struct gbdc_connector *vconn, struct GB02STR189 *phymode,
				int row,int col)
{
	int vidx = 0;

	for(vidx = 0 ;vidx < ARRAY_SIZE(vconn->vpad); vidx++){
		if(!vconn->vpad[vidx].vheight || !vconn->vpad[vidx].vwidth)
			continue;
		if((vconn->vpad[vidx].vheight == phymode->height * row) &&
			(vconn->vpad[vidx].vwidth == phymode->width * col))
			return vidx;
	}
	return -1;
}
static void __maybe_unused GB02FUNC794(struct gbdc_connector *vconn)
{
	int idx = 0;

	for(idx =0; idx < ARRAY_SIZE(vconn->vpad);idx++){
		//if(vconn->vpad[idx].vwidth || vconn->vpad[idx].vwidth)
			dump_vmode_pad(vconn->vpad, idx);
	}
}
int GB02FUNC796(struct gbdc_connector *virt_conn, int row_cnt, int col_cnt)
{
	int row, col;
	struct gbdc_connector *gbdc_phyconn;
	struct gbdc_connector *gbdc_1stconn;
	int max_padw = 0 , max_padh = 0, pad_w, pad_h;
	int idx ,vidx = 0 ;
	//int *pad_h = NULL, *pad_w = NULL;
	int found = 0;
	struct GB02STR189 *phy_1stpadding ;//= virt_conn->connector_array[0][0]->padding[0];
	struct GB02STR189 *ret_mode = NULL;

	gbdc_1stconn = virt_conn->connector_array[0][0];


	memset(virt_conn->vpad, 0, sizeof(virt_conn->vpad));

	for (idx = 0; idx < ARRAY_SIZE(gbdc_1stconn->padding) ; idx++) {
		max_padw = 0;

		phy_1stpadding = &gbdc_1stconn->padding[idx];
		found = 0;
		ret_mode = NULL;

		if(!phy_1stpadding->width || !phy_1stpadding->height)
			continue;
		pr_debug("conn%d:%dx%d:\n",gbdc_1stconn->connector_id,phy_1stpadding->width,
			phy_1stpadding->height);
		//dump_mode_pad(phy_1stpadding,idx);
		for (row = 0; row < row_cnt; row++) {
			pad_w = 0;
			for (col = 0; col < col_cnt; col++) {
				gbdc_phyconn = virt_conn->connector_array[row][col];
				/*if(gbdc_phyconn == gbdc_1stconn)
					continue;*/
				if(GB02FUNC787(phy_1stpadding, gbdc_phyconn,&ret_mode)) {
					found++;
					pad_w += ret_mode->left + ret_mode->right;
				}
			}//col
			if(pad_w > max_padw)
				max_padw = pad_w;
		}//row
		if((found) == (row_cnt * col_cnt)) {
			virt_conn->vpad[vidx].vwidth = phy_1stpadding->width * col_cnt;
			virt_conn->vpad[vidx].vheight = phy_1stpadding->height * row_cnt;
			virt_conn->vpad[vidx].max_wpad = max_padw;
			vidx++;
		}
	}//for

	for (idx = 0; idx < ARRAY_SIZE(gbdc_1stconn->padding) ; idx++) {
		phy_1stpadding = &gbdc_1stconn->padding[idx];
		found = 0;
		ret_mode = NULL;
		max_padh = 0;

		if(!phy_1stpadding->width || !phy_1stpadding->height)
			continue;
		for (col = 0; col < col_cnt; col++) {
			pad_h = 0;
			for (row = 0; row < row_cnt; row++) {
				gbdc_phyconn = virt_conn->connector_array[row][col];
				/*if(gbdc_phyconn == gbdc_1stconn)
					continue;*/
				if(GB02FUNC787(phy_1stpadding, gbdc_phyconn, &ret_mode)) {
					found++;
					pad_h += ret_mode->top + ret_mode->bottom;
				}
			}//row
			if(pad_h > max_padh)
				max_padh = pad_h;
		}//col
		pr_debug("col search,found %d\n",found);
		if((found) == (row_cnt * col_cnt)) {
			vidx = GB02FUNC791(virt_conn, phy_1stpadding, row_cnt, col_cnt);
			if(vidx != -1)
				virt_conn->vpad[vidx].max_hpad = max_padh;

		}
	}
	//GB02FUNC794(virt_conn);
	/*if(pad_h)
		kfree(pad_h);
	if(pad_w)
		kfree(pad_w);*/
	return 0;
}
struct drm_display_mode *GB02FUNC804(struct gbdc_connector *vconn,bool need_mode_pad)
{
	struct drm_connector *connector = &vconn->base ;
	//struct gbdc_connector_state *gbdc_connector_state;

	struct drm_connector_state *conn_state;
	struct drm_crtc *crtc;
	struct drm_display_mode *vmode = NULL;

	conn_state = connector->state;
	if (conn_state && conn_state->crtc) {
		crtc = conn_state->crtc;
	}
	else {
		drm_for_each_crtc(crtc, connector->dev) {
			if(crtc->index == connector->index)
				break;
		}
	}

	if (crtc->state) {
		vmode = &crtc->state->adjusted_mode;
	}
	else
		vmode = &crtc->mode;
	//pr_info("%s: curr vmode dump\n",__func__);
	dump_mode_more(vmode, false);

	return  vmode;
}
#if 0
bool GB02FUNC809(struct gbdc_connector *virt_conn)
{
	struct gbdc_connector *gbdc_conn;
	bool need_mode_pad = false;

	list_for_each_entry(gbdc_conn, &virt_conn->head, head) {
		if(gbdc_conn->padding.left || gbdc_conn->padding.right
			|| gbdc_conn->padding.top || gbdc_conn->padding.bottom) {
				need_mode_pad = 1;
				break;
			}
	}
	gb_printf(KERN_ERR, "%s: need_mode_pad %d\n",__func__, need_mode_pad);
	return need_mode_pad;
}
#endif
bool GB02FUNC812(struct gbdc_connector *vconn, struct drm_display_mode *mode)
{
	struct drm_display_mode *vnative_mode;
	bool found = false;
	int wpad = 0 ,hpad = 0;

	if (!mode || !vconn)
		return false;

	list_for_each_entry(vnative_mode, &vconn->base.modes, head) {
		if (drm_mode_equal(vnative_mode, mode)) {
			found = true;
			break;
		}
	}
	if (found && GB02FUNC777(vconn,vnative_mode,&wpad,&hpad))
		return true;
	else
		return false;


}
struct GB02STR189 GB02FUNC814(struct drm_display_mode *virt_mode,
					struct gbdc_connector *phy_conn)
{
	struct GB02STR189 ret = {0};
	struct drm_display_mode  *phy_mode = NULL;
	int midx = 0;
	phy_mode = GB02FUNC906(virt_mode, phy_conn);

	if(phy_mode){
		for (midx = 0; midx < ARRAY_SIZE(phy_conn->padding); midx++) {
			if (phy_conn->padding[midx].width == phy_mode->hdisplay &&
				phy_conn->padding[midx].height == phy_mode->vdisplay) {
					ret = phy_conn->padding[midx];
					break;
				}
		}
	}
	return ret;
}
static bool GB02FUNC816(struct gbdc_connector *vconn, int w, int h)
{
	int idx = 0;
	bool has_pad = false;
	struct GB02STR135 vmode_pad = {0};

	for (idx = 0 ; idx < ARRAY_SIZE(vconn->vpad); idx++){
		vmode_pad = vconn->vpad[idx];
		if (!vmode_pad.vwidth || !vmode_pad.vheight)
			continue;
		if (vmode_pad.vwidth == w && vmode_pad.vheight == h){
			has_pad = true;
			vconn->cur_pad_width = vmode_pad.max_wpad;
			vconn->cur_pad_height = vmode_pad.max_hpad;
		}
	}

	if (has_pad)
		return true;
	else
		return false;
}
#if 0
int __maybe_unused GB02FUNC818(struct gbdc_connector *vconn)
{
	struct drm_display_mode *vmode;
	struct drm_display_mode *pmode;
	int modes = 0 ;
	bool need_dup = false;
	int saved_width,saved_height;

	if(list_empty(&vconn->base.modes)) {
		pr_info("%s,vconn mode empty\n",__func__);
	 	return 0;
	}

	list_for_each_entry(vmode, &vconn->base.modes, head) {
		if (vmode->private_flags & GB02MAC2681) {

			dump_func_param("vmode->padwidth %d,padheight %d\n",
					vmode->private_flags & GB02MAC2683,
				(vmode->private_flags & GB02MAC2685) >>10);
			saved_width = GB02MAC2689(vmode->private_flags);
			saved_height = GB02MAC2688(vmode->private_flags);
			dump_mode(vmode, true);
			pmode = drm_mode_duplicate(vconn->base.dev,vmode);
			if (pmode)
				drm_mode_probed_add(&vconn->base, pmode);
		}
	}
	/*if(need_dup) {
		list_for_each_entry(mode, &vconn->base.modes, head) {
			if (mode->status == MODE_STALE) {
					pmode = drm_mode_duplicate(vconn->base.dev,mode);
					if(pmode){
						dump_mode(mode,true);
						drm_mode_probed_add(&vconn->base, pmode);
						modes++;
					}
				}
		}
	}*/
	dump_func_param("modes %d,need_dup %d\n", modes,need_dup);
	return modes;
}
#endif
static int GB02FUNC823(struct drm_device *dev,struct gbdc_connector *virt_conn)
{
 	//struct gbdc_connector *conn[6];
	//struct gbdc_connector *tmp= virt_conn;
	struct drm_display_mode *first_mode;
	struct drm_display_mode *mode, *t;
	int row_cnt = 0;//virt_conn->infinity_row;
	int col_cnt = 0;//virt_conn->infinity_col;
	//struct gbdc_connector *first;
	struct gbdc_connector *first_connector = NULL, *tmp;
	int row, col, modes_cnt = 0, w, h ,wn, hn;
	struct gbdc_connector ***physical_connectors = NULL;
	struct  list_head *modes_head = NULL;
	//struct gbdc_connector *gbdc_connector;
	struct gbdc_connector_state *gbdc_connector_state;

	gbdc_connector_state = to_gbdc_connector_state(virt_conn->base.state);

	tmp = list_first_entry(&virt_conn->head, struct gbdc_connector, head);
	pr_debug("%s : %d x %d\n",__func__,virt_conn->infinity_row,virt_conn->infinity_col);
	if (tmp) {
		row_cnt = tmp->infinity_row;
		col_cnt = tmp->infinity_col;
		if(!row_cnt || !col_cnt
		|| row_cnt >  GB02MAC2558
		|| col_cnt > GB02MAC2558)
			return 0;
	}

	list_for_each_entry(tmp, &virt_conn->head, head) {
		if(!tmp)
			continue;
		pr_debug("%s:phyconn %d,row %d,col %d\n",__func__, tmp->connector_id,
			tmp->infinity_row, tmp->infinity_col);
		wait_event_interruptible_timeout(tmp->waitq, tmp->fill_modes_done, 5 * HZ);
		pr_debug("wait conn %d fill modes done\n",tmp->connector_id);

	}

	//pr_info("%s: infinityrow %d,infinity_col %d\n",__func__,virt_conn->infinity_row, virt_conn->infinity_col);
	if(!virt_conn->connector_array || !list_empty(&virt_conn->head))
		physical_connectors = init_connector_array(virt_conn, row_cnt, col_cnt);
	else
	 	physical_connectors = virt_conn->connector_array;

	if(physical_connectors)
		first_connector = physical_connectors[0][0];
	else
		return 0;

	if (!first_connector)
		return 0;

	pr_debug("%s: firstconn id %d ,modes empty %d\n", __func__, first_connector->connector_id,
			list_empty(&first_connector->base.modes));

	modes_head = &first_connector->base.modes;
	if (list_empty(modes_head) && !list_empty(&first_connector->modes))
		modes_head = &first_connector->modes;


	virt_conn->need_pad_mode = GB02FUNC804(virt_conn, true);
	GB02FUNC796(virt_conn, row_cnt, col_cnt);
	//GB02FUNC818(virt_conn);


	//if ((modes_cnt = GB02FUNC818(virt_conn))!= 0)
	//	return modes_cnt;

	list_for_each_entry(first_mode, modes_head, head) {

		bool found = true;
		bool mode_pad = false;
		struct drm_display_mode *phycon_mode = NULL;
		wn = 0; hn = 0;

		if (!first_mode)
			pr_info("first_mode is null\n");
		// Check if the mode exists in other phyconnector
		for (row = 0; row < row_cnt; row++) {
			for (col = 0; col < col_cnt; col++) {

			struct gbdc_connector *gbdc_phyconn = physical_connectors[row][col];

			// Skip the first connector
			if (gbdc_phyconn == first_connector)
				continue;

			if (!GB02FUNC783(first_mode, &gbdc_phyconn->base, &phycon_mode)) {
				found = false;
				break;
				}
			}//for
			if (!found)
				break;

		}//for

		pr_debug("1st conn,mode %px,vdisplay %d,hdisplay %d,status %d\n", first_mode,
		first_mode->vdisplay, first_mode->hdisplay, first_mode->status);
		pr_debug("found is %d\n",found);

		if (found) {
			struct drm_display_mode *new_mode = NULL;
			struct drm_display_mode *new_padmode = NULL;
			w = first_mode->hdisplay * col_cnt;
			h = first_mode->vdisplay * row_cnt;

			if(GB02FUNC816(virt_conn, w, h)){
				wn = w + virt_conn->cur_pad_width;
				hn = h + virt_conn->cur_pad_height;
				mode_pad = true;
				new_padmode = drm_gtf_mode(dev, wn, hn, drm_mode_vrefresh(first_mode),
								false, false);
				new_padmode->type = DRM_MODE_TYPE_DRIVER;
				new_padmode->hdisplay = wn;
				new_padmode->vdisplay = hn;
				//new_padmode->flags |= GB02MAC2681;
				/*new_padmode->rivate_flags |= GB02MAC2686(virt_conn->cur_pad_width)
						| GB02MAC2687(virt_conn->cur_pad_height);*/

				drm_mode_set_name(new_padmode);
				pr_debug("cur_pad_width %d,cur_pad_height %d\n",virt_conn->cur_pad_width,virt_conn->cur_pad_height);

				gb_printf(KERN_INFO, DRM_MODE_FMT, DRM_MODE_ARG(new_padmode));
				drm_mode_probed_add(&virt_conn->base, new_padmode);
			}

			new_mode = drm_gtf_mode(dev, w, h, drm_mode_vrefresh(first_mode),
								false, false);
			pr_debug("phy mode %p,hdisplay %d,vdisplay %d,status %d\n", phycon_mode,
				phycon_mode->hdisplay, phycon_mode->vdisplay, phycon_mode->status);
			pr_debug(DRM_MODE_FMT, DRM_MODE_ARG(phycon_mode));
			pr_debug("new_mode %p,hdisplay %d,vdisplay %d,status %d\n", new_mode,
					new_mode->hdisplay, new_mode->vdisplay, new_mode->status);
			if (new_mode) {
				new_mode->type = DRM_MODE_TYPE_DRIVER;
				if (first_mode->type & DRM_MODE_TYPE_PREFERRED)
					new_mode->type |= DRM_MODE_TYPE_PREFERRED;

				gb_printf(KERN_INFO, DRM_MODE_FMT, DRM_MODE_ARG(new_mode));
				drm_mode_probed_add(&virt_conn->base, new_mode);
			}
			else
				drm_mode_destroy(dev, new_mode);

			}

	}
	list_for_each_entry_safe(mode, t, &virt_conn->base.probed_modes, head) {
                if (mode->hdisplay >= 1024 && mode->vdisplay >= 768)
                        modes_cnt++;
                else {
                        list_del(&mode->head);
                        drm_mode_destroy(dev, mode);
                }
        }
	virt_conn->modes_cnt = modes_cnt;
	pr_debug("%s: mode_cnt %d\n",__func__, modes_cnt);

	return  modes_cnt;
}
static int GB02FUNC844(struct drm_device *dev,struct gbdc_connector *virt_conn)
{
	int ret_cnt  = 0;
//	struct gbdc_connector *entry;

	if (!list_empty(&virt_conn->head))
		ret_cnt = GB02FUNC823(dev , virt_conn);
	else
		pr_warn("virtconn head list is empty\n");
#if 0
	if(ret_cnt) {
		int pos,ret;
		list_for_each_entry(entry, &virt_conn->head, head) {
			pos = entry->order.y * virt_conn->infinity_row
				+ entry->order.x;

			GB02FUNC1419(virt_conn, INFINITY_PADDING,
				pos, entry->padding);
			ret = GB02FUNC963(virt_conn, INFINITY_PADDING ,
				pos, entry->padding);
			if(!ret)
				dump_func_param("update conn%d padding ok\n",entry->connector_id);
		}
	}
#endif
	return ret_cnt;
}

static int GB02FUNC849(struct drm_connector *connector)
{
	int ret_cnt = 0;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);

	if(gbdc_conn->virt) {

		//dump_current;
		ret_cnt = GB02FUNC844(connector->dev, gbdc_conn);
		pr_debug("%s: virtcon modes_ret %d\n", __func__, ret_cnt);
	}

	return ret_cnt;
}
static int GB02FUNC850(struct drm_connector *connector)
{
	int ret = 0;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
	bool edid_getted = GB02FUNC1503(gbdc_conn->connector_id);

	if (edid_getted) {
		ret = GB02FUNC764(connector);
		gbdc_conn->edid_getted = true;
	}

	if (!edid_getted || (ret == 0)) {
		ret = GB02FUNC760(connector, 16384, 16384);
		drm_set_preferred_mode(connector, defx, defy);
	}

	return ret;
}

static int GB02FUNC853(struct drm_connector *connector,
				     struct drm_display_mode *mode)
{
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);

	if (gbdc_conn->virt)
		return MODE_OK;
	/*
	 * Make sure we can fit two framebuffers into video memory.
	 * This allows up to 1600x1200 with 16 MB (default size).
	 * If you want more try this:
	 *     'qemu -vga std -global VGA.vgamem_mb=32 $otherargs'
	 */
	if (mode->clock > GB02MAC1112) {
	 	dev_err(connector->dev->dev, "invalid pixel clock(%d)KHZ\n",mode->clock);

		return MODE_BAD;
	}
	if ((gbdc_conn->connector_id == CONNECTOR1) &&
			 (mode->clock > GB02MAC1113)) {
	/* dev_err(connector->dev->dev, "invalid pixel clock(%d)KHZ\n",
	 * mode->clock);
	 */	gb_printf(KERN_DEBUG,"mode bad 2\n");
		return MODE_BAD;
	}

	if (gbdc_conn->connector_id == CONNECTOR1) {
		/* delete 1600x1200@60 mode */
		if ((mode->hdisplay == 1600) && (mode->vdisplay == 1200)) {
			gb_printf(KERN_DEBUG,"mode bad 3\n");
			return MODE_BAD;
		}
	}

	if (GB02FUNC507(gbdc_conn->connector_id)) {
		if ((mode->hdisplay > 1920) ||
		    (mode->vdisplay > 1080) ||
		    (mode->clock > 148500)){
			gb_printf(KERN_DEBUG,"mode bad 4\n");
			return MODE_BAD;
		    }
	}
	//pr_info("%s: conn idx %d MODE OK\n",__func__, gbdc_conn->connector_id);
	return MODE_OK;
}

static struct drm_encoder *GB02FUNC859(struct drm_connector
						       *connector)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
	int enc_id = connector->encoder_ids[0];

	/* pick the encoder ids */
	if (enc_id) {
#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
		gb_printf(KERN_INFO, "--best encoder---\n");
		return drm_encoder_find(connector->dev, NULL, enc_id);
#else
		return drm_encoder_find(connector->dev, enc_id);
#endif
	}

	gb_printf(KERN_INFO, "@@@@@@@@@@@@@@@@@@@@@@@@@@@ %s: %d return NULL\n",
	       __func__, __LINE__);

	return NULL;
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0) */
	struct drm_encoder *encoder;

	/* There is only one encoder per connector */
	drm_connector_for_each_possible_encoder(connector, encoder)
		return encoder;

	return NULL;
#endif
}

static const struct drm_connector_helper_funcs gbdc_connector_connector_helper_funcs = {
	.get_modes = GB02FUNC850,
	.mode_valid = GB02FUNC853,
	.best_encoder = GB02FUNC859,
};

static const struct drm_connector_helper_funcs gbdc_virt_connector_helper_funcs = {
	.get_modes = GB02FUNC849,
	.mode_valid = GB02FUNC853,
	.best_encoder = GB02FUNC859,
};
static enum drm_connector_status GB02FUNC865(struct drm_connector
						  *connector, bool force)
{
	int conn_id;
	unsigned int conn_stat = 0;
	struct GB02STR155 *gb_dev = GB02FUNC85(connector->dev->dev_private);
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);

	conn_id = gbdc_connector->connector_id;

	if (!gbdc_connector->virt && modeset_ops->connector_state) {
		if (!GB02FUNC1680(connector, NULL, false))
			conn_stat = modeset_ops->connector_state(
					&gb_dev->pcie_info.ip_config, conn_id);
		else
			conn_stat = 0;
	}
	else {
		if (!GB02FUNC1680(connector, NULL, false))
			conn_stat = 0;
		else
			conn_stat = 1;
	}

	conn_stat = conn_stat ? connector_status_connected :
				connector_status_disconnected;

	gb_printf(KERN_INFO, "gbdc crtc%d: %s\n", conn_id, conn_stat ==
		connector_status_connected ? "connected" : "disconnected");
	return conn_stat;
}

enum drm_connector_status GB02FUNC872(int id)
{
	return connector_status_disconnected;
}

enum drm_connector_status GB02FUNC874(struct drm_connector
						  *connector)
{
	int conn_id;
	unsigned int conn_stat = 0;
	struct GB02STR155 *gb_dev = GB02FUNC85(connector->dev->dev_private);
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);

	conn_id = gbdc_connector->connector_id;

	if (!gbdc_connector->virt && modeset_ops->connector_state)
		conn_stat = modeset_ops->connector_state(
				&gb_dev->pcie_info.ip_config, conn_id);

	conn_stat = conn_stat ? connector_status_connected :
				connector_status_disconnected;

	return conn_stat;
}

static __maybe_unused enum drm_connector_status GB02FUNC876(struct drm_connector
						  *connector, bool force)
{
	int conn_id;
	unsigned int conn_stat = 0;
	//struct GB02STR155 *gb_dev = GB02FUNC85(connector->dev->dev_private);
	//struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);

	conn_id = gbdc_connector->connector_id;

	conn_stat = conn_stat ? connector_status_connected :
				connector_status_disconnected;
	if (gbdc_connector->virt)
		conn_stat = connector_status_connected;
	gb_printf(KERN_INFO, "gbdc crtc%d: %s\n", conn_id, conn_stat ==
		connector_status_connected ? "connected" : "disconnected");
	return conn_stat;
}
static bool GB02FUNC880(struct GB02STR189 *padding,int size,
					struct drm_display_mode *mode)
{
  	int idx;
	bool found = false;
	for (idx = 0; idx < size; idx++){
		if(padding[idx].width == mode->hdisplay &&
			padding[idx].height == mode->vdisplay) {
				found = true;
				break;
			}
	}
	return found;
}
static void GB02FUNC881(struct gbdc_connector *phyconn)
{
	int idx = 0;
	for(idx = 0 ;idx < ARRAY_SIZE(phyconn->padding); idx++){
		if(phyconn->padding[idx].width || phyconn->padding[idx].height)
			gb_printf(KERN_INFO, "%d: %d x %d\n",idx, phyconn->padding[idx].width,
				phyconn->padding[idx].height);
	}
}
static void GB02FUNC883(struct gbdc_connector *gbdc_conn)
{
	struct drm_display_mode *mode, *cpy_mode;
	struct drm_display_mode *bmode, *bmt;
	struct list_head *src = NULL;
	int idx = 0;

	if (list_empty(&gbdc_conn->base.modes))
		return;

	src = &gbdc_conn->base.modes;

	if (!list_empty(&gbdc_conn->modes)) {//del prev saved mode
		list_for_each_entry_safe(bmode, bmt, &gbdc_conn->modes ,head) {
			if (bmode) {
				list_del(&bmode->head);
				pr_debug("Del prev saved %s mode\n", bmode->name);
				drm_mode_destroy(gbdc_conn->base.dev, bmode);
			}
		}
	}
	list_for_each_entry(mode, src, head) {//save
		cpy_mode = drm_mode_duplicate(gbdc_conn->base.dev, mode);
		if (!cpy_mode)
			DRM_ERROR("Failed to duplicate mode\n");
		else {
			pr_debug("%s: cpymode %px,name %s\n", __func__, cpy_mode, cpy_mode->name);
			list_add_tail(&cpy_mode->head, &gbdc_conn->modes);
			if (idx < ARRAY_SIZE(gbdc_conn->padding)) {
				if(!GB02FUNC880(gbdc_conn->padding,
						ARRAY_SIZE(gbdc_conn->padding),cpy_mode)) {

					gbdc_conn->padding[idx].width = cpy_mode->hdisplay;
					gbdc_conn->padding[idx].height = cpy_mode->vdisplay;
					idx++;
					pr_debug("%s: idx %d,%d x %d\n",__func__,idx,
						cpy_mode->hdisplay,cpy_mode->vdisplay);
					}
				}
			else
				WARN_ON(1);

		}
	}
	GB02FUNC881(gbdc_conn);

}

bool GB02FUNC890(struct drm_connector *connector)
{
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);

	return !!GB02FUNC1503(gbdc_conn->connector_id);
}

bool GB02FUNC891(struct drm_connector *connector)
{
        struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
        bool edid_getted = GB02FUNC1503(gbdc_conn->connector_id);
	int count = 20;

	while(unlikely(!edid_getted) && --count) {
		edid_getted = GB02FUNC1503(gbdc_conn->connector_id);
		msleep(100);
	}

        return !!edid_getted;
}

void GB02FUNC894(struct drm_connector *connector)
{
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);

	if(!gbdc_connector->virt && gbdc_connector->edid_getted){
		gbdc_connector->fill_modes_done = true;
		gb_printf(KERN_INFO, "phy conn%d fill modes done\n", gbdc_connector->connector_id);
		GB02FUNC883(gbdc_connector);
	}
}

static int GB02FUNC896(struct drm_connector *connector,
	uint32_t maxX, uint32_t maxY)
{
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	//struct gbdc_connector *virt;
	int ret = 0;

	if (GB02FUNC507(gbdc_connector->connector_id)) {
		maxX = 1920;
		maxY = 1080;
	}

	//connector->force = 0;
	gb_printf(KERN_INFO, "%s: gbdc_conn %d \n",__func__,gbdc_connector->connector_id);

	ret =  drm_helper_probe_single_connector_modes(connector,
			maxX, maxY);

	if(ret && (!gbdc_connector->virt) && gbdc_connector->edid_getted){
		gbdc_connector->fill_modes_done = 1;
		gb_printf(KERN_INFO, "phy conn%d fill modes done\n", gbdc_connector->connector_id);
		GB02FUNC883(gbdc_connector);
		wake_up_interruptible(&gbdc_connector->waitq);

	}
	return ret;
}
void GB02FUNC901(struct gbdc_connector *vconn)
{
	vconn->cur_pad_width = 0;
	vconn->cur_pad_height = 0;
	vconn->need_pad_mode = NULL;
}
#if 0
static __maybe_unused void GB02FUNC902(struct gbdc_connector *virt_conn)
{
	struct drm_display_mode *vmode, *phy_mode;
	struct gbdc_connector *phy_conn;
	struct gbdc_connector *gbdc_connector;
	struct gbdc_connector_state *gbdc_connector_state;
	struct list_head *modes_head = NULL;

	gbdc_connector_state = to_gbdc_connector_state(virt_conn->base.state);

	list_for_each_entry(vmode, &virt_conn->base.modes, head) {

		gb_printf(KERN_INFO, "virt mode dump-");
		if(vmode->private_flags & GB02MAC2681)
			continue;
		//dump_mode(vmode, true);
		//list_for_each_entry(phy_conn, &virt_conn->head, head) {
		gbdc_for_each_gbdc_obj(connector) {
			phy_conn = gbdc_connector;
			if (!phy_conn)
				continue;

			pr_debug("phyconn id %d\n", phy_conn->connector_id);
			modes_head = list_empty(&phy_conn->base.modes) ?
						&phy_conn->modes : &phy_conn->base.modes;
			list_for_each_entry(phy_mode, modes_head, head) {
				if (GB02FUNC780(virt_conn, vmode, phy_mode))
					phy_mode->private = (int *)vmode;
				//dump_mode(phy_mode);
				pr_debug("phyconn mode %px, private %px,hdisplay %d,vdisplay %d,status %d\n",
					phy_mode,phy_mode->private,
					phy_mode->hdisplay, phy_mode->vdisplay,phy_mode->status);
				}
		}
	}
	if (virt_conn->need_pad_mode)
		GB02FUNC901(virt_conn);


}
#endif
static struct gbdc_connector *GB02FUNC905(struct gbdc_connector *gbdc_conn)
{
	struct gbdc_connector *entry;
	//struct drm_connector_list_iter conn_iter;
	struct gbdc_connector *virt_conn = NULL;


	list_for_each_entry(entry, &gbdc_conn->head, head) {
		if (entry->virt) {
			virt_conn = entry;
			break;
		}
	}
	//if (virt_conn)
	//	dump_func_param("vconn id %d\n", virt_conn->connector_id);
	return virt_conn;
}
struct drm_display_mode *GB02FUNC906(struct drm_display_mode *virt_mode,
					struct gbdc_connector *phy_conn)
{
	struct drm_display_mode  *entry = NULL;
	struct drm_display_mode  *phy_mode = NULL;
	struct drm_display_mode  *vnative_mode = NULL;
	struct gbdc_connector	*virt_conn;
	struct list_head *modes_head;
	bool found = false;

	modes_head = list_empty(&phy_conn->base.modes) ?
				&phy_conn->modes : &phy_conn->base.modes;

	if (!virt_mode || !phy_conn)
		return NULL;

	virt_conn = GB02FUNC905(phy_conn);
	if (!virt_conn)
		return NULL;
	/*virt mode-->native mode*/
	list_for_each_entry(vnative_mode, &virt_conn->base.modes, head) {
		if (drm_mode_equal(vnative_mode, virt_mode)) {
			found = true;
			break;
		}
	}
	if (!found) {
		dump_func_param("cannot found phymode, get origin one\n");
		return NULL;
		/*#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
		if (virt_mode->private) {
			vnative_mode = (struct drm_display_mode *)virt_mode->private;
			if (vnative_mode)
				dump_mode_more(vnative_mode, true);
		} else {
			#if 0
			int distance = virt_mode->hdisplay;
			struct drm_display_mode *temp = NULL;
			dump_func_param("find nearest one\n");
			list_for_each_entry(vnative_mode, &virt_conn->base.modes, head) {
				int diff = virt_mode->hdisplay - vnative_mode->hdisplay;
				if (diff >= 0 && distance > diff) {
					distance = diff;
					temp = vnative_mode;
				}
			}
			vnative_mode = temp;
			#else
			struct drm_display_mode *temp = NULL;
			dump_func_param("find prefer one\n");
			list_for_each_entry(vnative_mode, &virt_conn->base.modes, head) {
				if (vnative_mode->type & DRM_MODE_TYPE_PREFERRED) {
					temp = vnative_mode;
					break;
				}
			}
			vnative_mode = temp;
			#endif
		}*/
	}

	if (!vnative_mode)
		return NULL;

	gb_printf(KERN_INFO, "virt_mode %px,%d:%d:%d\n", virt_mode,
			virt_mode->hdisplay, virt_mode->vdisplay, virt_mode->clock);
	gb_printf(KERN_INFO, "vnative_mode %px,%d:%d:%d\n", vnative_mode,
			vnative_mode->hdisplay, vnative_mode->vdisplay, vnative_mode->clock);

	if(1){
		list_for_each_entry(entry, modes_head, head) {
			if (GB02FUNC780(virt_conn, vnative_mode, entry)){
				phy_mode = entry;
				gb_printf(KERN_INFO, "padmode found conn%d,phymode %s:%d:%d\n",
				phy_conn->connector_id, phy_mode->name, drm_mode_vrefresh(phy_mode),
				phy_mode->clock);
				break;
			}
		}
	}
	/*else {
		list_for_each_entry(entry, modes_head, head) {
		if (entry->private == (int *)vnative_mode) {
			phy_mode = entry;
			gb_printf(KERN_ERR, "found conn%d,phymode %s:%d:%d\n",
			phy_conn->connector_id, phy_mode->name, phy_mode->vrefresh,
			phy_mode->clock);
			break;
			}
		}
	}*/

	if(!phy_mode)
		dump_func_param("cannot found phymode\n");

	return phy_mode;
}
static int GB02FUNC917(struct drm_connector *connector,
	uint32_t maxX, uint32_t maxY)
{
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	int ret = 0;

	ret =  drm_helper_probe_single_connector_modes(connector, maxX, maxY);

	if (ret && gbdc_connector->virt) {
		//gb_printf(KERN_INFO, "%s:call GB02FUNC902\n", __func__);
		//GB02FUNC902(gbdc_connector);
	}

	return ret;
}

static void GB02FUNC920(struct drm_connector *connector)
{
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);

	if (GB02FUNC951(connector))
		GB02FUNC1875(&gbdc_connector->vga);

	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);

	//kfree(gbdc_connector);
}

static int GB02FUNC922(struct drm_connector *connector,
				   struct drm_connector_state *state,
				   struct drm_property *property,
				   uint64_t val)
{
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev = GB02FUNC85(dev->dev_private);
	//struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	struct gbdc_connector_state *gbdc_state =
		to_gbdc_connector_state(state);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 20, 0)
	if (strcmp(current->comm, "st-infinity-vie"))
		return 0;
#endif
	if (property == gb_dev->kms_info.infinity_enable_property)
		gbdc_state->gbdc_infinity_enable = val;
	else if (property == gb_dev->kms_info.infinity_set_property) {
		if (val) {
			struct drm_property_blob *blob =
						drm_property_lookup_blob(dev, val);
			if (blob) {
				if (blob->length != sizeof(struct GB02STR196)) {
					GB_PRINT_INFO("infinity_set_property length (%ld) error, need %ld\n", \
						blob->length, sizeof(struct GB02STR196));
					return -EINVAL;
				}
				memcpy(&gbdc_state->infinity_set, blob->data, blob->length);
				GB_PRINT_INFO("infinity_set_property (%d) (%dx%d))\n", \
						gbdc_state->infinity_set.infinity_id, \
						gbdc_state->infinity_set.infinity_row, \
						gbdc_state->infinity_set.infinity_col);
			}
			drm_property_blob_put(blob);
		}
	} else if (property == gb_dev->kms_info.infinity_adjust_property) {
		if (val) {
			struct drm_property_blob *blob =
						drm_property_lookup_blob(dev, val);
			if (blob) {
				unsigned int *data = (unsigned int *)blob->data;
				unsigned int raw = data[0];
				unsigned int col = data[1];
				int len = sizeof(struct GB02STR195) + \
					(sizeof(struct GB02STR193) * raw * col);
				if (blob->length != len) {
					GB_PRINT_INFO("infinity_adjust_property length (%ld) error, need %d\n", \
						blob->length, len);
					return -EINVAL;
				}
				memcpy(&gbdc_state->infinity_adj, blob->data, blob->length);
				GB_PRINT_INFO("infinity_adjust_property (%dx%d))\n", \
						gbdc_state->infinity_adj.infinity_row, \
						gbdc_state->infinity_adj.infinity_col);
			}
			drm_property_blob_put(blob);
		}
	}

	return 0;
}

static int GB02FUNC926(struct drm_connector *connector,
				   const struct drm_connector_state *state,
				   struct drm_property *property,
				   uint64_t *val)
{
	int ret = 0;

	return ret;
}

void GB02FUNC928(struct drm_connector *connector,
									struct drm_connector_state *state)
{
	struct gbdc_connector_state *gbdc_conn_state = to_gbdc_connector_state(state);

	__drm_atomic_helper_connector_destroy_state(&gbdc_conn_state->base);

	kfree(gbdc_conn_state);

}

struct drm_connector_state *
gbdc_connector_atomic_duplicate_state(struct drm_connector *connector)
{
	struct gbdc_connector_state *state =
		to_gbdc_connector_state(connector->state);

	struct gbdc_connector_state *new_state = kzalloc(sizeof(*new_state) +
		GB02MAC2569, GFP_KERNEL);

	if (!new_state)
		return NULL;

	__drm_atomic_helper_connector_duplicate_state(connector, &new_state->base);

	new_state->is_virtual = state->is_virtual;
	new_state->gbdc_infinity_enable = state->gbdc_infinity_enable;
	memcpy(&new_state->infinity_set, &state->infinity_set, sizeof(state->infinity_set));
	new_state->infinity_connector_list = state->infinity_connector_list;
	memcpy(&new_state->infinity_adj, &state->infinity_adj, sizeof(state->infinity_adj));
	memcpy(new_state->infinity_adj.adjusts, state->infinity_adj.adjusts, GB02MAC2569);
	new_state->crtc_need_disable = false;

	return &new_state->base;
}

void GB02FUNC931(struct drm_connector *connector)
{
	struct gbdc_connector_state *gbdc_connector_state =
		kzalloc(sizeof(*gbdc_connector_state) +
			GB02MAC2569, GFP_KERNEL);

	if (connector->state)
		__drm_atomic_helper_connector_destroy_state(connector->state);

	kfree(connector->state);

	if (gbdc_connector_state) {
		__drm_atomic_helper_connector_reset(connector, &gbdc_connector_state->base);
		gbdc_connector_state->infinity_connector_list = to_gbdc_connector(connector);
		INIT_LIST_HEAD(&gbdc_connector_state->infinity_connector_list->head);
		gbdc_connector_state->is_virtual = to_gbdc_connector(connector)->virt;
	}
}

static const struct drm_connector_funcs gbdc_connector_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.fill_modes = GB02FUNC896,
	.destroy = GB02FUNC920,
	.detect = GB02FUNC865,
	.reset = GB02FUNC931,
	.atomic_duplicate_state = gbdc_connector_atomic_duplicate_state,
	.atomic_destroy_state = GB02FUNC928,
	.atomic_set_property = GB02FUNC922,
	.atomic_get_property = GB02FUNC926,
};
static const struct drm_connector_funcs gbdc_virt_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.fill_modes = GB02FUNC917,
	.destroy = GB02FUNC920,
	.detect = GB02FUNC865,
	.reset = GB02FUNC931,
	.atomic_duplicate_state = gbdc_connector_atomic_duplicate_state,
	.atomic_destroy_state = GB02FUNC928,
	.atomic_set_property = GB02FUNC922,
	.atomic_get_property = GB02FUNC926,
};
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
void gbdc_dm_dp_connector(struct gbdc_connector *gbdc_connector, struct drm_device *drm_dev)
{
	gbdc_connector->aux.drm_dev = drm_dev;
#else
void gbdc_dm_dp_connector(struct gbdc_connector *gbdc_connector)
{
#endif
	gbdc_connector->aux.name = "dmdc";
	gbdc_connector->aux.dev = gbdc_connector->base.kdev;
	gbdc_connector->aux.transfer = GB02FUNC1030;

	drm_dp_aux_register(&gbdc_connector->aux);
}

/**
 * GB02FUNC936() - init connecor dev
 * @dev: drm device info
 * @enc_id: bind possible crtc id
 *
 * Returns the GB02STR156 dev info by zalloc.
 */
struct gbdc_connector *GB02FUNC936(struct drm_device *dev, int conn_id,bool virt)
{
	int ret = 0;
	int connector_type = DRM_MODE_CONNECTOR_DisplayPort;
	struct gbdc_connector *gbdc_connector;
	struct GB02STR155 *gb_dev;
	struct drm_connector *connector;
	struct GB02STR39 *gbdev = (struct GB02STR39 *)dev->dev_private;
	gb_dev = GB02FUNC85(dev->dev_private);

	gbdc_connector = kzalloc(sizeof(*gbdc_connector), GFP_KERNEL);
	if (!gbdc_connector){
		gb_printf(KERN_ERR, "no memory for gbdc connector\n");
		return ERR_PTR(-ENOMEM);
	}

	gbdc_connector->connector_id = conn_id;
	init_waitqueue_head(&gbdc_connector->waitq);
	INIT_LIST_HEAD(&gbdc_connector->modes);
	connector = &gbdc_connector->base;
	mutex_init(&gbdc_connector->gbdc_infinity_mutex);
	mutex_init(&gbdc_connector->gbdc_infinity_atomic_mutex);

	if (GB02FUNC951(connector)) {
		/* MXM VGA */
		ret = GB02FUNC1874(&gbdc_connector->vga);
		if (ret < 0) {
			dev_err(dev->dev, "%s:%d:vga bridge init failed(%d)\n",
					 __func__, __LINE__, ret);
			goto out;
		}

		gbdc_connector->bridge = &(gbdc_connector->vga.bridge);
		gbdc_connector->vga.connector = connector;

		return gbdc_connector;
	}
	connector_type = virt ? DRM_MODE_CONNECTOR_VIRTUAL: DRM_MODE_CONNECTOR_DisplayPort;

	if (GB02FUNC503(gbdev->gb_pcie) == PCIE_LPDDR4 ||
		GB02FUNC503(gbdev->gb_pcie) == PCIE_C0_200) {
		if (2 == conn_id) {
			connector_type = DRM_MODE_CONNECTOR_HDMIA;
		} else if (3 == conn_id) {
			connector_type = DRM_MODE_CONNECTOR_VGA;
		}
	} else if (GB02FUNC503(gbdev->gb_pcie) == PCIE_FULL_LPDDR4) {
		if (1 == conn_id) {
			connector_type = DRM_MODE_CONNECTOR_LVDS;
		} else if (4 == conn_id) {
			connector_type = DRM_MODE_CONNECTOR_VGA;
		} else if (5 == conn_id) {
			connector_type = DRM_MODE_CONNECTOR_HDMIA;
		}
	} else if (GB02FUNC503(gbdev->gb_pcie) == PCIE_M6FL8G_LPDDR4) {
		if ((conn_id == 1) || (conn_id == 3) || (conn_id == 4) || (conn_id == 5))
			connector_type = DRM_MODE_CONNECTOR_HDMIA;
	} else if (GB02FUNC503(gbdev->gb_pcie) == PCIE_HIE1LP4_LPDDR4) {
		if ((conn_id == 0) || (conn_id == 2) || (conn_id == 3) || (conn_id == 5))
			connector_type = DRM_MODE_CONNECTOR_HDMIA;
	} else if (GB02FUNC503(gbdev->gb_pcie) == PCIE_M4HL8G_LPDDR4) {
		if ((conn_id == 0) || (conn_id == 2) || (conn_id == 3) || (conn_id == 5))
			connector_type = DRM_MODE_CONNECTOR_HDMIA;
	} else if (GB02FUNC503(gbdev->gb_pcie) == GB02MAC1363) {
		if (2 == conn_id)
			connector_type = DRM_MODE_CONNECTOR_HDMIA;
		else if (3 == conn_id)
			connector_type = DRM_MODE_CONNECTOR_VGA;
		else if (0 == conn_id || 1 == conn_id)
			connector_type = DRM_MODE_CONNECTOR_eDP;
		else if (4 == conn_id || 5 == conn_id)
			connector_type = DRM_MODE_CONNECTOR_DisplayPort;
	}

	ret = drm_connector_init(dev, connector, virt ? &gbdc_virt_connector_funcs :
			&gbdc_connector_connector_funcs,
			connector_type);
	if(ret){
		dev_err(dev->dev, "drm_connector_init failed\n");
		goto out;
	}

	if (conn_id == CONNECTOR0 && gb_dev->dev_info->gb_hdmi_ddc) {
		/* connector 0 */
		ret = gb_dev->dev_info->gb_hdmi_ddc(gbdc_connector);
		if (ret) {
			dev_err(dev->dev, "no memory for gbdc connector\n");
			goto out;
		}
	}

	drm_connector_helper_add(connector, virt ? &gbdc_virt_connector_helper_funcs :
				&gbdc_connector_connector_helper_funcs);

	connector->polled = DRM_CONNECTOR_POLL_HPD;
	if (connector_type == DRM_MODE_CONNECTOR_VIRTUAL) {
		drm_object_attach_property(&connector->base,
				      gbdev->gbdc_dev->kms_info.infinity_enable_property, 0);
		drm_object_attach_property(&connector->base,
						gbdev->gbdc_dev->kms_info.infinity_info_property, 0);
		drm_object_attach_property(&connector->base,
						gbdev->gbdc_dev->kms_info.infinity_adjust_property, 0);
		drm_object_attach_property(&connector->base,
						gb_dev->kms_info.virtual_flag_property, 1);
		GB02FUNC1406(connector);
	} else {
		drm_object_attach_property(&connector->base,
						gb_dev->kms_info.infinity_set_property, 0);
		drm_object_attach_property(&connector->base,
						gb_dev->kms_info.virtual_flag_property, 0);
	}
	drm_connector_register(connector);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	gbdc_dm_dp_connector(gbdc_connector, dev);
#else
	gbdc_dm_dp_connector(gbdc_connector);
#endif
	return gbdc_connector;

out:
	kfree(gbdc_connector);
	return NULL;
}

int GB02FUNC951(struct drm_connector *connector)
{
	int ret = 0;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
	genbu_board_type_e bd_type = GB02FUNC1184();

	if ((bd_type == GENBU_MXM_BOARD) &&
		(gbdc_conn->connector_id == CONNECTOR1))
		ret = 1;

	return ret;
}
