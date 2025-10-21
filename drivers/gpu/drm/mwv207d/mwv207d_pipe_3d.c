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
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/list.h>
#include <linux/irqdomain.h>
#include <drm/ttm/ttm_tt.h>

#include "mwv207d_bo.h"
#include "mwv207d_sched.h"
#include "mwv207d_drm.h"
#include "mwv207d_irq.h"
#include "mwv207d_ctx.h"
#include "mwv207d_vm.h"
#include "mwv207d_vbios.h"

#define to_3d_pipe(pipe) container_of(pipe, struct mwv207d_pipe_3d, base)

struct mwv207d_pipe_3d {
	struct mwv207d_pipe base;
	struct mwv207d_bo *ringbuf_bo;
	u32  *ringbuf;
	u32  *head;
	u32  *tail;
	u32  *end;
	u64   ringbuf_gpu_addr;
	unsigned int irq;

	struct mwv207d_vm *executing_vm;

	void __iomem *regbase;

	u32 completed_fence;
	u32 cluster_mask;

	DECLARE_BITMAP(event_bitmap, 0x1E);
	struct dma_fence *event_fence[0x1E];
	spinlock_t event_lock;

	struct task_struct *poll_thread;
	u32 fe_id;
};

struct fe_stack {
	const char *name;
	u32 count;
	u32 highsel;
	u32 lowsel;
	u32 linksel;
	u32 clear;
	u32 next;
};

static inline void pipe_3d_write(struct mwv207d_pipe_3d *pipe,
				 u32 reg, u32 value)
{
	writel_relaxed(value, pipe->regbase + reg);
}

static inline u32 pipe_3d_read(struct mwv207d_pipe_3d *pipe, u32 reg)
{
	return readl_relaxed(pipe->regbase + reg);
}

static inline bool fence_after(u32 a, u32 b)
{
	return (s32)(a - b) > 0;
}

static inline struct dma_fence *
pipe_3d_event_pop_irq(struct mwv207d_pipe_3d *pipe, u32 event)
{
	struct dma_fence *fence;

	spin_lock(&pipe->event_lock);
	fence = pipe->event_fence[event];
	pipe->event_fence[event] = NULL;
	clear_bit(event, pipe->event_bitmap);
	spin_unlock(&pipe->event_lock);

	return fence;
}

static inline u32 pipe_3d_event_push(struct mwv207d_pipe_3d *pipe,
				     struct dma_fence *fence)
{
	u32 event;

	spin_lock_irq(&pipe->event_lock);
	event = find_first_zero_bit(pipe->event_bitmap, 0x1E);
	if (likely(event < 0x1E))
		pipe->event_fence[event] = dma_fence_get(fence);
	set_bit(event, pipe->event_bitmap);
	spin_unlock_irq(&pipe->event_lock);

	BUG_ON(event >= 0x1E);

	return event;
}

static inline u32 pipe_3d_ptr_span(void *end, void *start)
{
	return (unsigned long)end - (unsigned long)start;
}

static inline u64 pipe_3d_gpu_addr(struct mwv207d_pipe_3d *pipe, u32 *ptr)
{
	return pipe->ringbuf_gpu_addr + pipe_3d_ptr_span(ptr, pipe->ringbuf);
}

static inline void pipe_3d_link(struct mwv207d_pipe_3d *pipe,
				u32 prefetch, u64 addr)
{
	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	BUG_ON(pipe->tail >= pipe->end - 1
			|| (((prefetch + 7) / 8) & 0xffff0000)
			|| addr & 0x7);

	*pipe->tail++ = 0xB8000000 | ((prefetch + 7) / 8);
	*pipe->tail++ = 0x00000000;
	*pipe->tail++ = (u32)addr;
	*pipe->tail++ = (u32)(addr >> 32);
}

static inline void pipe_3d_wait(struct mwv207d_pipe_3d *pipe)
{
	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	BUG_ON(pipe->tail >= pipe->end);

	*pipe->tail++ = 0xE8000000 | 200;
	*pipe->tail++ = 0xabcdef78;
	*pipe->tail++ = 0xabcdef78;
	*pipe->tail++ = 0xabcdef78;
}

