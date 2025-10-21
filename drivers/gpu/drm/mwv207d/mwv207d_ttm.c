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
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/swap.h>

#include <drm/drm.h>
#include <drm/drm_print.h>
#include <drm/drm_file.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_cache.h>
#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_placement.h>
#include <drm/ttm/ttm_range_manager.h>
#include <drm/ttm/ttm_tt.h>

#include "mwv207d_drv.h"
#include "mwv207d_bo.h"
#include "mwv207d_vm.h"
#include "mwv207d_sched.h"
#include "mwv207d_drm.h"
#include "mwv207d_monitor.h"
#include "mwv207d_gart.h"

static int mwv207d_2d_blt = 1;
module_param(mwv207d_2d_blt, int, 0444);
MODULE_PARM_DESC(mwv207d_2d_blt, "ttm move by 2d, enabled by default");

struct mwv207d_ttm_tt {
	struct ttm_tt ttm;
	u64 offset;
	bool bound;
};
#define ttm_to_gtt(_ttm) container_of(_ttm, struct mwv207d_ttm_tt, ttm)

static int mwv207d_ttm_tt_bind(struct ttm_device *bdev,
			       struct ttm_tt *ttm,
			       struct ttm_resource *bo_mem)
{
	struct mwv207d_ttm_tt *gtt = ttm_to_gtt(ttm);
	struct mwv207d_device *mdev = bdev_to_mdev(bdev);
	uint32_t flags = 0x1 |
			 0x2 |
			 0x4;
	int ret;

	if (gtt->bound)
		return 0;
	WARN(!ttm->num_pages, "no pages to bind!\n");

	gtt->offset = (unsigned long)(bo_mem->start << PAGE_SHIFT);

	mwv207d_gart_bind(mdev, gtt->offset, ttm->num_pages,
				gtt->ttm.dma_address, flags);

	if (mdev->gart)
		ret = mwv207d_vm_map_linear(mdev->vm,
				gtt->offset + 0x1000000000ULL,
				ttm->num_pages * PAGE_SIZE,
				gtt->offset + 0x1000000000ULL, 0);
	else
		ret = mwv207d_vm_map_pages(mdev->vm,
				gtt->offset + 0x1000000000ULL,
				ttm->num_pages,
				gtt->ttm.dma_address, 0);

	if (ret) {
		pr_err("failed to map ttm at 0x%llx, size = %lu\n",
			gtt->offset, ttm->num_pages * PAGE_SIZE);
		mwv207d_gart_unbind(mdev, gtt->offset, ttm->num_pages);
		return ret;
	}

	gtt->bound = true;
	return 0;
}

static void mwv207d_ttm_tt_unbind(struct ttm_device *bdev,
				  struct ttm_tt *ttm)
{
	struct mwv207d_device *mdev = bdev_to_mdev(bdev);
	struct mwv207d_ttm_tt *gtt = ttm_to_gtt(ttm);

	if (!gtt->bound)
		return;

	mwv207d_vm_unmap(mdev->vm, gtt->offset + 0x1000000000ULL,
			ttm->num_pages);
	mwv207d_gart_unbind(mdev, gtt->offset, ttm->num_pages);

	gtt->bound = false;
}

static struct ttm_tt *mwv207d_ttm_tt_create(struct ttm_buffer_object *bo,
					    uint32_t page_flags)
{
	struct mwv207d_ttm_tt *gtt;

	gtt = kzalloc(sizeof(*gtt), GFP_KERNEL);
	if (!gtt)
		return NULL;

	if (ttm_sg_tt_init(&gtt->ttm, bo, page_flags, ttm_cached)) {
		kfree(gtt);
		return NULL;
	}

	return &gtt->ttm;
}

static void mwv207d_ttm_tt_destroy(struct ttm_device *bdev, struct ttm_tt *tt)
{
	struct mwv207d_ttm_tt *gtt = ttm_to_gtt(tt);

	ttm_tt_fini(tt);
	kfree(gtt);
}

static int mwv207d_ttm_tt_populate(struct ttm_device *bdev, struct ttm_tt *ttm,
				   struct ttm_operation_ctx *ctx)
{
	return ttm_pool_alloc(&bdev->pool, ttm, ctx);
}

static void mwv207d_ttm_tt_unpopulate(struct ttm_device *bdev,
				      struct ttm_tt *ttm)
{
	mwv207d_ttm_tt_unbind(bdev, ttm);
	ttm_pool_free(&bdev->pool, ttm);
}

