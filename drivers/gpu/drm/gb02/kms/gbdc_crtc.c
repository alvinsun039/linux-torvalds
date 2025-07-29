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
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#else
#include <drm/drm_device.h>
#include <drm/drm_vblank.h>
#include <drm/drm_drv.h>
#endif
#include <drm/drm_modes.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
#include <drm/drm_atomic_uapi.h>
#endif
#include <drm/drm_crtc.h>
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

#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <video/videomode.h>
#include <drm/drm_plane_helper.h>
#include <generated/uapi/linux/version.h>
#include <drm/drm_fb_helper.h>
#include <linux/iopoll.h>
#include <drm/drm_fourcc.h>
#include "gbdc_mode.h"
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "device/gb_dev_res.h"
#include "gbdc_pm.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "device/gb_ip.h"

#include "gbdc_mm.h"
#include "gbdc_drv.h"
#include "gbdc_planes.h"
#include "gbdc_crtc.h"
#include "gbdc_mode.h"
#include "device/gbdc_ops.h"
#include "device/reg_ops.h"
#include "device/gbdc_regs.h"
#include "gbdc_display.h"
#include "gpu/gb_device.h"
#include "gpu/gb_ttm.h"
#include "gbdc_irq.h"
#include "ip/gb_dp.h"
#ifdef	GB02MAC292
#include "audio/i2s_platform.h"
#endif
#include "gbdc_connector.h"
extern unsigned int drm_debug;

int GB02FUNC970(struct drm_crtc *crtc, int x, int y,
								   struct drm_framebuffer *old_fb)
{
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR159 *gbdc_fb;
	struct gbdc_plane *gb_plane;
	struct GB02STR56 *bo;
	int crtc_id = gbdc_crtc->crtc_id;
	u64 plane_vram_addr = 0;
	int ret;

	gb_plane = &gbdc_crtc->cplanes[DC_PLANE_GRAPHIC];
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

	if (WARN_ON(crtc->primary->fb == NULL)) {
		gb_printf(KERN_ERR, "%s: Error in %d\n", __func__, __LINE__);
		return -EINVAL;
	}
	gb_printf(KERN_INFO, "%s: crtc%d\n", __func__, crtc_id);

	gbdc_fb = to_gbdc_framebuffer(crtc->primary->fb);
	//gb_plane = to_gb_plane_info(crtc->primary);
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
	if (ret)
		return ret;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	ret = GB02FUNC1485(&bo->ttm_bo, TTM_PL_FLAG_VRAM, &plane_vram_addr);
#else
	ret = GB02FUNC1485(&bo->ttm_bo, TTM_PL_VRAM, &plane_vram_addr);
#endif

	if (ret) {
		ttm_bo_unreserve(&bo->ttm_bo.bo);
		return ret;
	}

	ttm_bo_unreserve(&bo->ttm_bo.bo);
#if !(defined SYS_CENTOS7_COMPILE_ENV || defined SYS_CENTOS7_9_2009) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
	gb_plane->cur_fb_offset = (unsigned long)plane_vram_addr + y * gbdc_fb->base.pitches[0] + x * (gbdc_fb->base.bits_per_pixel / 8);
#else
	gb_plane->cur_fb_offset = (unsigned long)plane_vram_addr + y * gbdc_fb->base.pitches[0] + x * gbdc_fb->base.format->cpp[0];
#endif
	return 0;

}

static void GB02FUNC978(struct drm_crtc *crtc, bool enabled)
{
	struct GB02STR155 *gb_dev;
	struct gbdc_crtc *gb_crtc;
	struct GB02STR249 *dc_config;
	void __iomem *dc_base;
	void __iomem *de_base;
	int j;
	u32 val;

	if (!crtc)
		return;
	gb_crtc = GB02FUNC1573(crtc);
	if(gb_crtc->virt)
		return;
	gb_dev = GB02FUNC85(crtc->dev->dev_private);

	if (gb_crtc) {
		dc_config = &gb_dev->pcie_info.dc_config[gb_crtc->crtc_id];
		dc_base = dc_config->dc_base;
		de_base = dc_config->de_base;

		for (j = 0; j < gb_dev->kms_info.num_plane; j++) {
			struct GB02STR77 *plane_res = &gb_dev->kms_info.plane_res[j];

			val = GB02FUNC730(de_base, plane_res->base + GB02MAC696);
			if (enabled)
				val |= GB02MAC698;
			else
				val &= ~GB02MAC698;
			GB02FUNC733(de_base, plane_res->base + GB02MAC696, val);
		}
		GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	}

	return;
}

static int GB02FUNC983(struct drm_crtc *crtc)
{
	void __iomem *hdmi_base;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	int crtc_id = gbdc_crtc->crtc_id;
	struct GB02STR155 *gb_dev =
		GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;

	hdmi_base= ip_config->hdmi_config_base;
	if (!gbdc_crtc->virt && modeset_ops->switch_conn)
		modeset_ops->switch_conn(hdmi_base, crtc_id,  GB02MAC1090);

	return 0;
}

static int GB02FUNC984(struct drm_crtc *crtc)
{
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev =
		GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;

	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;
	int crtc_id = gbdc_crtc->crtc_id;
	void __iomem *hdmi_base = ip_config->hdmi_config_base;
	int virt_crtc_id = -1;
	struct drm_crtc *virt_crtc = NULL;

	if (WARN_ON(gbdc_crtc->virt))
		return -EPERM;

	virt_crtc_id =
		GB02FUNC1692(crtc->dev, gbdc_crtc->crtc_id, false);
	if (virt_crtc_id != -1) {
		drm_for_each_crtc(virt_crtc, crtc->dev) {
			if (virt_crtc_id == GB02FUNC1573(virt_crtc)->crtc_id)
				break;
		}
	}

	if (!virt_crtc ? !virt_crtc : !GB02FUNC1573(virt_crtc)->is_enable)
		if (modeset_ops->switch_conn)
			modeset_ops->switch_conn(hdmi_base, crtc_id, GB02MAC1091);
	return 0;
}

