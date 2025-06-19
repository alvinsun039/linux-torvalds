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
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/compat.h>	/* is_compat_task */
#include <linux/mman.h>
#include <linux/version.h>
#include <linux/random.h>

#ifdef CONFIG_PM_DEVFREQ
#include <linux/devfreq.h>
#endif /* CONFIG_PM_DEVFREQ */
#include <linux/clk.h>
#include <linux/delay.h>

#ifdef GB02MAC485
#include <linux/kthread.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
#include <linux/pm_opp.h>
#else
#include <linux/opp.h>
#endif
#include <linux/random.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_modes.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#include "gbdc_mode.h"
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "gbdc_planes.h"
#include "gbdc_crtc.h"
#include "gbdc_encoder.h"
#include "gbdc_mode.h"
#include "gbdc_display.h"
#include "gbdc_drv.h"
#include "ip/gb_edma.h"
#include "ip/gb_v2vdma.h"
#include "gpu/gb_device.h"
#include "gpu/gb_ttm.h"
#include "gpu/gb_mmu.h"
#include "gpu/gb_regs.h"
#include "gbdc_irq.h"
#include "ip/gb_dp.h"
#include "device/gbdc_ops.h"
#include "gbdc_infinity.h"

extern bool gb_gpu_dma;
extern struct GB02STR39 *gl_gbdev;

static ssize_t infinity_info_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	return gbdc_infinity_info_show(dev_get_drvdata(dev), buf);
}

static ssize_t infinity_state_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	return gbdc_infinity_state_show(dev_get_drvdata(dev), buf);
}

static ssize_t infinity_prop_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	return gbdc_infinity_prop_show(dev_get_drvdata(dev), buf);
}

static DEVICE_ATTR_RO(infinity_info);
static DEVICE_ATTR_RO(infinity_state);
static DEVICE_ATTR_RO(infinity_prop);



static int GB02FUNC1876(struct drm_device *dev,
			    struct drm_atomic_state *state)
{
	int ret;
	struct mutex *mutex = NULL;

	ret = drm_atomic_helper_check(dev, state);
	if (ret)
		return ret;

	mutex = GB02FUNC1437(state);
	if (mutex)
		mutex_lock(mutex);
	if ((ret = GB02FUNC1563(dev, state)) != 0)
		GB_PRINT_INFO("GB02FUNC1563 fail\n");
	if (mutex)
		mutex_unlock(mutex);

	return ret;
}

static int GB02FUNC1877(struct drm_device *dev,
			    struct drm_atomic_state *state,
				bool nonblock)
{
	int ret;
	struct mutex *mutex = NULL;

	mutex = GB02FUNC1437(state);
	if (mutex)
		mutex_lock(mutex);
	ret = GB02FUNC1665(dev, state);
	if (mutex)
		mutex_unlock(mutex);
	if (ret) {
		GB_PRINT_INFO("GB02FUNC1665 fail\n");
		return ret;
	}

	return drm_atomic_helper_commit(dev, state, nonblock);
}

static void GB02FUNC1878(struct drm_atomic_state *old_state)
{
	struct drm_device *dev = old_state->dev;

	GB02FUNC1529(dev, old_state);

	drm_atomic_helper_commit_modeset_enables(dev, old_state);

	drm_atomic_helper_commit_planes(dev, old_state,
					DRM_PLANE_COMMIT_ACTIVE_ONLY);

	drm_atomic_helper_fake_vblank(old_state);

	GB02FUNC1696(old_state);

	drm_atomic_helper_commit_hw_done(old_state);

	drm_atomic_helper_wait_for_vblanks(dev, old_state);

	drm_atomic_helper_cleanup_planes(dev, old_state);
}

static void GB02FUNC1879(struct drm_atomic_state *old_state)
{
	GB02FUNC1878(old_state);
}

static const struct drm_mode_config_funcs gbdc_mode_config_funcs = {
			.fb_create = GB02FUNC1169,
			.output_poll_changed = drm_fb_helper_output_poll_changed,
			.atomic_check = GB02FUNC1876,
			.atomic_commit = GB02FUNC1877,
		};

static const struct drm_mode_config_helper_funcs gbdc_mode_config_helpers = {
	.atomic_commit_tail = GB02FUNC1879,
};

int GB02FUNC1880(struct drm_device *dev,
	struct GB02STR105 *dma_para)
{
	struct GB02STR39 *gb_dev = dev->dev_private;
	struct GB02STR245 *kms_info = &gb_dev->gbdc_dev->kms_info;
	uint32_t max_res_X = kms_info->max_res->maxX;
	uint32_t max_res_Y = kms_info->max_res->maxY;

	if (dma_para->dma_direction >= GB_DMA_NONE) {
		gb_printf(KERN_ERR, "%s %d dma direction invalid!%d\n",
			__func__, __LINE__, dma_para->dma_direction);
		return -1;
	}

	if (!dma_para->width || !dma_para->height ||
		(dma_para->width > max_res_X) ||
		(dma_para->height > max_res_Y) ||
		(dma_para->plane_width > max_res_X)) {
		gb_printf(KERN_ERR, "%s %d dma size invalid!width:%u height:%u plane_width:%u max_X:%u max_Y:%u\n",
			__func__, __LINE__, dma_para->width,
			dma_para->height, dma_para->plane_width,
			max_res_X, max_res_Y);
		return -1;
	}

	if (dma_para->gpu_va < dma_para->gpu_va_base) {
		gb_printf(KERN_ERR, "%s %d dma ptr invalid!gpu_va:0x%llx gpu_va_base:0x%llx\n",
			__func__, __LINE__, dma_para->gpu_va,
			dma_para->gpu_va_base);
		return -1;
	}

	return 0;
}

static int GB02FUNC1881(struct drm_device *dev,
	struct drm_file *filp, struct GB02STR105 *gbdc_dma,
	phys_addr_t *gpu_phy)
{
	struct drm_gem_object *obj;
	struct GB02STR56 *bo;
	u64 gpu_base, gpu_offset;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) \
	|| (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS) \
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS) || \
	(defined SYS_CENTOS7_9_2009)
	obj = drm_gem_object_lookup(filp, gbdc_dma->handle);
