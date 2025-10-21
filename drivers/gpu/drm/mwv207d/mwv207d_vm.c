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

#include <linux/interval_tree_generic.h>
#include <linux/bitmap.h>
#include <linux/vmalloc.h>
#include <drm/ttm/ttm_tt.h>
#include "mwv207d_drv.h"
#include "mwv207d_vm.h"
#include "mwv207d_bo.h"

#define MWV207D_PMD_IS_HUGE(entry) ((entry) & 0x8)

#define START(node)  ((node)->start)
#define LAST(node)   ((node)->last)
INTERVAL_TREE_DEFINE(struct mwv207d_mapping, rb, u64, __subtree_last,
		START, LAST, static, mwv207d_vm_it)

#undef START
#undef LAST

struct mwv207d_pa {
	dma_addr_t *dma_addr;
	unsigned long pages;
	u64 pa;
	int is_dma;
	u64 cursor;
};

static int mwv207d_pta_add_vm(struct mwv207d_pta *pta, struct mwv207d_vm *vm);

static void mwv207d_pta_del_vm(struct mwv207d_pta *pta, struct mwv207d_vm *vm);

static void mwv207d_vm_pa_init(struct mwv207d_pa *pa,
		dma_addr_t *dma_addr, unsigned long pages, u64 paddr, int is_dma)
{
	BUG_ON(is_dma && dma_addr == NULL);

	pa->dma_addr = dma_addr;
	pa->pages = pages;
	pa->pa = paddr;
	pa->is_dma = is_dma;
	pa->cursor = 0;
}

static void mwv207d_vm_pa_incr(struct mwv207d_pa *padesc, u64 incr)
{
	padesc->cursor += incr;
}

static u64 mwv207d_vm_pa_addr(struct mwv207d_pa *padesc)
{
	u64 addr;
	u32 idx;

	if (!padesc->is_dma)
		return padesc->pa + padesc->cursor;

	idx = padesc->cursor >> PAGE_SHIFT;
	addr =  padesc->dma_addr[idx] + (padesc->cursor & (PAGE_SIZE - 1));
	addr += 0x1000000000ULL;
	return addr;
}

static int mwv207d_pmd_hugetlb(u64 start, u64 next, u64 pa)
{
	return ((start | next | pa) & ((1 << 21) - 1)) ? 0 : 1;
}

static int mwv207d_vm_alloc_pgtable(struct mwv207d_device *mdev, struct mwv207d_pt *pt)
{
	spin_lock(&mdev->bitmap_lock);
	pt->bit = find_first_zero_bit(mdev->pt_bitmap, mdev->pt_bits);
	pt->entry = mdev->ap_vaddr + (pt->bit << 12);
	pt->phys_addr = pt->bit << 12;
	__set_bit(pt->bit, mdev->pt_bitmap);
	spin_unlock(&mdev->bitmap_lock);

	memset(pt->entry, 0, SZ_4K);
	return 0;
}

static void mwv207d_vm_free_pgtable(struct mwv207d_device *mdev, struct mwv207d_pt *pt)
{
	BUG_ON(!test_bit(pt->bit, mdev->pt_bitmap)
			|| !test_bit(0, mdev->pt_bitmap)
			|| !test_bit(1, mdev->pt_bitmap));
	spin_lock(&mdev->bitmap_lock);
	__clear_bit(pt->bit, mdev->pt_bitmap);
	spin_unlock(&mdev->bitmap_lock);
}

struct mwv207d_vm *mwv207d_vm_create(struct mwv207d_device *mdev)
{
	struct mwv207d_vm *vm;
	int ret;

	vm = kzalloc(sizeof(struct mwv207d_vm), GFP_KERNEL);
	if (!vm)
		return ERR_PTR(-ENOMEM);

	ret = mwv207d_vm_alloc_pgtable(mdev, &vm->pgd.pt);
	if (ret)
		goto free_vm;

