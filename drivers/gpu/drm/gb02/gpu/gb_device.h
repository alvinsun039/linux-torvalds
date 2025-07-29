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
#ifndef		__GB_DEVICE_H__
#define		__GB_DEVICE_H__
#include <linux/atomic.h>
#include <linux/version.h>
#if KERNEL_VERSION(5, 0, 0) < LINUX_VERSION_CODE
#include <linux/io-pgtable.h>
#endif
#include <linux/spinlock.h>
#include <drm/drm_device.h>
#include <drm/drm_mm.h>
#include <drm/gpu_scheduler.h>
#include "gb_mmu.h"
#include "gb_gem.h"  //include gb_bo.h
#include "gb_pcie_info.h"
//#include "gb_ttm.h"
#include "vpu/vpu_comm/gb_vpu.h"
#include "vpu/vpu_mm/gb_vpu_ttm.h"
#include "gb_ip_offset.h"
#include "gb_ttm.h"
#include "kms/device/gb_ip.h"
#include <generated/uapi/linux/version.h>
#include "mcu_peripherals/gpu_freq/gb_devfreq.h"


struct GB02STR39;
struct gb_mmu;
struct GB02STR165;
struct GB02STR162;
//#define GB_PRINT_VERBOSE
//#define DEBUG_FOR_JC_DATA
#define GB02MAC286
#define GB02MAC287
#define GB02MAC288		3
#define GB02MAC289		8
#define GB02MAC290	1
#define GB02MAC291
#define	GB02MAC292
//#define	GB_VDEC
//#define	GB_VDEC1
//#define	GB_VENC
//#define	GB02MAC483
//#define	GB02MAC484
//#define	GB_DP_MODULE
#define	GB02MAC293
#ifdef GB02MAC481
//#define GB_ALLSCREEN
#endif
#define	GB02MAC294
#define	GB02MAC295
#define KB      (1UL<<10)
#define MB      (1UL<<20)
#define GB      (1UL<<30)
#define GB_PCI_NAME		"genbu_pcie"

enum {
	GB_RST_IDLE = 0,
	GB_RST_PENDING = 1,
	GB_RST_HW_STARTING = 2,
	GB_RST_HW_COMPLETE = 3,
};
enum {
	FULL_DDR4_VAL = 0,
	FULL_LPDDR4_VAL,
	PCIE_LPDDR4_VAL,
	PCIE_M6FL8G_LPDDR4_VAL,
	PCIE_HIE1LP4_LPDDR4_VAL,
	PCIE_CQ2040_P21_VAL,
	PCIE_M4HL8G_LPDDR4_VAL = 7,
	CQ2040_MXM_M60_VAL,
	PCIE_C0_200_VAL = 9,
	PCIE_HIE1LP42_LPDDR4_VAL = 11,
};

struct GB02STR28 {
	u16 id;
	u16 revision;

	u64 shader_present;
	u64 tiler_present;
	u64 l2_present;
	u64 stack_present;
	u32 as_present;
	u32 ss_present;

	u32 l2_features;
	u32 tiler_features;
	u32 mem_features;
	u32 mmu_features;
	u32 thread_features;
	u32 max_threads;
	u32 thread_max_workgroup_sz;
	u32 thread_max_barrier_sz;
	u32 coherency_features;
	u32 texture_features[4];
	u32 ss_features[16];

	u32 nr_core_groups;
	u32 heap_memory_growable;
	u32 vpu_bo_save_flag;
};

struct gb_devfreq_slot {
	ktime_t busy_time;
	ktime_t idle_time;
	ktime_t time_last_update;
	bool busy;
};

#define GB02MAC308(x)                (GB02FUNC464())

#define GB02MAC311                   (4UL*KB)
#define GB02MAC313                  (12UL)
#define GB02MAC315                   (~(GB02MAC311-1))
#define GB02MAC317                   (PAGE_SIZE / GB02MAC311)

// Normal page alloced base.
#define GB02MAC319(x) 	(GB02FUNC471(x))
#define GB02MAC321(x) 		(GB02MAC308(x) - GB02MAC319(x))
#define GB02MAC323(x)  (GB02MAC321(x) / PAGE_SIZE)
#define GB02MAC324(x)  (GB02MAC321(x) / GB02MAC311)
#define GB02MAC326(x)          (GB02MAC321(x) - (GB02MAC323(x) * PAGE_SIZE))  // Summary the unaliagned size by PAGE_SIZE
#define GB02MAC328(x) 	(GB02MAC505 + GB02MAC493)
/* Inside MIPS arch, PAGE_SIZE is 16KB, others' are 4KB */
#define GB02MAC330(x)          ((GB02MAC324(x) + 63UL) / (64UL))
#define GB02MAC331(x)         ((GB02MAC323(x) + 63UL) / (64UL))

struct GB02STR35 {
        spinlock_t ddr_lock;
        atomic_t used_pages;
        atomic_t used_mmu_pages;
        volatile u64 page_highest_addr;
        volatile u64 mmu_lowest_addr;
        volatile u64 *usage;
        volatile u64 *mmu_usage;
};