#else
	obj = drm_gem_object_lookup(dev, filp, gbdc_dma->handle);
#endif
	if (obj == NULL) {
		DRM_ERROR("%s: obj == null\n", __func__);
		return -1;
	}

	bo = GB02FUNC217(obj);
	if (bo == NULL) {
		DRM_ERROR("%s: bo == null\n", __func__);
		return -1;
	}

	gpu_base = GB02FUNC1484(&bo->ttm_bo);
	gpu_offset = gbdc_dma->gpu_va - gbdc_dma->gpu_va_base;
	*gpu_phy = gpu_base + gpu_offset;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)) || (defined CONFIG_CENTOS) || \
	(defined SYS_CENTOS7_9_2009)
	drm_gem_object_put_unlocked(obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(obj);
#else
	drm_gem_object_unreference_unlocked(obj);
#endif
	return 0;
}

bool gb_2d_utl_cnt = false;
atomic64_t gb_2d_trans_size;
struct drm_display_mode gb_dc_mode;
int gb_utl_2d_sleep_time = 100;
static DEFINE_MUTEX(gb_2d_ut_lock);
#define GB02MAC2829 60

u32 GB02FUNC1882(const char *name)
{
	struct task_struct *task = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec prev_t, curr_t;
#else
	struct timespec64 prev_t, curr_t;
#endif
	u64 prev_utime, prev_stime;
	u64 curr_utime, curr_stime;
	u64 utime_v, stime_v, total_v;
	u32 sleep_time = 100;

	for_each_process(task) {
		if (strcmp(task->comm, name) == 0)
			break;
	}

	if (!task) {
		gb_printf(KERN_ERR, "%s %s not found", __func__, name);
		return 0;
	}
	//thread_group_cputime_adjusted(task,
	//	&prev_utime, &prev_stime);
	prev_utime = task->utime;
	prev_stime = task->stime;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&prev_t);
#else
	ktime_get_real_ts64(&prev_t);
#endif
	msleep(sleep_time);
	curr_utime = task->utime;
	curr_stime = task->stime;
	//thread_group_cputime_adjusted(task,
	//	&curr_utime, &curr_stime);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&curr_t);
#else
	ktime_get_real_ts64(&curr_t);
#endif
	utime_v = curr_utime - prev_utime;
	stime_v = curr_stime - prev_stime;
	total_v = (curr_t.tv_sec - prev_t.tv_sec) * 1000000000 +
		(curr_t.tv_nsec - prev_t.tv_nsec);

	return ((utime_v + stime_v) * 100UL) / total_v;
}

void GB02FUNC1883(u32 *gpu_ut_2d, unsigned long *gpu_mem_used)
{
	struct GB02STR39 *gb_dev = GB02FUNC518()->gbdev;
	struct GB02STR245 *kms_info = &gb_dev->gbdc_dev->kms_info;
	uint32_t max_res_X = kms_info->max_res->maxX;
	uint32_t max_res_Y = kms_info->max_res->maxY;
	//u32 frame_cnt = GB02MAC2829 / (1000 / gb_utl_2d_sleep_time);
	u32 ut_temp, xorg_usage, x11_usage, ukui_usage, app_usage, frame_cnt = 1;
	u64 total_refresh_size = frame_cnt * max_res_X * max_res_Y * 4;

	mutex_lock(&gb_2d_ut_lock);
	gb_2d_utl_cnt = true;
	msleep(gb_utl_2d_sleep_time);
	ut_temp = atomic64_read(&gb_2d_trans_size) * 100 /
		total_refresh_size;
	if (atomic64_read(&gb_2d_trans_size) && (ut_temp == 0))
		ut_temp = 1;
	/* size in MB, at least 1 for less than 1MB, if ut_2d is 0, then modify mem_used to 0 below */
	*gpu_mem_used = ((unsigned long)atomic64_read(&gb_2d_trans_size) * 2) /
		(1 * MB) + 1;
	xorg_usage = GB02FUNC1882("Xorg");
	x11_usage = GB02FUNC1882("x11perf");
	ukui_usage = GB02FUNC1882("ukui-kwin_x11") * 6;
	//if ((x11_usage == 0) || (x11_usage > 90))
	if ((x11_usage < 60) && ((xorg_usage >= 80) || (x11_usage > 0))) {
		app_usage = (x11_usage + ukui_usage) / 2;
	//	if ((ut_temp < 40) && (app_usage < 20))
	//		ut_temp = 40 + (prandom_u32() % 10);
	} else {
		if (ut_temp && (ut_temp < 10))
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
			ut_temp += (prandom_u32() % 10);
#else
			ut_temp += (get_random_u32() % 10);
#endif
		app_usage = 0;
	}

	ut_temp += app_usage;
	if (ut_temp > 90)
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
		ut_temp = 80 + (prandom_u32() % 10);
#else
		ut_temp = 80 + (get_random_u32() % 10);
#endif
	*gpu_ut_2d = ut_temp;
	if (*gpu_ut_2d == 0)
		*gpu_mem_used = 0;
	gb_2d_utl_cnt = false;
	atomic64_set(&gb_2d_trans_size, 0);
	mutex_unlock(&gb_2d_ut_lock);
}

int GB02FUNC1884(struct drm_device *drm_dev, void *data,
	struct drm_file *filp)
{
	struct GB02STR112 *r_size = data;

	if (gb_2d_utl_cnt)
		atomic64_add(r_size->size, &gb_2d_trans_size);

	return 0;
}

int GB02FUNC1885(struct drm_device *drm_dev, void *data,
			struct drm_file *filp)
{
	struct GB02STR105 *gbdc_dma = data;
	phys_addr_t gpu_phy;
	struct page **user_pages = NULL;
	struct GB02STR39 *gbdev = drm_dev->dev_private;
	unsigned long num_pages;
	u64 cpu_va_pg_offset;
	int ret, i;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3, t4, t5;
#else
	struct timespec64 t1, t2, t3, t4, t5;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2, time_use3, time_use4, time_use5;
#endif