static void GB02FUNC986(struct drm_crtc *crtc, int mode)
{
	int mode0;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev =
			GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR249 *dc_config;
	struct GB02STR252 *ip_config;
	int crtc_id = gbdc_crtc->crtc_id;

	ip_config = &gb_dev->pcie_info.ip_config;
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];

	mode0 = kms_ops->get_resolution(&crtc->mode);

	switch (mode) {
		case DRM_MODE_DPMS_ON:
			gb_printf(KERN_INFO, "%s mode=%d crtc_id = %d, mode0=%d\n", __func__, mode, crtc_id, mode0);
			if(gbdc_crtc->mod_flag != MODE_SET_DONE)
				GB02FUNC983(crtc);
			break;
		case DRM_MODE_DPMS_STANDBY:
			gb_printf(KERN_INFO, "%s mode=%d crtc_id = %d, mode0 =%d\n", __func__, mode, crtc_id, mode0);
			break;
		case DRM_MODE_DPMS_SUSPEND:
			gb_printf(KERN_INFO, "%s mode=%d crtc_id = %d, mode0 =%d\n", __func__, mode, crtc_id, mode0);
			break;
		case DRM_MODE_DPMS_OFF:
			gb_printf(KERN_INFO, "%s mode=%d crtc_id = %d, mode0 =%d\n", __func__, mode, crtc_id, mode0);
			GB02FUNC984(crtc);
			gbdc_crtc->mod_flag = MODE_SET_NONE;
			break;
		default:
			return;
	}
}
static void GB02FUNC989(struct drm_crtc *crtc, int mode)
{
	struct gbdc_crtc *virt_crtc = GB02FUNC1573(crtc);
	struct gbdc_crtc *gbdc_crtc;
	struct gbdc_crtc_state *gbdc_crtc_state = to_gbdc_crtc_state(crtc->state);
	int virt_crtc_id = virt_crtc->crtc_id;
	int crtc_id;

	pr_info("%s: virt crtcid %d\n", __func__, virt_crtc_id);

	gbdc_for_each_gbdc_obj(crtc) {
		if (!gbdc_crtc)
			continue;

		crtc_id = gbdc_crtc->crtc_id;

		switch (mode) {
			case DRM_MODE_DPMS_ON:
				gb_printf(KERN_INFO, "%s:DPMSON,crtcid = %d\n", __func__, crtc_id);
				if(gbdc_crtc->mod_flag != MODE_SET_DONE)
					GB02FUNC983(crtc);
				break;
			case DRM_MODE_DPMS_STANDBY:
				gb_printf(KERN_INFO, "%s:STANDBY,crtcid%d\n", __func__, crtc_id);
				break;
			case DRM_MODE_DPMS_SUSPEND:
				gb_printf(KERN_INFO, "%s:SUSPEND,crtc_id%d\n", __func__, crtc_id);
				break;
			case DRM_MODE_DPMS_OFF:
				gb_printf(KERN_INFO, "%s:DPMSOFF,crtc_id%d\n", __func__, crtc_id);
				GB02FUNC984(crtc);
				gbdc_crtc->mod_flag = MODE_SET_NONE;
				break;
			default:
				return;
		}

	}

}
static int GB02FUNC990(struct drm_crtc *crtc,
							  struct drm_display_mode *mode,
							  struct drm_display_mode *adjusted_mode,
							  int x, int y, struct drm_framebuffer *old_fb)
{
	int mode0;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev =
			GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;
	struct GB02STR249 *dc_config;
	int crtc_id = gbdc_crtc->crtc_id;
	struct videomode vmode;
	void __iomem *dc_base;
	void __iomem *de_base;
	struct GB02STR77 *pplane;
	struct GB02STR39 *gbdev = NULL;
	struct GB02STR77 *cplane;
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;

	//dump_stack();
	if(gbdc_crtc->virt)
		return 0;
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	pplane = &gb_dev->kms_info.plane_res[DC_PLANE_GRAPHIC];
	cplane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;

	mode0 = kms_ops->get_resolution(&gbdc_crtc->base.mode);

	gb_printf(KERN_INFO, "pthread name =%s crtc %d mode: %dx%d adjusted_mode: %dx%d clock:%dkHz\n",
		current->comm, crtc_id, mode->hdisplay, mode->vdisplay,
		adjusted_mode->hdisplay,
		adjusted_mode->vdisplay,
		adjusted_mode->clock);
	if (modeset_ops->switch_conn)
		modeset_ops->switch_conn(ip_config->hdmi_config_base, crtc_id,
			GB02MAC1090);

	if (modeset_ops->prepare_config)
		modeset_ops->prepare_config(dc_base, ip_config, mode0, crtc_id);
	kms_ops->enter_config(dc_base, de_base);
	mdelay(10);
	if (modeset_ops->modeset_config(ip_config,
		(const struct drm_display_mode *)&crtc->mode, crtc_id)) {
		gb_printf(KERN_ERR, "%s set crtc clock failed!!!", __func__);
		kms_ops->leave_config(dc_base, de_base);
		return -1;
	}
	GB02FUNC1160(adjusted_mode, &vmode);
	kms_ops->config_mode(de_base, &vmode);
	GB02FUNC970(crtc, x, y, old_fb);
	kms_ops->config_base(crtc, pplane, de_base);
	GB02FUNC281(dc_base, de_base);

	mdelay(10);
	kms_ops->leave_config(dc_base, de_base);
	gbdev = GB02FUNC183(gb_dev->drm_dev);
	GB02FUNC1737(gbdev, crtc_id, gbdc_crtc->base.mode.vdisplay, gbdc_crtc->base.mode.vdisplay);
	gbdc_crtc->mod_flag = MODE_SET_DONE;
	gb_printf(KERN_INFO, "End modeset...\n");
#ifdef	GB02MAC292
	GB02FUNC556(crtc_id, AUDIO_NEED_RESTART);
#endif
	return 0;
}

static void GB02FUNC993(struct drm_crtc *crtc)
{
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	int ret = 0;

	if (gbdc_crtc && gbdc_crtc->cursor_resume) {
		gbdc_crtc->cursor_resume = 0;
		gb_printf(KERN_INFO, "crtc_id %d ,handle %d\n", gbdc_crtc->crtc_id,
				gbdc_crtc->drm_file_handle);
		if (crtc->funcs->cursor_set2)
			ret = crtc->funcs->cursor_set2(crtc, gbdc_crtc->drmfile,
			gbdc_crtc->drm_file_handle, gbdc_crtc->cursor_width,
			gbdc_crtc->cursor_height, gbdc_crtc->cursor_hot_x,
			gbdc_crtc->cursor_hot_y);
		gb_printf(KERN_INFO, "%s: resume cursor set2 ret %d\n", __func__, ret);
	}
}

static bool GB02FUNC997(struct drm_crtc *crtc,
				const struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	return true;
}
static bool GB02FUNC999(struct drm_crtc *crtc,
					const struct drm_display_mode *mode,
					struct drm_display_mode *adjusted_mode)
{
	return true;
}
static void GB02FUNC1004(struct drm_crtc *crtc, struct drm_crtc_state *old_state)
{
	struct drm_crtc_state *state = crtc->state;
	struct drm_color_lut *lut;
	void __iomem *dc_base;
	void __iomem *de_base;
	struct GB02STR249 *dc_config;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	int crtc_id = gbdc_crtc->crtc_id;
	struct GB02STR155 *gb_dev =
		 GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;

	if (gbdc_crtc->virt || !state->gamma_lut)
		return;
	
	gb_printf(KERN_DEBUG,"crtcid %d\n",crtc_id);
	
	if (drm_color_lut_size(state->gamma_lut) != GB02MAC1424)
		return;

	lut = state->gamma_lut->data;
	if (lut[1].blue < GB02MAC1425)
		return;

	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;
	kms_ops->gamma_set(dc_base, de_base, lut);
	return;
}
static void GB02FUNC1007(struct drm_crtc *crtc, struct drm_crtc_state *old_state)
{
	struct drm_crtc_state *state = crtc->state;
	struct drm_color_lut *lut;
	void __iomem *dc_base;
	void __iomem *de_base;
	struct GB02STR249 *dc_config;
	//struct gbdc_crtc *vcrtc = GB02FUNC1573(crtc);
	int crtc_id;
	struct gbdc_crtc *gbdc_crtc;
	struct GB02STR155 *gb_dev =
		 GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct gbdc_crtc_state *gbdc_crtc_state = to_gbdc_crtc_state(old_state);
	if (!state->gamma_lut)
		return;
	if (drm_color_lut_size(state->gamma_lut) != GB02MAC1424)
		return;

	lut = state->gamma_lut->data;
	if (lut[1].blue < GB02MAC1425)
		return;

	gbdc_for_each_gbdc_obj(crtc) {
		if (!gbdc_crtc)
			continue;
		crtc_id = gbdc_crtc->crtc_id;
		gb_printf(KERN_DEBUG,"crtc_id %d\n",crtc_id);
		dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
		dc_base = dc_config->dc_base;
		de_base = dc_config->de_base;
		kms_ops->gamma_set(dc_base, de_base, lut);
	}


}
struct drm_vblank_crtc *GB02FUNC1011(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	unsigned int pipe = drm_crtc_index(crtc);
	struct drm_vblank_crtc *vblank = &dev->vblank[pipe];

	return vblank;
}
int GB02FUNC1012(struct drm_crtc *crtc)
{
	struct GB02STR155 *gb_dev =
		 GB02FUNC85(crtc->dev->dev_private);
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

	if (WARN_ON(gbdc_crtc->virt))
		return -EPERM;

	if (WARN_ON(!gbdc_crtc->is_enable))
		return -EPERM;

	GB02FUNC1741(gb_dev, crtc, true);

	return 0;
}

void GB02FUNC1014(struct drm_crtc *crtc)
{
	struct GB02STR155 *gb_dev =
		 GB02FUNC85(crtc->dev->dev_private);
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	int virt_crtc_id = -1;
	struct drm_crtc *virt_crtc = NULL;

	if (WARN_ON(gbdc_crtc->virt))
		return;

	if (WARN_ON(!gbdc_crtc->is_enable))
		return;

	virt_crtc_id = GB02FUNC1692(crtc->dev, gbdc_crtc->crtc_id, false);
	if (virt_crtc_id != -1) {
		drm_for_each_crtc(virt_crtc, crtc->dev) {
			if (virt_crtc_id == GB02FUNC1573(virt_crtc)->crtc_id)
				break;
		}
	}

	if (!virt_crtc ? !virt_crtc : !GB02FUNC1573(virt_crtc)->is_enable)
		GB02FUNC1741(gb_dev, crtc, false);
}