static inline void pipe_3d_wl(struct mwv207d_pipe_3d *pipe)
{
	u32 *nop = pipe->tail;

	pipe->tail = PTR_ALIGN(pipe->tail, 16);
	while (nop != pipe->tail)
		*nop++ = 0x18000000;

	pipe_3d_wait(pipe);
	pipe_3d_link(pipe, 32, pipe_3d_gpu_addr(pipe, pipe->tail) - 16);
}

static inline void pipe_3d_wtol(struct mwv207d_pipe_3d *pipe, u32 *w,
				u64 target, u32 size)
{
	BUG_ON((((size + 7) / 8) & 0xffff0000) || target & 0x7
			|| w < pipe->ringbuf || w >= pipe->end - 1);
	w[2] = (u32)target;
	w[3] = (u32)(target >> 32);
	w[1] = 0x00000000;
	dma_wmb();
	w[0] = 0xB8000000 | DIV_ROUND_UP(size, 8);
}

static inline void pipe_3d_loadstate(struct mwv207d_pipe_3d *pipe,
				     u32 reg, u32 val)
{
	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	BUG_ON(pipe->tail >= pipe->end - 1);

	*pipe->tail++ = 0x8010000 | reg;
	*pipe->tail++ = val;
}

static inline void pipe_3d_sem(struct mwv207d_pipe_3d *pipe, u32 to, u32 from, bool stall_prefetch)
{
	u32 fe_module = 0;

	if (stall_prefetch)
		fe_module =  0x3 << 28;

	from &= 0x1f;
	to   &= 0x1f;
	to  <<= 8;
	pipe_3d_loadstate(pipe, 0xE02, from | to | fe_module);
}

static inline void pipe_3d_stall(struct mwv207d_pipe_3d *pipe, u32 to, u32 from, bool stall_prefetch)
{
	u32 fe_module = 0;

	if (stall_prefetch)
		fe_module =  0x3 << 28;

	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	BUG_ON(pipe->tail >= pipe->end - 1);
	from &= 0x1f;
	to   &= 0x1f;
	to  <<= 8;
	*pipe->tail++ = 0x48000000;
	*pipe->tail++ = from | to | fe_module;
}

static inline void pipe_3d_start(struct mwv207d_pipe_3d *pipe)
{
	u32 *start = pipe->tail;
	u32 len;

	pipe_3d_loadstate(pipe, 0x0e44, pipe->cluster_mask);
	pipe_3d_loadstate(pipe, 0x0e21, 0x00020202);
	pipe_3d_loadstate(pipe, 0x0e80, 0x00000000);
	pipe_3d_loadstate(pipe, 0x7003, 0x00000001);

	pipe_3d_wl(pipe);

	len = (pipe->tail - start) * 4;
	pipe_3d_write(pipe, 0x14, 0xffffffff);
	pipe_3d_write(pipe, 0x300, pipe->ringbuf_gpu_addr >> 32);
	pipe_3d_write(pipe, 0x654, pipe->ringbuf_gpu_addr);
	dma_wmb();
	pipe_3d_write(pipe, 0x658, 0x10000 | (len / 8));
	pipe_3d_write(pipe, 0x3A4, 0x10000 | (len / 8));
}

static void pipe_3d_stop(struct mwv207d_pipe_3d *pipe)
{
}

static inline bool pipe_3d_is_reset(struct mwv207d_pipe_3d *pipe)
{
	return !!(pipe_3d_read(pipe, 0x3A8) & (0x1 << 5));
}

static int pipe_3d_hw_reset(struct mwv207d_pipe_3d *pipe)
{
	struct mwv207d_device *mdev = pipe->base.mdev;
	int ret;

	if (mdev->hw.is_pf || mdev->hw.is_emulation) {
		pipe_3d_write(pipe, 0x8200, 0x20202001);
		pipe_3d_write(pipe, 0x3A8, 1);
		ret = wait_for(pipe_3d_is_reset(pipe), 2000);
	} else
		ret = mwv207d_hw_vf_reset_3d(mdev, 2000);

	if (ret)
		pr_err("error, reset %s failed", pipe->base.fname);
	return ret;
}

