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

#include "mwv207d_sched.h"
#include "mwv207d_drm.h"
#include "mwv207d_irq.h"
#include "mwv207d_vm.h"
#include "mwv207d_ctx.h"
#include "mwv207d_vbios.h"

struct mwv207d_pipe_2d {
	struct mwv207d_pipe base;
	u32 w_pos;
	u32 r_pos;
	u32 units;
	u64 rb_gpu_addr;
	struct mwv207d_bo *rb_bo;
	void *rb_vaddr;
	unsigned int irq;

	void __iomem *regbase;
	u32 nr_clusters;
	int vm_identity;

	spinlock_t fence_queue_lock;
	struct list_head fence_queue;

	struct task_struct *poll_thread;

	u64         sync_va;
};

#define to_pipe_2d(p) container_of(p, struct mwv207d_pipe_2d, base)

static inline void pipe_2d_write(struct mwv207d_pipe_2d *pipe,
				 u32 reg, u32 value)
{
	writel_relaxed(value, pipe->regbase + reg);
}

static inline u32 pipe_2d_read(struct mwv207d_pipe_2d *pipe, u32 reg)
{
	return readl_relaxed(pipe->regbase + reg);
}

static void pipe_2d_mmu_write(void *priv, u32 offset, u32 val)
{
	struct mwv207d_pipe_2d *pipe = priv;
	u32 i, base = 0x8000;

	for (i = 0; i < pipe->nr_clusters; i++) {
		pipe_2d_write(pipe, base + offset, val);
		base += 0x1000;
	}
}

static void pipe_2d_mmu_read(void *priv, u32 reg)
{
	struct mwv207d_pipe_2d *pipe = priv;
	u32 i, base = 0x8000;

	for (i = 0; i < pipe->nr_clusters; i++) {
		pr_info("2d mmu[%d] reg[0x%x]=0x%08x", i, reg, pipe_2d_read(pipe, base + reg));
		base += 0x1000;
	}
}

static inline bool fence16_after(u16 a, u16 b)
{
	return (s16)(a - b) > 0;
}

static inline u32 pipe_2d_ptr_span(void *end, void *start)
{
	return (unsigned long)end - (unsigned long)start;
}

static inline u64 pipe_2d_gpu_addr(struct mwv207d_pipe_2d *pipe, u32 pos)
{
	return pipe->rb_gpu_addr + (pos * 16);
}

static int pipe_2d_reset_hw(struct mwv207d_pipe_2d *pipe)
{
	struct mwv207d_vm *vm;
	int ret;

	pipe_2d_write(pipe, 0x500, 1);
	ret = wait_for(!pipe_2d_read(pipe, 0x500), 100);
	if (ret) {
		pr_err("error, reset %s core failed", pipe->base.fname);
		return ret;
	}

	mb();
	udelay(50);

	pipe_2d_write(pipe, 0x704, 0);
	ret = wait_for(pipe_2d_read(pipe, 0x704), 100);
	if (ret)
		pr_err("error, reset %s status ram failed", pipe->base.fname);

	vm = pipe->base.mdev->vm;
	BUG_ON(vm->pta_id != 0);
	mwv207d_mmu_init(pipe->base.mdev, vm, pipe_2d_mmu_write, pipe);
	pipe->vm_identity = vm->identity;
	pipe->sync_va = vm->pgd.pt.phys_addr;

	ret = mwv207d_vm_map_linear(vm, pipe->sync_va, SZ_4K,
			pipe->sync_va, 0x1);
	if (ret)
		pr_err("failed to map kernel vm");
	return ret;
}

static void pipe_2d_reset_fence(struct mwv207d_pipe_2d *pipe)
{
	struct mwv207d_fence *fence, *p;
	unsigned long flags;

	spin_lock_irqsave(&pipe->fence_queue_lock, flags);
	list_for_each_entry_safe(fence, p, &pipe->fence_queue, q) {
		list_del(&fence->q);
		dma_fence_put(&fence->base);
		mwv207d_pipe_record_idle(&pipe->base);
	}
	spin_unlock_irqrestore(&pipe->fence_queue_lock, flags);
}

