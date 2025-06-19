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
#ifndef __VPU_VC8000E_VCMD_H__
#define __VPU_VC8000E_VCMD_H__
#include "common/gb_uk.h"
#include "vpu/vpu_comm/vpu_list.h"
#include "vpu/vpu_comm/vpu_vcmd_registers.h"
#include "vpu/vpu_comm/vpu_comm.h"
#include "vpu/vpu_comm/vpu_basetype.h"

/* VCMD CONFIGURATION BY CUSTOMER********/
//video encoder vcmd configuration
#define GB02MAC2376                      0x0
#define GB02MAC2377                      (GB02MAC4 * 4)
#define GB02MAC2378                      -1
#define GB02MAC2379                  0
#define GB02MAC2380             0x1000
#define GB02MAC2381           0XFFFF
#define GB02MAC2383          0XFFFF
#define GB02MAC2384             0XFFFF
#define GB02MAC2385             0XFFFF
#define GB02MAC2387           0XFFFF
#define GB02MAC2389           0XFFFF

#define GB02MAC2391                      0x91000
#define GB02MAC2393                      (GB02MAC4 * 4)
#define GB02MAC2395                      -1
#define GB02MAC2397                  0
#define GB02MAC2399             0x0000
#define GB02MAC2401           0XFFFF
#define GB02MAC2403          0XFFFF
#define GB02MAC2405              0XFFFF

#define GB02MAC2407                      0x92000
#define GB02MAC2409                      (GB02MAC4 * 4)
#define GB02MAC2411                      -1
#define GB02MAC2413                  0
#define GB02MAC2415             0x0000
#define GB02MAC2417           0XFFFF
#define GB02MAC2419          0XFFFF
#define GB02MAC2421              0XFFFF

#define GB02MAC2424                      0x93000
#define GB02MAC2425                      (GB02MAC4 * 4)
#define GB02MAC2426                      -1
#define GB02MAC2428                  0
#define GB02MAC2429             0x0000
#define GB02MAC2430           0XFFFF
#define GB02MAC2431          0XFFFF
#define GB02MAC2432              0XFFFF

/* video encoder cutree/IM  vcmd configuration */
#define GB02MAC2433                      0x94000
#define GB02MAC2435                      (GB02MAC4 * 4)
#define GB02MAC2436                      -1
#define GB02MAC2437                  1
#define GB02MAC2438             0x1000
#define GB02MAC2439           0XFFFF
#define GB02MAC2440          0XFFFF
#define GB02MAC2441             0XFFFF
#define GB02MAC2442             0XFFFF
#define GB02MAC2443           0XFFFF
#define GB02MAC2444           0XFFFF

#define GB02MAC2445                      0xa1000
#define GB02MAC2446                      (GB02MAC4 * 4)
#define GB02MAC2447                      -1
#define GB02MAC2448                  1
#define GB02MAC2449             0x0000
#define GB02MAC2450           0XFFFF
#define GB02MAC2451          0XFFFF
#define GB02MAC2452              0XFFFF

#define GB02MAC2453                      0xa2000
#define GB02MAC2454                      (GB02MAC4 * 4)
#define GB02MAC2455                      -1
#define GB02MAC2456                  1
#define GB02MAC2457             0x0000
#define GB02MAC2458           0XFFFF
#define GB02MAC2459          0XFFFF
#define GB02MAC2460              0XFFFF

#define GB02MAC2461                      0xa3000
#define GB02MAC2462                      (GB02MAC4 * 4)
#define GB02MAC2463                      -1
#define GB02MAC2464                  1
#define GB02MAC2465             0x0000
#define GB02MAC2466           0XFFFF
#define GB02MAC2467          0XFFFF
#define GB02MAC2468              0XFFFF

//video decoder vcmd configuration

