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
#ifndef MWV207D_BO_H_T4ETFCB2
#define MWV207D_BO_H_T4ETFCB2

#include <linux/types.h>
#include <drm/drm_gem.h>
#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_placement.h>

#include "mwv207d_drv.h"
#include "mwv207d_drm.h"

struct mwv207d_bo {
	struct ttm_buffer_object  tbo;
	struct ttm_placement      placement;
	struct ttm_bo_kmap_obj    kmap;
	struct ttm_place          placements[3];
	u32                       prime_shared_count;
	u32                       domain;
	u32                       flags;
	void                     *kptr;
	struct mwv207d_device    *mdev;

	struct list_head          bo_vm_list;

	u64                       tiling_flags;
	u64                       metadata_flags;
	void                      *metadata;
	u32                       metadata_size;
};

#define to_mbo(bo) container_of(bo, struct mwv207d_bo, tbo)
#define gbo_to_mbo(_gbo) container_of(_gbo, struct mwv207d_bo, tbo.base)

static inline u64 mwv207d_bo_size(struct mwv207d_bo *mbo)
{
	return mbo->tbo.base.size;
}

static inline u64 mwv207d_bo_mmap_offset(struct drm_gem_object *gobj)
{
	struct mwv207d_bo *mbo = gbo_to_mbo(gobj);

	return drm_vma_node_offset_addr(&mbo->tbo.base.vma_node);
}

static inline u64 mwv207d_bo_gpu_offset(struct mwv207d_bo *mbo)
{
	u64 addr = mbo->tbo.resource->start << PAGE_SHIFT;

	if (mbo->tbo.resource->mem_type == TTM_PL_TT)
		addr += 0x1000000000ULL;

	return addr;
}

void mwv207d_bo_placement_from_domain(struct mwv207d_bo *mbo, u32 domain);

bool mwv207d_ttm_bo_is_mwv207d_bo(struct ttm_buffer_object *tbo);

int mwv207d_bo_create(struct mwv207d_device *mdev, u64 size,
		      u64 byte_align, bool kernel, u32 domain, u32 flags,
		      struct sg_table *sg, struct dma_resv *resv,
		      struct mwv207d_bo **bo_ptr);

int mwv207d_bo_create_pin_mapped(struct mwv207d_device *mdev,
				 u64 size, u64 align, u32 domain, u32 flags,
				 struct mwv207d_bo **mbop, u64 *pap, void **vap);
void mwv207d_bo_destroy_pin_mapped(struct mwv207d_bo *mbo);

int mwv207d_bo_reserve(struct mwv207d_bo *mbo, bool no_intr);
void mwv207d_bo_unreserve(struct mwv207d_bo *mbo);

int mwv207d_bo_pin(struct mwv207d_bo *mbo, u32 domain, u64 *gpu_addr);
void mwv207d_bo_unpin(struct mwv207d_bo *mbo);

int mwv207d_bo_kmap(struct mwv207d_bo *mbo, void **ptr);
void mwv207d_bo_kunmap(struct mwv207d_bo *mbo);

struct mwv207d_bo *mwv207d_bo_ref(struct mwv207d_bo *mbo);
void mwv207d_bo_unref(struct mwv207d_bo **mbo);

vm_fault_t mwv207d_bo_fault_reserve_notify(struct ttm_buffer_object *bo);

int mwv207d_mm_init(struct mwv207d_device *mdev);
void mwv207d_mm_fini(struct mwv207d_device *mdev);
int mwv207d_mm_suspend(struct mwv207d_device *mdev);
void mwv207d_mm_resume(struct mwv207d_device *mdev);

int mwv207d_bo_set_tiling_flags(struct mwv207d_bo *bo, u64 tiling_flags);
void mwv207d_bo_get_tiling_flags(struct mwv207d_bo *bo, u64 *tiling_flags);
int mwv207d_bo_set_metadata(struct mwv207d_bo *bo, void *metadata,
			    u32 metadata_size, u64 flags);
int mwv207d_bo_get_metadata(struct mwv207d_bo *bo, void *buffer,
			    size_t buffer_size, u32 *metadata_size,
			    u64 *flags);

void mwv207d_update_memory_usage(struct ttm_buffer_object *bo, struct ttm_resource *mem,
				 bool add);

#endif