static inline void pipe_2d_set_native_endian(struct mwv207d_pipe_2d *pipe)
{
#ifdef __BIG_ENDIAN
	pipe_2d_write(pipe, 0x100, 1 << 31);
#else
	pipe_2d_write(pipe, 0x100, 0 << 31);
#endif
}

static u32 *pipe_2d_emit(u32 *cmd, u32 reg, u32 val)
{
	*cmd++ = 0x40000000 | reg;
	*cmd++ = val;
	return cmd;
}

static void pipe_2d_start(struct mwv207d_pipe_2d *pipe)
{
	u32 ctl = (u32)(pipe->rb_gpu_addr & 0xffffffff) >> 4;
	u32 ext = (u32)(pipe->rb_gpu_addr >> 32);
	u32 *w_pos = (u32 *)pipe->rb_vaddr;

	pipe->w_pos = pipe->r_pos = 0;
	pipe_2d_write(pipe, 0x404, 0xd | 0x1 << 8);
	pipe_2d_write(pipe, 0x200, ctl);
	pipe_2d_write(pipe, 0x220, ext);
	pipe_2d_write(pipe, 0x204, 0);
	pipe_2d_write(pipe, 0x208, pipe->w_pos);
	pipe_2d_write(pipe, 0x20C, pipe->r_pos);
	pipe_2d_write(pipe, 0x224, 0);
	pipe_2d_write(pipe, 0x214, pipe->units - 1);
	pipe_2d_set_native_endian(pipe);
	pipe_2d_write(pipe, 0x210, 1);

	w_pos = pipe_2d_emit(w_pos, 0x4, ffs(pipe->nr_clusters));
	*w_pos++ = 0x80000000;
	*w_pos++ = 0x80000000;

	mb();
	pipe->w_pos = 1;
	pipe_2d_write(pipe, 0x208, pipe->w_pos);
}

static inline void pipe_2d_stop(struct mwv207d_pipe_2d *pipe)
{
}

static void mwv207d_pipe_2d_reset(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_2d *pipe = to_pipe_2d(mpipe);

	if (pipe_2d_reset_hw(pipe))
		return;
	pipe_2d_reset_fence(pipe);
	pipe_2d_start(pipe);
}

static inline bool space_available(struct mwv207d_pipe_2d *pipe, u32 size)
{
	u32 avail;

	pipe->r_pos = pipe_2d_read(pipe, 0x20C);

	avail = (pipe->units - 1 + pipe->r_pos - pipe->w_pos) % pipe->units;

	return avail >= (size / 16);
}

static int pipe_2d_wait_for_space(struct mwv207d_pipe_2d *pipe, u32 size)
{
	if ((size / 16) >= pipe->units)
		return -EINVAL;
	return wait_for(space_available(pipe, size), 100) ? -EBUSY : 0;
}

static u32 pipe_2d_submit_cmd(struct mwv207d_pipe_2d *pipe,
			      const void *cmds, u32 cmd_size, u32 w_pos_byte)
{
	u32 size = ALIGN(cmd_size, 16);
	u32 r_pos_byte, rb_size, nop_cnt;

	r_pos_byte = pipe->r_pos * 16;
	rb_size = pipe->units * 16;

	if (r_pos_byte > w_pos_byte) {
		memcpy(pipe->rb_vaddr + w_pos_byte,
			    cmds, cmd_size);
		w_pos_byte += cmd_size;
	} else {
		int cp_len = min_t(u32, cmd_size, rb_size - w_pos_byte);

		memcpy(pipe->rb_vaddr + w_pos_byte, cmds, cp_len);

		if (cp_len < cmd_size) {
			memcpy(pipe->rb_vaddr, cmds + cp_len,
				    cmd_size - cp_len);
			w_pos_byte = cmd_size - cp_len;
		} else {
			w_pos_byte = (w_pos_byte + cp_len) % rb_size;
		}
	}

	BUG_ON(w_pos_byte >= rb_size);

	for (nop_cnt = size - cmd_size; nop_cnt > 0; nop_cnt -= 4) {
		*(u32 *)(pipe->rb_vaddr + w_pos_byte) = 0x80000000;
		w_pos_byte = (w_pos_byte + 4) % rb_size;
	}

	BUG_ON(w_pos_byte & 0xf);

	return w_pos_byte;
}