	kref_init(&vm->refcnt);
	mutex_init(&vm->mutex);
	vm->mdev = mdev;
	vm->va = RB_ROOT_CACHED;

	ret = mwv207d_pta_add_vm(mdev->pta, vm);
	if (ret)
		goto free_pgd;

	vm->identity = atomic64_inc_return(&mdev->pta->identity);
	vm->pgd.pt.entry[510] = vm->identity;

	return vm;
free_pgd:
	mwv207d_vm_free_pgtable(mdev, &vm->pgd.pt);
free_vm:
	kfree(vm);
	return ERR_PTR(ret);
}

void mwv207d_vm_reserve(struct mwv207d_vm *vm, u64 start, u64 size)
{
	struct mwv207d_mapping *mapping;

	mutex_lock(&vm->mutex);

	mapping = &vm->resvd[vm->nr_reserved_maps++];
	BUG_ON(vm->nr_reserved_maps > 0x10);

	mapping->start = round_down(start, PAGE_SIZE);
	mapping->last = round_up(start + size, PAGE_SIZE) - 1;
	mwv207d_vm_it_insert(mapping, &vm->va);

	mutex_unlock(&vm->mutex);
}

static void mwv207d_vm_destroy_pte(struct mwv207d_device *mdev,
				   struct mwv207d_pte *pte)
{
	mwv207d_vm_free_pgtable(mdev, &pte->pt);
	kfree(pte);
}

static void mwv207d_vm_destroy_pmd(struct mwv207d_device *mdev,
				   struct mwv207d_pmd *pmd)
{
	int i;

	for (i = 0; i < 512; ++i) {
		if (pmd->pte[i])
			mwv207d_vm_destroy_pte(mdev, pmd->pte[i]);
	}
	mwv207d_vm_free_pgtable(mdev, &pmd->pt);
	kfree(pmd);
}

static void mwv207d_vm_destroy_pud(struct mwv207d_device *mdev,
				   struct mwv207d_pud *pud)
{
	int i;

	for (i = 0; i < 512; ++i) {
		if (pud->pmd[i])
			mwv207d_vm_destroy_pmd(mdev, pud->pmd[i]);
	}
	mwv207d_vm_free_pgtable(mdev, &pud->pt);
	kfree(pud);
}

static void mwv207d_vm_destroy(struct kref *kref)
{
	struct mwv207d_vm *vm = container_of(kref, struct mwv207d_vm, refcnt);
	int i;

	mwv207d_pta_del_vm(vm->mdev->pta, vm);

	BUG_ON(vm->pgd.pt.entry[510] != vm->identity);
	BUG_ON((((-1UL) >> 0x27) & 0x1) > 1);
	for (i = 0; i < 2; i++) {
		if (vm->pgd.pud[i])
			mwv207d_vm_destroy_pud(vm->mdev, vm->pgd.pud[i]);
	}

	mwv207d_vm_free_pgtable(vm->mdev, &vm->pgd.pt);
	kfree(vm);
}

struct mwv207d_vm *mwv207d_vm_get(struct mwv207d_vm *vm)
{
	kref_get(&vm->refcnt);
	return vm;
}

void mwv207d_vm_put(struct mwv207d_vm *vm)
{
	kref_put(&vm->refcnt, mwv207d_vm_destroy);
}

static int mwv207d_vm_ensure_pte(struct mwv207d_device *mdev,
				 struct mwv207d_pmd *pmd, int i)
{
	struct mwv207d_pte *pte;
	int ret;

	if (pmd->pte[i])
		return 0;

	pte = kzalloc(sizeof(struct mwv207d_pte), GFP_KERNEL);
	if (!pte)
		return -ENOMEM;
	ret = mwv207d_vm_alloc_pgtable(mdev, &pte->pt);
	if (ret) {
		kfree(pte);
		return ret;
	}

	pmd->pte[i] = pte;
	pmd->pt.entry[i] = pte->pt.phys_addr | 0x1;
	pmd->nr_active++;
	return 0;
}

