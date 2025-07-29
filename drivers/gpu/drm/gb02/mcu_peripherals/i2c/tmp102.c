// SPDX-License-Identifier: GPL-2.0
/* Texas Instruments TMP102 SMBus temperature sensor driver
 *
 * Copyright (C) 2010 Steven King <sfking@fdwdc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/regmap.h>
#include "i2c-gb02-mcu.h"
#include "tmp102.h"

/* convert left adjusted 13-bit TMP102 register value to milliCelsius */
static inline int GB02FUNC1062(s16 val)
{
	return ((val & ~0x01) * 1000) / 128;
}

/* convert milliCelsius to left adjusted 13-bit TMP102 register value */
static inline u16 GB02FUNC1065(int val)
{
	return (val * 128) / 1000;
}

static int GB02FUNC1067(struct device *dev, enum hwmon_sensor_types type,
					   u32 attr, int channel, long *temp)
{
	struct tmp102 *tmp102 = dev_get_drvdata(dev);
	unsigned int regval;
	int err, reg;

	switch (attr) {
	case hwmon_temp_input:
		/* Is it too early to return a conversion ? */
		if (time_before(jiffies, tmp102->ready_time)) {
			dev_dbg(dev, "%s: Conversion not ready yet..\n", __func__);
			return -EAGAIN;
		}
		reg = GB02MAC1289;
		break;
	case hwmon_temp_max_hyst:
		reg = GB02MAC1304;
		break;
	case hwmon_temp_max:
		reg = GB02MAC1306;
		break;
	default:
		return -EOPNOTSUPP;
	}

	err = regmap_read(tmp102->regmap, reg, &regval);
	if (err < 0)
		return err;
	*temp = GB02FUNC1062(regval);

	return 0;
}

static int GB02FUNC1070(struct device *dev, enum hwmon_sensor_types type,
						u32 attr, int channel, long temp)
{
	struct tmp102 *tmp102 = dev_get_drvdata(dev);
	int reg;

	switch (attr) {
	case hwmon_temp_max_hyst:
		reg = GB02MAC1304;
		break;
	case hwmon_temp_max:
		reg = GB02MAC1306;
		break;
	default:
		return -EOPNOTSUPP;
	}

	temp = clamp_val(temp, -256000, 255000);
	return regmap_write(tmp102->regmap, reg, GB02FUNC1065(temp));
}

static umode_t GB02FUNC1075(const void *data, enum hwmon_sensor_types type,
								 u32 attr, int channel)
{
	if (type != hwmon_temp)
		return 0;

	switch (attr) {
	case hwmon_temp_input:
		return 0444;
	case hwmon_temp_max_hyst:
	case hwmon_temp_max:
		return 0444 | 0200;
	default:
		return 0;
	}
}

static u32 tmp102_chip_config[] = {
	HWMON_C_REGISTER_TZ,
	0
};

static const struct hwmon_channel_info tmp102_chip = {
	.type = hwmon_chip,
	.config = tmp102_chip_config,
};

static u32 tmp102_temp_config[] = {
	HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_MAX_HYST,
	0
};

static const struct hwmon_channel_info tmp102_temp = {
	.type = hwmon_temp,
	.config = tmp102_temp_config,
};

static const struct hwmon_channel_info *tmp102_info[] = {
	&tmp102_chip,
	&tmp102_temp,
	NULL
};

static const struct hwmon_ops tmp102_hwmon_ops = {
	.is_visible = GB02FUNC1075,
	.read = GB02FUNC1067,
	.write = GB02FUNC1070,
};

static const struct hwmon_chip_info tmp102_chip_info = {
	.ops = &tmp102_hwmon_ops,
	.info = tmp102_info,
};

static void GB02FUNC1080(void *data)
{
	struct tmp102 *tmp102 = data;

	regmap_write(tmp102->regmap, GB02MAC1290, tmp102->config_orig);
}

static bool GB02FUNC1081(struct device *dev, unsigned int reg)
{
	return reg != GB02MAC1289;
}

static bool GB02FUNC1082(struct device *dev, unsigned int reg)
{
	return reg == GB02MAC1289;
}

const struct regmap_config tmp102_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
	.max_register = GB02MAC1306,
	.writeable_reg = GB02FUNC1081,
	.volatile_reg = GB02FUNC1082,
	.val_format_endian = REGMAP_ENDIAN_BIG,
	.cache_type = REGCACHE_RBTREE,
	.use_single_rw = true,
};

static int GB02FUNC1083(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct device *hwmon_dev;
	struct tmp102 *tmp102;
	unsigned int regval;
	int err;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_WORD_DATA)) {
		dev_err(dev,
				"adapter doesn't support SMBus word transactions\n");
		return -ENODEV;
	}

	tmp102 = devm_kzalloc(dev, sizeof(*tmp102), GFP_KERNEL);
	if (!tmp102)
		return -ENOMEM;

	i2c_set_clientdata(client, tmp102);

	tmp102->regmap = devm_regmap_init_i2c(client, &tmp102_regmap_config);
	if (IS_ERR(tmp102->regmap))
		return PTR_ERR(tmp102->regmap);

	err = regmap_read(tmp102->regmap, GB02MAC1290, &regval);
	if (err < 0) {
		dev_err(dev, "error reading config register\n");
		return err;
	}

	if ((regval & ~GB02MAC1307) !=
		(GB02MAC1296 | GB02MAC1297)) {
		dev_err(dev, "unexpected config register value\n");
		return -ENODEV;
	}

	tmp102->config_orig = regval;

	err = devm_add_action_or_reset(dev, GB02FUNC1080, tmp102);
	if (err)
		return err;

	regval &= ~GB02MAC1308;
	regval |= GB02MAC1309;

	err = regmap_write(tmp102->regmap, GB02MAC1290, regval);
	if (err < 0) {
		dev_err(dev, "error writing config register\n");
		return err;
	}

	/*
	 * Mark that we are not ready with data until the first
	 * conversion is complete
	 */
	tmp102->ready_time = jiffies + msecs_to_jiffies(GB02MAC1310);

	hwmon_dev = devm_hwmon_device_register_with_info(dev, client->name,
				tmp102,
				&tmp102_chip_info,
				NULL);
	if (IS_ERR(hwmon_dev)) {
		dev_dbg(dev, "unable to register hwmon device\n");
		return PTR_ERR(hwmon_dev);
	}

	return 0;
}

int GB02FUNC1088(struct GB02STR133 *gb02_i2c)
{
	struct i2c_board_info tmp102_board_info = {
		I2C_BOARD_INFO(TMP102_DEVICE_NAME, GB02MAC1313),
	};
	if (gb02_i2c->pdev->id != GB02MAC1314)
		return 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	gb02_i2c->tmp102_i2c_client = i2c_new_device(&gb02_i2c->adapter.adap, &tmp102_board_info);
#else
	gb02_i2c->tmp102_i2c_client = i2c_new_client_device(&gb02_i2c->adapter.adap, &tmp102_board_info);
#endif
	return GB02FUNC1083(gb02_i2c->tmp102_i2c_client);
}

int GB02FUNC1089(struct GB02STR133 *gb02_i2c)
{
	struct i2c_client *client = gb02_i2c->tmp102_i2c_client;

	if (gb02_i2c->pdev->id > GB02MAC1314)
		return 0;
	i2c_unregister_device(client);
	return 0;
}
