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
#include <linux/delay.h>

#include "mwv207d_drv.h"
#include "mwv207d_bo.h"
#include "mwv207d_gart.h"

struct mwv207d_gart {
	void __iomem *mmio;

	dma_addr_t pte_addr;
	u64 *pte;
	u64 nr_pte;

	spinlock_t lock;

	dma_addr_t safe_page_addr;
	void *safe_page;

	struct mwv207d_device *mdev;
};

static inline void gart_write(struct mwv207d_gart *gart, u32 reg, u32 value)
{
	writel_relaxed(value, gart->mmio + reg);
}

static inline u32 gart_read(struct mwv207d_gart *gart, u32 reg)
{
	return readl_relaxed(gart->mmio + reg);
}

static inline u32 gart_nr_pages(u32 pages)
{
	return pages * (PAGE_SIZE / SZ_4K);
}

static inline u64 gart_pte(u64 addr, u32 flags)
{
	addr = addr >> 12;
	if (flags & 0x1)
		addr |= (1UL<<52);
	if (flags & 0x2)
		addr |= (1UL<<54);
	if (flags & 0x4)
		addr |= (1UL<<55);
	return addr;
}

static void mwv207d_gart_invalidate(struct mwv207d_gart *gart,
				   u64 base, u64 range)
{

	wmb();

	spin_lock(&gart->lock);
	gart_write(gart, 0x0, 0);
	gart_write(gart, 0x4, (u32)base);
	gart_write(gart, 0x8, (u32)(base >> 32));
	gart_write(gart, 0xC, (u32)range);
	gart_write(gart, 0x10, (u32)(range >> 32));
	gart_write(gart, 0x0, 1);
	spin_unlock(&gart->lock);
}

void mwv207d_gart_bind(struct mwv207d_device *mdev, u64 offset, u32 pages,
		      dma_addr_t *dma_addr, u32 flags)
{
	struct mwv207d_gart *gart = mdev->gart;
	dma_addr_t phys_addr;
	u64 *pte;
	int i, j;

	if (!gart)
		return;

	pte = &gart->pte[offset >> 12];
	for (i = 0; i < pages; i++) {
		phys_addr = dma_addr[i];
		for (j = 0; j < PAGE_SIZE / SZ_4K; j++) {
			*pte++ = gart_pte(phys_addr, flags);
			phys_addr += SZ_4K;
		}
	}

	mwv207d_gart_invalidate(gart, offset >> 12, gart_nr_pages(pages));
}

void mwv207d_gart_unbind(struct mwv207d_device *mdev, u64 offset, u32 pages)
{
	struct mwv207d_gart *gart = mdev->gart;
	u64 *pte;

	if (!gart)
		return;

	pte = &gart->pte[offset >> 12];
	memset(pte, 0, gart_nr_pages(pages) * 8);

	mwv207d_gart_invalidate(gart, offset >> 12, gart_nr_pages(pages));
}

static void mwv207d_gart_enable(struct mwv207d_gart *gart)
{
	gart_write(gart, 0x14, (u32)gart->pte_addr);
	gart_write(gart, 0x18, (u32)(gart->pte_addr >> 32));
	gart_write(gart, 0x1C, (u32)gart->safe_page_addr);
	gart_write(gart, 0x20, (u32)(gart->safe_page_addr >> 32));

	if (gart->mdev->hw.is_pf)
		mdev_write(gart->mdev, 0x1700800, 1);

	mwv207d_gart_invalidate(gart, 0, gart->nr_pte);
}

static void mwv207d_gart_disable(struct mwv207d_gart *gart)
{
	if (gart->mdev->hw.is_pf)
		mdev_write(gart->mdev, 0x1700800, 0);
}

void mwv207d_gart_fini(struct mwv207d_device *mdev)
{
	struct mwv207d_gart *gart = mdev->gart;

	if (!gart)
		return;

	mwv207d_gart_disable(gart);
	mdev->gart = NULL;
}

int mwv207d_gart_init(struct mwv207d_device *mdev)
{
	struct mwv207d_gart *gart;
	int ret;

	if (!mdev->hw.has_gart)
		return 0;

	BUG_ON(mdev->gtt_size > SZ_2G);

	gart = devm_kzalloc(mdev->dev, sizeof(*gart), GFP_KERNEL);
	if (!gart)
		return -ENOMEM;
	gart->mdev = mdev;
	gart->mmio = mdev->mmio +
		(mdev->hw.is_pf ? 0x2B8800 : 0x8000);
	spin_lock_init(&gart->lock);

	gart->nr_pte = mdev->gtt_size >> 12;
	gart->pte = dmam_alloc_coherent(mdev->dev, gart->nr_pte * 8,
					&gart->pte_addr,
					GFP_KERNEL | __GFP_ZERO);
	if (!gart->pte)
		return -ENOMEM;
	gart->safe_page = dmam_alloc_coherent(mdev->dev, SZ_4K,
					      &gart->safe_page_addr,
					      GFP_KERNEL | __GFP_ZERO);
	if (!gart->safe_page)
		return -ENOMEM;

	ret = dma_set_mask_and_coherent(mdev->dev, DMA_BIT_MASK(64));
	if (ret)
		return ret;

	mwv207d_gart_enable(gart);
	mdev->gart = gart;
	return 0;
}

int mwv207d_gart_suspend(struct mwv207d_device *mdev)
{
	return 0;
}

void mwv207d_gart_resume(struct mwv207d_device *mdev)
{
	if (!mdev->gart)
		return;

	mwv207d_gart_enable(mdev->gart);
}
