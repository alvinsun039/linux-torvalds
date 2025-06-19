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

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <sched/signal.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#if (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE)
#include <linux/dma-resv.h>
#else
#include <linux/kthread.h>
#include <linux/reservation.h>
#endif
#include <drm/drm.h>
#include <drm/gpu_scheduler.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include "common/gb_uk.h"
#include "gb_device.h"
#include "gb_stage.h"
#include "gb_gem.h"
#include "gb_regs.h"
#include "gb_gpu.h"
#include "gb_mmu.h"
#include "gb_gpu_irq.h"
#include "gb_pages_alloc.h"
#include "gb_ttm.h"
#include "gpu_test/gb_mem_tool.h"
#include "mcu_peripherals/gpu_freq/gpu_freq.h"

extern bool gb_time_debug;
extern int job_timeout_ms;

#define GB02MAC1939 (16 * 1024 * 1024)
u32 *err_vram_buff = NULL;

#define to_drm_sched_job(sched_job)	\
		container_of((sched_job), struct drm_sched_job, queue_node)
#define to_gb_remain_job(sched_job)	\
		container_of((sched_job), struct GB02STR163, queue_node)


static struct GB02STR162 *GB02FUNC1228(struct drm_sched_job *sched_job)
{
	return container_of(sched_job, struct GB02STR162, base);
}

struct GB02STR158 {
	struct dma_fence base;
	struct drm_device *dev;

	u64 seqno;
	int queue;
};

int GB02FUNC1230(char *time_buf)
{
	long long tv_usec;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec now = {
		.tv_sec=0,
		.tv_nsec=0
	};
	getnstimeofday(&now);
#else
	struct timespec64 now = {
		.tv_sec=0,
		.tv_nsec=0
	};
	ktime_get_real_ts64(&now);

#endif
	tv_usec = now.tv_sec * 1000000 + now.tv_nsec / 1000;

	sprintf(time_buf, "UTC TIME: %lld", tv_usec);
	return 0;
}

int GB02FUNC1232(u64 size)
{
	int i;
	u64 size_t;
	struct GB02STR59 *bo = NULL;
	struct GB02STR138 *mmu_mode;
	struct drm_mm_node *node;
	struct gb_heap_ttm_bo *gb_heap_bo;
	struct GB02STR70 *pcie_dev = GB02FUNC518();
	struct GB02STR39 *gbdev = pcie_dev->gbdev;
	mmu_mode = gbdev->mmu_mode;

	mutex_lock(&mmu_mode->mm_lock);
	for (i = 0; i < GB_MAX_VA_MM; i++) {
		drm_mm_for_each_node(node, &mmu_mode->mm[i]) {
			bo = GB02FUNC821(node);
			if (bo->is_heap_growable) {
				pr_info("%s heap_growable:0x%llx start iovaddr:0x%llx\n", __func__,
					(unsigned long long)bo, (unsigned long long)bo->base.iovaddr);
				list_for_each_entry(gb_heap_bo, &bo->gb_base.ttm_bo_root, ttm_bo_list) {
					if (size) {
						size_t = size;
					} else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
                                                size_t = PFN_UP(gb_heap_bo->ttm_bo.bo.base.size) * PAGE_SIZE;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
						size_t = gb_heap_bo->ttm_bo.bo.resource->num_pages * PAGE_SIZE;
#else
						size_t = gb_heap_bo->ttm_bo.bo.mem.num_pages * PAGE_SIZE;
#endif
					}
					GB02FUNC188(size_t, gb_heap_bo->pages[0], NULL, true);
				}
				pr_info("%s heap_growable:0x%llx end \n", __func__, (unsigned long long)bo);
			} else {
				pr_info("%s normol:0x%llx start iovaddr:0x%llx\n", __func__,
					(unsigned long long)bo, (unsigned long long)bo->base.iovaddr);
				if (size)
					size_t = size;
				else
					size_t = bo->base.nr_pages * PAGE_SIZE;
				GB02FUNC188(size_t, bo->base.pages[0], NULL, true);
				pr_info("%s normol:0x%llx end iovaddr:0x%llx\n", __func__,
					(unsigned long long)bo, (unsigned long long)bo->base.iovaddr);
			}
		}
	}
	mutex_unlock(&mmu_mode->mm_lock);

	return 0;
}

static inline struct GB02STR158 *GB02FUNC1237(struct dma_fence *fence)
{
	return (struct GB02STR158 *)fence;
}

static const char *GB02FUNC1238(struct dma_fence *fence)
{
	return "gb";
}

static const char *GB02FUNC1240(struct dma_fence *fence)
{
	struct GB02STR158 *fen = GB02FUNC1237(fence);

	switch (fen->queue) {
	case 0:
		return "gb-slot0";
	case 1:
		return "gb-slot1";
	case 2:
		return "gb-slot2";
	default:
		return NULL;
	}
}

static void GB02FUNC1243(struct rcu_head *rcu)
{
        struct dma_fence *f = container_of(rcu, struct dma_fence, rcu);
        struct GB02STR158 *fence = GB02FUNC1237(f);
        kfree(fence);
}

void GB02FUNC1244(struct dma_fence *fence)
{
        struct GB02STR158 *g_fence = GB02FUNC1237(fence);
	call_rcu(&g_fence->base.rcu, GB02FUNC1243);
}

