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
#ifndef	__GBDC_PM_H__
#define	__GBDC_PM_H__

struct GB02STR241 {
	int (*dpms_off)(void __iomem *dcBase, int conn_id);
};

struct GB02STR242{
	struct GB02STR241 *pm_ops;
};
#endif
