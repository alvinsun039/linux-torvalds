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
#include <drm/drm_crtc.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_plane.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_framebuffer.h>
#include <drm/display/drm_hdmi_helper.h>
#include <linux/dma-resv.h>

#include "mwv207d_vm.h"
#include "mwv207d_bo.h"
#include "mwv207d_va.h"
#include "mwv207d_vi.h"
#include "mwv207d_drm.h"
#include "mwv207d_irq.h"
#include "mwv207d_sched.h"
#include "mwv207d_vbios.h"

#define crtc_to_va(_crtc) container_of(_crtc, struct mwv207d_va, crtc)
#define plane_to_va(plane) (((plane)->type == DRM_PLANE_TYPE_PRIMARY) \
		? container_of(plane, struct mwv207d_va, primary)    \
		: container_of(plane, struct mwv207d_va, cursor))

struct mwv207d_palette {
	struct mwv207d_bo *bo;
	void *addr;
	u64 phys;
};

struct mwv207d_blt_args {
	struct mwv207d_bo *src_bo;
	struct mwv207d_bo *dst_bo;
	u64 dst_ts_offset;
	u32 src_stride;
	u32 dst_stride;
	u32 src_w;
	u32 src_h;
	u32 src_x;
	u32 src_y;
	u32 compressed;
};

struct mwv207d_workaround_bo {
	struct delayed_work cleanup_work;
	struct kref refcount;
	struct mwv207d_bo *shadow_bo;
};

struct mwv207d_va {
	char name[32];
	struct drm_crtc   crtc;
	struct drm_plane  primary;
	struct drm_plane  cursor;
	void __iomem   *mmio;
	struct mwv207d_device *mdev;
	struct drm_pending_vblank_event *event;
	int idx;
	uint16_t lutdata[768];

	struct mwv207d_workaround_bo *wbo[0x3];
	struct mwv207d_workaround_bo *cur_workaround_bo;
	struct mwv207d_blt_args args;
	struct delayed_work page_flush_work;
	u64 vblank_period;
	u64 last_fence_seq;
	u64 last_fence_context;
	int blt_idx;

	struct mwv207d_palette palette;
	int va_irq;

	struct hrtimer timer;
	ktime_t period_ns;
};

static u32 rgb_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XRGB2101010,
};

static uint64_t format_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID
};

static u32 cursor_formats[] = {
	DRM_FORMAT_ARGB8888,
};

static struct mwv207d_workaround_bo *mwv207d_workaround_bo_get(struct mwv207d_workaround_bo *wbo)
{
	kref_get(&wbo->refcount);
	return wbo;
}

static void mwv207d_workaround_bo_release(struct kref *kref)
{
	struct mwv207d_workaround_bo *wbo = container_of(kref,
				struct mwv207d_workaround_bo, refcount);

	mwv207d_bo_unref(&wbo->shadow_bo);
	kfree(wbo);
}

static void mwv207d_workaround_bo_put(struct mwv207d_workaround_bo *wbo)
{
	kref_put(&wbo->refcount, mwv207d_workaround_bo_release);
}

static void mwv207d_va_cleanup_work_func(struct work_struct *work)
{
	struct mwv207d_workaround_bo *wbo =
		container_of(work, struct mwv207d_workaround_bo, cleanup_work.work);

	if (mwv207d_bo_reserve(wbo->shadow_bo, false)) {
		pr_warn("cleanup work failed to reserve bo");
		mwv207d_workaround_bo_put(wbo);
		return;
	}
	mwv207d_bo_unpin(wbo->shadow_bo);
	mwv207d_bo_unreserve(wbo->shadow_bo);
	mwv207d_workaround_bo_put(wbo);
}

static void mwv207d_va_cleanup_workaround_bo(struct mwv207d_workaround_bo *wbo)
{
	queue_delayed_work(system_wq, &wbo->cleanup_work, HZ);
}

static int mwv207d_workaround_bo_create(struct mwv207d_va *va,
					struct mwv207d_workaround_bo **wbo_ptr,
					int blt_size)
{
	struct mwv207d_workaround_bo *wbo;
	int ret;

	wbo = kzalloc(sizeof(*wbo), GFP_KERNEL);
	if (!wbo)
		return -ENOMEM;

	INIT_DELAYED_WORK(&wbo->cleanup_work, mwv207d_va_cleanup_work_func);
	kref_init(&wbo->refcount);
	ret = mwv207d_bo_create(va->mdev, blt_size, PAGE_SIZE, true,
					0x2,
					0x2,
					NULL, NULL, &wbo->shadow_bo);
	if (ret) {
		kfree(wbo);
		return ret;
	}

	*wbo_ptr = wbo;
	return 0;
}

static enum hrtimer_restart mwv207d_va_vblank_sw_simulate(struct hrtimer *timer)
{
	struct mwv207d_va *va = container_of(timer, struct mwv207d_va, timer);
	struct drm_crtc *crtc = &va->crtc;
	u64 ret_overrun;
	bool ret;

	ret_overrun = hrtimer_forward_now(&va->timer, va->period_ns);

	ret = drm_crtc_handle_vblank(crtc);
	if (!ret)
		return HRTIMER_NORESTART;

	return HRTIMER_RESTART;
}

static int mwv207d_va_enable_sw_vblank(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);
	unsigned int pipe = drm_crtc_index(crtc);
	struct drm_vblank_crtc *vblank = &crtc->dev->vblank[pipe];

	if (!crtc->state->active)
		return 0;

	drm_calc_timestamping_constants(crtc, &crtc->mode);

	va->period_ns = ktime_set(0, vblank->framedur_ns);
	hrtimer_start(&va->timer, va->period_ns, HRTIMER_MODE_REL);

	return 0;
}

static void mwv207d_va_disable_sw_vblank(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);

	hrtimer_cancel(&va->timer);
}

static inline u32 mwv207d_va_read(struct mwv207d_va *va, u32 reg)
{
	return readl_relaxed(va->mmio + reg);
}

static inline void mwv207d_va_write(struct mwv207d_va *va, u32 reg, u32 value)
{
	writel_relaxed(value, va->mmio + reg);
}

static inline void mwv207d_va_modify(struct mwv207d_va *va, u32 reg,
				     u32 mask, u32 value)
{
	u32 rvalue = mwv207d_va_read(va, reg);

	rvalue = (rvalue & ~mask) | (value & mask);
	mwv207d_va_write(va, reg, rvalue);
}

static bool mwv207d_plane_format_mod_supported(struct drm_plane *plane,
					       uint32_t format,
					       uint64_t modifier)
{
	if (modifier == DRM_FORMAT_MOD_LINEAR)
		return true;
	return false;
}

static int mwv207d_plane_atomic_check(struct drm_plane *plane,
				      struct drm_atomic_state *state)
{
	struct mwv207d_va *va = plane_to_va(plane);
	struct drm_plane_state *plane_state;
	struct drm_crtc_state *crtc_state;
	struct drm_framebuffer *fb;
	struct mwv207d_bo *mbo;
	u32 width_rd, xinc, src_w, tile_mode;
	u64 tile_flags;