static void pipe_3d_mmu_write(void *priv, u32 offset, u32 val)
{
	struct mwv207d_pipe_3d *pipe = priv;

	if (offset == 0x1ac || offset == 0x1b4) {
		struct ttm_buffer_object *bo;
		u64 gpu_addr;
		u32 len = 0;
		int ret;

		bo = &pipe->ringbuf_bo->tbo;
		if (pipe->base.mdev->gart || bo->resource->mem_type == TTM_PL_VRAM)
			gpu_addr = pipe->ringbuf_gpu_addr;
		else
			gpu_addr = bo->ttm->dma_address[0] + 0x1000000000ULL;

		pipe->ringbuf[len++] = 0x08010000 | (offset / 4);
		pipe->ringbuf[len++] = val;
		pipe->ringbuf[len++] = 0x10000000;
		pipe->ringbuf[len++] = 0x00000000;
		pipe_3d_write(pipe, 0x300, gpu_addr >> 32);
		pipe_3d_write(pipe, 0x654, gpu_addr);
		mb();
		len *= 4;
		pipe_3d_write(pipe, 0x658, 0x10000 | (len / 8));
		pipe_3d_write(pipe, 0x3A4, 0x10000 | (len / 8));

		ret = wait_for(pipe_3d_read(pipe, 0x4) & 0x1, 2000);
		if (ret)
			pr_err("failed to set 3d mmu page_table id");

		return;
	}

	pipe_3d_write(pipe, offset, val);
}

static int pipe_3d_hw_init(struct mwv207d_pipe_3d *pipe)
{
	u32 id = pipe->fe_id << 20;
	int ret;

	ret = pipe_3d_hw_reset(pipe);
	if (ret)
		return ret;
	pipe_3d_write(pipe, 0x0, id | 0x00010900);

	pipe_3d_write(pipe, 0x55C, 0x00ffffff);
	pipe_3d_write(pipe, 0x414, 0x3c000000);
	pipe_3d_write(pipe, 0x90,
			pipe_3d_read(pipe, 0x90) & 0xffffffbf);

	pipe_3d_write(pipe, 0x0, id | 0x00010900);
	pipe_3d_write(pipe, 0x0, id | 0x00070100);
	pipe_3d_write(pipe, 0x3A8, 0x2);

	pipe_3d_write(pipe, 0x3C, 0xffffffff);
	pipe_3d_write(pipe, 0x3C, 0x00000000);

	pipe_3d_write(pipe, 0x100, 0x00140021);

	mwv207d_mmu_init(pipe->base.mdev, pipe->executing_vm,
			pipe_3d_mmu_write, pipe);

	pipe_3d_write(pipe, 0x154, 0x1);
	udelay(10);
	mb();
	pipe_3d_write(pipe, 0x104, 0x00430408);
	pipe_3d_write(pipe, 0x10C, 0x015b0880);

	pipe_3d_write(pipe, 0x15C, 0x2);
	pipe_3d_read(pipe, 0x10);

	return 0;
}

static void mwv207d_pipe_3d_reset(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_3d *pipe = to_3d_pipe(mpipe);
	int i;

	spin_lock_irq(&pipe->event_lock);
	bitmap_zero(pipe->event_bitmap, 0x1E);
	for (i = 0; i < 0x1E; i++) {
		if (!pipe->event_fence[i])
			continue;
		dma_fence_put(pipe->event_fence[i]);
		pipe->event_fence[i] = NULL;
		mwv207d_pipe_record_idle(mpipe);
	}
	spin_unlock_irq(&pipe->event_lock);

	if (mpipe->mdev->hw.is_pf || mpipe->mdev->hw.is_emulation)
		pipe_3d_write(pipe, 0x83bc, pipe->cluster_mask);

	if (pipe_3d_hw_init(pipe))
		return;
	pipe->tail = pipe->ringbuf;
	pipe->head = pipe->ringbuf;

	pipe_3d_start(pipe);
}

