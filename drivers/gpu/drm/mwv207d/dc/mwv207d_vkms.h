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

#ifndef MWV207D_VKMS_H
#define MWV207D_VKMS_H

#define crtc_to_vdisplay(target) \
	container_of(target, struct mwv207d_vdisplay, crtc)
#define connector_to_vdisplay(target) \
	container_of(target, struct mwv207d_vdisplay, connector)
#define encoder_to_vdisplay(target) \
	container_of(target, struct mwv207d_vdisplay, encoder)

int mwv207d_vkms_init(struct mwv207d_device *mdev);
void mwv207d_vkms_fini(struct mwv207d_device *mdev);
#endif