static void mwv207d_ttm_evict_flags(struct ttm_buffer_object *bo,
				    struct ttm_placement *placement)
{
	static const struct ttm_place placements = {
		.fpfn = 0,
		.lpfn = 0,
		.mem_type = TTM_PL_SYSTEM,
		.flags = 0
	};
	struct mwv207d_device *mdev;
	struct mwv207d_bo *mbo;

	if (!mwv207d_ttm_bo_is_mwv207d_bo(bo)) {
		placement->placement = &placements;
		placement->busy_placement = &placements;
		placement->num_placement = 1;
		placement->num_busy_placement = 1;
		return;
	}

	mbo = to_mbo(bo);
	mdev = bdev_to_mdev(bo->bdev);

	switch (bo->resource->mem_type) {
	case TTM_PL_VRAM:
		if (mdev->ap_size < mdev->hw.vram_size &&
		    bo->resource->start < (mdev->ap_size >> PAGE_SHIFT)) {
			unsigned int fpfn = mdev->ap_size >> PAGE_SHIFT;
			int i;

			mbo->flags &= ~0x1;
			mwv207d_bo_placement_from_domain(mbo,
					0x2 |
					0x4);
			mbo->placement.num_busy_placement = 0;

			for (i = 0; i < mbo->placement.num_placement; i++) {
				if (mbo->placements[i].mem_type == TTM_PL_VRAM) {
					if (mbo->placements[i].fpfn < fpfn)
						mbo->placements[i].fpfn = fpfn;
				} else {
					mbo->placement.busy_placement =
						&mbo->placements[i];
					mbo->placement.num_busy_placement = 1;
				}
			}
		} else
			mwv207d_bo_placement_from_domain(mbo, 0x4);
		break;
	case TTM_PL_TT:
	default:
		mwv207d_bo_placement_from_domain(mbo, 0x1);
	}
	*placement = mbo->placement;
}

static int mwv207d_ttm_job_submit(struct mwv207d_device *mdev, struct mwv207d_job *mjob,
				  struct dma_fence **fence)
{
	int ret;

	mutex_lock(&mdev->qlock);
	ret = drm_sched_job_init(&mjob->base, mjob->engine_entity, NULL);
	if (ret)
		goto out;

	drm_sched_job_arm(&mjob->base);
	*fence = &mjob->base.s_fence->finished;

	mwv207d_job_get(mjob);
	drm_sched_entity_push_job(&mjob->base);
out:
	mutex_unlock(&mdev->qlock);
	return ret;
}

static struct mwv207d_job *mwv207d_ttm_job_alloc(struct ttm_buffer_object *bo)
{
	struct mwv207d_job *mjob;
	struct mwv207d_tvb *mtvb;
	int ret;

	mjob = mwv207d_job_alloc();
	if (!mjob)
		return ERR_PTR(-ENOMEM);

	mjob->mtvb = kzalloc(sizeof(struct mwv207d_tvb), GFP_KERNEL);
	if (!mjob->mtvb) {
		ret = -ENOMEM;
		goto err;
	}
	mtvb = &mjob->mtvb[0];
	list_add_tail(&mtvb->base.head, &mjob->tvblist);

	ret = mwv207d_job_add_resv_fence(mjob, bo->base.resv, 1);
	if (ret)
		goto err;

	return mjob;
err:
	mwv207d_job_put(mjob);
	return ERR_PTR(ret);
}

