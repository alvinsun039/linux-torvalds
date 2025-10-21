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
#include <linux/dma-buf.h>
#include <linux/uaccess.h>
#include <drm/drm_drv.h>
#include <drm/drm_utils.h>
#include <drm/drm_prime.h>
#include <drm/drm_gem_ttm_helper.h>
#include <drm/ttm/ttm_tt.h>

#include "mwv207d_gem.h"
#include "mwv207d_bo.h"
#include "mwv207d_drv.h"
#include "mwv207d_drm.h"
#include "mwv207d_vm.h"

static vm_fault_t mwv207d_gem_fault(struct vm_fault *vmf)
{
	struct ttm_buffer_object *bo = vmf->vma->vm_private_data;
	vm_fault_t ret;

	ret = ttm_bo_vm_reserve(bo, vmf);
	if (ret)
		return ret;
	ret = mwv207d_bo_fault_reserve_notify(bo);
	if (ret)
		goto unlock_resv;
	ret = ttm_bo_vm_fault_reserved(vmf, vmf->vma->vm_page_prot,
				       TTM_BO_VM_NUM_PREFAULT);
	if (ret == VM_FAULT_RETRY && !(vmf->flags & FAULT_FLAG_RETRY_NOWAIT))
		return ret;
unlock_resv:
	dma_resv_unlock(bo->base.resv);
	return ret;
}

static const struct vm_operations_struct mwv207d_ttm_vm_ops = {
	.fault   = mwv207d_gem_fault,
	.open    = ttm_bo_vm_open,
	.close   = ttm_bo_vm_close,
	.access  = ttm_bo_vm_access,
};

static struct sg_table *
mwv207d_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct mwv207d_bo *mbo = gbo_to_mbo(obj);

	return drm_prime_pages_to_sg(obj->dev, mbo->tbo.ttm->pages,
				     mbo->tbo.resource->size >> PAGE_SHIFT);
}

struct drm_gem_object *
mwv207d_gem_prime_import_sg_table(struct drm_device *dev,
				  struct dma_buf_attachment *attach,
				  struct sg_table *sg)
{
	struct dma_resv *resv = attach->dmabuf->resv;
	struct mwv207d_device *mdev = drm_to_mdev(dev);
	struct mwv207d_bo *mbo;
	int ret;

	dma_resv_lock(resv, NULL);
	ret = mwv207d_bo_create(mdev, attach->dmabuf->size, PAGE_SIZE, false,
				0x4, 0, sg, resv, &mbo);
	dma_resv_unlock(resv);
	if (ret)
		return ERR_PTR(ret);

	mbo->tbo.base.funcs = &mwv207d_gem_object_funcs;

	mbo->prime_shared_count = 1;
	return &mbo->tbo.base;
}

static int mwv207d_gem_prime_pin(struct drm_gem_object *obj)
{
	struct mwv207d_bo *mbo = gbo_to_mbo(obj);
	struct dma_resv_iter cursor;
	struct dma_fence *fence;
	int ret = 0;

	ret = mwv207d_bo_reserve(mbo, false);
	if (unlikely(ret != 0))
		return ret;

	ret = mwv207d_bo_pin(mbo, 0x4, NULL);
	if (unlikely(ret != 0))
		goto error;

	dma_resv_iter_begin(&cursor, mbo->tbo.base.resv, DMA_RESV_USAGE_READ);
	dma_resv_for_each_fence_unlocked(&cursor, fence) {
		ret = dma_fence_wait(fence, false);
		if (unlikely(ret)) {
			dma_resv_iter_end(&cursor);
			mwv207d_bo_unpin(mbo);
			goto error;
		}
	}
	dma_resv_iter_end(&cursor);
	mbo->prime_shared_count++;

error:
	mwv207d_bo_unreserve(mbo);
	return ret;
}

static void mwv207d_gem_prime_unpin(struct drm_gem_object *obj)
{
	struct mwv207d_bo *bo = gbo_to_mbo(obj);
	int ret;

	ret = mwv207d_bo_reserve(bo, false);
	if (unlikely(ret))
		return;
	mwv207d_bo_unpin(bo);
	if (bo->prime_shared_count)
		bo->prime_shared_count--;
	mwv207d_bo_unreserve(bo);
}

