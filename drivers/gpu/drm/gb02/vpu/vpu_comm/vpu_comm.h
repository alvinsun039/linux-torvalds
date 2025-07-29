/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * jessy 20221019 Sietium
 */

#ifndef __VPU_COMM_H__
#define __VPU_COMM_H__
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>

#include "vpu_list.h"
#include "vpu/vpu_comm/gb_vpu.h"
#include "vpu_vcmd_registers.h"
#include "vpu_basetype.h"

enum vcmd_module_type {
	VCMD_TYPE_ENCODER = 0,
	VCMD_TYPE_CUTREE,
	VCMD_TYPE_DECODER,
	VCMD_TYPE_JPEG_ENCODER,
	VCMD_TYPE_JPEG_DECODER,
	MAX_VCMD_TYPE
};

#define GB02MAC143    26
#define GB02MAC144			(128 * 4)
#define GB02MAC145          0
#define GB02MAC146       1
#define GB02MAC147        0
#define GB02MAC148    1
#define GB02MAC149    2
#define GB02MAC150      0
#define GB02MAC151        1
#define GB02MAC152				(MAX_VCMD_TYPE * 4)

#define GB02MAC153                  0x0012
#define GB02MAC154   (4096 * 2160 * 4 * 1)
#define GB02MAC155       3
/* variables declaration related with race condition */
#define GB02MAC156			(512 * 4 * 4)

/* approximately=128x(320x240)=128x2k=128x8kbyte=1Mbytes */
#define GB02MAC158		(2 * 1024 * 1024)
#define GB02MAC159 (GB02MAC158 / GB02MAC156 / 2)
#define GB02MAC160 \
	(9 * 1024 * 1024 - GB02MAC158 * 2)

#define GB02MAC161 (4096 * 2160 * 4 * GB02MAC269)
#define GB02MAC162           (2147483648U) // 32K*32K*2
#define GB02MAC163\
	(GB02MAC161 > GB02MAC162 ?\
	GB02MAC161:GB02MAC162)

#ifdef EMU
#define GB02MAC164  0x04100000
#define GB02MAC165  0x04200000
#define GB02MAC166  0x04300000
#else
#define GB02MAC164  0x00100000
#define GB02MAC165  0x00200000
#define GB02MAC166  0x00300000
#endif

#define GB02MAC167              0
#define GB02MAC168               (GB02MAC167 + 6 * 4)
#define GB02MAC169               (GB02MAC167 + 97 * 4)
#define GB02MAC170       (GB02MAC167 + 107 * 4)
#define GB02MAC171             (GB02MAC167 + 226 * 4)
#define GB02MAC172             (GB02MAC167 + 227 * 4)
#define GB02MAC173         (GB02MAC167 + 228 * 4)

/* VCMD */
#define GB02MAC174					(0x01 << 27)
#define GB02MAC175					(0x02 << 27)
#define GB02MAC176					(0x03 << 27)
#define GB02MAC177					(0x16 << 27)
#define GB02MAC178					(0x18 << 27)
#define GB02MAC179					(0x19 << 27)
#define GB02MAC180				(0x09 << 27)
#define GB02MAC181				(0x1a << 27)
#define GB02MAC182				(0x19 << 27)
#define GB02MAC183				((0x19 << 27) | (1 << 26))
#define GB02MAC184					(1 << 25)
#define GB02MAC185					(1 << 26)

#define GB02MAC186					0x00120fff
#define GB02MAC187					0x43421102

#define GB02MAC188 768
/* these size need to be modified according to hw config */
#define GB02MAC189              (479 * 4)
#define GB02MAC190              (GB02MAC188 * 4)
#define GB02MAC191                   (479 * 4)
#define GB02MAC192         (479 * 4)
#define GB02MAC193         (GB02MAC188 * 4)