static int __maybe_unused GB02FUNC1017(struct drm_crtc *crtc)
{
	struct drm_encoder *encoder;
	struct drm_connector *connector, *virt_connector = NULL;
	struct drm_connector_list_iter conn_iter;
	struct gbdc_connector *gbdc_virtconn, *gbdc_phyconn;
	int vb_id = 0;

	drm_for_each_encoder(encoder, crtc->dev) {
		if (encoder->crtc != crtc)
			continue;

		drm_connector_list_iter_begin(crtc->dev, &conn_iter);
		drm_for_each_connector_iter(connector, &conn_iter) {
			if (connector->encoder != encoder)
				continue;
			if (connector->index == crtc->index) {
				virt_connector = connector;
				pr_info("%s:found virtconn %s\n",__func__,virt_connector->name);
				break;
			}

		}
		drm_connector_list_iter_end(&conn_iter);
	}

	gbdc_virtconn = to_gbdc_connector(virt_connector);
	pr_info("%s: virtconn id %d\n",__func__, gbdc_virtconn->connector_id);
	pr_info("%s: virtconn list empty %d\n",__func__, list_empty(&gbdc_virtconn->head));
	if (!list_empty(&gbdc_virtconn->head)) {
		list_for_each_entry_reverse(gbdc_phyconn, &gbdc_virtconn->head, head)
			if (gbdc_phyconn && !gbdc_phyconn->virt) {
				pr_info("%s:found tail phyconn %d\n",__func__, gbdc_phyconn->connector_id);
				vb_id = gbdc_phyconn->connector_id;
				break;
			}
	}
	return vb_id;

}
int GB02FUNC1023(struct drm_crtc *crtc)
{
	struct GB02STR155 *gbdc_dev =
		 GB02FUNC85(crtc->dev->dev_private);
	struct gbdc_crtc *virt_crtc = GB02FUNC1573(crtc);
	struct gbdc_crtc *gbdc_crtc = NULL;

	//if (WARN_ON(!virt_crtc->is_enable))
	//	return -EPERM;
	if (!virt_crtc->is_enable)
		return -EPERM;

	list_for_each_entry(gbdc_crtc, &virt_crtc->head, head)
		if(gbdc_crtc && !gbdc_crtc->virt && (unsigned)gbdc_crtc->crtc_id < GB02MAC2693)
			GB02FUNC1743(gbdc_dev, &gbdc_crtc->base, true);

	return 0;
}

void GB02FUNC1025(struct drm_crtc *crtc)
{
	struct GB02STR155 *gbdc_dev =
		 GB02FUNC85(crtc->dev->dev_private);
	struct gbdc_crtc *virt_crtc = GB02FUNC1573(crtc);
	struct gbdc_crtc *gbdc_crtc = NULL;

	if (WARN_ON(!virt_crtc->is_enable))
		return;

	list_for_each_entry_reverse(gbdc_crtc, &virt_crtc->head, head)
		if(gbdc_crtc && !gbdc_crtc->virt && (unsigned)gbdc_crtc->crtc_id < GB02MAC2693)
			GB02FUNC1743(gbdc_dev, &gbdc_crtc->base, false);

}
void GB02FUNC1026(struct drm_crtc *crtc)
{
	struct GB02STR77 *plane;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev =
	 GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR249 *dc_config;
	int crtc_id = gbdc_crtc->crtc_id;
	void __iomem *dc_base;
	void __iomem *de_base;

	plane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;

	kms_ops->hide_csr(plane, dc_base, de_base);

}

int GB02FUNC1028(struct drm_plane *plane, struct drm_crtc *crtc)
{
	struct gbdc_plane *new_plane = NULL;
	int plane_id;
	struct GB02STR77 *plane_res;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev =
		 GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR249 *dc_config;
	int crtc_id = gbdc_crtc->crtc_id;
	void __iomem *dc_base;
	void __iomem *de_base;

	new_plane = container_of(plane, struct gbdc_plane, base);
	plane_id = new_plane->plane_id;
	plane_res = &gb_dev->kms_info.plane_res[plane_id];
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;

	kms_ops->colse_overly_plane(plane_res->base, dc_base, de_base);

	return 0;
}

int GB02FUNC1032(struct drm_crtc *crtc,
					int x, int y)
{
	int width = 0, height = 0, stride = 0;
	struct GB02STR77 *plane;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR249 *dc_config;
	int crtc_id = gbdc_crtc->crtc_id;
	void __iomem *dc_base;
	void __iomem *de_base;
	int cursor_xend,frame_xend;
	int cursor_yend,frame_yend;

	plane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;

	//crtcid=%d, x %d y %d c->x %d c->y %d\n", gbdc_crtc->cursor_width, gbdc_crtc->cursor_height, gbdc_crtc->cursor_hot_x, gbdc_crtc->cursor_hot_y, crtc_id, x, y, crtc->x, crtc->y);
	gbdc_crtc->cursor_x = x;
	gbdc_crtc->cursor_y = y;
	width = gbdc_crtc->cursor_width;
	height = gbdc_crtc->cursor_height;
	stride = gbdc_crtc->cursor_width * 4;

	if (x < -gbdc_crtc->cursor_width) {
		GB02FUNC1026(crtc);
		gbdc_crtc->cursor_addr = gbdc_crtc->static_cursor_addr;
		return 0;
	} else if (x < 0) {
		gbdc_crtc->cursor_addr = gbdc_crtc->static_cursor_addr - 4 * x;
		width = gbdc_crtc->cursor_width + x;
		x = 0;
	} else {
		gbdc_crtc->cursor_addr = gbdc_crtc->static_cursor_addr;
	}
	if (y < -gbdc_crtc->cursor_height) {
		GB02FUNC1026(crtc);
		return 0;
	} else if (y < 0) {
		gbdc_crtc->cursor_addr = gbdc_crtc->cursor_addr - stride * y;
		height = gbdc_crtc->cursor_height + y;
		y = 0;
	} else {
	}

	cursor_xend = x + width;//2556
	frame_xend = crtc->mode.hdisplay;
	if (cursor_xend >= frame_xend) {
		width = width - (cursor_xend -frame_xend);//2556+128-2560
	} else if (cursor_xend <= 0) {
		goto out_of_bounds;
	}
	if (width < 8) {
		if (width < 5) {
			goto out_of_bounds;
		}
		width = 8;
		x = frame_xend - width;
	}

	cursor_yend = y + height;
	frame_yend = crtc->mode.vdisplay;
	if (cursor_yend >= frame_yend) {
		height = height - (cursor_yend -frame_yend);
	} else if (cursor_yend <= 0) {
		goto out_of_bounds;
	}
	if (height <= 0) {
		goto out_of_bounds;
	}

	GB02FUNC1054(crtc);
	gb_printf(KERN_INFO, "*************crtc id=%d--,x=%d, y=%d, width=%d, h=%d--stride=%d---\n",crtc_id, x, y, width, height, stride);
	kms_ops->config_csr_pos(plane, dc_base, de_base, x, y, width, height, stride);

	return 0;

out_of_bounds:
	//pr_info("%s: out of bounds,call hide\n",__func__);
	GB02FUNC1026(crtc);
	return 0;
}

int GB02FUNC1045(struct drm_crtc *crtc,
					int x, int y)
{
	int width = 0, height = 0, stride = 0;
	struct GB02STR77 *plane;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR249 *dc_config;
	int crtc_id = gbdc_crtc->crtc_id;
	void __iomem *dc_base;
	void __iomem *de_base;
	int cursor_xend,frame_xend;
	int cursor_yend,frame_yend;

	plane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;

	gbdc_crtc->cursor_x = x;
	gbdc_crtc->cursor_y = y;
	width = gbdc_crtc->cursor_width;
	height = gbdc_crtc->cursor_height;
	stride = gbdc_crtc->cursor_width * 4;

	if (x < -gbdc_crtc->cursor_width) {
		GB02FUNC1026(crtc);
		gbdc_crtc->cursor_addr = gbdc_crtc->static_cursor_addr;
		return 0;
	} else if (x < 0) {
		gbdc_crtc->cursor_addr = gbdc_crtc->static_cursor_addr - 4 * x;
		width = gbdc_crtc->cursor_width + x;
		x = 0;
	} else {
		gbdc_crtc->cursor_addr = gbdc_crtc->static_cursor_addr;
	}
	if (y < -gbdc_crtc->cursor_height) {
		GB02FUNC1026(crtc);
		return 0;
	} else if (y < 0) {
		gbdc_crtc->cursor_addr = gbdc_crtc->cursor_addr - stride * y;
		height = gbdc_crtc->cursor_height + y;
		y = 0;
	} else {
	}

	cursor_xend = x + width;//2556
	frame_xend = gbdc_crtc->adjusted_mode.hdisplay;
	if (cursor_xend >= frame_xend) {
		width = width - (cursor_xend -frame_xend);//2556+128-2560
	} else if (cursor_xend <= 0) {
		goto out_of_bounds;
	}
	if (width < 8) {
		if (width < 5) {
			goto out_of_bounds;
		}
		width = 8;
		x = frame_xend - width;
	}

	cursor_yend = y + height;
	frame_yend = gbdc_crtc->adjusted_mode.vdisplay;
	if (cursor_yend >= frame_yend) {
		height = height - (cursor_yend -frame_yend);
	} else if (cursor_yend <= 0) {
		goto out_of_bounds;
	}
	if (height <= 0) {
		goto out_of_bounds;
	}

	GB02FUNC1054(crtc);
	gb_printf(KERN_INFO, "*************crtc id=%d--,x=%d, y=%d, width=%d, h=%d--stride=%d---\n",crtc_id, x, y, width, height, stride);
	kms_ops->config_csr_pos(plane, dc_base, de_base, x, y, width, height, stride);

	return 0;

out_of_bounds:
	//pr_info("%s: out of bounds,call hide\n",__func__);
	GB02FUNC1026(crtc);
	return 0;
}