static struct dma_buf *
mwv207d_gem_prime_export(struct drm_gem_object *gobj, int flags)
{
	return drm_gem_prime_export(gobj, flags);
}

static void mwv207d_gem_object_free(struct drm_gem_object *gobj)
{
	struct mwv207d_bo *mbo = gbo_to_mbo(gobj);

	mwv207d_bo_unref(&mbo);
}

static int mwv207d_gem_object_open(struct drm_gem_object *obj,
				   struct drm_file *file_priv)
{
	struct mwv207d_fpriv *fpriv = to_fpriv(file_priv);
	struct mwv207d_bo *bo = gbo_to_mbo(obj);
	struct mwv207d_bo_vm *bo_vm;
	int ret;

	ret = mwv207d_bo_reserve(bo, false);
	if (ret)
		return ret;

	bo_vm = mwv207d_bo_vm_find(bo, fpriv->vm);
	if (bo_vm) {
		bo_vm->refcnt++;
		goto unlock;
	}

	bo_vm = kzalloc(sizeof(struct mwv207d_bo_vm), GFP_KERNEL);
	if (!bo_vm) {
		ret = -ENOMEM;
		goto unlock;
	}
	bo_vm->refcnt = 1;
	bo_vm->vm = fpriv->vm;
	INIT_LIST_HEAD(&bo_vm->pending_maps);
	INIT_LIST_HEAD(&bo_vm->valid_maps);
	list_add(&bo_vm->bo_vm_node, &bo->bo_vm_list);
unlock:
	mwv207d_bo_unreserve(bo);

	return ret;
}

static void mwv207d_gem_object_close(struct drm_gem_object *obj,
				     struct drm_file *file_priv)
{
	struct mwv207d_fpriv *fpriv = to_fpriv(file_priv);
	struct mwv207d_bo *bo = gbo_to_mbo(obj);
	struct mwv207d_mapping *mapping, *mtmp;
	struct mwv207d_bo_vm *bo_vm, *tmp;
	int ret;

	ret = mwv207d_bo_reserve(bo, true);
	if (ret) {
		pr_warn("failed to reserve bo when closing");
		return;
	}

	bo_vm = mwv207d_bo_vm_find(bo, fpriv->vm);
	if (--bo_vm->refcnt)
		goto unlock;

	list_for_each_entry_safe(bo_vm, tmp, &bo->bo_vm_list, bo_vm_node) {
		if (bo_vm->vm == fpriv->vm) {
			list_del(&bo_vm->bo_vm_node);
			break;
		}
	}

	BUG_ON(bo_vm->vm != fpriv->vm);

	list_for_each_entry_safe(mapping, mtmp, &bo_vm->pending_maps, list) {
		list_del(&mapping->list);
		mwv207d_mapping_put(mapping);
		bo_vm->nr_maps--;
	}
	list_for_each_entry_safe(mapping, mtmp, &bo_vm->valid_maps, list) {
		list_del(&mapping->list);
		mwv207d_mapping_put(mapping);
		bo_vm->nr_maps--;
	}
	BUG_ON(bo_vm->nr_maps != 0);
	kfree(bo_vm);
unlock:
	mwv207d_bo_unreserve(bo);
}

const struct drm_gem_object_funcs mwv207d_gem_object_funcs = {
	.free           = mwv207d_gem_object_free,
	.open           = mwv207d_gem_object_open,
	.close          = mwv207d_gem_object_close,
	.export         = mwv207d_gem_prime_export,
	.pin            = mwv207d_gem_prime_pin,
	.unpin          = mwv207d_gem_prime_unpin,
	.get_sg_table   = mwv207d_gem_prime_get_sg_table,
	.vmap           = drm_gem_ttm_vmap,
	.vunmap         = drm_gem_ttm_vunmap,
	.mmap           = drm_gem_ttm_mmap,
	.vm_ops         = &mwv207d_ttm_vm_ops,
};