static const struct dma_fence_ops gb_fence_ops = {
        .get_driver_name = GB02FUNC1238,
        .get_timeline_name = GB02FUNC1240,
        .release = GB02FUNC1244,
};

static struct dma_fence *GB02FUNC1245(struct GB02STR39 *gbdev, int ss_num)
{
	struct GB02STR158 *fence;
	struct GB02STR165 *ss = gbdev->ss;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return ERR_PTR(-ENOMEM);

	fence->dev = gbdev->ddev;
	fence->queue = ss_num;
	fence->seqno = ++ss->queue[ss_num].emit_seqno;
	dma_fence_init(&fence->base, &gb_fence_ops, &ss->stages_lock,
		       ss->queue[ss_num].fence_context, fence->seqno);

	return &fence->base;
}

int GB02FUNC1248(struct GB02STR162 *stage)
{
	/* slot0 for fragment stages.
	 * slot1 for vertex/tiler stages
	 * slot2 for compute stages
	 */
	if (stage->slot_req & GB02MAC920)
		return 0;

	return 1;
}

static void GB02FUNC1251(struct GB02STR39 *gbdev,
					u32 slot_req,
					int ss)
{
	u64 affinity;

	affinity = gbdev->features.shader_present;

	stage_write(gbdev, GB02MAC1518(ss), affinity & 0xFFFFFFFF);
	stage_write(gbdev, GB02MAC1519(ss), affinity >> 32);
}

static void GB02FUNC1254(struct GB02STR162 *stage, int ss)
{
	struct GB02STR39 *gbdev = stage->gbdev;
	struct GB02STR47 *priv = stage->file_priv;
	u32 cfg;
	u64 sc_head = stage->sc;
	int ret;
	unsigned long flags;
	char time_buf[GB02MAC1960];

	ret = pm_runtime_get_sync(gbdev->dev);
	if (ret < 0)
		return;

	if (WARN_ON(stage_read(gbdev, GB02MAC1522(ss)))) {
		pm_runtime_put_sync_autosuspend(gbdev->dev);
		return;
	}

	cfg = GB02FUNC935();
	stage->as = cfg;
	/*update priv for gb_reset */
	gbdev->priv[cfg] = priv;
	GB02FUNC698(gbdev, ss);

	stage_write(gbdev, GB02MAC1516(ss), sc_head & 0xFFFFFFFF);
	stage_write(gbdev, GB02MAC1517(ss), sc_head >> 32);

	GB02FUNC1251(gbdev, stage->slot_req, ss);

	cfg |= GB02MAC1746(8) |
		GB02MAC1731 |
		GB02MAC1740;

	cfg |= GB02MAC1742;

	stage_write(gbdev, GB02MAC1520(ss), cfg);

	stage_write(gbdev, GB02MAC1524(ss), stage->flush_id);

	gb_printf(KERN_DEBUG, "SS: Submitting atom %pK to ss[%d] with head=0x%llx, as %d", stage, ss, sc_head, cfg&0xf);

	if (gb_time_debug) {
		GB02FUNC1230(time_buf);
		pr_info("[%s,pid:%d] %s %s --stage:%llx\n",
			current->comm,current->pid,
			__func__, time_buf, (long long)stage);
	}
	spin_lock_irqsave(&gbdev->ss->stages_lock, flags);
	stage_write(gbdev, GB02MAC1522(ss), GB02MAC1684);
	spin_unlock_irqrestore(&gbdev->ss->stages_lock, flags);

}
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static void GB02FUNC1261(struct drm_gem_object **bos, int bo_count,
				     struct dma_fence **implicit_fences)
{
	int i;
#if (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)
	struct GB02STR59 *gb_bo;
#endif
	for (i = 0; i < bo_count; i++)
	{
#if (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0))
		implicit_fences[i] = dma_resv_get_excl_rcu(bos[i]->resv);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0))
		implicit_fences[i] = dma_resv_get_excl_unlocked(bos[i]->resv);
#else
		gb_bo = GB02FUNC212(bos[i]);
		implicit_fences[i] = reservation_object_get_excl_rcu(gb_bo->gb_base.ttm_bo.bo.resv);
#endif
	}
}
#else
static int GB02FUNC1258(struct drm_sched_job *job,
					    struct drm_gem_object *obj, bool write)
{
	struct dma_resv_iter cursor;
	struct dma_fence *fence;

	int ret;
	dma_resv_assert_held(obj->resv);
	//printk(KERN_INFO "%s %d\n", __func__, __LINE__);
	dma_resv_for_each_fence(&cursor, obj->resv, dma_resv_usage_rw(write),
				fence) {

		/* Make sure to grab an additional ref on the added fence */
		dma_fence_get(fence);
		ret = drm_sched_job_add_dependency(job, fence);
		if (ret) {
			dma_fence_put(fence);
			return ret;
		}
	}
	return 0;
}

static int GB02FUNC1261(struct drm_gem_object **bos, int bo_count,
				    struct drm_sched_job *job)
{
	int ret;
	int i;

	for (i = 0; i < bo_count; i++) {
		ret = dma_resv_reserve_fences(bos[i]->resv, 1);
		if (ret)
			return ret;

	/* GenBu always uses write mode in its current uapi */
        ret = GB02FUNC1258(job, bos[i],
					true);
		if (ret)
			return ret;
	}

