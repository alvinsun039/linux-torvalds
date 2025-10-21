#ifndef MWV207D_HDMI_AUDIO_H
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
#define MWV207D_HDMI_AUDIO_H

struct mwv207d_device;
struct mwv207d_audio_card *mwv207d_audio_create(struct mwv207d_device *mdev);
void mwv207d_audio_destroy(struct mwv207d_device *mdev);

void mwv207d_audio_suspend(struct mwv207d_device *mdev);
void mwv207d_audio_resume(struct mwv207d_device *mdev);
void mwv207d_audio_switch(struct mwv207d_device *mdev, int idx, uint8_t *eld,
			u32 clock, bool active, bool hw_connected);
void mwv207d_audio_isr(struct mwv207d_device *mdev, int idx);

#endif