static u32 *pipe_2d_emit_vm_switch(struct mwv207d_pipe_2d *pipe,
				   struct mwv207d_vm *vm, u32 *cmds)
{
	u64 fence_va = pipe->sync_va + SZ_4K - 16;
	u64 fill_va = pipe->sync_va + SZ_1K;

	cmds = pipe_2d_emit(cmds, 0x22c, vm->pta_id);

	cmds = pipe_2d_emit(cmds, 0x400, 0x00);
	cmds = pipe_2d_emit(cmds, 0x404, 0x00);
	cmds = pipe_2d_emit(cmds, 0x420, 0x00);
	cmds = pipe_2d_emit(cmds, 0x424, 0x00);
	cmds = pipe_2d_emit(cmds, 0x440, 0x00);
	cmds = pipe_2d_emit(cmds, 0x444, 0x00);
	cmds = pipe_2d_emit(cmds, 0x460, 0x00);
	cmds = pipe_2d_emit(cmds, 0x464, 0x00);
	cmds = pipe_2d_emit(cmds, 0x480, 0x00);
	cmds = pipe_2d_emit(cmds, 0x484, 0x00);

	if (pipe->base.type == 0x5)
		return cmds;

	cmds = pipe_2d_emit(cmds, 0x4, 1);

	cmds = pipe_2d_emit(cmds, 0x0c, 0x60);
	cmds = pipe_2d_emit(cmds, 0x14, 0x60);
	cmds = pipe_2d_emit(cmds, 0x54, 0x00);
	cmds = pipe_2d_emit(cmds, 0x74, (fill_va >> 8) & 0xffff);
	*cmds++ = 0x82000054;
	*cmds++ = 0x00000000;
	*cmds++ = ((SZ_2K / 256) << 16) | ((fill_va >> 24) & 0xffff);
	*cmds++ = 0x00000000;
	*cmds++ = (1 << 16) | 2;
	*cmds++ = 0x81000000;

	cmds = pipe_2d_emit(cmds, 0x74, (fence_va >> 8) & 0xffff);
	cmds = pipe_2d_emit(cmds, 0x0c, 0xe0);
	*cmds++ = 0x82000095;
	*cmds++ = (fence_va >> 4) & 0xf;
	*cmds++ = (fence_va >> 24) & 0xffff;
	*cmds++ = vm->identity & 0xffffffff;
	*cmds++ = vm->identity >> 32;
	*cmds++ = 100;
	*cmds++ = 0x81000000;

	return cmds;
}

static u32 pipe_2d_handle_vm(struct mwv207d_pipe_2d *pipe, struct mwv207d_vm *vm)
{
	u32 vm_cmds[64];
	u32 *cmds;

	if (vm->identity == pipe->vm_identity &&
			!test_and_clear_bit(0x0, &vm->dirty))
		return pipe->w_pos * 16;

	cmds = pipe_2d_emit(vm_cmds, 0x4, ffs(pipe->nr_clusters));
	cmds = pipe_2d_emit(cmds, 0x230, 0x80000000);
	if (pipe->vm_identity != vm->identity) {
		cmds = pipe_2d_emit_vm_switch(pipe, vm, cmds);
		pipe->vm_identity = vm->identity;
	}

	BUG_ON(cmds - 1 > &vm_cmds[63]);
	return pipe_2d_submit_cmd(pipe, vm_cmds, (cmds - &vm_cmds[0]) << 2,
				  pipe->w_pos * 16);
}

static void
pipe_2d_append_fence(struct mwv207d_pipe_2d *pipe, u32 *w_pos_byte,
		struct mwv207d_job *mjob)
{
	struct mwv207d_fence *fence = &mjob->hw_fence;
	unsigned long flags;
	u32 fence_cmds[16];
	u32 *cmds;
	u32 regno;

	regno = (fence->base.seqno & 0x1f) + 1;

	dma_fence_get(&fence->base);
	spin_lock_irqsave(&pipe->fence_queue_lock, flags);
	list_add_tail(&fence->q, &pipe->fence_queue);
	spin_unlock_irqrestore(&pipe->fence_queue_lock, flags);

	cmds = pipe_2d_emit(fence_cmds, 0x4, ffs(pipe->nr_clusters));
	*cmds++ = 0x81000000;