	return 0;
}
#endif

static void GB02FUNC1266(struct drm_gem_object **bos,
				    int bo_count, struct dma_fence *fence)
{
	int i;
#if (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)
	struct GB02STR59 *gb_bo;
#endif

	for (i = 0; i < bo_count; i++)
	{
#if (KERNEL_VERSION(6, 6, 0) <= LINUX_VERSION_CODE)
		dma_resv_add_fence(bos[i]->resv, fence, DMA_RESV_USAGE_WRITE); 
#elif (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE)
		dma_resv_add_excl_fence(bos[i]->resv, fence);
#else
		gb_bo = GB02FUNC212(bos[i]);
		reservation_object_add_excl_fence(gb_bo->gb_base.ttm_bo.bo.resv, fence);
#endif
	}
}

int GB02FUNC1270(struct GB02STR44 *jsl)
{
	int ret;

	ret = down_trylock(&jsl->job_semaphore);
	if(!ret)
		gb_printf(KERN_INFO, "%s: %p down %u ret:%d\n",
			__func__, current,
			jsl->job_semaphore.count, ret);

	return ret;
}

void GB02FUNC1272(struct GB02STR44 *jsl)
{
	int ret;
	struct GB02STR39 *gbdev = container_of(jsl,
			struct GB02STR39, jsl);

	//atomic_set(&proc_status_mask, 0);
	do {
		if (atomic_read(&gbdev->reset.pending))
			goto sleep;

		ret = GB02FUNC1270(jsl);
		if (!ret)
			return;

#if 0
		if (signal_pending(current))
			return;

		ret = wait_event_timeout(proc_wq,
					atomic_read(&proc_status_mask),
					msecs_to_jiffies(GB02MAC1953));
		if (ret) {
			gb_printf(KERN_INFO, "%s wait job compelet success , atomic:%d\n",
				__func__, atomic_read(&proc_status_mask));
			return;
		}
#endif

sleep:
		schedule_timeout_interruptible(GB02MAC1953/4);
	} while(1);
}