static void mwv207d_vm_free_pte(struct mwv207d_device *mdev,
				struct mwv207d_pte *pte)
{
	if (!pte)
		return;
	mwv207d_vm_free_pgtable(mdev, &pte->pt);
	kfree(pte);
}

static void mwv207d_vm_try_free_pte(struct mwv207d_device *mdev,
				 struct mwv207d_pmd *pmd, int i)
{
	struct mwv207d_pte *pte = pmd->pte[i];

	if (!pte || pte->nr_active > 0)
		return;
	BUG_ON(pte->nr_active < 0);

	mwv207d_vm_free_pte(mdev, pte);
	pmd->pt.entry[i] = 0;
	pmd->pte[i] = NULL;
	pmd->nr_active--;
}

static int mwv207d_vm_ensure_pmd(struct mwv207d_device *mdev,
				 struct mwv207d_pud *pud, int i)
{
	struct mwv207d_pmd *pmd;
	int ret;

	if (pud->pmd[i])
		return 0;

	pmd = kzalloc(sizeof(struct mwv207d_pmd), GFP_KERNEL);
	if (!pmd)
		return -ENOMEM;
	ret = mwv207d_vm_alloc_pgtable(mdev, &pmd->pt);
	if (ret) {
		kfree(pmd);
		return ret;
	}

	pud->pmd[i] = pmd;
	pud->pt.entry[i] = pmd->pt.phys_addr | 0x1;
	pud->nr_active++;
	return 0;
}

static void mwv207d_vm_try_free_pmd(struct mwv207d_device *mdev,
				 struct mwv207d_pud *pud, int i)
{
	struct mwv207d_pmd *pmd = pud->pmd[i];

	if (!pmd || pmd->nr_active > 0)
		return;
	BUG_ON(pmd->nr_active < 0);

	mwv207d_vm_free_pgtable(mdev, &pmd->pt);
	kfree(pmd);
	pud->pt.entry[i] = 0;
	pud->pmd[i] = NULL;
	pud->nr_active--;
}

static int mwv207d_vm_ensure_pud(struct mwv207d_device *mdev,
				 struct mwv207d_pgd *pgd, int i)
{
	struct mwv207d_pud *pud;
	int ret;

	if (pgd->pud[i])
		return 0;

	pud = kzalloc(sizeof(struct mwv207d_pud), GFP_KERNEL);
	if (!pud)
		return -ENOMEM;
	ret = mwv207d_vm_alloc_pgtable(mdev, &pud->pt);
	if (ret) {
		kfree(pud);
		return ret;
	}

	pgd->pud[i] = pud;
	pgd->pt.entry[i] = pud->pt.phys_addr | 0x1;
	return 0;
}

static void mwv207d_vm_try_free_pud(struct mwv207d_device *mdev,
				 struct mwv207d_pgd *pgd, int i)
{
	struct mwv207d_pud *pud = pgd->pud[i];

	if (!pud || pud->nr_active > 0)
		return;
	BUG_ON(pud->nr_active < 0);

	mwv207d_vm_free_pgtable(mdev, &pud->pt);
	kfree(pud);
	pgd->pt.entry[i] = 0;
	pgd->pud[i] = NULL;
}

static int mwv207d_vm_map_pte(struct mwv207d_device *mdev,
			      struct mwv207d_pte *pte,
			      u64 start, u64 end,
			      struct mwv207d_pa *pa, u32 flags)
{
	u64 next, phys_addr;
	u32 perm;
	int i;

	perm = (flags & 0x1) ? 0x1 : 0x11;
	do {
		next = round_up(start + 1, (u64)1 << 0xC);
		next = min_t(u64, next, end);
		i = (((start) >> 0xC) & 0x1ff);
		if (!pte->pt.entry[i])
			pte->nr_active++;
		phys_addr = mwv207d_vm_pa_addr(pa);
		pte->pt.entry[i] = round_down(phys_addr, 1 << 0xC)    | perm;
	} while (mwv207d_vm_pa_incr(pa, next - start), start = next, start != end);