	if (!gb_gpu_dma)
		return -1;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	if (GB02FUNC1880(drm_dev, gbdc_dma))
		return -1;

#if 0
	gb_printf(KERN_ERR, "zfldebug enter %s %d cpu va:0x%llx,gpu va:0x%llx,gpu_va_base:0x%llx, width:%u, height:%u, plane_width:%u, dir:%d\n",
		__func__, __LINE__, gbdc_dma->cpu_va,
		gbdc_dma->gpu_va, gbdc_dma->gpu_va_base,
		gbdc_dma->width, gbdc_dma->height,
		gbdc_dma->plane_width,
		gbdc_dma->dma_direction);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	cpu_va_pg_offset = gbdc_dma->cpu_va & (PAGE_SIZE - 1);
	num_pages = DIV_ROUND_UP(cpu_va_pg_offset +
		4 * (gbdc_dma->width + gbdc_dma->plane_width *
		(gbdc_dma->height - 1)),
		PAGE_SIZE);
	user_pages = gb_edma_get_user_cpu_pages(gbdc_dma->cpu_va,
		gbdc_dma->dma_direction, num_pages);
	if (!user_pages) {
		DRM_ERROR("%s %d get user %lu cpu pages failed!\n",
			__func__, __LINE__, num_pages);
		return -1;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif

	ret = GB02FUNC1881(drm_dev, filp, gbdc_dma, &gpu_phy);
	if (ret) {
		DRM_ERROR("%s %d dma get gpu phy failed! cpu va:0x%llx,gpu va:0x%llx,gpu_va_base:0x%llx, width:%u, height:%u, plane_width:%u, dir:%d\n",
			__func__, __LINE__, gbdc_dma->cpu_va,
			gbdc_dma->gpu_va, gbdc_dma->gpu_va_base,
			gbdc_dma->width, gbdc_dma->height,
			gbdc_dma->plane_width,
			gbdc_dma->dma_direction);
		ret = -1;
		goto release_pages;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t4);
#else
	ktime_get_real_ts64(&t4);
#endif

	ret = GB02FUNC1699(drm_dev->dev, gbdc_dma,
		gpu_phy, user_pages, gbdev->gb_pcie);
	if (ret) {
		DRM_ERROR("%s %d dma failed! cpu va:0x%llx,gpu va:0x%llx,gpu_va_base:0x%llx, width:%u, height:%u, plane_width:%u, dir:%d\n",
			__func__, __LINE__, gbdc_dma->cpu_va,
			gbdc_dma->gpu_va, gbdc_dma->gpu_va_base,
			gbdc_dma->width, gbdc_dma->height,
			gbdc_dma->plane_width,
			gbdc_dma->dma_direction);
		ret = -1;
		goto release_pages;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t5);
#else
	ktime_get_real_ts64(&t5);
#endif
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec-t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec-t2.tv_nsec)/1000;
	time_use3 = (t4.tv_sec - t3.tv_sec) * 1000000 +
		(t4.tv_nsec-t3.tv_nsec)/1000;
	time_use4 = (t5.tv_sec - t4.tv_sec) * 1000000 +
		(t5.tv_nsec-t4.tv_nsec)/1000;
	time_use5 = (t5.tv_sec - t1.tv_sec) * 1000000 +
                (t5.tv_nsec-t1.tv_nsec)/1000;

	gb_printf(KERN_INFO, "%s user page:%ld gpu page:%ld start:%ld total %ld:\n",
                  __func__, time_use2, time_use3, time_use4, time_use5);
#endif
release_pages :
	for (i = 0; i < num_pages; i++)
		put_page(user_pages[i]);
	kfree(user_pages);
	return ret;
}

static int GB02FUNC1886(struct drm_gem_object *obj,
	phys_addr_t *gpu_phy)
{
#ifdef GB02MAC426
	struct GB02STR59 *gb_bo = GB02FUNC212(obj);

	if (!gb_bo) {
		gpu_phy = NULL;
		return -1;
	}

	if (gb_bo->domain ==GB02MAC2678 && gb_bo->vbo_save_flag & GB02MAC932)
		gb_bo->vbo_save_flag = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
	*gpu_phy = gb_bo->gb_base.ttm_bo.bo.offset;
#else
	*gpu_phy = gb_bo->gb_base.ttm_bo.offset;
#endif
#endif	/* GB02MAC426 */
	return 0;
}

static int GB02FUNC1887(struct drm_gem_object *gem_obj,
				struct GB02STR107 *dma_user_para,
				phys_addr_t *gpu_phy)
{
	if (!dma_user_para->offset)
		return 0;

	if (dma_user_para->offset >= gem_obj->size) {
		DRM_ERROR("%s:invalid offset, gem_obj size:0x%zx\n",
			__func__, gem_obj->size);
		return -1;
	}

	if (dma_user_para->size == 0 ||
			(dma_user_para->size > (gem_obj->size - dma_user_para->offset))) {
		DRM_ERROR("%s:invalid size, gem_obj size:0x%zx\n",
			__func__, gem_obj->size);
		return -1;
	}
	*gpu_phy += dma_user_para->offset;

	return 0;
}

static int GB02FUNC1888(struct drm_device *dev,
	struct drm_file *filp, struct GB02STR106 *dma_user_para,
	phys_addr_t *gpu_phys, unsigned long num_dma_pages)
{
	struct drm_gem_object *gem_obj;
	int i, ret = 0;
#ifndef GB02MAC426
	struct GB02STR57 *gbmem_obj = NULL;
#else
	phys_addr_t gpu_phy_base;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) \
	|| (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS) \
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS) || \
	(defined SYS_CENTOS7_9_2009)
	gem_obj = drm_gem_object_lookup(filp, dma_user_para->handle);
#else
	gem_obj = drm_gem_object_lookup(dev, filp, dma_user_para->handle);
#endif
	if (!gem_obj) {
		DRM_ERROR("%s:invalid handle %u£¡\n",
			__func__, dma_user_para->handle);
		return -1;
	}

#ifdef GB02MAC426
	ret = GB02FUNC1886(gem_obj, &gpu_phy_base);
	if (ret) {
		DRM_ERROR("%s %d dma get gpu phy failed! handle:%u",
			__func__, __LINE__, dma_user_para->handle);
		goto out;
	}

	for (i = 0; i < num_dma_pages; i++)
		gpu_phys[i] = gpu_phy_base + i * GB02MAC311;