void GB02FUNC1277(struct GB02STR44 *jsl)
{
	up(&jsl->job_semaphore);
	//atomic_set(&proc_status_mask, 1);
	//wake_up(&proc_wq);
	gb_printf(KERN_INFO, "%s: %p up %u\n", __func__, current, jsl->job_semaphore.count);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
int gb_gem_lock_reservations(struct drm_gem_object **objs, int count,
                          struct ww_acquire_ctx *acquire_ctx)
{
	int contended = -1;
	int i, j, ret;
	struct GB02STR59 *bo;

	ww_acquire_init(acquire_ctx, &reservation_ww_class);

retry:
	if (contended != -1) {
		bo = GB02FUNC212(objs[contended]);
		ret = ww_mutex_lock_slow_interruptible(&bo->gb_base.ttm_bo.bo.resv->lock,
			acquire_ctx);
		if (ret) {
			ww_acquire_done(acquire_ctx);
			return ret;
		}
	}

	for (i = 0; i < count; i++) {
		if (i == contended)
			continue;

		bo = GB02FUNC212(objs[i]);
		ret = ww_mutex_lock_interruptible(&bo->gb_base.ttm_bo.bo.resv->lock,
                                                            acquire_ctx);
		if (ret) {
			for (j = 0; j < i; j++) {
				bo = GB02FUNC212(objs[j]);
				ww_mutex_unlock(&bo->gb_base.ttm_bo.bo.resv->lock);
			}

			if (contended != -1 && contended >= i) {
				bo = GB02FUNC212(objs[contended]);
				ww_mutex_unlock(&bo->gb_base.ttm_bo.bo.resv->lock);
			}

			if (ret == -EDEADLK) {
				contended = i;
				goto retry;
			}

			ww_acquire_done(acquire_ctx);
			return ret;
		}
	}

	ww_acquire_done(acquire_ctx);

	return 0;
}

void gb_gem_unlock_reservations(struct drm_gem_object **objs, int count,
                            struct ww_acquire_ctx *acquire_ctx)
{
        int i;
	struct GB02STR59 *bo;

	for (i = 0; i < count; i++)
	{
		bo = GB02FUNC212(objs[i]);
		ww_mutex_unlock(&bo->gb_base.ttm_bo.bo.resv->lock);
	}

        ww_acquire_fini(acquire_ctx);
}
#endif

int GB02FUNC1279(struct GB02STR162 *stage)
{
	struct GB02STR39 *gbdev = stage->gbdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	int slot = GB02FUNC1248(stage);
	struct drm_sched_entity *entity = &stage->file_priv->sched_entity[slot];
#endif
	struct ww_acquire_ctx acquire_ctx;
	int ret = 0;
	char time_buf[GB02MAC1960];

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	ret = drm_gem_lock_reservations(stage->bos, stage->bo_count,
					    &acquire_ctx);
#else
	ret = gb_gem_lock_reservations(stage->bos, stage->bo_count,
					   &acquire_ctx);
#endif
	if (ret) {
		gb_printf(KERN_INFO, "%s: drm_gem_lock_reservations ret %d\n",
				__func__, ret);
		goto fail;
	}

	mutex_lock(&gbdev->sched_lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	ret = drm_sched_job_init(&stage->base, entity, NULL);
	if (ret) {
		gb_printf(KERN_ERR, "%s: drm_sched_job_init ret %d\n",
				__func__, ret);
		if (stage->base.s_fence)
			kvfree(stage->base.s_fence);
		mutex_unlock(&gbdev->sched_lock);
		goto unlock;
	}
#else
	drm_sched_job_arm(&stage->base);
#endif

	stage->render_done_fence = dma_fence_get(&stage->base.s_fence->finished);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	/* Add implicit_fences from bo */
	GB02FUNC1261(stage->bos, stage->bo_count, stage->implicit_fences);
#else
	ret = GB02FUNC1261(stage->bos, stage->bo_count, &stage->base);
	if (ret) {
		gb_printf(KERN_ERR, "%s: gb_acquire_object_fencess ret %d\n",
			__func__, ret);
		mutex_unlock(&gbdev->sched_lock);
		goto unlock;
	}

	kref_get(&stage->refcount);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	drm_sched_entity_push_job(&stage->base, entity);
#else
	drm_sched_entity_push_job(&stage->base);
#endif
	mutex_unlock(&gbdev->sched_lock);

	if (gb_time_debug) {
		GB02FUNC1230(time_buf);
		pr_info("[%s,pid:%d] %s %s --stage:%llx\n",
			current->comm,current->pid,
			__func__, time_buf, (long long)stage);
	}

	/* Bind render_done_fence to bo */
	GB02FUNC1266(stage->bos, stage->bo_count, stage->render_done_fence);

unlock:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	drm_gem_unlock_reservations(stage->bos, stage->bo_count, &acquire_ctx);
#else
	gb_gem_unlock_reservations(stage->bos, stage->bo_count, &acquire_ctx);
#endif
fail:
	return ret;
}

static void GB02FUNC1287(struct kref *ref)
{
	struct GB02STR162 *stage = container_of(ref, struct GB02STR162,
						refcount);
	unsigned int i;
	char time_buf[GB02MAC1960];

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	if (stage->in_fences) {
		/* Drm scheduler call drm_sched_entity_add_dependency_cb before push job.
		* in_fence for drm_sched_entity sync, if current entity and in_fence
		* for same or other scheduler, dma_fence_put will called and append callback in entity->cb.
		* So we need't to release, but I think we should do some check.
		*/
		for (i = 0; i < stage->in_fence_count; i++)
			dma_fence_put(stage->in_fences[i]);
		kvfree(stage->in_fences);
	}


	if (stage->implicit_fences) {
		for (i = 0; i < stage->bo_count; i++)
			dma_fence_put(stage->implicit_fences[i]);
		kvfree(stage->implicit_fences);
	}
#endif

	dma_fence_put(stage->irq_done_fence);
	dma_fence_put(stage->render_done_fence);

	if (stage->bos) {
		for (i = 0; i < stage->bo_count; i++) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
			drm_gem_object_put_unlocked(stage->bos[i]);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
			drm_gem_object_put(stage->bos[i]);
#else
			drm_gem_object_unreference_unlocked(stage->bos[i]);
#endif
		}
		kvfree(stage->bos);
	}

	if (gb_time_debug) {
		GB02FUNC1230(time_buf);
		pr_info("[%s,pid:%d] %s %s --stage:%llx\n",
			current->comm,current->pid,
			__func__, time_buf, (long long)stage);
	}

	kvfree(stage);
	stage = NULL;
}

void GB02FUNC1294(struct GB02STR162 *stage)
{
	if (stage)
		kref_put(&stage->refcount, GB02FUNC1287);
}

static void GB02FUNC1296(struct drm_sched_job *sched_job)
{
	struct GB02STR162 *stage = GB02FUNC1228(sched_job);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	drm_sched_job_cleanup(sched_job);
#endif
	GB02FUNC1294(stage);
}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 20, 0)
static struct dma_fence *gb_stage_dependency(struct drm_sched_job *sched_job,
						 struct drm_sched_entity *s_entity)
{
	struct GB02STR162 *stage = GB02FUNC1228(sched_job);
	struct dma_fence *fence;
	unsigned int i;
	char time_buf[GB02MAC1960];

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	for (i = 0; i < stage->in_fence_count; i++) {
		if (stage->in_fences[i]) {
			fence = stage->in_fences[i];
			stage->in_fences[i] = NULL;
			return fence;
		}
	}

	for (i = 0; i < stage->bo_count; i++) {
		if (stage->implicit_fences[i]) {
			fence = stage->implicit_fences[i];
			stage->implicit_fences[i] = NULL;
			return fence;
		}
	}
#endif

	if (gb_time_debug) {
		GB02FUNC1230(time_buf);
		pr_info("[%s,pid:%d] %s %s --stage:%llx\n",
			current->comm,current->pid,
			__func__, time_buf, (long long)stage);
	}
	return NULL;
}
#endif

static struct dma_fence *GB02FUNC1302(struct drm_sched_job *sched_job)
{
	struct GB02STR162 *stage = GB02FUNC1228(sched_job);
	struct GB02STR39 *gbdev = stage->gbdev;
	int slot = GB02FUNC1248(stage);
	struct dma_fence *fence = NULL;
	char time_buf[GB02MAC1960];

