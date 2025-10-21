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
#ifndef MWV207D_GART_H
#define MWV207D_GART_H

struct mwv207d_gart;

void mwv207d_gart_bind(struct mwv207d_device *mdev, u64 offset, u32 pages,
		      dma_addr_t *dma_addr, u32 flags);
void mwv207d_gart_unbind(struct mwv207d_device *mdev, u64 offset, u32 pages);

int mwv207d_gart_init(struct mwv207d_device *mdev);
void mwv207d_gart_fini(struct mwv207d_device *mdev);

int mwv207d_gart_suspend(struct mwv207d_device *mdev);
void mwv207d_gart_resume(struct mwv207d_device *mdev);

#endif
