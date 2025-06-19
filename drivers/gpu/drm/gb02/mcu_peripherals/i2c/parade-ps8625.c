// SPDX-License-Identifier: GPL-2.0
/*
 * Parade PS8625 eDP/LVDS bridge driver
 *
 * Copyright (C) 2014 Google, Inc.
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

#include <linux/backlight.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/regulator/consumer.h>
#include <generated/uapi/linux/version.h>
#include <drm/drm_atomic_helper.h>
#if KERNEL_VERSION(5, 1, 0) < LINUX_VERSION_CODE
#include <drm/drm_probe_helper.h>
#endif
#include <drm/drm_crtc.h>
#include <drm/drm_bridge.h>
#include <drm/drm_print.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include "stdbool.h"
#include "i2c-gb02-mcu.h"
#include "mcu_peripherals/gpio/pinctrl-gb02-mcu.h"
/* Brightness scale on the Parade chip */
#define GB02MAC1276 0xff

/* Timings taken from the version 1.7 datasheet for the PS8625 */
#define GB02MAC1277 10
#define GB02MAC1278 10000
#define GB02MAC1279 3000
#define GB02MAC1280 30000
#define GB02MAC1281 200
#define GB02MAC1282 10000
#define GB02MAC1283 500

#if ((GB02MAC1279 + GB02MAC1278) > \
	(GB02MAC1280 + GB02MAC1277))
#error "T2.min + T1.max must be less than T2.max + T1.min"
#endif

#define GB02MAC1284 23
//#define PS_8625_GPIO_SLP 39
//#define PS8625_GPIO_PWR 40
#define GB02MAC1285 2
//#define PS8625_DEBUG
#ifdef PS8625_DEBUG
#define GB02MAC1286(arg...) gb_printf(KERN_INFO, "[" GB02_PS8625_DEV_NAME "] " arg)
#else
#define GB02MAC1286(arg...)
#endif
struct GB02STR139 {
	struct drm_connector connector;
	struct i2c_client *client;
	struct drm_bridge bridge;
	struct regulator *v12;
	struct backlight_device *bl;

	u32 max_lane_count;
	u32 lane_count;

	bool enabled;
};

static inline struct GB02STR139 *
bridge_to_ps8625(struct drm_bridge *bridge)
{
	return container_of(bridge, struct GB02STR139, bridge);
}

static inline struct GB02STR139 *
connector_to_ps8625(struct drm_connector *connector)
{
	return container_of(connector, struct GB02STR139, connector);
}

static int GB02FUNC996(struct i2c_client *client, u8 page, u8 reg, u8 val)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	u8 data[] = {reg, val};

	msg.addr = client->addr + page;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	ret = i2c_transfer(adap, &msg, 1);
	if (ret != 1)
		pr_warn("PS8625 I2C write (0x%02x,0x%02x,0x%02x) failed: %d\n",
				client->addr + page, reg, val, ret);
	return !(ret == 1);
}