#define GB02MAC194        1U
#define GB02MAC195       2U
#define GB02MAC196        3U
#define GB02MAC197              4U
#define GB02MAC198         5U
#define GB02MAC199       6U
#define GB02MAC200         7U
#define GB02MAC201         8U
#define GB02MAC202          9U
#define GB02MAC203         10U
#define GB02MAC204         11U
#define GB02MAC205        12U
#define GB02MAC206           14U
#define GB02MAC207     15U
#define GB02MAC208        16U
#define GB02MAC209         17U
#define GB02MAC211      31U

/* hantro G2 reg config */
#define GB02MAC214              337 /* G2 total regs */
#define GB02MAC215              155 /* G1 total regs */
#define GB02MAC216             GB02MAC188 /* VC8000D total regs */

/* Logic module IRQs */
#define GB02MAC217                    -1
#define MAX(a, b)                       (((a) > (b)) ? (a) : (b))
#define DEC_IO_SIZE_MAX\
	(MAX(MAX(GB02MAC214, GB02MAC215),\
		GB02MAC216) * 4)

/* Base address DDR register */
#define GB02MAC218 0

/* Base address got control register */
#define GB02MAC219                 4

/* PCIe hantro driver offset in control register */
#define GB02MAC220               0x600000
#define GB02MAC221               0x700000

#define GB02MAC222                   DEC_IO_SIZE_MAX /* bytes */
#define GB02MAC223                   DEC_IO_SIZE_MAX /* bytes */

#define GB02MAC224                       GB02MAC217
#define GB02MAC225                       GB02MAC217
#define IS_G1(hw_id)                    (((hw_id) == 0x6731) ? 1 : 0)
#define IS_G2(hw_id)                    (((hw_id) == 0x6732) ? 1 : 0)
#define IS_VC8000D(hw_id)               (((hw_id) == 0x8001) ? 1 : 0)
#define IS_BIGOCEAN(hw_id)              (((hw_id) == 0xB16D) ? 1 : 0)

#define GB02MAC226            336
#define GB02MAC227                4
#define GB02MAC228         4096
#define GB02MAC229       1

#define GB02MAC230              144
#define GB02MAC231                  1
#define GB02MAC232           4096
#define GB02MAC233         1

#define GB02MAC234(ct)          case (ct): return(#ct + 3)

#define GB02MAC235 0xFFFF


/* Use 'v' as magic number for vcmd */
#define GB02MAC236  'v'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#define HANTRO_VCMD_IOCH_GET_CMDBUF_PARAMETER\
	_IOWR(GB02MAC236, 20, struct GB02STR16 *)
#define HANTRO_VCMD_IOCH_GET_HWINFO_FROM_VCMD\
	_IOWR(GB02MAC236, 21, unsigned long)
#define HANTRO_VCMD_IOCH_SET_CMDBUF_POOL_BASE\
	_IOWR(GB02MAC236, 22, unsigned long)
#define HANTRO_VCMD_IOCH_GET_VCMD_PARAMETER\
	_IOWR(GB02MAC236, 24, struct GB02STR45 *)

#define HANTRO_VCMD_IOCH_LINK_RUN_CMDBUF _IOR(GB02MAC236, 26, u16 *)
#define HANTRO_VCMD_IOCH_WAIT_CMDBUF    _IOR(GB02MAC236, 27, u16 *)
#define HANTRO_VCMD_IOCH_RELEASE_CMDBUF _IOR(GB02MAC236, 28, u16 *)

#define HANTRO_VCMD_IOCH_POLLING_CMDBUF _IOR(GB02MAC236, 40, u16 *)

#define GB02MAC237		50

/* priority support */
#define MAX_CMDBUF_PRIORITY_TYPE    2 // 0:normal priority,1:high priority

// video decoder vcmd configuration
#define GB02MAC238					0x600000
#define GB02MAC239                  (GB02MAC4 * 4)
#define GB02MAC241                  -1
#define GB02MAC243              2
#define GB02MAC244         0x1000
#define GB02MAC246       0XFFFF
#define GB02MAC248      0X2000
#define GB02MAC250          0XFFFF

