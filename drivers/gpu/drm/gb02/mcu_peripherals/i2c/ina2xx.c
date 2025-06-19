// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for Texas Instruments INA219, INA226 power monitor chips
 *
 * INA219:
 * Zero Drift Bi-Directional Current/Power Monitor with I2C Interface
 * Datasheet: http://www.ti.com/product/ina219
 *
 * INA220:
 * Bi-Directional Current/Power Monitor with I2C Interface
 * Datasheet: http://www.ti.com/product/ina220
 *
 * INA226:
 * Bi-Directional Current/Power Monitor with I2C Interface
 * Datasheet: http://www.ti.com/product/ina226
 *
 * INA230:
 * Bi-directional Current/Power Monitor with I2C Interface
 * Datasheet: http://www.ti.com/product/ina230
 *
 * Copyright (C) 2012 Lothar Felten <lothar.felten@gmail.com>
 * Thanks to Jan Volkering
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/util_macros.h>
#include <linux/regmap.h>
#include "i2c-gb02-mcu.h"
#include "ina2xx.h"

//#define INA2XXX_DEBUG
#ifdef INA2XXX_DEBUG
#define GB02MAC1189(arg...) gb_printf(KERN_INFO, "[" GB02_INA2XXX_DEV_NAME "] " arg)
#else
#define GB02MAC1189(arg...)
#endif

static struct regmap_config ina2xx_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
};
static const struct GB02STR136 GB02STR136[] = {
	[ina219] = {
		.config_default = INA219_CONFIG_DEFAULT,
		.calibration_value = 4096,
		.registers = GB02MAC1228,
		.shunt_div = 100,
		.bus_voltage_shift = 3,
		.bus_voltage_lsb = 4000,
		.power_lsb_factor = 20,
	},
	[ina226] = {
		.config_default = INA226_CONFIG_DEFAULT,
		.calibration_value = 2048,
		.registers = GB02MAC1230,
		.shunt_div = 400,
		.bus_voltage_shift = 0,
		.bus_voltage_lsb = 1250,
		.power_lsb_factor = 25,
	},
};

/*
 * Available averaging rates for ina226. The indices correspond with
 * the bit values expected by the chip (according to the ina226 datasheet,
 * table 3 AVG bit settings, found at
 * http://www.ti.com/lit/ds/symlink/ina226.pdf.
 */
static const int ina226_avg_tab[] = { 1, 4, 16, 64, 128, 256, 512, 1024 };

static int GB02FUNC941(u16 config)
{
	int avg = ina226_avg_tab[GB02MAC1244(config)];

	/*
	 * Multiply the total conversion time by the number of averages.
	 * Return the result in milliseconds.
	 */
	return DIV_ROUND_CLOSEST(avg * GB02MAC1255, 1000);
}

/*
 * Return the new, shifted AVG field value of CONFIG register,
 * to use with regmap_update_bits
 */
static u16 GB02FUNC942(int interval)
{
	int avg, avg_bits;

	avg = DIV_ROUND_CLOSEST(interval * 1000,
							GB02MAC1255);
	avg_bits = find_closest(avg, ina226_avg_tab,
							ARRAY_SIZE(ina226_avg_tab));

	return GB02MAC1246(avg_bits);
}

/*
 * Calibration register is set to the best value, which eliminates
 * truncation errors on calculating current register in hardware.
 * According to datasheet (eq. 3) the best values are 2048 for
 * ina226 and 4096 for ina219. They are hardcoded as calibration_value.
 */
static int GB02FUNC943(struct GB02STR137 *data)
{
	GB02MAC1189("ina_calibrate\n");
	return regmap_write(data->regmap, GB02MAC1219,
						data->config->calibration_value);
}

/*
 * Initialize the configuration and calibration registers.
 */
static int GB02FUNC946(struct GB02STR137 *data)
{
	int ret = regmap_write(data->regmap, GB02MAC1209,
						   data->config->config_default);
	if (ret < 0)
		return ret;

	GB02MAC1189("ina_init\n");
	return GB02FUNC943(data);
}