static u32 *pipe_3d_wait_for_space(struct mwv207d_pipe_3d *pipe, u32 size)
{
	u32 *head;
	int i;

	for (i = 0; i < 10000; ++i) {
		head = READ_ONCE(pipe->head);
		if (head <= pipe->tail) {
			if (pipe_3d_ptr_span(pipe->end, pipe->tail) >= size)
				return pipe->tail;
			if (pipe_3d_ptr_span(head, pipe->ringbuf) >= size)
				return pipe->ringbuf;
		} else {
			if (pipe_3d_ptr_span(head, pipe->tail) >= size)
				return pipe->tail;
		}
		usleep_range(200, 200);
	}
	return NULL;
}

static void pipe_3d_call(struct mwv207d_pipe_3d *pipe,
			 u64 va, u32 size, u32 **ret_prefetch)
{
	u64 ret_addr;

	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	*pipe->tail++ = 0xF0000000 | DIV_ROUND_UP(size, 8);

	*ret_prefetch = pipe->tail;
	*pipe->tail++ = DIV_ROUND_UP(24, 8);

	*pipe->tail++ = va & 0xffffffff;
	*pipe->tail++ = va >> 32;

	ret_addr = pipe_3d_gpu_addr(pipe, pipe->tail) + 8;
	*pipe->tail++ = ret_addr & 0xffffffff;
	*pipe->tail++ = ret_addr >> 32;
}

static struct dma_fence *mwv207d_pipe_3d_submit(struct mwv207d_pipe *mpipe,
						struct mwv207d_job *mjob)
{
	struct mwv207d_pipe_3d *pipe = to_3d_pipe(mpipe);
	struct mwv207d_fence *fence = &mjob->hw_fence;
	struct mwv207d_vm *vm;
	u32 size, event, cmd_prefetch = 0;
	u32 *ret_start = NULL, *ret_prefetch = NULL;
	u32 *last_tail, *start;
	bool stall_prefetch = false;

	if (mjob->cmd_mode == 0x0)
		size = ALIGN(mjob->cmd_size, 8);
	else if (mjob->cmd_mode == 0x1)
		size = 0;
	else
		size = 256UL * mjob->cmd_size;

	size += 256;

	last_tail = pipe->tail;
	start = pipe_3d_wait_for_space(pipe, size);
	if (!start) {
		pr_err("error, %s is stuck!", mpipe->fname);

		return ERR_PTR(-EBUSY);
	}
	pipe->tail = start;

	vm = mjob->ctx ? mjob->ctx->vm : mpipe->mdev->vm;
	if (vm != pipe->executing_vm ||
			test_and_clear_bit(0x1, &vm->dirty)) {
		stall_prefetch = true;
		pipe_3d_loadstate(pipe, 0x0e44, pipe->cluster_mask);

		pipe_3d_loadstate(pipe, 0x006b, vm->pta_id);
		pipe_3d_loadstate(pipe, 0x006d, 0x80000000);

		pipe_3d_loadstate(pipe, 0x502E, 0x1);
		pipe_3d_loadstate(pipe, 0x50CE, pipe->cluster_mask);
		pipe_3d_sem(pipe, 0x10, 0x1, stall_prefetch);
		pipe_3d_stall(pipe, 0x10, 0x1, stall_prefetch);
		pipe_3d_loadstate(pipe, 0x502E, 0x0);

	}

	if (mjob->cmd_mode == 0x0) {
		memcpy_toio(pipe->tail, mjob->cmd_ptr, mjob->cmd_size);
		pipe->tail += mjob->cmd_size / 4;
	} else if (mjob->cmd_mode == 0x2) {
		int i;
		BUG_ON(mjob->cmd_size <= 0);

		for (i = 0; i < mjob->cmd_size; i++) {
			pipe_3d_call(pipe,
				     mjob->cmd_array[i].cmd_va,
				     mjob->cmd_array[i].cmd_size,
				     &ret_prefetch);
			if (i == 0)
				cmd_prefetch = pipe_3d_ptr_span(pipe->tail, start);
		}
		ret_start = pipe->tail;
	} else {
		pipe_3d_call(pipe, mjob->cmd_va, mjob->cmd_size, &ret_prefetch);
		cmd_prefetch = pipe_3d_ptr_span(pipe->tail, start);
		ret_start = pipe->tail;
	}

