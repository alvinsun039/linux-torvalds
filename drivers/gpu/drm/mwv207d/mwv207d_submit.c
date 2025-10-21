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
#include <linux/uaccess.h>
#include <linux/sync_file.h>
#include <linux/file.h>
#include <drm/drm_syncobj.h>

#include "mwv207d_drm.h"
#include "mwv207d_submit.h"
#include "mwv207d_sched.h"
#include "mwv207d_ctx.h"
#include "mwv207d_bo.h"
#include "mwv207d_gem.h"
#include "mwv207d_vm.h"

struct mwv207d_submit_post_dep {
	struct drm_syncobj *syncobj;
	u64 point;
	struct dma_fence_chain *chain;
};

static inline int mwv207d_submit_select_engine(struct drm_device *dev,
					       struct mwv207d_job *mjob,
					       struct drm_mwv207d_submit *args)
{
	struct mwv207d_device *mdev = drm_to_mdev(dev);
	struct mwv207d_ctx_entity *centity;
	int idmax, id;

	if (unlikely(args->engine_type >= 0x6))
		return -EINVAL;
	idmax = mdev->nr_pipe[args->engine_type];
	if (unlikely(idmax <= 0))
		return -ENODEV;

	id = args->engine_id;
	if (id == 0xFFFFFFFF)
		id = idmax;
	else if (unlikely(id < 0 || id > idmax))
		return -ENODEV;
	centity = mjob->ctx->centities[args->engine_type][id];
	if (unlikely(!centity))
		return -ENODEV;
	mjob->engine_entity = &centity->entity;
	mjob->qlock = &centity->qlock;

	return 0;
}

static int mwv207d_submit_init_job_bo(struct mwv207d_job *mjob,
				      struct drm_mwv207d_submit *args,
				      struct drm_file *filp)
{
	struct drm_mwv207d_bo_acc ubos_stack[0x20];
	struct drm_mwv207d_bo_acc *ubos = ubos_stack;
	struct ttm_validate_buffer *tvb;
	struct drm_gem_object *gobj;
	int ret, i, nr_bos;

	nr_bos = (int)args->nr_bos;
	if (nr_bos < 0)
		return -EINVAL;
	else if (nr_bos == 0)
		return 0;

	if (!args->bos)
		return -EINVAL;

	if (nr_bos > 0x20)
		ubos = kvmalloc_array(nr_bos, sizeof(*ubos), GFP_KERNEL);
	if (unlikely(!ubos))
		return -ENOMEM;

	ret = copy_from_user(ubos, u64_to_user_ptr(args->bos), nr_bos * sizeof(*ubos));
	if (ret)
		goto out;

	mjob->mtvb = kvcalloc(nr_bos, sizeof(struct mwv207d_tvb), GFP_KERNEL);
	if (!mjob->mtvb) {
		ret = -ENOMEM;
		goto out;
	}

	spin_lock(&filp->table_lock);
	for (i = 0; i < nr_bos; i++) {
		tvb = &mjob->mtvb[i].base;
		tvb->num_shared = (ubos[i].flags & 0x1) ? 0 : 1;
		gobj = idr_find(&filp->object_idr, ubos[i].handle);
		if (unlikely(!gobj)) {
			spin_unlock(&filp->table_lock);
			ret = -ENOENT;
			goto out;
		}
		tvb->bo = &mwv207d_bo_ref(gbo_to_mbo(gobj))->tbo;
		list_add_tail(&tvb->head, &mjob->tvblist);
	}
	spin_unlock(&filp->table_lock);
out:
	if (unlikely(ubos != ubos_stack))
		kvfree(ubos);

	return ret;
}

static int mwv207d_submit_init_job_cmd_ram(struct mwv207d_job *mjob,
					   struct drm_mwv207d_submit *args)
{
	if (args->cmd_size & 0x3)
		return -EINVAL;

	mjob->cmd_size = args->cmd_size;
	if (mjob->cmd_size > SZ_128K)
		return -E2BIG;

	mjob->cmd_ptr = kvmalloc(args->cmd_size, GFP_KERNEL);
	if (!mjob->cmd_ptr)
		return -ENOMEM;

	mjob->cmd_mode = 0x0;
	return copy_from_user(mjob->cmd_ptr, u64_to_user_ptr(args->cmds),
			      mjob->cmd_size);
}

static int mwv207d_submit_init_job_cmd_local(struct mwv207d_job *mjob,
					   struct drm_mwv207d_submit *args)
{
	if (args->cmd_size & 0x3)
		return -EINVAL;

	if (args->engine_type != 0x0)
		return -EINVAL;

	mjob->cmd_size = args->cmd_size;
	if (mjob->cmd_size > (SZ_512K-SZ_4K))
		return -E2BIG;

	mjob->cmd_va = args->cmds;
	if (!mjob->cmd_va)
		return -EINVAL;
	mjob->cmd_mode = 0x1;