#define GB02MAC253                  0x700000
#define GB02MAC255                  (GB02MAC4 * 4)
#define GB02MAC257                  -1
#define GB02MAC259              2
#define GB02MAC261         0x1000
#define GB02MAC263       0XFFFF
#define GB02MAC265      0X2000
#define GB02MAC267          0XFFFF

#define GB02MAC269		(4 * 8)

struct GB02STR14 {
	u32 *virtual_address;
	dma_addr_t  bus_address;
	size_t mmu_bus_address;  /* buffer physical address in MMU */
	u32 size;
	u16 cmdbuf_id;
};

struct GB02STR15 {
	u64 executing_time;
	u16 module_type; /* input vc8000e=0,IM=1,vc8000d=2,jpege=3,jpegd=4 */
	u16 cmdbuf_size; /* input, reserve not used; link and run is input */
	u16 priority;    /* input,normal=0, high/live=1 */
	u16 cmdbuf_id;   /* output, it is unique in driver */
	u16 core_id;     /* just used for polling */
};
struct GB02STR16 {
	u32 *virt_cmdbuf_addr;
	ptr_t phy_cmdbuf_addr;
	u32 mmu_phy_cmdbuf_addr;
	u32 cmdbuf_total_size;
	u16 cmdbuf_unit_size;
	u32 *virt_status_cmdbuf_addr;
	ptr_t phy_status_cmdbuf_addr;
	u32 mmu_phy_status_cmdbuf_addr;
	u32 status_cmdbuf_total_size;
	u16 status_cmdbuf_unit_size;
	ptr_t base_ddr_addr;
};


struct GB02STR17 {
	struct drm_file *filp;
	u64 total_exe_time;
	spinlock_t spinlock;
	wait_queue_head_t wait_queue;
};

struct GB02STR18 {
	u32 module_type; /* input vc8000e=0,IM=1,vc8000d=2,jpege=3, jpegd=4 */
	u32 priority;    /* current CMDBUFpriority: normal=0, high=1 */
	u64 executing_time;
	u32 cmdbuf_size;
	u32 *cmdbuf_virtual_address;
	size_t cmdbuf_bus_address;
	size_t mmu_cmdbuf_bus_address;
	u32 *status_virtual_address;
	size_t status_bus_address;
	size_t mmu_status_bus_address;
	u32 status_size;
	u32 executing_status;
	struct drm_file *filp;
	u16 core_id;
	u16 cmdbuf_id;
	u8 cmdbuf_data_loaded;
	u8 cmdbuf_data_linked;
	u8 cmdbuf_run_done;
	u8 cmdbuf_need_remove;
	u32 waited;
	u8 has_end_cmdbuf;
	u8 no_normal_int_cmdbuf;
	struct GB02STR17 *GB02STR17;
};

struct GB02STR20 {
	u32 *vpu_vaddr_base;
	dma_addr_t  vpu_paddr_base;
	u32 *vram_vaddr_base;
	dma_addr_t  vram_paddr_base;
	u32 size;
};

struct GB02STR23 {
	unsigned long base_addr;
	int irq;
	u32 subsys_type;
	u32 submodule_offset[HW_CORE_MAX];
	u32 submodule_iosize[HW_CORE_MAX];
	volatile u8 *submodule_hwregs[HW_CORE_MAX];
	int has_apbfilter[HW_CORE_MAX];
};

struct GB02STR24 {
	u32 nbr_mask_regs;
	u32 mask_reg_offset;
	u32 page_sel_addr;
	u8  num_mode;
	u8  mask_bits_per_reg;
	u32 id; /* id of the subsystem */
	u32 type; /* type of core to be written */
	u32 has_apbfilter;
};