static int GB02FUNC1002(struct GB02STR139 *ps8625)
{
	struct i2c_client *cl = ps8625->client;
	int err = 0;

	/* HPD low */
	err = GB02FUNC996(cl, 0x02, 0xa1, 0x01);
	if (err)
		goto error;

	/* SW setting: [1:0] SW output 1.2V voltage is lower to 96% */
	err = GB02FUNC996(cl, 0x04, 0x14, 0x01);
	if (err)
		goto error;

	/* RCO SS setting: [5:4] = b01 0.5%, b10 1%, b11 1.5% */
	err = GB02FUNC996(cl, 0x04, 0xe3, 0x20);
	if (err)
		goto error;

	/* [7] RCO SS enable */
	err = GB02FUNC996(cl, 0x04, 0xe2, 0x80);
	if (err)
		goto error;

	/* RPHY Setting
	 * [3:2] CDR tune wait cycle before measure for fine tune
	 * b00: 1us b01: 0.5us b10:2us, b11: 4us
	 */
	err = GB02FUNC996(cl, 0x04, 0x8a, 0x0c);
	if (err)
		goto error;

	/* [3] RFD always on */
	err = GB02FUNC996(cl, 0x04, 0x89, 0x08);
	if (err)
		goto error;

	/* CTN lock in/out: 20000ppm/80000ppm. Lock out 2 times. */
	err = GB02FUNC996(cl, 0x04, 0x71, 0x2d);
	if (err)
		goto error;

	/* 2.7G CDR settings: NOF=40LSB for HBR CDR  setting */
	err = GB02FUNC996(cl, 0x04, 0x7d, 0x07);
	if (err)
		goto error;

	/* [1:0] Fmin=+4bands */
	err = GB02FUNC996(cl, 0x04, 0x7b, 0x00);
	if (err)
		goto error;

	/* [7:5] DCO_FTRNG=+-40% */
	err = GB02FUNC996(cl, 0x04, 0x7a, 0xfd);
	if (err)
		goto error;

	/* 1.62G CDR settings: [5:2]NOF=64LSB [1:0]DCO scale is 2/5 */
	err = GB02FUNC996(cl, 0x04, 0xc0, 0x12);
	if (err)
		goto error;

	/* Gitune=-37% */
	err = GB02FUNC996(cl, 0x04, 0xc1, 0x92);
	if (err)
		goto error;

	/* Fbstep=100% */
	err = GB02FUNC996(cl, 0x04, 0xc2, 0x1c);
	if (err)
		goto error;

	/* [7] LOS signal disable */
	err = GB02FUNC996(cl, 0x04, 0x32, 0x80);
	if (err)
		goto error;

	/* RPIO Setting: [7:4] LVDS driver bias current : 75% (250mV swing) */
	err = GB02FUNC996(cl, 0x04, 0x00, 0xb0);
	if (err)
		goto error;

	/* [7:6] Right-bar GPIO output strength is 8mA */
	err = GB02FUNC996(cl, 0x04, 0x15, 0x40);
	if (err)
		goto error;

	/* EQ Training State Machine Setting, RCO calibration start */
	err = GB02FUNC996(cl, 0x04, 0x54, 0x10);
	if (err)
		goto error;

	/* Logic, needs more than 10 I2C command */
	/* [4:0] MAX_LANE_COUNT set to max supported lanes */
	err = GB02FUNC996(cl, 0x01, 0x02, 0x80 | ps8625->max_lane_count);
	if (err)
		goto error;

	/* [4:0] LANE_COUNT_SET set to chosen lane count */
	err = GB02FUNC996(cl, 0x01, 0x21, 0x80 | ps8625->lane_count);
	if (err)
		goto error;

	err = GB02FUNC996(cl, 0x00, 0x52, 0x20);
	if (err)
		goto error;

	/* HPD CP toggle enable */
	err = GB02FUNC996(cl, 0x00, 0xf1, 0x03);
	if (err)
		goto error;

	err = GB02FUNC996(cl, 0x00, 0x62, 0x41);
	if (err)
		goto error;

	/* Counter number, add 1ms counter delay */
	err = GB02FUNC996(cl, 0x00, 0xf6, 0x01);
	if (err)
		goto error;

	/* [6]PWM function control by DPCD0040f[7], default is PWM block */
	err = GB02FUNC996(cl, 0x00, 0x77, 0x06);
	if (err)
		goto error;

	/* 04h Adjust VTotal toleranceto fix the 30Hz no display issue */
	err = GB02FUNC996(cl, 0x00, 0x4c, 0x04);
	if (err)
		goto error;

	/* DPCD00400='h00, Parade OUI ='h001cf8 */
	err = GB02FUNC996(cl, 0x01, 0xc0, 0x00);
	if (err)
		goto error;

	/* DPCD00401='h1c */
	err = GB02FUNC996(cl, 0x01, 0xc1, 0x1c);
	if (err)
		goto error;

	/* DPCD00402='hf8 */
	err = GB02FUNC996(cl, 0x01, 0xc2, 0xf8);
	if (err)
		goto error;

	/* DPCD403~408 = ASCII code, D2SLV5='h4432534c5635 */
	err = GB02FUNC996(cl, 0x01, 0xc3, 0x44);
	if (err)
		goto error;

	/* DPCD404 */
	err = GB02FUNC996(cl, 0x01, 0xc4, 0x32);
	if (err)
		goto error;

	/* DPCD405 */
	err = GB02FUNC996(cl, 0x01, 0xc5, 0x53);
	if (err)
		goto error;

	/* DPCD406 */
	err = GB02FUNC996(cl, 0x01, 0xc6, 0x4c);
	if (err)
		goto error;

	/* DPCD407 */
	err = GB02FUNC996(cl, 0x01, 0xc7, 0x56);
	if (err)
		goto error;

	/* DPCD408 */
	err = GB02FUNC996(cl, 0x01, 0xc8, 0x35);
	if (err)
		goto error;

	/* DPCD40A, Initial Code major revision '01' */
	err = GB02FUNC996(cl, 0x01, 0xca, 0x01);
	if (err)
		goto error;

	/* DPCD40B, Initial Code minor revision '05' */
	err = GB02FUNC996(cl, 0x01, 0xcb, 0x05);
	if (err)
		goto error;

	if (ps8625->bl) {
		/* DPCD720, internal PWM */
		err = GB02FUNC996(cl, 0x01, 0xa5, 0xa0);
		if (err)
			goto error;

		/* FFh for 100% brightness, 0h for 0% brightness */
		err = GB02FUNC996(cl, 0x01, 0xa7,
						 ps8625->bl->props.brightness);
		if (err)
			goto error;
	} else {
		/* DPCD720, external PWM */
		err = GB02FUNC996(cl, 0x01, 0xa5, 0x80);
		if (err)
			goto error;
	}

	/* Set LVDS output as 6bit-VESA mapping, single LVDS channel */
	err = GB02FUNC996(cl, 0x01, 0xcc, 0x13);
	if (err)
		goto error;

	/* Enable SSC set by register */
	err = GB02FUNC996(cl, 0x02, 0xb1, 0x20);
	if (err)
		goto error;

	/* Set SSC enabled and +/-1% central spreading */
	err = GB02FUNC996(cl, 0x04, 0x10, 0x16);
	if (err)
		goto error;

	/* Logic end */
	/* MPU Clock source: LC => RCO */
	err = GB02FUNC996(cl, 0x04, 0x59, 0x60);
	if (err)
		goto error;

	/* LC -> RCO */
	err = GB02FUNC996(cl, 0x04, 0x54, 0x14);
	if (err)
		goto error;

	/* HPD high */
	err = GB02FUNC996(cl, 0x02, 0xa1, 0x91);

error:
	return err ? -EIO : 0;
}

