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

#include "common/gb_common.h"
#ifndef __GB_HDMAC_REGMAP_H__
#define __GB_HDMAC_REGMAP_H__

#define GB02MAC2717 GB02MAC503
#define GB02MAC2718 (10*MB)
#define GB02MAC2719 (GB02MAC503 + (GB02MAC491 - GB02MAC2718))

#define GB02MAC2720 0x210000000
#define GB02MAC2721 0x110000000

#define GB02MAC2722 48

#if GB02MAC2722 > 39
#define GB02MAC1194    0
#else
#define GB02MAC1194    1
#endif

#define GB02MAC1195 3


#define GB02MAC1193 512

#define GB02MAC313 (12)
#define GB02MAC311 (1 << GB02MAC313)
#define GB02MAC2723 (63)
#define GB02MAC2724 (11)

#define GB02MAC2725 0
#define GB02MAC2726 0

#if !GB02MAC2725
#define GB_DEBUG_PRINT_TRACE \
		"In file: " __FILE__
#if !GB02MAC2726
#define GB02MAC2727 __func__
#else
#define GB02MAC2727 ""
#endif
#else
#define GB_DEBUG_PRINT_TRACE ""
#endif

#define GB_DEBUG_ASSERT_OUT(trace, function, ...)\
		do { \
			gb_printf(KERN_ERR, "Botson<ASSERT>: %s function:%s ",\
			 trace, function);\
			gb_printf(KERN_ERR, __VA_ARGS__);\
			gb_printf(KERN_ERR, "\n");\
		} while (false)

