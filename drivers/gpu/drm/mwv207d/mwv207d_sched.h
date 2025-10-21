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
#ifndef MWV207D_SCHED_H_TGQQ2LXS
#define MWV207D_SCHED_H_TGQQ2LXS

#include <linux/list.h>
#include <linux/xarray.h>
#include <drm/gpu_scheduler.h>
#include <drm/ttm/ttm_execbuf_util.h>

#include "mwv207d_drv.h"
#include "mwv207d_bo.h"
#include "mwv207d_vbios.h"

#define CLUSTER_MASK(_n) ((1u << (_n)) - 1)

struct mwv207d_vm;

struct mwv207d_tvb {
	struct ttm_validate_buffer base;
	unsigned int nr_maps;
	struct mwv207d_mapping **mapping;
};

struct mwv207d_fence {
	struct dma_fence base;
	union {
		struct list_head q;
		u32 *rpos;
	};
};

#define to_mwv207d_fence(fence) container_of(fence, struct mwv207d_fence, base)
#define to_mwv207d_job(job) container_of(job, struct mwv207d_job, base)
struct mwv207d_job {
	struct drm_sched_job base;

	struct list_head tvblist;
	struct mwv207d_tvb *mtvb;

	int cmd_mode;
	int cmd_size;
	union {
		char *cmd_ptr;
		uint64_t cmd_va;
		struct drm_mwv207d_submit_cmds *cmd_array;
	};

	struct xarray deps;
	unsigned long last_dep;

	struct mwv207d_ctx *ctx;
	struct drm_sched_entity *engine_entity;
	struct mutex *qlock;

	struct kref refcount;

	struct mwv207d_vm *replaced_vm;

	struct mwv207d_fence hw_fence;

	char comm[TASK_COMM_LEN];

	struct drm_mwv207d_submit *submit;
};

#define mwv207d_for_each_mtvb(mtvb, mjob) \
	list_for_each_entry(mtvb, &(mjob)->tvblist, base.head)

struct mwv207d_job *mwv207d_job_alloc(void);
struct mwv207d_job *mwv207d_job_get(struct mwv207d_job *mjob);
void mwv207d_job_put(struct mwv207d_job *mjob);
int mwv207d_job_add_resv_fence(struct mwv207d_job *mjob,
		struct dma_resv *resv, int write);

struct mwv207d_dma_loc {
	u64 base;
	u64 pg_nr_type;
	u64 offset;
	u64 stride;
};

struct mwv207d_dma_cmd {
	struct mwv207d_dma_loc src;
	struct mwv207d_dma_loc dst;
	u64 width;
	u64 height;
};

struct mwv207d_pipe_info {
	char name[12];
	int id;
	u32 reg_base;
	u32 pf_irq;
	u32 vf_irq;
};

struct mwv207d_pipe_usage {
	spinlock_t lock;
	ktime_t busy_time;
	ktime_t idle_time;
	ktime_t period_start;
	ktime_t last_busy_time;
	ktime_t last_idle_time;
	ktime_t last_update_time;
	int busy_count;
};

struct mwv207d_pipe {
	struct dma_fence* (*submit)(struct mwv207d_pipe *pipe,
				    struct mwv207d_job *job);
	void (*reset)(struct mwv207d_pipe *pipe);
	void (*destroy)(struct mwv207d_pipe *pipe);
	void (*dump_state)(struct mwv207d_pipe *pipe);
	int  (*attach_vm)(struct mwv207d_pipe *pipe, struct mwv207d_vm *vm);
	int (*detach_vm)(struct mwv207d_pipe *pipe, struct mwv207d_vm *vm);
	const char *fname;
	enum mwv207d_pll_id pll_id;
	u8 type;
	u8 id;
	struct mwv207d_pipe_usage usage;
	struct mwv207d_device *mdev;
};

#define to_mwv207d_sched(sched) container_of(sched, struct mwv207d_sched, base)
struct mwv207d_sched {
	struct drm_gpu_scheduler base;
	struct mwv207d_pipe *pipe;

	u64 fence_ctx;
	u64 fence_seqno;
	spinlock_t fence_lock;
};

int  mwv207d_sched_init(struct mwv207d_device *mdev);
void mwv207d_sched_fini(struct mwv207d_device *mdev);
int  mwv207d_sched_suspend(struct mwv207d_device *mdev);
void mwv207d_sched_resume(struct mwv207d_device *mdev);

int mwv207d_sched_key_to_type(enum drm_mwv207d_info_key key);
int mwv207d_sched_key_to_nr_pipe(struct mwv207d_device *mdev,
				 enum drm_mwv207d_info_key key);
struct drm_gpu_scheduler **mwv207d_sched_key_to_sched(
		struct mwv207d_device *mdev, enum drm_mwv207d_info_key key);

void mwv207d_pipe_record_busy(struct mwv207d_pipe *pipe);
void mwv207d_pipe_record_idle(struct mwv207d_pipe *pipe);

struct mwv207d_pipe *mwv207d_pipe_3d_create(struct mwv207d_device *mdev,
					const struct mwv207d_pipe_info *info);
struct mwv207d_pipe *mwv207d_pipe_2d_create(struct mwv207d_device *mdev,
					const struct mwv207d_pipe_info *info);
struct mwv207d_pipe *mwv207d_pipe_dma_create(struct mwv207d_device *mdev,
					const struct mwv207d_pipe_info *info);
struct mwv207d_pipe *mwv207d_pipe_codec_create(struct mwv207d_device *mdev,
					const struct mwv207d_pipe_info *info);

u32 mwv207d_pipe_get_usage(struct mwv207d_device *mdev,
			   u8 pipe_type, u8 pipe_id);

void mwv207d_pipe_detach_vm(struct mwv207d_device *mdev, struct mwv207d_vm *vm);
int mwv207d_pipe_attach_vm(struct mwv207d_device *mdev, struct mwv207d_vm *vm);
#endif