#if 0
#define GB02MAC238                      0xb0000
#define GB02MAC239                      (GB02MAC4 * 4)
#define GB02MAC241                      -1
#define GB02MAC243                  2
#define GB02MAC244             0x0000
#define GB02MAC246           0XFFFF
#define GB02MAC248          0XFFFF
#define GB02MAC250              0XFFFF

#define GB02MAC253                      0xb1000
#define GB02MAC255                      (GB02MAC4 * 4)
#define GB02MAC257                      -1
#define GB02MAC259                  2
#define GB02MAC261             0x0000
#define GB02MAC263           0XFFFF
#define GB02MAC265          0XFFFF
#define GB02MAC267              0XFFFF

#define GB02MAC2469                      0xb2000
#define GB02MAC2470                      (GB02MAC4 * 4)
#define GB02MAC2471                      -1
#define GB02MAC2472                  2
#define GB02MAC2473             0x0000
#define GB02MAC2474           0XFFFF
#define GB02MAC2475          0XFFFF
#define GB02MAC2476              0XFFFF

#define GB02MAC2477                      0xb3000
#define GB02MAC2478                      (GB02MAC4 * 4)
#define GB02MAC2479                      -1
#define GB02MAC2480                  2
#define GB02MAC2481             0x0000
#define GB02MAC2482           0XFFFF
#define GB02MAC2483          0XFFFF
#define GB02MAC2484              0XFFFF

//JPEG encoder vcmd configuration

#define GB02MAC2485                      0x90000
#define GB02MAC2486                      (GB02MAC4 * 4)
#define GB02MAC2487                      -1
#define GB02MAC2488                  3
#define GB02MAC2489             0x1000
#define GB02MAC2490           0XFFFF //0X4000
#define GB02MAC2491          0XFFFF
#define GB02MAC2492             0XFFFF //0X2000
#define GB02MAC2493             0XFFFF
#define GB02MAC2494           0XFFFF //0X3000
#define GB02MAC2495           0XFFFF

#define GB02MAC2496                      0xC1000
#define GB02MAC2497                      (GB02MAC4 * 4)
#define GB02MAC2498                      -1
#define GB02MAC2499                  3
#define GB02MAC2500             0x0000
#define GB02MAC2501           0XFFFF
#define GB02MAC2502          0XFFFF
#define GB02MAC2503              0XFFFF

#define GB02MAC2504                      0xC2000
#define GB02MAC2505                      (GB02MAC4 * 4)
#define GB02MAC2506                      -1
#define GB02MAC2507                  3
#define GB02MAC2508             0x0000
#define GB02MAC2509           0XFFFF
#define GB02MAC2510          0XFFFF
#define GB02MAC2511              0XFFFF

#define GB02MAC2512                      0xC3000
#define GB02MAC2513                      (GB02MAC4 * 4)
#define GB02MAC2514                      -1
#define GB02MAC2515                  3
#define GB02MAC2516             0x0000
#define GB02MAC2517           0XFFFF
#define GB02MAC2518          0XFFFF
#define GB02MAC2519              0XFFFF

//JPEG decoder vcmd configuration

#define GB02MAC2520                      0xD0000
#define GB02MAC2521                      (GB02MAC4 * 4)
#define GB02MAC2522                      -1
#define GB02MAC2523                  4
#define GB02MAC2524             0x0000
#define GB02MAC2525           0XFFFF
#define GB02MAC2526          0XFFFF
#define GB02MAC2527              0XFFFF

#define GB02MAC2528                      0xD1000
#define GB02MAC2529                      (GB02MAC4 * 4)
#define GB02MAC2530                      -1
#define GB02MAC2531                  4
#define GB02MAC2532             0x0000
#define GB02MAC2533           0XFFFF
#define GB02MAC2534          0XFFFF
#define GB02MAC2535              0XFFFF

#define GB02MAC2536                      0xD2000
#define GB02MAC2537                      (GB02MAC4 * 4)
#define GB02MAC2538                      -1
#define GB02MAC2539                  4
#define GB02MAC2540             0x0000
#define GB02MAC2541           0XFFFF
#define GB02MAC2542          0XFFFF
#define GB02MAC2543              0XFFFF

