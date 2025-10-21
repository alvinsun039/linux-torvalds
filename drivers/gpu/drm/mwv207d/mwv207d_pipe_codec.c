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

#include "mwv207d_sched.h"
#include "mwv207d_irq.h"
#include "mwv207d_vm.h"
#include "mwv207d_ctx.h"

#define to_codec_pipe(pipe) \
	container_of(pipe, struct mwv207d_pipe_codec, base)

struct mwv207d_pipe_codec {
	struct mwv207d_pipe base;
	void __iomem *regbase;

	struct mwv207d_bo *cmd_bo;
	void *cmd;
	u64 cmd_addr;

	bool is_enc;
	unsigned int irq;
	int id;
	int vm_identity;

	struct mwv207d_fence *fence;

	struct task_struct *poll_tsk;
	u32 init_reg_offset;
	bool mmu_inited;
};

static inline void pipe_codec_write(struct mwv207d_pipe_codec *pipe,
				    u32 reg, u32 value)
{
	writel_relaxed(value, pipe->regbase + reg);
}

static inline u32 pipe_codec_read(struct mwv207d_pipe_codec *pipe, u32 reg)
{
	return readl_relaxed(pipe->regbase + reg);
}

static inline void pipe_arb_write(struct mwv207d_pipe_codec *pipe,
				  u32 reg, u32 value)
{
	writel_relaxed(value, pipe->regbase + 0x38000 + reg);
}

static inline u32 pipe_arb_read(struct mwv207d_pipe_codec *pipe, u32 reg)
{
	return readl(pipe->regbase + 0x38000 + reg);
}

static irqreturn_t mwv207d_pipe_codec_isr(int irq_unused, void *data)
{
	struct mwv207d_pipe_codec *pipe = data;
	struct mwv207d_fence *fence;
	u32 stat;

	stat = pipe_codec_read(pipe, 0x44);

	if (!(stat & 0x1))
		return IRQ_NONE;

	fence = xchg(&pipe->fence, NULL);
	if (!fence) {
		pr_warn("warning, %s get unexpected irq, %#x",
			pipe->base.fname, stat);
		pipe_codec_write(pipe, 0x44, stat);
		return IRQ_HANDLED;
	}

	mwv207d_pipe_record_idle(&pipe->base);
	pipe_codec_write(pipe, 0x44, stat);

	dma_fence_signal(&fence->base);
	dma_fence_put(&fence->base);

	return IRQ_HANDLED;
}

static int pipe_codec_polling(void *priv)
{
	set_freezable();

	while (!kthread_should_stop()) {
		usleep_range(500, 1000);
		try_to_freeze();

		mwv207d_pipe_codec_isr(-1, priv);
	}

	return 0;
}

static int mwv207d_pipe_extern_reset(struct mwv207d_pipe_codec *pipe)
{
	int ret;
	u32 stat;

	stat = pipe_arb_read(pipe, 0x14);

	pipe_arb_write(pipe, 0x40, 0x01f);

	pipe_arb_write(pipe, 0x508c, 0x1);
	ret = wait_for(pipe_arb_read(pipe, 0x5000) >> 31, 10000);
	if (ret) {
		pr_err("error, %s flush axife failed, 0x%08x",
			pipe->base.fname, pipe_arb_read(pipe, 0x5000));
		return ret;
	}

	pipe_arb_write(pipe, 0x10, 0x1e00ef);
	pipe_arb_write(pipe, 0x10, 0x1e04a6);

	pipe_arb_write(pipe, 0x1c, 0x40000000);
	pipe_arb_write(pipe, 0x18, 0x3);

	pipe_arb_write(pipe, 0x40, 0x91f);
	pipe_arb_write(pipe, 0x40, 0x11f);

	pipe_arb_write(pipe, 0x10, 0xef);

	pipe_arb_write(pipe, 0x14, stat);

	pipe_arb_write(pipe, 0x10, 0x1);

	return 0;
}

static void mwv207d_pipe_emit_init_cmd(struct mwv207d_pipe_codec *pipe,
				       u32 reg, u32 val)
{
	pipe_codec_write(pipe, pipe->init_reg_offset, 0x0c010000 + reg);
	pipe_codec_write(pipe, pipe->init_reg_offset + 4, val);
	pipe->init_reg_offset += 8;
}
static void mwv207d_pipe_mmu_write(void *priv, u32 reg, u32 val)
{
	struct mwv207d_pipe_codec *pipe = priv;

	mwv207d_pipe_emit_init_cmd(pipe, 0x4000 + reg, val);
}