	plane_state = drm_atomic_get_new_plane_state(state, plane);
	crtc_state = drm_atomic_get_new_crtc_state(state, &va->crtc);
	fb = plane_state->fb;
	if (!fb)
		return 0;

	mbo = gbo_to_mbo(fb->obj[0]);
	mwv207d_bo_get_tiling_flags(mbo, &tile_flags);
	tile_mode = MWV207D_TILING_GET(tile_flags, MODE);

	if (tile_mode == MWV207D_TILING_MODE_SUPERX)
		xinc = 8;
	else
		xinc = fb->format->cpp[0] == 4 ? 64 : 128;

	src_w = plane_state->src_w >> 16;
	width_rd  = src_w + xinc - 1;
	width_rd -= width_rd % xinc;

	if (width_rd > fb->pitches[0])
		return -EINVAL;

	return drm_atomic_helper_check_plane_state(plane_state, crtc_state,
						   DRM_PLANE_NO_SCALING,
						   DRM_PLANE_NO_SCALING,
						   true, true);
}

static int mwv207d_plane_prepare_fb(struct drm_plane *plane,
				struct drm_plane_state *new_state)
{
	struct mwv207d_bo *mbo;
	u64 tile_flags;
	u32 tile_mode;
	int ret;

	if (!new_state->fb || !new_state->fb->obj[0])
		return 0;

	mbo = gbo_to_mbo(new_state->fb->obj[0]);
	mwv207d_bo_get_tiling_flags(mbo, &tile_flags);
	tile_mode = MWV207D_TILING_GET(tile_flags, MODE);

	if (tile_mode != MWV207D_TILING_MODE_LINEAR) {
		mbo->flags &= ~0x1;
		mbo->flags |= 0x2;
	}

	ret = mwv207d_bo_reserve(mbo, false);
	if (ret)
		return ret;
	ret = mwv207d_bo_pin(mbo, 0x2, NULL);
	mwv207d_bo_unreserve(mbo);
	if (ret)
		return ret;

	drm_gem_plane_helper_prepare_fb(plane, new_state);

	return 0;
}

static void mwv207d_plane_cleanup_fb(struct drm_plane *plane,
				struct drm_plane_state *old_state)
{
	struct mwv207d_bo *mbo;

	if (!old_state->fb || !old_state->fb->obj[0])
		return;

	mbo = gbo_to_mbo(old_state->fb->obj[0]);

	if (mwv207d_bo_reserve(mbo, false)) {
		pr_warn("failed to reserve bo");
		return;
	}
	mwv207d_bo_unpin(mbo);
	mwv207d_bo_unreserve(mbo);
}

static u64 mwv207d_tile_offset(u32 x, u32 y, struct drm_framebuffer *fb)
{
	u64 offset;

	offset  = (y >> 5) & 0x1;
	offset  = (offset << 1) | ((x >> 5) & 0x1);
	offset  = (offset << 1) | ((y >> 4) & 0x1);
	offset  = (offset << 1) | ((x >> 4) & 0x1);
	offset  = (offset << 1) | ((y >> 3) & 0x1);
	offset  = (offset << 1) | ((x >> 3) & 0x1);
	offset  = (offset << 1) | ((y >> 2) & 0x1);
	offset  = (offset << 1) | ((x >> 2) & 0x1);
	offset  = (offset << 2) | (y & 0x3);
	offset  = (offset << 2) | (x & 0x3);

	offset = (y & (~0x3f)) * fb->pitches[0]  +
		 ((x & (~0x3f)) * 64 + offset) * fb->format->cpp[0];

	return offset;
}

static u64 mwv207d_plane_fb_addr(struct drm_plane *plane, u32 tile_mode)
{
	struct drm_framebuffer *fb;
	struct mwv207d_bo *mbo;
	u32 src_x, src_y;
	u64 fbaddr;

	src_x = plane->state->src_x >> 16;
	src_y = plane->state->src_y >> 16;

	fb = plane->state->fb;
	mbo = gbo_to_mbo(fb->obj[0]);
	fbaddr = mwv207d_bo_gpu_offset(mbo);

	if (tile_mode == MWV207D_TILING_MODE_LINEAR)
		fbaddr += src_x * fb->format->cpp[0] + fb->pitches[0] * src_y;
	else
		fbaddr += mwv207d_tile_offset(src_x, src_y, fb);

	return fbaddr;
}

static struct mwv207d_job *mwv207d_blt_job_alloc(struct mwv207d_bo *src_bo,
						struct mwv207d_bo *dst_bo)
{
	struct mwv207d_job *mjob;
	struct mwv207d_bo *mbo[2];
	struct ttm_validate_buffer *tvb;
	int i;

	mjob = mwv207d_job_alloc();
	if (!mjob)
		return ERR_PTR(-ENOMEM);

	mjob->mtvb = kvcalloc(2, sizeof(struct mwv207d_tvb), GFP_KERNEL);
	if (!mjob->mtvb) {
		mwv207d_job_put(mjob);
		return ERR_PTR(-ENOMEM);
	}

	mbo[0] = src_bo;
	mbo[1] = dst_bo;

	for (i = 0; i < 2; i++) {
		tvb = &mjob->mtvb[i].base;
		tvb->num_shared = i == 0 ? 1 : 0;
		tvb->bo = &mwv207d_bo_ref(mbo[i])->tbo;
		list_add_tail(&tvb->head, &mjob->tvblist);
	}

	return mjob;
}

static u32 *mwv207d_cmd_emit(u32 *cmd, u32 reg, u32 val)
{
	*cmd++ = 0x40000000 | reg;
	*cmd++ = val;
	return cmd;
}

