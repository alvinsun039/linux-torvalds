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
#include <linux/fb.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bug.h>
#include <video/videomode.h>
// #include <drm/drm_modes.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#else
#include <drm/drm_vblank.h>
#endif
#include <drm/drm_modes.h>
//#ifdef CONFIG_GBDC_ATOMIC_FEATURE
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
//#endif
#include <drm/drm_crtc.h>
#include <drm/drm_plane.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#include <drm/drm_fb_helper.h>
#include "drm/drm_fourcc.h"
#include "drm/drm_plane_helper.h"
#include <generated/uapi/linux/version.h>
#include <linux/delay.h>

#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "device/gbdc_regs.h"
#include "device/reg_ops.h"
#include "gbdc_mm.h"
#include "gbdc_planes.h"
#include "gbdc_connector.h"
#include "gbdc_encoder.h"
#include "gbdc_drv.h"
#include "gbdc_crtc.h"
#include "gbdc_display.h"
#include <drm/drm_framebuffer.h>

static const uint32_t safe_modeset_formats[] = {
		DRM_FORMAT_XRGB8888,
		DRM_FORMAT_ARGB8888,
		DRM_FORMAT_NV12,
};

#ifdef CONFIG_GBDC_ATOMIC_FEATURE

extern void __drm_atomic_helper_plane_duplicate_state(struct drm_plane *plane,
                                                      struct drm_plane_state *state);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
extern void __drm_atomic_helper_plane_destroy_state(struct drm_plane_state *state);
#else
extern void __drm_atomic_helper_plane_destroy_state(struct drm_plane *plane,
                                                    struct drm_plane_state *state);
#endif

static int GB02FUNC1843(struct gbdc_plane *gb_plane, int x, int y,
								   struct drm_framebuffer *fb, struct drm_framebuffer *old_fb)
{
	struct GB02STR159 *gbdc_fb;
	struct GB02STR56 *bo;
	u64 plane_vram_addr = 0;
	int ret;

	if (old_fb) {
		gbdc_fb = to_gbdc_framebuffer(old_fb);
#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
		bo = GB02FUNC217(gbdc_fb->obj);
#else
		bo = GB02FUNC217(gbdc_fb->obj);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) || (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS)\
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS)
		ret = ttm_bo_reserve(&bo->ttm_bo.bo, true, false, NULL);
#else // support CONFIG_ARM64 CONFIG_MIPS
		ret = ttm_bo_reserve(&bo->ttm_bo.bo, true, false, false, NULL);
#endif
		if (ret) {
			DRM_ERROR("failed to reserve old_fb bo\n");
		} else {
			GB02FUNC1487(&bo->ttm_bo);
			ttm_bo_unreserve(&bo->ttm_bo.bo);
		}
	} else {
		gb_printf(KERN_INFO, "old_fb is NULL %s: %d\n", __func__, __LINE__);
	}

	if (WARN_ON(fb == NULL)) {
		gb_printf(KERN_ERR, "%s: Error in %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	gbdc_fb = to_gbdc_framebuffer(fb);
#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	bo = GB02FUNC217(gbdc_fb->obj);
#else
	bo = GB02FUNC217(gbdc_fb->obj);
#endif
#if KERNEL_VERSION(5, 0, 0) < LINUX_VERSION_CODE
	gb_printf(KERN_DEBUG, "%s: bo addr: %pK, vm_node.start: %llx\n",
		       __func__, bo, bo->ttm_bo.bo.base.vma_node.vm_node.start);
#else
	gb_printf(KERN_DEBUG, "%s: bo addr: %pK, vm_node.start: %llx\n",
		       __func__, bo, bo->ttm_bo.bo.vma_node.vm_node.start);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) || (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS)\
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS)
	ret = ttm_bo_reserve(&bo->ttm_bo.bo, true, false, NULL);
#else // support CONFIG_ARM64 CONFIG_MIPS
	ret = ttm_bo_reserve(&bo->ttm_bo.bo, true, false, false, NULL);
#endif
	if (ret)
		return ret;

	//pr_info("bo %px ,domain %d\n",bo ,bo->initial_domain);

	ret = GB02FUNC1485(&bo->ttm_bo, bo->initial_domain, &plane_vram_addr);
	if (ret) {
		ttm_bo_unreserve(&bo->ttm_bo.bo);
		return ret;
	}

	ttm_bo_unreserve(&bo->ttm_bo.bo);

	if(gb_plane->base.type == DRM_PLANE_TYPE_CURSOR)
	{
		gb_plane->cur_fb_offset =(unsigned long)plane_vram_addr;
	}
	else
	{
	#if !(defined SYS_CENTOS7_COMPILE_ENV || defined SYS_CENTOS7_9_2009) && \
		LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
		gb_plane->cur_fb_offset = (unsigned long)plane_vram_addr + y * gbdc_fb->base.pitches[0] + x * (gbdc_fb->base.bits_per_pixel / 8);
	#else
		gb_plane->cur_fb_offset = (unsigned long)plane_vram_addr + y * gbdc_fb->base.pitches[0] + x * gbdc_fb->base.format->cpp[0];
	#endif
	}

	return 0;

}

void GB02FUNC1844(struct drm_plane *plane)
{
	struct gbdc_plane *mp = to_gbdc_plane_info(plane);

	gb_printf(KERN_INFO, "%s: plane %px \n", __func__,plane);
	WARN_ON_ONCE(1);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	drm_plane_cleanup(plane);
	kfree(mp);
#else
	if (mp->base.fb)
		drm_framebuffer_unreference(mp->base.fb);

	drm_plane_helper_disable(plane);
	drm_plane_cleanup(plane);
	devm_kfree(plane->dev->dev, mp);
#endif
}
static struct drm_plane_state *GB02FUNC1845(struct drm_plane *plane)
{
	struct gbdc_plane_state *state, *old_state;

	gb_printf(KERN_DEBUG, "******%s \n", __func__);

	if (!plane->state)
		return NULL;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return NULL;

	old_state = to_gbdc_plane_state(plane->state);

	__drm_atomic_helper_plane_duplicate_state(plane, &state->base);

	state->rotmem_size = old_state->rotmem_size;
	state->format = old_state->format;
	state->n_planes = old_state->n_planes;

	state->infinity_plane_list = old_state->infinity_plane_list;
	state->order = old_state->order;
    state->geometry = old_state->geometry;
    state->padding = old_state->padding;

	return &state->base;
}

static void GB02FUNC1846(struct drm_printer *p,const struct drm_plane_state *state)
{
	struct gbdc_plane_state *ms = to_gbdc_plane_state(state);

	drm_printf(p, "\trotmem_size=%u\n", ms->rotmem_size);
	drm_printf(p, "\tformat_id=%u\n", ms->format);
	drm_printf(p, "\tn_planes=%u\n", ms->n_planes);
}

