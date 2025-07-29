// SPDX-License-Identifier: GPL-2.0
/*
 * ads1015.c - lm_sensors driver for ads1015 12-bit 4-input ADC
 * (C) Copyright 2010
 * Dirk Eibach, Guntermann & Drunck GmbH <eibach@gdsys.de>
 *
 * Based on the ads7828 driver by Steve Hardy.
 *
 * Datasheet available at: http://focus.ti.com/lit/ds/symlink/ads1015.pdf
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/of.h>

//#include <linux/platform_data/ads1015.h>
#include "i2c-gb02-mcu.h"
#include "ads1015.h"

//#define ADS1015_DEBUG
#ifdef ADS1015_DEBUG
#define GB02MAC1114(arg...) gb_printf(KERN_INFO, "[" GB02_ADS1015_DEV_NAME "] " arg)
#else
#define GB02MAC1114(arg...)
#endif

static int GB02FUNC765(struct i2c_client *client, unsigned int channel)
{
	u16 config;
	struct GB02STR120 *data = i2c_get_clientdata(client);
	unsigned int pga = data->channel_data[channel].pga;
	unsigned int data_rate = data->channel_data[channel].data_rate;
	unsigned int conversion_time_ms;
	const unsigned int * const rate_table = data->id == ads1115 ?
		data_rate_table_1115 : data_rate_table_1015;
	int res;

	mutex_lock(&data->update_lock);

	/* get channel parameters */
	res = i2c_smbus_read_word_swapped(client, ADS1015_CONFIG);
	if (res < 0)
		goto err_unlock;
	config = res;
	conversion_time_ms = DIV_ROUND_UP(1000, rate_table[data_rate]);

	/* setup and start single conversion */
	config &= 0x001f;
	config |= (1 << 15) | (1 << 8);//single conversion and start it
	config |= (channel & 0x0007) << 12;

	config |= (pga & 0x0003) << 9;//FSR = [-1.024:+1.024]DS1015_DEFAULT_PGA

	config |= (data_rate & 0x0004) << 5;

	res = i2c_smbus_write_word_swapped(client, ADS1015_CONFIG, config);
	if (res < 0)
		goto err_unlock;
	GB02MAC1114("set conversion success! config value :0x%x\n", config);
	/* wait until conversion finished */
	msleep(conversion_time_ms);
	res = i2c_smbus_read_word_swapped(client, ADS1015_CONFIG);
	if (res < 0)
		goto err_unlock;
	config = res;
	if (!(config & (1 << 15))) {
		/* conversion not finished in time */
		res = -EIO;
		goto err_unlock;
	}

	GB02MAC1114("conversion finish! config value :0x%x\n", config);
	res = i2c_smbus_read_word_swapped(client, ADS1015_CONVERSION);
	GB02MAC1114("GB02FUNC772 channel %d ,data 0x%x\n", channel, res);
err_unlock:
	mutex_unlock(&data->update_lock);
	return res;
}

static int GB02FUNC770(struct i2c_client *client, unsigned int channel,
		s16 reg)
{
	struct GB02STR120 *data = i2c_get_clientdata(client);
	unsigned int pga = data->channel_data[channel].pga;
	int fullscale = fullscale_table[pga];
	const int mask = data->id == ads1115 ? 0x7fff : 0x7ff0;

	return DIV_ROUND_CLOSEST(reg * fullscale, mask);
}

/* sysfs callback function */
static ssize_t GB02FUNC772(struct device *dev, struct device_attribute *da,
		char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	int res;
	int index = attr->index;

	res = GB02FUNC765(client, index);
	if (res < 0)
		return res;

	return sprintf(buf, "%d\n", GB02FUNC770(client, index, res));
}

static const struct sensor_device_attribute ads1015_in[] = {
	SENSOR_ATTR(in0_input, 0444, GB02FUNC772, NULL, 0),
	SENSOR_ATTR(in1_input, 0444, GB02FUNC772, NULL, 1),
	SENSOR_ATTR(in2_input, 0444, GB02FUNC772, NULL, 2),
	SENSOR_ATTR(in3_input, 0444, GB02FUNC772, NULL, 3),
	SENSOR_ATTR(in4_input, 0444, GB02FUNC772, NULL, 4),
	SENSOR_ATTR(in5_input, 0444, GB02FUNC772, NULL, 5),
	SENSOR_ATTR(in6_input, 0444, GB02FUNC772, NULL, 6),
	SENSOR_ATTR(in7_input, 0444, GB02FUNC772, NULL, 7),
};

/*
 * Driver interface
 */
void GB02FUNC775(struct i2c_client *client)
{
	int k;
	struct GB02STR120 *data = i2c_get_clientdata(client);

	i2c_unregister_device(client);
	hwmon_device_unregister(data->hwmon_dev);
	for (k = 0; k < GB02MAC1118; ++k)
		device_remove_file(&client->dev, &ads1015_in[k].dev_attr);
}

static void GB02FUNC776(struct i2c_client *client)
{
	unsigned int k;
	struct GB02STR120 *data = i2c_get_clientdata(client);
	struct GB02STR121 *pdata = dev_get_platdata(&client->dev);

	/* prefer platform data */
	if (pdata) {
		memcpy(data->channel_data, pdata->channel_data,
		       sizeof(data->channel_data));
		return;
	}

	/* fallback on default configuration */
	for (k = 0; k < GB02MAC1118; ++k) {
		data->channel_data[k].enabled = true;
		data->channel_data[k].pga = GB02MAC1116;
		data->channel_data[k].data_rate = GB02MAC1117;
	}
}

int GB02FUNC779(struct i2c_client *client)
{
	struct GB02STR120 *data;
	int err;
	unsigned int k;

	data = devm_kzalloc(&client->dev, sizeof(struct GB02STR120),
		GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->id = ads1015;
	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	/* build sysfs attribute group */
	GB02FUNC776(client);
	for (k = 0; k < GB02MAC1118; ++k) {
		if (!data->channel_data[k].enabled)
			continue;
		err = device_create_file(&client->dev, &ads1015_in[k].dev_attr);
		if (err)
			goto exit_remove;
	}

	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	return 0;

exit_remove:
	for (k = 0; k < GB02MAC1118; ++k)
		device_remove_file(&client->dev, &ads1015_in[k].dev_attr);
	return err;
}
