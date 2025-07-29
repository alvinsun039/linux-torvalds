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

#ifndef __GB02_HDMAC_H
#define __GB02_HDMAC_H
#include "common/gb_pcie_info.h"

//#define GB02MAC1068	3
//#define GB02MAC2844	GB02MAC1068
#define GB02MAC2629  0
#define GB02MAC2630  1
#define GB02MAC2631  2
#define GB02MAC2632 0x10ee
#define GB02MAC2633 0x8018

//#define HDMAC_MULTI_PROCESS_TEST

//#define HDMAC_DBG
//#define HDMAC_MMU_DUMP
#ifdef HDMAC_DBG
#define hdmac_debug pr_err
#else
#define hdmac_debug(format, arg...) \
	do {} while (0)
#endif

struct GB02STR202 {
	void __iomem *vram_addr;
	void __iomem *regs;
	void __iomem *hdma_regs;
};
int GB02FUNC1742(struct GB02STR70 *pcie_info);
#endif