	cmds = pipe_2d_emit(cmds, 0x800, fence->base.seqno & 0xffff);
	cmds = pipe_2d_emit(cmds, 0x800 | (regno << 2), regno);
	*cmds++ = 0x81000000;

	BUG_ON(cmds - 1 > &fence_cmds[15]);

	*w_pos_byte = pipe_2d_submit_cmd(pipe, fence_cmds,
					(cmds - &fence_cmds[0]) << 2, *w_pos_byte);
	pipe_2d_write(pipe, 0x800 + regno * 4, regno);
}

static void mwv207d_pipe_2d_dump_reg(struct mwv207d_pipe_2d *pipe, u32 reg)
{
	u32 core_offset = 0x10000;
	int i;

	for (i = 0; i < pipe->nr_clusters; i++) {
		pr_info("2d core[%d] reg[0x%x]=0x%08x", i, reg,
				pipe_2d_read(pipe, core_offset + reg));
		core_offset += 0x1000;
	}
}

static void mwv207d_pipe_2d_dump_state(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_2d *pipe = to_pipe_2d(mpipe);

	pr_info("%s %08x: %08x", pipe->base.fname,
		0x100, pipe_2d_read(pipe, 0x100));
	mwv207d_pipe_2d_dump_reg(pipe, 0x048);

	pr_info("mmu fault status");
	pipe_2d_mmu_read(pipe, 0x384);
	pr_info("mmu fault addr low");
	pipe_2d_mmu_read(pipe, 0x380);
	pr_info("mmu fault addr high");
	pipe_2d_mmu_read(pipe, 0x3b8);
}

static struct dma_fence *
mwv207d_pipe_2d_submit(struct mwv207d_pipe *mpipe, struct mwv207d_job *mjob)
{
	struct mwv207d_pipe_2d *pipe = to_pipe_2d(mpipe);
	u32 size = ALIGN(mjob->cmd_size, 16) + 256 + (0x400 << 2);
	struct mwv207d_vm *vm;
	u32 w_pos_byte;
	int ret;

	BUG_ON(mjob->cmd_mode != 0x0);

	ret = pipe_2d_wait_for_space(pipe, size);
	if (ret) {
		pr_err("error, %s is stuck!", mpipe->fname);
		return ERR_PTR(ret);
	}

	vm = mjob->ctx ? mjob->ctx->vm : mpipe->mdev->vm;
	w_pos_byte = pipe_2d_handle_vm(pipe, vm);
	w_pos_byte = pipe_2d_submit_cmd(pipe, mjob->cmd_ptr,
			mjob->cmd_size, w_pos_byte);

	pipe_2d_append_fence(pipe, &w_pos_byte, mjob);

	pipe->w_pos = w_pos_byte / 16;
	mwv207d_pipe_record_busy(mpipe);
	dma_wmb();
	pipe_2d_write(pipe, 0x208, pipe->w_pos);

	return dma_fence_get(&mjob->hw_fence.base);
}

static irqreturn_t mwv207d_pipe_2d_isr(int irq_unused, void *data)
{
	struct mwv207d_pipe_2d *pipe = data;
	struct mwv207d_fence *fence, *p;
	u32 seqno;

	pipe_2d_write(pipe, 0x308, 1);
	seqno = pipe_2d_read(pipe, 0x800);

	spin_lock(&pipe->fence_queue_lock);
	list_for_each_entry_safe(fence, p, &pipe->fence_queue, q) {
		if (fence16_after(fence->base.seqno & 0xffff, seqno & 0xffff))
			break;
		list_del(&fence->q);
		mwv207d_pipe_record_idle(&pipe->base);
		dma_fence_signal(&fence->base);
		dma_fence_put(&fence->base);
	}
	spin_unlock(&pipe->fence_queue_lock);

	return IRQ_HANDLED;
}

static int pipe_2d_polling(void *priv)
{
	set_freezable();

	while (!kthread_should_stop()) {
		usleep_range(1000, 10000);
		try_to_freeze();

		mwv207d_pipe_2d_isr(-1, priv);
	}

	return 0;
}