void GB02FUNC1054(struct drm_crtc *crtc)
{
	struct GB02STR77 *plane;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR249 *dc_config;
	int crtc_id = gbdc_crtc->crtc_id;
	void __iomem *dc_base;
	void __iomem *de_base;

	plane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;
	kms_ops->save_csr_image(plane, dc_base, de_base,
			gbdc_crtc->cursor_addr, gbdc_crtc->cursor_width,
			gbdc_crtc->cursor_height);
}

static int GB02FUNC1056(struct drm_crtc *crtc,
				      struct drm_file *file_priv,
				      uint32_t handle,
				      uint32_t width,
				      uint32_t height,
				      int32_t hot_x,
					  int32_t hot_y)
{
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct drm_gem_object *obj;
	struct GB02STR56 *gobj;
	int ret;
	struct GB02STR77 *cplane;
	void __iomem *de_base;
	struct GB02STR249 *dc_config;
	struct GB02STR155 *gb_dev = GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	int crtc_id = gbdc_crtc->crtc_id;

	cplane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	de_base = dc_config->de_base;

	if (!handle) {
		/* turn off cursor */
		GB02FUNC1026(crtc);
		obj = NULL;
		goto unpin;
		//return -EINVAL;
	}

	kms_ops->config_base(crtc, cplane, de_base);

	if ((width > gbdc_crtc->max_cursor_width) ||
	    (height > gbdc_crtc->max_cursor_height)) {
		DRM_ERROR("bad cursor width or height %d x %d\n",
		 width, height);
		return -EINVAL;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) ||\
 (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS)\
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS)
	obj = drm_gem_object_lookup(file_priv, handle);
#else
	obj = drm_gem_object_lookup(crtc->dev, file_priv, handle);
#endif
	if (!obj) {
		DRM_ERROR("Cannot find cursor object %x for crtc %d\n",
		 handle, gbdc_crtc->crtc_id);
		return -ENOENT;
	}

	gobj = GB02FUNC217(obj);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) ||\
 (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS)\
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS)
		ret = ttm_bo_reserve(&gobj->ttm_bo.bo, true, false, NULL);
#else
		ret = ttm_bo_reserve(&gobj->ttm_bo.bo, true, false, false, NULL);
#endif

	if (ret != 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
		drm_gem_object_put_unlocked(obj);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(obj);
#else
		drm_gem_object_unreference_unlocked(obj);
#endif
		return ret;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	ret = GB02FUNC1485(&gobj->ttm_bo, TTM_PL_FLAG_VRAM, &gbdc_crtc->cursor_addr);
#else
	ret = GB02FUNC1485(&gobj->ttm_bo, TTM_PL_VRAM, &gbdc_crtc->cursor_addr);
#endif

	ttm_bo_unreserve(&gobj->ttm_bo.bo);
	if (ret) {
		DRM_ERROR("Failed to pin new cursor BO (%d)\n", ret);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
		drm_gem_object_put_unlocked(obj);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(obj);
#else
		drm_gem_object_unreference_unlocked(obj);
#endif
		return ret;
	}
	gbdc_crtc->static_cursor_addr = gbdc_crtc->cursor_addr;

	gbdc_crtc->cursor_width = width;
	gbdc_crtc->cursor_height = height;
	gbdc_crtc->cursor_hot_x = hot_x;
	gbdc_crtc->cursor_hot_y = hot_y;

	ret = GB02FUNC1032(crtc,
	 gbdc_crtc->cursor_x, gbdc_crtc->cursor_y);
	if (ret) {
		GB02FUNC1026(crtc);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
		drm_gem_object_put_unlocked(obj);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(obj);
#else
		drm_gem_object_unreference_unlocked(obj);
#endif
		return ret;
	}
	gbdc_crtc->drm_file_handle = handle;
	gbdc_crtc->drmfile = file_priv;
	GB02FUNC1054(crtc);

unpin:
	if (gbdc_crtc->cursor_bo) {
		struct GB02STR56 *gobj = GB02FUNC217(gbdc_crtc->cursor_bo);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) ||\
 (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS)\
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS)
		ret = ttm_bo_reserve(&gobj->ttm_bo.bo, false, false, NULL);
#else
		ret = ttm_bo_reserve(&gobj->ttm_bo.bo, false, false, false, NULL);
#endif
		if (likely(ret == 0)) {
			GB02FUNC1487(&gobj->ttm_bo);
			ttm_bo_unreserve(&gobj->ttm_bo.bo);
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
		drm_gem_object_put_unlocked(gbdc_crtc->cursor_bo);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(gbdc_crtc->cursor_bo);
#else
		drm_gem_object_unreference_unlocked(gbdc_crtc->cursor_bo);
#endif
	}

	gbdc_crtc->cursor_bo = obj;
	return 0;
}

static struct drm_crtc_state *GB02FUNC1074(struct drm_crtc *crtc)
{
	struct gbdc_crtc_state *gbdc_state, *old_state;
//	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

	if (WARN_ON(!crtc->state))
		return NULL;

	old_state = to_gbdc_crtc_state(crtc->state);
	gbdc_state = kzalloc(sizeof(*gbdc_state), GFP_KERNEL);
	if (!gbdc_state)
		return NULL;
	gb_printf(KERN_DEBUG, "%s: %d, crtc name %s, driver name:%s\n",
			__func__, __LINE__, crtc->name, crtc->dev->driver->name);

	__drm_atomic_helper_crtc_duplicate_state(crtc, &gbdc_state->base);

	memcpy(gbdc_state->gamma_coeffs, old_state->gamma_coeffs,
		sizeof(gbdc_state->gamma_coeffs));
	memcpy(gbdc_state->coloradj_coeffs, old_state->coloradj_coeffs,
		sizeof(gbdc_state->coloradj_coeffs));
	memcpy(&gbdc_state->scaler_config, &old_state->scaler_config,
		sizeof(gbdc_state->scaler_config));
	gbdc_state->scaled_planes_mask = 0;
	gbdc_state->infinity_crtc_list = old_state->infinity_crtc_list;
	gbdc_state->vblank_id = old_state->vblank_id;
	gbdc_state->need_disable_changed = false;
	gbdc_state->need_disable = false;

	return &gbdc_state->base;
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_crtc_atomic_enable(struct drm_crtc *crtc,
									struct drm_atomic_state *state)
{
	struct drm_crtc_state *old_state = drm_atomic_get_old_crtc_state(state, crtc);
#else
static void gbdc_crtc_atomic_enable(struct drm_crtc *crtc,
					struct drm_crtc_state *old_state)
{
#endif
	int mode0;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct GB02STR155 *gb_dev =
			GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;
	struct GB02STR249 *dc_config;
	int crtc_id = gbdc_crtc->crtc_id;
	struct videomode vmode;
	void __iomem *dc_base;
	void __iomem *de_base;
	struct GB02STR77 *pplane;
	struct GB02STR39 *gbdev = NULL;
	struct GB02STR77 *cplane;
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;
	struct drm_display_mode *adjusted_mode = &crtc->state->adjusted_mode;
#ifdef GB02MAC240
	struct GB02STR19 *modeinfo;
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 3, 0)
	if (old_state && old_state->self_refresh_active) {
		drm_crtc_vblank_on(crtc);
		GB02FUNC978(crtc, true);
		return;
	}
#endif

	gb_printf(KERN_INFO, "%s: crtcid %d\n", __func__, gbdc_crtc->crtc_id);
	if(gbdc_crtc->virt)
		return;
	mutex_lock(&gbdc_crtc->gbdc_crtc_lock);

	GB02FUNC986(crtc, DRM_MODE_DPMS_ON);

	WARN_ON(gbdc_crtc->event);

	gbdc_crtc->is_enable = true;
	gbdc_crtc->need_disable = false;

	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	pplane = &gb_dev->kms_info.plane_res[DC_PLANE_GRAPHIC];
	cplane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;
#ifdef GB02MAC240
	modeinfo = &gb_dev->gb_dbg_info.modeinfo[crtc_id];
	modeinfo->vdisplay = adjusted_mode->vdisplay;
	modeinfo->hdisplay = adjusted_mode->hdisplay;
#endif
	mode0 = kms_ops->get_resolution(&gbdc_crtc->base.mode);

	gb_printf(KERN_INFO, "crtc %d adjusted_mode: %dx%d clock:%dkHz\n",
		crtc_id,
		adjusted_mode->hdisplay,
		adjusted_mode->vdisplay,
		adjusted_mode->clock);
	if (modeset_ops->switch_conn)
		modeset_ops->switch_conn(ip_config->hdmi_config_base, crtc_id,
			GB02MAC1090);