static int mwv207d_ttm_init_2d_job(struct mwv207d_device *mdev,
				   struct mwv207d_job *mjob,
				   u64 src_base, u64 dst_base, u64 size)
{
	u64 width, height, slice, remain;
	u32 *start, *cmd;
	u32 cmd_size;

	height = size / SZ_4K;
	slice = (height / 0x7fff + 1);
	cmd_size = slice * 128 + 128;
	start = kvmalloc(cmd_size, GFP_KERNEL);
	if (!start)
		return -ENOMEM;

	width = SZ_4K / 4;
	cmd = start;

	*cmd++ = 0x40000004;
	*cmd++ = ffs(mdev->hw.nr_2d_clusters);
	*cmd++ = 0x40000400;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000404;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000420;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000424;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000440;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000444;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000460;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000464;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000480;
	*cmd++ = 0x00000000;
	*cmd++ = 0x40000484;
	*cmd++ = 0x00000000;
	for (remain = height; remain > 0; remain -= height) {
		height = min_t(u64, 0x7fff, remain);
		*cmd++ = 0x40000030;
		*cmd++ = 0x00000060;
		*cmd++ = 0x4000000c;
		*cmd++ = 0x00000060;

		*cmd++ = 0x40000070;
		*cmd++ = ((src_base >> 8) & 0xffff);
		*cmd++ = 0x40000074;
		*cmd++ = ((dst_base >> 8) & 0xffff);

		*cmd++ = 0x8200001c;
		*cmd++ = (0x2 << 16) | 0xcc;
		*cmd++ = (dst_base >> 24) | ((width * 4 / 256) << 16);
		*cmd++ = 0x00000000;
		*cmd++ = (height << 16) | width;
		*cmd++ = (src_base >> 24) | ((width * 4 / 256) << 16);
		*cmd++ = 0x00000000;
		*cmd++ = (height << 16) | width;

		*cmd++ = 0x00000000;
		*cmd++ = 0x00000000;
		*cmd++ = 0x00000000;
		*cmd++ = 0x00000000;
		*cmd++ = 0x00000000;

		*cmd++ = 0x81000000;

		src_base += height * width * 4;
		dst_base += height * width * 4;
	}

	mjob->cmd_mode = 0x0;
	mjob->cmd_ptr = (char *)start;
	mjob->cmd_size = (unsigned long)cmd - (unsigned long)start;
	BUG_ON(mjob->cmd_size > cmd_size);
	mjob->engine_entity = mdev->blt_2d_entity;

	return 0;
}

static u32 *mwv207d_ttm_3d_emit(u32 *cmd, u32 reg, u32 val)
{
	*cmd++ = 0x08010000 | reg;
	*cmd++ = val;
	return cmd;
}

static u32 *mwv207d_ttm_3d_emit_addr(u32 *cmd, u32 hi_reg, u32 lo_reg, u64 addr)
{
	cmd = mwv207d_ttm_3d_emit(cmd, hi_reg, addr >> 32);
	cmd =  mwv207d_ttm_3d_emit(cmd, lo_reg, addr & 0xffffffff);
	return cmd;
}

static int mwv207d_ttm_init_3d_job(struct mwv207d_device *mdev,
				   struct mwv207d_job *mjob,
				   u64 src_base, u64 dst_base, u64 size)
{
	u64 len, slice, remain, src_addr, dst_addr, chunk, cluster_len;
	u32 *start, *cmd;
	int i;

	slice = (size / SZ_2G) + 1;
	start = kvmalloc(slice * 200 * mdev->hw.nr_3d_clusters, GFP_KERNEL);
	if (!start)
		return -ENOMEM;

	cmd = start;
	for (remain = size; remain > 0; remain -= chunk) {
		chunk = min_t(u64, SZ_2G, remain);
		src_addr = src_base;
		dst_addr = dst_base;

		cluster_len = chunk / mdev->hw.nr_3d_clusters;
		len = chunk - cluster_len * (mdev->hw.nr_3d_clusters - 1);
		BUG_ON(len == 0 || cluster_len == 0);

		cmd = mwv207d_ttm_3d_emit(cmd, 0x502e, 1);
		for (i = 0; i < mdev->hw.nr_3d_clusters; i++) {

			cmd = mwv207d_ttm_3d_emit(cmd, 0x50ce, 1 << i);
			cmd = mwv207d_ttm_3d_emit_addr(cmd, 0x50fb, 0x5000, src_addr);
			cmd = mwv207d_ttm_3d_emit_addr(cmd, 0x50fd, 0x5006, dst_addr);
			cmd = mwv207d_ttm_3d_emit(cmd, 0x5015, len);

			src_addr += len;
			dst_addr += len;
			len = cluster_len;
		}

		cmd = mwv207d_ttm_3d_emit(cmd, 0x50ce, (1 << mdev->hw.nr_3d_clusters) - 1);
		cmd = mwv207d_ttm_3d_emit_addr(cmd, 0x619d, 0x5004, 0);
		cmd = mwv207d_ttm_3d_emit_addr(cmd, 0x619e, 0x5008, 0);
		cmd = mwv207d_ttm_3d_emit(cmd, 0x5018, 0x3);
		cmd = mwv207d_ttm_3d_emit(cmd, 0x502e, 0);

		src_base += chunk;
		dst_base += chunk;
	}

	mjob->cmd_mode = 0x0;
	mjob->cmd_ptr = (char *)start;
	mjob->cmd_size = (unsigned long)cmd - (unsigned long)start;
	BUG_ON(mjob->cmd_size > slice * 200 * mdev->hw.nr_3d_clusters);
	mjob->engine_entity = mdev->blt_3d_entity;

	return 0;
}