static int mwv207d_gem_object_create(struct mwv207d_device *mdev, u64 size,
				     u64 align, u32 domain,
				     u32 flags, bool kernel,
				     struct drm_gem_object **obj)
{
	struct mwv207d_bo *mbo;
	int ret;

	*obj = NULL;

	if (align < PAGE_SIZE)
		align = PAGE_SIZE;
	size = ALIGN(size, PAGE_SIZE);

retry:

	ret = mwv207d_bo_create(mdev, size, align, kernel, domain,
				flags, NULL, NULL, &mbo);
	if (ret) {
		if (ret != -ERESTARTSYS) {
			if (flags & 0x1) {
				flags &= ~0x1;
				goto retry;
			}
			if (domain == 0x2) {
				domain |= 0x1;
				goto retry;
			}
			DRM_DEBUG("Failed to allocate GEM object (%lld, %d, %llu, %d)\n",
				  size, domain, align, ret);
		}
		return ret;
	}
	*obj = &mbo->tbo.base;
	(*obj)->funcs = &mwv207d_gem_object_funcs;

	return 0;
}

int mwv207d_gem_dumb_create(struct drm_file *file, struct drm_device *dev,
			    struct drm_mode_create_dumb *args)
{
	struct mwv207d_device *mdev = drm_to_mdev(dev);
	struct drm_gem_object *gobj;
	u32 domain, handle;
	int ret;

	domain = mdev->hw.is_emulation ? 0x1 : 0x2;
	args->pitch = ALIGN(args->width * DIV_ROUND_UP(args->bpp, 8), 64);
	args->size = args->pitch * args->height;
	args->size = ALIGN(args->size, PAGE_SIZE);
	ret = mwv207d_gem_object_create(mdev, args->size, 0x10000,
					domain, 0x1,
					false, &gobj);
	if (ret)
		return ret;

	ret = drm_gem_handle_create(file, gobj, &handle);
	drm_gem_object_put(gobj);
	if (ret)
		return ret;
	args->handle = handle;
	return 0;
}

int mwv207d_gem_create_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *filp)
{
	struct mwv207d_device *mdev = drm_to_mdev(dev);
	union drm_mwv207d_gem_create *args = data;
	struct drm_gem_object *gobj;
	int ret;

	if (args->in.size == 0)
		return -EINVAL;
	if (args->in.alignment & (args->in.alignment - 1))
		return -EINVAL;
	if (args->in.preferred_domain & ~MWV207D_GEM_DOMAIN_MASK)
		return -EINVAL;
	if (args->in.flags & ~MWV207D_GEM_CREATE_MASK)
		return -EINVAL;

	ret = mwv207d_gem_object_create(mdev, args->in.size,
					args->in.alignment,
					args->in.preferred_domain,
					args->in.flags, false, &gobj);
	if (ret)
		return ret;

	ret = drm_gem_handle_create(filp, gobj, &args->out.handle);
	drm_gem_object_put(gobj);

	return ret;
}

int mwv207d_gem_mmap_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *filp)
{
	union drm_mwv207d_gem_mmap *args = data;
	struct drm_gem_object *obj;

	if (args->in.pad)
		return -EINVAL;

	obj = drm_gem_object_lookup(filp, args->in.handle);
	if (!obj)
		return -ENOENT;
	args->out.offset = mwv207d_bo_mmap_offset(obj);
	drm_gem_object_put(obj);

	return 0;
}

int mwv207d_gem_wait_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *filp)
{
	struct drm_mwv207d_gem_wait *args = data;
	long timeout;
	bool write;
	int ret;

	if (args->op & ~(0x2 | 0x1))
		return -EINVAL;

	write = args->op & 0x2;
	timeout = drm_timeout_abs_to_jiffies(args->timeout);

	ret = drm_gem_dma_resv_wait(filp, args->handle, write, timeout);
	if (ret == -ETIME)
		ret = timeout ? -ETIMEDOUT : -EBUSY;

	return ret;
}

