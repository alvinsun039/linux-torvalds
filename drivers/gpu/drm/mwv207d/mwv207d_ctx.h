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
#ifndef MWV207D_CTX_H_QFMRESBO
#define MWV207D_CTX_H_QFMRESBO
#include <drm/drm_file.h>
#include <drm/gpu_scheduler.h>
#include "mwv207d_drv.h"

#define to_mwv207d_ctx_entity(entity) container_of(entity, struct mwv207d_ctx_entity, entity)
struct mwv207d_ctx_entity {
	struct drm_sched_entity	 entity;
	struct mutex             qlock;
	u32                      sequence;
	struct dma_fence        *fences[];
};

struct mwv207d_ctx {
	struct kref refcnt;
	struct mwv207d_ctx_entity
		*centities[0x6][0x8 + 1];
	struct mwv207d_device *mdev;
	atomic_t guilty;
	struct mwv207d_vm *vm;
};

struct mwv207d_ctx *mwv207d_ctx_lookup(struct drm_device *dev,
				       struct drm_file *filp, u32 handle);
int mwv207d_ctx_put(struct mwv207d_ctx *ctx);

void mwv207d_ctx_add_fence(struct mwv207d_ctx *ctx,
			   struct drm_sched_entity *entity,
			   struct dma_fence *fence);

int mwv207d_ctx_wait_prev_fence(struct mwv207d_ctx *ctx,
				struct drm_sched_entity *entity);

int  mwv207d_ctx_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *filp);

int mwv207d_ctx_mgr_init(struct drm_device *dev, struct mwv207d_ctx_mgr *mgr);
void mwv207d_ctx_mgr_fini(struct drm_device *dev, struct mwv207d_ctx_mgr *mgr);

int mwv207d_kctx_init(struct mwv207d_device *mdev);
void mwv207d_kctx_fini(struct mwv207d_device *mdev);

#endif