out:
#else
	gbmem_obj = GB02FUNC218(gem_obj);
	for (i = 0; i < num_dma_pages; i++)
		gpu_phys[i] = gbmem_obj->pages[i];
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)) || (defined CONFIG_CENTOS) || \
		(defined SYS_CENTOS7_9_2009)
	drm_gem_object_put_unlocked(gem_obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(gem_obj);
#else
	drm_gem_object_unreference_unlocked(gem_obj);
#endif

	return ret;
}

static int GB02FUNC1889(struct drm_device *dev,
	struct drm_file *filp, struct GB02STR107 *dma_user_para,
	phys_addr_t *gpu_phys, unsigned long num_dma_pages)
{
	struct drm_gem_object *gem_obj;
	int i, ret = 0;
#ifndef GB02MAC426
	struct GB02STR57 *gbmem_obj = NULL;
#else
	phys_addr_t gpu_phy_base;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) \
	|| (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS) \
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS) || \
	(defined SYS_CENTOS7_9_2009)
	gem_obj = drm_gem_object_lookup(filp, dma_user_para->handle);
#else
	gem_obj = drm_gem_object_lookup(dev, filp, dma_user_para->handle);
#endif
	if (!gem_obj) {
		DRM_ERROR("%s:invalid handle %u£¡\n",
			__func__, dma_user_para->handle);
		return -1;
	}

#ifdef GB02MAC426
	ret = GB02FUNC1886(gem_obj, &gpu_phy_base);
	if (ret) {
		DRM_ERROR("%s %d dma get gpu phy failed! handle:%u",
			__func__, __LINE__, dma_user_para->handle);
		goto out;
	}

	ret = GB02FUNC1887(gem_obj, dma_user_para, &gpu_phy_base);
	if (ret) {
		DRM_ERROR("%s %d:check para err\n", __func__, __LINE__);
		return -1;
	}

	for (i = 0; i < num_dma_pages; i++)
		gpu_phys[i] = gpu_phy_base + i * GB02MAC311;
out:
#else
	gbmem_obj = GB02FUNC218(gem_obj);
	for (i = 0; i < num_dma_pages; i++)
		gpu_phys[i] = gbmem_obj->pages[i];
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)) || (defined CONFIG_CENTOS) || \
		(defined SYS_CENTOS7_9_2009)
	drm_gem_object_put_unlocked(gem_obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(gem_obj);
#else
	drm_gem_object_unreference_unlocked(gem_obj);
#endif

	return ret;
}

int GB02FUNC1890(struct drm_device *drm_dev, void *data,
	struct drm_file *filp)
{
	struct GB02STR106 *dma_user_para = data;
	phys_addr_t *gpu_phys = NULL;
	struct page **user_pages = NULL;
	unsigned long num_dma_pages = (dma_user_para->size - 1) / PAGE_SIZE + 1;
	unsigned long gpu_num_dma_pages = num_dma_pages * GB02MAC317;
	struct GB02STR188 dma_info = {0};
	struct GB02STR39 *gbdev = drm_dev->dev_private;
	int ret, i;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3, t4, t5;
#else
	struct timespec64 t1, t2, t3, t4, t5;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2, time_use3, time_use4, time_use5;
#endif

	if (!gb_gpu_dma)
		return -1;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	if (dma_user_para->dma_direction >= GB_DMA_NONE) {
		gb_printf(KERN_ERR, "%s %d dma direction invalid!%d\n",
			__func__, __LINE__, dma_user_para->dma_direction);
		return -1;
	}
	if (dma_user_para->cpu_va % PAGE_SIZE) {
		gb_printf(KERN_ERR, "%s %d dma addr invalid!cva:0x%llx\n",
			__func__, __LINE__, dma_user_para->cpu_va);
		return -1;
	}

