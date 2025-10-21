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
#include <linux/moduleparam.h>
#include <drm/drm_gem.h>
#include <linux/dma-resv.h>
#include <linux/sync_file.h>
#include "mwv207d_drv.h"
#include "mwv207d_sched.h"
#include "mwv207d_bo.h"
#include "mwv207d_ctx.h"
#include "mwv207d_vm.h"
#include "mwv207d_db.h"

static int dump_hang_job = 1;
module_param(dump_hang_job, int, 0644);
MODULE_PARM_DESC(dump_hang_job, "dump time out job");

static unsigned long pipe_usage_period = 50;
module_param(pipe_usage_period, ulong, 0444);
MODULE_PARM_DESC(pipe_usage_period, "pipe usage calculation period in ms");

struct mwv207d_sched_info {
	struct mwv207d_pipe *(*create)(struct mwv207d_device *mdev,
			     const struct mwv207d_pipe_info *info);
	unsigned int hw_submission;
	unsigned int hang_limit;
	long timeout;
	struct mwv207d_pipe_info pipes[0x8];
};

static struct mwv207d_sched_info mwv207d_sched_pipe_info[0x6] = {
	[0x0] = {
		.create = mwv207d_pipe_3d_create,
		.hw_submission = 4,
		.hang_limit = 4,
		.timeout = 2000,
		.pipes = {
			{ "mwv207d_3d0", 0, 0x40000, 64, 0 },
		}
	},
	[0x1] = {
		.create = mwv207d_pipe_codec_create,
		.hw_submission = 1,
		.hang_limit = 4,
		.timeout = 2000,
		.pipes = {
			{ "mwv207d_dec0", 2, 0x180000, 104, 5 },
			{ "mwv207d_dec1", 3, 0x1c0000, 108, 6 },
		}
	},
	[0x2] = {
		.create = mwv207d_pipe_codec_create,
		.hw_submission = 1,
		.hang_limit = 4,
		.timeout = 2000,
		.pipes = {
			{ "mwv207d_enc0", 0, 0x100000, 96, 3 },
			{ "mwv207d_enc1", 1, 0x140000, 100, 4 },
		}
	},
	[0x3] = {
		.create = mwv207d_pipe_2d_create,
		.hw_submission = 4,
		.hang_limit = 4,
		.timeout = 2000,
		.pipes = {
			{ "mwv207d_2d0", 0, 0x88000, 72, 1 },
		}
	},
	[0x5] = {
		.create = mwv207d_pipe_2d_create,
		.hw_submission = 4,
		.hang_limit = 4,
		.timeout = 2000,
		.pipes = {
			{ "mwv207d_fus0", 1, 0xC8000, 128, 7 },
		}
	},
};

static u32 mwv207d_pipe_update_usage(struct mwv207d_pipe *pipe)
{
	struct mwv207d_pipe_usage *usage = &pipe->usage;
	unsigned long total_time;
	unsigned long busy_time;
	unsigned long delta;
	ktime_t last;
	ktime_t now;

	now = ktime_get();
	last = usage->last_update_time;

	delta = ktime_to_ms(ktime_sub(now, usage->period_start));

	if (usage->busy_count > 0)
		usage->busy_time += ktime_sub(now, last);
	else
		usage->idle_time += ktime_sub(now, last);

	usage->last_update_time = now;

	if (delta > pipe_usage_period) {
		usage->last_busy_time = usage->busy_time;
		usage->last_idle_time = usage->idle_time;
		usage->busy_time = 0;
		usage->idle_time = 0;
		usage->period_start = now;
	}

	total_time = ktime_to_ns(ktime_add(usage->last_busy_time,
					   usage->last_idle_time));
	busy_time = ktime_to_ns(usage->last_busy_time);

	if (likely(total_time))
		return (u32)div64_u64(busy_time, total_time / 100);
	return 100;
}

