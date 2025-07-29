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

#ifndef	__GB_PCIE_MAP_H__
#define	__GB_PCIE_MAP_H__

#include <linux/module.h>
#include "common/gb_pcie_info.h"

#define	GB02MAC2844		3
#define	GB02MAC2845		6
#define	GB02MAC2846	0x1000
struct GB02STR248 {
	enum genbu_asic_type gb_type;
	unsigned long vram_size;
	int pci_bar_cnt;
	int gpu_reg_bar_id;
	int dc_reg_bar_id;
	int ddr_bar_id;
	int pll_bar_id;
};
struct GB02STR249{

	//const u32		gpu_conf_base;
	void __iomem 	*de_base;
	/* address offset of the SE registers bank */
	void __iomem 	*se_base;
	/* address offset of the DC registers bank */
	void __iomem 	*dc_base;

};

struct GB02STR250 {
	int	(*mm_read)(void *baseAddr, u32 offset);
	int (*mm_write)(void *baseAddr, u32 offset, u32 value);
	int (*io_read)(void *baseAddr, u32 offset);
	int (*io_write)(void *baseAddr, u32 offset, u32 value);
	int (*set_bits)(void *baseAddr, u32 offset, u32 mask);
	int (*clear_bits)(void *baseAddr, u32 offset, u32 mask);
};

struct GB02STR251 {
	int (*get_version)(void __iomem *baseAddr, void *gb_version);
};
struct GB02STR252{
	void __iomem	*pcie_config_base;
	void __iomem 	*dpll_config_base;
	void __iomem 	*pll_config_base;
	void __iomem 	*hdmi_config_base;
	void __iomem 	*hdmi_debug_base;
	void __iomem 	*fpga_pll_base;
	int		fpga_pll_step;
};

struct GB02STR253{
	void __iomem 	*fb_map;
	unsigned long 	fb_base;
	u64		fb_start_offset;
	u64 vram_size;
};


struct GB02STR254{
	//struct GB02STR155 				*gbDev;
//	void __iomem 					*regs;
	void __iomem 	*pci_mmio_bar[GB02MAC2845];
	uintptr_t	start_bar[GB02MAC2845];
	u64		len_bar[GB02MAC2845];

	int		dc_num;
	int		dc_base_offset;
	struct GB02STR249 	*dc_config;
	struct GB02STR252 	ip_config;
	struct GB02STR253		vram_config;
	struct GB02STR67		asic_dinfo;
//	struct GB02STR250		*reg_ops;
	struct GB02STR251		*gb_cops;
};
#endif