	pipe_3d_loadstate(pipe, 0xE03, 0xc63);
	pipe_3d_loadstate(pipe, 0x594, 0x41);

	pipe_3d_loadstate(pipe, 0x502E, 0x1);
	pipe_3d_loadstate(pipe, 0x502B, 0x3);
	pipe_3d_sem(pipe, 0x10, 0x1, stall_prefetch);
	pipe_3d_stall(pipe, 0x10, 0x1, stall_prefetch);
	pipe_3d_loadstate(pipe, 0x502E, 0x0);

	fence->rpos = pipe->tail;
	event = pipe_3d_event_push(pipe, &fence->base);

	pipe_3d_loadstate(pipe, 0x502E, 0x1);
	pipe_3d_loadstate(pipe, 0x50CE, pipe->cluster_mask);
	pipe_3d_loadstate(pipe, 0xE01, 0x80 | event);
	pipe_3d_loadstate(pipe, 0x502E, 0x0);

	if (mjob->replaced_vm)
		mwv207d_vm_put(mjob->replaced_vm);
	mjob->replaced_vm = pipe->executing_vm;
	pipe->executing_vm = mwv207d_vm_get(vm);

	pipe_3d_wl(pipe);

	if (ret_prefetch)
		*ret_prefetch = DIV_ROUND_UP(
				pipe_3d_ptr_span(pipe->tail, ret_start),
				8);

	BUG_ON(pipe_3d_ptr_span(pipe->tail, start) > size);
	if (!cmd_prefetch)
		cmd_prefetch = pipe_3d_ptr_span(pipe->tail, start);

	mwv207d_pipe_record_busy(mpipe);
	pipe_3d_wtol(pipe, last_tail - 8, pipe_3d_gpu_addr(pipe, start),
			cmd_prefetch);

	return dma_fence_get(&fence->base);
}

static inline u64 pipe_get_current_addr(struct mwv207d_pipe_3d *pipe)
{
	u32 low, high;

	low = pipe_3d_read(pipe, 0x664);
	high = pipe_3d_read(pipe, 0x304);

	return ((u64)high << 32) | low;
}

static void mwv207d_pipe_3d_dump_fe_stack(struct mwv207d_pipe_3d *pipe,
					  u32 index, u32 data)
{
	static struct fe_stack fe_stacks[2] = {
		{"PRE_STACK", 32, 0x1A, 0x9A, 0x00, 0x1B, 0x1E},
		{"CMD_STACK", 32, 0x1C, 0x9C, 0x1E, 0x1D, 0x1E},
	};
	u32 stack[32][2];
	u32 link[32];
	int i, j;

	for (i = 0; i < 2; ++i) {
		pipe_3d_write(pipe, index, fe_stacks[i].clear);
		for (j = 0; j < fe_stacks[i].count; j++) {
			pipe_3d_write(pipe, index, fe_stacks[i].highsel);
			stack[j][0] = pipe_3d_read(pipe, data);
			pipe_3d_write(pipe, index, fe_stacks[i].lowsel);
			stack[j][1] = pipe_3d_read(pipe, data);
			pipe_3d_write(pipe, index, fe_stacks[i].next);
			if (fe_stacks[i].linksel) {
				pipe_3d_write(pipe, index, fe_stacks[i].linksel);
				link[j] = pipe_3d_read(pipe, data);
			}
		}
		pr_info("mwv207d %s:", fe_stacks[i].name);
		for (j = 31; j >= 3; j -= 4)
			pr_info("%08x %08x %08x %08x %08x %08x %08x %08x",
				 stack[j][0], stack[j][1],
				 stack[j - 1][0], stack[j - 1][1],
				 stack[j - 2][0], stack[j - 2][1],
				 stack[j - 3][0], stack[j - 3][1]);
		if (fe_stacks[i].linksel) {
			pr_info("mwv207d LINK STACK:");
			for (j = 31; j >= 3; j -= 4)
				pr_info("%08x %08x %08x %08x %08x %08x %08x %08x",
					 link[j], link[j],
					 link[j - 1], link[j - 1],
					 link[j - 2], link[j - 2],
					 link[j - 3], link[j - 3]);
		}
	}
}