u32 mwv207d_pipe_get_usage(struct mwv207d_device *mdev,
			   u8 pipe_type, u8 pipe_id)
{
	struct mwv207d_pipe *pipe;
	unsigned long flags;
	u32 usage;

	BUG_ON(pipe_type >= 0x6);
	BUG_ON(pipe_id >= 0x8);

	if (pipe_id >= mdev->nr_pipe[pipe_type])
		return 0;
	if (!mdev->sched[pipe_type][pipe_id])
		return 0;

	pipe = to_mwv207d_sched(mdev->sched[pipe_type][pipe_id])->pipe;

	spin_lock_irqsave(&pipe->usage.lock, flags);

	usage = mwv207d_pipe_update_usage(pipe);

	spin_unlock_irqrestore(&pipe->usage.lock, flags);

	return usage;
}

void mwv207d_pipe_record_busy(struct mwv207d_pipe *pipe)
{
	unsigned long flags;

	spin_lock_irqsave(&pipe->usage.lock, flags);

	mwv207d_pipe_update_usage(pipe);

	pipe->usage.busy_count++;

	spin_unlock_irqrestore(&pipe->usage.lock, flags);
}

void mwv207d_pipe_record_idle(struct mwv207d_pipe *pipe)
{
	unsigned long flags;

	spin_lock_irqsave(&pipe->usage.lock, flags);

	mwv207d_pipe_update_usage(pipe);

	WARN_ON(--pipe->usage.busy_count < 0);

	spin_unlock_irqrestore(&pipe->usage.lock, flags);
}

static enum drm_mwv207d_info_key sched_type_to_key(int type)
{
	switch (type) {
	case 0x0:
		return DRM_MWV207D_ACTIVE_3D_NR;
	case 0x1:
		return DRM_MWV207D_ACTIVE_DEC_NR;
	case 0x2:
		return DRM_MWV207D_ACTIVE_ENC_NR;
	case 0x3:
		return DRM_MWV207D_ACTIVE_2D_NR;
	case 0x4:
		return DRM_MWV207D_ACTIVE_DMA_NR;
	case 0x5:
		return DRM_MWV207D_ACTIVE_FUS_NR;
	default:
		BUG_ON(1);
		return 0;
	}
}

static const char *mwv207d_fence_get_driver_name(struct dma_fence *fence)
{
	return "mwv207d";
}

static const char *mwv207d_fence_get_timeline_name(struct dma_fence *fence)
{
	struct mwv207d_job *mjob = container_of(fence, struct mwv207d_job, hw_fence.base);
	struct mwv207d_sched *sched = to_mwv207d_sched(mjob->base.sched);

	return sched->pipe->fname;
}

static void mwv207d_fence_release(struct dma_fence *fence)
{
	struct mwv207d_job *mjob = container_of(fence, struct mwv207d_job, hw_fence.base);

	kfree_rcu(mjob, hw_fence.base.rcu);
}

static const struct dma_fence_ops mwv207d_fence_ops = {
	.get_driver_name = mwv207d_fence_get_driver_name,
	.get_timeline_name = mwv207d_fence_get_timeline_name,
	.release = mwv207d_fence_release,
};

struct mwv207d_job *mwv207d_job_alloc(void)
{
	struct mwv207d_job *mjob;

	mjob = kzalloc(sizeof(*mjob), GFP_KERNEL);
	if (!mjob)
		return NULL;

	kref_init(&mjob->refcount);
	INIT_LIST_HEAD(&mjob->tvblist);
	xa_init_flags(&mjob->deps, XA_FLAGS_ALLOC);
	mjob->last_dep = 0;
	return mjob;
}