	if (stage->reset_time >= GB02MAC1958) {
		pr_err("%s stage:0x%llx rander more then MAX error time\n",
			__func__, (long long)stage);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 11))
		return ERR_PTR(ERR_SUBMITTING_JOB);
#else
		return NULL;
#endif
	}

	if (unlikely(stage->base.s_fence->finished.error))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 11))
		return ERR_PTR(ERR_FINISHED_JOB);
#else
		return NULL;
#endif

	if (GB_RST_HW_STARTING == atomic_read(&gbdev->reset.pending)) {
		WARN_ON_ONCE(1);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 11))
		return ERR_PTR(ERR_PENDING_RST);
#else
		return NULL;
#endif
	}

	if (gb_time_debug) {
		GB02FUNC1230(time_buf);
		pr_info("[%s,pid:%d] %s %s --stage:%llx\n",
			current->comm,current->pid,
			__func__, time_buf, (long long)stage);
	}

	gbdev->stages[slot] = stage;

	fence = GB02FUNC1245(gbdev, slot);
	if (IS_ERR(fence)) {
		gb_printf(KERN_ERR, "%s: GB02FUNC1245 failed\n",
				__func__);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 11))
		return ERR_PTR(ERR_FENCE_CREATE);
#else
		return NULL;
#endif
	}

	if (stage->irq_done_fence)
		dma_fence_put(stage->irq_done_fence);
	stage->irq_done_fence = dma_fence_get(fence);

	GB02FUNC1254(stage, slot);

	return fence;
}

void GB02FUNC1305(struct GB02STR39 *gbdev)
{
	int j;
	u32 irq_mask = 0;

	for (j = 0; j < GB02MAC288; j++)
		irq_mask |= GB02MAC1955(j);

	stage_write(gbdev, GB02MAC1499, irq_mask);
	stage_write(gbdev, GB02MAC1500, irq_mask);
}


void GB02FUNC1307(struct GB02STR39 *gbdev)
{
	int j;
	u32 irq_mask = 0;

	for (j = 0; j < GB02MAC288; j++)
		irq_mask |= GB02MAC1955(j);

	stage_write(gbdev, GB02MAC1499, irq_mask);
	gb_printf(KERN_INFO, "%s: IRQ_CLEAR slot 0x%x\n", __func__, irq_mask);
}

void GB02FUNC1310(struct GB02STR39 *gbdev)
{
	int j;
	u32 irq_mask = 0;

	for (j = 0; j < GB02MAC288; j++)
		irq_mask |= GB02MAC1955(j);

	stage_write(gbdev, GB02MAC1499, ~irq_mask);
	stage_write(gbdev, GB02MAC1500, ~irq_mask);
	gb_printf(KERN_INFO, "%s: disable JOB slot mask 0x%x\n", __func__, irq_mask);
}