static int mwv207d_bo_move_blit(struct ttm_buffer_object *bo, bool evict,
				struct ttm_resource *new_mem)
{
	struct mwv207d_device *mdev;
	struct mwv207d_job *mjob;
	struct dma_fence *fence;
	u64 src_base, dst_base, size;
	int ret = 0;

	mdev = bdev_to_mdev(bo->bdev);
	mjob = mwv207d_ttm_job_alloc(bo);
	if (IS_ERR(mjob))
		return PTR_ERR(mjob);

	src_base = bo->resource->start << PAGE_SHIFT;
	dst_base = new_mem->start << PAGE_SHIFT;
	size = (u64)new_mem->size;
	if (bo->resource->mem_type == TTM_PL_TT)
		src_base += 0x1000000000ULL;
	else {
		ret = mwv207d_vm_map_linear(mdev->vm, src_base, size,
					    src_base, 0);
		if (ret)
			goto out;
	}

	if (new_mem->mem_type == TTM_PL_TT)
		dst_base += 0x1000000000ULL;
	else {
		ret = mwv207d_vm_map_linear(mdev->vm, dst_base, size,
					    dst_base, 0);
		if (ret)
			goto out;
	}

	if (mwv207d_2d_blt)
		ret = mwv207d_ttm_init_2d_job(mdev, mjob, src_base, dst_base,
				bo->resource->size);
	else
		ret = mwv207d_ttm_init_3d_job(mdev, mjob, src_base, dst_base,
				bo->resource->size);
	if (ret)
		goto out;

	ret = mwv207d_ttm_job_submit(mdev, mjob, &fence);
	if (!ret)
		ret = ttm_bo_move_accel_cleanup(bo, fence, evict, true, new_mem);
out:
	mwv207d_job_put(mjob);
	return ret;
}

static void mwv207d_update_vram_usage(struct ttm_buffer_object *bo, struct ttm_resource *mem, bool add)
{
	struct mwv207d_device *mdev = bdev_to_mdev(bo->bdev);
	u64 visible_size = 0;
	u64 start = (u64)mem->start << PAGE_SHIFT;

	if (start < mdev->ap_size)
		visible_size = min_t(u64, bo->base.size, mdev->ap_size - start);

	if (add) {
		atomic64_add(bo->base.size, &mdev->vram_used_bytes);
		atomic64_add(visible_size, &mdev->visible_vram_used_bytes);
	} else {
		atomic64_sub(bo->base.size, &mdev->vram_used_bytes);
		atomic64_sub(visible_size, &mdev->visible_vram_used_bytes);
	}

}

void mwv207d_update_memory_usage(struct ttm_buffer_object *bo,
				 struct ttm_resource *mem, bool add)
{
	struct mwv207d_device *mdev;

	if (!mem || !mwv207d_ttm_bo_is_mwv207d_bo(bo))
		return;
	mdev = bdev_to_mdev(bo->bdev);

	switch (mem->mem_type) {
	case TTM_PL_TT:
		if (add)
			atomic64_add(bo->base.size, &mdev->gtt_used_bytes);
		else
			atomic64_sub(bo->base.size, &mdev->gtt_used_bytes);
		break;
	case TTM_PL_VRAM:
		mwv207d_update_vram_usage(bo, mem, add);
		break;
	}
}

static int mwv207d_bo_move(struct ttm_buffer_object *bo, bool evict,
			   struct ttm_operation_ctx *ctx,
			   struct ttm_resource *new_mem,
			   struct ttm_place *hop)
{
	struct ttm_resource *old_mem = bo->resource;
	struct mwv207d_device *mdev = bdev_to_mdev(bo->bdev);
	int ret;

	if (WARN_ON_ONCE(bo->pin_count > 0))
		return -EINVAL;

	ret = ttm_bo_wait_ctx(bo, ctx);
	if (ret)
		return ret;