static void mwv207d_job_fini(struct kref *kref)
{
	struct mwv207d_job *mjob = container_of(kref, struct mwv207d_job, refcount);
	struct mwv207d_tvb *mtvb;
	struct mwv207d_bo *mbo;
	struct dma_fence *fence;
	unsigned long index;
	int i;

	xa_for_each(&mjob->deps, index, fence)
		dma_fence_put(fence);
	xa_destroy(&mjob->deps);

	if (mjob->base.s_fence)
		drm_sched_job_cleanup(&mjob->base);

	mwv207d_for_each_mtvb(mtvb, mjob) {
		for (i = 0; i < mtvb->nr_maps; i++)
			if (likely(mtvb->mapping[i]))
				mwv207d_mapping_put(mtvb->mapping[i]);
		kfree(mtvb->mapping);

		if (mtvb->base.bo) {
			mbo = to_mbo(mtvb->base.bo);
			mwv207d_bo_unref(&mbo);
		}
	}

	if (mjob->ctx)
		mwv207d_ctx_put(mjob->ctx);

	if (mjob->cmd_mode == 0x0)
		kvfree(mjob->cmd_ptr);
	else if (mjob->cmd_mode == 0x2)
		kvfree(mjob->cmd_array);
	kvfree(mjob->mtvb);

	if (mjob->hw_fence.base.ops)
		dma_fence_put(&mjob->hw_fence.base);
	else
		kfree(mjob);
}

struct mwv207d_job *mwv207d_job_get(struct mwv207d_job *mjob)
{
	kref_get(&mjob->refcount);
	return mjob;
}

void mwv207d_job_put(struct mwv207d_job *mjob)
{
	kref_put(&mjob->refcount, mwv207d_job_fini);
}

int mwv207d_job_add_resv_fence(struct mwv207d_job *mjob,
			       struct dma_resv *resv,
			       int write)
{
	struct dma_resv_iter cursor;
	struct dma_fence *fence;
	int ret = 0;

	dma_resv_iter_begin(&cursor, resv, dma_resv_usage_rw(write));
	dma_resv_for_each_fence_unlocked(&cursor, fence) {
		fence = dma_fence_get(fence);
		ret = drm_sched_job_add_dependency(&mjob->base, fence);
		if (ret)
			continue;
	}
	dma_resv_iter_end(&cursor);

	return ret;
}

static void mwv207d_sched_dump_job(struct mwv207d_job *mjob)
{
	int lines, i;
	u32 *cmd;

	if (mjob->cmd_mode != 0x0)
		return;

	cmd = (u32 *)mjob->cmd_ptr;
	lines = (mjob->cmd_size / 4) & ~0x3;
	for (i = 0; i < lines; i += 4) {
		pr_info("0x%08x : %08x %08x %08x %08x", i,
			cmd[i], cmd[i+1], cmd[i+2], cmd[i+3]);
	}

	switch (mjob->cmd_size / 4 - lines) {
	case 0:
		return;
	case 1:
		pr_info("0x%08x : %08x", i, cmd[i]);
		break;
	case 2:
		pr_info("0x%08x : %08x %08x", i, cmd[i], cmd[i+1]);
		break;
	case 3:
		pr_info("0x%08x : %08x %08x %08x", i,
			cmd[i], cmd[i+1], cmd[i+2]);
		break;
	default:
		pr_err("should never happen");
		break;

	}
}

static struct dma_fence *
mwv207d_sched_dependency(struct drm_sched_job *job,
			 struct drm_sched_entity *entity)
{
	struct mwv207d_job *mjob = to_mwv207d_job(job);

	if (!xa_empty(&mjob->deps))
		return xa_erase(&mjob->deps, mjob->last_dep++);

	return NULL;
}

static struct dma_fence *mwv207d_sched_run_job(struct drm_sched_job *job)
{
	struct mwv207d_sched *sched = to_mwv207d_sched(job->sched);
	struct mwv207d_job *mjob = to_mwv207d_job(job);
	struct dma_fence *fence;

	if (unlikely(job->s_fence->finished.error))
		return ERR_PTR(-ECANCELED);