static u32 *mwv207d_copy_cmd_init(u32 *cmd, u64 src_base, u64 dst_base,
				u32 src_stride, u32 dst_stride, u64 src_tsaddr,
				u64 dst_tsaddr, u32 src_x, u32 src_y,
				u32 dst_x, u32 dst_y, u32 w, u32 h,
				u32 cluster_mask, u32 src_compressed,
				u32 dst_compressed)
{
	cmd = mwv207d_cmd_emit(cmd, 0x4, cluster_mask);
	cmd = mwv207d_cmd_emit(cmd, 0x8, 0x0);
	cmd = mwv207d_cmd_emit(cmd, 0xc, (1 << 7) | (1 << 5));
	cmd = mwv207d_cmd_emit(cmd, 0x30, (1 << 7) | (1 << 5));
	cmd = mwv207d_cmd_emit(cmd, 0x34, 0x0);
	cmd = mwv207d_cmd_emit(cmd, 0x14, (1 << 7) | (1 << 5));
	cmd = mwv207d_cmd_emit(cmd, 0x10,
			((src_base >> 24) & 0xffff) | ((src_stride / 256) << 16));
	cmd = mwv207d_cmd_emit(cmd, 0x18, (src_y << 16) | src_x);
	cmd = mwv207d_cmd_emit(cmd, 0x400,
			(dst_compressed << 31) | ((dst_tsaddr >> 32) & 0xff));
	cmd = mwv207d_cmd_emit(cmd, 0x404, dst_tsaddr & 0xffffffff);
	cmd = mwv207d_cmd_emit(cmd, 0x408, 0x6);
	cmd = mwv207d_cmd_emit(cmd, 0x41c, 0x1);
	cmd = mwv207d_cmd_emit(cmd, 0x420,
			(src_compressed << 31) | ((src_tsaddr >> 32) & 0xff));
	cmd = mwv207d_cmd_emit(cmd, 0x424, src_tsaddr & 0xffffffff);
	cmd = mwv207d_cmd_emit(cmd, 0x480,
			(src_compressed << 31) | ((src_tsaddr >> 32) & 0xff));
	cmd = mwv207d_cmd_emit(cmd, 0x484, src_tsaddr & 0xffffffff);
	cmd = mwv207d_cmd_emit(cmd, 0x428, 0x2);
	cmd = mwv207d_cmd_emit(cmd, 0x488, 0x2);
	cmd = mwv207d_cmd_emit(cmd, 0x70, (src_base >> 8) & 0xffff);
	cmd = mwv207d_cmd_emit(cmd, 0x78, (src_base >> 8) & 0xffff);
	cmd = mwv207d_cmd_emit(cmd, 0x74, (dst_base >> 8) & 0xffff);
	cmd = mwv207d_cmd_emit(cmd, 0x54, 0xffffffff);

	*cmd++ = 0x8200001c;
	*cmd++ = (0x2 << 16) | 0xcc;
	*cmd++ =  ((dst_base >> 24) & 0xffff) | ((dst_stride / 256) << 16);
	*cmd++ = (dst_y << 16) | dst_x;
	*cmd++ = (h << 16) | w;
	*cmd++ = ((src_base >> 24) & 0xffff) | (((src_stride) / 256) << 16);
	*cmd++ = (src_y << 16) | src_x;
	*cmd++ = (h << 16) | w;
	*cmd++ = 0x00000000;
	*cmd++ = 0x00000000;
	*cmd++ = 0x00000000;
	*cmd++ = 0x00000000;
	*cmd++ = 0x00000000;
	*cmd++ = 0x81000000;

	return cmd;
}

static int mwv207d_va_pin_map_bo(struct mwv207d_va *va, struct mwv207d_bo *mbo)
{
	int ret;

	ret = mwv207d_bo_reserve(mbo, false);
	if (ret)
		return ret;
	ret = mwv207d_bo_pin(mbo, 0x2, NULL);
	mwv207d_bo_unreserve(mbo);
	if (ret)
		return ret;
	ret = mwv207d_vm_map_bo(va->mdev->vm, mbo, mwv207d_bo_gpu_offset(mbo), 0);
	if (ret)
		goto unpin_bo;

	return 0;

unpin_bo:
	if (!mwv207d_bo_reserve(mbo, false)) {
		mwv207d_bo_unpin(mbo);
		mwv207d_bo_unreserve(mbo);
	}
	return ret;
}

static int mwv207d_blt_job_init(struct mwv207d_va *va,
				struct mwv207d_job *mjob)
{
	struct mwv207d_device *mdev = va->mdev;
	struct mwv207d_blt_args *args = &va->args;
	struct mwv207d_bo *src_bo, *dst_bo;
	u64 src_base, dst_base, tile_flags;
	u64 src_ts_offset, dst_ts_offset, src_tsaddr = 0, dst_tsaddr = 0;
	u32 src_x, src_y, width, height, src_stride, dst_stride;
	u32 src_w, src_h, compressed;
	u32 *start, *cmd;

	src_bo = args->src_bo;
	dst_bo = args->dst_bo;

	mwv207d_bo_get_tiling_flags(src_bo, &tile_flags);
	compressed = MWV207D_TILING_GET(tile_flags, COMPRESS);
	src_ts_offset = MWV207D_TILING_GET(tile_flags, OFFSET_256B) << 8;
	dst_ts_offset = args->dst_ts_offset;

	src_base = mwv207d_bo_gpu_offset(src_bo);
	dst_base = mwv207d_bo_gpu_offset(dst_bo);

	src_stride = args->src_stride;
	dst_stride = args->dst_stride;

	if (compressed) {
		args->compressed = compressed;
		src_tsaddr = src_base + src_ts_offset;
		dst_tsaddr = dst_base + dst_ts_offset;
	}

	src_x = args->src_x;
	src_y = args->src_y;
	width = args->src_w;
	height = args->src_h;
	src_w = ALIGN_DOWN(width, 256);
	src_h = height;

	start = kvmalloc(0x10000, GFP_KERNEL);
	if (!start)
		return -ENOMEM;

	cmd = start;
	cmd = mwv207d_copy_cmd_init(cmd, src_base, dst_base,
			src_stride, dst_stride,
			src_tsaddr, dst_tsaddr,
			src_x, src_y, 0, 0, src_w, src_h,
			ffs(mdev->hw.nr_2d_clusters), compressed, compressed);

	if (width & 0xff) {
		cmd = mwv207d_copy_cmd_init(cmd, src_base, dst_base,
				src_stride, dst_stride,
				src_tsaddr, dst_tsaddr,
				src_x + src_w, src_y, src_w, 0, width & 0xff, src_h,
				ffs(mdev->hw.nr_2d_clusters), compressed, compressed);
	}

	mjob->cmd_mode = 0x0;
	mjob->cmd_ptr = (char *)start;
	mjob->cmd_size = (unsigned long)cmd - (unsigned long)start;
	mjob->engine_entity = mdev->blt_2d_entity;

	return 0;
}

static int mwv207d_blt_job_attach_dependency(struct mwv207d_job *mjob)
{
	struct mwv207d_tvb *mtvb;
	int ret;

	mwv207d_for_each_mtvb(mtvb, mjob) {
		ret = mwv207d_job_add_resv_fence(mjob,
				mtvb->base.bo->base.resv,
				mtvb->base.num_shared == 0);
		if (ret)
			return ret;
	}

	return 0;
}

static int mwv207d_blt_job_submit(struct mwv207d_va *va,
				struct mwv207d_job *mjob,
				struct dma_fence **fence)
{
	int ret;

	mutex_lock(&va->mdev->qlock);

	ret = drm_sched_job_init(&mjob->base, mjob->engine_entity, NULL);
	if (ret)
		goto out;
	drm_sched_job_arm(&mjob->base);
	*fence = &mjob->base.s_fence->finished;

	mwv207d_job_get(mjob);
	drm_sched_entity_push_job(&mjob->base);
out:
	mutex_unlock(&va->mdev->qlock);
	return ret;
}

