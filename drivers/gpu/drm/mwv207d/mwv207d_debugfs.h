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
#ifndef MWV207D_DEBUGFS_H
#define MWV207D_DEBUGFS_H

#ifdef CONFIG_DEBUG_FS

void mwv207d_debugfs_init(struct mwv207d_device *mdev);

#else

static inline void mwv207d_debugfs_init(struct mwv207d_device *mdev)
{

}

#endif

#endif