	++sched->fence_seqno;
	if (unlikely(mjob->hw_fence.base.ops))
		mjob->hw_fence.base.seqno = sched->fence_seqno;
	else
		dma_fence_init(&mjob->hw_fence.base,
			       &mwv207d_fence_ops, &sched->fence_lock,
			       sched->fence_ctx, sched->fence_seqno);

	fence = sched->pipe->submit(sched->pipe, mjob);
	if (IS_ERR_OR_NULL(fence))
		pr_err("%s submit failed, errcode=%ld",
		       sched->pipe->fname, PTR_ERR(fence));

	return fence;
}

static enum drm_gpu_sched_stat
mwv207d_sched_timedout_job(struct drm_sched_job *job)
{
	struct mwv207d_sched *sched = to_mwv207d_sched(job->sched);
	struct mwv207d_job *mjob = to_mwv207d_job(job);

	pr_warn("warning, job from '%s' to '%s', timed out!",
		mjob->comm, sched->pipe->fname);

	drm_sched_stop(&sched->base, job);

	if (dump_hang_job) {
		sched->pipe->dump_state(sched->pipe);
		mwv207d_sched_dump_job(mjob);
	}

	drm_sched_increase_karma(job);

	sched->pipe->reset(sched->pipe);

	drm_sched_resubmit_jobs(&sched->base);
	drm_sched_start(&sched->base, true);

	return DRM_GPU_SCHED_STAT_NOMINAL;
}

static void mwv207d_sched_free_job(struct drm_sched_job *job)
{
	struct mwv207d_job *mjob = to_mwv207d_job(job);

	if (mjob->replaced_vm)
		mwv207d_vm_put(mjob->replaced_vm);

	mwv207d_job_put(mjob);
}

static const struct drm_sched_backend_ops mwv207d_sched_ops = {
	.prepare_job = mwv207d_sched_dependency,
	.run_job = mwv207d_sched_run_job,
	.timedout_job = mwv207d_sched_timedout_job,
	.free_job = mwv207d_sched_free_job,
};

static struct drm_gpu_scheduler *
mwv207d_sched_create(struct mwv207d_device *mdev, struct mwv207d_pipe *pipe,
		     unsigned int hw_submission, unsigned int hang_limit,
		     long timeout)
{
	struct mwv207d_sched *sched;
	int ret;

	sched = devm_kzalloc(mdev->dev, sizeof(*sched), GFP_KERNEL);
	if (!sched)
		goto err;
	sched->pipe = pipe;

	ret = drm_sched_init(&sched->base, &mwv207d_sched_ops, hw_submission,
			     hang_limit, msecs_to_jiffies(timeout),
			     NULL, NULL, pipe->fname, mdev->dev);
	if (ret)
		goto err;
	spin_lock_init(&sched->fence_lock);
	sched->fence_ctx = dma_fence_context_alloc(1);
	sched->fence_seqno = 0;

	return &sched->base;
err:
	pipe->destroy(pipe);
	return NULL;
}

static void mwv207d_sched_destroy(struct drm_gpu_scheduler *sched)
{
	struct mwv207d_sched *msched;

	if (!sched)
		return;

	msched = to_mwv207d_sched(sched);
	drm_sched_fini(sched);
	msched->pipe->destroy(msched->pipe);
}

static int mwv207d_sched_pipe_create(struct mwv207d_device *mdev, int type)
{
	struct mwv207d_sched_info *info = &mwv207d_sched_pipe_info[type];
	struct mwv207d_pipe *pipe;
	int i;

	BUG_ON(mdev->hw.nr_pipe[type] > 0x8);

	for (i = 0; i < mdev->hw.nr_pipe[type]; i++) {
		if (!info->pipes[i].reg_base)
			break;
		pipe = info->create(mdev, &info->pipes[i]);
		if (!pipe)
			break;
		mdev->sched[type][i] = mwv207d_sched_create(mdev, pipe,
							    info->hw_submission,
							    info->hang_limit,
							    info->timeout);
		if (!mdev->sched[type][i]) {
			pr_err("error, create %s failed", info->pipes[i].name);
			break;
		}
	}

	return i;
}