static int mwv207d_va_blt_to_shadow(struct mwv207d_va *va)
{
	struct mwv207d_job *mjob;
	struct ww_acquire_ctx ticket;
	struct dma_fence *fence = NULL;
	int ret;

	mjob = mwv207d_blt_job_alloc(va->args.src_bo, va->args.dst_bo);
	if (IS_ERR(mjob))
		return PTR_ERR(mjob);

	ret = mwv207d_blt_job_init(va, mjob);
	if (ret)
		goto free_job;

	ret = ttm_eu_reserve_buffers(&ticket, &mjob->tvblist, true, NULL);
	if (ret)
		goto free_job;

	ret = mwv207d_blt_job_attach_dependency(mjob);
	if (ret)
		goto unreserve;

	ret = mwv207d_blt_job_submit(va, mjob, &fence);

unreserve:
	if (likely(fence))
		ttm_eu_fence_buffer_objects(&ticket, &mjob->tvblist, fence);
	else
		ttm_eu_backoff_reservation(&ticket, &mjob->tvblist);

	ret = dma_fence_wait(fence, false);

free_job:
	mwv207d_job_put(mjob);
	return ret;
}

static bool mwv207d_va_workaround_need_blt(struct mwv207d_va *va)
{
	struct mwv207d_blt_args *args = &va->args;
	struct dma_fence *fence = NULL;
	struct dma_resv_iter cursor;
	bool need_blt = true;

	dma_resv_iter_begin(&cursor, args->src_bo->tbo.base.resv, DMA_RESV_USAGE_WRITE);

	dma_resv_for_each_fence_unlocked(&cursor, fence) {
		if (va->last_fence_context != fence->context) {
			va->last_fence_context = fence->context;
			va->last_fence_seq = fence->seqno;
			goto out;
		}
		if (va->last_fence_seq != fence->seqno) {
			va->last_fence_seq = fence->seqno;
			goto out;
		}
		need_blt = false;
	}
	if(need_blt) {
		va->last_fence_context = 0;
		va->last_fence_seq = 0;
	}
out:
	dma_resv_iter_end(&cursor);
	return true;
}

static int mwv207d_plane_ensure_shadow_bo(struct mwv207d_va *va, int blt_size)
{
	struct mwv207d_workaround_bo *wbo;
	int ret, i;

	for (i = 0; i < 0x3; i++) {
		if (va->wbo[i] &&
				mwv207d_bo_size(va->wbo[i]->shadow_bo) >= blt_size)
			continue;

		ret = mwv207d_workaround_bo_create(va, &wbo, blt_size);
		if (ret)
			return ret;

		if (va->wbo[i])
			mwv207d_workaround_bo_put(va->wbo[i]);

		va->wbo[i] = wbo;
	}

	return 0;
}

static int mwv207d_plane_prepare_workaround(struct mwv207d_va *va,
					    struct drm_plane_state *plane_state)
{
	struct mwv207d_blt_args *args = &va->args;
	struct mwv207d_bo *mbo;
	u32 src_w, src_h, blt_size, img_size, dst_stride;
	int ret;

	src_w = ALIGN(plane_state->src_w >> 16, 64);
	src_h = ALIGN(plane_state->src_h >> 16, 64);
	dst_stride = ALIGN(src_w * 4, 256);
	img_size = ALIGN(dst_stride * src_h, PAGE_SIZE);
	blt_size = img_size + ALIGN(img_size >> 9, 64) + 128;
	blt_size = ALIGN(blt_size, PAGE_SIZE);

	ret = mwv207d_plane_ensure_shadow_bo(va, blt_size);
	if (ret)
		return ret;

	mbo = gbo_to_mbo(plane_state->fb->obj[0]);
	ret = mwv207d_vm_map_bo(va->mdev->vm, mbo, mwv207d_bo_gpu_offset(mbo), 0);
	if (ret)
		return ret;
	args->src_bo = mbo;

	args->src_stride = plane_state->fb->pitches[0];
	args->dst_stride = dst_stride;
	args->src_w = plane_state->src_w >> 16;
	args->src_h = plane_state->src_h >> 16;
	args->src_x = plane_state->src_x >> 16;
	args->src_y = plane_state->src_y >> 16;
	args->dst_ts_offset = img_size;

	return 0;
}

static bool mwv207d_plane_need_workaround(struct drm_plane_state *state,
					u32 tile_mode)
{
	u32 src_x, src_y;

	src_x = state->src_x >> 16;
	src_y = state->src_y >> 16;

	return tile_mode == MWV207D_TILING_MODE_SUPERX &&
		((src_x & 0x38) || (src_y & 0x38));
}

static void mwv207d_hw_set_format(struct mwv207d_va *va, u32 format, u32 tile_mode)
{
	switch (format) {
	case DRM_FORMAT_RGB565:
		mwv207d_va_modify(va, 0x80, 0x1, 1);
		break;
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_ARGB2101010:
		mwv207d_va_modify(va, 0x80, 0x1, 0);
		mwv207d_va_modify(va, 0x80, 0x1 << 4, 1 << 4);
		break;
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
		mwv207d_va_modify(va, 0x80, 0x1, 0);
		mwv207d_va_modify(va, 0x80, 0x1 << 4, 0);
		break;
	default:
		break;
	}

	switch (tile_mode) {
	case MWV207D_TILING_MODE_LINEAR:
		mwv207d_va_modify(va, 0x80, 0x3 << 8, 3 << 8);
		break;
	case MWV207D_TILING_MODE_SUPERX:
		mwv207d_va_modify(va, 0x80, 0x3 << 8, 1 << 8);
		break;
	default:
		break;
	}

	mwv207d_va_modify(va, 0x80, 0x1 << 12, 0);
	mwv207d_va_modify(va, 0x80, 0x1 << 16, 0);
}

static void mwv207d_hw_config_compress(struct mwv207d_va *va,
					u64 tsaddr, u64 fbaddr)
{
	mwv207d_va_modify(va, 0x390, 1, 0);
	mwv207d_va_write(va,  0x2D4, (u32)(fbaddr & 0xFFFFFFFF));
	mwv207d_va_write(va,  0x2FC, (u32)((fbaddr >> 32) & 0xFF));
	mwv207d_va_write(va,  0x2D0, (u32)(tsaddr & 0xFFFFFFFF));
	mwv207d_va_write(va,  0x2F8, (u32)((tsaddr >> 32) & 0xFF));
}

