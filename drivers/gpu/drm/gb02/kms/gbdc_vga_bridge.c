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
#include <linux/version.h>
#include <linux/module.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic_helper.h>
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
#if (defined CONFIG_CENTOS && !defined SYS_CENTOS7_COMPILE_ENV) \
    || LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#include <drm/drm_probe_helper.h>
#endif
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "gbdc_mm.h"
#include "gbdc_drv.h"
#include "gbdc_connector.h"
#include "gbdc_vga_bridge.h"

#undef GET_VGA_EDID

static inline struct GB02STR243 *
drm_bridge_to_dumb_vga(struct drm_bridge *bridge)
{
	return container_of(bridge, struct GB02STR243, bridge);
}

static int GB02FUNC1867(struct drm_connector *connector)
{
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	struct GB02STR243 *vga = (struct GB02STR243 *)(&gbdc_connector->vga);
	struct edid *edid;
	int ret;

	if (IS_ERR(vga->ddc))
		goto fallback;

#ifdef GET_VGA_EDID
	edid = drm_get_edid(connector, vga->ddc);
#else
	edid = NULL;
#endif
	if (!edid) {
		DRM_INFO("EDID readout failed, falling back to standard modes\n");
		goto fallback;
	}

#if (!(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	defined SYS_CENTOS7_COMPILE_ENV) \
	|| (KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE &&\
	!defined CONFIG_CENTOS_OS)
	drm_mode_connector_update_edid_property(connector, edid);
#else
	drm_connector_update_edid_property(connector, edid);
#endif
	ret = drm_add_edid_modes(connector, edid);
	kfree(edid);
	return ret;

fallback:
	/*
	 * In case we cannot retrieve the EDIDs (broken or missing i2c
	 * bus), fallback on the XGA standards
	 */
	ret = drm_add_modes_noedid(connector, 1920, 1200);

	/* And prefer a mode pretty much anyone can handle */
	drm_set_preferred_mode(connector, 1024, 768);

	return ret;
}

static int GB02FUNC1868(struct drm_connector *connector,
				struct drm_display_mode *mode)
{
	return MODE_OK;
}

static struct drm_encoder *GB02FUNC1869(struct drm_connector
					*connector)
{
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);

	gb_printf(KERN_INFO, "@@@@@@@@@@@@@@@@@@@@@@@@@@@ %s: %d return NULL\n",
			__func__, __LINE__);
	return gbdc_connector->bridge->encoder;
}

static const struct drm_connector_helper_funcs dumb_vga_con_helper_funcs = {
	.get_modes	= GB02FUNC1867,
	.mode_valid = GB02FUNC1868,
	.best_encoder = GB02FUNC1869,
};

/*
 * TODO:
 * Because it's not safe to get EDID for vga monitor, and thers isn't a
 * good way to judge whether the VGA monitor is connected, we assume that
 * VGA is always connected. What's more, there may be a bug when dual
 * monitors are both used.
 */
static enum drm_connector_status
dumb_vga_connector_detect(struct drm_connector *connector, bool force)
{
	int conn_id;
	unsigned int conn_stat = 0;
	struct GB02STR155 *gb_dev =
			 GB02FUNC85(connector->dev->dev_private);
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);

#ifdef GET_VGA_EDID
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	struct GB02STR243 *vga = (struct GB02STR243 *)(&gbdc_connector->vga);
#endif

	/*
	 * Even if we have an I2C bus, we can't assume that the cable
	 * is disconnected if drm_probe_ddc fails. Some cables don't
	 * wire the DDC pins, or the I2C bus might not be working at
	 * all.
	 */
#ifdef GET_VGA_EDID
	if (!IS_ERR(vga->ddc) && drm_probe_ddc(vga->ddc))
		return connector_status_connected;