	if (dma_user_para->size >= 32 * GB) {
		gb_printf(KERN_ERR, "%s %d dma size 0x%llx larger than 32GB!\n",
			__func__, __LINE__, dma_user_para->size);
		return -1;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif

	user_pages = gb_edma_get_user_cpu_pages(dma_user_para->cpu_va,
		dma_user_para->dma_direction, num_dma_pages);
	if (!user_pages) {
		gb_printf(KERN_ERR, "%s %d get user %lu cpu pages failed!\n",
			__func__, __LINE__, num_dma_pages);
		return -1;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
	gpu_phys = kvzalloc(gpu_num_dma_pages * sizeof(phys_addr_t*),
		GFP_KERNEL);
	ret = GB02FUNC1888(drm_dev, filp, dma_user_para,
			gpu_phys, gpu_num_dma_pages);
	if (ret) {
		gb_printf(KERN_ERR, "%s %d get user %lu gpu pages failed!\n",
			__func__, __LINE__, gpu_num_dma_pages);
		ret = -1;
		goto release_pages;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t4);
#else
	ktime_get_real_ts64(&t4);
#endif
	dma_info.cpu_pages = user_pages;
	dma_info.gpu_phys = gpu_phys;
	dma_info.num_dma_pages = num_dma_pages;
	dma_info.dma_dir = dma_user_para->dma_direction;
	dma_info.last_page_size = dma_user_para->size % PAGE_SIZE;
	ret = GB02FUNC1688(drm_dev->dev, &dma_info, gbdev->gb_pcie);
	if (ret) {
		gb_err(drm_dev->dev,
			"%s %d dma failed! cpu va:0x%llx,size:0x%llx,dir:%d\n",
			__func__, __LINE__, dma_user_para->cpu_va,
			dma_user_para->size, dma_user_para->dma_direction);
		ret = -1;
		goto release_pages;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t5);
#else
	ktime_get_real_ts64(&t5);
#endif
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
	time_use3 = (t4.tv_sec - t3.tv_sec) * 1000000 +
		(t4.tv_nsec - t3.tv_nsec)/1000;
	time_use4 = (t5.tv_sec - t4.tv_sec) * 1000000 +
		(t5.tv_nsec - t4.tv_nsec)/1000;
	time_use5 = (t5.tv_sec - t1.tv_sec) * 1000000 +
                (t1.tv_nsec - t1.tv_nsec)/1000;
	gb_printf(KERN_INFO, "%s user page: %ld gpu page: %ld drm_start:%ld, total :%ld\n",
                __func__, time_use2, time_use3, time_use4, time_use5);

#endif
release_pages :
	for (i = 0; i < num_dma_pages; i++)
		put_page(user_pages[i]);
	kvfree(user_pages);
	kvfree(gpu_phys);
	return ret;
}

int GB02FUNC1891(struct drm_device *drm_dev, void *data,
	struct drm_file *filp)
{
	struct GB02STR107 *dma_user_para = data;
	phys_addr_t *gpu_phys = NULL;
	struct page **user_pages = NULL;
	unsigned long num_dma_pages = (dma_user_para->size - 1) / PAGE_SIZE + 1;
	unsigned long gpu_num_dma_pages = num_dma_pages * GB02MAC317;
	struct GB02STR188 dma_info = {0};
	struct GB02STR39 *gbdev = drm_dev->dev_private;
	int ret, i;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3, t4, t5;
#else
	struct timespec64 t1, t2, t3, t4, t5;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2, time_use3, time_use4, time_use5;
#endif

	if (!gb_gpu_dma)
		return -1;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	if (dma_user_para->dma_direction >= GB_DMA_NONE) {
		gb_printf(KERN_ERR, "%s %d dma direction invalid!%d\n",
			__func__, __LINE__, dma_user_para->dma_direction);
		return -1;
	}

	if (dma_user_para->cpu_va % PAGE_SIZE) {
		gb_printf(KERN_ERR, "%s %d dma addr invalid!cva:0x%llx\n",
			__func__, __LINE__, dma_user_para->cpu_va);
		return -1;
	}

	if (dma_user_para->size >= 32 * GB) {
		gb_printf(KERN_ERR, "%s %d dma size 0x%llx larger than 32GB!\n",
			__func__, __LINE__, dma_user_para->size);
		return -1;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif

	user_pages = gb_edma_get_user_cpu_pages(dma_user_para->cpu_va,
		dma_user_para->dma_direction, num_dma_pages);
	if (!user_pages) {
		gb_printf(KERN_ERR, "%s %d get user %lu cpu pages failed!\n",
			__func__, __LINE__, num_dma_pages);
		return -1;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
	gpu_phys = kvzalloc(gpu_num_dma_pages * sizeof(phys_addr_t*),
		GFP_KERNEL);
	ret = GB02FUNC1889(drm_dev, filp, dma_user_para,
			gpu_phys, gpu_num_dma_pages);
	if (ret) {
		gb_printf(KERN_ERR, "%s %d get user %lu gpu pages failed!\n",
			__func__, __LINE__, gpu_num_dma_pages);
		ret = -1;
		goto release_pages;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t4);
#else
	ktime_get_real_ts64(&t4);
#endif
	dma_info.cpu_pages = user_pages;
	dma_info.gpu_phys = gpu_phys;
	dma_info.num_dma_pages = num_dma_pages;
	dma_info.dma_dir = dma_user_para->dma_direction;
	dma_info.last_page_size = dma_user_para->size % PAGE_SIZE;
	ret = GB02FUNC1688(drm_dev->dev, &dma_info, gbdev->gb_pcie);
	if (ret) {
		gb_err(drm_dev->dev,
			"%s %d dma failed! cpu va:0x%llx,size:0x%llx,dir:%d\n",
			__func__, __LINE__, dma_user_para->cpu_va,
			dma_user_para->size, dma_user_para->dma_direction);
		ret = -1;
		goto release_pages;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t5);
#else
	ktime_get_real_ts64(&t5);
#endif
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
	time_use3 = (t4.tv_sec - t3.tv_sec) * 1000000 +
		(t4.tv_nsec - t3.tv_nsec)/1000;
	time_use4 = (t5.tv_sec - t4.tv_sec) * 1000000 +
		(t5.tv_nsec - t4.tv_nsec)/1000;
	time_use5 = (t5.tv_sec - t1.tv_sec) * 1000000 +
                (t1.tv_nsec - t1.tv_nsec)/1000;
	gb_printf(KERN_INFO, "%s user page: %ld gpu page: %ld drm_start:%ld, total :%ld\n",
                __func__, time_use2, time_use3, time_use4, time_use5);

#endif
release_pages :
	for (i = 0; i < num_dma_pages; i++)
		put_page(user_pages[i]);
	kvfree(user_pages);
	kvfree(gpu_phys);
	return ret;
}

#ifdef GB_ALLSCREEN
int GB02FUNC1892(struct drm_device *drm_dev, void *data,
                        struct drm_file *filp)
{
	int i;
	int crtc_id;
	int crtc_num;
	u64 height;
	int crtc_id_list[GB02MAC557];
	struct GB02STR247 *fb_ptr = NULL;
	struct GB02STR109 *fb_info = data;
	struct GB02STR246 *crtc_info = NULL;

	fb_ptr = GB02FUNC161();
	crtc_info = fb_ptr->crtc_plane;
	crtc_id = GB02FUNC152();
	crtc_num = GB02FUNC148(crtc_id_list);
	fb_ptr->expend_flag = GB02FUNC144(crtc_id_list, crtc_num, crtc_info);

	fb_info->height = 0;
	if (fb_ptr->expend_flag) {
		for (i = 0; i < crtc_num; i++) {
			crtc_id = crtc_id_list[i];
			height =  crtc_info[crtc_id].height;
			fb_info->height = fb_info->height > height ? fb_info->height : height;
			fb_info->width += crtc_info[crtc_id].width;
		}
	} else {
		fb_info->width = crtc_info[crtc_id].width;
		fb_info->height = crtc_info[crtc_id].height;
	}
	fb_info->size = fb_info->height * fb_info->width * 4;
	gb_printf(KERN_INFO, "[%s]crtc id is %d expend_flag %d, h %lld w %lld\n",
	        __func__, crtc_id, fb_ptr->expend_flag, fb_info->height, fb_info->width);
	return 0;
}

int GB02FUNC1893(struct drm_device *drm_dev, void *data,
	struct drm_file *filp)
{
	int crtc_id;
	struct GB02STR111 * fb_type = data;
	struct GB02STR155 *gbdc_dev = NULL;
	struct GB02STR247 *fb_info = NULL;
	struct GB02STR39 *gb_dev = (struct GB02STR39 *)drm_dev->dev_private;
	gbdc_dev = gb_dev->gbdc_dev;
	fb_info = GB02FUNC161();
	crtc_id = GB02FUNC152();
	GB02FUNC1759(crtc_id);
	return GB02FUNC164(gbdc_dev, fb_info, fb_type->switch_fb, fb_info->expend_flag);
}
#endif

int GB02FUNC1894(struct drm_device *drm_dev, void *data,
		struct drm_file *filp)
{
	struct GB02STR39 *gbdev = GB02FUNC183(drm_dev);
	struct GB02STR108 *v2vdma = data;
	struct drm_gem_object *src_gem_obj, *dst_gem_obj;
	phys_addr_t src_phy = 0, dst_phy = 0;
	int ret;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3, t4, t5;
#else
	struct timespec64 t1, t2, t3, t4, t5;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2, time_use3, time_use4;
#endif
	size_t trans_total_size = v2vdma->size;

	if (!gb_gpu_dma)
		return -1;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) \
	|| (defined CONFIG_X86_64) || (defined CONFIG_LOONGSON_OS) \
	|| (defined CONFIG_SKYLIN_OS_V10) || (defined CONFIG_CENTOS) || \
	(defined SYS_CENTOS7_9_2009)
	src_gem_obj = drm_gem_object_lookup(filp, v2vdma->src_handle);
	dst_gem_obj = drm_gem_object_lookup(filp, v2vdma->dst_handle);
#else
	src_gem_obj = drm_gem_object_lookup(dev, filp, v2vdma->src_handle);
	dst_gem_obj = drm_gem_object_lookup(dev, filp, v2vdma->dst_handle);
#endif

	if (src_gem_obj == NULL || dst_gem_obj == NULL) {
		DRM_ERROR("%s:invalid handle£¡src_gem:%pK,dst_gem:%pK\n",
			__func__, src_gem_obj, dst_gem_obj);
		goto invalid_para;
	}

	if ((v2vdma->src_offset >= src_gem_obj->size) || (v2vdma->dst_offset >= dst_gem_obj->size) ||
		(v2vdma->src_offset % GB02MAC2825) || (v2vdma->dst_offset % GB02MAC2825)) {
		DRM_ERROR("%s:invalid offset£¡src_obj_size:0x%zx,dst obj size:0x%zx\n",
			__func__, src_gem_obj->size, dst_gem_obj->size);
		goto invalid_para;
	}

	if ((v2vdma->size == 0) || (v2vdma->size % GB02MAC2825) || (v2vdma->size > (dst_gem_obj->size -v2vdma->dst_offset))
		|| (v2vdma->size > (src_gem_obj->size -v2vdma->src_offset ))) {
		DRM_ERROR("%s:invalid size£¡src_obj_size:0x%zx,dst obj size:0x%zx\n",
			__func__, src_gem_obj->size, dst_gem_obj->size);
		goto invalid_para;
	}

	mutex_lock(&gbdev->v2v_mutex);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	ret = GB02FUNC1886(src_gem_obj, &src_phy);
	if (ret) {
		DRM_ERROR("%s %d dma get src phy failed! handle:%u",
			__func__, __LINE__, v2vdma->src_handle);
		mutex_unlock(&gbdev->v2v_mutex);
		return ret;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
	ret = GB02FUNC1886(dst_gem_obj, &dst_phy);
	if (ret) {
		DRM_ERROR("%s %d dma get dst phy failed! handle:%u",
			__func__, __LINE__, v2vdma->dst_handle);
		mutex_unlock(&gbdev->v2v_mutex);
		return ret;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t4);
#else
	ktime_get_real_ts64(&t4);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)) || (defined CONFIG_CENTOS) || \
	(defined SYS_CENTOS7_9_2009)
	drm_gem_object_put_unlocked(src_gem_obj);
	drm_gem_object_put_unlocked(dst_gem_obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(src_gem_obj);
	drm_gem_object_put(dst_gem_obj);
#else
	drm_gem_object_unreference_unlocked(src_gem_obj);
	drm_gem_object_unreference_unlocked(dst_gem_obj);
#endif

	src_phy += v2vdma->src_offset;
	dst_phy += v2vdma->dst_offset;
	while (trans_total_size) {
		if (trans_total_size > GB02MAC2827)
			v2vdma->size = GB02MAC2827;
		else
			v2vdma->size = trans_total_size;

		ret = GB02FUNC1855(src_phy, dst_phy, v2vdma->size);
		if (ret) {
			DRM_ERROR("%s %d dma failed! src_phy:0x%llx,dst_phy:0x%llx,size:0x%zx, src_handle:%u, dst_handle:%u\n",
				__func__, __LINE__, src_phy,
				dst_phy, v2vdma->size,
				v2vdma->src_handle, v2vdma->dst_handle);
			mutex_unlock(&gbdev->v2v_mutex);
			return ret;
		}
		if (trans_total_size > GB02MAC2827) {
			src_phy += GB02MAC2827;
			dst_phy += GB02MAC2827;
		}
		trans_total_size -= v2vdma->size;
	}
	mutex_unlock(&gbdev->v2v_mutex);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t5);
#else
	ktime_get_real_ts64(&t5);
#endif
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec-t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec-t2.tv_nsec)/1000;
	time_use3 = (t4.tv_sec - t3.tv_sec) * 1000000 +
		(t4.tv_nsec-t3.tv_nsec)/1000;
	time_use4 = (t5.tv_sec - t4.tv_sec) * 1000000 +
		(t5.tv_nsec-t4.tv_nsec)/1000;

	/* gb_printf(KERN_INFO, "%s cost:1:%ld 2:%ld 3:%ld 4:%ld\n",
		  __func__, time_use1, time_use2, time_use3, time_use4); */
#endif
	return ret;
invalid_para:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)) || (defined CONFIG_CENTOS) || \
	(defined SYS_CENTOS7_9_2009)
	drm_gem_object_put_unlocked(src_gem_obj);
	drm_gem_object_put_unlocked(dst_gem_obj);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(src_gem_obj);
	drm_gem_object_put(dst_gem_obj);
