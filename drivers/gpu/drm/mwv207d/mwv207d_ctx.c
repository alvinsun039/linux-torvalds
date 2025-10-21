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

#include <linux/dma-fence.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/gpu_scheduler.h>
#include "mwv207d_drv.h"
#include "mwv207d_drm.h"
#include "mwv207d_ctx.h"
#include "mwv207d_vm.h"
#include "mwv207d_sched.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int mwv207d_sched_jobs = 32;
module_param(mwv207d_sched_jobs, int, 0444);
MODULE_PARM_DESC(mwv207d_sched_jobs, "max number of jobs in software queue(default=32)");

struct mwv207d_ctx *mwv207d_ctx_lookup(struct drm_device *dev,
				       struct drm_file *filp,
				       u32 handle)
{
	struct mwv207d_fpriv *fpriv = to_fpriv(filp);
	struct mwv207d_ctx_mgr *mgr = &fpriv->ctx_mgr;
	struct mwv207d_ctx *ctx;

	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->handle_table, handle);
	if (ctx)
		kref_get(&ctx->refcnt);
	mutex_unlock(&mgr->lock);

	return ctx;
}

static void mwv207d_ctx_entity_fini(struct mwv207d_ctx *ctx)
{
	int i, j, k;

	for (i = 0; i < 0x6; i++) {
		for (j = 0; j < 0x8 + 1; j++) {
			if (ctx->centities[i][j]) {
				for (k = 0; k < mwv207d_sched_jobs; k++)
					dma_fence_put(ctx->centities[i][j]->fences[k]);
				drm_sched_entity_destroy(&ctx->centities[i][j]->entity);
				kfree(ctx->centities[i][j]);
				ctx->centities[i][j] = NULL;
			}
		}
	}
}

static int mwv207d_ctx_entity_init_single(struct mwv207d_ctx *ctx,
					  struct drm_gpu_scheduler **sched,
					  struct mwv207d_ctx_entity **centities,
					  int nr)
{
	struct mwv207d_ctx_entity *centity;
	int i, ret;

	for (i = 0; i < nr; i++) {
		centity = kzalloc(struct_size(centity, fences, mwv207d_sched_jobs),
				 GFP_KERNEL);
		if (!centity)
			return -ENOMEM;
		mutex_init(&centity->qlock);
		ret = drm_sched_entity_init(&centity->entity,
					    DRM_SCHED_PRIORITY_NORMAL,
					    &sched[i], 1, &ctx->guilty);
		if (ret) {
			kfree(centity);
			return ret;
		}
		centities[i] = centity;
	}

	if (i) {
		centity = kzalloc(struct_size(centity, fences, mwv207d_sched_jobs),
				 GFP_KERNEL);
		if (!centity)
			return -ENOMEM;
		mutex_init(&centity->qlock);
		ret = drm_sched_entity_init(&centity->entity,
					    DRM_SCHED_PRIORITY_NORMAL,
					    sched, i, &ctx->guilty);
		if (ret) {
			kfree(centity);
			return ret;
		}
		centities[i] = centity;
	}

	return 0;
}

static int mwv207d_ctx_entity_init(struct drm_device *dev,
				   struct mwv207d_ctx *ctx)
{
	struct mwv207d_device *mdev = drm_to_mdev(dev);
	int ret;
	int i;

	for (i = 0; i < 0x6; i++) {
		ret = mwv207d_ctx_entity_init_single(ctx, mdev->sched[i],
						     &ctx->centities[i][0],
						     mdev->nr_pipe[i]);
		if (ret)
			goto fini;
	}

	return 0;
fini:
	mwv207d_ctx_entity_fini(ctx);
	return ret;
}

static int mwv207d_ctx_create(struct drm_device *dev,
			      struct mwv207d_ctx_mgr *mgr,
			      struct mwv207d_vm *vm,
			      u32 flags, u32 *handle)
{
	struct mwv207d_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL)
		return -ENOMEM;

	ctx->mdev = drm_to_mdev(dev);
	kref_init(&ctx->refcnt);

	ret = mwv207d_ctx_entity_init(dev, ctx);
	if (ret)
		goto out_free;

	ret = mutex_lock_interruptible(&mgr->lock);
	if (ret)
		goto out_fini_entites;
	ret = idr_alloc(&mgr->handle_table, ctx, 1, 0, GFP_KERNEL);
	mutex_unlock(&mgr->lock);

	if (ret < 0)
		goto out_fini_entites;

	ctx->vm = mwv207d_vm_get(vm);
	*handle = ret;
	return 0;
out_fini_entites:
	mwv207d_ctx_entity_fini(ctx);
out_free:
	kfree(ctx);
	return ret;
}

static void mwv207d_ctx_release(struct kref *kref)
{
	struct mwv207d_ctx *ctx = container_of(kref, struct mwv207d_ctx, refcnt);

	mwv207d_vm_put(ctx->vm);
	mwv207d_ctx_entity_fini(ctx);
	kfree(ctx);
}

int mwv207d_ctx_put(struct mwv207d_ctx *ctx)
{
	return	kref_put(&ctx->refcnt, mwv207d_ctx_release);
}