	return 0;
}

static int mwv207d_vm_map_pmd(struct mwv207d_device *mdev, struct mwv207d_pmd *pmd,
			      u64 start, u64 end,
			      struct mwv207d_pa *pa, u32 flags)
{
	u64 next, phys_addr;
	int ret, i;
	u32 attr;

	attr = (flags & 0x1) ? 0x1 : 0x11;
	attr |= 0x8;
	do {
		next = round_up(start + 1, (u64)1 << 0x15);
		next = min_t(u64, next, end);
		i = (((start) >> 0x15) & 0x1ff);
		phys_addr = mwv207d_vm_pa_addr(pa);

		if (!pa->is_dma && mwv207d_pmd_hugetlb(start, next, phys_addr)) {

			if (pmd->pt.entry[i]) {
				mwv207d_vm_free_pte(mdev, pmd->pte[i]);
				pmd->pte[i] = NULL;
			} else
				pmd->nr_active++;
			pmd->pt.entry[i] = phys_addr | attr;
			mwv207d_vm_pa_incr(pa, next - start);
		} else {

			if (MWV207D_PMD_IS_HUGE(pmd->pt.entry[i])) {
				pmd->nr_active--;
				pmd->pt.entry[i] = 0;

				BUG_ON(!mwv207d_pmd_hugetlb(start, next, 0));
			}

			ret = mwv207d_vm_ensure_pte(mdev, pmd, i);
			if (ret)
				return ret;
			ret = mwv207d_vm_map_pte(mdev, pmd->pte[i],
					start, next, pa, flags);
			if (ret)
				return ret;
		}
	} while (start = next, start != end);

	return 0;
}

static int mwv207d_vm_map_pud(struct mwv207d_device *mdev,
			      struct mwv207d_pud *pud,
			      u64 start, u64 end,
			      struct mwv207d_pa *pa, u32 flags)
{
	int ret, i;
	u64 next;

	do {
		next = round_up(start + 1, (u64)1 << 0x1E);
		next = min_t(u64, next, end);
		i = (((start) >> 0x1E) & 0x1ff);
		ret = mwv207d_vm_ensure_pmd(mdev, pud, i);
		if (ret)
			return ret;
		ret = mwv207d_vm_map_pmd(mdev, pud->pmd[i],
					start, next, pa, flags);
		if (ret)
			return ret;
	} while (start = next, start != end);

	return 0;
}

static int mwv207d_vm_map(struct mwv207d_vm *vm, u64 va, u64 size,
		   struct mwv207d_pa *pa, u32 flags)
{
	struct mwv207d_pgd *pgd = &vm->pgd;
	u64 start, end, next;
	int ret, i;

	mutex_lock(&vm->mutex);
	for (start = va, end = va + size; start < end; start = next) {
		next = round_up(start + 1, (u64)1 << 0x27);
		next = min_t(u64, next, end);
		i = (((start) >> 0x27) & 0x1);
		ret = mwv207d_vm_ensure_pud(vm->mdev, pgd, i);
		if (ret)
			goto unlock;
		ret = mwv207d_vm_map_pud(vm->mdev, pgd->pud[i],
					start, next, pa, flags);
		if (ret)
			goto unlock;
	}
	ret = 0;
unlock:

	mb();
	vm->dirty = 0xffffffff;
	mutex_unlock(&vm->mutex);
	return ret;
}

int mwv207d_vm_map_linear(struct mwv207d_vm *vm, u64 va, u64 size,
			  u64 paddr, u32 flags)
{
	struct mwv207d_pa pa;

	BUG_ON(size & (SZ_4K - 1));

	if (vm == vm->mdev->vm && va < 0x1000000000ULL) {
		size += va - round_down(va, SZ_2M);
		size = round_up(size, SZ_2M);
		va = round_down(va, SZ_2M);
		paddr = round_down(paddr, SZ_2M);

		BUG_ON(va != paddr);
	}

	mwv207d_vm_pa_init(&pa, NULL, 0, paddr, 0);
	return mwv207d_vm_map(vm, va, size, &pa, flags);
}