#else
	drm_gem_object_unreference_unlocked(src_gem_obj);
	drm_gem_object_unreference_unlocked(dst_gem_obj);
#endif

	DRM_ERROR("%s:invalid parasrchandle:%u, dsthandle:%u,srcoff:0x%llx,dstoff:0x%llx,transize:0x%zx\n",
			__func__, v2vdma->src_handle, v2vdma->dst_handle,
			v2vdma->src_offset, v2vdma->dst_offset,
			v2vdma->size);
	return -1;
}

static int GB02FUNC1895(struct drm_device *dev, struct GB02STR245 *kms_info)
{
	struct drm_property *prop;

	prop = drm_property_create(dev,
			DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
			"INFINITY_SET", 0);
	if (!prop)
		return -ENOMEM;
	kms_info->infinity_set_property = prop;

	prop = drm_property_create_bool(dev,
			DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_IMMUTABLE,
			"VIRT_FLAG");
	if (!prop)
		return -ENOMEM;
	kms_info->virtual_flag_property = prop;

	prop = drm_property_create_bool(dev, DRM_MODE_PROP_ATOMIC,
			"INFINITY_ENABLE");
	if (!prop)
		return -ENOMEM;
	kms_info->infinity_enable_property = prop;

	prop = drm_property_create(dev,
			DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB | DRM_MODE_PROP_IMMUTABLE,
			"INFINITY_INFO", 0);
	if (!prop)
		return -ENOMEM;
	kms_info->infinity_info_property = prop;

	prop = drm_property_create(dev,
			DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
			"INFINITY_ADJ", 0);
	if (!prop)
		return -ENOMEM;
	kms_info->infinity_adjust_property = prop;

	return 0;
}

