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
#include <linux/io-mapping.h>
#include <drm/drm_file.h>
#include <linux/dma-resv.h>

#include "mwv207d_bo.h"
#include "mwv207d_gem.h"
#include "mwv207d_drm.h"

vm_fault_t mwv207d_bo_fault_reserve_notify(struct ttm_buffer_object *bo)
{
	struct ttm_operation_ctx ctx = { false, false };
	struct mwv207d_bo *mbo;
	unsigned long offset;
	int ret;

	if (!mwv207d_ttm_bo_is_mwv207d_bo(bo))
		return 0;
	mbo = to_mbo(bo);

	if (bo->resource->mem_type != TTM_PL_VRAM)
		return 0;

	offset = bo->resource->start << PAGE_SHIFT;
	if ((offset + bo->base.size) <= mbo->mdev->ap_size)
		return 0;

	if (bo->pin_count > 0)
		return VM_FAULT_SIGBUS;

	mbo->flags |= 0x1;

	mwv207d_bo_placement_from_domain(mbo, 0x2 |
					 0x4);

	mbo->placement.num_busy_placement = 1;
	mbo->placement.busy_placement = &mbo->placements[1];

	ret = ttm_bo_validate(bo, &mbo->placement, &ctx);
	if (unlikely(ret == -EBUSY || ret == -ERESTARTSYS))
		return VM_FAULT_NOPAGE;
	else if (unlikely(ret))
		return VM_FAULT_SIGBUS;

	ttm_bo_move_to_lru_tail_unlocked(bo);

	return 0;
}

static void mwv207d_ttm_bo_destroy(struct ttm_buffer_object *bo)
{
	struct mwv207d_bo *mbo = to_mbo(bo);

	if (bo->base.import_attach)
		drm_prime_gem_destroy(&bo->base, bo->sg);
	drm_gem_object_release(&bo->base);
	kfree(mbo);
}

bool mwv207d_ttm_bo_is_mwv207d_bo(struct ttm_buffer_object *mbo)
{
	return mbo->destroy == &mwv207d_ttm_bo_destroy;
}

int mwv207d_bo_kmap(struct mwv207d_bo *mbo, void **ptr)
{
	bool is_iomem;
	int ret;

	if (mbo->kptr) {
		if (ptr)
			*ptr = mbo->kptr;
		return 0;
	}
	ret = ttm_bo_kmap(&mbo->tbo, 0, mbo->tbo.resource->size >> PAGE_SHIFT,
			  &mbo->kmap);
	if (ret)
		return ret;
	mbo->kptr = ttm_kmap_obj_virtual(&mbo->kmap, &is_iomem);
	if (ptr)
		*ptr = mbo->kptr;
	return 0;
}

void mwv207d_bo_kunmap(struct mwv207d_bo *mbo)
{
	if (!mbo->kptr)
		return;
	mbo->kptr = NULL;
	ttm_bo_kunmap(&mbo->kmap);
}

int mwv207d_bo_pin(struct mwv207d_bo *mbo, u32 domain, u64 *gpu_addr)
{
	struct ttm_operation_ctx ctx = {false, false};
	struct mwv207d_device *mdev = mbo->mdev;
	int ret, i;

	if (mbo->tbo.pin_count) {
		ttm_bo_pin(&mbo->tbo);
		if (gpu_addr)
			*gpu_addr = mwv207d_bo_gpu_offset(mbo);
		return 0;
	}

	if (mbo->prime_shared_count && domain == 0x2)
		return -EINVAL;

	mwv207d_bo_placement_from_domain(mbo, domain);
	for (i = 0; i < mbo->placement.num_placement; i++) {

		if ((mbo->placements[i].mem_type == TTM_PL_VRAM) &&
		    !(mbo->flags & 0x2))
			mbo->placements[i].lpfn = mdev->ap_size >> PAGE_SHIFT;
		else
			mbo->placements[i].lpfn = 0;
	}

	ret = ttm_bo_validate(&mbo->tbo, &mbo->placement, &ctx);
	if (unlikely(ret)) {
		dev_err(mdev->dev, "error, bo pin failed\n");
		return ret;
	}

	ttm_bo_pin(&mbo->tbo);
	if (gpu_addr)
		*gpu_addr = mwv207d_bo_gpu_offset(mbo);
	if (domain == 0x2)
		mdev->vram_pinned_bytes += mwv207d_bo_size(mbo);
	else
		mdev->gtt_pinned_bytes += mwv207d_bo_size(mbo);

	return 0;
}

