/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ADS1015_H
#define __ADS1015_H
//#include <linux/platform_data/ads1015.h>
#include "i2c-gb02-mcu.h"
/* ADS1015 registers */
enum {
	ADS1015_CONVERSION = 0,
	ADS1015_CONFIG = 1,
};

/* PGA fullscale voltages in mV */
static const unsigned int fullscale_table[8] = {
	6144, 4096, 2048, 1024, 512, 256, 256, 256
};

/* Data rates in samples per second */
static const unsigned int data_rate_table_1015[8] = {
	128, 250, 490, 920, 1600, 2400, 3300, 3300
};

static const unsigned int data_rate_table_1115[8] = {
	8, 16, 32, 64, 128, 250, 475, 860
};

#define GB02MAC1115 0xff
#define GB02MAC1116 3
#define GB02MAC1117 4
#define GB02MAC1118 4
enum ads1015_chips {
	ads1015,
	ads1115,
};

struct GB02STR119 {
	bool enabled;
	unsigned int pga;
	unsigned int data_rate;
};

struct GB02STR120 {
	struct device *hwmon_dev;
	struct mutex update_lock; /* mutex protect updates */
	struct GB02STR119 channel_data[GB02MAC1118];
	enum ads1015_chips id;
};

struct GB02STR121 {
	struct GB02STR119 channel_data[GB02MAC1118];
};

#define GB02MAC1119  0x49
#define GB02MAC1120  0
#define ADS1015_DEVICE0_NAME ("ADS1015-0")
#define GB02MAC1121  0x48
#define GB02MAC1122  1
#define ADS1015_DEVICE1_NAME ("ADS1015-1")
#define GB02MAC1123  0x48
#define GB02MAC1124  2
#define ADS1015_DEVICE2_NAME ("ADS1015-2")
#ifdef CONFIG_MCU_I2C
int GB02FUNC779(struct i2c_client *client);
void GB02FUNC775(struct i2c_client *client);
#else
static int GB02FUNC779(struct i2c_client *client)
{
	return 0;
}
static void GB02FUNC775(struct i2c_client *client)
{
	return;
}
#endif
#endif
