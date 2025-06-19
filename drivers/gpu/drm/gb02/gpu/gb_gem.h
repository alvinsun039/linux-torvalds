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

#ifndef __GB_GEM_H__
#define __GB_GEM_H__

#include <drm/drm_mm.h>
#if KERNEL_VERSION(5, 0, 0) < LINUX_VERSION_CODE
#include <drm/drm_gem_shmem_helper.h>
#endif
#include "common/gb_bo.h"
#include "common/gb_common.h"
#include "gb_mmu.h"

#define GB02MAC1127 0
#define GB02MAC1128 1
#define GB02MAC1129 2

#define GB02MAC1130 0
#define GB02MAC1131 1

extern struct list_head gb_list_head;
struct gb_mmu;
struct GB02STR39;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
extern const struct vm_operations_struct gb_vm_ops;
extern const struct drm_gem_object_funcs gb_gem_object_funcs;
#endif

struct GB02STR125 {
	u64 gpu_phy;
	struct list_head lru;
	volatile u32 pfn;
	struct GB02STR39 *gbdev;
};

static inline
struct  GB02STR59 *GB02FUNC821(struct drm_mm_node *node)
{
	struct GB02STR57 *gb_bo_base = container_of(node, struct GB02STR57, node);
	return gb_bo_base->bo;
}

struct drm_gem_object *GB02FUNC797(struct drm_device *dev, size_t size);
void GB02FUNC688(struct drm_gem_object *obj);

struct drm_gem_object *
gb_gem_prime_import_sg_table(struct drm_device *dev,
				   struct dma_buf_attachment *attach,
				   struct sg_table *sgt);

struct GB02STR59 *
GB02FUNC800(struct drm_file *file_priv,
	struct drm_device *dev, size_t size, int init_domain,
		u32 flags, bool kernel, uint32_t *handle);

struct GB02STR59 *
gb_gpu_gem_create_with_handle(struct drm_file *file_priv,
				struct drm_device *dev, size_t size,
				u32 flags,
				uint32_t *handle);

struct GB02STR59 *GB02FUNC824(struct drm_device *dev,
				size_t size, int init_domain,
				u32 flags, bool kernel);

int GB02FUNC728(struct file *filp, struct vm_area_struct *vma);
int GB02FUNC767(struct GB02STR138 *mmu_mode,
			u64 vpfn, phys_addr_t *phys, size_t nr_pages, unsigned long flags, int dump);
int GB02FUNC774(struct GB02STR138 *mmu_mode, u64 vpfn, size_t nr);

int GB02FUNC715(struct drm_gem_object *obj, struct drm_file *file_priv);
void GB02FUNC723(struct drm_gem_object *obj, struct drm_file *file_priv);
struct reservation_object *gb_gem_prime_res_obj(struct drm_gem_object *obj);

struct rb_root *GB02FUNC209(struct GB02STR47 *gb_priv, int mode);
struct GB02STR60 *GB02FUNC226(void);
struct GB02STR60 *GB02FUNC241(struct rb_root *rb_root, __u64 vaddr, int mode);
int GB02FUNC228(struct rb_root *rb_root, struct GB02STR60 *new);
int GB02FUNC233(struct rb_root *rb_root, struct GB02STR60 *data, int mode);
void GB02FUNC250(struct GB02STR59 *bo, struct GB02STR89 *args);
int GB02FUNC259(struct GB02STR59 *bo, struct vm_area_struct *vma);
void GB02FUNC274(struct GB02STR59 *bo, struct GB02STR47 *gb_priv);
void GB02FUNC271(struct rb_root *rb_root, __u64 vaddr, int mode);
void GB02FUNC283(struct rb_root *rb_root);

#define gb_err(dev, format, arg...) \
        dev_err(dev, "GB Error: "#format, ##arg)
#define gb_info(dev, format, arg...) \
        dev_info(dev, "GB INFO: "#format, ##arg)
#define gb_warn(dev, format, arg...) \
        dev_warn(dev, "GB WARN: "#format, ##arg)
#define gb_verb_printf printk

#endif /* __GB_GEM_H__ */
