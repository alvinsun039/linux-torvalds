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

#ifndef __GB_MMU_H__
#define __GB_MMU_H__
#include <drm/drm_mm.h>
#define GB02MAC1192 		(2 * 1024 * 1024)
#define GB02MAC1193 512
#define GB02MAC1194    0
#define GB02MAC1195 3

#define GB02MAC1196 (3 << 24)
#define GB02MAC1197 (1 << 24)
#define GB02MAC1198 (2 << 24)

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

#define GB02MAC1210      0
#define GB02MAC1212    1
#define GB02MAC1214    2
#define GB02MAC1216  6
#define GB02MAC1218 8

/*
 * Begin AARCH64 MMU TRANSTAB register values
 */
#define GB02MAC1220 40
#define GB02MAC1221 ((1ULL << GB02MAC1220) - (1ULL << 4))

#define GB02MAC1224 0x2600
#define GB02MAC1226	(512UL << 20) // 512MB
#define GB02MAC1227	(1ULL << 30)  // 1G
#define GB02MAC1229	(3ULL << 30)  // 3G
#define GB02MAC1231    (4ULL << 30)  // 4G
#define GB02MAC1233 ((1ULL << 48) - GB02MAC1231) // 48 bit 256TB

#define GB02MAC1234 1 << 0
#define GB02MAC1236 1 << 1

/* gpu mmu flag start */
/* Free region */
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
#define GB02MAC1254          (1ul << 9)
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

#define GB02MAC1264     3ULL
/* For valid ATEs bit 1 = (level == 3) ? 1 : 0.
 * The MMU is only ever configured by the driver so that ATEs
 * are at level 3, so bit 1 should always be set
 */
#define GB02MAC1265        3ULL
#define GB02MAC1266      2ULL
#define GB02MAC1267        3ULL

#define GB02MAC1268 (7ULL << 2)	/* bits 4:2 */
#define GB02MAC1269 (1ULL << 6)     /* bits 6:7 */
#define GB02MAC1270 (3ULL << 6)
#define GB02MAC1271 (3ULL << 8)	/* bits 9:8 */
#define GB02MAC1272 (1ULL << 10)
#define GB02MAC1273 (1ULL << 54)
#define GB02MAC1274 1
#define GB02MAC1275 0
/* gpu mmu flag end */

struct GB02STR59;
struct GB02STR47;
struct gb_mmu;
struct GB02STR39;

void GB02FUNC933(struct GB02STR39 *gbdev, int enabled);
void GB02FUNC930(struct GB02STR39 *gbdev, int as_nr);
struct GB02STR59 *GB02FUNC949(struct GB02STR39 *gbdev, u64 addr);
void GB02FUNC937(struct GB02STR39 *gbdev);
void GB02FUNC972(struct GB02STR39 *gbdev);
int GB02FUNC974(struct GB02STR39 *gbdev);
int GB02FUNC935(void);

int GB02FUNC939(struct GB02STR39 *gbdev);
void GB02FUNC945(struct GB02STR39 *gbdev);
void GB02FUNC757(struct GB02STR39 *gbdev);
void GB02FUNC938(struct GB02STR39 *gbdev, int as, u64 iova, size_t size, bool sync);
int GB02FUNC921(struct GB02STR39 *gbdev, int as_nr, u64 iova, size_t size, u32 op);
int GB02FUNC923(struct GB02STR39 *gbdev, u64 iova, size_t size, u32 op);
struct GB02STR138 *GB02FUNC1010(void);

enum {
	GB_LOW_VA_MM,
	GB_HIGH_VA_MM,
	GB_MAX_VA_MM
};

struct GB02STR138 {
	phys_addr_t pgd;
	struct mutex mmu_lock;
	struct GB02STR39 *gbdev;
	u64 *mmu_teardown_pages;
	struct drm_mm mm[GB_MAX_VA_MM];
	struct mutex mm_lock;
	void (*mmu_controll)(struct GB02STR39 *gbdev, int enabled);
	void (*disable_as)(struct GB02STR39 *gbdev, int as_nr);
	void (*enable_as)(struct GB02STR39 *gbdev, int as_nr);
	phys_addr_t (*GB02FUNC995)(u64 entry);
	int (*GB02FUNC998)(u64 ate);
	int (*GB02FUNC1000)(u64 pte);
	void (*GB02FUNC1006)(u64 *entry, phys_addr_t phy, unsigned long flags);
	void (*GB02FUNC1008)(u64 *entry, phys_addr_t phy);
	void (*GB02FUNC1009)(u64 *entry);
};

#endif
