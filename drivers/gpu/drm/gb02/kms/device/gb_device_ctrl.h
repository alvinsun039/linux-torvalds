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

#include <linux/module.h>
#include "common/gb_pcie_info.h"
#ifndef __GB_DEVICE_CTRL_H__
#define __GB_DEVICE_CTRL_H__



#define GB02MAC872 0x3082010
#define GB02MAC873 0x3083010

void GB02FUNC515(void *gpio_addr, u32 gpio_bit);
void GB02FUNC515(void *gpio_addr, u32 gpio_bit);
void HDMI_power_down(struct GB02STR70 *gpi);
void HDMI_power_reset_all(struct GB02STR70 *gpi);
void HDMI_power_reset(struct GB02STR70 *gpi, int dp_index);

#endif