void mwv207d_bo_unpin(struct mwv207d_bo *bo)
{
	ttm_bo_unpin(&bo->tbo);

	if (!bo->tbo.pin_count) {
		if (bo->tbo.resource->mem_type == TTM_PL_VRAM)
			bo->mdev->vram_pinned_bytes -= mwv207d_bo_size(bo);
		else
			bo->mdev->gtt_pinned_bytes -= mwv207d_bo_size(bo);
	}
}

struct mwv207d_bo *mwv207d_bo_ref(struct mwv207d_bo *bo)
{
	if (bo)
		ttm_bo_get(&bo->tbo);
	return bo;
}

void mwv207d_bo_unref(struct mwv207d_bo **bo)
{
	struct ttm_buffer_object *tbo;

	if ((*bo) == NULL)
		return;
	tbo = &((*bo)->tbo);
	ttm_bo_put(tbo);
	*bo = NULL;
}

void mwv207d_bo_placement_from_domain(struct mwv207d_bo *mbo, u32 domain)
{
	struct mwv207d_device *mdev = mbo->mdev;
	u32 c = 0;

	mbo->placement.placement = mbo->placements;
	mbo->placement.busy_placement = mbo->placements;

	if (domain & 0x2) {
		mbo->placements[c].fpfn = 0;
		mbo->placements[c].lpfn = 0;
		mbo->placements[c].mem_type = TTM_PL_VRAM;
		mbo->placements[c].flags = 0;

		if (mbo->flags & 0x1)
			mbo->placements[c].lpfn = mdev->ap_size >> PAGE_SHIFT;
		else
			mbo->placements[c].flags |= TTM_PL_FLAG_TOPDOWN;

		mbo->placements[c].fpfn = mdev->pgtable_segment_size >> PAGE_SHIFT;
		c++;
	}

	if (domain & 0x4) {
		mbo->placements[c].fpfn = 0;
		mbo->placements[c].lpfn = 0;
		mbo->placements[c].mem_type = TTM_PL_TT;
		mbo->placements[c++].flags = 0;
	}

	if (domain & 0x1) {
		mbo->placements[c].fpfn = 0;
		mbo->placements[c].lpfn = 0;
		mbo->placements[c].mem_type = TTM_PL_SYSTEM;
		mbo->placements[c++].flags = 0;
	}
	if (!c) {
		mbo->placements[c].fpfn = 0;
		mbo->placements[c].lpfn = 0;
		mbo->placements[c].mem_type = TTM_PL_SYSTEM;
		mbo->placements[c++].flags = 0;
	}

	mbo->placement.num_placement = c;
	mbo->placement.num_busy_placement = c;
}

int mwv207d_bo_create(struct mwv207d_device *mdev, u64 size,
		      u64 byte_align, bool kernel, u32 domain, u32 flags,
		      struct sg_table *sg, struct dma_resv *resv,
		      struct mwv207d_bo **bo_ptr)
{
	unsigned long page_align = roundup(byte_align, PAGE_SIZE) >> PAGE_SHIFT;
	enum ttm_bo_type type;
	struct mwv207d_bo *mbo;
	int ret;

	size = ALIGN(size, PAGE_SIZE);

	if (kernel)
		type = ttm_bo_type_kernel;
	else if (sg)
		type = ttm_bo_type_sg;
	else
		type = ttm_bo_type_device;
	*bo_ptr = NULL;

	mbo = kzalloc(sizeof(struct mwv207d_bo), GFP_KERNEL);
	if (mbo == NULL)
		return -ENOMEM;
	drm_gem_private_object_init(&mdev->base, &mbo->tbo.base, size);
	mbo->mdev = mdev;
	mbo->domain = domain & MWV207D_GEM_DOMAIN_MASK;
	mbo->flags = flags;
	INIT_LIST_HEAD(&mbo->bo_vm_list);

	mwv207d_bo_placement_from_domain(mbo, domain);

	ret = ttm_bo_init_validate(&mdev->bdev, &mbo->tbo, type,
			  &mbo->placement, page_align, !kernel,
			  sg, resv, &mwv207d_ttm_bo_destroy);
	if (unlikely(ret != 0))
		return ret;

	*bo_ptr = mbo;

	return 0;
}

int mwv207d_bo_reserve(struct mwv207d_bo *mbo, bool no_intr)
{
	int ret;

	ret = ttm_bo_reserve(&mbo->tbo, !no_intr, false, NULL);
	if (unlikely(ret != 0)) {
		if (ret != -ERESTARTSYS)
			dev_err(mbo->mdev->dev, "error, bo reserve failed\n");
	}

	return ret;
}