static void mwv207d_plane_update_hw(struct mwv207d_va *va, u32 src_w, u32 src_h,
				    u32 stride, u32 compressed, u32 tile_mode,
				    u64 fbaddr, u64 tsaddr, u64 fbbase, u32 format)
{
	mwv207d_va_write(va, 0x0, 0);

	mwv207d_va_write(va, 0x74, 0);
	mwv207d_va_write(va, 0x90, ((src_h - 1) << 16) | (src_w - 1));

	mwv207d_va_write(va, 0x84, fbaddr & 0xFF);
	mwv207d_va_write(va, 0x88, (u32)(fbaddr >> 8));

	mwv207d_hw_set_format(va, format, tile_mode);

	mwv207d_va_write(va, 0x8C,
			(tile_mode == MWV207D_TILING_MODE_SUPERX ?
			 stride * 64 : stride) >> 4);

	if (compressed)
		mwv207d_hw_config_compress(va, tsaddr, fbbase);
	else
		mwv207d_va_modify(va, 0x390, 1, 1);

	mwv207d_va_write(va, 0x0, 1);

}

static void mwv207d_va_do_workaround(struct mwv207d_va *va,
				     int threshold, bool is_page_flip)
{
	struct mwv207d_blt_args *args = &va->args;
	struct mwv207d_workaround_bo *wbo;
	u64 workaround_period;
	u64 fbaddr;
	int i;

	if (!mwv207d_va_workaround_need_blt(va))
		goto out;

	wbo = va->wbo[va->blt_idx];
	if (mwv207d_va_pin_map_bo(va, wbo->shadow_bo))
		goto out;

	args->dst_bo = wbo->shadow_bo;
	for (i = 0; i < 5; i++) {
		if (!mwv207d_va_blt_to_shadow(va))
			break;
	}
	if (i == 5)
		dev_warn(va->mdev->dev, "va %d blt to shadow failed", va->idx);

	dev_dbg(va->mdev->dev, "va %d %s\n", va->idx, __func__);

	fbaddr = mwv207d_bo_gpu_offset(args->dst_bo);
	mwv207d_plane_update_hw(va, args->src_w, args->src_h, args->dst_stride,
				args->compressed, MWV207D_TILING_MODE_SUPERX,
				fbaddr, fbaddr + args->dst_ts_offset, fbaddr,
				DRM_FORMAT_ARGB8888);

	if (va->cur_workaround_bo)
		mwv207d_va_cleanup_workaround_bo(va->cur_workaround_bo);
	va->cur_workaround_bo = mwv207d_workaround_bo_get(wbo);
	va->blt_idx = (va->blt_idx + 1) % 0x3;

out:
	if (is_page_flip)
		workaround_period = va->vblank_period * threshold;
	else
		workaround_period = va->vblank_period / threshold;

	queue_delayed_work(system_wq, &va->page_flush_work, workaround_period);
}

static void mwv207d_va_page_flush_work_func(struct work_struct *work)
{
	struct mwv207d_va *va = container_of(work, struct mwv207d_va, page_flush_work.work);

	dev_dbg(va->mdev->dev, "va %d %s\n", va->idx, __func__);

	mwv207d_va_do_workaround(va, 2, false);
}

static void mwv207d_plane_update_workaround(struct drm_plane *plane,
					    struct drm_atomic_state *old_state)
{
	struct mwv207d_va *va = plane_to_va(plane);
	struct drm_vblank_crtc *vblank;
	struct drm_crtc *crtc;
	unsigned int pipe;
	int i;

	crtc = &va->crtc;
	pipe = drm_crtc_index(crtc);
	vblank = &crtc->dev->vblank[pipe];
	drm_calc_timestamping_constants(crtc, &crtc->state->adjusted_mode);
	va->vblank_period = nsecs_to_jiffies(vblank->framedur_ns);

	for (i = 0; i < 5; i++) {
		if (!mwv207d_plane_prepare_workaround(plane_to_va(plane), plane->state))
			break;
	}

	if (i == 5) {
		dev_warn(va->mdev->dev, "va %d prepar workaround failed", va->idx);
		return;
	}

	mwv207d_va_do_workaround(va, 2, true);
}

static void mwv207d_plane_update(struct drm_plane *plane,
				 struct drm_atomic_state *old_state)
{
	struct mwv207d_bo *mbo;
	u64 fbaddr, fbbase, tsaddr;
	u64 tile_flags, ts_offset;
	u32 src_w, src_h, compressed, tile_mode, stride;

	mbo = gbo_to_mbo(plane->state->fb->obj[0]);
	mwv207d_bo_get_tiling_flags(mbo, &tile_flags);
	tile_mode  = MWV207D_TILING_GET(tile_flags, MODE);
	compressed = MWV207D_TILING_GET(tile_flags, COMPRESS);
	ts_offset = MWV207D_TILING_GET(tile_flags, OFFSET_256B) << 8;

	fbaddr = mwv207d_plane_fb_addr(plane, tile_mode);
	fbbase = mwv207d_bo_gpu_offset(mbo);
	tsaddr = fbbase + ts_offset;
	stride = plane->state->fb->pitches[0];
	src_w  = plane->state->src_w >> 16;
	src_h  = plane->state->src_h >> 16;

	mwv207d_plane_update_hw(plane_to_va(plane), src_w, src_h, stride,
			compressed, tile_mode, fbaddr, fbbase + ts_offset, fbbase,
			plane->state->fb->format->format);
}

static void mwv207d_primary_atomic_update(struct drm_plane *plane,
					  struct drm_atomic_state *old_state)
{
	struct mwv207d_va *va = plane_to_va(plane);
	struct mwv207d_bo *mbo;
	u64 tile_flags;

	if (!plane->state->fb || !plane->state->crtc)
		return;

	mbo = gbo_to_mbo(plane->state->fb->obj[0]);
	mwv207d_bo_get_tiling_flags(mbo, &tile_flags);

	cancel_delayed_work_sync(&va->page_flush_work);

	if (mwv207d_plane_need_workaround(plane->state,
				MWV207D_TILING_GET(tile_flags, MODE)))
		mwv207d_plane_update_workaround(plane, old_state);
	else
		mwv207d_plane_update(plane, old_state);
}

static void mwv207d_primary_atomic_disable(struct drm_plane *plane,
					   struct drm_atomic_state *old_state)
{
	struct mwv207d_va *va = plane_to_va(plane);
	cancel_delayed_work_sync(&va->page_flush_work);
	if (va->cur_workaround_bo) {
		mwv207d_va_cleanup_workaround_bo(va->cur_workaround_bo);
		va->cur_workaround_bo = NULL;
	}
}

static void mwv207d_cursor_switch(struct mwv207d_va *va, bool on)
{
	mwv207d_va_modify(va, 0xB4, 0xf << 16, on ? (0xa << 16) : 0);
}

static void mwv207d_cursor_atomic_update(struct drm_plane *plane,
					 struct drm_atomic_state *old_state)
{
	int crtc_x, crtc_y, pos_x, pos_y, hot_x, hot_y;
	struct mwv207d_va *va = plane_to_va(plane);
	u64 fbaddr;

	if (!plane->state->fb || !plane->state->crtc)
		return;

	crtc_x = plane->state->crtc_x;
	crtc_y = plane->state->crtc_y;
	fbaddr = mwv207d_plane_fb_addr(plane, MWV207D_TILING_MODE_LINEAR);