static void mwv207d_pipe_set_init_mode(struct mwv207d_pipe_codec *pipe, struct mwv207d_vm *vm)
{
	pipe->init_reg_offset = 0x80;
	mwv207d_pipe_emit_init_cmd(pipe, 0x5028, 0x2);
	mwv207d_pipe_emit_init_cmd(pipe, 0x502c, 0x2);

	if (!pipe->mmu_inited) {
		mwv207d_mmu_init(pipe->base.mdev, vm,
				mwv207d_pipe_mmu_write, pipe);
		pipe->mmu_inited = true;
	} else if (pipe->vm_identity != vm->identity ||
			test_and_clear_bit(0x3 + pipe->id,
				&vm->dirty)) {

		mwv207d_mmu_flush(pipe->base.mdev, vm,
				mwv207d_pipe_mmu_write, pipe);
	}
	pipe->vm_identity = vm->identity;

	pipe_codec_write(pipe, pipe->init_reg_offset, 0x10000000);

	BUG_ON(pipe->init_reg_offset > 0x80 + 4 * 32);
}

static void mwv207d_pipe_codec_reset(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_codec *pipe = to_codec_pipe(mpipe);
	struct mwv207d_fence *fence;
	u32 status;

	if (mpipe->mdev->hw.is_pf)
		mwv207d_pipe_extern_reset(pipe);

	pipe_codec_write(pipe, 0x48, 0x0);

	pipe_codec_write(pipe, 0x40, 0x4);

	status = pipe_codec_read(pipe, 0x44);

	pipe_codec_write(pipe, 0x44, status);

	pipe_codec_write(pipe, 0xC, 0x0);

	pipe_codec_write(pipe, 0x48, 0x1);

	pipe->mmu_inited = false;

	fence = xchg(&pipe->fence, NULL);
	if (fence) {
		dma_fence_put(&fence->base);
		mwv207d_pipe_record_idle(mpipe);
	}
}

static const u32 codec_interrupt_clear_cmds[2][2] = {
	[0] = { 0xd2001004, 0x03ffff00 },
	[1] = { 0xd0001004, 0xffffffff }
};

static struct dma_fence *mwv207d_pipe_codec_submit(struct mwv207d_pipe *mpipe,
						   struct mwv207d_job *mjob)
{
	struct mwv207d_pipe_codec *pipe = to_codec_pipe(mpipe);
	struct mwv207d_fence *fence;
	struct mwv207d_vm *vm;
	u32 *cmd = pipe->cmd + 0x20 * 4;

	BUG_ON(mjob->cmd_mode != 0x0);

	if (mjob->cmd_size % 8 || mjob->cmd_size >
			0x10000 - 32 - 0x20 * 4) {
		WARN_ON(1);
		return ERR_PTR(-EINVAL);
	}

	fence = pipe->fence;
	if (fence)
		return ERR_PTR(-EBUSY);

	fence = &mjob->hw_fence;
	dma_fence_get(&fence->base);

	memcpy(cmd, mjob->cmd_ptr, mjob->cmd_size);
	cmd += mjob->cmd_size / 4;

	*cmd++ = codec_interrupt_clear_cmds[pipe->is_enc][0];
	*cmd++ = codec_interrupt_clear_cmds[pipe->is_enc][1];

	*cmd++ = 0x10000000;
	*cmd++ = 0x00000000;

	pipe_codec_write(pipe, 0x40, 0xa0);
	pipe_codec_write(pipe, 0x44, 0x0);
	pipe_codec_write(pipe, 0x50,
			 (u32)pipe->cmd_addr);
	pipe_codec_write(pipe, 0x54,
			 (u32)(pipe->cmd_addr >> 32));
	pipe_codec_write(pipe, 0x58,
			 mjob->cmd_size / 8 + 2 + 0x20 / 2);
	pipe_codec_write(pipe, 0x60, 0x1);
	pipe_codec_write(pipe, 0x64, 0xffffffff);

	vm = mjob->ctx ? mjob->ctx->vm : mpipe->mdev->vm;
	mwv207d_pipe_set_init_mode(pipe, vm);

	pipe_codec_write(pipe, 0x70, 0x100);
	pipe_codec_write(pipe, 0x48, 0x1);

	pipe->fence = fence;
	mwv207d_pipe_record_busy(mpipe);

	dma_wmb();

	pipe_codec_write(pipe, 0x40, 0xa1);

	return dma_fence_get(&fence->base);
}

static void mwv207d_pipe_codec_dump_state(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_codec *pipe = to_codec_pipe(mpipe);
	int i;

	pr_info("cmd buf gpu addr: 0x%llx", pipe->cmd_addr);

	for (i = 1; i <= 28; i += 4) {
		pr_info("0x%08x : %08x %08x %08x %08x", i,
			pipe_codec_read(pipe, i * 4),
			pipe_codec_read(pipe, (i + 1) * 4),
			pipe_codec_read(pipe, (i + 2) * 4),
			pipe_codec_read(pipe, (i + 3) * 4));
	}

	pr_info("0x%08x : %08x %08x", i,
		pipe_codec_read(pipe, i * 4),
		pipe_codec_read(pipe, (i + 1) * 4));
}

