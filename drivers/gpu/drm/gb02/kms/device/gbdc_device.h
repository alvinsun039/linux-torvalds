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
#ifndef		__GBDC_DEVICE_H__
#define		__GBDC_DEVICE_H__

extern struct GB02STR80 gbPlaneCap[];

int GB02FUNC43(int card_type, struct GB02STR254 *pcie_info,
	struct GB02STR67 *pcie_res,
	struct GB02STR76 *gbip_reg_offset,
	struct GB02STR75 *ctc_offset);
void GB02FUNC59(struct GB02STR245 *kms_info, struct GB02STR82 *kms_res);
void GB02FUNC64(struct GB02STR242 *gb_pm);
void GB02FUNC67(struct GB02STR155 *gb_dev,
		    struct drm_device *drm_dev,
		    struct pci_dev *pdev, int card_type);
void GB02FUNC73(struct GB02STR155 *gb_dev);
int GB02FUNC76(struct GB02STR155 *gb_dev);
struct GB02STR155 *GB02FUNC85(void *dev_private);
struct GB02STR155 *gb_get_gb_device(void *dev_private);
static inline bool GB02FUNC96(struct GB02STR245 *kms_info)
{
	return kms_info->infinity;
}



#endif
