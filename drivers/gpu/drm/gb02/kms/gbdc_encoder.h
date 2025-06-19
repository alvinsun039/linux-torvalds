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
#ifndef		__GBDC_ENCODER_H__
#define		__GBDC_ENCODER_H__
struct GB02STR156 {
	struct drm_encoder base;
	struct drm_display_mode native_mode;
	uint32_t encoder_enum;
	uint32_t encoder_id;
	uint32_t devices;
	uint32_t active_device;

	bool virt;
};

typedef enum {
	GBDC_ENCODER0 = 0,
	GBDC_ENCODER1,
	GBDC_ENCODER_MAX
} gbdc_encoder_e;

struct GB02STR156 *GB02FUNC1206(struct drm_device *dev, int i,bool virt);
#define to_gbdc_encoder(x)   container_of(x, struct GB02STR156, base)
int GB02FUNC1207(struct drm_encoder *encoder);
#endif