	if (modeset_ops->prepare_config)
		modeset_ops->prepare_config(dc_base, ip_config, mode0, crtc_id);
	kms_ops->enter_config(dc_base, de_base);
	if (modeset_ops->modeset_config(ip_config,
		(const struct drm_display_mode *)&crtc->state->mode, crtc_id)) {
		gb_printf(KERN_ERR, "%s set crtc clock failed!!!", __func__);
		kms_ops->leave_config(dc_base, de_base);
		mutex_unlock(&gbdc_crtc->gbdc_crtc_lock);
		return;
	}
	GB02FUNC1160(adjusted_mode, &vmode);
	kms_ops->config_mode(de_base, &vmode);
	GB02FUNC281(dc_base, de_base);

	kms_ops->leave_config(dc_base, de_base);
	gbdev = GB02FUNC183(gb_dev->drm_dev);
	GB02FUNC1737(gbdev, crtc_id, gbdc_crtc->base.mode.vdisplay, gbdc_crtc->base.mode.vdisplay);
	gbdc_crtc->mod_flag = MODE_SET_DONE;
	drm_crtc_vblank_on(crtc);
	gb_printf(KERN_INFO, "End modeset...\n");
#ifdef	GB02MAC292
	GB02FUNC556(crtc_id, AUDIO_NEED_RESTART);
#endif
	if (crtc->state->gamma_lut)
		GB02FUNC1004(crtc, old_state);
	mutex_unlock(&gbdc_crtc->gbdc_crtc_lock);
	return;
}
#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_virt_crtc_atomic_enable(struct drm_crtc *crtc,
									struct drm_atomic_state *state)
{
	struct drm_crtc_state *old_state = drm_atomic_get_old_crtc_state(state, crtc);
#else
static void gbdc_virt_crtc_atomic_enable(struct drm_crtc *crtc,
					struct drm_crtc_state *old_state)
{
#endif
	int mode0;
	struct gbdc_crtc *virt_crtc = GB02FUNC1573(crtc);
	struct gbdc_crtc *gbdc_crtc;
	struct GB02STR155 *gb_dev =
			GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR244 *kms_ops = gb_dev->kms_info.kms_ops;
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;
	struct GB02STR249 *dc_config;
	int crtc_id;
	struct videomode vmode;
	void __iomem *dc_base;
	void __iomem *de_base;
	struct GB02STR77 *pplane;
	struct GB02STR39 *gbdev = NULL;
	struct GB02STR77 *cplane;
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;
	struct drm_display_mode *vadjusted_mode = &crtc->state->adjusted_mode;
	struct drm_display_mode *adjusted_mode;
	struct gbdc_crtc_state *gbdc_crtc_state = to_gbdc_crtc_state(old_state);


	gb_printf(KERN_INFO, "%s: virt %d,crtcid %d,vblank_id %d\n", __func__,
			virt_crtc->virt, virt_crtc->crtc_id,virt_crtc->vblank_id);

	dump_mode(vadjusted_mode, false);


	mutex_lock(&virt_crtc->gbdc_crtc_lock);
	virt_crtc->is_enable = true;
	virt_crtc->need_disable = false;

	gbdc_for_each_gbdc_obj(crtc) {

		if (!gbdc_crtc)
			continue;
		crtc_id = gbdc_crtc->crtc_id;

		mutex_lock(&gbdc_crtc->gbdc_crtc_lock);
		GB02FUNC986(crtc, DRM_MODE_DPMS_ON);

		gbdc_crtc->is_enable = true;
		gbdc_crtc->need_disable = false;

		dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
		pplane = &gb_dev->kms_info.plane_res[DC_PLANE_GRAPHIC];
		cplane = &gb_dev->kms_info.plane_res[DC_PLANE_SMART];
		dc_base = dc_config->dc_base;
		de_base = dc_config->de_base;

		adjusted_mode = &gbdc_crtc->adjusted_mode;
		if(!adjusted_mode->hdisplay || !adjusted_mode->vdisplay || !adjusted_mode->clock) {
			mutex_unlock(&gbdc_crtc->gbdc_crtc_lock);
			continue;
		}
		mode0 = kms_ops->get_resolution(adjusted_mode);

		gb_printf(KERN_INFO, "%s: crtc %d adjusted_mode: %dx%d clock:%dkHz\n",
			__func__, crtc_id, adjusted_mode->hdisplay, adjusted_mode->vdisplay,
			adjusted_mode->clock);
		if (modeset_ops->switch_conn)
			modeset_ops->switch_conn(ip_config->hdmi_config_base, crtc_id,
				GB02MAC1090);

		if (modeset_ops->prepare_config)
			modeset_ops->prepare_config(dc_base, ip_config, mode0, crtc_id);
		kms_ops->enter_config(dc_base, de_base);
		if (modeset_ops->modeset_config(ip_config,
			(const struct drm_display_mode *)adjusted_mode, crtc_id)) {
			gb_printf(KERN_ERR, "%s set crtc clock failed!!!", __func__);
			kms_ops->leave_config(dc_base, de_base);
			mutex_unlock(&gbdc_crtc->gbdc_crtc_lock);
			continue;
		}
		GB02FUNC1160(adjusted_mode, &vmode);
		kms_ops->config_mode(de_base, &vmode);
		GB02FUNC281(dc_base, de_base);

		kms_ops->leave_config(dc_base, de_base);
		gbdev = GB02FUNC183(gb_dev->drm_dev);
		//GB02FUNC1737(gbdev, crtc_id, gbdc_crtc->base.mode.vdisplay,
		//	gbdc_crtc->base.mode.vdisplay);
		gbdc_crtc->mod_flag = MODE_SET_DONE;
		//drm_crtc_vblank_on(crtc);
		gb_printf(KERN_INFO, "%s:end modeset...\n", __func__);
	#ifdef	GB02MAC292
		GB02FUNC556(crtc_id, AUDIO_NEED_RESTART);
	#endif

		mutex_unlock(&gbdc_crtc->gbdc_crtc_lock);

	}
	if (crtc->state->gamma_lut)
		GB02FUNC1007(crtc, old_state);
	drm_crtc_vblank_on(&virt_crtc->base);
	mutex_unlock(&virt_crtc->gbdc_crtc_lock);

}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_crtc_atomic_disable(struct drm_crtc *crtc,
									 struct drm_atomic_state *state)
{
#else
static void gbdc_crtc_atomic_disable(struct drm_crtc *crtc,
									 struct drm_crtc_state *old_state)
{
#endif
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

	gb_printf(KERN_INFO, "%s: %d\n", __func__, __LINE__);
	gb_printf(KERN_INFO, "%s: crtcid %d\n", __func__, gbdc_crtc->crtc_id);

	WARN_ON(gbdc_crtc->event);
	mutex_lock(&gbdc_crtc->gbdc_crtc_lock);
	drm_crtc_vblank_off(crtc);
	gbdc_crtc->is_enable = false;
	GB02FUNC978(crtc, false);
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 3, 0)
	if (!crtc->state->self_refresh_active)
#endif
	GB02FUNC986(crtc, DRM_MODE_DPMS_OFF);
	mutex_unlock(&gbdc_crtc->gbdc_crtc_lock);
	if (crtc->state->event && !crtc->state->active) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock_irq(&crtc->dev->event_lock);

		crtc->state->event = NULL;
	}
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_virt_crtc_atomic_disable(struct drm_crtc *crtc, struct drm_atomic_state *state)
{	
	struct drm_crtc_state *old_state = drm_atomic_get_old_crtc_state(state, crtc);
#else
static void gbdc_virt_crtc_atomic_disable(struct drm_crtc *crtc, struct drm_crtc_state *old_state)
{
#endif
	struct gbdc_crtc_state *gbdc_crtc_state = to_gbdc_crtc_state(old_state);
	struct gbdc_crtc *virt_crtc = GB02FUNC1573(crtc);
	struct gbdc_crtc *gbdc_crtc;
	struct drm_crtc *phy_crtc = NULL;


	gb_printf(KERN_INFO, "%s: virt %d,crtcid %d\n", __func__,
			virt_crtc->virt, virt_crtc->crtc_id);

	mutex_lock(&virt_crtc->gbdc_crtc_lock);

	gbdc_for_each_gbdc_obj(crtc) {
		if (!gbdc_crtc)
			continue;
		gb_printf(KERN_INFO, "%s:phy%d,gbdc_crtc id%d\n", __func__,
			gbdc_crtc->virt, gbdc_crtc->crtc_id);
		phy_crtc = &gbdc_crtc->base;

		gbdc_crtc->is_enable = false;
		GB02FUNC978(phy_crtc, false);
		#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 3, 0)
		if (!crtc->state->self_refresh_active)
		#endif
		GB02FUNC986(phy_crtc, DRM_MODE_DPMS_OFF);

	}
	drm_crtc_vblank_off(crtc);
	virt_crtc->is_enable = false;

	mutex_unlock(&virt_crtc->gbdc_crtc_lock);

	if (crtc->state->event && !crtc->state->active) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock_irq(&crtc->dev->event_lock);
		crtc->state->event = NULL;
	}


}
#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static int gbdc_crtc_atomic_check(struct drm_crtc *crtc,
								  struct drm_atomic_state *atomic_state)
{
	struct drm_crtc_state *state = drm_atomic_get_new_crtc_state(atomic_state, crtc);
#else
static int gbdc_crtc_atomic_check(struct drm_crtc *crtc,
								  struct drm_crtc_state *state)
{
#endif
	gb_printf(KERN_DEBUG, "enter %s: %d\n", __func__, __LINE__);

