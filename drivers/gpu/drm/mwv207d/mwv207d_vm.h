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

#ifndef MWV207D_VM_H_HSUY19WV
#define MWV207D_VM_H_HSUY19WV
#include <linux/atomic.h>
#include <linux/kref.h>
#include <linux/rbtree.h>
#include <linux/idr.h>

struct mwv207d_bo;

struct mwv207d_pt {
	u64  *entry;
	u64 phys_addr;
	int bit;
};

struct mwv207d_pte {
	struct mwv207d_pt pt;
	int nr_active;
};
struct mwv207d_pmd {
	struct mwv207d_pt pt;
	struct mwv207d_pte *pte[512];
	int nr_active;
};
struct mwv207d_pud {
	struct mwv207d_pt pt;
	struct mwv207d_pmd *pmd[512];
	int nr_active;
};
struct mwv207d_pgd {
	struct mwv207d_pt pt;
	struct mwv207d_pud *pud[512];
};

struct mwv207d_mapping {
	struct rb_node rb;
	u64 __subtree_last;

	u64 start;
	u64 last;
	u64 offset;
	u32 flags;

	struct mwv207d_vm *vm;

	struct kref refcnt;
	bool updated_once;

	struct list_head list;
};

struct mwv207d_vm {
	struct mwv207d_device *mdev;
	struct kref refcnt;
	unsigned long dirty;

	int pta_id;

	u64  identity;

	struct mutex mutex;
	struct rb_root_cached va;
	int nr_reserved_maps;
	struct mwv207d_mapping resvd[0x10];
	struct mwv207d_pgd pgd;
};

struct mwv207d_mmu_desc {
	u32 low;
	u32 high;
};
struct mwv207d_pta {
	struct mwv207d_mmu_desc *desc;
	u64 phys_addr;
	u64 safe_pg_addr;
	struct ida pt_id;
	atomic64_t identity;
	spinlock_t lock;
};

struct mwv207d_bo_vm {
	struct mwv207d_vm *vm;

	struct list_head bo_vm_node;

	struct list_head pending_maps;
	struct list_head valid_maps;

	bool moved;
	unsigned int nr_maps;
	int refcnt;
};

int mwv207d_pta_init(struct mwv207d_device *mdev);
void mwv207d_pta_fini(struct mwv207d_device *mdev);
void mwv207d_mmu_init(struct mwv207d_device *mdev, struct mwv207d_vm *vm,
		      void (*mmu_write)(void *priv, u32 offset, u32 val),
		      void *priv);
void mwv207d_mmu_flush(struct mwv207d_device *mdev, struct mwv207d_vm *vm,
		       void (*mmu_write)(void *priv, u32 offset, u32 val),
		       void *priv);

struct mwv207d_vm *mwv207d_vm_create(struct mwv207d_device *mdev);
void mwv207d_vm_reserve(struct mwv207d_vm *vm, u64 start, u64 size);
struct mwv207d_vm *mwv207d_vm_get(struct mwv207d_vm *vm);
void mwv207d_vm_put(struct mwv207d_vm *vm);

int mwv207d_vm_map_pages(struct mwv207d_vm *vm, u64 va, unsigned long pages,
			 dma_addr_t *dma_addr, u32 flags);
int mwv207d_vm_map_linear(struct mwv207d_vm *vm, u64 va, u64 size,
			  u64 pa, u32 flags);
void mwv207d_vm_unmap(struct mwv207d_vm *vm, u64 offset, unsigned long pages);
int mwv207d_vm_map_bo(struct mwv207d_vm *vm, struct mwv207d_bo *bo, u64 va, u32 flags);

struct mwv207d_bo_vm *mwv207d_bo_vm_find(struct mwv207d_bo *bo,
		struct mwv207d_vm *vm);
int mwv207d_vm_validate_bo_vm(struct mwv207d_bo *bo, struct mwv207d_bo_vm *bo_vm);
void mwv207d_vm_invalidate_bo(struct mwv207d_bo *bo, bool erase);

int mwv207d_mapping_create(struct mwv207d_bo_vm *bo_vm, u64 offset,
		u64 saddr, u64 size, u32 flags);
int mwv207d_mapping_remove(struct mwv207d_bo_vm *bo_vm,
		u64 va_start, u64 va_last);
struct mwv207d_mapping *mwv207d_mapping_get(struct mwv207d_mapping *mapping);
void mwv207d_mapping_put(struct mwv207d_mapping *mapping);

int mwv207d_vm_suspend(struct mwv207d_device *mdev);
void mwv207d_vm_resume(struct mwv207d_device *mdev);
#endif