int GB02FUNC1312(void)
{
	int i, max_len;
	max_len = (GB02MAC1939) /16;

	if (!err_vram_buff) {
		gb_printf(KERN_INFO, "%s: no err vram buff to dump\n", __func__);
		return 0;
	}
	pr_info("start to dump err_vram_buff\n");
	for (i = 0; i < max_len; i++)
		pr_info("0x%08x 0x%08x 0x%08x 0x%08x\n",
				err_vram_buff[i * 4],
				err_vram_buff[i * 4 + 1],
				err_vram_buff[i * 4 + 2],
				err_vram_buff[i * 4 + 3]);
	pr_info("dump err_vram_buff end\n");

	kvfree(err_vram_buff);
	err_vram_buff = NULL;
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
static void gb_stage_timeout(struct drm_sched_job *sched_job)
#else
static enum drm_gpu_sched_stat
gb_stage_timeout(struct drm_sched_job *sched_job)
#endif
{
	int i, j = 0, max_dump;
	u32 size, buf_offset = 0;
	unsigned long flags;
	struct gb_heap_ttm_bo *gb_heap_bo;
	struct GB02STR162 *stage = GB02FUNC1228(sched_job);
	struct GB02STR39 *gbdev = stage->gbdev;
	int ss = GB02FUNC1248(stage);
	GB02FUNC193(stage);
	if (dma_fence_is_signaled(stage->irq_done_fence))
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
		return;
#else
		return DRM_GPU_SCHED_STAT_ENODEV;
#endif

	/* For App goout expectedly, we should check file_priv */
	if (NULL == stage->file_priv) {
		gb_printf(KERN_ERR, "%s: App goout, so we are out now",
				__func__);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
		return;
#else
		return DRM_GPU_SCHED_STAT_ENODEV;
#endif
	}

	dev_err(gbdev->dev,
		"gpu sched timeout, ss=%d, config=0x%x, "
		"status=0x%x, head=0x%x, tail=0x%x, "
		"sched_job=%pK, reset_time:%u, user_pid:0x%llx\n",
		ss,
		stage_read(gbdev, GB02MAC1512(ss)),
		stage_read(gbdev, GB02MAC1515(ss)),
		stage_read(gbdev, GB02MAC1506(ss)),
		stage_read(gbdev, GB02MAC1508(ss)),
		sched_job, stage->reset_time, stage->user_pid);

	if (!mutex_trylock(&gbdev->sched_lock))
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
		return;
#else
		return DRM_GPU_SCHED_STAT_ENODEV;
#endif

	stage->reset_time++;
	spin_lock_irqsave(&gbdev->as_lock, flags);

	if (err_vram_buff)
		memset((void *)err_vram_buff, 0, GB02MAC1939);
	else
		err_vram_buff = kvzalloc(GB02MAC1939, GFP_KERNEL);

	max_dump = (GB02MAC1939) / GB02MAC1192;
	for (i = 0; i < stage->bo_count; i++) {
                struct GB02STR59 *gem_obj;
                gem_obj = GB02FUNC212(stage->bos[i]);
		if (!gem_obj->is_heap_growable)
			continue;
		list_for_each_entry(gb_heap_bo, &gem_obj->gb_base.ttm_bo_root, ttm_bo_list) {
			if (j > max_dump - 1)
				break;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
			size = PFN_UP(gb_heap_bo->ttm_bo.bo.base.size) * PAGE_SIZE;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
			size = gb_heap_bo->ttm_bo.bo.resource->num_pages * PAGE_SIZE;
#else
			size = gb_heap_bo->ttm_bo.bo.mem.num_pages * PAGE_SIZE;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
			gb_printf(KERN_INFO, "save heap bo offset:0x%llx size:0x%llx\n",
							gb_heap_bo->ttm_bo.bo.offset, (long long)size);
			GB02FUNC188(size, gb_heap_bo->ttm_bo.bo.offset,
				(u32*)((u64)err_vram_buff + buf_offset), 0);
#else
			gb_printf(KERN_INFO, "save heap bo offset:0x%llx size:0x%llx\n",
							gb_heap_bo->ttm_bo.offset, (long long)size);
			GB02FUNC188(size, gb_heap_bo->ttm_bo.offset,
				(u32*)((u64)err_vram_buff + buf_offset), 0);
#endif
			buf_offset += size;
			j++;
		}
	}
	spin_unlock_irqrestore(&gbdev->as_lock, flags);

	for (i = 0; i < GB02MAC288; i++) {
		struct drm_gpu_scheduler *sched = &gbdev->ss->queue[i].sched;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)) || (LINUX_VERSION_CODE == KERNEL_VERSION(4, 4, 131))
		drm_sched_stop(sched, sched_job);
#else
		kthread_park(sched->thread);
		drm_sched_hw_job_reset(sched, sched_job);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		if (ss != i)
			/* Ensure any timeouts on other slots have finished */
			cancel_delayed_work_sync(&sched->work_tdr);
#endif
	}

	atomic_set(&gbdev->reset.pending, GB_RST_HW_STARTING);

	/* Close mmu AS*/
	gbdev->mmu_mode->mmu_controll(gbdev, GB02MAC1275);

	spin_lock_irqsave(&gbdev->ss->stages_lock, flags);
	for (i = 0; i < GB02MAC288; i++) {
		if (gbdev->stages[i]) {
			pm_runtime_put_noidle(gbdev->dev);
			gbdev->stages[i] = NULL;
		}
	}
	spin_unlock_irqrestore(&gbdev->ss->stages_lock, flags);

	GB02FUNC698(gbdev, ss);
	GB02FUNC882(gbdev);
	GB02FUNC136(gbdev);

	atomic_set(&gbdev->reset.pending, GB_RST_HW_COMPLETE);
	gbdev->mmu_mode->mmu_controll(gbdev, GB02MAC1274);

	/* Resubmit jobs from sched ring_mirror_list */
	for (i = 0; i < GB02MAC288; i++)
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 131)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
		drm_sched_job_recovery(&gbdev->ss->queue[i].sched);
#else
		drm_sched_resubmit_jobs(&gbdev->ss->queue[i].sched);
#endif

	/* restart scheduler after GPU is usable again */
	for (i = 0; i < GB02MAC288; i++) {
		struct drm_gpu_scheduler *sched = &gbdev->ss->queue[i].sched;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)) || (LINUX_VERSION_CODE == KERNEL_VERSION(4, 4, 131))
		drm_sched_start(sched, true);
#else
		drm_sched_job_recovery(sched);
		kthread_unpark(sched->thread);
#endif
	}

	atomic_set(&gbdev->reset.pending, GB_RST_IDLE);

	mutex_unlock(&gbdev->sched_lock);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
	return DRM_GPU_SCHED_STAT_NOMINAL;
#endif
}

static const struct drm_sched_backend_ops gb_sched_ops = {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 20, 0)
	.dependency = gb_stage_dependency,
#endif
	.run_job = GB02FUNC1302,
	.timedout_job = gb_stage_timeout,
	.free_job = GB02FUNC1296
};