enum system_working_status {
	HIBERNATE_SLEEP = 0,
	SUSPEND_SLEEP,
	SYSTEM_WORKING,
	SYSTEM_MAX_STATUS,
};

struct GB02STR39 {
	struct device *dev;
	struct drm_device *ddev;
	struct GB02STR155 *gbdc_dev;
	struct GB02STR70 *gb_pcie;
	struct gpu_info *gpu_device_info;
	struct GB02STR72 gb_peri[GB02MAC867];

	void __iomem *gpu_reg_base;
	struct regulator *regulator;
	struct reset_control *rstc;

	struct GB02STR28 features;

	struct GB02STR117 version;

	spinlock_t as_lock;
	spinlock_t mmu_hw_lock;
	struct mutex mmu_as_lock;
	unsigned long as_in_use_mask;
	unsigned long as_alloc_mask;
	struct list_head as_lru_list;

	struct GB02STR165 *ss;

	struct GB02STR162 *stages[GB02MAC288];
	struct list_head scheduled_stages;

	struct mutex sched_lock;
	struct GB02STR44 {
		spinlock_t lock;
		struct semaphore job_semaphore;
		unsigned hw_job_limit;
	} jsl;
	struct {
		struct spsc_queue       remain_job_queue;
		void *resume_job[GB02MAC289][GB02MAC288];
		struct workqueue_struct *reset_workq;
		struct work_struct work;
		atomic_t pending;
	} reset;

	struct GB02STR47 *priv[GB02MAC289];
	struct mutex gb_file_priv_lock;

	spinlock_t hw_irq_lock;

	struct {
		struct gb_devfreq *devfreq;
		struct thermal_cooling_device *cooling;
		unsigned long cur_freq;
		unsigned long cur_volt;
		struct gb_devfreq_slot slot[GB02MAC288];
	} devfreq;

	struct GB02STR35 GB02STR35;
	struct GB02STR125 *global_gb_page;
	struct GB02STR125 *mmu_top_page;
	//struct GB02STR67 *dev_info;
	struct mutex vram_mutex;
	struct GB02STR138 *mmu_mode;

	struct GB02STR176 gb_mm;

	struct GB02STR130  ipoffset_base;

	//for vpu
	struct rw_semaphore		exclusive_lock;
	struct rw_semaphore		mclk_lock;
	atomic_t used_dec0_cnt;
	atomic_t used_dec1_cnt;
	struct list_head vpu_bo_list_head;
	//struct GB02STR200			gem;
	atomic64_t			vram_usage;

		/* memory management */
	void			*vpu_ttm;
	struct GB02STR10		init_dvcmd;
	struct GB02STR11		init_evcmd;
	void  *gbdec_data;
	void  *gbenc_data;
	//u64 vram_pin_size;

	struct GB02STR70 pcie_info;

	struct mutex v2v_mutex;
	struct mutex rb_mutex;
	enum system_working_status sleep_status;
	atomic_t perf_time_per;
	struct timer_list perf_timer;
};

struct GB02STR46 {
	struct drm_file *drm_file;
	struct GB02STR47 *gb_priv;
	struct list_head list;
	struct rb_root gpu_root_node;
	struct rb_root cpu_root_node;
};

struct GB02STR47 {
	struct GB02STR39 *gbdev;

	struct drm_sched_entity sched_entity[GB02MAC288];
	struct task_struct *pcb;
	struct drm_file *drm_file;

	struct mutex perfcnt_lock;
	struct GB02STR46 list_node;
	int dec_id;
	int cmdbuf_type;
/*nomal vram=1; vcmd buff = 2; vcmd status = 3 */
	__u32  bo_type;
	void *dec_priv;
	void *dec1_priv;
	void *enc_priv;
};
static inline struct GB02STR39 *GB02FUNC183(struct drm_device *ddev)
{
	return ddev->dev_private;
}
void GB02FUNC84(struct GB02STR39 *gbdev);
int GB02FUNC107(struct GB02STR39 *gbdev, struct GB02STR67 *GB02STR153);
void GB02FUNC128(struct GB02STR39 *gbdev);
void GB02FUNC136(struct GB02STR39 *gbdev);

int GB02FUNC137(struct device *dev);
int GB02FUNC138(struct device *dev);
struct GB02STR47* GB02FUNC91(void);
void GB02FUNC443(void);
void GB02FUNC95(struct GB02STR39 *gbdev);

const char *GB02FUNC130(struct GB02STR39 *gbdev, u32 exception_code);

#define reg_write(dev, reg, data) writel(data, dev->gpu_reg_base + reg)
#define reg_read(dev, reg) readl(dev->gpu_reg_base + reg)

#define gpu_write(dev, reg, data) writel(data, dev->gpu_reg_base + reg)
#define gpu_read(dev, reg) readl(dev->gpu_reg_base + reg)

#define stage_write(dev, reg, data) writel(data, dev->gpu_reg_base + (reg))
#define stage_read(dev, reg) readl(dev->gpu_reg_base + (reg))

#define mmu_write(dev, reg, data) writel(data, dev->gpu_reg_base + reg)
#define mmu_read(dev, reg) readl(dev->gpu_reg_base + reg)

#endif
