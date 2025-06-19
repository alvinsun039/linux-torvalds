/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __TMP102_H
#define __TMP102_H
#include "i2c-gb02-mcu.h"
#define	GB02MAC1289			0x00
#define	GB02MAC1290			0x01
/* note: these bit definitions are byte swapped */
#define		GB02MAC1291		0x0100
#define		GB02MAC1292		0x0200
#define		GB02MAC1293		0x0400
#define		GB02MAC1294		0x0800
#define		GB02MAC1295		0x1000
#define		GB02MAC1296		0x2000
#define		GB02MAC1297		0x4000
#define		GB02MAC1298		0x8000
#define		GB02MAC1299		0x0010
#define		GB02MAC1300		0x0020
#define		GB02MAC1301		0x0040
#define		GB02MAC1302		0x0080
#define	GB02MAC1304			0x02
#define	GB02MAC1306		0x03

#define GB02MAC1307	(GB02MAC1291 | GB02MAC1292 | \
				 GB02MAC1293 | GB02MAC1294 | \
				 GB02MAC1295 | GB02MAC1298 | \
				 GB02MAC1299 | GB02MAC1300 | \
				 GB02MAC1301 | GB02MAC1302)

#define GB02MAC1308	(GB02MAC1291 | GB02MAC1298 | \
				 GB02MAC1301)
#define GB02MAC1309	(GB02MAC1292 | GB02MAC1299 | \
				 GB02MAC1302)

#define GB02MAC1310		35	/* in milli-seconds */

#define GB02MAC1311 4
#define GB02MAC1312 5
#define GB02MAC1313  0x50
#define GB02MAC1314  0
#define TMP102_DEVICE_NAME "tmp102"
struct tmp102 {
	struct regmap *regmap;
	u16 config_orig;
	unsigned long ready_time;
};

#ifdef CONFIG_MCU_I2C
int GB02FUNC1088(struct GB02STR133 *gb02_i2c);
int GB02FUNC1089(struct GB02STR133 *gb02_i2c);
#else
static inline int GB02FUNC1088(struct GB02STR133 *gb02_i2c)
{
	return 0;
}
static inline int GB02FUNC1089(struct GB02STR133 *gb02_i2c)
{
	return 0;
}
#endif
#endif
