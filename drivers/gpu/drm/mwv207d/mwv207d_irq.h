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
#ifndef MWV207D_IRQ_H_Z8YGVNB2
#define MWV207D_IRQ_H_Z8YGVNB2
#include "mwv207d_drv.h"

int mwv207d_irq_init(struct mwv207d_device *mdev);
void mwv207d_irq_fini(struct mwv207d_device *mdev);
void mwv207d_irq_suspend(struct mwv207d_device *mdev);
void mwv207d_irq_resume(struct mwv207d_device *mdev);

u32 mwv207d_irq_find(struct mwv207d_device *mdev, u32 pf_irq, u32 vf_irq);

#endif
