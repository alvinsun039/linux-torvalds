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

#ifndef MWV207D_VBIOS_H
#define MWV207D_VBIOS_H

struct mwv207d_device;

enum mwv207d_pll_id {
	MWV207D_PLL_MEM_NOC_3D,
	MWV207D_PLL_CFG_NOC_3D,
	MWV207D_PLL_CORE_3D,

	MWV207D_PLL_MEM_NOC_2D,
	MWV207D_PLL_CFG_NOC_2D,
	MWV207D_PLL_CORE_2D,

	MWV207D_PLL_MEM_NOC_HD,
	MWV207D_PLL_CFG_NOC_HD,
	MWV207D_PLL_CORE_HD,

	MWV207D_PLL_MEM_NOC_DMA,
	MWV207D_PLL_CFG_NOC_FUS_DMA,
	MWV207D_PLL_CORE_FUS,

	MWV207D_PLL_DDR0,
	MWV207D_PLL_DDR1,
	MWV207D_PLL_DDR2,
	MWV207D_PLL_DDR3,

	MWV207D_PLL_VA0,
	MWV207D_PLL_VA1,
	MWV207D_PLL_VA2,
	MWV207D_PLL_VA3,

	MWV207D_PLL_VI_HDMI_PHY_EDP,
	MWV207D_PLL_VA_SYS,
	MWV207D_PLL_PIX_CLK_IN,

	MWV207D_PLL_MEM_NOC,
	MWV207D_PLL_COREF0,
	MWV207D_PLL_COREF1,
	MWV207D_PLL_PCIE_DM,
	MWV207D_PLL_NR,
};

struct mwv207d_vdat {
	const void *dat;
	u32 len;
};

const struct mwv207d_vdat *
mwv207d_vbios_vdat(struct mwv207d_device *mdev, u32 die, u32 key);
int mwv207d_vbios_init(struct mwv207d_device *mdev);
const char *mwv207d_vbios_get_version(struct mwv207d_device *mdev);
void mwv207d_vbios_fini(struct mwv207d_device *mdev);

int mwv207d_vbios_set_pll(struct mwv207d_device *mdev,
			  enum mwv207d_pll_id id,
			  u32 kfreq);
int mwv207d_vbios_get_pll(struct mwv207d_device *mdev,
			  enum mwv207d_pll_id id,
			  u32 *kfreq);
int mwv207d_vbios_set_volt(struct mwv207d_device *mdev,
			   int volt_id, u32 volt);

int mwv207d_read_vlog(struct mwv207d_device *mdev, char *buf, u32 size);

u32 mwv207d_vbios_get_temp(struct mwv207d_device *mdev);
u32 mwv207d_vbios_get_fan_speed(struct mwv207d_device *mdev);

int mwv207d_vbios_get_ddr_bandwidth(struct mwv207d_device *mdev, u32 t,
			u32 *rbw, u32 *wbw, u32 *tbw);

u32 mwv207d_vbios_get_max_duration(struct mwv207d_device *mdev);
#endif