static __attribute((unused)) void gbdc_destroy_plane_state(struct drm_plane *plane,struct drm_plane_state *state)
{
	struct gbdc_plane_state *m_state = to_gbdc_plane_state(state);

	__drm_atomic_helper_plane_destroy_state(&m_state->base);

	if(m_state)
		kfree(m_state);
}


static void GB02FUNC1847(struct drm_plane *plane)
{
	struct gbdc_plane_state *plane_state;

	if (plane->state){ 
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
        	__drm_atomic_helper_plane_destroy_state(plane->state);
	#else
        	__drm_atomic_helper_plane_destroy_state(plane, plane->state);
	#endif
	}
	
	kfree(plane->state);

	plane_state = kzalloc(sizeof(*plane_state), GFP_KERNEL);

	if (plane_state) {
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
		__drm_atomic_helper_plane_reset(plane, &plane_state->base);
	#else
		plane_state->base.plane = plane;
		plane_state->base.rotation = DRM_MODE_ROTATE_0;
		plane->state = &plane_state->base;
	#endif
		plane_state->infinity_plane_list = to_gbdc_plane_info(plane);
		INIT_LIST_HEAD(&plane_state->infinity_plane_list->head);
	}
}


static const struct drm_plane_funcs gbdc_de_plane_funcs = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
		.update_plane = drm_atomic_helper_update_plane,
		.disable_plane = drm_atomic_helper_disable_plane,
#endif
		.destroy = GB02FUNC1844,
		.reset = GB02FUNC1847,
		.atomic_duplicate_state = GB02FUNC1845,
		.atomic_destroy_state = gbdc_destroy_plane_state,
		.atomic_print_state = GB02FUNC1846,
};

static __attribute((unused)) void gbdc_atomic_plane_print_state(struct drm_plane_state *state)
{
#ifdef CONFIG_GBDC_DEBUG
	struct drm_plane *plane = state->plane;
	struct drm_rect src  = drm_plane_state_src(state);
	struct drm_rect dest = drm_plane_state_dest(state);
	int i = 0;

	gb_printf(KERN_ERR, "plane[%u]: %s\n", plane->base.id, plane->name);
	gb_printf(KERN_ERR, "\tcrtc=%s\n", state->crtc ? state->crtc->name : "(null)");
	gb_printf(KERN_ERR, "\tfb=%u\n", state->fb ? state->fb->base.id : 0);
	if (state->fb) {
		gb_printf(KERN_ERR, "allocated by = %s\n", state->fb->comm);
		gb_printf(KERN_ERR, "refcount=%u\n",
				drm_framebuffer_read_refcount(state->fb));
		gb_printf(KERN_ERR, "modifier=0x%llx\n", state->fb->modifier);
		gb_printf(KERN_ERR, "size=%ux%u\n", state->fb->width, state->fb->height);
		gb_printf(KERN_ERR, "layers:\n");

		for (i = 0; i < state->fb->format->num_planes; i++) {
			gb_printf(KERN_ERR, "size[%u]=%dx%d\n", i,
					drm_framebuffer_plane_width(state->fb->width, state->fb, i),
					drm_framebuffer_plane_height(state->fb->height, state->fb, i));
			gb_printf(KERN_ERR, "pitch[%u]=%u\n", i, state->fb->pitches[i]);
			gb_printf(KERN_ERR, "offset[%u]=%u\n", i, state->fb->offsets[i]);
			gb_printf(KERN_ERR, "obj[%u]:%s\n", i,
					  state->fb->obj[i] ? "" : "(null)");
		}
	}
	gb_printf(KERN_ERR, "\tcrtc-pos=" DRM_RECT_FMT "\n", DRM_RECT_ARG(&dest));
	gb_printf(KERN_ERR, "\tsrc-pos=" DRM_RECT_FP_FMT "\n", DRM_RECT_FP_ARG(&src));
	gb_printf(KERN_ERR, "\trotation=%x\n", state->rotation);
	gb_printf(KERN_ERR, "\tnormalized-zpos=%x\n", state->normalized_zpos);
#endif
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static int gbdc_de_plane_check(struct drm_plane *plane, struct drm_atomic_state *atomic_state)
{
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atomic_state,
									plane);
#else
static int gbdc_de_plane_check(struct drm_plane *plane,struct drm_plane_state *state)
{
#endif
	//struct gbdc_plane_state *gbstate = to_gbdc_plane_state(plane->state);
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_framebuffer *fb = state->fb;
	int ret;
	int min_scale =0 ,max_scale =INT_MAX;

	if (!crtc || !fb)
	{
		if(!plane->state)
			gb_printf(KERN_INFO, "%s: crtc %px, fb %px\n",__func__, crtc, fb);
		if(plane && plane->state && plane->state->crtc)
			gb_printf(KERN_INFO, "%s: crtc %px, fb %px,plane->crtc %px,plane->state->crtc %px\n",
				__func__,crtc, fb, plane->crtc,plane->state->crtc);
		return 0;
	}
	crtc_state = drm_atomic_get_existing_crtc_state(state->state, crtc);
	if (WARN_ON(!crtc_state))
		return -EINVAL;

	ret = drm_atomic_helper_check_plane_state(state, crtc_state,
						  min_scale, max_scale,
						  true, true);
	if (ret)
	{
		pr_err("%s: ret %d,state->visible %d\n",__func__,ret,state->visible);
		return ret;
	}
	if (!state->visible)
		return 0;
	//if(gbstate)
		//gbdc_atomic_plane_print_state(&gbstate->base);

	return 0;
}
static int GB02FUNC1848(struct gbdc_plane *gb_plane,struct gbdc_crtc *gbdc_crtc,struct drm_plane_state *state,struct drm_framebuffer *fb)
{
	struct drm_crtc *crtc = &gbdc_crtc->base;//GB02FUNC1573(crtc);
	struct drm_gem_object *obj;
	//struct GB02STR56 *gobj;
	int ret;
	struct GB02STR77 *cplane;
	void __iomem *de_base;
	//struct GB02STR249 *dc_config;
	//int crtc_id = gbdc_crtc->crtc_id;
	struct GB02STR159 *gbdc_fb = NULL ;
	struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	//struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;

	if(!crtc){
		DRM_ERROR("%s:crtc null---\n", __func__);
		return -EINVAL;
	}
	//struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	cplane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
	//dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	de_base = get_de_base(gb_dev ,gbdc_crtc->crtc_id);//dc_config->de_base;

	if(fb)
	 	gbdc_fb = to_gbdc_framebuffer(fb);
	gb_printf(KERN_INFO, "%s:crtc %px, crtc id %d,gbdc_fb %px ,fb %px\n",__func__, crtc,
					gbdc_crtc->crtc_id,gbdc_fb, fb);

	//width = width >> 16;
	//height = height >>16 ;

	if ((state->crtc_w > gbdc_crtc->max_cursor_width) ||
	    (state->crtc_h > gbdc_crtc->max_cursor_height)) {
		DRM_ERROR("bad cursor width or height %d x %d\n", state->crtc_w, state->crtc_h);
		return -EINVAL;
	}
	//pr_info("width %d ,height %d, hotx %d,hoty %d \n",state->crtc_w ,state->crtc_h,fb->hot_x, fb->hot_y);

	gbdc_crtc->cursor_addr = gb_plane->cur_fb_offset;

	//pr_info("get cursor_addr 0x%llx \n",gbdc_crtc->cursor_addr);
	gbdc_crtc->static_cursor_addr = gbdc_crtc->cursor_addr;

	gbdc_crtc->cursor_width = state->crtc_w;
	gbdc_crtc->cursor_height = state->crtc_h;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)	
	gbdc_crtc->cursor_hot_x = fb->hot_x;
	gbdc_crtc->cursor_hot_y = fb->hot_y;
#endif	

	gbdc_crtc->cursor_x = state->crtc_x;
	gbdc_crtc->cursor_y = state->crtc_y;

	//kms_ops->config_base_atomic(crtc, cplane, state, de_base);//gbdc_ops

	//pr_info("crtc->cursor_x %d,crtc->cursor_y %d \n",crtc->cursor_x, crtc->cursor_y);
	obj = gbdc_fb->obj;
	ret = GB02FUNC1032(crtc,gbdc_crtc->cursor_x , gbdc_crtc->cursor_y);
	if (ret) {
		pr_err("cursor move err,so hide \n");
		GB02FUNC1026(crtc);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(obj);
#else
		drm_gem_object_put_unlocked(obj);
#endif
		return ret;
	}
	//gbdc_crtc->drm_file_handle = handle;
	//gbdc_crtc->drmfile = file_priv;

	/*if(!gbdc_crtc->is_hide)
		GB02FUNC1054(crtc);*/
//unpin:
	/*if (gbdc_crtc->cursor_bo) {
		struct GB02STR56 *gobj = GB02FUNC217(gbdc_crtc->cursor_bo);
		ret = ttm_bo_reserve(&gobj->ttm_bo.bo, false, false, NULL);

		if (likely(ret == 0)) {
			GB02FUNC1487(&gobj->ttm_bo);
			ttm_bo_unreserve(&gobj->ttm_bo.bo);
		}
		drm_gem_object_put_unlocked(gbdc_crtc->cursor_bo);
	}*/

	gbdc_crtc->cursor_bo = obj;
	return 0;

}