struct GB02STR25 {
	u8 axi_rd_chn_num;
	u8 axi_wr_chn_num;
	u8 axi_rd_burst_length;
	u8 axi_wr_burst_length;
	u8 fe_mode;
	u32 id;
};

struct GB02STR26 {
	u32 slice; /* id of the slice */
	u32 id; /* id of the subsystem */
	u32 type; /* type of core to be written */
	u32 size; /* iosize of the core */
};

struct GB02STR27 {
	u32 id; /* id of the subsystem */
	u32 type; /* type of core to be written */
	u32 *regs; /* pointer to user registers */
	u32 size; /* size of register space */
	u32 reg_id; /* id of reigster to be read/written */
};

struct GB02STR29 {
	u32 slice; /* id of the slice */
	u32 id; /* id of the subsystem */
	u32 type; /* type of core to be written */
	u32 size; /* iosize of the core */
	u32 asic_id; /* asic id of the core */
};

struct GB02STR30 {
	u32 subsys_num;   /* total subsystems count */
	u32 subsys_vcmd_num;  /* subsystems with vcmd */
};

typedef struct {
	u32 cfg[GB02MAC12];  /* indicate the supported format */
	u32 cfg_backup[GB02MAC12];       /* back up of cfg */
	int its_main_core_id[GB02MAC12]; /* indicate if main core exist */
	int its_aux_core_id[GB02MAC12];  /* indicate if aux core exist */
} core_cfg;

struct GB02STR31 {
	int dec_irq;
	int pp_irq;
	u32 dec_regs[GB02MAC12][DEC_IO_SIZE_MAX / 4];
	u32 apbfilter_regs[GB02MAC12][DEC_IO_SIZE_MAX / 4 + 1];
	u32 shadow_dec_regs[GB02MAC12][DEC_IO_SIZE_MAX / 4];
	int reg_access_opt;
	atomic_t *irq_rx;
	atomic_t *irq_tx;

	struct GB02STR20 dec_pcie;

	core_cfg config;
	struct GB02STR23 vpu_subsys[GB02MAC9];
	struct GB02STR24 GB02STR24[GB02MAC9][HW_CORE_MAX];
	struct GB02STR25 GB02STR25[GB02MAC9];
	struct semaphore dec_core_sem;
	struct drm_file *dec_owner[GB02MAC12];
	struct file *pp_owner[GB02MAC12];
};

enum VPU_MM_Status {
	MM_STATUS_OK                          =    0,
	MM_STATUS_FALSE                       =   -1,
	MM_STATUS_INVALID_ARGUMENT            =   -2,
	MM_STATUS_INVALID_OBJECT              =   -3,
	MM_STATUS_OUT_OF_MEMORY               =   -4,
	MM_STATUS_NOT_FOUND                   =   -19,
};

struct GB02STR32 {
	size_t	bus_address;  /* buffer virtual address */
	size_t	mmu_bus_address;  /* buffer physical address in MMU */
	unsigned int size;	/* physical size */
};

struct GB02STR33 {
	void *virtual_address;  /* buffer virtual address */
	size_t bus_address;  /* buffer physical address */
	unsigned int size;  /* physical size */
};

/* Used in vcmd initialization in hantro_vcmd_xxx.c. */
/* May be unified in next step. */
struct GB02STR34 {
	unsigned long vcmd_base_addr;
	u32 vcmd_iosize;
	int vcmd_irq;
	u32 sub_module_type;        // input vc8000e=0,IM=1,vc8000d=2,jpege=3,jpegd=4
	u16 submodule_main_addr;    // in byte
	u16 submodule_dec400_addr;
	u16 submodule_L2Cache_addr; // in byte
	u16 submodule_MMU_addr;     // in byte
	u16 submodule_MMUWrite_addr;// in byte
	u16 submodule_axife_addr;   // in byte
};