	if (state->color_mgmt_changed && state->gamma_lut) {
		unsigned int len;

		len = drm_color_lut_size(state->gamma_lut);
		if (len != crtc->gamma_size) {
			gb_printf(KERN_ERR, "Invalid gamma size, got %d expect %d\n", len, crtc->gamma_size);
			return -EINVAL;
		}
	}

	return 0;
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_crtc_atomic_begin(struct drm_crtc *crtc,
								   struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state, crtc);
	struct drm_crtc_state *old_crtc_state = drm_atomic_get_old_crtc_state(state, crtc);

	if (crtc_state->color_mgmt_changed && !crtc_state->active_changed)
		GB02FUNC1004(crtc, old_crtc_state);
}
#else
static void gbdc_crtc_atomic_begin(struct drm_crtc *crtc,
								   struct drm_crtc_state *old_crtc_state)
{
	struct gbdc_crtc *gbdc_crtc;

	gbdc_crtc = GB02FUNC1573(crtc);

	if(!gbdc_crtc->virt) {
		if (old_crtc_state->color_mgmt_changed && !old_crtc_state->active_changed)
			GB02FUNC1004(crtc, old_crtc_state);
	}
	else {
		if (old_crtc_state->color_mgmt_changed && !old_crtc_state->active_changed)
			GB02FUNC1007(crtc, old_crtc_state);
	}
}
#endif

static bool GB02FUNC1098(void __iomem *de_base)
{
	unsigned int irq_status = GB02FUNC730(de_base, GB02MAC580);
	return !!(irq_status & (1 << 12));
}

static void GB02FUNC1101(struct drm_crtc *crtc, void __iomem *de_base)
{
	bool pending;
	int ret;
	struct GB02STR155 *gbdc_dev = GB02FUNC85(crtc->dev->dev_private);
	struct GB02STR39 *gb_dev = GB02FUNC183(gbdc_dev->drm_dev);;

	/*
	 * Spin until frame start interrupt status bit goes low, which means
	 * that interrupt handler was invoked and cleared it. The timeout of
	 * 10 msecs is really too long, but it is just a safety measure if
	 * something goes really wrong. The wait will only happen in the very
	 * unlikely case of a vblank happening exactly at the same time and
	 * shouldn't exceed microseconds range.
	 */
	ret = readx_poll_timeout_atomic(GB02FUNC1098, de_base, pending,
					!pending, 0, 10 * 1000);
	if (ret)
		DRM_DEV_ERROR(gb_dev->dev, "GB vblank IRQ stuck for 10 ms\n");

	synchronize_irq(gb_dev->gb_pcie->irqs[0]);
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_crtc_atomic_flush(struct drm_crtc *crtc,
								   struct drm_atomic_state *state)
{
	struct drm_crtc_state *old_crtc_state = drm_atomic_get_old_crtc_state(state, crtc);
#else
static void gbdc_crtc_atomic_flush(struct drm_crtc *crtc,
								   struct drm_crtc_state *old_crtc_state)
{
#endif
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct drm_atomic_state *old_state = old_crtc_state->state;
	struct drm_plane_state *old_plane_state, *new_plane_state;
	struct drm_plane *plane;
	struct GB02STR249 *dc_config;
	struct GB02STR155 *gb_dev;
	void __iomem *dc_base;
	void __iomem *de_base;
	int crtc_id = 0;
	unsigned long irqflags;
	int i;

	if (gbdc_crtc->need_disable)
		return;

	if (WARN_ON(!gbdc_crtc->is_enable))
		return;

	/*config valid*/
	crtc_id = gbdc_crtc->crtc_id;
	gb_dev = GB02FUNC85(crtc->dev->dev_private);
	dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;
	GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	/*
	* There is a (rather unlikely) possiblity that a vblank interrupt
	* fired before we set the cfg_done bit. To avoid spuriously
	* signalling flip completion we need to wait for it to finish.
	*/
	if (0) GB02FUNC1101(crtc, de_base);

	spin_lock_irqsave(&crtc->dev->event_lock, irqflags);
	if (crtc->state->event) {
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		WARN_ON(gbdc_crtc->event);

		gbdc_crtc->event = crtc->state->event;
		crtc->state->event = NULL;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, irqflags);

	/*TODO: flip queue*/
	for_each_oldnew_plane_in_state(old_state, plane, old_plane_state, new_plane_state, i) {
		if (!old_plane_state->fb)
			continue;
		if (old_plane_state->fb == new_plane_state->fb)
			continue;

		drm_framebuffer_get(old_plane_state->fb);
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		drm_flip_work_queue(&gbdc_crtc->gbdc_fb_unref_work, old_plane_state->fb);
		set_bit(GBDC_PENDING_FB_UNREF, &gbdc_crtc->pending);
	}
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static void gbdc_virt_crtc_atomic_flush(struct drm_crtc *crtc,
								   struct drm_atomic_state *state)
{
	struct drm_crtc_state *old_crtc_state = drm_atomic_get_old_crtc_state(state, crtc);
#else
static void gbdc_virt_crtc_atomic_flush(struct drm_crtc *crtc,
					struct drm_crtc_state *old_crtc_state)
{
#endif
	struct gbdc_crtc *gbdc_crtc = NULL;
	struct gbdc_crtc *virt_crtc = NULL;
	struct drm_atomic_state *old_state = old_crtc_state->state;
	struct gbdc_crtc_state *gbdc_crtc_state = to_gbdc_crtc_state(old_crtc_state);
	struct drm_plane_state *old_plane_state, *new_plane_state;
	struct GB02STR249 *dc_config;
	struct drm_plane *plane;
	struct GB02STR155 *gb_dev;
	void __iomem *dc_base;
	void __iomem *de_base;
	int crtc_id = 0 ,i = 0;
	unsigned long irqflags;

	virt_crtc = GB02FUNC1573(crtc);

	if (virt_crtc->need_disable)
		return;

	gb_printf(KERN_DEBUG, "%s: crtcid %d\n", __func__, virt_crtc->crtc_id);

	gb_dev = GB02FUNC85(crtc->dev->dev_private);

	if ((!virt_crtc->is_enable))
		return;

	gbdc_for_each_gbdc_obj(crtc) {
		if (!gbdc_crtc)
			continue;

		if ((!gbdc_crtc->is_enable))
			continue;

		/*config valid*/
		crtc_id = gbdc_crtc->crtc_id;

		dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
		dc_base = dc_config->dc_base;
		de_base = dc_config->de_base;
		GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	}
	spin_lock_irqsave(&crtc->dev->event_lock, irqflags);
	if (crtc->state->event) {
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		WARN_ON(virt_crtc->event);

		virt_crtc->event = crtc->state->event;
		crtc->state->event = NULL;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, irqflags);

	/*TODO: flip queue*/
	for_each_oldnew_plane_in_state(old_state, plane, old_plane_state, new_plane_state, i) {
		if (!old_plane_state->fb)
			continue;
		if (old_plane_state->fb == new_plane_state->fb)
			continue;

		drm_framebuffer_get(old_plane_state->fb);
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		drm_flip_work_queue(&virt_crtc->gbdc_fb_unref_work, old_plane_state->fb);
		set_bit(GBDC_PENDING_FB_UNREF, &virt_crtc->pending);
	}
}

static const struct drm_crtc_helper_funcs gbdc_crtc_helper_funcs = {
	.dpms = GB02FUNC986,
	.mode_set = GB02FUNC990,
	//.mode_set_base = gbdc_do_mode_set_base,
	.mode_fixup = GB02FUNC997,
	.commit = GB02FUNC993,
	.atomic_check = gbdc_crtc_atomic_check,
	.atomic_enable = gbdc_crtc_atomic_enable,
	.atomic_disable = gbdc_crtc_atomic_disable,
	.atomic_begin = gbdc_crtc_atomic_begin,
	.atomic_flush = gbdc_crtc_atomic_flush,
};

static const struct drm_crtc_helper_funcs gbdc_virt_crtc_helper_funcs = {
	.dpms = GB02FUNC989,
	//.mode_set = GB02FUNC990,
	//.mode_set_base = gbdc_do_mode_set_base,
	.mode_fixup = GB02FUNC999,
	.commit = GB02FUNC993,
	.atomic_check = gbdc_crtc_atomic_check,
	.atomic_enable = gbdc_virt_crtc_atomic_enable,
	.atomic_disable = gbdc_virt_crtc_atomic_disable,
	.atomic_begin = gbdc_crtc_atomic_begin,
	.atomic_flush = gbdc_virt_crtc_atomic_flush,
};

static void GB02FUNC1113(struct drm_crtc *crtc,
									struct drm_crtc_state *state)
{
	struct gbdc_crtc_state *gb_state = NULL;

	gb_printf(KERN_DEBUG, "%s: %d, crtc name %s, driver name:%s\n",
			__func__, __LINE__, crtc->name, crtc->dev->driver->name);
	if (state) {
		gb_state = to_gbdc_crtc_state(state);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
		__drm_atomic_helper_crtc_destroy_state(&gb_state->base);
#else
		__drm_atomic_helper_crtc_destroy_state(crtc, &gb_state->base);
#endif
		kfree(gb_state);
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
static void
__drm_atomic_helper_crtc_reset(struct drm_crtc *crtc,
			       struct drm_crtc_state *crtc_state)
{
	if (crtc_state)
		crtc_state->crtc = crtc;

	crtc->state = crtc_state;
}
#endif

static void GB02FUNC1116(struct drm_crtc *crtc)
{
	struct GB02STR155 *gb_dev;
	struct gbdc_crtc *gb_crtc;
	struct GB02STR249 *dc_config;
	void __iomem *de_base, *dc_base;
	int count = 1000;
	u32 status;

	if (!crtc)
		return;

	/* reset config */
	gb_crtc = GB02FUNC1573(crtc);
	if(gb_crtc->virt)
		return;
	gb_dev = GB02FUNC85(crtc->dev->dev_private);
	dc_config = &gb_dev->pcie_info.dc_config[gb_crtc->crtc_id];
	dc_base = dc_config->dc_base;
	de_base = dc_config->de_base;

	GB02FUNC733(dc_base, GB02MAC694, GB02MAC680 | \
							GB02MAC681 | GB02MAC682);
	while (count) {
		status =
				GB02FUNC730(dc_base, GB02MAC580);
		if ((status & GB02MAC680) == GB02MAC680)
			break;
		/*
		 * entering config mode can take as long as the rendering
		 * of a full frame, hence the long sleep here
		 */
		msleep(1);
		count--;
	}

	WARN(count == 0, "timeout while entering config mode, status:0x%x",
	 status);
}

static void GB02FUNC1120(struct drm_crtc *crtc)
{
	struct gbdc_crtc_state *crtc_state =
		kzalloc(sizeof(*crtc_state), GFP_KERNEL);
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

	gb_printf(KERN_INFO, "%s: %d\n", __func__, __LINE__);

	//GB02FUNC1116(crtc);
	if (crtc->state)
		GB02FUNC1113(crtc, crtc->state);

	if (crtc_state) {
		__drm_atomic_helper_crtc_reset(crtc, &crtc_state->base);
		crtc_state->infinity_crtc_list = gbdc_crtc;
		INIT_LIST_HEAD(&crtc_state->infinity_crtc_list->head);
	}
}

static void GB02FUNC1122(struct drm_flip_work *work, void *val)
{
	struct gbdc_crtc *gbdc_crtc = container_of(work, struct gbdc_crtc, gbdc_fb_unref_work);
	struct drm_framebuffer *fb = val;

	drm_crtc_vblank_put(&gbdc_crtc->base);
	drm_framebuffer_put(fb);
}

static int GB02FUNC1124(struct drm_atomic_state *state,
			    struct drm_crtc *crtc,
			    struct drm_framebuffer *fb,
			    struct drm_pending_vblank_event *event,
			    uint32_t flags)
{
	struct drm_plane *plane = crtc->primary;
	struct drm_plane_state *plane_state;
	struct drm_crtc_state *crtc_state;
	int ret = 0;

	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state)) {
		gb_printf(KERN_ERR, "%s:%d drm_atomic_get_crtc_state fail\n",
				__func__, __LINE__);
		return PTR_ERR(crtc_state);
	}

	crtc_state->event = event;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 18)
	crtc_state->async_flip = flags & DRM_MODE_PAGE_FLIP_ASYNC;
#else
	crtc_state->pageflip_flags = flags;
#endif

	plane_state = drm_atomic_get_plane_state(state, plane);
	if (IS_ERR(plane_state)) {
		gb_printf(KERN_ERR, "%s:%d drm_atomic_get_plane_state fail\n",
				__func__, __LINE__);
		return PTR_ERR(plane_state);
	}

	ret = drm_atomic_set_crtc_for_plane(plane_state, crtc);
	if (ret != 0) {
		gb_printf(KERN_ERR, "%s:%d drm_atomic_set_crtc_for_plane fail\n",
				__func__, __LINE__);
		return ret;
	}
	drm_atomic_set_fb_for_plane(plane_state, fb);

	/* Make sure we don't accidentally do a full modeset. */
	state->allow_modeset = false;
	if (!crtc_state->active) {
		DRM_DEBUG_ATOMIC("[CRTC:%d:%s] disabled, rejecting legacy flip\n",
				 crtc->base.id, crtc->name);
		gb_printf(KERN_ERR, "[CRTC:%d:%s] disabled, rejecting legacy flip\n",
				 crtc->base.id, crtc->name);
		return -EINVAL;
	}

	return ret;
}

/**
 * drm_atomic_helper_page_flip - execute a legacy page flip
 * @crtc: DRM crtc
 * @fb: DRM framebuffer
 * @event: optional DRM event to signal upon completion
 * @flags: flip flags for non-vblank sync'ed updates
 * @ctx: lock acquisition context
 *
 * Provides a default &drm_crtc_funcs.page_flip implementation
 * using the atomic driver interface.
 *
 * Returns:
 * Returns 0 on success, negative errno numbers on failure.
 *
 * See also:
 * drm_atomic_helper_page_flip_target()
 */
static int GB02FUNC1127(struct drm_crtc *crtc,
				struct drm_framebuffer *fb,
				struct drm_pending_vblank_event *event,
				uint32_t flags,
				struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_plane *plane = crtc->primary;
	struct drm_atomic_state *state;
	int ret = 0;
	struct mutex *mutex = NULL;

	state = drm_atomic_state_alloc(plane->dev);
	if (!state) {
		gb_printf(KERN_ERR, "%s:%d drm_atomic_state_alloc fail\n",
				__func__, __LINE__);
		return -ENOMEM;
	}

	state->acquire_ctx = ctx;

	//GB02FUNC1430(plane->dev);
	mutex = GB02FUNC1433(state);
	if (mutex)
		mutex_lock(mutex);

	ret = GB02FUNC1124(state, crtc, fb, event, flags);
	if (ret != 0) {
		if (mutex)
			mutex_unlock(mutex);
		gb_printf(KERN_ERR, "%s:%d GB02FUNC1124 fail\n",
				__func__, __LINE__);
		goto fail;
	}

	ret = drm_atomic_nonblocking_commit(state);
	if (mutex)
		mutex_unlock(mutex);
	//GB02FUNC1431(plane->dev);
	if (ret)
		gb_printf(KERN_ERR, "%s:%d drm_atomic_commit fail\n",
				__func__, __LINE__);
fail:
	drm_atomic_state_put(state);
	return ret;
}

static const struct drm_crtc_funcs gbdc_crtc_funcs = {
	.cursor_set2 = GB02FUNC1056,
	.cursor_move = GB02FUNC1032,
#if (KERNEL_VERSION(5, 12, 0) > LINUX_VERSION_CODE)
	.gamma_set = drm_atomic_helper_legacy_gamma_set,
#endif
	.destroy = drm_crtc_cleanup,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
#endif
	.reset = GB02FUNC1120,
	.atomic_duplicate_state = GB02FUNC1074,
	.atomic_destroy_state = GB02FUNC1113,
	.enable_vblank = GB02FUNC1012,
	.disable_vblank = GB02FUNC1014,
};
static const struct drm_crtc_funcs gbdc_virt_crtc_funcs = {
#if (KERNEL_VERSION(5, 12, 0) > LINUX_VERSION_CODE)
	.gamma_set = drm_atomic_helper_legacy_gamma_set,
#endif
	.destroy = drm_crtc_cleanup,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	.set_config = drm_atomic_helper_set_config,
	.page_flip = GB02FUNC1127,//drm_atomic_helper_page_flip,
#endif
	.reset = GB02FUNC1120,
	.atomic_duplicate_state = GB02FUNC1074,
	.atomic_destroy_state = GB02FUNC1113,
	.enable_vblank = GB02FUNC1023,
	.disable_vblank = GB02FUNC1025,
};
int GB02FUNC1134(struct drm_device *drm_dev, int crtc_id)
{
	struct drm_crtc *crtc, *ctmp;
	struct drm_plane *plane, *tmp;
	struct gbdc_crtc *gbdc_crtc = NULL;

	list_for_each_entry_safe(plane, tmp, &drm_dev->mode_config.plane_list,
				 head)
		GB02FUNC1844(plane);

	list_for_each_entry_safe(crtc, ctmp, &drm_dev->mode_config.crtc_list,
				 head) {
		gbdc_crtc = GB02FUNC1573(crtc);
		//drm_self_refresh_helper_cleanup(crtc);
		drm_crtc_cleanup(crtc);
		drm_flip_work_cleanup(&gbdc_crtc->gbdc_fb_unref_work);
	}

	return 0;
}

static int GB02FUNC1136(struct drm_device *drm_dev, struct GB02STR245 *kms_info,
		struct drm_plane *prime_plane, struct drm_plane *cursor_plane, int idx, bool virt)
{
	struct gbdc_crtc *gb_crtc;
	int ret = 0;
	struct drm_crtc *crtc = NULL;
	struct drm_plane *tmp;
	struct gbdc_plane *gb_plane = NULL;
	static int crtc_grp_id = 0;

	gb_crtc = kzalloc(sizeof(*gb_crtc), GFP_KERNEL);
	if (!gb_crtc)
		return (-ENOMEM);

	gb_crtc->crtc_id = idx;
	gb_crtc->max_cursor_width = 128;
	gb_crtc->max_cursor_height = 128;
	if(!kms_info->crtcs[idx])
		kms_info->crtcs[idx] = gb_crtc;
	gb_crtc->mod_flag = MODE_SET_NONE;
	gb_crtc->pixel_formats = kms_info->plane_cap;
	gb_crtc->num_formats = kms_info->num_plane_cap;
	gb_crtc->is_enable = false;

	gb_crtc->virt = virt;
	gb_crtc->vblank_id = -1;
	pr_info("%s:virt %d,crtcid %d\n",__func__, virt, idx);
	INIT_LIST_HEAD(&gb_crtc->node);
	INIT_LIST_HEAD(&gb_crtc->head);

	if (virt) {
		list_add_tail(&gb_crtc->node, &kms_info->virt_crtc_head);
		gb_crtc->grp_id = crtc_grp_id++;//0 is single,>0 is joint
	}
	drm_flip_work_init(&gb_crtc->gbdc_fb_unref_work, "gbdc_fb_unref",
			   GB02FUNC1122);

	mutex_init(&gb_crtc->gbdc_crtc_lock);
	tasklet_init(&gb_crtc->task_vblank, GB02FUNC1747, (unsigned long)gb_crtc);

	ret = drm_crtc_init_with_planes(drm_dev, &gb_crtc->base, prime_plane, cursor_plane,
					virt ? &gbdc_virt_crtc_funcs : &gbdc_crtc_funcs, NULL);
	if (ret)
		goto err_out;

	drm_mode_crtc_set_gamma_size(&gb_crtc->base, GB02MAC1424);
	drm_crtc_enable_color_mgmt(&gb_crtc->base, 0, true, GB02MAC1424);


	drm_crtc_helper_add(&gb_crtc->base, virt ?
		&gbdc_virt_crtc_helper_funcs : &gbdc_crtc_helper_funcs);

	GB02FUNC1116(&gb_crtc->base);
	GB02FUNC978(&gb_crtc->base, false);

	return 0;

err_out:
	prime_plane->funcs->destroy(prime_plane);

	drm_for_each_plane(tmp, drm_dev) {
		gb_plane = to_gbdc_plane_info(tmp);
		kfree(gb_plane);
	}
	drm_for_each_crtc(crtc, drm_dev) {
		gb_crtc = GB02FUNC1573(crtc);
		kfree(gb_crtc);
	}
	return ret;
}

int GB02FUNC1139(struct drm_device *drm_dev, struct GB02STR245 *kms_info)
{
	unsigned int i, idx;
	struct gbdc_crtc *gb_crtc = NULL;
	struct drm_crtc *crtc = NULL;
	int ret = 0;
	struct gbdc_plane *gb_plane;
	struct drm_plane *prim_plane = NULL;
	//struct gbdc_crtc_state *state;
	enum drm_plane_type type;
	struct drm_plane *tmp, *cursor_plane = NULL;
	int crtc_num = kms_info->num_crtc;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);
	int crtc_id_tmp;

	for(idx = 0; idx < crtc_num ; idx++) {
		if (GB02FUNC1902(gb_type, idx))
			continue;

		crtc_id_tmp = GB02FUNC1904(gb_type, idx);

		for (i = 0; i < kms_info->num_plane; i++) {

			gb_plane = kzalloc(sizeof(*gb_plane), GFP_KERNEL);
			if (!gb_plane)
				return (-ENOMEM);

			gb_plane->plane_id = i;
			gb_plane->crtc_id = idx;//crtc id
			gb_plane->pixel_formats = kms_info->plane_cap;
			gb_plane->num_formats = kms_info->num_plane_cap;
			gb_plane->plane_res = kms_info->plane_res[i];
			INIT_LIST_HEAD(&gb_plane->head);

			type = GB02FUNC1858(i);
			ret = GB02FUNC1860(drm_dev, gb_plane, 1 << crtc_id_tmp, type, i);
			if (ret)
				goto err_plane;

			if(type == DRM_PLANE_TYPE_PRIMARY ){
				prim_plane = &gb_plane->base;
			}
			if(type == DRM_PLANE_TYPE_CURSOR)
				cursor_plane = &gb_plane->base;
		}
		pr_info("prime plane %px \n",prim_plane);
		if(!cursor_plane)
			goto err_plane;

		pr_info("cursor plane %px\n", cursor_plane);
		ret = GB02FUNC1136(drm_dev, kms_info, prim_plane, cursor_plane, idx, false);

	}

	return ret;

err_plane:
	drm_for_each_plane(tmp, drm_dev) {
		gb_plane = to_gbdc_plane_info(tmp);
		kfree(gb_plane);
	}
	drm_for_each_crtc(crtc, drm_dev) {
		gb_crtc = GB02FUNC1573(crtc);
		kfree(gb_crtc);
	}
	return ret;
}
int GB02FUNC1140(struct drm_device *drm_dev, struct GB02STR245 *kms_info ,int vidx)
{
	unsigned int i;
	struct gbdc_crtc *gb_crtc = NULL;
	struct drm_crtc *crtc = NULL;
	int ret = 0;
	struct gbdc_plane *gb_plane;
	struct drm_plane *prim_plane = NULL;
	//struct gbdc_crtc_state *state;
	enum drm_plane_type type;
	struct drm_plane *tmp, *cursor_plane = NULL;
	//int crtc_num = kms_info->num_crtc;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);
	int seqno;

	seqno = GB02FUNC1903(gb_type, vidx);

	pr_info("%s: seqno %d\n",__func__,seqno);

	for (i = 0; i < kms_info->num_plane; i++) {

		gb_plane = kzalloc(sizeof(*gb_plane), GFP_KERNEL);
		if (!gb_plane)
			return (-ENOMEM);

		gb_plane->plane_id = i;
		gb_plane->crtc_id = vidx;//crtc id

		gb_plane->pixel_formats = kms_info->plane_cap;
		gb_plane->num_formats = kms_info->num_plane_cap;
		gb_plane->plane_res = kms_info->plane_res[i];

		gb_plane->virt = true;
		INIT_LIST_HEAD(&gb_plane->head);
		type = GB02FUNC1858(i);
		ret = GB02FUNC1860(drm_dev, gb_plane, 1 << seqno, type, i);
		if (ret)
			goto err_plane;

		if(type == DRM_PLANE_TYPE_PRIMARY )
			prim_plane = &gb_plane->base;

		if(type == DRM_PLANE_TYPE_CURSOR)
			cursor_plane = &gb_plane->base;
	}
		pr_info("%s:prime plane %px ,crtcid%d\n",__func__,prim_plane, vidx);

		//cursor_plane = drm_plane_from_index(drm_dev,DC_PLANE_SMART);
		if(!cursor_plane)
			goto err_plane;
		pr_info("%s:cursor plane %px,crtcid%d\n", __func__,cursor_plane,vidx);

		ret = GB02FUNC1136(drm_dev, kms_info, prim_plane, cursor_plane, vidx, true);

	return ret;

err_plane:
	drm_for_each_plane(tmp, drm_dev) {
		gb_plane = to_gbdc_plane_info(tmp);
		kfree(gb_plane);
	}
	drm_for_each_crtc(crtc, drm_dev) {
		gb_crtc = GB02FUNC1573(crtc);
		kfree(gb_crtc);
	}
	return ret;
}