	if (new_mem->mem_type == TTM_PL_TT) {
		ret = mwv207d_ttm_tt_bind(bo->bdev, bo->ttm, new_mem);
		if (ret)
			return ret;
	}

	if (!old_mem || (old_mem->mem_type == TTM_PL_SYSTEM && bo->ttm == NULL)) {
		ttm_bo_move_null(bo, new_mem);
		goto out;
	}
	mwv207d_vm_invalidate_bo(to_mbo(bo), old_mem->mem_type == TTM_PL_TT);

	mwv207d_update_memory_usage(bo, bo->resource, false);

	if (old_mem->mem_type == TTM_PL_SYSTEM && bo->ttm == NULL) {
		ttm_bo_move_null(bo, new_mem);
		goto out;
	}

	if (old_mem->mem_type == TTM_PL_SYSTEM && new_mem->mem_type == TTM_PL_TT) {
		ttm_bo_move_null(bo, new_mem);
		goto out;
	}

	if (old_mem->mem_type == TTM_PL_TT &&
	    new_mem->mem_type == TTM_PL_SYSTEM) {
		mwv207d_ttm_tt_unbind(bo->bdev, bo->ttm);
		ttm_bo_move_null(bo, new_mem);
		goto out;
	}

	if ((old_mem->mem_type == TTM_PL_SYSTEM && new_mem->mem_type == TTM_PL_VRAM) ||
	    (old_mem->mem_type == TTM_PL_VRAM && new_mem->mem_type == TTM_PL_SYSTEM)) {
		hop->fpfn = 0;
		hop->lpfn = 0;
		hop->mem_type = TTM_PL_TT;
		hop->flags = 0;
		ret = -EMULTIHOP;
		goto out;
	}

	ret = mwv207d_bo_move_blit(bo, evict, new_mem);
	if (ret) {
		ret = ttm_bo_move_memcpy(bo, ctx, new_mem);
		if (ret)
			goto out;
	}

	atomic64_add(bo->base.size, &mdev->moved_bytes);
out:
	mwv207d_update_memory_usage(bo, bo->resource, true);
	return ret;
}

static void mwv207d_ttm_delete_mem_notify(struct ttm_buffer_object *bo)
{
	mwv207d_update_memory_usage(bo, bo->resource, false);
}

static int mwv207d_ttm_io_mem_reserve(struct ttm_device *bdev,
				      struct ttm_resource *mem)
{
	struct mwv207d_device *mdev = bdev_to_mdev(bdev);
	size_t bus_size = mem->size;