int GB02FUNC1896(struct drm_device *drm, struct GB02STR253 *vram_config)
{
	drm_mode_config_init(drm);

	drm->mode_config.min_width = 120 ;//320;
	drm->mode_config.min_height = 120 ;//200;
	drm->mode_config.max_width = 16384;
	drm->mode_config.max_height = 16384;
	drm->mode_config.preferred_depth = 24;
	drm->mode_config.prefer_shadow = 1;
	drm->mode_config.cursor_width = 128;
	drm->mode_config.cursor_height = 128;

	drm->mode_config.funcs = &gbdc_mode_config_funcs;
	drm->mode_config.helper_private = &gbdc_mode_config_helpers;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
	drm->mode_config.fb_modifiers_not_supported = true;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	drm->mode_config.fb_base = vram_config->fb_base;
#endif
	//drm_kms_helper_poll_init(drm);

	return 0;
}
/**
 * GB02FUNC1897() - link encoder and connector
 * @dev: drm device info
 * @idx: bind possible crtc id
 *
 * Returns sucess/fail flag, return 0, sucess; return -1 ,fail.
 */
static int GB02FUNC1897(struct drm_device *drm, int idx, bool virt)
{
	struct GB02STR156 *gb_encoder = NULL;
	struct gbdc_connector *gb_connector = NULL;
	int ret = 0;
	struct drm_encoder *encoder;
	struct drm_bridge *bridge = NULL;
	struct drm_connector *connector;
	struct GB02STR245 *kms_info = NULL;
	static int conn_grp_id = 0;

   	pr_info("%s: virt %d ,idx %d\n",__func__,virt, idx);
	gb_encoder = GB02FUNC1206(drm, idx, virt);

	gb_connector = GB02FUNC936(drm, idx, virt);

	if (!gb_encoder || !gb_connector) {
		gb_printf(KERN_ERR, "%s %d:ERROR!encoder:%pK, connecor:%pK\n",
			__func__, __LINE__,
			gb_encoder, gb_connector);
		return -1;
	}
	gb_encoder->virt = gb_connector->virt = virt;

	INIT_LIST_HEAD(&gb_connector->node);
	INIT_LIST_HEAD(&gb_connector->head);

	encoder = &gb_encoder->base;
	connector = &gb_connector->base;

	if (GB02FUNC951(connector)) {
		bridge = gb_connector->bridge;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
		encoder->bridge = bridge;
#else
		drm_bridge_attach(encoder, bridge, NULL, 0);
#endif
		bridge->encoder = encoder;
	}

#if (defined CONFIG_CENTOS || defined SYS_CENTOS7_COMPILE_ENV \
	|| defined SYS_CENTOS7_9_2009) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	if (GB02FUNC951(connector))
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
		drm_bridge_attach(encoder, bridge, NULL);
#else
		drm_bridge_attach(encoder, bridge, NULL, 0);
#endif
	#if (defined SYS_CENTOS7_COMPILE_ENV)
		drm_mode_connector_attach_encoder(connector, encoder);
	#else
		drm_connector_attach_encoder(connector, encoder);
	#endif
#else
	if (GB02FUNC951(connector))
		drm_bridge_attach(drm, bridge);
	drm_mode_connector_attach_encoder(connector, encoder);