int GB02FUNC1849(int row, int col, struct drm_plane_state *state)
{

	struct gbdc_plane *gbdc_plane = NULL;
	struct gbdc_plane_state *gbdc_plane_state = to_gbdc_plane_state(state);
	struct GB02STR239 *gb_border = gbdc_plane_state->gb_border;
	int i, j, m, n, x, y;
	int pos;

	memset(gb_border, 0, sizeof(struct GB02STR239) * GB02MAC2558);

	gbdc_for_each_gbdc_obj(plane) {
		if (!gbdc_plane)
			continue;
		x = gbdc_plane->order.x;
		y = gbdc_plane->order.y;
		if (x >= row || y >= col)
			continue;

		pos = x * col + y;
		if (pos >= GB02MAC2558) {
			gb_printf(KERN_ERR, "the number of screen is err(%d)", pos);
			return -1;
		}
		gb_border[pos].top = gbdc_plane->padding.top;
		gb_border[pos].right = gbdc_plane->padding.right;
		gb_border[pos].left = gbdc_plane->padding.left;
		gb_border[pos].bottom = gbdc_plane->padding.bottom;
		gb_border[pos].width = gbdc_plane->geometry.w;
		gb_border[pos].high = gbdc_plane->geometry.h;
		gb_printf(KERN_DEBUG, "%s x = %d, y = %d, top = %d, left=%d, right = %d, bottom = %d\n",
			__func__, x, y, gb_border[pos].top, gb_border[pos].left,
			gb_border[pos].right, gb_border[pos].bottom);
	}

	for (i = 0; i < row; i++ ) {
		for (j = 0; j < col; j++) {
			pos = i * col + j;
			for (m = 0; m < j; m++)
				gb_border[pos].w_total +=
					gb_border[i * col + m].left +
					gb_border[i * col + m].width +
					gb_border[i * col + m].right;

			gb_border[pos].w_total += gb_border[pos].left;

			for (n = 0; n < i; n++)
				gb_border[pos].h_total +=
					gb_border[n * col + j].top +
					gb_border[n * col + j].high +
					gb_border[n * col + j].bottom;

			gb_border[pos].h_total += gb_border[pos].top;
			gb_printf(KERN_DEBUG, "%s i = %d, j = %d w_total = %d h_total = %d\n",
				__func__, i, j, gb_border[pos].w_total, gb_border[pos].h_total);
		}
	}

	return 0;
}


int GB02FUNC1850(int row, int col, struct gbdc_crtc_state *gbdc_crtc_state,
				struct GB02STR239 *gb_border)
{

	struct gbdc_crtc *gbdc_crtc = NULL;
	int i,j,m,n,x,y;
	int pos;

	memset(gb_border, 0, sizeof(struct GB02STR239) * GB02MAC2558);

	gbdc_for_each_gbdc_obj(crtc) {
		if (!gbdc_crtc)
			continue;
		x = gbdc_crtc->order.x;
		y = gbdc_crtc->order.y;
		if (x >= row || y >= col)
			continue;
		pos = x * col + y;
		if (pos >= GB02MAC2558) {
			gb_printf(KERN_ERR, "the number of screen is err(%d)", pos);
			return -1;
		}
		gb_border[pos].top = gbdc_crtc->padding.top;
		gb_border[pos].right = gbdc_crtc->padding.right;
		gb_border[pos].left = gbdc_crtc->padding.left;
		gb_border[pos].bottom = gbdc_crtc->padding.bottom;
		gb_border[pos].width = gbdc_crtc->geometry.w;
		gb_border[pos].high = gbdc_crtc->geometry.h;
		gb_printf(KERN_DEBUG, "%s x = %d, y = %d, top = %d, left=%d, right = %d, bottom = %d\n",
			__func__, x, y, gb_border[pos].top, gb_border[pos].left,
			gb_border[pos].right, gb_border[pos].bottom);
	}

