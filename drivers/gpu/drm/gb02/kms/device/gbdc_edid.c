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
#include <linux/init.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#else
#include <drm/drm_modes.h>
#endif
#include "gbdc_ops.h"

const struct drm_display_mode gbdc_modes[] = {
	/* 0x52 - 1920x1080@60Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
#if 0
	/* 95 - 3840x2160@30Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4016,
                   4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
                   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC)},
	/* 97 - 3840x2160@60Hz 16:9 */
        { DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
                   4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
                   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC)},
	/* 0x4c - 2560x1600@60Hz RB */
        { DRM_MODE("2560x1600", DRM_MODE_TYPE_DRIVER, 268500, 2560, 2608,
                   2640, 2720, 0, 1600, 1603, 1609, 1646, 0,
                   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x55 - 1280x720@60Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x1c - 1280x800@60Hz */
	{ DRM_MODE("1280x800", DRM_MODE_TYPE_DRIVER, 83500, 1280, 1352,
		   1480, 1680, 0, 800, 803, 809, 831, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x51 - 1366x768@60Hz */
	{ DRM_MODE("1366x768", DRM_MODE_TYPE_DRIVER, 85500, 1366, 1436,
		   1579, 1792, 0, 768, 771, 774, 798, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x53 - 1600x900@60Hz */
	{ DRM_MODE("1600x900", DRM_MODE_TYPE_DRIVER, 108000, 1600, 1624,
		   1704, 1800, 0, 900, 901, 904, 1000, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x33 - 1600x1200@60Hz */
	{ DRM_MODE("1600x1200", DRM_MODE_TYPE_DRIVER, 162000, 1600, 1664,
		   1856, 2160, 0, 1200, 1201, 1204, 1250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
#endif
	/* 0x04 - 640x480@60Hz */
	{ DRM_MODE("640x480", DRM_MODE_TYPE_DRIVER, 25175, 640, 656,
		   752, 800, 0, 480, 490, 492, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x10 - 1024x768@60Hz */
	{ DRM_MODE("1024x768", DRM_MODE_TYPE_DRIVER, 65000, 1024, 1048,
		   1184, 1344, 0, 768, 771, 777, 806, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x09 - 800x600@60Hz */
	{ DRM_MODE("800x600", DRM_MODE_TYPE_DRIVER, 40000, 800, 840,
		   968, 1056, 0, 600, 601, 605, 628, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x27 - 1360x768@60Hz */
	{ DRM_MODE("1360x768", DRM_MODE_TYPE_DRIVER, 85500, 1360, 1424,
		   1536, 1792, 0, 768, 771, 777, 795, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x2f - 1440x900@60Hz */
	{ DRM_MODE("1440x900", DRM_MODE_TYPE_DRIVER, 106500, 1440, 1520,
		   1672, 1904, 0, 900, 903, 909, 934, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x3a - 1680x1050@60Hz */
	{ DRM_MODE("1680x1050", DRM_MODE_TYPE_DRIVER, 146250, 1680, 1784,
		   1960, 2240, 0, 1050, 1053, 1059, 1089, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC) },
};

int GB02FUNC117(int index)
{
		return (sizeof(gbdc_modes) / sizeof(gbdc_modes[0]));
}

const struct drm_display_mode *GB02FUNC119(int index, int dindex)
{
		return &gbdc_modes[dindex];
}

const struct drm_display_mode *GB02FUNC122(int index)
{
	int i;
	/*
	 * The val[] length is greater than drm_display_mode.name[32]
	 */
	char val[64] = {0};

	GB02FUNC394("resolution", val, sizeof(val));
	for (i = 0; i < GB02FUNC117(index); i++) {
		if (strcmp(val, gbdc_modes[i].name) == 0)
			return &gbdc_modes[i];
	}
	return NULL;
}