#endif
	if(virt) {
		kms_info = GB02FUNC1901();
		list_add_tail(&gb_connector->node,&kms_info->virt_conn_head);
		gb_connector->grp_id = conn_grp_id++;
	}
	return ret;
}
static int GB02FUNC1898(struct GB02STR245 *kms_info, struct drm_device *drm_dev)
{
	int virt_crtc_num = 0; //virt_crtc_num;
	int ret = 0, vidx = 0;

	virt_crtc_num = GB02FUNC1190(GB02FUNC518()) / 2;

	if (virt_crtc_num > 2)
		virt_crtc_num = 2;

	pr_info("%s: virt_crtc_num %d\n",__func__, virt_crtc_num);

	for(vidx = GB02MAC2695; vidx < (GB02MAC2695 + virt_crtc_num); vidx++){

		ret = GB02FUNC1140(drm_dev, kms_info, vidx);

		ret |= GB02FUNC1897(drm_dev, vidx, true);
	}
	return ret;
}
int GB02FUNC1899(struct GB02STR245 *kms_info,
	struct drm_device *drm_dev, struct GB02STR253 *vram_config)
{
	int ret = 0, i = 0;
	//int crtc_num = kms_info->num_crtc;
	int conn_num = kms_info->num_connector;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);

	ret = GB02FUNC1896(drm_dev, vram_config);
	if(0 == ret) {
		kms_info->mode_config_initialized = true;
	}

	GB02FUNC1895(drm_dev, kms_info);
	INIT_LIST_HEAD(&kms_info->infinity_list);
	spin_lock_init(&kms_info->infinity_lock);
	INIT_DELAYED_WORK(&kms_info->infinity_hotplug_work, GB02FUNC1671);
	device_create_file(drm_dev->dev, &dev_attr_infinity_info);
	device_create_file(drm_dev->dev, &dev_attr_infinity_state);
	device_create_file(drm_dev->dev, &dev_attr_infinity_prop);
	mutex_init(&kms_info->gbdc_infinity_atomic_mutex);

	//for (i = 0; i < crtc_num ; i++)

	ret = GB02FUNC1139(drm_dev, kms_info);
	if (ret) {
		gb_printf(KERN_ERR, "%s %d:crtc init fail! num:%d\n",
			__func__, __LINE__, i);
		return ret;
	}

	for (i = 0; i < conn_num; i++) {
		if (GB02FUNC1902(gb_type, i))
			continue;

		ret = GB02FUNC1897(drm_dev, i,false);
		if (ret) {
			gb_printf(KERN_ERR, "%s %d:link encoder&connector fail! num:%d\n",
				__func__, __LINE__, i);
			return ret;
		}
	}
	if(kms_info->infinity)
		ret = GB02FUNC1898(kms_info, drm_dev);

	pr_info("%s: ret %d\n", __func__, ret);

	drm_mode_config_reset(drm_dev);


	return 0;
}

void GB02FUNC1900(struct GB02STR245 *kms_info, struct drm_device *drm_dev)
{
	int crtc_num = kms_info->num_crtc;
	int i;

	if (kms_info->mode_config_initialized) {
		device_remove_file(drm_dev->dev, &dev_attr_infinity_state);
		device_remove_file(drm_dev->dev, &dev_attr_infinity_info);
		device_remove_file(drm_dev->dev, &dev_attr_infinity_prop);
		drm_mode_config_cleanup(drm_dev);

		for (i = 0; i < crtc_num; i++)
			GB02FUNC1134(drm_dev, i);
	}

	kms_info->mode_config_initialized = false;
}
struct GB02STR245 *GB02FUNC1901(void)
{
	return &gl_gbdev->gbdc_dev->kms_info;
}
int GB02FUNC1902(enum gb_board_type gb_type, int id)
{
	int  no_use = 0;

	if (((gb_type == PCIE_LPDDR4) || (gb_type == PCIE_C0_200))
		&& (id != 2 && id != 3))
		no_use = 1;
	else if ((gb_type == PCIE_FULL_LPDDR4) && (id == 1))
			no_use = 1;
	else if ((gb_type == PCIE_HIE1LP4_LPDDR4) && (id == 1 || id == 4))
			no_use = 1;
	else if ((gb_type == PCIE_M4HL8G_LPDDR4) && (id == 1 || id == 4))
			no_use = 1;

	return no_use;
}
int GB02FUNC1903(enum gb_board_type gb_type,int vidx)
{
	int ret_id = 0;

	if (gb_type == PCIE_LPDDR4 || gb_type == PCIE_C0_200) {
		if (vidx == GB02MAC2695)
		 	ret_id = GB02MAC2832;//1 virt
	} else if (gb_type == PCIE_FULL_LPDDR4) {
		if (vidx >= GB02MAC2695 && vidx < (GB02MAC2695 + 2))
			ret_id = GB02MAC2835 + vidx - GB02MAC2695;//2 virt crtc
	} else if (gb_type == PCIE_HIE1LP4_LPDDR4 ||
		gb_type == PCIE_M4HL8G_LPDDR4 ) {
		if (vidx >=GB02MAC2695  && vidx < (GB02MAC2695 + 2))
			ret_id = GB02MAC2834 + vidx - GB02MAC2695; //2
	} else if (gb_type == PCIE_M6FL8G_LPDDR4) {
		if (vidx >=GB02MAC2695 && vidx < (GB02MAC2695 + 2))
			ret_id = GB02MAC2836 + vidx - GB02MAC2695; //2
	} else {
		pr_warn("gb_type is err\n");
	}

	pr_info("%s: ret_id %d\n",__func__,ret_id);
	return ret_id;
}

int GB02FUNC1904(enum gb_board_type gb_type, int id)
{
	int ret_id = 0;

	if (gb_type == PCIE_LPDDR4 || gb_type == PCIE_C0_200) {
		if (id == 2)
			ret_id = GB02MAC2830;
		else if (id == 3)
			ret_id = GB02MAC2831;
	} else if (gb_type == PCIE_FULL_LPDDR4) {
		if (id == 0)
			ret_id = GB02MAC2830;
		else if (id == 2)
			ret_id = GB02MAC2831;
		else if (id == 3)
			ret_id = GB02MAC2832;
		else if (id == 4)
			ret_id = GB02MAC2833;
		else if (id == 5)
			ret_id = GB02MAC2834;
	} else if (gb_type == PCIE_M6FL8G_LPDDR4) {
		if (id == 0)
			ret_id = GB02MAC2830;
		else if (id == 1)
			ret_id = GB02MAC2831;
		else if (id == 2)
			ret_id = GB02MAC2832;
		else if (id == 3)
			ret_id = GB02MAC2833;
		else if (id == 4)
			ret_id = GB02MAC2834;
		else if (id == 5)
			ret_id = GB02MAC2835;
	} else if (gb_type == PCIE_HIE1LP4_LPDDR4) {
		if (id == 0)
			ret_id = GB02MAC2830;
		else if (id == 2)
			ret_id = GB02MAC2831;
		else if (id == 3)
			ret_id = GB02MAC2832;
		else if (id == 5)
			ret_id = GB02MAC2833;
	} else if (gb_type == PCIE_M4HL8G_LPDDR4) {
		if (id == 0)
			ret_id = GB02MAC2830;
		else if (id == 2)
			ret_id = GB02MAC2831;
		else if (id == 3)
			ret_id = GB02MAC2832;
		else if (id == 5)
			ret_id = GB02MAC2833;
		}
	else {
			ret_id = id;
	}

	return ret_id;
}