static int GB02FUNC1027(struct backlight_device *bl)
{
	struct GB02STR139 *ps8625 = dev_get_drvdata(&bl->dev);
	int ret, brightness = bl->props.brightness;

	if (bl->props.power != FB_BLANK_UNBLANK ||
		bl->props.state & (BL_CORE_SUSPENDED | BL_CORE_FBBLANK))
		brightness = 0;

	if (!ps8625->enabled)
		return -EINVAL;

	ret = GB02FUNC996(ps8625->client, 0x01, 0xa7, brightness);

	return ret;
}

static const struct backlight_ops ps8625_backlight_ops = {
	.update_status	= GB02FUNC1027,
};

static void GB02FUNC1029(struct drm_bridge *bridge)
{
	struct GB02STR139 *ps8625 = bridge_to_ps8625(bridge);
	int ret;

	if (ps8625->enabled)
		return;

	GB02FUNC312(GB02MAC1284, 0);

	if (ps8625->v12) {
		ret = regulator_enable(ps8625->v12);
		if (ret)
			DRM_ERROR("fails to enable ps8625->v12");
	}
	//gpiod_set_value(ps8625->gpio_slp, 1);

	/*
	 * T1 is the range of time that it takes for the power to rise after we
	 * enable the lcd/ps8625 fet. T2 is the range of time in which the
	 * data sheet specifies we should deassert the reset pin.
	 *
	 * If it takes T1.max for the power to rise, we need to wait atleast
	 * T2.min before deasserting the reset pin. If it takes T1.min for the
	 * power to rise, we need to wait at most T2.max before deasserting the
	 * reset pin.
	 */
	usleep_range(GB02MAC1279 + GB02MAC1278,
				 GB02MAC1280 + GB02MAC1277);

	GB02FUNC312(GB02MAC1284, 1);

	/* wait 20ms after RST high */
	usleep_range(20000, 30000);

	ret = GB02FUNC1002(ps8625);
	if (ret) {
		DRM_ERROR("Failed to send config to bridge (%d)\n", ret);
		return;
	}

	ps8625->enabled = true;
}

static void GB02FUNC1037(struct drm_bridge *bridge)
{

}

static void GB02FUNC1038(struct drm_bridge *bridge)
{
	msleep(GB02MAC1281);
}