	mwv207d_va_write(va, 0x210, (u32)(fbaddr >> 8));

	pos_x = crtc_x < 0 ? 0 : crtc_x;
	hot_x = crtc_x < 0 ? -crtc_x : 0;
	pos_y = crtc_y < 0 ? 0 : crtc_y;
	hot_y = crtc_y < 0 ? -crtc_y : 0;

	mwv207d_va_write(va, 0x224, 0);
	mwv207d_va_write(va, 0x214, (hot_x & 0x7f) | ((hot_y & 0x7f) << 16));
	mwv207d_va_write(va, 0x218, (pos_x & 0xffff) | ((pos_y & 0xffff) << 16));

	mwv207d_cursor_switch(va, true);
}

static void mwv207d_cursor_atomic_disable(struct drm_plane *plane,
					  struct drm_atomic_state *old_state)
{
	struct mwv207d_va *va = plane_to_va(plane);

	mwv207d_cursor_switch(va, false);
}

static int mwv207d_plane_atomic_async_check(struct drm_plane *plane,
					    struct drm_atomic_state *state)
{

	if (plane->type != DRM_PLANE_TYPE_CURSOR)
		return -EINVAL;

	return 0;
}

static void mwv207d_plane_atomic_async_update(struct drm_plane *plane,
					      struct drm_atomic_state *state)
{
	struct drm_plane_state *new_state =
		drm_atomic_get_new_plane_state(state, plane);

	WARN_ON((new_state->crtc == NULL && new_state->fb != NULL) ||
			(new_state->crtc != NULL && new_state->fb == NULL));

	if (!new_state->fb) {
		mwv207d_cursor_atomic_disable(plane, state);
		return;
	}

	swap(plane->state->fb, new_state->fb);

	plane->state->src_x = new_state->src_x;
	plane->state->src_y = new_state->src_y;
	plane->state->src_w = new_state->src_w;
	plane->state->src_h = new_state->src_h;
	plane->state->crtc_x = new_state->crtc_x;
	plane->state->crtc_y = new_state->crtc_y;
	plane->state->crtc_w = new_state->crtc_w;
	plane->state->crtc_h = new_state->crtc_h;

	mwv207d_cursor_atomic_update(plane, state);
}

static const struct drm_plane_funcs mwv207d_plane_funcs = {
	.reset			= drm_atomic_helper_plane_reset,
	.update_plane	        = drm_atomic_helper_update_plane,
	.disable_plane	        = drm_atomic_helper_disable_plane,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_plane_destroy_state,
	.format_mod_supported   = mwv207d_plane_format_mod_supported,
	.destroy	        = drm_plane_cleanup,
};

static const struct drm_plane_helper_funcs mwv207d_primary_helper = {
	.prepare_fb     = mwv207d_plane_prepare_fb,
	.cleanup_fb     = mwv207d_plane_cleanup_fb,
	.atomic_check   = mwv207d_plane_atomic_check,
	.atomic_update  = mwv207d_primary_atomic_update,
	.atomic_disable = mwv207d_primary_atomic_disable,
};

static const struct drm_plane_helper_funcs mwv207d_cursor_helper = {
	.prepare_fb     = mwv207d_plane_prepare_fb,
	.cleanup_fb     = mwv207d_plane_cleanup_fb,
	.atomic_check   = mwv207d_plane_atomic_check,
	.atomic_async_check = mwv207d_plane_atomic_async_check,
	.atomic_async_update = mwv207d_plane_atomic_async_update,
	.atomic_update  = mwv207d_cursor_atomic_update,
	.atomic_disable = mwv207d_cursor_atomic_disable,
};

static void mwv207d_va_lut_enable(struct mwv207d_va *va)
{
	struct mwv207d_palette *palette = &va->palette;
	u32 *palette_addr = palette->addr;
	u16 *lut = va->lutdata;
	u32 value;
	int i, j;

	for (i = 0; i < 256; ++i) {
		value = lut[i] << 22 | lut[i + 256] << 12 | lut[i + 512] << 2;
		for (j = 0; j < 4; ++j)
			palette_addr[i * 4 + j] = value;
	}
	memcpy(palette_addr + 1024, palette_addr, 1024 * 4);
	memcpy(palette_addr + 2048, palette_addr, 1024 * 4);

	mwv207d_va_read(va, 0xFC);
	mb();

	mwv207d_va_modify(va, 0xFC, 1 << 31, 0 << 31);
	mwv207d_va_modify(va, 0x118, 1 << 31, 0 << 31);
	mwv207d_va_write(va, 0xF0, palette->phys >> 8);
}

static int mwv207d_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);

	mwv207d_va_modify(va, 0x230, 1, 0x1);
	mwv207d_va_modify(va, 0x23C, 1, 0x1);

	if (va->mdev->isr_poll)
		mwv207d_va_enable_sw_vblank(crtc);

	return 0;
}

static void mwv207d_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);

	mwv207d_va_modify(va, 0x230, 1, 0x0);
	mwv207d_va_modify(va, 0x23C, 1, 0x0);

	if (va->mdev->isr_poll)
		mwv207d_va_disable_sw_vblank(crtc);
}

static int mwv207d_crtc_atomic_check(struct drm_crtc *crtc,
				     struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state, crtc);

	if (crtc_state->gamma_lut && drm_color_lut_size(crtc_state->gamma_lut) != 256)
		return -EINVAL;

	return 0;
}

static void mwv207d_va_set_timing(struct mwv207d_va *va)
{
	struct drm_display_mode *m = &va->crtc.state->adjusted_mode;
	u32 hfrontporch, hsync, hbackporch, htotal, hactive;
	u32 vfrontporch, vsync, vbackporch, vtotal, vactive;
	u32 value;
	int pal, interleaved;
	int ret;

	ret = mwv207d_vbios_set_pll(va->mdev, va->idx + MWV207D_PLL_VA0,
				    m->crtc_clock);
	if (ret) {
		DRM_ERROR("mwv207d: failed to set crtc%d clock, %d",
			  va->idx, ret);
		return;
	}

	hfrontporch = m->crtc_hsync_start - m->crtc_hdisplay;
	hsync       = m->crtc_hsync_end - m->crtc_hsync_start;
	hbackporch  = m->crtc_htotal - m->crtc_hsync_end;
	htotal      = m->crtc_htotal;
	hactive     = m->crtc_hdisplay;

	vfrontporch = m->crtc_vsync_start - m->crtc_vdisplay;
	vsync       = m->crtc_vsync_end - m->crtc_vsync_start;
	vbackporch  = m->crtc_vtotal - m->crtc_vsync_end;
	vtotal      = m->crtc_vtotal;
	vactive     = m->crtc_vdisplay;

	mwv207d_va_write(va, 0x180, hfrontporch);
	mwv207d_va_write(va, 0x184, hsync);
	mwv207d_va_write(va, 0x188, hbackporch);
	mwv207d_va_write(va, 0x18C, htotal);

	mwv207d_va_write(va, 0x190, vfrontporch);
	mwv207d_va_write(va, 0x194, vsync);
	mwv207d_va_write(va, 0x198, vbackporch);
	mwv207d_va_write(va, 0x19C, vtotal);

	value = ((vtotal - 1) & 0xFFFF);
	value |= (((vfrontporch + vsync + vbackporch - 1) & 0xFFFF) << 16);
	mwv207d_va_write(va, 0x1a4, value);

	value = ((vfrontporch + vsync + vbackporch + (vactive / 2) - 1) & 0xFFFF);
	value |= (((2 * (vfrontporch + vsync + vbackporch) + (vactive / 2)) & 0xFFFF) << 16);
	mwv207d_va_write(va, 0x1a8, value);

	interleaved = DRM_MODE_FLAG_INTERLACE & m->flags ? 1 : 0;
	pal = (hactive == 1440 && vactive == 288) ? 1 : 0;
	mwv207d_va_write(va, 0x1A0, interleaved | (pal << 1));
	mwv207d_va_write(va, 0x78, 1);
}