int mwv207d_vm_map_pages(struct mwv207d_vm *vm, u64 va, unsigned long pages,
					 dma_addr_t *dma_addr, u32 flags)
{
	struct mwv207d_pa pa;

	mwv207d_vm_pa_init(&pa, dma_addr, pages, 0, 1);
	return mwv207d_vm_map(vm, va, pages << PAGE_SHIFT, &pa, flags);
}

static void mwv207d_vm_zap_pte(struct mwv207d_device *mdev,
			      struct mwv207d_pte *pte,
			      u64 start, u64 end)
{
	u64 next;
	int i;

	if (!pte)
		return;
	do {
		next = round_up(start + 1, (u64)1 << 0xC);
		next = min_t(u64, next, end);
		i = (((start) >> 0xC) & 0x1ff);
		if (pte->pt.entry[i])
			pte->nr_active--;
		pte->pt.entry[i] = 0;
	} while (start = next, start != end);
}

static void mwv207d_vm_zap_pmd(struct mwv207d_device *mdev, struct mwv207d_pmd *pmd,
			      u64 start, u64 end)
{
	u64 next;
	int i;

	if (!pmd)
		return;
	do {
		next = round_up(start + 1, (u64)1 << 0x15);
		next = min_t(u64, next, end);
		i = (((start) >> 0x15) & 0x1ff);
		if (MWV207D_PMD_IS_HUGE(pmd->pt.entry[i])) {

			BUG_ON(!mwv207d_pmd_hugetlb(start, next, 0));
			pmd->pt.entry[i] = 0;
			pmd->nr_active--;
		} else {
			mwv207d_vm_zap_pte(mdev, pmd->pte[i], start, next);
			mwv207d_vm_try_free_pte(mdev, pmd, i);
		}
	} while (start = next, start != end);
}

static void mwv207d_vm_zap_pud(struct mwv207d_device *mdev,
			      struct mwv207d_pud *pud,
			      u64 start, u64 end)
{
	u64 next;
	int  i;

	if (!pud)
		return;
	do {
		next = round_up(start + 1, (u64)1 << 0x1E);
		next = min_t(u64, next, end);
		i = (((start) >> 0x1E) & 0x1ff);
		mwv207d_vm_zap_pmd(mdev, pud->pmd[i], start, next);
		mwv207d_vm_try_free_pmd(mdev, pud, i);
	} while (start = next, start != end);
}

void mwv207d_vm_zap_linear(struct mwv207d_vm *vm, u64 va, unsigned long pages)
{
	struct mwv207d_pgd *pgd = &vm->pgd;
	u64 start, end, next;
	int i;

	mutex_lock(&vm->mutex);
	for (start = va, end = va + pages * PAGE_SIZE;
	     start < end; start = next) {
		next = round_up(start + 1, (u64)1 << 0x27);
		next = min_t(u64, next, end);
		i = (((start) >> 0x27) & 0x1);
		mwv207d_vm_zap_pud(vm->mdev, pgd->pud[i], start, next);
		mwv207d_vm_try_free_pud(vm->mdev, pgd, i);
	}

	mb();
	vm->dirty = 0xffffffff;
	mutex_unlock(&vm->mutex);
}