void GB02FUNC1326(struct GB02STR39 *gbdev, u32 status)
{
	int j;
	u32 mask;
	char time_buf[GB02MAC1960];
	struct GB02STR162 *stage = NULL;

	dev_dbg(gbdev->dev, "stageslot irq status=%x\n", status);

	for (j = 0; status; j++) {
		mask = GB02MAC1955(j);

		if (!(status & mask))
			continue;

		stage_write(gbdev, GB02MAC1499, status);

		if (status & GB02MAC1956(j)) {
			stage_write(gbdev, GB02MAC1522(j), GB02MAC1681);
			gb_printf(KERN_ERR, "%s slot:%d stage sched fail for status:0x%x\n",
					__func__, j, status);

			stage = gbdev->stages[j];
			if (stage) {
				dev_err(gbdev->dev,
				"%s ss fault, ss=%d, status=%s, "
				"head=0x%x, tail=0x%x, job=0x%llx user_pid:0x%llx\n",
				__func__,
				j,GB02FUNC130(gbdev, stage_read(gbdev, GB02MAC1515(j))),
				stage_read(gbdev, GB02MAC1506(j)),
				stage_read(gbdev, GB02MAC1508(j)),
				(u64)gbdev->stages[j], stage->user_pid);

				if (stage->reset_time >= GB02MAC1958) {
					gbdev->stages[j] = NULL;
					gb_printf(KERN_ERR, "%s stage is faild to done \n",
						__func__);

					dma_fence_signal_locked(stage->irq_done_fence);
					pm_runtime_put_autosuspend(gbdev->dev);
				} else {
					/* TODO for fault job*/
						stage->reset_time++;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 131)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#if (defined(GB_KYLIN_SERV_TERCEL)) && (LINUX_VERSION_CODE == KERNEL_VERSION(4, 19, 90))
					mod_delayed_work(system_wq,
							&gbdev->ss->queue[j].sched.work_tdr, 0);
#else
					mod_delayed_work(system_wq,
							&stage->base.work_tdr, 0);
#endif
#else
					mod_delayed_work(system_wq,
							&gbdev->ss->queue[j].sched.work_tdr, 0);
#endif
				}
				GB02FUNC193(stage);
			}
		}

		gb_printf(KERN_DEBUG, "SS: Receiver job %p slot %d sta %s\n", gbdev->stages[j], j,
				(status & GB02MAC1957(j))? "ok":"fail");

		if (status & GB02MAC1957(j)) {
			stage = gbdev->stages[j];
			if (stage) {
				gbdev->stages[j] = NULL;

				dma_fence_signal_locked(stage->irq_done_fence);
				pm_runtime_put_autosuspend(gbdev->dev);
				GB02FUNC698(gbdev, j);

				if (gb_time_debug) {
					GB02FUNC1230(time_buf);
					pr_info("%s UTC TIME:%s --stage:0x%llx slot %d\n",
						__func__, time_buf, (long long)stage, j);
				}
			}
		}

		status &= ~mask;
	}
}

void GB02FUNC1331(struct GB02STR39 *gbdev)
{
	u32 stage_status = reg_read(gbdev, GB02MAC1498);

	while (stage_status) {
		pm_runtime_mark_last_busy(gbdev->dev);

		spin_lock(&gbdev->ss->stages_lock);
		GB02FUNC1326(gbdev, stage_status);
		spin_unlock(&gbdev->ss->stages_lock);
		stage_status = reg_read(gbdev, GB02MAC1498);
	}
}

irqreturn_t GB02FUNC1334(int irq, void *data)
{
	struct GB02STR39 *gbdev = data;

	GB02FUNC1331(gbdev);

	stage_write(gbdev, GB02MAC1500,
		  GENMASK(16 + GB02MAC288 - 1, 16) |
		  GENMASK(GB02MAC288 - 1, 0));

	return IRQ_HANDLED;
}

irqreturn_t GB02FUNC1336(int irq, void *data)
{
	u32 stage_status;
	struct GB02STR39 *gbdev = data;

	stage_status = reg_read(gbdev, GB02MAC1501);
	if (!stage_status)
		return IRQ_NONE;

	stage_write(gbdev, GB02MAC1500, 0);

	return IRQ_WAKE_THREAD;
}

irqreturn_t GB02FUNC1339(int irq, void *data)
{
	u32 stage_status;
	struct GB02STR39 *gbdev = data;

	stage_status = reg_read(gbdev, GB02MAC1501);
	if (!stage_status)
		return IRQ_NONE;

	stage_write(gbdev, GB02MAC1500, 0);

	return GB02FUNC1334(irq, data);
}

void GB02FUNC1343(struct GB02STR39 *gbdev)
{
	int i;
	struct GB02STR164 *queue;
	/* Seem everything ready, so we try to start schedulers */
	for (i = 0; i < GB02MAC288; i++) {
		queue = &gbdev->ss->queue[i];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		drm_sched_start(&queue->sched, true);
#else
		drm_sched_job_recovery(&queue->sched);
		kthread_unpark(queue->sched.thread);
#endif
	}
}

