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

#include "gb_pcie_info.h"

u64 GB02FUNC456(void)
{
	u64 vram_size = 0;
	struct GB02STR70 *gbdev = GB02FUNC518();
	int mcu_peri_bar_id = GB02FUNC474(gbdev->GB02STR153);

	vram_size = GB02FUNC530(gbdev->pci_bars[mcu_peri_bar_id].mmio, GB02MAC675);

	if(vram_size > 0 && vram_size <= GB02MAC677){
		gb_printf(KERN_INFO, "get vram size from board: 0x%llx\n", vram_size);
		return vram_size*GB;
	}else{
		gb_printf(KERN_ERR, "%s get vram size faild, mcu_peri_bar_id:%d\n",
			__func__, mcu_peri_bar_id);
		return GB02FUNC464();
	}
}

enum genbu_asic_type GB02FUNC460(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->gb_type;
}

u64 GB02FUNC462(void)
{
	u64 vram_size = GB02FUNC464();
	if (vram_size == GB02MAC486)
		return GB02MAC487;
	else
		return vram_size / 2;
}

u64 GB02FUNC464(void)
{
	struct GB02STR70 *gbdev = GB02FUNC518();
	int ddr_var_cnt = GB02FUNC469(gbdev->GB02STR153);
	int ddr_bar_id = GB02FUNC468(gbdev->GB02STR153);

	if (ddr_bar_id <= ddr_var_cnt)
		return gbdev->pci_bars[ddr_bar_id].len;

	gb_printf(KERN_ERR, "%s get vram size faild, ddr_bar_id:%d ddr_var_cnt:%d\n",
			__func__, ddr_bar_id, ddr_var_cnt);
	return 0;
}

int GB02FUNC465(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->gpu_reg_bar_id;
}

int GB02FUNC468(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->ddr_bar_id;
}

int GB02FUNC469(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->pci_bar_cnt;
}

u64 GB02FUNC471(struct GB02STR67 *GB02STR153)
{
	return GB02MAC496;
}

int GB02FUNC472(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->irq_num;
}

int GB02FUNC474(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->mcu_peri_bar_id;
}

int GB02FUNC477(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->peri_base_bar_id;
}

int GB02FUNC479(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->dc_reg_bar_id;
}
int GB02FUNC480(struct GB02STR67 *GB02STR153)
{
	return GB02STR153->crtc_num;
}