static int mwv207d_vm_map_bo_ex(struct mwv207d_vm *vm, struct mwv207d_bo *bo,
		u64 pg_off, unsigned long num_pages,
		u64 va, u32 flags)
{
	struct ttm_resource *res = bo->tbo.resource;
	u64 start = (res->start + pg_off) << PAGE_SHIFT;

	switch (res->mem_type) {
	case TTM_PL_VRAM:
		return mwv207d_vm_map_linear(vm, va,
				num_pages << PAGE_SHIFT, start, flags);
	case TTM_PL_TT:
		if (vm->mdev->gart)
			return mwv207d_vm_map_linear(vm, va, num_pages << PAGE_SHIFT,
					start + 0x1000000000ULL, flags);
		else
			return mwv207d_vm_map_pages(vm, va, num_pages,
					bo->tbo.ttm->dma_address + pg_off, flags);

	default:
		BUG_ON(1);
	}

	return 0;
}
int mwv207d_vm_map_bo(struct mwv207d_vm *vm, struct mwv207d_bo *bo, u64 va, u32 flags)
{
	struct ttm_resource *res = bo->tbo.resource;

	return mwv207d_vm_map_bo_ex(vm, bo, 0, res->size >> PAGE_SHIFT, va, flags);
}

int mwv207d_vm_update_mapping(struct mwv207d_vm *vm, struct mwv207d_bo *bo,
		struct mwv207d_mapping *mapping)
{
	struct ttm_resource *res = bo->tbo.resource;
	u32 num_pages;

	dma_resv_assert_held(bo->tbo.base.resv);

	num_pages = ((mapping->last + 1) - mapping->start) >> PAGE_SHIFT;
	BUG_ON(num_pages > (res->size >> PAGE_SHIFT) - (mapping->offset >> PAGE_SHIFT));

	mapping->updated_once = true;
	return mwv207d_vm_map_bo_ex(vm, bo, mapping->offset >> PAGE_SHIFT,
			num_pages, mapping->start, mapping->flags);
}

void mwv207d_vm_erase_mapping(struct mwv207d_vm *vm, struct mwv207d_mapping *mapping)
{
	mwv207d_vm_zap_linear(vm, mapping->start,
			(mapping->last + 1 - mapping->start) >> PAGE_SHIFT);
}

void mwv207d_vm_unmap(struct mwv207d_vm *vm, u64 offset, unsigned long pages)
{
	mwv207d_vm_zap_linear(vm, offset, pages);
}

int mwv207d_pta_init(struct mwv207d_device *mdev)
{
	u32 size = sizeof(struct mwv207d_mmu_desc) * 0x1000;
	struct mwv207d_pta *pta;
	int index, start, pta_bits;

	pta = devm_kzalloc(mdev->dev, sizeof(struct mwv207d_pta), GFP_KERNEL);
	if (!pta)
		return -ENOMEM;

	size = ALIGN(size + SZ_4K, SZ_4K);
	pta_bits = size / SZ_4K;

	BUG_ON(pta_bits > BITS_PER_LONG);
	index = BITS_TO_LONGS(mdev->pt_bits) - 1;
	BUG_ON(mdev->pt_bitmap[index]);

	start = index * BITS_PER_LONG;
	bitmap_set(mdev->pt_bitmap, start, pta_bits);
	pta->desc = mdev->ap_vaddr + start * SZ_4K;
	pta->phys_addr = start * SZ_4K;

	memset(pta->desc + 0x1000, 0x18, SZ_4K);
	pta->safe_pg_addr = pta->phys_addr + size;

	ida_init(&pta->pt_id);
	atomic64_set(&pta->identity, 0);
	spin_lock_init(&pta->lock);

	mdev->pta = pta;
	return 0;
}

void mwv207d_pta_fini(struct mwv207d_device *mdev)
{
	ida_destroy(&mdev->pta->pt_id);
}

static int mwv207d_pta_add_vm(struct mwv207d_pta *pta, struct mwv207d_vm *vm)
{
	unsigned long flags;
	u64 phys_addr;
	int id;

	phys_addr = vm->pgd.pt.phys_addr;
	BUG_ON((phys_addr >> 32) & ~0xff);
	spin_lock_irqsave(&pta->lock, flags);
	id = ida_alloc_range(&pta->pt_id, 0, 0x1000 - 1, GFP_KERNEL);
	if (id < 0) {
		spin_unlock_irqrestore(&pta->lock, flags);
		return id;
	}

	vm->pta_id = id;

	BUG_ON(phys_addr & 0xfff);
	pta->desc[id].low = phys_addr & 0xffffffff;
	pta->desc[id].high = phys_addr >> 32;
	spin_unlock_irqrestore(&pta->lock, flags);

	return 0;
}