static int mwv207d_pipe_codec_attach_vm(struct mwv207d_pipe *mpipe, struct mwv207d_vm *vm)
{
	struct mwv207d_pipe_codec *pipe = to_codec_pipe(mpipe);
	int ret;

	ret = mwv207d_vm_map_bo(vm, pipe->cmd_bo,
				pipe->cmd_addr, 0x1);
	if (!ret)
		mwv207d_vm_reserve(vm, pipe->cmd_addr, mwv207d_bo_size(pipe->cmd_bo));

	return ret;
}

static void mwv207d_pipe_codec_destroy(struct mwv207d_pipe *mpipe)
{
	struct mwv207d_pipe_codec *pipe = to_codec_pipe(mpipe);

	if (!mpipe->mdev->isr_poll)
		free_irq(pipe->irq, pipe);
	else
		kthread_stop(pipe->poll_tsk);

	mwv207d_bo_destroy_pin_mapped(pipe->cmd_bo);
}

static inline int pipe_codec_vf_irq(struct mwv207d_pipe_codec *pipe, u32 vf_irq)
{
	struct mwv207d_hwinfo *hw = &pipe->base.mdev->hw;

	switch (hw->vf_sel) {
	case 0 ... 3:
	case 8 ... 9:
	case 0xc:

	case 0xf:
		return vf_irq;
	case 4 ... 7:
	case 0xa ... 0xb:
	case 0xd:
		return vf_irq + 1;
	default:
		BUG_ON(1);
		return 0;
	}
}

struct mwv207d_pipe *mwv207d_pipe_codec_create(struct mwv207d_device *mdev,
					const struct mwv207d_pipe_info *info)
{
	struct mwv207d_pipe_codec *pipe;
	struct mwv207d_bo *mbo;
	bool is_enc = info->id < 2;
	bool isr_poll;
	u32 *cmd;
	int ret, i;

	pipe = devm_kzalloc(mdev->dev, sizeof(*pipe), GFP_KERNEL);
	if (!pipe)
		return NULL;

	pipe->base.mdev = mdev;
	pipe->base.type = is_enc ? 0x2 : 0x1;
	pipe->base.id = is_enc ? info->id : (info->id - 2);
	pipe->base.fname = info->name;
	pipe->base.submit = mwv207d_pipe_codec_submit;
	pipe->base.reset = mwv207d_pipe_codec_reset;
	pipe->base.destroy = mwv207d_pipe_codec_destroy;
	pipe->base.dump_state = mwv207d_pipe_codec_dump_state;
	pipe->base.attach_vm = mwv207d_pipe_codec_attach_vm;
	pipe->regbase = mdev->mmio + info->reg_base;
	pipe->is_enc = is_enc;
	pipe->id = info->id;

	ret = mwv207d_bo_create_pin_mapped(mdev, 0x10000, SZ_4K,
					  0x4, 0,
					  &mbo, &pipe->cmd_addr, &pipe->cmd);
	if (ret)
		return NULL;

	cmd = pipe->cmd;
	for (i = 0; i < 0x20 / 2; i++) {
		*cmd++ = 0x18000000;
		*cmd++ = 0x0;
	}

	ret = mwv207d_vm_map_bo(mdev->vm, mbo,
			pipe->cmd_addr, 0x1);
	if (ret)
		goto free_bo;

	pipe->cmd_bo = mbo;

	isr_poll = mdev->isr_poll || (!mdev->hw.is_pf &&
				     (mdev->hw.vf_sel == 0xf) &&
				     (info->id & 0x1));
	if (!isr_poll) {
		pipe->irq = mwv207d_irq_find(mdev, info->pf_irq,
				pipe_codec_vf_irq(pipe, info->vf_irq));
		if (!pipe->irq) {
			pr_err("%s failed to find irq nr", info->name);
			goto free_bo;
		}

		ret = request_irq(pipe->irq, mwv207d_pipe_codec_isr, 0,
				  info->name, pipe);
		if (ret) {
			pr_err("%s failed to request irq", info->name);
			goto free_bo;
		}
	} else {
		pipe->poll_tsk = kthread_run(pipe_codec_polling, pipe, info->name);
		if (IS_ERR(pipe->poll_tsk))
			goto free_bo;
	}

	mwv207d_bo_unreserve(mbo);

	mwv207d_pipe_codec_reset(&pipe->base);
	return &pipe->base;
free_bo:
	mwv207d_bo_destroy_pin_mapped(mbo);
	return NULL;
}