	for (i = 0; i < row; i++ ) {
		for (j = 0; j < col; j++) {
			pos = i * col + j;
			for (m = 0; m < j; m++)
				gb_border[pos].w_total +=
					gb_border[i * col + m].left +
					gb_border[i * col + m].width +
					gb_border[i * col + m].right;

			gb_border[pos].w_total += gb_border[pos].left;

			for (n = 0; n < i; n++)
				gb_border[pos].h_total +=
					gb_border[n * col + j].top +
					gb_border[n * col + j].high +
					gb_border[n * col + j].bottom;

			gb_border[pos].h_total += gb_border[pos].top;

			gb_border[pos].x1 = gb_border[pos].w_total;
			gb_border[pos].y1 = gb_border[pos].h_total;
			gb_border[pos].x2 = gb_border[pos].w_total + gb_border[pos].width;
			gb_border[pos].y2 = gb_border[pos].h_total + gb_border[pos].high;

			gb_printf(KERN_DEBUG, "%s i = %d, j = %d w_total = %d h_total = %d\n",
				__func__, i, j, gb_border[pos].w_total, gb_border[pos].h_total);
			gb_printf(KERN_DEBUG, "%s (x1, y1)= (%d, %d) (x2, y2) = (%d, %d)\n",
				__func__, gb_border[pos].x1, gb_border[pos].y1,
				gb_border[pos].x2, gb_border[pos].y2);
		}
	}

	return 0;
}

static int GB02FUNC1851(struct gbdc_plane *gb_plane, struct gbdc_crtc *gbdc_crtc,struct drm_plane_state *state,struct drm_framebuffer *fb)
{
	struct drm_crtc *crtc = &gbdc_crtc->base;
	struct drm_gem_object *obj;
	int ret = 0;
	struct GB02STR77 *cplane;
	struct GB02STR159 *gbdc_fb = NULL ;
	struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	struct gbdc_crtc_state *gbdc_crtc_state = NULL;
	int row, col;
	struct GB02STR240 gb_screen_xy[CURSOE_SHOW_MAX] = {0};
	int screen_x, screen_y;
	int screen_tmp_x, screen_tmp_y;
	struct gbdc_plane_state *gbdc_plane_state = to_gbdc_plane_state(state);

	int x1, y1, x2, y2;
	int x, y;
	int x_e, y_e;
	int i, j;
	int pos;

	if(!crtc){
		DRM_ERROR("%s:crtc null---\n", __func__);
		return -EINVAL;
	}
	cplane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];

	if(fb)
	 	gbdc_fb = to_gbdc_framebuffer(fb);
	gb_printf(KERN_DEBUG, "%s:crtc %px, crtc id %d,gbdc_fb %px ,fb %px\n",__func__, crtc,
					gbdc_crtc->crtc_id,gbdc_fb, fb);


	if ((state->crtc_w > gbdc_crtc->max_cursor_width) ||
	    (state->crtc_h > gbdc_crtc->max_cursor_height)) {
		DRM_ERROR("bad cursor width or height %d x %d\n", state->crtc_w, state->crtc_h);
		return -EINVAL;
	}

	gb_printf(KERN_INFO, "state->crtc_x = %d, state->crtc_y = %d \n",state->crtc_x, state->crtc_y);
	obj = gbdc_fb->obj;


	row = gbdc_crtc->infinity_row;
	col = gbdc_crtc->infinity_col;
	if (row == 0 || col == 0) {
		gb_printf(KERN_ERR, "%s erro value row = %d cow = %d\n", __func__, row, col);
		return -1;
	}

	gbdc_crtc_state = to_gbdc_crtc_state(crtc->state);
	if (gbdc_crtc_state == NULL) {
		gb_printf(KERN_ERR, "%s gbdc_ctrc_state == NULL\n", __func__);
		return -1;
	}

	ret = GB02FUNC1850(row, col, gbdc_crtc_state, gbdc_plane_state->gb_border);
	if (ret) {
		gb_printf(KERN_ERR, "%s GB02FUNC1850 err\n", __func__);
		return -1;
	}

	for (i = 0; i < row; i++) {
		for(j = 0; j < col; j++) {
			pos = i * col + j;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)			
			gbdc_plane_state->gb_border[pos].x1 -= fb->hot_x;
			gbdc_plane_state->gb_border[pos].y1 -= fb->hot_y;
			gbdc_plane_state->gb_border[pos].x2 -= (fb->hot_x + 1);
			gbdc_plane_state->gb_border[pos].y2 -= (fb->hot_y + 1);
#endif
			x1 = gbdc_plane_state->gb_border[pos].x1;
			y1 = gbdc_plane_state->gb_border[pos].y1;
			x2 = gbdc_plane_state->gb_border[pos].x2;
			y2 = gbdc_plane_state->gb_border[pos].y2;

			if (state->crtc_x >= x1 && state->crtc_x <= x2 &&
				state->crtc_y  >= y1 && state->crtc_y <= y2) {
				gb_screen_xy[CURSOE_SHOW_THIS].x = i;
				gb_screen_xy[CURSOE_SHOW_THIS].y = j;
				gb_screen_xy[CURSOE_SHOW_THIS].show = true;
			}

			x_e = state->crtc_x + state->crtc_w;
			y_e = state->crtc_y;
			if (x_e >= x1 && x_e <= x2 && y_e >= y1 && y_e <= y2) {
				gb_screen_xy[CURSOE_SHOW_RIGHT].x = i;
				gb_screen_xy[CURSOE_SHOW_RIGHT].y = j;
				gb_screen_xy[CURSOE_SHOW_RIGHT].show = true;
			}

			x_e = state->crtc_x;
			y_e = state->crtc_y + state->crtc_w;
			if (x_e >= x1 && x_e <= x2 && y_e >= y1 && y_e <= y2) {
				gb_screen_xy[CURSOE_SHOW_BOTTOM].x = i;
				gb_screen_xy[CURSOE_SHOW_BOTTOM].y = j;
				gb_screen_xy[CURSOE_SHOW_BOTTOM].show = true;
			}

			x_e = state->crtc_x + state->crtc_w;
			y_e = state->crtc_y + state->crtc_w;
			if (x_e >= x1 && x_e <= x2 && y_e >= y1 && y_e <= y2) {
				gb_screen_xy[CURSOE_SHOW_BOTTOM_RIGHT].x = i;
				gb_screen_xy[CURSOE_SHOW_BOTTOM_RIGHT].y = j;
				gb_screen_xy[CURSOE_SHOW_BOTTOM_RIGHT].show = true;
			}
		}
	}

	gbdc_for_each_gbdc_obj(crtc) {
		gb_printf(KERN_DEBUG, "%s enter virt cursor move\n", __func__);
		if (!gbdc_crtc)
			continue;

		screen_x = gbdc_crtc->order.x;
		screen_y = gbdc_crtc->order.y;
		crtc = &gbdc_crtc->base;
		gbdc_crtc->cursor_width = state->crtc_w;
		gbdc_crtc->cursor_height = state->crtc_h;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)		
		gbdc_crtc->cursor_hot_x = fb->hot_x;
		gbdc_crtc->cursor_hot_y = fb->hot_y;