void GB02FUNC1346(struct GB02STR47 *gb_priv)
{
	int i;

	for (i = 0; i < GB02MAC288; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
		while(!list_empty(&gb_priv->gbdev->ss->queue[i].sched.pending_list)) {
#else
		while(!list_empty(&gb_priv->gbdev->ss->queue[i].sched.ring_mirror_list)) {
#endif
			msleep(50);
		}
	}
}

void GB02FUNC1349(int time_ms)
{
	int i;
	struct GB02STR164 *queue;
	struct GB02STR70 *pcie_dev = GB02FUNC518();
	struct GB02STR39 *gbdev = pcie_dev->gbdev;

	for (i = 0; i < GB02MAC288; i++) {
		queue = &gbdev->ss->queue[i];
		queue->sched.timeout = msecs_to_jiffies(time_ms);
	}
	job_timeout_ms = time_ms;

}

void GB02FUNC1352(struct GB02STR39 *gbdev)
{
	int i;
	struct GB02STR164 *queue;

	/* stop all schedulers next-to-next */
	for (i = 0; i < GB02MAC288; i++) {
		queue = &gbdev->ss->queue[i];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		cancel_delayed_work_sync(&queue->sched.work_tdr);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		drm_sched_stop(&queue->sched, NULL);
#else
		kthread_park(queue->sched.thread);
		drm_sched_hw_job_reset(&queue->sched, NULL);
#endif
	}
}

int GB02FUNC1355(struct GB02STR39 *gbdev)
{
	struct GB02STR165 *ss;
	int ret, j;

	gbdev->reset.reset_workq = alloc_workqueue("gb_rst", 0, 1);
	if (NULL == gbdev->reset.reset_workq)
		return -ENOMEM;

	gbdev->ss = ss = devm_kzalloc(gbdev->dev, sizeof(*ss), GFP_KERNEL);
	if (!ss)
		return -ENOMEM;

	spin_lock_init(&ss->fence_lock);
	spin_lock_init(&ss->stages_lock);
	spsc_queue_init(&gbdev->reset.remain_job_queue);
	sema_init(&gbdev->jsl.job_semaphore, gbdev->jsl.hw_job_limit);
	spin_lock_init(&gbdev->jsl.lock);

	for (j = 0; j < GB02MAC288; j++) {
		mutex_init(&ss->queue[j].lock);

		ss->queue[j].fence_context = dma_fence_context_alloc(1);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
		ret = drm_sched_init(&ss->queue[j].sched,
				     &gb_sched_ops, GB02MAC1952, GB02MAC289,
				     msecs_to_jiffies(job_timeout_ms), "genbu");
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
		ret = drm_sched_init(&ss->queue[j].sched,
				     &gb_sched_ops, GB02MAC1952, GB02MAC289,
				     msecs_to_jiffies(job_timeout_ms),
				     NULL, NULL, "genbu");
#else
		ret = drm_sched_init(&ss->queue[j].sched,
				&gb_sched_ops, GB02MAC1952, GB02MAC289,
				msecs_to_jiffies(job_timeout_ms),
				NULL, NULL, "genbu", gbdev->dev);
#endif
		if (ret) {
			dev_err(gbdev->dev, "Failed to create scheduler: %d.", ret);
			goto err_sched;
		}
	}
	GB02FUNC1305(gbdev);
	return 0;

err_sched:
	for (j--; j >= 0; j--)
		drm_sched_fini(&ss->queue[j].sched);

	return ret;
}

void GB02FUNC1359(struct GB02STR39 *gbdev)
{
	struct GB02STR165 *ss = gbdev->ss;
	int j;

	stage_write(gbdev, GB02MAC1500, 0);

	for (j = 0; j < GB02MAC288; j++) {
		drm_sched_fini(&ss->queue[j].sched);
		mutex_destroy(&ss->queue[j].lock);
	}
	flush_workqueue(gbdev->reset.reset_workq);
	destroy_workqueue(gbdev->reset.reset_workq);
	devm_kfree(gbdev->dev, gbdev->ss);
}

int GB02FUNC1363(struct GB02STR47 *gb_priv)
{
	struct GB02STR39 *gbdev = gb_priv->gbdev;
	struct GB02STR165 *ss = gbdev->ss;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
	struct drm_sched_rq *rq;
#else
	struct drm_gpu_scheduler *sched;
#endif
	int ret = 0, i;

	for (i = 0; i < GB02MAC288; i++) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
		rq = &ss->queue[i].sched.sched_rq[DRM_SCHED_PRIORITY_NORMAL];
		ret = drm_sched_entity_init(&gb_priv->sched_entity[i], &rq, 1, NULL);
#else
		sched = &ss->queue[i].sched;
		ret =  drm_sched_entity_init(&gb_priv->sched_entity[i],
				DRM_SCHED_PRIORITY_NORMAL, &sched,
				1, NULL);
#endif
		if (WARN_ON(ret))
			return ret;
	}

	return 0;
}

void GB02FUNC1365(struct GB02STR47 *gb_priv)
{
	int i;
	struct GB02STR39 *gbdev = gb_priv->gbdev;

	mutex_lock(&gbdev->gb_file_priv_lock);
	for (i = 0; i < GB02MAC289; i++)
		if (gb_priv == gbdev->priv[i]) {
			gbdev->priv[i] = NULL;
			break;
		}
	mutex_unlock(&gbdev->gb_file_priv_lock);

	for (i = 0; i < GB02MAC288; i++) {
		struct drm_sched_entity *entity = &gb_priv->sched_entity[i];
		if (entity && entity->rq) {
			drm_sched_entity_destroy(entity);
		}
	}
}

int GB02FUNC1368(struct GB02STR39 *gbdev)
{
	struct GB02STR165 *ss = gbdev->ss;
	int i, try_time = GB02MAC288;

try_again:
	for (i = 0; i < GB02MAC288; i++) {
		/* If there are any stages in the HW queue, we're not idle */
		if (atomic_read(&ss->queue[i].sched.hw_rq_count)) {
			mdelay(100);

			if (try_time) {
				try_time--;
				goto try_again;
			}
			else
				return false;
		}
	}

	return true;
}