static inline bool va_idle(struct mwv207d_va *va)
{
	return !!(mwv207d_va_read(va, 0x2B0) & 0x1);
}

static inline bool va_nopending(struct mwv207d_va *va)
{
	return (mwv207d_va_read(va, 0x2B4) & 0x7) == 0x7;
}

static void mwv207d_va_reset_clk(struct mwv207d_va *va, u32 state)
{
	mdev_modify(va->mdev, 0x2B0008,
		    0x1 << (16 + va->idx), state << (16 + va->idx));
	mdev_modify(va->mdev, 0x2B0008,
		    0x1 << va->idx, state << va->idx);
}

static void mwv207d_crtc_va_reset_channel(struct mwv207d_va *va)
{
	int ret;

	mwv207d_va_write(va, 0x34, 0);
	mwv207d_va_write(va, 0x78, 0);
	mwv207d_va_write(va, 0x258, 1);

	ret = wait_for(va_idle(va), 5000);
	if (ret)
		DRM_ERROR("mwv207d: va wait idle timeout");

	ret = wait_for(va_nopending(va), 5000);
	if (ret)
		DRM_ERROR("mwv207d: va wait nopending timeout");

	mwv207d_va_reset_clk(va, 0);
	mwv207d_va_set_timing(va);
	mwv207d_va_reset_clk(va, 1);
}

static void mwv207d_va_set_black_mode(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);

	mwv207d_va_write(va, 0x1f0, 0x0);
	mwv207d_va_write(va, 0x78, 0x0);
	mwv207d_va_write(va, 0x1ec, 0x0);
	mwv207d_va_write(va, 0x1f8, 0x0);
	mwv207d_va_write(va, 0x1f0, 0x1);
}

static void mwv207d_crtc_atomic_enable(struct drm_crtc *crtc,
				       struct drm_atomic_state *old_state)
{
	struct mwv207d_va *va = crtc_to_va(crtc);
	struct drm_encoder *encoder;

	drm_for_each_encoder_mask(encoder, crtc->dev,
				  crtc->state->encoder_mask)
		mwv207d_output_set_crtc(encoder, crtc);

	mwv207d_crtc_va_reset_channel(va);
	drm_crtc_vblank_on(crtc);
}

static void mwv207d_crtc_atomic_disable(struct drm_crtc *crtc,
					struct drm_atomic_state *old_state)
{
	mwv207d_va_set_black_mode(crtc);
	drm_crtc_vblank_off(crtc);
}

static void mwv207d_crtc_atomic_flush(struct drm_crtc *crtc,
				      struct drm_atomic_state *old_state)
{
	struct mwv207d_va *va = crtc_to_va(crtc);
	struct drm_color_lut *lut;
	int i;

	if (crtc->state->color_mgmt_changed) {
		if (crtc->state->gamma_lut) {
			lut = (struct drm_color_lut *)crtc->state->gamma_lut->data;
			for (i = 0; i < 256; i++) {
				va->lutdata[i] = drm_color_lut_extract(lut[i].red, 8);
				va->lutdata[i + 256] = drm_color_lut_extract(lut[i].green, 8);
				va->lutdata[i + 512] = drm_color_lut_extract(lut[i].blue, 8);
			}
			mwv207d_va_lut_enable(va);
		}
	}
}

static int mwv207d_va_lut_init(struct mwv207d_va *va)
{
	struct mwv207d_palette *palette = &va->palette;
	struct mwv207d_bo *mbo;
	uint16_t *lut = &va->lutdata[0];
	u64 gpu_addr;
	void *addr;
	int ret, i;

	for (i = 0; i < 256; i++)
		lut[i] = lut[i + 256] = lut[i + 512] = i;

	ret = mwv207d_bo_create(va->mdev, 4 * 1024 * 3, 0x1000, true,
				0x2,
				0x1,
				NULL, NULL, &mbo);
	if (ret)
		return ret;

	ret = mwv207d_bo_reserve(mbo, false);
	if (ret)
		goto unref_bo;
	ret = mwv207d_bo_pin(mbo, 0x2, &gpu_addr);
	if (ret)
		goto unres_bo;
	ret = mwv207d_bo_kmap(mbo, &addr);
	if (ret)
		goto unpin_bo;
	palette->bo = mbo;
	palette->addr = addr;
	palette->phys = gpu_addr;

	mwv207d_bo_unreserve(mbo);

	return 0;
unpin_bo:
	mwv207d_bo_unpin(mbo);
unres_bo:
	mwv207d_bo_unreserve(mbo);
unref_bo:
	mwv207d_bo_unref(&mbo);
	return ret;
}

static void mwv207d_va_lut_fini(struct mwv207d_va *va)
{
	struct mwv207d_bo *mbo = va->palette.bo;

	if (!mbo)
		return;

	if (mwv207d_bo_reserve(mbo, false)) {
		pr_err("mwv207d: failed to reserve palette bo");
		return;
	}
	mwv207d_bo_kunmap(mbo);
	mwv207d_bo_unpin(mbo);
	mwv207d_bo_unreserve(mbo);
	mwv207d_bo_unref(&mbo);
}

int mwv207d_va_lut_suspend(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);
	struct mwv207d_bo *mbo = va->palette.bo;
	int ret;

	ret = mwv207d_bo_reserve(mbo, true);
	if (ret)
		return ret;
	mwv207d_bo_kunmap(mbo);
	mwv207d_bo_unpin(mbo);
	mwv207d_bo_unreserve(mbo);

	return 0;
}

int mwv207d_va_lut_resume(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);
	struct mwv207d_bo *mbo = va->palette.bo;
	int ret;

	ret = mwv207d_bo_reserve(mbo, true);
	if (ret)
		return ret;
	ret = mwv207d_bo_pin(mbo, 0x2, &va->palette.phys);
	if (ret)
		goto unreserve_bo;
	ret = mwv207d_bo_kmap(mbo, &va->palette.addr);
	if (ret)
		mwv207d_bo_unpin(mbo);