#endif
		gbdc_crtc->cursor_addr = gb_plane->cur_fb_offset;
		gbdc_crtc->static_cursor_addr = gbdc_crtc->cursor_addr;

		for (i = 0; i < CURSOE_SHOW_MAX; i++) {
			screen_tmp_x = gb_screen_xy[i].x;
			screen_tmp_y = gb_screen_xy[i].y;
			if (gb_screen_xy[i].show && screen_tmp_x == screen_x &&	screen_tmp_y == screen_y) {
				pos = screen_x * col + screen_y;
				x = state->crtc_x - gbdc_plane_state->gb_border[pos].w_total;
				y = state->crtc_y - gbdc_plane_state->gb_border[pos].h_total;
				gb_printf(KERN_DEBUG, "x = %d y=%d\n", x, y);
				ret = GB02FUNC1045(crtc, x, y);
				break;
			}
		}
		if (i == CURSOE_SHOW_MAX) {
			GB02FUNC1026(crtc);
		}
	}

	if (ret) {
		pr_err("cursor move err,so hide \n");
		GB02FUNC1026(crtc);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		drm_gem_object_put(obj);
#else
		drm_gem_object_put_unlocked(obj);
#endif
		return ret;
	}
	gbdc_crtc->cursor_bo = obj;

	return 0;

}