	switch (mem->mem_type) {
	case TTM_PL_SYSTEM:
	case TTM_PL_TT:
		break;
	case TTM_PL_VRAM:
		mem->bus.offset = mem->start << PAGE_SHIFT;

		if ((mem->bus.offset + bus_size) > mdev->ap_size)
			return -EINVAL;
		mem->bus.addr = (u8 *)mdev->ap_vaddr + mem->bus.offset;
		mem->bus.offset += mdev->ap_base;
		mem->bus.is_iomem = true;
		mem->bus.caching = ttm_write_combined;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int mwv207d_ttm_access_memory(struct ttm_buffer_object *bo,
				     unsigned long offset, void *buf,
				     int len, int write)
{
	unsigned long pos = offset + mwv207d_bo_gpu_offset(to_mbo(bo));
	struct mwv207d_device *mdev = bdev_to_mdev(bo->bdev);
	void __iomem *addr = mdev->ap_vaddr + pos;
	int size;

	if (bo->resource->mem_type != TTM_PL_VRAM)
		return -EIO;

	if (pos >= mdev->visible_vram_size) {
		pr_err("%s offset(%lu) over range", __func__, pos);
		return -EIO;
	}

	size = min((u64)(pos + len), mdev->visible_vram_size) - pos;
	if (len - size)
		pr_warn("%s: warning, need %d bytes, but %d bytes accessible",
			__func__, len, size);

	if (write)
		memcpy_toio(addr, buf, size);
	else
		memcpy_fromio(buf, addr, size);

	return size;
}

static struct ttm_device_funcs bo_driver = {
	.ttm_tt_create       = mwv207d_ttm_tt_create,
	.ttm_tt_destroy      = mwv207d_ttm_tt_destroy,
	.ttm_tt_populate     = mwv207d_ttm_tt_populate,
	.ttm_tt_unpopulate   = mwv207d_ttm_tt_unpopulate,
	.eviction_valuable   = ttm_bo_eviction_valuable,
	.evict_flags         = mwv207d_ttm_evict_flags,
	.move                = mwv207d_bo_move,
	.delete_mem_notify   = mwv207d_ttm_delete_mem_notify,
	.io_mem_reserve      = mwv207d_ttm_io_mem_reserve,
	.access_memory       = mwv207d_ttm_access_memory,
};

static int mwv207d_ttm_init(struct mwv207d_device *mdev)
{
	int ret;

	ret = ttm_device_init(&mdev->bdev, &bo_driver, mdev->dev,
			      mdev->base.anon_inode->i_mapping,
			      mdev->base.vma_offset_manager,
			      mdev->hw.has_gart ? false : true,
			      mdev->hw.has_gart ? false : true);
	if (ret)
		return ret;
	ret = ttm_range_man_init(&mdev->bdev, TTM_PL_VRAM,
				 false, mdev->hw.vram_size >> PAGE_SHIFT);
	if (ret)
		goto dev_fini;
	ret = ttm_range_man_init(&mdev->bdev, TTM_PL_TT,
				 true, mdev->gtt_size >> PAGE_SHIFT);
	if (ret)
		goto vram_range_fini;

	DRM_INFO("%lldM of VRAM\n", mdev->hw.vram_size >> 20);
	DRM_INFO("%lldM of GTT\n", mdev->gtt_size >> 20);

	return 0;
vram_range_fini:
	ttm_range_man_fini(&mdev->bdev, TTM_PL_VRAM);
dev_fini:
	ttm_device_fini(&mdev->bdev);
	return ret;
}

static void mwv207d_ttm_fini(struct mwv207d_device *mdev)
{
	ttm_range_man_fini(&mdev->bdev, TTM_PL_TT);
	ttm_range_man_fini(&mdev->bdev, TTM_PL_VRAM);
	ttm_device_fini(&mdev->bdev);
}

int mwv207d_mm_init(struct mwv207d_device *mdev)
{
	int ret;

	ret = mwv207d_gart_init(mdev);
	if (ret)
		return ret;

	ret = mwv207d_ttm_init(mdev);
	if (ret)
		goto gart_fini;

	ret = mwv207d_monitor_init(mdev);
	if (ret)
		goto ttm_fini;

	ret = mwv207d_pta_init(mdev);
	if (ret)
		goto monitor_fini;

	mdev->vm = mwv207d_vm_create(mdev);
	if (IS_ERR(mdev->vm)) {
		ret = PTR_ERR(mdev->vm);
		goto pta_fini;
	}

	return 0;
pta_fini:
	mwv207d_pta_fini(mdev);
monitor_fini:
	mwv207d_monitor_fini(mdev);
ttm_fini:
	mwv207d_ttm_fini(mdev);
gart_fini:
	mwv207d_gart_fini(mdev);
	return ret;
}

void mwv207d_mm_fini(struct mwv207d_device *mdev)
{
	mwv207d_vm_put(mdev->vm);
	mwv207d_pta_fini(mdev);
	mwv207d_monitor_fini(mdev);
	mwv207d_ttm_fini(mdev);
	mwv207d_gart_fini(mdev);
}

int mwv207d_mm_suspend(struct mwv207d_device *mdev)
{
	int ret;

	ret = mwv207d_monitor_suspend(mdev);
	if (ret)
		return ret;

	ret = ttm_resource_manager_evict_all(&mdev->bdev,
			ttm_manager_type(&mdev->bdev, TTM_PL_VRAM));
	if (ret)
		goto resume_monitor;
	ret = ttm_resource_manager_evict_all(&mdev->bdev,
			ttm_manager_type(&mdev->bdev, TTM_PL_TT));
	if (ret)
		goto resume_monitor;

	ret = mwv207d_vm_suspend(mdev);
	if (ret)
		goto resume_monitor;

	ret = mwv207d_gart_suspend(mdev);
	if (ret)
		goto resume_vm;

	return 0;

resume_vm:
	mwv207d_vm_resume(mdev);
resume_monitor:
	mwv207d_monitor_resume(mdev);
	return ret;
}

void mwv207d_mm_resume(struct mwv207d_device *mdev)
{
	mwv207d_gart_resume(mdev);
	mwv207d_vm_resume(mdev);
	mwv207d_monitor_resume(mdev);
}