static void mwv207d_pipe_3d_dump_state(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_3d *pipe = to_3d_pipe(mpipe);
	u32 state0, state1;
	u64 addr0, addr1;
	int i;

	state0 = pipe_3d_read(pipe, 0x4);
	addr0  = pipe_get_current_addr(pipe);
	for (i = 0; i < 500; ++i) {
		state1 = pipe_3d_read(pipe, 0x4);
		addr1  = pipe_get_current_addr(pipe);
		if (state1 != state0 || addr1 != addr0)
			break;
	}

	if (state1 != state0)
		pr_info("%s state changing, s0: 0x%08x, s1: 0x%08x", mpipe->fname, state0, state1);
	pr_info("%s current stat: 0x%08x", mpipe->fname, pipe_3d_read(pipe, 0x4));

	if (addr1 != addr0)
		pr_info("%s addr changing, a0: 0x%010llx, a1: 0x%010llx", mpipe->fname, addr0, addr1);
	pr_info("%s current addr: 0x%010llx", mpipe->fname, pipe_get_current_addr(pipe));

	pr_info("%s current cmdl: 0x%08x", mpipe->fname, pipe_3d_read(pipe, 0x668));
	pr_info("%s current cmdh: 0x%08x", mpipe->fname, pipe_3d_read(pipe, 0x66c));

	mwv207d_pipe_3d_dump_fe_stack(pipe, 0xf0, 0x450);
}

static irqreturn_t mwv207d_pipe_3d_isr(int irq_unused, void *dev_id)
{
	struct mwv207d_pipe_3d *pipe = dev_id;
	struct dma_fence *fence;
	u32 event, intr;

	intr = pipe_3d_read(pipe, 0x10);
	if (unlikely(!intr))
		return IRQ_NONE;

	if (unlikely(intr & 0x80000000)) {
		pr_warn("error, %s axi bus error", pipe->base.fname);
		intr &= ~0x80000000;
	}
	if (unlikely(intr & 0x40000000)) {
		pr_warn("error, %s mmu error", pipe->base.fname);
		pr_warn("error addr[0x380]: 0x%08x", pipe_3d_read(pipe, 0x380));
		pr_warn("error adhi[0x3b8]: 0x%08x", pipe_3d_read(pipe, 0x3b8));
		intr &= ~0x40000000;
	}
	while ((event = ffs(intr))) {
		event -= 1;
		intr  &= ~(1 << event);

		fence = pipe_3d_event_pop_irq(pipe, event);
		if (!fence) {
			pr_warn("unexpected %s event %d",
				pipe->base.fname, event);
			continue;
		}

		if (fence_after(fence->seqno, pipe->completed_fence)) {
			struct mwv207d_fence *pipe_fence = to_mwv207d_fence(fence);

			pipe->completed_fence = fence->seqno;
			pipe->head = pipe_fence->rpos;
		}

		mwv207d_pipe_record_idle(&pipe->base);
		dma_fence_signal(fence);
		dma_fence_put(fence);
	}
	return IRQ_HANDLED;
}

static int pipe_3d_polling(void *priv)
{
	set_freezable();

	while (!kthread_should_stop()) {
		usleep_range(1000, 10000);
		try_to_freeze();

		mwv207d_pipe_3d_isr(-1, priv);
	}

	return 0;
}

static int mwv207d_pipe_3d_attach_vm(struct mwv207d_pipe *mpipe, struct mwv207d_vm *vm)
{
	struct mwv207d_pipe_3d *pipe = to_3d_pipe(mpipe);
	int ret;

	ret = mwv207d_vm_map_bo(vm, pipe->ringbuf_bo,
				pipe->ringbuf_gpu_addr, 0x1);
	if (!ret) {
		mwv207d_vm_reserve(vm, pipe->ringbuf_gpu_addr,
				mwv207d_bo_size(pipe->ringbuf_bo));
		mwv207d_vm_reserve(vm, 0, SZ_4K);
	}

	return ret;
}