unreserve_bo:
	mwv207d_bo_unreserve(mbo);
	return ret;
}

static void mwv207d_va_reset(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);

	memset(&va->args, 0, sizeof(struct mwv207d_blt_args));
	va->last_fence_context = 0;
	va->last_fence_seq = 0;
	va->blt_idx = 0;

	mwv207d_cursor_switch(va, false);
	mwv207d_va_set_black_mode(crtc);
	mwv207d_va_lut_enable(va);

	drm_atomic_helper_crtc_reset(crtc);
}

static void mwv207d_va_irq_fini(struct mwv207d_va *va);

static void mwv207d_va_destroy(struct drm_crtc *crtc)
{
	struct mwv207d_va *va = crtc_to_va(crtc);
	int i;

	cancel_delayed_work_sync(&va->page_flush_work);

	for (i = 0; i < 0x3; i++) {
		if (!va->wbo[i])
			continue;
		mwv207d_workaround_bo_put(va->wbo[i]);
	}

	mwv207d_va_lut_fini(va);
	mwv207d_va_irq_fini(va);
	drm_crtc_cleanup(crtc);
}

static const struct drm_crtc_funcs mwv207d_crtc_funcs = {
	.reset                  = mwv207d_va_reset,
	.destroy                = mwv207d_va_destroy,
	.set_config             = drm_atomic_helper_set_config,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_crtc_destroy_state,
	.page_flip              = drm_atomic_helper_page_flip,
	.enable_vblank          = mwv207d_crtc_enable_vblank,
	.disable_vblank         = mwv207d_crtc_disable_vblank,
};

static const struct drm_crtc_helper_funcs mwv207d_crtc_helper = {
	.atomic_check   = mwv207d_crtc_atomic_check,
	.atomic_flush   = mwv207d_crtc_atomic_flush,
	.atomic_enable  = mwv207d_crtc_atomic_enable,
	.atomic_disable = mwv207d_crtc_atomic_disable,
};

void mwv207d_crtc_prepare_vblank(struct drm_crtc *crtc)
{
	struct drm_pending_vblank_event *event = crtc->state->event;
	struct mwv207d_va *va = crtc_to_va(crtc);
	unsigned long flags;

	if (event) {
		if (!va->mdev->isr_poll && crtc->state->active) {
			WARN_ON(drm_crtc_vblank_get(crtc) != 0);
			spin_lock_irqsave(&crtc->dev->event_lock, flags);
			va->event = event;
			spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
		} else {
			spin_lock_irqsave(&crtc->dev->event_lock, flags);
			drm_crtc_send_vblank_event(crtc, crtc->state->event);
			spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
		}
		crtc->state->event = NULL;
	}
}

static void mwv207d_crtc_finish_vblank(struct mwv207d_va *va)
{
	struct drm_crtc *crtc = &va->crtc;
	int put_vblank = false;
	unsigned long flags;

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (va->event) {
		drm_crtc_send_vblank_event(crtc, va->event);
		va->event = NULL;
		put_vblank = true;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

	if (put_vblank)
		drm_crtc_vblank_put(crtc);
}

irqreturn_t mwv207d_va_handle_vblank(int irq, void *data)
{
	struct mwv207d_va *va = data;
	u32 fired;

	fired = mwv207d_va_read(va, 0x238);
	if (!fired)
		return IRQ_NONE;
	mwv207d_va_write(va, 0x234, fired);

	if (fired & 0x1) {
		drm_crtc_handle_vblank(&va->crtc);
		mwv207d_crtc_finish_vblank(va);
	}

	return IRQ_HANDLED;
}

static int mwv207d_va_irq_init(struct mwv207d_va *va)
{
	int va_irq = 0x8 + va->idx;
	int ret;

	if (va->mdev->isr_poll)
		return 0;

	va->va_irq = mwv207d_irq_find(va->mdev, va_irq, 0);
	BUG_ON(!va->va_irq);

	ret = request_irq(va->va_irq, mwv207d_va_handle_vblank, 0, va->name, va);
	if (ret) {
		dev_err(va->mdev->dev, "error, failed to request irq %d, %d\n",
			va_irq, ret);
		return ret;
	}

	return 0;
}

static void mwv207d_va_irq_fini(struct mwv207d_va *va)
{
	free_irq(va->va_irq, va);
}

static int __va_init(struct mwv207d_device *mdev, int idx)
{
	struct mwv207d_va *va;
	int ret;

	va = devm_kzalloc(mdev->dev, sizeof(*va), GFP_KERNEL);
	if (!va)
		return -ENOMEM;
	va->idx = idx;
	va->mdev = mdev;
	va->blt_idx = 0;
	va->mmio = mdev->mmio + (0x370000 + 0x1000 * (idx));
	sprintf(va->name, "mwv207d_va%d", idx);

	ret = drm_universal_plane_init(&mdev->base, &va->primary, 1 << idx,
				       &mwv207d_plane_funcs, rgb_formats,
				       ARRAY_SIZE(rgb_formats),
				       NULL, DRM_PLANE_TYPE_PRIMARY, NULL);
	if (ret)
		return ret;
	drm_plane_helper_add(&va->primary, &mwv207d_primary_helper);

	ret = drm_universal_plane_init(&mdev->base, &va->cursor, 1 << idx,
				       &mwv207d_plane_funcs, cursor_formats,
				       ARRAY_SIZE(cursor_formats),
				       NULL, DRM_PLANE_TYPE_CURSOR, NULL);
	if (ret)
		return ret;
	drm_plane_helper_add(&va->cursor, &mwv207d_cursor_helper);

	ret = drm_crtc_init_with_planes(&mdev->base, &va->crtc,
					&va->primary, &va->cursor,
					&mwv207d_crtc_funcs, NULL);
	if (ret)
		return ret;
	drm_crtc_helper_add(&va->crtc, &mwv207d_crtc_helper);

	ret = mwv207d_va_lut_init(va);
	if (ret)
		return ret;
	drm_crtc_enable_color_mgmt(&va->crtc, 0, false, 256);
	ret = drm_mode_crtc_set_gamma_size(&va->crtc, 256);
	if (ret)
		return ret;

	INIT_DELAYED_WORK(&va->page_flush_work, mwv207d_va_page_flush_work_func);

	hrtimer_init(&va->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	va->timer.function = &mwv207d_va_vblank_sw_simulate;

	BUG_ON(drm_crtc_index(&va->crtc) != va->idx);

	ret = mwv207d_va_irq_init(va);
	if (ret)
		return ret;
	return 0;
}

int mwv207d_va_init(struct mwv207d_device *mdev)
{
	int i, ret;

	for (i = 0; i < 4; i++) {
		ret = __va_init(mdev, i);
		if (ret)
			return ret;
	}

	return 0;
}
