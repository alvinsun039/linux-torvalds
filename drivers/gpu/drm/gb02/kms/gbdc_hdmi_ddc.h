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

#ifndef __GBDC_HDMI_DDC_H__
#define __GBDC_HDMI_DDC_H__

extern int GB02FUNC1242(struct gbdc_connector *gbdc_conn);
extern int GB02FUNC1246(void *data, u8 *buf, unsigned int block,
								size_t len);

#endif