static int mwv207d_ctx_destroy(struct drm_device *dev,
			       struct mwv207d_ctx_mgr *mgr,
			       u32 handle)
{
	struct mwv207d_ctx *ctx;
	int ret;

	ret = mutex_lock_interruptible(&mgr->lock);
	if (ret)
		return ret;
	ctx = idr_remove(&mgr->handle_table, handle);
	mutex_unlock(&mgr->lock);

	if (ctx == NULL)
		return -EINVAL;

	mwv207d_ctx_put(ctx);
	return 0;
}

void mwv207d_ctx_add_fence(struct mwv207d_ctx *ctx,
			   struct drm_sched_entity *entity,
			   struct dma_fence *fence)
{
	struct mwv207d_ctx_entity *centity = to_mwv207d_ctx_entity(entity);
	struct dma_fence *prev;
	unsigned int idx;

	BUG_ON(!mutex_is_locked(&centity->qlock));

	idx = centity->sequence & (mwv207d_sched_jobs - 1);
	prev = centity->fences[idx];

	if (likely(prev)) {
		BUG_ON(!dma_fence_is_signaled(prev));
		dma_fence_put(prev);
	}

	centity->fences[idx] = dma_fence_get(fence);
	centity->sequence++;
}

int mwv207d_ctx_wait_prev_fence(struct mwv207d_ctx *ctx,
				struct drm_sched_entity *entity)
{
	struct mwv207d_ctx_entity *centity = to_mwv207d_ctx_entity(entity);
	struct dma_fence *prev;
	unsigned int idx;

	BUG_ON(!mutex_is_locked(&centity->qlock));

	idx = centity->sequence & (mwv207d_sched_jobs - 1);
	prev = centity->fences[idx];
	if (likely(prev))
		return dma_fence_wait(prev, true);

	return 0;
}

int  mwv207d_ctx_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *filp)
{
	struct drm_mwv207d_ctx *args = (struct drm_mwv207d_ctx *)data;
	struct mwv207d_fpriv *fpriv = to_fpriv(filp);

	if (args->resv || args->flags)
		return -EINVAL;

	switch (args->op) {
	case 0x0:
		return mwv207d_ctx_create(dev, &fpriv->ctx_mgr, fpriv->vm,
					  args->flags, &args->handle);
	case 0x1:
		return mwv207d_ctx_destroy(dev, &fpriv->ctx_mgr, args->handle);
	default:
		return -EINVAL;
	}
}

int mwv207d_ctx_mgr_init(struct drm_device *dev, struct mwv207d_ctx_mgr *mgr)
{
	mutex_init(&mgr->lock);
	idr_init(&mgr->handle_table);
	return 0;
}

void mwv207d_ctx_mgr_fini(struct drm_device *dev, struct mwv207d_ctx_mgr *mgr)
{
	struct mwv207d_ctx *ctx;
	uint32_t id;

	idr_for_each_entry(&mgr->handle_table, ctx, id)
		mwv207d_ctx_put(ctx);

	idr_destroy(&mgr->handle_table);
	mutex_destroy(&mgr->lock);
}

static struct drm_sched_entity *mwv207d_create_kentity(
		struct mwv207d_device *mdev, int type)
{
	struct drm_gpu_scheduler **sched = mdev->sched[type];
	int nr_pipe = mdev->nr_pipe[type];
	struct drm_sched_entity *entity;
	int ret;

	entity = devm_kzalloc(mdev->dev,
			      sizeof(struct drm_sched_entity),
			      GFP_KERNEL);
	if (!entity)
		return NULL;

	ret = drm_sched_entity_init(entity,
				    DRM_SCHED_PRIORITY_HIGH,
				    sched, nr_pipe, NULL);
	if (ret)
		return NULL;

	return entity;
}

int mwv207d_kctx_init(struct mwv207d_device *mdev)
{
	if (mwv207d_sched_jobs < 4) {
		pr_warn("jobs(%d) at least 4", mwv207d_sched_jobs);
		mwv207d_sched_jobs = 4;
	} else if (!is_power_of_2(mwv207d_sched_jobs)) {
		pr_warn("jobs(%d) must be power of 2", mwv207d_sched_jobs);
		mwv207d_sched_jobs = roundup_pow_of_two(mwv207d_sched_jobs);
	}

	mdev->blt_2d_entity = mwv207d_create_kentity(mdev, 0x3);
	mdev->blt_3d_entity = mwv207d_create_kentity(mdev, 0x0);

	if (!mdev->blt_2d_entity || !mdev->blt_3d_entity) {
		mwv207d_kctx_fini(mdev);
		return -ENOMEM;
	}
	mutex_init(&mdev->qlock);

	return 0;
}

void mwv207d_kctx_fini(struct mwv207d_device *mdev)
{
	if (mdev->blt_2d_entity)
		drm_sched_entity_destroy(mdev->blt_2d_entity);
	if (mdev->blt_3d_entity)
		drm_sched_entity_destroy(mdev->blt_3d_entity);
}