	return 0;
}

static int mwv207d_submit_init_job_cmd_split(struct mwv207d_job *mjob,
					     struct drm_mwv207d_submit *args)
{
	int ret, i;

	if (args->engine_type != 0x0)
		return -EINVAL;

	if (args->cmd_size >= SZ_128K / 256)
		return -E2BIG;

	mjob->cmd_mode = 0x2;
	mjob->cmd_size = args->cmd_size;
	mjob->cmd_array = kvmalloc_array(mjob->cmd_size,
				 sizeof(struct drm_mwv207d_submit_cmds), GFP_KERNEL);
	if (!mjob->cmd_array)
		return -ENOMEM;

	ret = copy_from_user(mjob->cmd_array, u64_to_user_ptr(args->cmds),
			mjob->cmd_size * sizeof(struct drm_mwv207d_submit_cmds));
	if (ret)
		return ret;

	for (i = 0; i < mjob->cmd_size; i++) {
		if (mjob->cmd_array[i].padding)
			return -EINVAL;
		if (!mjob->cmd_array[i].cmd_va)
			return -EINVAL;
		if (mjob->cmd_array[i].cmd_size > (SZ_512K-SZ_4K))
			return -E2BIG;
	}

	return 0;
}

static int mwv207d_submit_init_job_cmd(struct mwv207d_job *mjob,
				       struct drm_mwv207d_submit *args)
{
	if ((int)args->cmd_size < 0)
		return -EINVAL;

	if ((args->flags & 0x40)
			&& (args->flags & 0x20))
		return -EINVAL;

	if (args->flags & 0x40)
		return mwv207d_submit_init_job_cmd_local(mjob, args);
	else if (args->flags & 0x20)
		return mwv207d_submit_init_job_cmd_split(mjob, args);
	else
		return mwv207d_submit_init_job_cmd_ram(mjob, args);
}

static int mwv207d_submit_init_job(struct drm_device *dev,
				   struct mwv207d_job *mjob,
				   struct drm_mwv207d_submit *args,
				   struct drm_file *filp)
{
	int ret;

	get_task_comm(mjob->comm, current);

	mjob->ctx = mwv207d_ctx_lookup(dev, filp, args->ctx);
	if (!mjob->ctx)
		return -ENOENT;
	ret = mwv207d_submit_select_engine(dev, mjob, args);
	if (ret)
		return ret;
	ret = drm_sched_job_init(&mjob->base, mjob->engine_entity, filp->driver_priv);
	if (ret)
		return ret;
	drm_sched_job_arm(&mjob->base);
	ret = mwv207d_submit_init_job_bo(mjob, args, filp);
	if (ret)
		return ret;
	ret = mwv207d_submit_init_job_cmd(mjob, args);
	if (ret)
		return ret;

	return 0;
}

static int mwv207d_submit_parse_post_deps(struct mwv207d_job *mjob,
					  struct drm_mwv207d_submit *args,
					  struct drm_file *filp,
					  struct mwv207d_submit_post_dep **pdep)
{
	struct mwv207d_submit_post_dep *post_deps = NULL;
	struct drm_mwv207d_submit_syncobj *syncobjs;
	u32 nr, i, j;
	int ret;

	if ((args->flags & 0x10) && args->nr_out_syncobjs) {
		nr = args->nr_out_syncobjs;
		post_deps = kcalloc(nr, sizeof(*post_deps), GFP_KERNEL);
		if (!post_deps)
			return -ENOMEM;
		syncobjs = memdup_user(u64_to_user_ptr(args->out_syncobjs),
				nr * sizeof(struct drm_mwv207d_submit_syncobj));
		if (IS_ERR(syncobjs)) {
			ret = PTR_ERR(syncobjs);
			goto err_free;
		}

		for (i = 0; i < nr; i++) {
			if (syncobjs[i].padding) {
				ret = -EINVAL;
				goto err_synobj;
			}
			post_deps[i].syncobj = drm_syncobj_find(filp, syncobjs[i].handle);
			if (!post_deps[i].syncobj) {
				ret = -ENOENT;
				goto err_synobj;
			}
			post_deps[i].point = syncobjs[i].point;
			if (post_deps[i].point) {
				post_deps[i].chain = dma_fence_chain_alloc();
				if (!post_deps[i].chain) {
					ret = -ENOMEM;
					goto err_synobj;
				}
			}
		}
		kfree(syncobjs);
	}
	*pdep = post_deps;
	return 0;

err_synobj:
	for (j = 0; j <= i; j++) {
		if (post_deps[j].chain)
			dma_fence_chain_free(post_deps[j].chain);
		if (post_deps[j].syncobj)
			drm_syncobj_put(post_deps[j].syncobj);
	}
	kfree(syncobjs);
err_free:
	kfree(post_deps);
	return ret;
}