static void GB02FUNC1039(struct drm_bridge *bridge)
{
	struct GB02STR139 *ps8625 = bridge_to_ps8625(bridge);

	if (!ps8625->enabled)
		return;

	ps8625->enabled = false;

	/*
	 * This doesn't matter if the regulators are turned off, but something
	 * else might keep them on. In that case, we want to assert the slp gpio
	 * to lower power.
	 */
	//gpiod_set_value(ps8625->gpio_slp, 0);

	if (ps8625->v12)
		regulator_disable(ps8625->v12);

	/*
	 * Sleep for at least the amount of time that it takes the power rail to
	 * fall to prevent asserting the rst gpio from doing anything.
	 */
	usleep_range(GB02MAC1282,
				 2 * GB02MAC1282);
	GB02FUNC312(GB02MAC1284, 0);

	msleep(GB02MAC1283);
}
static const struct drm_connector_funcs ps8625_connector_funcs = {
	//.fill_modes = drm_helper_probe_single_connector_modes,
	//.destroy = drm_connector_cleanup,
	//.reset = drm_atomic_helper_connector_reset,
	//.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	//.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
static int ps8625_attach(struct drm_bridge *bridge)
#else
static int ps8625_attach(struct drm_bridge *bridge,
						 enum drm_bridge_attach_flags flags)
#endif
{
	struct GB02STR139 *ps8625 = bridge_to_ps8625(bridge);
	int ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
	if (flags & DRM_BRIDGE_ATTACH_NO_CONNECTOR) {
		DRM_ERROR("Fix bridge driver to make connector optional!");
		return -EINVAL;
	}
#endif

	if (!bridge->encoder) {
		DRM_ERROR("Parent encoder object not found");
		return -ENODEV;
	}

	ps8625->connector.polled = DRM_CONNECTOR_POLL_HPD;
	ret = drm_connector_init(bridge->dev, &ps8625->connector,
							 &ps8625_connector_funcs, DRM_MODE_CONNECTOR_LVDS);
	if (ret) {
		DRM_ERROR("Failed to initialize connector with drm\n");
		return ret;
	}
	drm_connector_register(&ps8625->connector);
	drm_connector_attach_encoder(&ps8625->connector,
								 bridge->encoder);
//	drm_helper_hpd_irq_event(ps8625->connector.dev);

	return ret;
}

static const struct drm_bridge_funcs ps8625_bridge_funcs = {
	.pre_enable = GB02FUNC1029,
	.enable = GB02FUNC1037,
	.disable = GB02FUNC1038,
	.post_disable = GB02FUNC1039,
	.attach = ps8625_attach,
};

int GB02FUNC1046(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct GB02STR139 *ps8625;
	int ret;

	ps8625 = devm_kzalloc(dev, sizeof(*ps8625), GFP_KERNEL);
	if (!ps8625)
		return -ENOMEM;
	ps8625->client = client;

	ps8625->v12 = devm_regulator_get(dev, "vdd12");
	if (IS_ERR(ps8625->v12)) {
		GB02MAC1286("no 1.2v regulator found for PS8625\n");
		ps8625->v12 = NULL;
	}
#if 0
	ps8625->gpio_slp = gpio_to_desc(PS_8625_GPIO_SLP);
	if (IS_ERR(ps8625->gpio_slp)) {
		ret = PTR_ERR(ps8625->gpio_slp);
		dev_err(dev, "cannot get gpio_slp %d\n", ret);
		return ret;
	}
#endif
	/*
	 * Assert the reset pin high to avoid the bridge being
	 * initialized prematurely
	 */
	ret = GB02FUNC295(GB02MAC1284);
	if (ret) {
		dev_err(dev, "ps8625 gpio request failed %d\n", ret);
		return ret;
	}

	ps8625->max_lane_count = GB02MAC1285;
	ps8625->lane_count = ps8625->max_lane_count;
	ps8625->bl = backlight_device_register("ps8625-backlight", dev, 
			ps8625, &ps8625_backlight_ops, NULL);
	if (IS_ERR(ps8625->bl)) {
		DRM_ERROR("failed to register backlight\n");
		ret = PTR_ERR(ps8625->bl);
		ps8625->bl = NULL;
		return ret;
	}
	ps8625->bl->props.max_brightness = GB02MAC1276;
	ps8625->bl->props.brightness = GB02MAC1276;
	ps8625->bridge.funcs = &ps8625_bridge_funcs;
	drm_bridge_add(&ps8625->bridge);
	i2c_set_clientdata(client, ps8625);

	return 0;
}

void GB02FUNC1051(struct i2c_client *client)
{
	struct GB02STR139 *ps8625 = i2c_get_clientdata(client);

	backlight_device_unregister(ps8625->bl);
	drm_bridge_remove(&ps8625->bridge);
	i2c_unregister_device(client);
}