#define GB_DEBUG_ASSERT(expr) \
	GB_DEBUG_ASSERT_MSG(expr, #expr)

#define GB_DEBUG_ASSERT_MSG(expr, ...) \
		do { \
			if (!(expr)) { \
				GB_DEBUG_ASSERT_OUT(GB_DEBUG_PRINT_TRACE,\
				 GB02MAC2727, __VA_ARGS__);\
				gb_printf(KERN_ERR, "Assert Error: inside %s %d\n",\
				 __func__, __LINE__); \
			} \
		} while (false)

enum gb_hdmac_mmu_fault_type {
	GB_MMU_FAULT_TYPE_UNKNOWN = 0,
	GB_MMU_FAULT_TYPE_PAGE,
	GB_MMU_FAULT_TYPE_BUS,
	GB_MMU_FAULT_TYPE_PAGE_UNEXPECTED,
	GB_MMU_FAULT_TYPE_BUS_UNEXPECTED
};

enum gb_hdmac_share_attr_bits {
	/* (1ULL << 8) bit is reserved */
	SHARE_BOTH_BITS = (2ULL << 8), /* inner and out shareable coherency */
	SHARE_INNER_BITS = (3ULL << 8) /* inner shareable coherency */
};

struct GB02STR225 {
	u64	transtab;
	u64	memattr;
	u64	transcfg;
};

struct GB02STR226 {
	int number;
	enum gb_hdmac_mmu_fault_type fault_type;
	bool protected_mode;
	u32 fault_status;
	u64 fault_addr;
	u64 fault_extra_addr;
	struct GB02STR225 current_setup;
};

struct GB02STR228 {
	pid_t tgid;
	pid_t pid;
	int as_nr;
	phys_addr_t pgd;
	struct GB02STR229 *mmu_mode;
	u64 fault_cpu_addr;
	u64 fault_gpu_addr;
};

struct GB02STR229 {
	void (*update)(struct GB02STR228 *gbctx);
	void (*get_as_setup)(struct GB02STR228 *gbctx,
			struct GB02STR225 * const setup);
	void (*disable_as)(struct GB02STR228 *gbctx);
	phys_addr_t (*GB02FUNC995)(u64 entry);
	int (*GB02FUNC998)(u64 ate);
	int (*GB02FUNC1000)(u64 pte);
	void (*GB02FUNC1006)(u64 *entry, phys_addr_t phy, unsigned long flags);
	void (*GB02FUNC1008)(u64 *entry, phys_addr_t phy);
	void (*GB02FUNC1009)(u64 *entry);
};

/*
 * Maximum number of loops polling the GPU for a cache flush before
 * we assume it must have completed
 */
#define GB02MAC2729     100000
/*
 * Maximum number of loops polling the GPU for an AS command to complete
 * before we assume the GPU has hung
 */
#define GB02MAC2731     10


/* region flags */
#define GB02MAC1238              (1ul << 0)
/* CPU write access */
#define GB02MAC1240            (1ul << 1)
/* GPU write access */
#define GB02MAC1241            (1ul << 2)
/* No eXecute flag */
#define GB02MAC1243            (1ul << 3)
/* Is CPU cached? */
#define GB02MAC1245        (1ul << 4)
/* Is GPU cached? */
#define GB02MAC1247        (1ul << 5)

#define GB02MAC1249          (1ul << 6)
/* Can grow on pf? */
#define GB02MAC1251           (1ul << 7)

/* VA managed by us */
#define GB02MAC1253         (1ul << 8)

/* inner shareable coherency */
#define GB02MAC1254          (1ul << 9)
/* inner & outer shareable coherency */
#define GB02MAC1256        (1ul << 10)

/* Space for 4 different zones */
#define GB02MAC1257         (3ul << 11)
#define GB02MAC1258(x)           (((x) & 3) << 11)

/* GPU read access */
#define GB02MAC1259            (1ul<<13)
/* CPU read access */
#define GB02MAC1260            (1ul<<14)

/* Index of chosen MEMATTR for this region (0..7) */
#define GB02MAC1261      (7ul << 16)
#define GB02MAC1262(x)  (((x) & 7) << 16)
#define GB02MAC1263(x)  (((x) & GB02MAC1261) >> 16)

#define GB02MAC2740            (1ul << 19)

#define GB02MAC2742         (1ul << 20)

/* hdmA reg, just for read to debug */
#define GB02MAC2743		0x20
#define GB02MAC2744		0x24
#define GB02MAC2745		0x28
#define GB02MAC2747		0x2c
#define GB02MAC2749			0x1c
#define GB02MAC2751			0x00

#define GB02MAC2754		0x120
#define GB02MAC2755		0x124
#define GB02MAC2756		0x128
#define GB02MAC2757		0x12c
#define GB02MAC2758			0x11c
#define GB02MAC2759			0x100

#define GB02MAC2760(i)		(GB02MAC2743 + 0x200 * (i))
#define GB02MAC2761(i)		(GB02MAC2744 + 0x200 * (i))
#define GB02MAC2762(i)		(GB02MAC2745 + 0x200 * (i))
#define GB02MAC2763(i)		(GB02MAC2747 + 0x200 * (i))
#define GB02MAC2764(i)		(GB02MAC2749 + 0x200 * (i))
#define GB02MAC2765(i)		(GB02MAC2751 + 0x200 * (i))

#define GB02MAC2766(i)		(GB02MAC2754 + 0x200 * (i))
#define GB02MAC2767(i)		(GB02MAC2755 + 0x200 * (i))
#define GB02MAC2768(i)		(GB02MAC2756 + 0x200 * (i))
#define GB02MAC2769(i)		(GB02MAC2757 + 0x200 * (i))
#define GB02MAC2770(i)		(GB02MAC2758 + 0x200 * (i))
#define GB02MAC2771(i)		(GB02MAC2759 + 0x200 * (i))

#if 1
#define GB02MAC2772 0x84
#define GB02MAC2773 0x184
#define GB02MAC2774(i) (GB02MAC2772 + 0x200 * (i))
#define GB02MAC2775(i) (GB02MAC2773 + 0x200 * (i))
#else
#define GB02MAC2772 0x84
#define GB02MAC2773 0x284
#define GB02MAC2774(i) (GB02MAC2772 + 0x400 * (i))
#define GB02MAC2775(i) (GB02MAC2773 + 0x400 * (i))
#endif

/* hdmA reg end */

#define GB02MAC2776		4
#define GB02MAC2777	0xfffffffe
#define GB02MAC2778		0xfffffffd

/* hdmaC reg map start */
#define GB02MAC2779				0x2000
#define GB02MAC2780					(GB02MAC2779 + 0x20)
/* bit0:set 0 to softreset mmu; bit1:set 0 to softreset hdmac */
#define GB02MAC2781				(GB02MAC2779 + 0x24)
/* 0~7, totally 8 AS */
#define GB02MAC2782				(GB02MAC2779 + 0x30)
//#define DMA_CHANNEL				(GB02MAC2779 + 0x5c)
#define GB02MAC2783			(GB02MAC2779 + 0x48)
#define GB02MAC2784			(GB02MAC2779 + 0x4c)
#define GB02MAC2785			(GB02MAC2779 + 0x50)
#define GB02MAC2786			(GB02MAC2779 + 0x54)
#define GB02MAC2787				(GB02MAC2779 + 0x58)
#define GB02MAC2788				(GB02MAC2779 + 0x44)
/*
 * must be set at last... bit0 = 1:start dma;
 * bit31 = 1:dma read; bit31 = 0:dma_write
 */
#define GB02MAC2789				(GB02MAC2779 + 0x40)

#define GB02MAC2790			(GB02MAC2779 + 0x60)
#define GB02MAC2791		(GB02MAC2779 + 0x64)
#define GB02MAC2792		(GB02MAC2779 + 0x68)
#define GB02MAC2793			(GB02MAC2779 + 0x6c)
#define GB02MAC2794			(GB02MAC2779 + 0x70)

#define GB02MAC2795	(GB02MAC2779 + 0x80)
#define GB02MAC2796	(GB02MAC2779 + 0x84)
#define GB02MAC2797(r)    (GB02MAC2795 + (8 * (r)))
#define GB02MAC2798(r)    (GB02MAC2796 + (8 * (r)))

#define GB02MAC2799	(GB02MAC2779 + 0x100)
#define GB02MAC2800	(GB02MAC2779 + 0x104)
#define GB02MAC2801(r)    (GB02MAC2799 + (8 * (r)))
#define GB02MAC2802(r)    (GB02MAC2800 + (8 * (r)))

#define GB02MAC2803		(GB02MAC2779 + 0x180)
#define GB02MAC2804(r)     (GB02MAC2803 + (4 * (r)))

#define GB02MAC2805	(GB02MAC2779 + 0x1c0)

/* 0 dianping; 1maichong  */
#define GB02MAC2806		(GB02MAC2779 + 0x1c4)

#define GB02MAC2807		(GB02MAC2779 + 0x1c8)

/* enable hdmac to clear hdma irq */
#define GB02MAC2808	(GB02MAC2779 + 0x1cc)

/* hdmaC reg map end */

/* mmu reg map */
#define GB02MAC1536  0x0
#define GB02MAC1538(r)              (GB02MAC1536 + (r))

#define GB02MAC1549     0x0	/* Configuration registers for address space 0 */

#define MMU_AS_REG(n, r)        (GB02MAC1538(GB02MAC1549 + ((n) << 6)) + (r))

/* (RW) Translation Table Base Address for address space n, low word */
#define GB02MAC2809			0x00
/* (RW) Translation Table Base Address for address space n, high word */
#define GB02MAC2810			0x04
/* (RW) Memory attributes for address space n, low word. */
#define GB02MAC2811			0x08
/* (RW) Memory attributes for address space n, high word. */
#define GB02MAC2812			0x0C
/* (RW) Lock region address for address space n, low word */
#define GB02MAC2813			0x10
/* (RW) Lock region address for address space n, high word */
#define GB02MAC2814			0x14

#define GB02MAC1580	0x18 /* (WO) MMU command register for address space n */
/* (RO) MMU fault status register for address space n */
#define GB02MAC1582 0x1C
/* (RO) Fault Address for address space n, low word */
#define GB02MAC2815		0x20
/* (RO) Fault Address for address space n, high word */
#define GB02MAC2816		0x24
#define GB02MAC1585	0x28	/*(RO) Status flags for address space n*/


/* (RW) Translation table configuration for address space n, low word */
#define GB02MAC2817			0x30
/* (RW) Translation table configuration for address space n, high word */
#define GB02MAC2818			0x34
/* (RO) Secondary fault address for address space n, low word */
#define GB02MAC2819		0x38
/* (RO) Secondary fault address for address space n, high word */
#define GB02MAC2820		0x3C

#define GB02MAC1540		0x400	/* (RW) Raw interrupt status register */
#define GB02MAC1542		0x404	/* (WO) Interrupt clear register */
#define GB02MAC1544		0x408	/* (RW) Interrupt mask register */
#define GB02MAC1546		0x40C	/* (RO) Interrupt status register */

/* End Register Offsets */

/*
 * GB02MAC1540 register values. Values are valid also for
   GB02MAC1542, GB02MAC1544, GB02MAC1546 registers.
 */

#define GB02MAC1603   16

/* Macros returning a bitmask to retrieve page fault or bus error flags from
 * MMU registers
 */
#define GB02MAC1605(n)      (1UL << (n))
#define GB02MAC1607(n)       (1UL << ((n) + GB02MAC1603))

/*
 * Begin LPAE MMU TRANSTAB register values
 */
#define GB02MAC1608   0xfffff000
#define GB02MAC1609  (0u << 0)
#define GB02MAC1610  (1u << 1)
#define GB02MAC1611     (3u << 0)
#define GB02MAC1613        (1u << 2)
#define GB02MAC1615       (1u << 4)

#define GB02MAC1616      0x00000003

/*
 * Begin AARCH64 MMU TRANSTAB register values
 */
#define GB02MAC1220 40
#define GB02MAC1221 ((1ULL << GB02MAC1220) - (1ULL << 4))

/*
 * Begin MMU STATUS register values
 */
#define GB02MAC1620 0x01

#define GB02MAC1622                    (0x7<<3)
#define GB02MAC1624       (0x0<<3)
#define GB02MAC1626        (0x1<<3)
#define GB02MAC1628      (0x2<<3)
#define GB02MAC1630             (0x3<<3)

#define GB02MAC1632      (0x4<<3)
#define GB02MAC1634 (0x5<<3)

#define GB02MAC1644                  (0x3<<8)
#define GB02MAC1646                (0x0<<8)
#define GB02MAC1648                    (0x1<<8)
#define GB02MAC1650                  (0x2<<8)
#define GB02MAC1652                 (0x3<<8)

/*
 * Begin MMU TRANSCFG register values
 */

#define GB02MAC1210      0
#define GB02MAC1212    1
#define GB02MAC1214    2
#define GB02MAC1216  6
#define GB02MAC1218 8

#define GB02MAC1661        0xF


/*
 * Begin TRANSCFG register values
 */
#define GB02MAC1196 (3 << 24)
#define GB02MAC1197 (1 << 24)
#define GB02MAC1198 (2 << 24)

#define GB02MAC1671 ((3 << 28))
#define GB02MAC1674 (2 << 28)
#define GB02MAC1677 (3 << 28)

/* GB02MAC1580 register commands */
#define GB02MAC1708         0x00	/* NOP Operation */
/* Broadcasts the values in AS_TRANSTAB and ASn_MEMATTR to all MMUs */
#define GB02MAC1711      0x01
#define GB02MAC1714	0x02 /* Issue a lock region command to all MMUs */
#define GB02MAC1717 0x03 /* Issue a flush region command to all MMUs */
/*
 * Flush all L2 caches then issue a flush region command to all MMUs
 * (deprecated - only for use with T60x)
 */
#define GB02MAC1720 0x04
/* Flush all L2 caches then issue a flush region command to all MMUs */
#define GB02MAC1724    0x04
/*
 * Wait for memory accesses to complete, flush all the L1s cache then
 * flush all L2 caches then issue a flush region command to all MMUs
 */
#define GB02MAC2821   0x05

/* AS<n>_MEMATTR values: */
/* Use GPU implementation-defined caching policy. */
#define GB02MAC1199 0x88ull
/* The attribute set to force all resources to be cached. */
#define GB02MAC1200    0x8Full
/* Inner write-alloc cache setup, no outer caching */
#define GB02MAC1201           0x8Dull

/* Set to implementation defined, outer caching */
#define GB02MAC1202 0x88ull
/* Set to write back memory, outer caching */
#define GB02MAC1203       0x8Dull

/* HW implementation defined caching */
#define GB02MAC1204 0
/* Force cache on */
#define GB02MAC1205    1
/* Write-alloc */
#define GB02MAC1206           2
/* Outer coherent, inner implementation defined policy */
#define GB02MAC1207        3
/* Outer coherent, write alloc inner */
#define GB02MAC1208              4
struct GB02STR229 *GB02FUNC1824(void);
phys_addr_t GB02FUNC1778(struct GB02STR228 *gbctx);
void GB02FUNC1780(void);
phys_addr_t GB02FUNC1772(void);
void GB02FUNC1775(void);
void GB02FUNC1800(void);
int GB02FUNC1805(struct GB02STR228 *gbctx, size_t nr_pages,
	bool is_vram, phys_addr_t *phys, u64 *va);
int GB02FUNC1828(u64 size, phys_addr_t *src_phys, phys_addr_t *dst_phys,
	bool is_read, int channel_id, bool is_done,
	u64 abort_pg_num, u64 cpu_virt, u64 gpu_virt);
void GB02FUNC1825(void);
u32 GB02FUNC1752(u32 offset);
void GB02FUNC1751(u32 offset, u32 value);
void GB02FUNC1802(void);
void GB02FUNC1804(void);
void GB02FUNC1836(void);
u32 GB02FUNC1838(void);
#endif
