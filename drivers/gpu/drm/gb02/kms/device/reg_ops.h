/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <common/xt.h>
#include <ip/gb_dp.h>

#ifndef		__REG_OPS_H__
#define		__REG_OPS_H__

#define DP_DC
static inline u32 GB02FUNC730(void *baseAddr, u32 offset)
{
#ifdef DP_DC
	void __iomem *opsAddr = baseAddr + offset;

	return ioread32(opsAddr);
#else
	return 0;
#endif
}

static inline void GB02FUNC733(void *baseAddr, u32 offset, u32 value)
{
#ifdef DP_DC
	void __iomem *opsAddr = baseAddr + offset;

	iowrite32(value, opsAddr);
#endif
}

static inline void GB02FUNC734(void *baseAddr, u32 offset, u32 mask)
{
#ifdef DP_DC
	u32 data;
	if(offset != 0x10){
		data = GB02FUNC730(baseAddr, offset);
	}else{
		data = 0x0;//0x20000;
	}
	data |= mask;
	GB02FUNC733(baseAddr, offset, data);
#endif
}
static inline void GB02FUNC735(void *baseAddr, u32 offset, u32 mask)
{
#ifdef DP_DC
	u32 data;
	if(offset != 0x10){
		data = GB02FUNC730(baseAddr, offset);
	}else{
		data = 0;//0x20000;
	}
	data &= ~mask;
	GB02FUNC733(baseAddr, offset, data);
#endif
}

#endif
