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
#include <generated/uapi/linux/version.h>
#include "gb_device.h"
#include "gb_mmu.h"

enum gb_share_attr_bits {
        /* (1ULL << 8) bit is reserved */
        SHARE_BOTH_BITS = (2ULL << 8),  /* inner and outer shareable coherency */
        SHARE_INNER_BITS = (3ULL << 8)  /* inner shareable coherency */
};


static inline void GB02FUNC991(u64 *pte, u64 phy)
{
	*pte = phy;
}

static void GB02FUNC992(struct GB02STR39 *gbdev, int as_nr)
{
}

static void GB02FUNC994(struct GB02STR39 *gbdev, int as_nr)
{
}

static phys_addr_t GB02FUNC995(u64 entry)
{
	if (!(entry & 1))
		return 0;

	return entry & ~(GB02MAC311 - 1);
}

static int GB02FUNC998(u64 ate)
{
	return ((ate & GB02MAC1264) == GB02MAC1265);
}

static int GB02FUNC1000(u64 pte)
{
	return ((pte & GB02MAC1264) == GB02MAC1267);
}

/*
 * Map GB_REG flags to MMU flags
 */
static u64 GB02FUNC1003(unsigned long flags)
{
	u64 mmu_flags = 0;

	// fixme: currently the flags parameter is always reset
	flags = GB02MAC1241 | GB02MAC1254;

	/* store mem_attr index as 4:2 (macro called ensures 3 bits already) */
	mmu_flags = GB02MAC1263(flags) << 2;

	/* Set access flags - note that AArch64 stage 1 does not support
	 * write-only access, so we use read/write instead
	 */
	if (flags & GB02MAC1241)
		mmu_flags |= GB02MAC1269;
	else if (flags & GB02MAC1259)
		mmu_flags |= GB02MAC1270;

	/* nx if requested */
	mmu_flags |= (flags & GB02MAC1243) ? GB02MAC1273 : 0;

	if (flags & GB02MAC1256) {
		/* inner and outer shareable */
		mmu_flags |= SHARE_BOTH_BITS;
	} else if (flags & GB02MAC1254) {
		/* inner shareable coherency */
		mmu_flags |= SHARE_INNER_BITS;
	}

	return mmu_flags;
}

static void GB02FUNC1006(u64 *entry, phys_addr_t phy, unsigned long flags)
{
	u64 mmu_flags = GB02FUNC1003(flags);
	GB02FUNC991(entry, (phy & ~0xFFF) | mmu_flags | GB02MAC1272 | GB02MAC1265);
}

static void GB02FUNC1008(u64 *entry, phys_addr_t phy)
{
	GB02FUNC991(entry, (phy & ~0xFFF) |
			GB02MAC1272 | GB02MAC1267);
}

static void GB02FUNC1009(u64 *entry)
{
	GB02FUNC991(entry, GB02MAC1266);
}

static struct GB02STR138 mmu64_mode = {
	.mmu_controll = GB02FUNC933,
	.disable_as = GB02FUNC992,
	.enable_as = GB02FUNC994,
	.GB02FUNC995 = GB02FUNC995,
	.GB02FUNC998 = GB02FUNC998,
	.GB02FUNC1000 = GB02FUNC1000,
	.GB02FUNC1006 = GB02FUNC1006,
	.GB02FUNC1008 = GB02FUNC1008,
	.GB02FUNC1009 = GB02FUNC1009
};

struct GB02STR138 *GB02FUNC1010(void)
{
	return &mmu64_mode;
}