static int mwv207d_pipe_2d_attach_vm(struct mwv207d_pipe *mpipe, struct mwv207d_vm *vm)
{
	struct mwv207d_pipe_2d *pipe = to_pipe_2d(mpipe);
	int ret;

	if (pipe->base.type == 0x5)
		return 0;
	ret = mwv207d_vm_map_linear(vm, pipe->sync_va, SZ_4K,
			vm->pgd.pt.phys_addr, 0x1);
	if (!ret)
		mwv207d_vm_reserve(vm, pipe->sync_va, SZ_4K);

	return ret;
}

static void mwv207d_pipe_2d_destroy(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_2d *pipe = to_pipe_2d(mpipe);

	if (!mpipe->mdev->isr_poll)
		free_irq(pipe->irq, pipe);
	else
		kthread_stop(pipe->poll_thread);

	pipe_2d_stop(pipe);

	if (pipe->rb_bo)
		mwv207d_bo_destroy_pin_mapped(pipe->rb_bo);
}

struct mwv207d_pipe *mwv207d_pipe_2d_create(struct mwv207d_device *mdev,
					  const struct mwv207d_pipe_info *info)
{
	struct mwv207d_pipe_2d *pipe;
	struct mwv207d_bo *mbo = NULL;
	bool is_fus = !!info->id;
	int ret;

	pipe = devm_kzalloc(mdev->dev, sizeof(*pipe), GFP_KERNEL);
	if (!pipe)
		return NULL;
	pipe->base.pll_id = is_fus ? MWV207D_PLL_CORE_FUS : MWV207D_PLL_CORE_2D;
	pipe->base.type = is_fus ? 0x5 : 0x3;
	pipe->base.id = 0;
	pipe->base.mdev = mdev;
	pipe->base.fname = info->name;
	pipe->base.submit = mwv207d_pipe_2d_submit;
	pipe->base.reset = mwv207d_pipe_2d_reset;
	pipe->base.destroy = mwv207d_pipe_2d_destroy;
	pipe->base.dump_state = mwv207d_pipe_2d_dump_state;
	pipe->base.attach_vm = mwv207d_pipe_2d_attach_vm;

	pipe->regbase = mdev->mmio + info->reg_base;
	pipe->nr_clusters = is_fus ?
		mdev->hw.nr_fus_clusters : mdev->hw.nr_2d_clusters;
	INIT_LIST_HEAD(&pipe->fence_queue);
	spin_lock_init(&pipe->fence_queue_lock);

	if (mdev->gart) {
		ret = mwv207d_bo_create_pin_mapped(mdev, 0x20000, SZ_4K,
				0x4, 0, &mbo,
				&pipe->rb_gpu_addr, &pipe->rb_vaddr);
		if (ret)
			return NULL;

		ret = mwv207d_vm_map_bo(mdev->vm, mbo, pipe->rb_gpu_addr,
				0x1);
		if (ret)
			goto free_bo;

		pipe->rb_bo = mbo;
	} else {
		pipe->rb_vaddr = dmam_alloc_coherent(mdev->dev,
				0x20000, &pipe->rb_gpu_addr, GFP_KERNEL);
		if (!pipe->rb_vaddr)
			return NULL;
		pipe->rb_gpu_addr += 0x1000000000ULL;
	}

	pipe->units = 0x20000 / 16;

	if (!mdev->isr_poll) {
		pipe->irq = mwv207d_irq_find(mdev, info->pf_irq, info->vf_irq);
		if (pipe->irq == 0) {
			pr_err("%s failed to find irq nr", info->name);
			goto free_bo;
		}

		ret = request_irq(pipe->irq, mwv207d_pipe_2d_isr, 0, info->name, pipe);
		if (ret) {
			pr_err("%s failed to request irq", info->name);
			goto free_bo;
		}
	} else {
		pipe->poll_thread = kthread_run(pipe_2d_polling,
						pipe, info->name);
		if (IS_ERR(pipe->poll_thread)) {
			pr_err("%s failed to create poll task", info->name);
			goto free_bo;
		}
	}

	mwv207d_pipe_2d_reset(&pipe->base);
	return &pipe->base;
free_bo:
	if (mbo)
		mwv207d_bo_destroy_pin_mapped(mbo);
	return NULL;
}
