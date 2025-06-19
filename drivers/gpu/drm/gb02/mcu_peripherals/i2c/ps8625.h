/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PS8625_H
#define __PS8625_H
#include "i2c-gb02-mcu.h"

#define GB02MAC1287  0x58
#define GB02MAC1288  1
#define PS8625_DEVICE0_NAME "PS8625-0"
#ifdef CONFIG_MCU_I2C
int GB02FUNC1046(struct i2c_client *client);
void GB02FUNC1051(struct i2c_client *client);
#else
static inline int GB02FUNC1046(struct i2c_client *client)
{
	return 0;
}
static inline void GB02FUNC1051(struct i2c_client *client)
{
	return;
}
#endif
#endif
