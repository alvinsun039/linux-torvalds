/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <drm/drm_file.h>
#include "mwv207d_drm.h"

#ifndef MWV207D_DB_H_AGPY6ZIQ
#define MWV207D_DB_H_AGPY6ZIQ

struct mwv207d_device;

int mwv207d_db_init(struct mwv207d_device *mdev);
void mwv207d_db_fini(struct mwv207d_device *mdev);
void mwv207d_db_sort(struct mwv207d_device *mdev);

int mwv207d_db_add(struct mwv207d_device *mdev, u32 key, u32 val);

int  mwv207d_db_ioctl(struct drm_device *dev, void *data,
		     struct drm_file *filp);

#endif