#endif
	conn_id = gbdc_connector->connector_id;

	if (modeset_ops->connector_state)
		conn_stat = modeset_ops->connector_state(
				&gb_dev->pcie_info.ip_config, conn_id);
	conn_stat = conn_stat ?	connector_status_connected :
				connector_status_disconnected;

	gb_printf(KERN_INFO, "gbdc crtc%d: %s\n", conn_id, conn_stat ==
		connector_status_connected ? "connected" : "disconnected");
	return conn_stat;
}

static void GB02FUNC1870(struct drm_connector *connector)
{
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	struct GB02STR243 *vga = (struct GB02STR243 *)(&gbdc_connector->vga);

	GB02FUNC1875(vga);
	drm_connector_cleanup(connector);
	kfree(gbdc_connector);
}

static int
gbdc_vga_probe_single_connector_modes(struct drm_connector *connector,
	uint32_t maxX, uint32_t maxY)
{
	return drm_helper_probe_single_connector_modes(connector,
			1920, 1080);
}

static const struct drm_connector_funcs dumb_vga_con_funcs = {
	.dpms			= drm_helper_connector_dpms,
	.detect			= dumb_vga_connector_detect,
	.fill_modes		= gbdc_vga_probe_single_connector_modes,
	.destroy		= GB02FUNC1870,
	.reset			= drm_atomic_helper_connector_reset,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
static int dumb_vga_attach(struct drm_bridge *bridge)
#else
static int dumb_vga_attach(struct drm_bridge *bridge,
		enum drm_bridge_attach_flags flags)
#endif
{
	struct GB02STR243 *vga = drm_bridge_to_dumb_vga(bridge);
	int ret = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
	if (flags & DRM_BRIDGE_ATTACH_NO_CONNECTOR) {
		DRM_ERROR("Fix bridge driver to make connector optional!");
		return -EINVAL;
	}
#endif
	if (!bridge->encoder) {
		DRM_ERROR("Missing encoder\n");
		return -ENODEV;
	}

	drm_connector_helper_add(vga->connector,
				 &dumb_vga_con_helper_funcs);
	ret = drm_connector_init(bridge->dev, vga->connector,
				 &dumb_vga_con_funcs, DRM_MODE_CONNECTOR_VGA);
	if (ret) {
		DRM_ERROR("Failed to initialize connector\n");
		return ret;
	}
	drm_connector_register(vga->connector);
	vga->connector->polled = DRM_CONNECTOR_POLL_HPD;

#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE
	drm_connector_attach_encoder(vga->connector,
					bridge->encoder);
#else
	drm_mode_connector_attach_encoder(vga->connector,
					bridge->encoder);
#endif

	return 0;
}

static void GB02FUNC1871(struct drm_bridge *bridge)
{

}

static void GB02FUNC1872(struct drm_bridge *bridge)
{

}

static const struct drm_bridge_funcs dumb_vga_bridge_funcs = {
	.attach		= dumb_vga_attach,
	.enable		= GB02FUNC1871,
	.disable	= GB02FUNC1872,
};

static struct i2c_adapter *GB02FUNC1873(void)
{
    /* not realized now */
	return ERR_PTR(-ENODEV);
}

int GB02FUNC1874(struct GB02STR243 *vga)
{
	if (!vga)
		return -ENOMEM;

	vga->ddc = GB02FUNC1873();
	if (IS_ERR(vga->ddc)) {
		if (PTR_ERR(vga->ddc) == -ENODEV) {
			gb_printf(KERN_ERR, "No i2c specified. Disabling EDID readout\n");
		} else {
			gb_printf(KERN_ERR, "Couldn't retrieve i2c bus\n");
			return PTR_ERR(vga->ddc);
		}
	}

	vga->bridge.funcs = &dumb_vga_bridge_funcs;

	drm_bridge_add(&vga->bridge);

	return 0;
}

int GB02FUNC1875(struct GB02STR243 *vga)
{
	drm_bridge_remove(&vga->bridge);

	if (!IS_ERR(vga->ddc))
		i2c_put_adapter(vga->ddc);

	return 0;
}
