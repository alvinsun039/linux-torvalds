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

#ifndef __GBDC_VGA_BRIDGE_H__
#define __GBDC_VGA_BRIDGE_H__
#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>

struct GB02STR243 {
	struct drm_bridge   bridge;
	struct drm_connector    *connector;

	struct i2c_adapter  *ddc;
};

int GB02FUNC1874(struct GB02STR243 *vga);
int GB02FUNC1875(struct GB02STR243 *vga);

#endif
