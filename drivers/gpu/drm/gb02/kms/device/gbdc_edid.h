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
#ifndef	__GBDC_EDID_H__
#define	__GBDC_EDID_H__

extern bool GB02FUNC759(void);

int GB02FUNC117(int index);
const struct drm_display_mode *GB02FUNC119(int index, int dindex);
const struct drm_display_mode *GB02FUNC122(int index, int dindex);
#endif
