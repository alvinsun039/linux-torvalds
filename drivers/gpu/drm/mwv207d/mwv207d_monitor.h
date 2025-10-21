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
#ifndef MWV207D_MONITOR_H_ZM8DG5W6
#define MWV207D_MONITOR_H_ZM8DG5W6

struct mwv207d_device;
int mwv207d_monitor_init(struct mwv207d_device *mdev);
void mwv207d_monitor_fini(struct mwv207d_device *mdev);
int mwv207d_monitor_suspend(struct mwv207d_device *mdev);
void mwv207d_monitor_resume(struct mwv207d_device *mdev);

#endif
