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
#ifndef		__GBDC_DISPLAY_H__
#define		__GBDC_DISPLAY_H__

void GB02FUNC1160(const struct drm_display_mode *dmode,
									struct videomode *vm);

struct drm_framebuffer *GB02FUNC1169(
		struct drm_device *dev, struct drm_file *filp,
		const struct drm_mode_fb_cmd2 *mode_cmd);
#endif