#define GB02MAC2544                      0xD3000
#define GB02MAC2545                      (GB02MAC4 * 4)
#define GB02MAC2546                      -1
#define GB02MAC2547                  4
#define GB02MAC2548             0x0000
#define GB02MAC2549           0XFFFF
#define GB02MAC2550          0XFFFF
#define GB02MAC2551              0XFFFF
#endif
#define NETINT

/* Use 'k' as magic number */
#define GB02MAC2552  'k'
#define GB02MAC1066    'm'

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

enum {
	CORE_VC8000E = 0,
	CORE_VC8000EJ = 1,
	CORE_CUTREE = 2,
	CORE_DEC400 = 3,
	CORE_MMU = 4,
	CORE_L2CACHE = 5,
	CORE_AXIFE = 6,
	CORE_APBFT = 7,
	CORE_MMU_1 = 8,
	CORE_AXIFE_1 = 9,
	CORE_MAX
};

typedef struct {
	u32 type_info;
	unsigned long offset[CORE_MAX];
	unsigned long regSize[CORE_MAX];
	int irq[CORE_MAX];
} SUBSYS_CORE_INFO;

typedef struct CoreWaitOut {
	u32 job_id[4];
	u32 irq_status[4];
	u32 irq_num;
} CORE_WAIT_OUT;

struct GB02STR184 {
	u16 module_type; /* in vc8000e=0,cutree=1,vc8000d=2,jpege=3,jpegd=4 */
	u16 vcmd_core_num;
	u16 submodule_main_addr;
	u16 submodule_dec400_addr;
	u16 submodule_L2Cache_addr;
	u16 submodule_MMU_addr[2];
	u16 submodule_axife_addr[2];
	u16 config_status_cmdbuf_id;
	u32 vcmd_hw_version_id;
};

#define HANTRO_IOCG_HWOFFSET  _IOR(GB02MAC2552,  3, unsigned long *)
#define HANTRO_IOCG_HWIOSIZE  _IOR(GB02MAC2552,  4, unsigned int *)
#define HANTRO_IOC_CLI        _IO(GB02MAC2552,  5)
#define HANTRO_IOC_STI        _IO(GB02MAC2552,  6)
#define HANTRO_IOCX_VIRT2BUS  _IOWR(GB02MAC2552, 7, unsigned long *)
#define HANTRO_IOCH_ARDRESET  _IO(GB02MAC2552, 8)
#define HANTRO_IOCG_SRAMOFFSET  _IOR(GB02MAC2552,  9, unsigned long *)
#define HANTRO_IOCG_SRAMEIOSIZE  _IOR(GB02MAC2552,  10, unsigned int *)
#define HANTRO_IOCH_ENC_RESERVE  _IOR(GB02MAC2552, 11, unsigned int *)
#define HANTRO_IOCH_ENC_RELEASE  _IOR(GB02MAC2552, 12, unsigned int *)
#define HANTRO_IOCG_CORE_NUM     _IOR(GB02MAC2552, 13, unsigned int *)
#define HANTRO_IOCG_CORE_INFO   _IOR(GB02MAC2552, 14, SUBSYS_CORE_INFO *)
#define HANTRO_IOCG_CORE_WAIT    _IOR(GB02MAC2552, 15, unsigned int *)
#define HANTRO_IOCG_ANYCORE_WAIT  _IOR(GB02MAC2552, 16, CORE_WAIT_OUT *)

#define HANTRO_IOCH_GET_CMDBUF_PARAMETER\
	_IOWR(GB02MAC2552, 25, struct GB02STR16 *)
#define HANTRO_IOCH_GET_CMDBUF_POOL_SIZE\
	_IOWR(GB02MAC2552, 26, unsigned long)
#define HANTRO_IOCH_SET_CMDBUF_POOL_BASE\
	_IOWR(GB02MAC2552, 27, unsigned long)