static inline bool sched_is_idle(struct drm_gpu_scheduler *sched)
{
	return !atomic_read(&sched->hw_rq_count);
}

int mwv207d_sched_suspend(struct mwv207d_device *mdev)
{
	int i, j;
	int ret;

	for (i = 0; i < 0x6; i++) {
		for (j = 0; j < 0x8; j++) {
			if (!mdev->sched[i][j])
				continue;
			ret = wait_for(sched_is_idle(mdev->sched[i][j]), 2000);
			if (ret) {
				dev_warn(mdev->dev, "%s not idle when suspend",
					 mdev->sched[i][j]->name);
				return -EBUSY;
			}
		}
	}

	return 0;
}

void mwv207d_sched_resume(struct mwv207d_device *mdev)
{
	struct mwv207d_sched *sched;
	int i, j;

	for (i = 0; i < 0x6; i++) {
		for (j = 0; j < 0x8; j++) {
			if (!mdev->sched[i][j])
				continue;
			sched = to_mwv207d_sched(mdev->sched[i][j]);
			sched->pipe->reset(sched->pipe);
		}
	}
}

static void mwv207d_sched_info_init(struct mwv207d_device *mdev)
{
	if (mdev->hw.is_emulation) {
		mwv207d_sched_pipe_info[0x0].timeout = MAX_SCHEDULE_TIMEOUT;
		dev_info(mdev->dev, "disable 3D timeout reset on cmodel");
	}

	if (mdev->hw.remap_codec) {
		mwv207d_sched_pipe_info[0x2].pipes[0].reg_base = 0xe0000;
		mwv207d_sched_pipe_info[0x2].pipes[1].reg_base = 0xe2000;
		mwv207d_sched_pipe_info[0x1].pipes[0].reg_base = 0xe1000;
		mwv207d_sched_pipe_info[0x1].pipes[1].reg_base = 0xe3000;
	}
}

int mwv207d_sched_init(struct mwv207d_device *mdev)
{
	int i;

	mwv207d_sched_info_init(mdev);

	for (i = 0; i < 0x6; i++) {
		mdev->nr_pipe[i] = mwv207d_sched_pipe_create(mdev, i);
		mwv207d_db_add(mdev, sched_type_to_key(i), mdev->nr_pipe[i]);
	}

	return 0;
}

void mwv207d_sched_fini(struct mwv207d_device *mdev)
{
	int i, j;

	for (i = 0; i < 0x6; ++i) {
		for (j = 0; j < 0x8; j++) {
			if (!mdev->sched[i][j])
				continue;
			mwv207d_sched_destroy(mdev->sched[i][j]);
		}
	}

}

int mwv207d_pipe_attach_vm(struct mwv207d_device *mdev, struct mwv207d_vm *vm)
{
	struct mwv207d_sched *msched ;
	int ret, i, j;

	for (i = 0; i < 0x6; ++i) {
		for (j = 0; j < 0x8; j++) {
			if (!mdev->sched[i][j])
				continue;
			msched = to_mwv207d_sched(mdev->sched[i][j]);
			if (!msched->pipe->attach_vm)
				continue;
			ret = msched->pipe->attach_vm(msched->pipe, vm);
			if (ret)
				return ret;
		}
	}

	return 0;
}

void mwv207d_pipe_detach_vm(struct mwv207d_device *mdev, struct mwv207d_vm *vm)
{
	struct mwv207d_sched *msched;
	int i, j;

	for (i = 0; i < 0x6; ++i) {
		for (j = 0; j < 0x8; j++) {
			if (!mdev->sched[i][j])
				continue;
			msched = to_mwv207d_sched(mdev->sched[i][j]);
			if (!msched->pipe->detach_vm)
				continue;
			msched->pipe->detach_vm(msched->pipe, vm);
		}
	}
}
