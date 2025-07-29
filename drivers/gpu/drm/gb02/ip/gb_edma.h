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

#ifndef _GB_EDMA_H
#define _GB_EDMA_H

#include <linux/device.h>
#include "common/gb_uk.h"
#include "common/gb_pcie_info.h"

struct GB02STR188 {
	/* user pages list */
	struct page **cpu_pages;
	/* gpu page phy list */
	phys_addr_t *gpu_phys;
	/* dma translate page cnt in cpu page size */
	unsigned long num_dma_pages;
	unsigned long num_dma_entry;
	enum gb_dma_data_direction dma_dir;
	/* ll table phy in gpu space, GB02MAC328~GB02MAC308 */
	phys_addr_t ll_table_addr;
	int last_page_size;
	dma_addr_t *dma_buffer;
};

extern struct page **gb_edma_get_user_cpu_pages(
	u64 cpu_va, enum gb_dma_data_direction dma_dir,
	unsigned long num_dma_pages);
extern int GB02FUNC1688(struct device *dev,
	struct GB02STR188 *dma_info,
	struct GB02STR70 *pcie_info);
extern int GB02FUNC1699(struct device *dev,
	struct GB02STR105 *dma_info,
	phys_addr_t gpu_phy,
	struct page **user_pages,
	struct GB02STR70 *pcie_info);

#endif