static int GB02FUNC950(struct device *dev, int reg, unsigned int *regval)
{
	struct GB02STR137 *data = dev_get_drvdata(dev);
	int ret, retry;

	dev_dbg(dev, "Starting register %d read\n", reg);

	for (retry = 5; retry; retry--) {

		ret = regmap_read(data->regmap, reg, regval);
		if (ret < 0)
			return ret;

		GB02MAC1189("read %d, val = 0x%04x\n", reg, *regval);

		/*
		 * If the current value in the calibration register is 0, the
		 * power and current registers will also remain at 0. In case
		 * the chip has been reset let's check the calibration
		 * register and reinitialize if needed.
		 * We do that extra read of the calibration register if there
		 * is some hint of a chip reset.
		 */
		if (*regval == 0) {
			unsigned int cal;

			ret = regmap_read(data->regmap, GB02MAC1219,
							  &cal);
			if (ret < 0)
				return ret;

			if (cal == 0) {
				dev_warn(dev, "chip not calibrated, reinitializing\n");

				ret = GB02FUNC946(data);
				if (ret < 0)
					return ret;
				/*
				 * Let's make sure the power and current
				 * registers have been updated before trying
				 * again.
				 */
				msleep(GB02MAC1237);
				continue;
			}
		}
		return 0;
	}

	/*
	 * If we're here then although all write operations succeeded, the
	 * chip still returns 0 in the calibration register. Nothing more we
	 * can do here.
	 */
	dev_err(dev, "unable to reinitialize the chip\n");
	return -ENODEV;
}

static int GB02FUNC954(struct GB02STR137 *data, u8 reg,
							unsigned int regval)
{
	int val;

	switch (reg) {
	case GB02MAC1211:
		/* signed register */
		val = DIV_ROUND_CLOSEST((s16)regval, data->config->shunt_div);
		break;
	case GB02MAC1213:
		val = (regval >> data->config->bus_voltage_shift)
			  * data->config->bus_voltage_lsb;
		val = DIV_ROUND_CLOSEST(val, 1000);
		break;
	case GB02MAC1215:
		val = regval * data->power_lsb_uW;
		break;
	case GB02MAC1217:
		/* signed register, result in mA */
		val = (s16)regval * data->current_lsb_uA;
		val = DIV_ROUND_CLOSEST(val, 1000);
		break;
	case GB02MAC1219:
		val = regval;
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		val = 0;
		break;
	}

	return val;
}

static ssize_t GB02FUNC956(struct device *dev,
								 struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct GB02STR137 *data = dev_get_drvdata(dev);
	unsigned int regval;

	int err = GB02FUNC950(dev, attr->index, &regval);

	if (err < 0)
		return err;
	GB02MAC1189("get_value:%d", regval);
	return snprintf(buf, PAGE_SIZE, "%d\n",
					GB02FUNC954(data, attr->index, regval));
}

/*
 * In order to keep calibration register value fixed, the product
 * of current_lsb and shunt_resistor should also be fixed and equal
 * to shunt_voltage_lsb = 1 / shunt_div multiplied by 10^9 in order
 * to keep the scale.
 */
static int GB02FUNC958(struct GB02STR137 *data, long val)
{
	unsigned int dividend = DIV_ROUND_CLOSEST(1000000000,
							data->config->shunt_div);
	if (val <= 0 || val > dividend)
		return -EINVAL;

	mutex_lock(&data->config_lock);
	data->rshunt = val;
	data->current_lsb_uA = DIV_ROUND_CLOSEST(dividend, val);
	data->power_lsb_uW = data->config->power_lsb_factor *
						 data->current_lsb_uA;
	mutex_unlock(&data->config_lock);

	return 0;
}

static ssize_t GB02FUNC960(struct device *dev,
								 struct device_attribute *da,
								 char *buf)
{
	struct GB02STR137 *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%li\n", data->rshunt);
}

static ssize_t GB02FUNC962(struct device *dev,
								  struct device_attribute *da,
								  const char *buf, size_t count)
{
	unsigned long val;
	int status;
	struct GB02STR137 *data = dev_get_drvdata(dev);

	status = kstrtoul(buf, 10, &val);
	if (status < 0)
		return status;

	status = GB02FUNC958(data, val);
	if (status < 0)
		return status;
	return count;
}