#define HANTRO_IOCH_GET_VCMD_PARAMETER\
	_IOWR(GB02MAC2552, 28, struct GB02STR184 *)
#define HANTRO_IOCH_LINK_RUN_CMDBUF   _IOR(GB02MAC2552, 30, u16 *)
#define HANTRO_IOCH_WAIT_CMDBUF       _IOR(GB02MAC2552, 31, u16 *)
#define HANTRO_IOCH_RELEASE_CMDBUF    _IOR(GB02MAC2552, 32, u16 *)
#define HANTRO_IOCH_POLLING_CMDBUF    _IOR(GB02MAC2552, 33, u16 *)
struct GB02STR185 {
	u32 hw_id;
	u32 cfg1val;
	u32 cfg2val;
	u32 cfg3val;
	u32 cfg4val;
	u32 cfg5val;
	u32 cfgaxi;
};
#define HANTRO_IOCH_GET_HWINFO_FROM_VCMD\
	_IOWR(GB02MAC2552, 34, struct GB02STR185 *)
#define HANTRO_IOCH_GET_VCMD_ENABLE _IOWR(GB02MAC2552, 50, unsigned long)

#define GB02MAC2553 60
#define GB02MAC1067 4
struct GB02STR186 {
	int vcmd_type_core_num[MAX_VCMD_TYPE];
	u16 vcmd_position[MAX_VCMD_TYPE];
	u16 total_vcmd_core_num;
	int software_triger_abort;
	u16 enc_cmdbuf_used[GB02MAC159];
	u16 enc_cmdbuf_used_pos;
	u16 enc_cmdbuf_used_residual;
	bi_list_node *enc_global_cmdbuf_node[GB02MAC159];
	bi_list enc_global_process_manager;
	size_t enc_base_ddr_addr;
	struct GB02STR20 enc_pcie;
	struct GB02STR3 vcmd_buff;
	struct GB02STR185 hw_info;
	struct semaphore enc_vcmd_reserve_cmdbuf_sem[MAX_VCMD_TYPE];
	int enc_vcmd_type_core_num[MAX_VCMD_TYPE];
	/* 0:vcmd_buf_mem_pool; 1:vcmd_status_buf; 2:vcmd_registers */
	struct GB02STR14	enc_vcmd_mem_pool[3];
	spinlock_t enc_owner_lock_vcmd[GB02MAC152];
	wait_queue_head_t enc_wait_queue_vcmd[GB02MAC152];
	wait_queue_head_t enc_abort_queue_vcmd[GB02MAC152];
	struct GB02STR3 *vcmd_pool;
	struct GB02STR3 *vcmd_reg;
};

struct GB02STR186 *GB02FUNC1639(void);
void GB02FUNC1616(struct GB02STR7 *dev,
	bi_list_node *last_linked_cmdbuf_node);
void GB02FUNC1542(struct GB02STR7 *dev,
	bi_list_node *last_linked_cmdbuf_node);
void GB02FUNC1554(struct GB02STR7 *dev,
	bi_list_node *first_linked_cmdbuf_node);
void GB02FUNC1659(void);
int enc_open(struct drm_file *filp);
long GB02FUNC1634(struct drm_file *filp, unsigned int cmd,
	void *arg);
int GB02FUNC1623(struct drm_file *filp, void *enc_priv);
int GB02FUNC1566(struct drm_file *filp,
	int cmdbuf_id, int cmdbuf_size, int *core_id);
unsigned int GB02FUNC1582(struct drm_file *filp, u32 cmdbuf_id,
	u32 *irq_status_ret);
int GB02FUNC1618(struct drm_file *filp, void **enc_priv,
	void *data);
int GB02FUNC1632(struct drm_file *filp, void *data);
long GB02FUNC1630(struct drm_file *filp, u16 cmdbuf_id);
int GB02FUNC1655(int irq_id);
void GB02FUNC1607(void);
void GB02FUNC1610(void);
#endif



