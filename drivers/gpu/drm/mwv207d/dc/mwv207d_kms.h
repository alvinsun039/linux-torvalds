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
#ifndef MWV207D_KMS_H_VBCHTKVP
#define MWV207D_KMS_H_VBCHTKVP

#include <drm/drm_gem.h>
#include "mwv207d_drv.h"

int mwv207d_framebuffer_init(struct drm_device *dev, struct drm_framebuffer *fb,
			     const struct drm_mode_fb_cmd2 *mode_cmd,
			     struct drm_gem_object *gobj);

int mwv207d_fbdev_init(struct mwv207d_device *mdev);
void mwv207d_fbdev_fini(struct mwv207d_device *mdev);
void mwv207d_fbdev_resume(struct mwv207d_device *mdev);

int mwv207d_kms_init(struct mwv207d_device *mdev);
void mwv207d_kms_fini(struct mwv207d_device *mdev);

int mwv207d_kms_suspend(struct mwv207d_device *mdev);
int mwv207d_kms_resume(struct mwv207d_device *mdev);

#endif