static ssize_t GB02FUNC965(struct device *dev,
								   struct device_attribute *da,
								   const char *buf, size_t count)
{
	struct GB02STR137 *data = dev_get_drvdata(dev);
	unsigned long val;
	int status;

	status = kstrtoul(buf, 10, &val);
	if (status < 0)
		return status;

	if (val > INT_MAX || val == 0)
		return -EINVAL;

	status = regmap_update_bits(data->regmap, GB02MAC1209,
								GB02MAC1242,
								GB02FUNC942(val));
	if (status < 0)
		return status;

	return count;
}

static ssize_t GB02FUNC967(struct device *dev,
									struct device_attribute *da, char *buf)
{
	struct GB02STR137 *data = dev_get_drvdata(dev);
	int status;
	unsigned int regval;

	status = regmap_read(data->regmap, GB02MAC1209, &regval);
	if (status)
		return status;

	return snprintf(buf, PAGE_SIZE, "%d\n", GB02FUNC941(regval));
}

/* shunt voltage */
static SENSOR_DEVICE_ATTR(in0_input, 0444, GB02FUNC956, NULL,
						  GB02MAC1211);

/* bus voltage */
static SENSOR_DEVICE_ATTR(in1_input, 0444, GB02FUNC956, NULL,
						  GB02MAC1213);

/* calculated current */
static SENSOR_DEVICE_ATTR(curr1_input, 0444, GB02FUNC956, NULL,
						  GB02MAC1217);

/* calculated power */
static SENSOR_DEVICE_ATTR(power1_input, 0444, GB02FUNC956, NULL,
						  GB02MAC1215);

/* shunt resistance */
static SENSOR_DEVICE_ATTR(shunt_resistor, 0444 | 0200,
						  GB02FUNC960, GB02FUNC962,
						  GB02MAC1219);

/* update interval (ina226 only) */
static SENSOR_DEVICE_ATTR(update_interval, 0444 | 0200,
						  GB02FUNC967, GB02FUNC965, 0);

/* pointers to created device attributes */
static struct attribute *ina2xx_attrs[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_curr1_input.dev_attr.attr,
	&sensor_dev_attr_power1_input.dev_attr.attr,
	&sensor_dev_attr_shunt_resistor.dev_attr.attr,
	NULL,
};

static const struct attribute_group ina2xx_group = {
	.attrs = ina2xx_attrs,
};

static struct attribute *ina226_attrs[] = {
	&sensor_dev_attr_update_interval.dev_attr.attr,
	NULL,
};

static const struct attribute_group ina226_group = {
	.attrs = ina226_attrs,
};

int GB02FUNC975(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct GB02STR137 *data;
	struct device *hwmon_dev;
	u32 val;
	int ret, group = 0;
	enum ina2xx_ids chip;

	GB02MAC1189("ina2xx_driver init\n");
	chip = ina219;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* set the device type */
	data->config = &GB02STR136[chip];
	mutex_init(&data->config_lock);

	val = GB02MAC1239;

	GB02FUNC958(data, val);

	ina2xx_regmap_config.max_register = data->config->registers;

	data->regmap = devm_regmap_init_i2c(client, &ina2xx_regmap_config);
	if (IS_ERR(data->regmap)) {
		dev_err(dev, "failed to allocate register map\n");
		return PTR_ERR(data->regmap);
	}

	ret = GB02FUNC946(data);
	if (ret < 0) {
		dev_err(dev, "error configuring the device: %d\n", ret);
		return -ENODEV;
	}

	data->groups[group++] = &ina2xx_group;
	if (chip == ina226)
		data->groups[group++] = &ina226_group;

	hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
				data, data->groups);
	if (IS_ERR(hwmon_dev))
		return PTR_ERR(hwmon_dev);

	GB02MAC1189("power monitor %s (Rshunt = %li uOhm)\n",
			 client->name, data->rshunt);

	return 0;
}

void GB02FUNC981(struct i2c_client *client)
{
	i2c_unregister_device(client);
}