static struct mwv207d_job *
mwv207d_submit_init(struct drm_device *dev,
		    struct drm_file *filp,
		    struct drm_mwv207d_submit *args,
		    struct mwv207d_submit_post_dep **pdeps)
{
	struct mwv207d_job *mjob;
	int ret;

	mjob = mwv207d_job_alloc();
	if (!mjob)
		return ERR_PTR(-ENOMEM);

	ret = mwv207d_submit_init_job(dev, mjob, args, filp);
	if (ret)
		goto err;
	ret = mwv207d_submit_parse_post_deps(mjob, args, filp, pdeps);
	if (ret)
		goto err;

	return mjob;
err:
	mwv207d_job_put(mjob);
	return ERR_PTR(ret);
}

static void mwv207d_submit_fini(struct mwv207d_job *mjob,
				struct drm_mwv207d_submit *args,
				struct mwv207d_submit_post_dep *post_deps)
{
	if (post_deps) {
		u32 i;
		for (i = 0; i < args->nr_out_syncobjs; i++) {
			if (post_deps[i].chain)
				dma_fence_chain_free(post_deps[i].chain);
			if (post_deps[i].syncobj)
				drm_syncobj_put(post_deps[i].syncobj);
		}
		kfree(post_deps);
	}

	mwv207d_job_put(mjob);
}

static int mwv207d_submit_validate_vm(struct mwv207d_job *mjob)
{
	struct mwv207d_mapping *mapping;
	struct mwv207d_bo_vm *bo_vm;
	struct mwv207d_tvb *mtvb;
	struct mwv207d_bo *mbo;
	int ret, i;

	mwv207d_for_each_mtvb(mtvb, mjob) {
		mbo = to_mbo(mtvb->base.bo);
		bo_vm = mwv207d_bo_vm_find(mbo, mjob->ctx->vm);
		if (!bo_vm)
			return -ENOENT;

		ret = mwv207d_vm_validate_bo_vm(mbo, bo_vm);
		if (ret)
			return ret;

		if (unlikely(!bo_vm->nr_maps))
			return -EINVAL;

		mtvb->mapping = kzalloc(sizeof(struct mwv207d_mapping *) *
				bo_vm->nr_maps, GFP_KERNEL);
		if (unlikely(!mtvb->mapping))
			return -ENOMEM;

		mtvb->nr_maps = bo_vm->nr_maps;

		i = 0;
		list_for_each_entry(mapping, &bo_vm->valid_maps, list)
			mtvb->mapping[i++] = mwv207d_mapping_get(mapping);
	}

	return 0;
}

static int mwv207d_submit_validate(struct drm_device *dev,
				   struct mwv207d_job *mjob)
{
	struct ttm_operation_ctx ctx = {
		.interruptible = true,
		.no_wait_gpu = false
	};
	struct mwv207d_tvb *mtvb;
	struct mwv207d_bo *mbo;
	int first_iter = 1, ret;
	u32 domain;

retry:
	ret = 0;
	mwv207d_for_each_mtvb(mtvb, mjob) {
		mbo = to_mbo(mtvb->base.bo);
		if (mbo->tbo.pin_count)
			continue;

		domain = mbo->domain & ~0x1;
		if (!domain) {
			domain =  0x4;
			mbo->domain = domain;
		}

		mbo->flags &= ~0x1;
		mwv207d_bo_placement_from_domain(mbo, domain);
		ret |= ttm_bo_validate(&mbo->tbo, &mbo->placement, &ctx);
		if (ret == -ENOMEM && first_iter)
			continue;
		if (ret)
			return ret;
	}

	if (first_iter && ret) {
		first_iter = 0;
		goto retry;
	}

	return mwv207d_submit_validate_vm(mjob);
}

static int mwv207d_submit_attach_implicit_dependency(struct mwv207d_job *mjob,
		struct drm_mwv207d_submit *args)
{
	struct mwv207d_tvb *mtvb;
	struct dma_fence *fence;
	struct dma_resv_iter cursor;
	int ret;

	if (args->flags & 0x4) {
		mwv207d_for_each_mtvb(mtvb, mjob) {
			dma_resv_iter_begin(&cursor, mtvb->base.bo->base.resv, DMA_RESV_USAGE_KERNEL);
			dma_resv_for_each_fence_unlocked(&cursor, fence) {
				ret = drm_sched_job_add_dependency(&mjob->base,fence);
				if (ret)
					return ret;
			}
			dma_resv_iter_end(&cursor);
		}
		return 0;
	}

	mwv207d_for_each_mtvb(mtvb, mjob) {
		ret = mwv207d_job_add_resv_fence(mjob,
				mtvb->base.bo->base.resv,
				mtvb->base.num_shared == 0);
		if (ret)
			return ret;
	}

	return 0;
}