struct GB02STR37 {
	struct GB02STR34 vcmd_core_cfg;
	u32 core_id; /* vcmd core id for driver and sw internal use */
	u32 sw_cmdbuf_rdy_num;
	spinlock_t *spinlock;
	wait_queue_head_t *wait_queue;
	wait_queue_head_t *wait_abort_queue;
	bi_list list_manager;
	volatile u8 *hwregs; /* IO mem base */
	u32 reg_mirror[GB02MAC4];
	u32 duration_without_int;  /* number of cmdbufs without interrupt */
	volatile u8 working_state;
	u64 total_exe_time;
	u16 status_cmdbuf_id; /* used for analyse configuration in cwl */
	u32 hw_version_id; /* megvii 0x43421001, later 0x43421102 */
	u32 *vcmd_reg_mem_virtual_address;
	size_t vcmd_reg_mem_bus_address;
	unsigned int mmu_vcmd_reg_mem_bus_address;
	u32  vcmd_reg_mem_size;
};

struct GB02STR41 {
	u32 hw_id;
	u32 build_id;
	u32 synth_cfg;
	u32 synth_cfg_2;
	u32 synth_cfg_3;
	u32 pp_synth_cfg;
	u32 fuse_cfg;
	u32 pp_cfg_stat;
};
struct GB02STR43 {
	int vcmd_type_core_num[MAX_VCMD_TYPE];
	u16 vcmd_position[MAX_VCMD_TYPE];
	u16 total_vcmd_core_num;
	u16 cmdbuf_used_pos;
	u16 cmdbuf_used_residual;
	u16 cmdbuf_used[GB02MAC159];
	unsigned long gBaseDDRHw;
	size_t base_ddr_addr;

	bi_list_node * global_cmdbuf_node[GB02MAC159];
	bi_list global_process_manager;
	struct semaphore vcmd_reserve_cmdbuf_sem[MAX_VCMD_TYPE];
	struct GB02STR14 dec_vcmd_mem_pool[3];
	spinlock_t owner_lock_vcmd[GB02MAC152];
	wait_queue_head_t wait_queue_vcmd[GB02MAC152];
	wait_queue_head_t abort_queue_vcmd[GB02MAC152];
	wait_queue_head_t mc_wait_queue;

	struct GB02STR3 vcmd_buff;
	struct GB02STR41 hw_info;
	struct GB02STR3 *vcmd_pool;
	struct GB02STR3 *vcmd_reg;
};

struct GB02STR45 {
	u16 module_type;
	u16 vcmd_core_num;
	u16 submodule_main_addr;
	u16 submodule_dec400_addr;
	u16 submodule_L2Cache_addr;
	u16 submodule_MMU_addr;
	u16 submodule_MMUWrite_addr;
	u16 submodule_axife_addr;
	u16 config_status_cmdbuf_id;
	u32 vcmd_hw_version_id;
};

/* SubsysDesc & CoreDesc are used for configuration */
struct SubsysDesc {
	int slice_index;    /* slice this subsys belongs to */
	int index;   /* subsystem index */
	long base;
};

struct CoreDesc {
	int slice;
	int subsys;     /* subsys this core belongs to */
	enum core_type core_type;
	int offset;     /* offset to subsystem base */
	int iosize;
	int irq;
	int has_apb;
};

bi_list_node *GB02FUNC260(bi_list_node *current_node);
unsigned long long GetMMUAddress(void);
void GB02FUNC264(bi_list_node *process_node);
bi_list_node *GB02FUNC176(bi_list *list,
					   bi_list_node *cmdbuf_node);
long GB02FUNC277(bi_list *list);
u64 GB02FUNC272(bi_list_node *exe_cmdbuf_node);
void GB02FUNC247(bi_list *global_process_manager);
bi_list_node *GB02FUNC243(void);
int GB02FUNC256(struct GB02STR17 *manager_obj);
void GB02FUNC254(struct GB02STR18 *GB02STR18);
struct GB02STR18 *GB02FUNC249(void);

#endif