static void mwv207d_pipe_3d_destroy(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_3d *pipe = to_3d_pipe(mpipe);

	if (!mpipe->mdev->isr_poll)
		free_irq(pipe->irq, pipe);
	else
		kthread_stop(pipe->poll_thread);

	pipe_3d_stop(pipe);

	mwv207d_vm_put(pipe->executing_vm);

	mwv207d_bo_destroy_pin_mapped(pipe->ringbuf_bo);
}

static inline u32 pipe_3d_get_fe_id(int vf_sel)
{

	switch (vf_sel) {
	case 0x0 ... 0x7:
		return vf_sel;
	case 0x8 ... 0xb:
		return (vf_sel - 0x8) * 2;
	case 0xc ... 0xd:
		return (vf_sel - 0xc) * 4;
	case 0xf:
		return 0;
	default:
		BUG_ON(1);
		return 0;
	}
}

struct mwv207d_pipe *mwv207d_pipe_3d_create(struct mwv207d_device *mdev,
					    const struct mwv207d_pipe_info *info)
{
	struct mwv207d_pipe_3d *pipe;
	struct mwv207d_bo *mbo;
	void *logical;
	int ret;

	pipe = devm_kzalloc(mdev->dev, sizeof(*pipe), GFP_KERNEL);
	if (!pipe)
		return NULL;

	pipe->base.pll_id = MWV207D_PLL_CORE_3D;
	pipe->base.type = 0x0;
	pipe->base.id = info->id;
	pipe->base.mdev = mdev;
	pipe->base.fname = info->name;
	pipe->base.submit = mwv207d_pipe_3d_submit;
	pipe->base.reset  = mwv207d_pipe_3d_reset;
	pipe->base.destroy = mwv207d_pipe_3d_destroy;
	pipe->base.dump_state = mwv207d_pipe_3d_dump_state;
	pipe->base.attach_vm = mwv207d_pipe_3d_attach_vm;

	pipe->regbase = mdev->mmio + info->reg_base;
	pipe->cluster_mask = CLUSTER_MASK(mdev->hw.nr_3d_clusters);
	spin_lock_init(&pipe->event_lock);

	pipe->fe_id = pipe_3d_get_fe_id(mdev->hw.vf_sel);

	ret = mwv207d_bo_create_pin_mapped(mdev, SZ_512K, SZ_4K,
					   0x4,
					   0x1, &mbo,
					   &pipe->ringbuf_gpu_addr, &logical);
	if (ret)
		return NULL;

	ret = mwv207d_vm_map_bo(mdev->vm, mbo,
			pipe->ringbuf_gpu_addr, 0x1);
	if (ret)
		goto free_bo;
	pipe->ringbuf_bo = mbo;

	pipe->ringbuf = (u32 *)logical;
	pipe->end = pipe->ringbuf + (SZ_512K / 4);

	if (!mdev->isr_poll) {
		pipe->irq = mwv207d_irq_find(mdev, info->pf_irq, info->vf_irq);
		if (!pipe->irq) {
			pr_err("%s failed to find irq nr", info->name);
			goto free_bo;
		}

		ret = request_irq(pipe->irq, mwv207d_pipe_3d_isr, 0,
				  info->name, pipe);
		if (ret) {
			pr_err("%s failed to request irq", info->name);
			goto free_bo;
		}
	} else {
		pipe->poll_thread = kthread_run(pipe_3d_polling, pipe, info->name);
		if (IS_ERR(pipe->poll_thread)) {
			pr_err("%s failed to create poll task", info->name);
			goto free_bo;
		}
	}

	pipe->executing_vm = mwv207d_vm_get(pipe->base.mdev->vm);
	mwv207d_pipe_3d_reset(&pipe->base);

	return &pipe->base;
free_bo:
	mwv207d_bo_destroy_pin_mapped(mbo);
	return NULL;
}
