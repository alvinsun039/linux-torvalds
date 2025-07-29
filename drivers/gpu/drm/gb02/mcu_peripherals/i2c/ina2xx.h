/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __INA2XX_H
#define __INA2XX_H
#include "i2c-gb02-mcu.h"
/* common register definitions */
#define GB02MAC1209			0x00
#define GB02MAC1211		0x01 /* readonly */
#define GB02MAC1213		0x02 /* readonly */
#define GB02MAC1215			0x03 /* readonly */
#define GB02MAC1217			0x04 /* readonly */
#define GB02MAC1219		0x05

/* INA226 register definitions */
#define GB02MAC1222		0x06
#define GB02MAC1223		0x07
#define GB02MAC1225			0xFF

/* register count */
#define GB02MAC1228		6
#define GB02MAC1230		8

#define GB02MAC1232		8

/* settings - depend on use case */
#define INA219_CONFIG_DEFAULT		0x399F	/* PGA=8 */
#define INA226_CONFIG_DEFAULT		0x4527	/* averages=16 */

/* worst case is 68.10 ms (~14.6Hz, ina219) */
#define GB02MAC1235		15
#define GB02MAC1237		69 /* worst case delay in ms */

#define GB02MAC1239		10000

/* bit mask for reading the averaging setting in the configuration register */
#define GB02MAC1242		0x0E00

#define GB02MAC1244(reg)		(((reg) & GB02MAC1242) >> 9)
#define GB02MAC1246(val)		((val) << 9)

/* common attrs, ina226 attrs and NULL */
#define GB02MAC1248	3
#define GB02MAC1250 0
#define GB02MAC1252  (0x40>>0)
#define INA219_DEVICE_NAME "gb02_ina219"

/*
 * Both bus voltage and shunt voltage conversion times for ina226 are set
 * to 0b0100 on POR, which translates to 2200 microseconds in total.
 */
#define GB02MAC1255	2200
enum ina2xx_ids { ina219, ina226 };

struct GB02STR136 {
	u16 config_default;
	int calibration_value;
	int registers;
	int shunt_div;
	int bus_voltage_shift;
	int bus_voltage_lsb;	/* uV */
	int power_lsb_factor;
};

struct GB02STR137 {
	const struct GB02STR136 *config;

	long rshunt;
	long current_lsb_uA;
	long power_lsb_uW;
	struct mutex config_lock;
	struct regmap *regmap;

	const struct attribute_group *groups[GB02MAC1248];
};

#ifdef CONFIG_MCU_I2C
int GB02FUNC975(struct i2c_client *client);
void GB02FUNC981(struct i2c_client *client);
#else
static inline int GB02FUNC975(struct i2c_client *client)
{
	return 0;
}
static inline void GB02FUNC981(struct i2c_client *client)
{
	return;
}
#endif
#endif
