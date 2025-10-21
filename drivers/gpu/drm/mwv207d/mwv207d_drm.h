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
#ifndef MWV207D_DRM_H
#define MWV207D_DRM_H

#include "drm/drm.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define MWV207D_GEM_DOMAIN_MASK           (0x1 | \
					   0x2 | \
					   0x4)

#define MWV207D_GEM_CREATE_MASK           (0x1 | \
					   0x2 | \
					   0x4)
struct drm_mwv207d_gem_create_in {
	__u64 size;
	__u64 alignment;
	__u32 preferred_domain;
	__u32 flags;
};
struct drm_mwv207d_gem_create_out {
	__u32 handle;
	__u32 resv;
};
union drm_mwv207d_gem_create {
	struct drm_mwv207d_gem_create_in in;
	struct drm_mwv207d_gem_create_out out;
};

struct drm_mwv207d_gem_mmap_in {
	__u32 handle;
	__u32 pad;
};
struct drm_mwv207d_gem_mmap_out {
	__u64 offset;
};
union drm_mwv207d_gem_mmap {
	struct drm_mwv207d_gem_mmap_in in;
	struct drm_mwv207d_gem_mmap_out out;
};

#define MWV207D_VM_PAGE_MASK  (0x1 \
		| 0x2)

struct drm_mwv207d_gem_va_resv {
	__u64 base;
	__u64 size;
};
struct drm_mwv207d_gem_va {
	__u32 handle;
	__u32 op;
	__u32 flags;
	__u32 padding;

	__u64 va;
	__u64 offset;

	__u64 size;
};

struct drm_mwv207d_gem_wait {
	__u32 handle;
	__u32 op;
	__s64 timeout;
};

#define MWV207D_TILING_OFFSET_256B_SHIFT      0
#define MWV207D_TILING_OFFSET_256B_MASK       0xffffffff
#define MWV207D_TILING_MODE_SHIFT             32
#define MWV207D_TILING_MODE_MASK              0x7
#define MWV207D_TILING_COMPRESS_SHIFT         35
#define MWV207D_TILING_COMPRESS_MASK          0x1
#define MWV207D_TILING_COMPRESS_FORMAT_SHIFT  36
#define MWV207D_TILING_COMPRESS_FORMAT_MASK   0xf

#define MWV207D_TILING_MODE_LINEAR            0x0
#define MWV207D_TILING_MODE_SUPERX            0x1

#define MWV207D_TILING_COMPRESS_FORMAT_ARGB8  0x0

#define MWV207D_TILING_SET(field, value) \
	(((__u64)(value) & MWV207D_TILING_##field##_MASK) << MWV207D_TILING_##field##_SHIFT)
#define MWV207D_TILING_GET(value, field) \
	(((__u64)(value) >> MWV207D_TILING_##field##_SHIFT) & MWV207D_TILING_##field##_MASK)

struct drm_mwv207d_gem_metadata {
	__u32 handle;
	__u32 op;
	__u64 tiling_flags;
	__u64 metadata_flags;
	__u32 metadata_size;
	__u32 metadata[64];
};

struct drm_mwv207d_ctx {
	__u32 op;
	__u32 flags;
	__u32 handle;
	__u32 resv;
};

#define MWV207D_SUBMIT_FLAG_MASK   \
	(0x1 | \
	 0x2 | \
	 0x8 | \
	 0x10 | \
	 0x4 | \
	 0x20 | \
	 0x40 \
	 )

struct drm_mwv207d_submit {
	__u32 ctx;
	__u32 engine_type;
	__u32 engine_id;
	__u32 flags;
	__u32 fence_fd;
	__u32 cmd_size;
	__u32 nr_bos;
	__u32 padding;

	__u64 cmds;
	__u64 bos;
	__u32 nr_in_syncobjs;
	__u32 nr_out_syncobjs;
	__u64 in_syncobjs;
	__u64 out_syncobjs;
};

struct drm_mwv207d_submit_cmds {
	__u64 cmd_va;
	__u32 cmd_size;
	__u32 padding;
};

struct drm_mwv207d_submit_syncobj {
	__u32 handle;
	__u32 padding;
	__u64 point;
};

struct drm_mwv207d_bo_acc {
	__u32 handle;
	__u32 flags;
};

enum drm_mwv207d_info_key {
	DRM_MWV207D_ACTIVE_3D_NR = 0,
	DRM_MWV207D_ACTIVE_DEC_NR,
	DRM_MWV207D_ACTIVE_ENC_NR,
	DRM_MWV207D_ACTIVE_2D_NR,
	DRM_MWV207D_ACTIVE_DMA_NR,
	DRM_MWV207D_ACTIVE_FUS_NR,
	DRM_MWV207D_3D_CLUSTER_NR,
	DRM_MWV207D_2D_CLUSTER_NR,
	DRM_MWV207D_FUS_CLUSTER_NR,
	DRM_MWV207D_VRAM_SIZE,
	DRM_MWV207D_GTT_SIZE,
	DRM_MWV207D_VISIBLE_VRAM_SIZE,
	DRM_MWV207D_FAMILY,
};

struct drm_mwv207d_info_rec {
	__u32 key;
	__u32 val;
};

struct drm_mwv207d_info {
	__u32 op;
	__u32 nr_recs;
	__u64 recs;
};

#define DRM_IOCTL_MWV207D_INFO          \
	DRM_IOWR(DRM_COMMAND_BASE + 0x0, struct drm_mwv207d_info)
#define DRM_IOCTL_MWV207D_GEM_CREATE    \
	DRM_IOWR(DRM_COMMAND_BASE + 0x1, union drm_mwv207d_gem_create)
#define DRM_IOCTL_MWV207D_GEM_MMAP      \
	DRM_IOWR(DRM_COMMAND_BASE + 0x2, union drm_mwv207d_gem_mmap)
#define DRM_IOCTL_MWV207D_GEM_VA        \
	DRM_IOWR(DRM_COMMAND_BASE + 0x6, struct drm_mwv207d_gem_va)
#define DRM_IOCTL_MWV207D_GEM_WAIT      \
	DRM_IOWR(DRM_COMMAND_BASE + 0x3, struct drm_mwv207d_gem_wait)
#define DRM_IOCTL_MWV207D_CTX           \
	DRM_IOWR(DRM_COMMAND_BASE + 0x4, struct drm_mwv207d_ctx)
#define DRM_IOCTL_MWV207D_SUBMIT        \
	DRM_IOWR(DRM_COMMAND_BASE + 0x5, struct drm_mwv207d_submit)
#define DRM_IOCTL_MWV207D_GEM_METADATA  \
	DRM_IOWR(DRM_COMMAND_BASE + 0x7, struct drm_mwv207d_gem_metadata)

#if defined(__cplusplus)
}
#endif

#endif
