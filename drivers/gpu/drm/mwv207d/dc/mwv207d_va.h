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
#include <linux/irq.h>
#ifndef MWV207D_VA_H_BSY8LF4F
#define MWV207D_VA_H_BSY8LF4F

struct drm_crtc;

int mwv207d_va_init(struct mwv207d_device *mdev);

int mwv207d_va_lut_suspend(struct drm_crtc *crtc);
int mwv207d_va_lut_resume(struct drm_crtc *crtc);

void mwv207d_crtc_prepare_vblank(struct drm_crtc *crtc);
irqreturn_t mwv207d_va_handle_vblank(int irq, void *data);
#endif