void GB02FUNC1852(struct drm_plane_state *state)
{
	if(state){
		pr_info("state->crtc_x %d\n",state->crtc_x);
		pr_info("state->crtc_y %d\n", state->crtc_y);
		pr_info("state->crtc_h %d\n", state->crtc_h);
		pr_info("state->crtc_w %d\n", state->crtc_w);
		pr_info("state->src_x %d\n", state->crtc_x);
		pr_info("state->src_y %d\n", state->src_y);
		pr_info("state->src_h %d\n", state->src_h);
		pr_info("state->src_w %d\n", state->src_w);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)		
		pr_info("state->fb->hot_x %d\n",state->fb->hot_x);
		pr_info("state->fb->hot_y %d\n",state->fb->hot_y);
#endif
		if(state->crtc)
			pr_info("state->crtc %px,crtc idx %d\n",state->crtc,state->crtc->index);
		if(state->fb)
			pr_info("state->fb %px,fb id %d\n",state->fb,state->fb->base.id);
		//dump_caller;
	}
}
void GB02FUNC1853(struct drm_plane_state *old_state)
{
	if(!old_state)
		return;
	if(!old_state->fb || !old_state->crtc)
		return;
	if(old_state && old_state->crtc)
		GB02FUNC1852(old_state);
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_plane_update(struct drm_plane *plane,
					 struct drm_atomic_state *atomic_state)
{
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atomic_state,
									plane);
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(atomic_state,
									plane);
#else
static void gbdc_plane_update(struct drm_plane *plane,
					 struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
#endif
	struct GB02STR155 *gb_dev;
	struct GB02STR245	*kms_info;
	struct drm_crtc *crtc;
	struct gbdc_crtc *gb_crtc;
	//struct videomode vmode ={0};
	//int crtc_id;
	struct drm_framebuffer *fb;
	struct drm_framebuffer *old_fb = NULL;
	//u32 src_x, src_y, src_w, src_h;
	void __iomem *de_base, *dc_base;
	struct GB02STR77 *plane_res = NULL;
	struct GB02STR249 *dc_config;
	struct gbdc_plane *gb_plane = NULL;
	struct gbdc_plane *new_plane = NULL;
	int plane_id;

	if (!plane || !plane->state || !plane->state->fb || !plane->state->crtc) {
		gb_printf(KERN_ERR, "%s plane err\n",__func__);
		return;
	}
	new_plane = container_of(plane, struct gbdc_plane, base);
	plane_id = new_plane->plane_id;

	if (!state->visible) {
		//gbdc_plane_disable(plane, old_state);
		gb_printf(KERN_ERR, "%s: state->visible == 0\n", __func__);
		return;
	}

	crtc = plane->state->crtc;
	gb_crtc = GB02FUNC1573(plane->state->crtc);

	if (gb_crtc->need_disable)
		return;

	gb_dev = GB02FUNC85(crtc->dev->dev_private);
	kms_info = &gb_dev->kms_info;
	//crtc_id = gb_crtc->crtc_id;
	dc_config = &gb_dev->pcie_info.dc_config[gb_crtc->crtc_id];
	de_base = dc_config->de_base;
	dc_base = dc_config->dc_base;

	gb_printf(KERN_DEBUG, "%s :crtc addr: %pK,idx %d\n", __func__, crtc,crtc->index);
	gb_printf(KERN_DEBUG, "%s :drm_crtc->base.id: %d,plane type %d\n", __func__ ,crtc->base.id, plane->type);

	if (WARN_ON(!gb_crtc->is_enable))
		return;

	//kms_info->kms_ops->enter_config(gb_dev->drm_dev, crtc_id);

	fb  = plane->state->fb;
	old_fb = old_state->fb;
	//crtc->primary->fb = fb;

	switch (plane->type) {
	case DRM_PLANE_TYPE_PRIMARY:
		#if 0
		if ((old_state->fb == state->fb) &&
			(old_state->crtc_x == state->crtc_x) &&
			(old_state->crtc_y == state->crtc_y) &&
			(old_state->crtc_w == state->crtc_w) &&
			(old_state->crtc_h == state->crtc_h) &&
			(old_state->src_x == state->src_x) &&
			(old_state->src_y == state->src_y) &&
			(old_state->src_w == state->src_w) &&
			(old_state->src_h == state->src_h)) {
			/* No change since last update, do not post cmd */
			DRM_DEBUG_DRIVER("No change, not posting cmd\n");
			return;
		}
		#endif
		gb_printf(KERN_DEBUG, "%s : primary type ,crtc->index %d\n",__func__,crtc->index);
		plane_res = &kms_info->plane_res[DC_PLANE_GRAPHIC];
		gb_plane = &gb_crtc->cplanes[DC_PLANE_GRAPHIC];
		if (0 == GB02FUNC1843(gb_plane, state->src_x >> 16, state->src_y >> 16, fb, old_fb)) {
			kms_info->kms_ops->config_base_atomic(crtc, plane_res, state, de_base);
			//GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
		} else
			printk("GB02FUNC1843 fail\n");
		break;
	case DRM_PLANE_TYPE_CURSOR:
		gb_printf(KERN_INFO, "%s:cursor type \n", __func__);
		//GB02FUNC1852(state);
		if(old_state && old_state->crtc)
		{
			gb_printf(KERN_INFO, "%s : old_state crtc %px\n",__func__,old_state->crtc);
			//GB02FUNC1853(old_state);
		}
		//plane_res = &kms_info->plane_res[DC_PLANE_SMART];
		gb_plane = &gb_crtc->cplanes[DC_PLANE_SMART];
		GB02FUNC1843(gb_plane, 0, 0, fb, old_fb);
		//GB02FUNC1848(gb_plane,gb_crtc,state->crtc_h,state->crtc_w,fb->hot_x, fb->hot_y,fb);
		GB02FUNC1848(gb_plane,gb_crtc,state,fb);
		gb_printf(KERN_INFO, "%s : cursor type done\n",__func__);
		break;
	case DRM_PLANE_TYPE_OVERLAY:
		gb_printf(KERN_INFO, "%s : overlay type \n",__func__);
		plane_res = &kms_info->plane_res[plane_id];
		gb_plane = &gb_crtc->cplanes[plane_id];
		if (GB02FUNC1843(gb_plane, 0, 0, fb, old_fb) == 0) {
			kms_info->kms_ops->config_overly_plane(crtc, plane, plane_res,
					fb, de_base, plane_id);
		} else {
			gb_printf(KERN_ERR, "DRM_PLANE_TYPE_OVERLAY fail\n");
		}
		break;
	}

	gb_printf(KERN_DEBUG, "plane update OK %s.........\n", __func__);
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_virtual_plane_update(struct drm_plane *plane,
					 struct drm_atomic_state *atomic_state)
{
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atomic_state,
									plane);
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(atomic_state,
									plane);
#else
static void gbdc_virtual_plane_update(struct drm_plane *plane,
					 struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
#endif

#if 0
	struct gbdc_plane *gbdc_plane = NULL;
	struct gbdc_plane_state *gbdc_plane_state = to_gbdc_plane_state(state);

	gbdc_for_each_gbdc_obj(plane) {
		if (!gbdc_plane)
			continue;

		gbdc_plane->order;
		gbdc_plane->geometry;
		gbdc_plane->padding;
	}
	#endif
	struct GB02STR155 *gb_dev;
	struct GB02STR245	*kms_info;
	struct drm_crtc *crtc;
	struct gbdc_crtc *gb_crtc;
	struct drm_framebuffer *fb;
	struct drm_framebuffer *old_fb = NULL;
	//void __iomem *de_base, *dc_base;
	struct GB02STR77 *plane_res = NULL;
	//struct GB02STR249 *dc_config;
	struct gbdc_plane *gb_plane = NULL;
	struct gbdc_plane *new_plane = NULL;
	int plane_id;

	if (!plane || !plane->state || !plane->state->fb || !plane->state->crtc) {
		gb_printf(KERN_ERR, "%s plane err\n",__func__);
		return;
	}
	new_plane = container_of(plane, struct gbdc_plane, base);
	plane_id = new_plane->plane_id;

	if (!state->visible) {
		gb_printf(KERN_ERR, "%s: state->visible == 0\n", __func__);
		return;
	}

	crtc = plane->state->crtc;
	gb_crtc = GB02FUNC1573(plane->state->crtc);

	if (gb_crtc->need_disable)
		return;

	gb_printf(KERN_DEBUG, "%s: virt %d, gbcrtc id %d\n",__func__, gb_crtc->virt,gb_crtc->crtc_id);
	gb_dev = GB02FUNC85(crtc->dev->dev_private);
	kms_info = &gb_dev->kms_info;
#if 0
	dc_config = &gb_dev->pcie_info.dc_config[gb_crtc->crtc_id];
	de_base = dc_config->de_base;
	dc_base = dc_config->dc_base;
#endif
	gb_printf(KERN_DEBUG, "%s :crtc addr: %pK,idx %d\n", __func__, crtc,crtc->index);
	gb_printf(KERN_DEBUG, "%s :drm_crtc->base.id: %d,plane type %d\n", __func__ ,crtc->base.id, plane->type);

	if (WARN_ON(!gb_crtc->is_enable))
		return;


	fb  = plane->state->fb;
	old_fb = old_state->fb;

	switch (plane->type) {
	case DRM_PLANE_TYPE_PRIMARY:
		#if 0
		if ((old_state->fb == state->fb) &&
			(old_state->crtc_x == state->crtc_x) &&
			(old_state->crtc_y == state->crtc_y) &&
			(old_state->crtc_w == state->crtc_w) &&
			(old_state->crtc_h == state->crtc_h) &&
			(old_state->src_x == state->src_x) &&
			(old_state->src_y == state->src_y) &&
			(old_state->src_w == state->src_w) &&
			(old_state->src_h == state->src_h)) {
			/* No change since last update, do not post cmd */
			DRM_DEBUG_DRIVER("No change, not posting cmd\n");
			return;
		}
		#endif
		gb_printf(KERN_DEBUG, "%s : primary type ,crtc->index %d\n",__func__,crtc->index);
		plane_res = &kms_info->plane_res[DC_PLANE_GRAPHIC];
		gb_plane = &gb_crtc->cplanes[DC_PLANE_GRAPHIC];
		if (0 == GB02FUNC1843(gb_plane, state->src_x >> 16, state->src_y >> 16, fb, old_fb)) {
			kms_info->kms_ops->config_virt_base_atomic(crtc, plane_res, state);
		} else
			printk("GB02FUNC1843 fail\n");
		break;
	case DRM_PLANE_TYPE_CURSOR:
		gb_printf(KERN_DEBUG, "%s:cursor type \n", __func__);
		if(old_state && old_state->crtc)
		{
			gb_printf(KERN_DEBUG, "%s : old_state crtc %px\n",__func__,old_state->crtc);
		}
		gb_plane = &gb_crtc->cplanes[DC_PLANE_SMART];
		GB02FUNC1843(gb_plane, 0, 0, fb, old_fb);
		GB02FUNC1851(gb_plane,gb_crtc,state,fb);
		gb_printf(KERN_DEBUG, "%s : cursor type done\n",__func__);
		break;
	case DRM_PLANE_TYPE_OVERLAY:
		gb_printf(KERN_DEBUG, "%s : overlay type \n",__func__);
#if 0
		plane_res = &kms_info->plane_res[plane_id];
		gb_plane = &gb_crtc->cplanes[plane_id];
		if (GB02FUNC1843(gb_plane, 0, 0, fb, old_fb) == 0) {
			kms_info->kms_ops->config_overly_plane(crtc, plane, plane_res,
					fb, de_base, plane_id);
		} else {
			gb_printf(KERN_ERR, "DRM_PLANE_TYPE_OVERLAY fail\n");
		}
#endif
		break;

	}
	gb_printf(KERN_DEBUG, "plane update OK %s.........\n", __func__);

}
void __iomem * get_dc_base(struct GB02STR155 *gb_dev, unsigned int crtc_id)
{
	void __iomem  *dc_base;
	struct GB02STR249 *dc_config;

	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];

	dc_base = dc_config->dc_base;
	return dc_base;
}
void __iomem * get_de_base(struct GB02STR155 *gb_dev ,unsigned int crtc_id)
{
	void __iomem *de_base =NULL;
	struct GB02STR249 *dc_config;

	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];

	de_base = dc_config->de_base;
	return de_base;
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_plane_disable(struct drm_plane *plane, struct drm_atomic_state *state)
{
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(state,
									plane);
#else
static void gbdc_plane_disable(struct drm_plane *plane, struct drm_plane_state *old_state)
{
#endif
	void __iomem *de_base, *dc_base;
	struct GB02STR77 *plane_res = NULL;
	struct GB02STR249 *dc_config;
	struct drm_crtc *crtc = old_state->crtc;
	struct gbdc_crtc *gb_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR245 *kms_info = &gb_dev->kms_info;
	volatile u32 value = 0;
	struct gbdc_plane *gb_plane = to_gbdc_plane_info(plane);

	dc_config = &gb_dev->pcie_info.dc_config[gb_crtc->crtc_id];
	de_base = dc_config->de_base;
	dc_base = dc_config->dc_base;

	if (!gb_plane || (gb_plane->plane_id >= kms_info->num_plane))
		return;

	plane_res = &kms_info->plane_res[gb_plane->plane_id];

	if (plane_res) {
		value = GB02FUNC730(de_base, plane_res->base + GB02MAC696);
		value &= ~GB02MAC698;
		GB02FUNC733(de_base, plane_res->base + GB02MAC696, value);
		GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	}

	return;
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_virual_plane_disable(struct drm_plane *plane, struct drm_atomic_state *state)
{
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(state,
									plane);
#else
static void gbdc_virual_plane_disable(struct drm_plane *plane, struct drm_plane_state *old_state)
{
#endif
	void __iomem *de_base, *dc_base;
	struct GB02STR77 *plane_res = NULL;
	struct GB02STR249 *dc_config;
	struct drm_crtc *crtc = old_state->crtc;
	//struct gbdc_crtc *gb_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR245 *kms_info = &gb_dev->kms_info;
	volatile u32 value = 0;
	//struct gbdc_plane *gb_plane = to_gbdc_plane_info(plane);

	struct gbdc_plane *gbdc_plane = NULL;
	struct drm_plane_state *pl_state = plane->state;
	struct gbdc_plane_state *gbdc_plane_state = to_gbdc_plane_state(pl_state);

	gbdc_for_each_gbdc_obj(plane) {
		if (!gbdc_plane)
			continue;

		dc_config = &gb_dev->pcie_info.dc_config[gbdc_plane->crtc_id];
		de_base = dc_config->de_base;
		dc_base = dc_config->dc_base;

		if (!gbdc_plane || (gbdc_plane->plane_id >= kms_info->num_plane))
			return;

		plane_res = &kms_info->plane_res[gbdc_plane->plane_id];

		if (plane_res) {
			value = GB02FUNC730(de_base, plane_res->base + GB02MAC696);
			value &= ~GB02MAC698;
			GB02FUNC733(de_base, plane_res->base + GB02MAC696, value);
			GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
		}
	}
	return;
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static int gbdc_plane_atomic_async_check(struct drm_plane *plane,
					struct drm_atomic_state *atomic_state)
{
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atomic_state,
									plane);
#else
static int gbdc_plane_atomic_async_check(struct drm_plane *plane,
					struct drm_plane_state *state)
{
#endif
	int min_scale =0 ,max_scale =INT_MAX;
	struct drm_crtc_state *crtc_state;

	if (plane != state->crtc->cursor)
		return -EINVAL;

	if (!plane->state)
		return -EINVAL;

	if (!plane->state->fb)
		return -EINVAL;

	if (state->state)
		crtc_state = drm_atomic_get_existing_crtc_state(state->state,
								state->crtc);
	else /* Special case for asynchronous cursor updates. */
		crtc_state = plane->crtc->state;

	return drm_atomic_helper_check_plane_state(plane->state, crtc_state,
						   min_scale, max_scale,
						   true, true);
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_plane_atomic_async_update(struct drm_plane *plane,
					  struct drm_atomic_state *state)
{
	struct drm_plane_state *new_state = drm_atomic_get_new_plane_state(state, plane);
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(plane->state->crtc);
	struct drm_framebuffer *old_fb = plane->state->fb;
	struct drm_plane_state modified_state;

	memcpy(&modified_state, plane->state, sizeof(struct drm_plane_state));
	state->planes[drm_plane_index(plane)].old_state = &modified_state;
#else
static void gbdc_plane_atomic_async_update(struct drm_plane *plane,
					  struct drm_plane_state *new_state)
{
	struct drm_plane_state *state;
	struct drm_plane_state old_state;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(plane->state->crtc);
	struct drm_framebuffer *old_fb = plane->state->fb;

	memcpy(&old_state, plane->state, sizeof(*state));
	state = &old_state;
#endif

	gb_printf(KERN_INFO, "%s: \n", __func__);

	plane->state->crtc_x = new_state->crtc_x;
	plane->state->crtc_y = new_state->crtc_y;
	plane->state->crtc_h = new_state->crtc_h;
	plane->state->crtc_w = new_state->crtc_w;
	plane->state->src_x = new_state->src_x;
	plane->state->src_y = new_state->src_y;
	plane->state->src_h = new_state->src_h;
	plane->state->src_w = new_state->src_w;
	swap(plane->state->fb, new_state->fb);

	if (gbdc_crtc->is_enable) {
		gb_printf(KERN_INFO, "%s: call plane_update \n",__func__);

    		if (gbdc_crtc->virt)
    			gbdc_virtual_plane_update(plane, state);
		else
			gbdc_plane_update(plane, state);

		/*
		 * A scanout can still be occurring, so we can't drop the
		 * reference to the old framebuffer. To solve this we get a
		 * reference to old_fb and set a worker to release it later.
		 * FIXME: if we perform 500 async_update calls before the
		 * vblank, then we can have 500 different framebuffers waiting
		 * to be released.
		 */
		if (old_fb && plane->state->fb != old_fb) {
			drm_framebuffer_get(old_fb);
			WARN_ON(drm_crtc_vblank_get(plane->state->crtc) != 0);
			drm_flip_work_queue(&gbdc_crtc->gbdc_fb_unref_work, old_fb);
			set_bit(GBDC_PENDING_FB_UNREF, &gbdc_crtc->pending);
		}
	}
}

static struct drm_plane_helper_funcs gbdc_de_plane_helper_funcs = {
		.atomic_check = gbdc_de_plane_check,
		.atomic_update = gbdc_plane_update,
		.atomic_disable = gbdc_plane_disable,
		.atomic_async_check = gbdc_plane_atomic_async_check,
		.atomic_async_update = gbdc_plane_atomic_async_update,
};

static struct drm_plane_helper_funcs gbdc_virtual_plane_helper_funcs = {
		.atomic_check = gbdc_de_plane_check,
		.atomic_update = gbdc_virtual_plane_update,
		.atomic_disable = gbdc_virual_plane_disable,
		.atomic_async_check = gbdc_plane_atomic_async_check,
		.atomic_async_update = gbdc_plane_atomic_async_update,
};

enum drm_plane_type GB02FUNC1858(unsigned int idx)
{
		if (idx == DC_PLANE_GRAPHIC) //1
			return DRM_PLANE_TYPE_PRIMARY;
		else if (idx == DC_PLANE_SMART) //3
			return DRM_PLANE_TYPE_CURSOR;
		else
			return DRM_PLANE_TYPE_OVERLAY;
}
static void GB02FUNC1859(struct drm_plane *plane,
						unsigned int zpos)
{
  struct drm_device *dev = plane->dev;
  struct drm_property *prop;

  prop = drm_property_create_range(dev, DRM_MODE_PROP_IMMUTABLE,
				   "zpos", 0, GB_MAX_PLANES - 1);
  if (!prop)
	  return;

  drm_object_attach_property(&plane->base, prop, zpos);
}
#if 1
int GB02FUNC1860(struct drm_device *dev,
			struct gbdc_plane *gb_plane,
			unsigned long possible_crtcs, enum drm_plane_type type,
			unsigned int zpos)
{
	int err;
	struct drm_plane *plane = NULL;

	plane = &gb_plane->base;
	err = drm_universal_plane_init(dev, plane, possible_crtcs,
					 &gbdc_de_plane_funcs, safe_modeset_formats,
					ARRAY_SIZE(safe_modeset_formats), NULL,
					 type, NULL);
	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		return err;
	}

  	drm_plane_helper_add(&gb_plane->base,
		gb_plane->virt ? &gbdc_virtual_plane_helper_funcs : &gbdc_de_plane_helper_funcs);

	gb_plane->zpos = zpos;

	if (type == DRM_PLANE_TYPE_OVERLAY)
	  GB02FUNC1859(&gb_plane->base, zpos);

	if(plane->funcs->reset)
		plane->funcs->reset(plane);

	return 0;
}
#else

int GB02FUNC1860(struct drm_device *dev,
			struct gbdc_plane *gb_plane,
			unsigned long possible_crtcs, u32 type,
			unsigned int zpos)
{
  int err;

  gb_plane->base.format_default = true;
  err = drm_universal_plane_init(dev, &gb_plane->base, possible_crtcs,
				  &drm_primary_helper_funcs, safe_modeset_formats,
				    ARRAY_SIZE(safe_modeset_formats), type, NULL);
  if (err) {
	  DRM_ERROR("failed to initialize plane\n");
	  return err;
  }

  return 0;
}
#endif
#else // CONFIG_GBDC_ATOMIC_FEATURE

static const u32 cursor_plane_formats[] = {
	DRM_FORMAT_ARGB8888,
};

static int GB02FUNC1861(struct drm_plane *plane,
	struct drm_plane_state *new_state)
{
	gb_printf(KERN_INFO, "---gb--%s-----\n", __func__);
	return 0;
}
static void GB02FUNC1862(struct drm_plane *plane,
	struct drm_plane_state *old_state)
{

	gb_printf(KERN_INFO, "---gb--%s-----\n", __func__);
}

static int GB02FUNC1863(struct drm_plane *plane,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0))
	struct drm_plane_state *state)
#else
	struct drm_atomic_state *state)
#endif
{

	gb_printf(KERN_INFO, "---gb--%s-----\n", __func__);
	return 0;
}

const struct drm_plane_helper_funcs csr_plane_helper_funcs = {
	.prepare_fb = GB02FUNC1861,
	.cleanup_fb = GB02FUNC1862,
	.atomic_check = GB02FUNC1863,

};

#if 0
int GB02FUNC1864(struct drm_plane *plane,
			    struct drm_crtc *crtc, struct drm_framebuffer *fb,
			    int crtc_x, int crtc_y,
			    unsigned int crtc_w, unsigned int crtc_h,
			    uint32_t src_x, uint32_t src_y,
			    uint32_t src_w, uint32_t src_h,
			    struct drm_modeset_acquire_ctx *ctx)
{
	return drm_plane_helper_update(plane,
			    crtc, fb,
			    crtc_x, crtc_y,
			    crtc_w, crtc_h,
			    src_x, src_y,
			    src_w, src_h);
}

int GB02FUNC1865(struct drm_plane *plane,
				 struct drm_modeset_acquire_ctx *ctx)
{
	return drm_plane_helper_disable(plane);
}

const struct drm_plane_funcs drm_plane_helper_funcs = {
	.update_plane = GB02FUNC1864,
	.disable_plane = GB02FUNC1865,
	.destroy = drm_primary_helper_destroy,
};
#else
#if 0
const struct drm_plane_funcs drm_plane_helper_funcs = {
	.update_plane = drm_primary_helper_update,
	.disable_plane = drm_primary_helper_disable,
	.destroy = drm_primary_helper_destroy,
};
#endif
#endif

struct drm_plane *GB02FUNC1866(struct drm_device *dev)
{
	struct drm_plane *cplane;
	unsigned int num_formats;
	const u32 *formats;
	int ret;

	cplane = kzalloc(sizeof(*cplane), GFP_KERNEL);
	if (cplane == NULL) {
		DRM_DEBUG_KMS("Failed to allocate cursor plane\n");
		return NULL;
	}

	num_formats = ARRAY_SIZE(cursor_plane_formats);
	formats = cursor_plane_formats;

#if (defined CONFIG_X86_64) || (defined CONFIG_CENTOS) \
	|| (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0))
	ret = drm_universal_plane_init(dev, cplane, 0,
//			&drm_plane_helper_funcs,
			&drm_primary_helper_funcs,
			formats, num_formats,
			NULL, DRM_PLANE_TYPE_CURSOR, NULL);
#else
	ret = drm_universal_plane_init(dev, cplane, 0,
//			&drm_plane_helper_funcs,
			&drm_primary_helper_funcs,
			formats, num_formats,
			DRM_PLANE_TYPE_CURSOR, NULL);

#endif
	if (ret) {
		kfree(cplane);
		cplane = NULL;
	}
	drm_plane_helper_add(cplane, &csr_plane_helper_funcs);

	return cplane;
}

#endif