int mwv207d_gem_va_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *filp)
{
	struct mwv207d_fpriv *fpriv = to_fpriv(filp);
	struct drm_mwv207d_gem_va *args = data;
	struct mwv207d_vm *vm = fpriv->vm;
	struct mwv207d_bo_vm *bo_vm;
	struct drm_gem_object *obj;
	struct mwv207d_bo *mbo;
	int ret;

	if (args->padding)
		return -EINVAL;
	if (args->op == 0x3) {
		struct drm_mwv207d_gem_va_resv resv[0x10];
		int i;

		ret = mutex_lock_interruptible(&vm->mutex);
		if (ret)
			return ret;

		if (args->size < vm->nr_reserved_maps) {
			mutex_unlock(&vm->mutex);
			return -ERANGE;
		}

		args->size = vm->nr_reserved_maps;
		for (i = 0; i < args->size; i++) {
			resv[i].base = vm->resvd[i].start;
			resv[i].size = vm->resvd[i].last + 1 - resv[i].base;
		}
		ret = copy_to_user((void __user *)args->va,
				resv, args->size * sizeof(resv[0]));
		mutex_unlock(&vm->mutex);
		return ret;
	}

	if (args->flags & ~MWV207D_VM_PAGE_MASK)
		return -EINVAL;
	if ((args->offset & ~PAGE_MASK) || (args->size & ~PAGE_MASK) ||
			(args->va & ~PAGE_MASK) || args->size == 0)
		return -EINVAL;
	if (args->offset + args->size <= args->offset)
		return -ERANGE;
	if (args->op == 0x2 && args->offset != 0)
		return -EINVAL;

	obj = drm_gem_object_lookup(filp, args->handle);
	if (!obj)
		return -ENOENT;
	mbo = gbo_to_mbo(obj);

	if (args->offset + args->size > mwv207d_bo_size(mbo)) {
		ret = -ERANGE;
		goto out;
	}

	ret = mwv207d_bo_reserve(mbo, false);
	if (ret)
		goto out;
	bo_vm = mwv207d_bo_vm_find(mbo, vm);
	if (!bo_vm) {
		ret = -ENOENT;
		goto unreserve;
	}

	switch (args->op) {
	case 0x1:
		ret = mwv207d_mapping_create(bo_vm, args->offset,
				args->va, args->size, args->flags);
		if (!ret)
			bo_vm->nr_maps++;
		break;
	case 0x2:
		ret = mwv207d_mapping_remove(bo_vm, args->va,
				args->va + args->size - 1);
		if (!ret)
			bo_vm->nr_maps--;
		break;
	default:
		ret = -EINVAL;
	}

unreserve:
	mwv207d_bo_unreserve(mbo);
out:
	drm_gem_object_put(obj);

	return ret;
}

int mwv207d_gem_metadata_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *filp)
{
	struct drm_mwv207d_gem_metadata *args = data;
	struct drm_gem_object *obj;
	struct mwv207d_bo *mbo;
	int ret;

	obj = drm_gem_object_lookup(filp, args->handle);
	if (!obj)
		return -ENOENT;
	mbo = gbo_to_mbo(obj);

	ret = mwv207d_bo_reserve(mbo, false);
	if (unlikely(ret))
		goto out;

	if (args->op == 0x2) {
		mwv207d_bo_get_tiling_flags(mbo, &args->tiling_flags);
		ret = mwv207d_bo_get_metadata(mbo, args->metadata,
					      sizeof(args->metadata),
					      &args->metadata_size,
					      &args->metadata_flags);
	} else if (args->op == 0x1) {
		if (args->metadata_size > sizeof(args->metadata)) {
			ret = -EINVAL;
			goto unreserve;
		}
		ret = mwv207d_bo_set_tiling_flags(mbo, args->tiling_flags);
		if (!ret)
			ret = mwv207d_bo_set_metadata(mbo, args->metadata,
						      args->metadata_size,
						      args->metadata_flags);
	} else {
		ret = -EINVAL;
	}

unreserve:
	mwv207d_bo_unreserve(mbo);
out:
	drm_gem_object_put(obj);
	return ret;
}