void mwv207d_bo_unreserve(struct mwv207d_bo *mbo)
{
	ttm_bo_unreserve(&mbo->tbo);
}

int mwv207d_bo_wait(struct mwv207d_bo *mbo, bool no_wait)
{
	int ret;
	struct ttm_operation_ctx ctx = { true, no_wait };

	ret = ttm_bo_reserve(&mbo->tbo, true, no_wait, NULL);
	if (unlikely(ret)) {
		if (ret != -ERESTARTSYS && ret != -EBUSY)
			dev_err(mbo->mdev->dev,
				"error, bo reserve failed for wait\n");
		return ret;
	}
	ret = ttm_bo_wait_ctx(&mbo->tbo, &ctx);
	ttm_bo_unreserve(&mbo->tbo);
	return ret;
}

int mwv207d_bo_create_pin_mapped(struct mwv207d_device *mdev,
				 u64 size, u64 align, u32 domain, u32 flags,
				 struct mwv207d_bo **mbop, u64 *pap, void **vap)
{
	struct mwv207d_bo *mbo;
	void *va;
	u64 pa;
	int ret;

	ret = mwv207d_bo_create(mdev, size, align, true,
				domain, flags, NULL, NULL, &mbo);
	if (ret)
		return ret;
	ret = mwv207d_bo_reserve(mbo, true);
	if (ret)
		goto free_bo;
	ret = mwv207d_bo_pin(mbo, domain, &pa);
	if (ret)
		goto unreserve_bo;
	ret = mwv207d_bo_kmap(mbo, &va);
	if (ret)
		goto unpin_bo;
	mwv207d_bo_unreserve(mbo);
	if (mbop)
		*mbop = mbo;
	if (pap)
		*pap = pa;
	if (vap)
		*vap = va;
	return 0;

unpin_bo:
	mwv207d_bo_unpin(mbo);
unreserve_bo:
	mwv207d_bo_unreserve(mbo);
free_bo:
	mwv207d_bo_unref(&mbo);
	return ret;
}

void mwv207d_bo_destroy_pin_mapped(struct mwv207d_bo *mbo)
{
	int ret;

	ret = mwv207d_bo_reserve(mbo, true);
	if (ret) {
		pr_warn("failed to reserve bo: %d", ret);
		return;
	}
	mwv207d_bo_kunmap(mbo);
	mwv207d_bo_unpin(mbo);
	mwv207d_bo_unreserve(mbo);
	mwv207d_bo_unref(&mbo);
}

int mwv207d_bo_set_tiling_flags(struct mwv207d_bo *bo, u64 tiling_flags)
{
	BUG_ON(bo->tbo.type == ttm_bo_type_kernel);

	bo->tiling_flags = tiling_flags;

	return 0;
}

void mwv207d_bo_get_tiling_flags(struct mwv207d_bo *bo, u64 *tiling_flags)
{
	BUG_ON(bo->tbo.type == ttm_bo_type_kernel);

	dma_resv_assert_held(bo->tbo.base.resv);

	if (tiling_flags)
		*tiling_flags = bo->tiling_flags;
}

int mwv207d_bo_set_metadata(struct mwv207d_bo *bo, void *metadata,
			    u32 metadata_size, u64 flags)
{
	void *buffer;

	BUG_ON(bo->tbo.type == ttm_bo_type_kernel);

	if (!metadata_size) {
		if (bo->metadata_size) {
			kfree(bo->metadata);
			bo->metadata = NULL;
			bo->metadata_size = 0;
		}
		return 0;
	}

	if (!metadata)
		return -EINVAL;

	buffer = kmemdup(metadata, metadata_size, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;
	kfree(bo->metadata);
	bo->metadata_flags = flags;
	bo->metadata = buffer;
	bo->metadata_size = metadata_size;

	return 0;
}

int mwv207d_bo_get_metadata(struct mwv207d_bo *bo, void *buffer,
			    size_t buffer_size, u32 *metadata_size,
			    u64 *flags)
{
	if (!buffer && !metadata_size)
		return -EINVAL;

	BUG_ON(bo->tbo.type == ttm_bo_type_kernel);

	if (metadata_size)
		*metadata_size = bo->metadata_size;

	if (buffer) {
		if (buffer_size < bo->metadata_size)
			return -EINVAL;
		if (bo->metadata_size)
			memcpy(buffer, bo->metadata, bo->metadata_size);
	}

	if (flags)
		*flags = bo->metadata_flags;

	return 0;
}
