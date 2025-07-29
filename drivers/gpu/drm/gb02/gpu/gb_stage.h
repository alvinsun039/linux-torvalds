/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __GB_JOB_H__
#define __GB_JOB_H__

#include <drm/gpu_scheduler.h>

extern int job_timeout_ms;
//#define DUMP_BO_VRAM_DATA
#define GB02MAC1952 	1
#define GB02MAC1953     4
#define GB02MAC1954		10
#define GB02MAC1955(j)                   (0x10001 << (j))
#define GB02MAC1956(j)             BIT((j) + 16)
#define GB02MAC1957(j)            BIT(j)
#define GB02MAC1958	1

#define GB02MAC1959(GB02MAC288)	((1 << GB02MAC288) - 1)
#define GB02MAC1960                64

struct GB02STR39;
struct GB02STR59;
struct GB02STR47;

struct GB02STR162 {
	struct drm_sched_job base;

	struct kref refcount;

	struct GB02STR39 *gbdev;
	struct GB02STR47 *file_priv;

	struct dma_fence **in_fences;
	u32 in_fence_count;

	struct dma_fence *irq_done_fence;
	int as;

	__u64 sc;
	__u32 slot_req;
	__u32 flush_id;

	struct dma_fence **implicit_fences;
	struct drm_gem_object **bos;
	u32 bo_count;

	struct dma_fence *render_done_fence;

	struct dma_fence *last_scheduled;
	u32 reset_time;

	u64 user_pid;
};

struct GB02STR163 {
	struct spsc_node queue_node;
	struct dma_fence_cb cb;
	struct drm_sched_job *job;
	struct task_struct *pcb;
	struct drm_gpu_scheduler *sched;
};

enum stage_run_err_t {
       OK_SUBMITED_JOB = 0,
       ERR_FINISHED_JOB = -1,
       ERR_PENDING_RST = -2,
       ERR_FENCE_CREATE = -3,
       ERR_STAGES_BUSY = -4,
       ERR_SUBMITTING_JOB = -5,
};
struct GB02STR164 {
	struct drm_gpu_scheduler sched;
	atomic_t status;
	struct mutex lock;
	u64 fence_context;
	u64 emit_seqno;
};

struct GB02STR165 {
	struct GB02STR164 queue[GB02MAC288];
	spinlock_t fence_lock;
	spinlock_t stages_lock;
};

int GB02FUNC1230(char *time_buf);
int GB02FUNC1232(u64 size);
int GB02FUNC1355(struct GB02STR39 *gbdev);
void GB02FUNC1359(struct GB02STR39 *gbdev);
int GB02FUNC1363(struct GB02STR47 *gb_priv);
void GB02FUNC1365(struct GB02STR47 *gb_priv);
int GB02FUNC1279(struct GB02STR162 *stage);
void GB02FUNC1294(struct GB02STR162 *stage);
void GB02FUNC1305(struct GB02STR39 *gbdev);
void GB02FUNC1307(struct GB02STR39 *gbdev);
int GB02FUNC1368(struct GB02STR39 *gbdev);
void GB02FUNC1346(struct GB02STR47 *gb_priv);
void GB02FUNC1343(struct GB02STR39 *gbdev);
void GB02FUNC1352(struct GB02STR39 *gbdev);
void GB02FUNC1349(int time_ms);
void GB02FUNC1305(struct GB02STR39 *gbdev);
int GB02FUNC1312(void);
int GB02FUNC1248(struct GB02STR162 *stage);

#endif