static void mwv207d_pta_del_vm(struct mwv207d_pta *pta, struct mwv207d_vm *vm)
{
	unsigned long flags;
	int id = vm->pta_id;

	spin_lock_irqsave(&pta->lock, flags);
	pta->desc[id].low = 0;
	pta->desc[id].high = 0;
	ida_free(&pta->pt_id, id);
	spin_unlock_irqrestore(&pta->lock, flags);
}

void mwv207d_mmu_init(struct mwv207d_device *mdev, struct mwv207d_vm *vm,
		      void (*mmu_write)(void *priv, u32 offset, u32 val),
		      void *priv)
{
	u64 addr = mdev->pta->phys_addr;
	u32 ext;

	BUG_ON(addr & ~0xfffffff000ULL);
	BUG_ON(vm->pta_id & ~0xffff);

	mmu_write(priv, 0x380, 0);
	mmu_write(priv, 0x384, 0);

	mmu_write(priv, 0x38C, (u32)(addr & 0xffffffff));
	mmu_write(priv, 0x390, (u32)(addr >> 32));
	mmu_write(priv, 0x394, 0x1000);

	addr = mdev->pta->safe_pg_addr;
	ext = (addr >> 32) & 0xff;
	mmu_write(priv, 0x398, addr);
	mmu_write(priv, 0x39C, addr);
	mmu_write(priv, 0x3A0, ext | (ext << 16));

	mmu_write(priv, 0x3AC, 0x222200);

	mmu_write(priv, 0x1B4, 0x80000000);
	mmu_write(priv, 0x1AC, vm->pta_id);

	dma_wmb();

	mmu_write(priv, 0x388, 0x1);
}

void mwv207d_mmu_flush(struct mwv207d_device *mdev, struct mwv207d_vm *vm,
		       void (*mmu_write)(void *priv, u32 offset, u32 val),
		       void *priv)
{
	mmu_write(priv, 0x1B4, 0x80000000);
	mmu_write(priv, 0x1AC, vm->pta_id);
}

struct mwv207d_bo_vm *mwv207d_bo_vm_find(struct mwv207d_bo *bo,
		struct mwv207d_vm *vm)
{
	struct mwv207d_bo_vm *bo_vm;

	dma_resv_assert_held(bo->tbo.base.resv);

	list_for_each_entry(bo_vm, &bo->bo_vm_list, bo_vm_node) {
		if (bo_vm->vm != vm)
			continue;
		return bo_vm;
	}

	return NULL;
}

int mwv207d_vm_validate_bo_vm(struct mwv207d_bo *bo, struct mwv207d_bo_vm *bo_vm)
{
	struct mwv207d_mapping *mapping;
	int ret;

	dma_resv_assert_held(bo->tbo.base.resv);

	if (bo_vm->moved) {
		list_for_each_entry(mapping, &bo_vm->valid_maps, list) {
			ret = mwv207d_vm_update_mapping(bo_vm->vm, bo, mapping);
			if (ret)
				return ret;
		}
		bo_vm->moved = false;
	}

	list_for_each_entry(mapping, &bo_vm->pending_maps, list) {
		ret = mwv207d_vm_update_mapping(bo_vm->vm, bo, mapping);
		if (ret)
			return ret;
	}

	list_splice_init(&bo_vm->pending_maps, &bo_vm->valid_maps);

	return 0;
}