static int mwv207d_submit_attach_dependency(struct mwv207d_job *mjob,
					    struct drm_file *filp,
					    struct drm_mwv207d_submit *args)
{
	struct drm_mwv207d_submit_syncobj *syncobjs;
	struct dma_fence *fence;
	u32 nr, i;
	int ret;

	if (args->flags & 0x1) {
		fence = sync_file_get_fence(args->fence_fd);
		if (!fence)
			return -EINVAL;
		ret = drm_sched_job_add_dependency(&mjob->base, fence);
		if (ret)
			return ret;
	}
	if ((args->flags & 0x8) && args->nr_in_syncobjs) {
		nr = args->nr_in_syncobjs;
		syncobjs = memdup_user(u64_to_user_ptr(args->in_syncobjs),
				nr * sizeof(struct drm_mwv207d_submit_syncobj));
		if (IS_ERR(syncobjs))
			return PTR_ERR(syncobjs);
		for (i = 0; i < nr; i++) {
			if (syncobjs[i].padding) {
				kfree(syncobjs);
				return -EINVAL;
			}
			ret = drm_sched_job_add_syncobj_dependency(&mjob->base,
								   filp,
								   syncobjs[i].handle,
								   syncobjs[i].point);
			if (ret) {
				kfree(syncobjs);
				return ret;
			}
		}
		kfree(syncobjs);
	}

	return mwv207d_submit_attach_implicit_dependency(mjob, args);
}

static int mwv207d_submit_commit(struct mwv207d_job *mjob,
				 struct drm_mwv207d_submit *args,
				 struct dma_fence **fence,
				 struct mwv207d_submit_post_dep *post_deps)
{
	int ret;

	ret = drm_sched_job_init(&mjob->base, mjob->engine_entity, mjob->ctx);
	if (ret)
		return ret;
	drm_sched_job_arm(&mjob->base);

	if (args->flags & 0x2) {
		struct sync_file *sync_file;
		int fd;

		fd = get_unused_fd_flags(O_CLOEXEC);
		if (fd < 0)
			return fd;

		sync_file = sync_file_create(&mjob->base.s_fence->finished);
		if (!sync_file) {
			put_unused_fd(fd);
			return -ENOMEM;
		}
		fd_install(fd, sync_file->file);
		args->fence_fd = fd;
	}

	if (post_deps) {
		u32 i;
		for (i = 0; i < args->nr_out_syncobjs; i++) {
			if (post_deps[i].chain) {
				drm_syncobj_add_point(post_deps[i].syncobj,
						post_deps[i].chain,
						&mjob->base.s_fence->finished,
						post_deps[i].point);
				post_deps[i].chain = NULL;
			} else
				drm_syncobj_replace_fence(post_deps[i].syncobj,
							  &mjob->base.s_fence->finished);
		}
	}

	*fence = &mjob->base.s_fence->finished;

	mwv207d_job_get(mjob);
	drm_sched_entity_push_job(&mjob->base);

	return 0;
}

int mwv207d_submit_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *filp)
{
	struct mwv207d_submit_post_dep *post_deps = NULL;
	struct drm_mwv207d_submit *args = data;
	struct ww_acquire_ctx ticket;
	struct mwv207d_job *mjob;
	struct dma_fence *fence = NULL;
	int ret;

	if (args->padding || (args->flags & ~MWV207D_SUBMIT_FLAG_MASK))
		return -EINVAL;
	if (args->cmd_size == 0 || args->cmds == 0)
		return -EINVAL;

	mjob = mwv207d_submit_init(dev, filp, args, &post_deps);
	if (IS_ERR(mjob))
		return PTR_ERR(mjob);
	ret = mutex_lock_interruptible(mjob->qlock);
	if (ret)
		goto fini;
	ret = mwv207d_ctx_wait_prev_fence(mjob->ctx, mjob->engine_entity);
	if (ret)
		goto unlock;
	ret = ttm_eu_reserve_buffers(&ticket, &mjob->tvblist, true, NULL);
	if (ret)
		goto unlock;
	ret = mwv207d_submit_validate(dev, mjob);
	if (ret)
		goto unreserve;
	ret = mwv207d_submit_attach_dependency(mjob, filp, args);
	if (ret)
		goto unreserve;
	ret = mwv207d_submit_commit(mjob, args, &fence, post_deps);
	if (ret)
		goto unreserve;

unreserve:
	if (likely(fence)) {
		ttm_eu_fence_buffer_objects(&ticket, &mjob->tvblist, fence);
		mwv207d_ctx_add_fence(mjob->ctx, mjob->engine_entity, fence);
	} else
		ttm_eu_backoff_reservation(&ticket, &mjob->tvblist);
unlock:
	mutex_unlock(mjob->qlock);
fini:
	mwv207d_submit_fini(mjob, args, post_deps);
	return ret;
}
