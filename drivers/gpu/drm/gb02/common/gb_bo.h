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

#ifndef __GB_BO_H__
#define __GB_BO_H__

#include <linux/version.h>
#if (KERNEL_VERSION (5, 4, 0) < LINUX_VERSION_CODE)
#include <drm/drm_gem_shmem_helper.h>
#else
#include <drm/drm_gem.h>
#endif
#include <drm/drm_mm.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_placement.h>
#endif

#include "common/gb_common.h"
#define GB02MAC422 512

#define GB02MAC426

struct gb_mmu;
struct GB02STR39;

struct GB02STR50 {
	struct GB02STR59 *gb_bo;
	int sub_bo_flag; //在create bo的时候看 设置是否为heap的子节点
	int pin_count;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	/* GPU address space is independent of CPU word size */
	uint64_t offset;
#endif
#if KERNEL_VERSION(5, 3, 0) > LINUX_VERSION_CODE
        struct drm_gem_object base;
#endif
	struct ttm_buffer_object bo;
	struct ttm_placement placement;
	struct ttm_bo_kmap_obj kmap;
	struct ttm_place placements[3];
};

struct gb_heap_ttm_bo
{
	struct list_head ttm_bo_list; //链表 连接这个bo的所有小bo 串联到gbgpu_bode ttm_bo_root上
	struct GB02STR59 *bo_base;
	struct GB02STR50 ttm_bo; // 小bo的主要组成，承载物理地址
	int grow_nr_pages;
	u64 gb_va_start;
	phys_addr_t *pages;
};

struct GB02STR55 {
	phys_addr_t *heap_pages;
	int heap_nr_pages;
	u64 heap_vpfn;
};

struct GB02STR56 {
	phys_addr_t *pages;  // physical address
	u64 iovaddr;
	struct GB02STR50 ttm_bo;
	struct list_head ttm_bo_root; // 连接heap内存的各个小bo
//11:dec vcmdbuf,21:enc vcmdbuf for vpu
	u32	flags;
	u32 initial_domain;
	void *kptr;
	unsigned prime_shared_count;
};

/* Reference to drm_gem_shmem_object and drm_gem_cma_object struct */
struct GB02STR57 {
	struct GB02STR59 *bo;
	phys_addr_t *pages;  // physical address
	int nr_pages;
	struct mutex pages_lock;
	struct drm_mm_node node;
	struct kref refcount;
	u64 iovaddr;
	u64 iopaddr;
};

struct GB02STR58 {
	unsigned long iovaddr;
	unsigned long vm_start;		/* Our start address within vm_mm. */
	unsigned long vm_end;		/* The first byte after our end address*/
	__u32 size;
};

struct GB02STR59 {
	struct GB02STR57 base;   //gem rely on bitmap
	struct GB02STR56 gb_base; //gem rely on ttm
	struct GB02STR39 *gbdev;
	struct GB02STR47 *private_data;
	struct GB02STR58 vm_info;

	bool is_mapped:1;
	bool noexec:1;
	bool is_heap:1;
	bool is_kms_bo:1;
	bool is_ttm_bo:1;

	bool is_fb_bo;
	bool is_low_va;
	bool is_heap_growable;
	bool is_noclear_bo;

	u32 domain;
	u32 vbo_save_flag;
	u32 vbo_save_flag_old;
	void *vpu_priv;
/*1=dec; 2-enc */
	int	 ip_type;
	int	 cmdbuf_id;
/*nomal vram=1; vcmd buff = 2; vcmd status = 3; */
	int  cmdbuf_type;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#ifdef GB02MAC426
	struct reservation_object *resv;
	struct reservation_object _resv;
#endif
#endif
};

struct GB02STR60 {
	struct rb_node gb_node;
	struct GB02STR59 *gb_gem_bo;
};

#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
#define gem_ob_to_ttm_bo(ttm_ob) \
	container_of(ttm_ob, struct GB02STR50, bo.base)
#else
#define gem_ob_to_ttm_bo(ttm_ob) \
        container_of(ttm_ob, struct GB02STR50, base)
#endif


#define ttm_ob_to_ttm_bo(ttm_ob) \
		container_of(ttm_ob, struct GB02STR50, bo)

#define ttm_bo_to_gbgpu_bo(bo) \
		container_of(bo, struct GB02STR56, ttm_bo)

#define gpu_bo_to_gb_bo(gpu_bo) \
	container_of(gpu_bo, struct GB02STR59, gb_base)

#define	gpu_mmbo_to_gb_bo(gpu_bo) \
	container_of(gpu_bo, struct GB02STR59, base)

#define	ttm_bo_to_heap_bo(ttm_heap_bo) \
	container_of(ttm_heap_bo, struct gb_heap_ttm_bo, ttm_bo)

static inline
struct GB02STR59 *GB02FUNC205(struct GB02STR50 *ttm_bo)
{
	struct GB02STR59 *gb_bo = NULL;
	struct GB02STR56 *gbg_bo = NULL;

	if (!ttm_bo) {
		gb_printf(KERN_ERR, "%s ttm_bo is null \n", __func__);
		return NULL;
	}

	gbg_bo = ttm_bo_to_gbgpu_bo(ttm_bo);
	if (!gbg_bo) {
		gb_printf(KERN_ERR, "%s gbg_bo is null \n", __func__);
		return NULL;
	}

	gb_bo = gpu_bo_to_gb_bo(gbg_bo);
	if (!gb_bo) {
		gb_printf(KERN_ERR, "%s gb_bo is null \n", __func__);
		return NULL;
	}

	return gb_bo;
}

static inline
struct GB02STR59 *GB02FUNC212(struct drm_gem_object *obj)
{
	struct GB02STR59 *gb_bo;
	struct GB02STR50 *ttm_bo = gem_ob_to_ttm_bo(obj);

	gb_bo = GB02FUNC205(ttm_bo); //ttm gem
	if (!gb_bo)
		gb_printf(KERN_ERR, "%s %d : drm gem get gb bo err \n", __func__, __LINE__);
	return gb_bo;
}

static inline
struct GB02STR56 *GB02FUNC217(struct drm_gem_object *gem)
{
	struct GB02STR59 *gb_bo = GB02FUNC212(gem);
	return &gb_bo->gb_base;
}

static inline
struct GB02STR57 *GB02FUNC218(struct drm_gem_object *obj)
{
	struct GB02STR59 *gem_obj = GB02FUNC212(obj);
	return &gem_obj->base;
}
#endif /* __GB_BO_H__ */
