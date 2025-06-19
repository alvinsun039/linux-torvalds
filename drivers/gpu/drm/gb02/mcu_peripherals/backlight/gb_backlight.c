// SPDX-License-Identifier: GPL-2.0-only
/*
 * GB backlight control, board code has to setup
 */

#include <linux/version.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include "gb_backlight.h"
#include "ip/gb_dp_dptx.h"
#include "ip/gb_dp.h"

#define GB02MAC14 (0x0003FC08)
#define GB02MAC16 0x37

static struct GB02STR12 *GB02STR154[GB02MAC658];
static u32 reserve_brightness[GB02MAC658];

struct GB02STR12 *GB02FUNC2(int index)
{
	return GB02STR154[index];
}

static void GB02FUNC3(struct dptx *dptx_info, u32 brightness)
{
	int index = dptx_info->index;
	struct GB02STR12 *data = GB02STR154[index];
	char brightness_buf[7] = {0x51, 0x84, 0x3, 0x10, 0x0, 0x0, 0x0}; //DPCD brightness configure CMD
	char temp = 0;

	if (!data || data->last_brightness == brightness)
		return;

	if (dptx_info->is_edp) {
		GB02FUNC528(brightness, data->sysctl_iofunc_base + 0x1000000, GB02MAC14 + index * 4);
	} else {
		if (!data->by_dptx)
			GB02FUNC340(dptx_info, false, true, true, true, GB02MAC16, &temp, 0x1);

		brightness_buf[5] = brightness;
		brightness_buf[6] = 0xa8 ^ brightness;
		GB02FUNC340(dptx_info, false, true, true, false, GB02MAC16, brightness_buf, 0x7);

		if (!data->by_dptx)
			GB02FUNC340(dptx_info, false, true, false, true, GB02MAC16, &temp, 0x1);
	}
	data->by_dptx = false;
	data->last_brightness = brightness;
	reserve_brightness[index] = brightness;
}

static int GB02FUNC8(struct backlight_device *bl)
{
	int brightness = bl->props.brightness;
	struct GB02STR12 *data = bl_get_data(bl);

	if (brightness == 0) {
		bl->props.brightness = data->last_brightness;
		return -EINVAL;
	}

	GB02FUNC3(data->dptx_info, brightness);
	return 0;
}

static const struct backlight_ops gb_backlight_ops = {
	.update_status = GB02FUNC8,
};

int GB02FUNC10(struct GB02STR70 *pcie_info, struct dptx *dptx_info)
{
	struct backlight_properties props;
	struct GB02STR12 *data;
	char backlight_name[20];
	int index = dptx_info->index;

	if (GB02STR154[index])
		return 0;

	GB02STR154[dptx_info->index] = kzalloc(sizeof(struct GB02STR12), GFP_KERNEL);
	if (!GB02STR154[dptx_info->index])
		return -ENOMEM;
	data = GB02STR154[index];

	data->sysctl_iofunc_base = pcie_info->pci_bars[0].mmio;
	data->dptx_info = dptx_info;

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = 100;
	sprintf(backlight_name, "gb_backlight%d", index);
	data->bl = backlight_device_register(backlight_name, dptx_info->dev, data,
					&gb_backlight_ops, &props);

	if (IS_ERR(data->bl)) {
		dev_err(dptx_info->dev, "failed to register backlight\n");
		data->bl = NULL;
		return -ENODEV;
	}
	if (reserve_brightness[index]) {
		data->bl->props.brightness = reserve_brightness[index];
		backlight_update_status(data->bl);
	}

	return 0;
}

void GB02FUNC20(struct dptx *dptx_info)
{
	int index = dptx_info->index;
	struct GB02STR12 *data = GB02STR154[index];

	if (!data)
		return;
	if (data->bl)
		backlight_device_unregister(data->bl);
	kfree(data);
	GB02STR154[index] = NULL;
}