void mwv207d_vm_invalidate_bo(struct mwv207d_bo *bo, bool erase)
{
	struct mwv207d_mapping *mapping;
	struct mwv207d_bo_vm *bo_vm;

	dma_resv_assert_held(bo->tbo.base.resv);

	list_for_each_entry(bo_vm, &bo->bo_vm_list, bo_vm_node) {
		bo_vm->moved = true;
		if (!erase)
			continue;

		BUG_ON(!dma_resv_test_signaled(bo->tbo.base.resv, DMA_RESV_USAGE_BOOKKEEP));
		list_for_each_entry(mapping, &bo_vm->valid_maps, list)
			mwv207d_vm_erase_mapping(bo_vm->vm, mapping);
	}
}

int mwv207d_mapping_create(struct mwv207d_bo_vm *bo_vm, u64 offset,
		u64 saddr, u64 size, u32 flags)
{
	struct mwv207d_mapping *mapping;
	int ret;

	ret = mutex_lock_interruptible(&bo_vm->vm->mutex);
	if (ret)
		return ret;

	mapping = mwv207d_vm_it_iter_first(&bo_vm->vm->va, saddr, saddr + size - 1);
	if (mapping) {
		ret = -EEXIST;
		goto unlock;
	}

	mapping = kzalloc(sizeof(struct mwv207d_mapping), GFP_KERNEL);
	if (!mapping) {
		ret = -ENOMEM;
		goto unlock;
	}

	mapping->start = saddr;
	mapping->last = saddr + size - 1;
	mapping->offset = offset;
	mapping->vm = bo_vm->vm;
	mapping->updated_once = false;
	if (flags & 0x2)
		mapping->flags = 0;
	else
		mapping->flags = 0x1;

	kref_init(&mapping->refcnt);
	list_add(&mapping->list, &bo_vm->pending_maps);

	mwv207d_vm_it_insert(mapping, &bo_vm->vm->va);

unlock:
	mutex_unlock(&bo_vm->vm->mutex);
	return ret;
}

int mwv207d_mapping_remove(struct mwv207d_bo_vm *bo_vm,
		u64 va_start, u64 va_last)
{
	struct mwv207d_mapping *mapping, *tmp;

	list_for_each_entry_safe(mapping, tmp, &bo_vm->pending_maps, list) {
		if (mapping->start == va_start && mapping->last == va_last) {
			list_del(&mapping->list);
			mwv207d_mapping_put(mapping);
			return 0;
		}
	}

	list_for_each_entry_safe(mapping, tmp, &bo_vm->valid_maps, list) {
		if (mapping->start == va_start && mapping->last == va_last) {
			list_del(&mapping->list);
			mwv207d_mapping_put(mapping);
			return 0;
		}
	}

	return -ENOENT;
}

struct mwv207d_mapping *mwv207d_mapping_get(struct mwv207d_mapping *mapping)
{
	kref_get(&mapping->refcnt);
	return mapping;
}

static void mwv207d_mapping_destroy(struct kref *kref)
{
	struct mwv207d_mapping *mapping = container_of(kref,
			struct mwv207d_mapping, refcnt);
	struct mwv207d_vm *vm = mapping->vm;

	if (mapping->updated_once)
		mwv207d_vm_erase_mapping(vm, mapping);

	mutex_lock(&vm->mutex);
	mwv207d_vm_it_remove(mapping, &vm->va);
	mutex_unlock(&vm->mutex);

	kfree(mapping);
}

void mwv207d_mapping_put(struct mwv207d_mapping *mapping)
{
	kref_put(&mapping->refcnt, mwv207d_mapping_destroy);
}

int mwv207d_vm_suspend(struct mwv207d_device *mdev)
{
	mdev->pgtable_segment = vmalloc(mdev->pgtable_segment_size);
	if (!mdev->pgtable_segment)
		return -ENOMEM;

	memcpy(mdev->pgtable_segment, mdev->ap_vaddr, mdev->pgtable_segment_size);
	return 0;
}

void mwv207d_vm_resume(struct mwv207d_device *mdev)
{
	memcpy(mdev->ap_vaddr, mdev->pgtable_segment, mdev->pgtable_segment_size);
	vfree(mdev->pgtable_segment);
}
