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
#include <linux/backlight.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_gem.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#include <drm/drm_connector.h>
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
//#include "device/gb_device.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "device/gb_ip.h"
#include "gbdc_drv.h"

static int GB02FUNC743(struct backlight_device *bd)
{
	struct GB02STR155 *gb_dev = bl_get_data(bd);
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;
	int brightness = bd->props.brightness;
	int ret = 0;

	DRM_DEBUG_KMS("updating gb01_backlight, brightness=%d/%d\n",
			 bd->props.brightness, bd->props.max_brightness);

	mutex_lock(&gb_dev->bk_mutex);
	ret = GB02FUNC711(ip_config->hdmi_config_base);
	GB02FUNC707(ip_config->hdmi_config_base,
					 brightness & 0xff);
	GB02FUNC709(ip_config->hdmi_config_base);
	ret = GB02FUNC711(ip_config->hdmi_config_base);
	mutex_unlock(&gb_dev->bk_mutex);

	return 0;
}

static int GB02FUNC746(struct backlight_device *bd)
{
	struct GB02STR155 *gb_dev = bl_get_data(bd);
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;
	int val = 0, ret = 0;

	mutex_lock(&gb_dev->bk_mutex);
	ret = GB02FUNC711(ip_config->hdmi_config_base);
	GB02FUNC704(ip_config->hdmi_config_base);
	ret = GB02FUNC711(ip_config->hdmi_config_base);

	val = GB02FUNC701(ip_config->hdmi_config_base);
	mutex_unlock(&gb_dev->bk_mutex);

	return val;
}

static const struct backlight_ops gb01_backlight_ops = {
	.update_status = GB02FUNC743,
	.get_brightness = GB02FUNC746,
};

int GB02FUNC749(struct GB02STR155 *gb_dev)
{
	struct backlight_properties props;
	struct backlight_device *bd;
	struct drm_device *drm = gb_dev->drm_dev;
#if KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE
	struct drm_connector_list_iter conn_iter;
#endif
	struct drm_connector *connector = NULL;
	struct gbdc_connector *gbdc_conn = NULL;

	mutex_init(&gb_dev->bk_mutex);

#if KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE
	drm_connector_list_iter_begin(drm, &conn_iter);
	drm_for_each_connector_iter(connector, &conn_iter) {
#else
	list_for_each_entry(connector, &drm->mode_config.connector_list, head) {
#endif
		gbdc_conn = to_gbdc_connector(connector);
		if (gbdc_conn->connector_id == CONNECTOR0)
			break;
	}
	if (gbdc_conn)
		connector = &gbdc_conn->base;
#if KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE
	drm_connector_list_iter_end(&conn_iter);
#endif

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = 100;
	bd = backlight_device_register("gb01_backlight", connector->kdev,
			gb_dev, &gb01_backlight_ops, &props);
	if (IS_ERR(bd)) {
		DRM_ERROR("Failed to register backlight: %ld\n", PTR_ERR(bd));
		gb_dev->backlight = NULL;
	return -ENODEV;
	}

	gb_dev->backlight = bd;
	bd->props.brightness = bd->ops->get_brightness(bd);
	backlight_update_status(bd);
	return 0;
}
