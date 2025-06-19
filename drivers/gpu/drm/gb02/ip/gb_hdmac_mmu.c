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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/gfp.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include "gb_hdmac.h"
#include "gb_hdmac_regmap.h"
#include "common/gb_pcie_info.h"
#include "common/gb_common.h"

static DEFINE_MUTEX(hdmac_gpu_page_alloc_lock);
static DEFINE_MUTEX(hdmac_pgd_alloc_lock);
static DEFINE_MUTEX(hdmac_gpu_vpfn_alloc_lock);
static DEFINE_MUTEX(hdmac_cpu_vpfn_alloc_lock);
static DEFINE_MUTEX(hdmac_start_lock);
static DEFINE_MUTEX(hdmac_done_lock);
static DEFINE_MUTEX(hdmac_abort_clear_lock);
static DEFINE_MUTEX(hdmac_status_clear_lock);
static DEFINE_MUTEX(hdmac_int_status_lock);
u64 gb_hdmac_pgt_offset;
u64 gb_hdmac_gpu_page_offset;
u64 gb_hdmac_gpu_vpfn_offset;
u64 gb_hdmac_cpu_vpfn_offset;

extern struct GB02STR202 *dma_infos;
void GB02FUNC1751(u32 offset, u32 value)
{
	iowrite32(value, dma_infos->regs + offset);
	hdmac_debug("writereg 0x%x 0x%x\n", offset, value);
	udelay(50);
}

u32 GB02FUNC1752(u32 offset)
{
	u32 val = 0;

	val = ioread32(dma_infos->regs + offset);
	hdmac_debug("readreg 0x%x 0x%x\n", offset, val);
	return val;
}

void GB02FUNC1755(u32 offset, u32 value)
{
	iowrite32(value, dma_infos->hdma_regs + offset);
	hdmac_debug("write hdmA reg:0x%x:0x%x\n", offset, value);
}

u32 GB02FUNC1756(u32 offset)
{
	u32 val = 0;

	val = ioread32(dma_infos->hdma_regs + offset);
	hdmac_debug("read hdmA reg:0x%x:0x%x\n", offset, val);
	return val;
}

static inline u64 GB02FUNC916(u64 pfn,
		u32 num_pages)
{
	u64 region;

	/* can't lock a zero sized range */
	GB_DEBUG_ASSERT(num_pages);

	region = pfn << GB02MAC313;
	/*
	 * fls returns (given the ASSERT above):
	 * 1 .. 32
	 *
	 * 10 + fls(num_pages)
	 * results in the range (11 .. 42)
	 */

	/* gracefully handle num_pages being zero */
	if (num_pages == 0)
		region |= 11;
	else {
		u8 region_width;

		region_width = 10 + fls(num_pages);
		if (num_pages != (1ul << (region_width - 11))) {
			/* not pow2, so must go up to the next pow2 */
			region_width += 1;
		}
		GB_DEBUG_ASSERT(region_width <= GB02MAC2723);
		GB_DEBUG_ASSERT(region_width >= GB02MAC2724);
		region |= region_width;
	}
	return region;
}

static int GB02FUNC911(unsigned int as_nr)
{
	unsigned int max_loops = GB02MAC2731;
	u32 val = GB02FUNC1752(MMU_AS_REG(as_nr, GB02MAC1585));

	/* Wait for the MMU status to indicate there is no active command, in
	 * case one is pending. Do not log remaining register accesses.
	 */
	while (--max_loops && (val & GB02MAC1620)) {
		val = GB02FUNC1752(MMU_AS_REG(as_nr, GB02MAC1585));
		msleep(500);
	}

	if (max_loops == 0)
		return -1;

	/* If waiting in loop was performed, log last read value. */
	if (GB02MAC2731 - 1 > max_loops)
		GB02FUNC1752(MMU_AS_REG(as_nr, GB02MAC1585));

	return 0;
}

static int GB02FUNC914(int as_nr, u32 cmd)
{
	int status;

	/* write GB02MAC1580 when MMU is ready to accept another command */
	status = GB02FUNC911(as_nr);
	if (status == 0)
		GB02FUNC1751(MMU_AS_REG(as_nr, GB02MAC1580), cmd);
	else
		gb_printf(KERN_ERR, "%s %d BUG...\n", __func__, __LINE__);

	return status;
}

void GB02FUNC1761(struct GB02STR226 *as)
{
	struct GB02STR225 *current_setup = &as->current_setup;
	u32 transcfg = 0;

	transcfg = current_setup->transcfg & 0xFFFFFFFFUL;

	/* Set flag GB02MAC1198 */
	/* Clear PTW_MEMATTR bits */
	transcfg &= ~GB02MAC1196;
	/* Enable correct PTW_MEMATTR bits */
	transcfg |= GB02MAC1198;

	GB02FUNC1751(MMU_AS_REG(as->number, GB02MAC2817),
			transcfg);
	GB02FUNC1751(MMU_AS_REG(as->number, GB02MAC2818),
			(current_setup->transcfg >> 32) & 0xFFFFFFFFUL);

	GB02FUNC1751(MMU_AS_REG(as->number, GB02MAC2809),
			current_setup->transtab & 0xFFFFFFFFUL);
	GB02FUNC1751(MMU_AS_REG(as->number, GB02MAC2810),
			(current_setup->transtab >> 32) & 0xFFFFFFFFUL);

	GB02FUNC1751(MMU_AS_REG(as->number, GB02MAC2811),
			current_setup->memattr & 0xFFFFFFFFUL);
	GB02FUNC1751(MMU_AS_REG(as->number, GB02MAC2812),
			(current_setup->memattr >> 32) & 0xFFFFFFFFUL);

	GB02FUNC914(as->number, GB02MAC1711);
}

static int GB02FUNC1764(int as_nr, u64 vpfn, u32 nr,
	u32 op, unsigned int handling_irq)
{
	int ret;

	if (op == GB02MAC1717)
		/* Unlock doesn't require a lock first */
		ret = GB02FUNC914(as_nr, GB02MAC1717);
	else {
		u64 lock_addr = GB02FUNC916(vpfn, nr);

		/* Lock the region that needs to be updated */
		GB02FUNC1751(MMU_AS_REG(as_nr, GB02MAC2813),
		 lock_addr & 0xFFFFFFFFUL);
		GB02FUNC1751(MMU_AS_REG(as_nr, GB02MAC2814),
		 (lock_addr >> 32) & 0xFFFFFFFFUL);
		GB02FUNC914(as_nr, GB02MAC1714);

		/* Run the MMU operation */
		GB02FUNC914(as_nr, op);

		/* Wait for the flush to complete */
		ret = GB02FUNC911(as_nr);
	}

	return ret;
}

u64 *GB02FUNC1769(phys_addr_t phy_add)
{
	u64 *virt;

	virt = (u64 *)(dma_infos->vram_addr + phy_add);
	return virt;
}

phys_addr_t GB02FUNC1772(void)
{
	phys_addr_t target_page;

	mutex_lock(&hdmac_gpu_page_alloc_lock);
	target_page = GB02MAC2717 + gb_hdmac_gpu_page_offset;
	gb_hdmac_gpu_page_offset += (1 << GB02MAC313);
	mutex_unlock(&hdmac_gpu_page_alloc_lock);

	return target_page;
}

void GB02FUNC1775(void)
{
	gb_hdmac_gpu_page_offset = 0;
}

phys_addr_t GB02FUNC1778(struct GB02STR228 *gbctx)
{
	u64 *page;
	phys_addr_t target_pgd;
	int i = 0;

	mutex_lock(&hdmac_pgd_alloc_lock);
	target_pgd = GB02MAC2719 + gb_hdmac_pgt_offset;
	gb_hdmac_pgt_offset += (1 << GB02MAC313);

	page = GB02FUNC1769(target_pgd);
	memset_io(page, 0, GB02MAC311);
	for (i = 0; i < GB02MAC1193; i++)
		gbctx->mmu_mode->GB02FUNC1009(&page[i]);
	mutex_unlock(&hdmac_pgd_alloc_lock);

	return target_pgd;
}

void GB02FUNC1780(void)
{
	gb_hdmac_pgt_offset = 0;
}

static int GB02FUNC758(struct GB02STR228 *gbctx,
		phys_addr_t *pgd, u64 vpfn, int level)
{
	u64 *page;
	phys_addr_t target_pgd;

	/*
	 * Architecture spec defines level-0 as being the top-most.
	 * This is a bit unfortunate here, but we keep the same convention.
	 */
	vpfn >>= (3 - level) * 9;
	vpfn &= 0x1FF;

	page = GB02FUNC1769(*pgd);
	if (page == NULL) {
		gb_printf(KERN_ERR, "%s: GB02FUNC1769 failure\n", __func__);
		return -EINVAL;
	}

	target_pgd = gbctx->mmu_mode->GB02FUNC995(page[vpfn]);

	if (!target_pgd) {
		target_pgd = GB02FUNC1778(gbctx);
		if (!target_pgd) {
			gb_printf(KERN_ERR, "%s: GB02FUNC1778 failure\n",
			 __func__);
			return -ENOMEM;
		}

		gbctx->mmu_mode->GB02FUNC1008(&page[vpfn], target_pgd);
	}

	*pgd = target_pgd;
	return 0;
}

static int GB02FUNC763(struct GB02STR228 *gbctx,
	u64 vpfn, phys_addr_t *out_pgd)
{
	phys_addr_t pgd;
	int l;

	pgd = gbctx->pgd;
	for (l = GB02MAC1194; l < GB02MAC1195; l++) {
		int err = GB02FUNC758(gbctx, &pgd, vpfn, l);

		/* Handle failure condition */
		if (err) {
			gb_printf(KERN_ERR, "%s: GB02FUNC758 failure\n", __func__);
			return err;
		}
	}

	*out_pgd = pgd;

	return 0;
}

int GB02FUNC1787(struct GB02STR228 *gbctx, u64 vpfn,
	phys_addr_t *phys, size_t nr, unsigned long flags)
{
	phys_addr_t pgd;
	u64 *pgd_page;
	/*
	 * In case the insert_pages only partially completes we need to be able
	 * to recover
	 */
	size_t remain = nr;
	int err;

	GB_DEBUG_ASSERT(vpfn != 0);

	/* Early out if there is nothing to do */
	if (nr == 0)
		return 0;

	while (remain) {
		unsigned int i;
		unsigned int index = vpfn & 0x1FF;
		unsigned int count = GB02MAC1193 - index;

		if (count > remain)
			count = remain;

		/*
		 * Repeatedly calling mmu_get_bottom_pte() is clearly
		 * suboptimal. We don't have to re-parse the whole tree
		 * each time (just cache the l0-l2 sequence).
		 * On the other hand, it's only a gain when we map more than
		 * 256 pages at once (on average). Do we really care?
		 */
		do {
			err = GB02FUNC763(gbctx, vpfn, &pgd);
			if (err != -ENOMEM)
				break;
		} while (!err);
		if (err) {
			gb_printf(KERN_ERR, "GB02FUNC1793: GB02FUNC763 failure\n");
			goto fail_unlock;
		}

		pgd_page = GB02FUNC1769(pgd);
		if (!pgd_page) {
			gb_printf(KERN_ERR, "GB02FUNC1793: GB02FUNC1769 failure\n");
			err = -ENOMEM;
			goto fail_unlock;
		}
		for (i = 0; i < count; i++) {
			unsigned int ofs = index + i;

			GB_DEBUG_ASSERT(0 == (pgd_page[ofs] & 1UL));
			gbctx->mmu_mode->GB02FUNC1006(&pgd_page[ofs],
			 phys[i], flags);
		}

		phys += count;
		vpfn += count;
		remain -= count;
	}

	return 0;

fail_unlock:
	return err;
}

int GB02FUNC1793(struct GB02STR228 *gbctx, u64 vpfn,
	phys_addr_t *phys, size_t nr, unsigned long flags)
{
	int err;

	err = GB02FUNC1787(gbctx, vpfn, phys, nr, flags);
	err |= GB02FUNC1764(gbctx->as_nr,
	 vpfn, nr, GB02MAC1724, 0);
	return err;
}

u64 GB02FUNC1796(size_t nr_pages)
{
	u64 target_vpfn;

	mutex_lock(&hdmac_gpu_vpfn_alloc_lock);
	target_vpfn = (GB02MAC2721 >> GB02MAC313)
	 + gb_hdmac_gpu_vpfn_offset;
	gb_hdmac_gpu_vpfn_offset += nr_pages;
	mutex_unlock(&hdmac_gpu_vpfn_alloc_lock);

	return target_vpfn;
}

u64 GB02FUNC1798(size_t nr_pages)
{
	u64 target_vpfn;

	mutex_lock(&hdmac_cpu_vpfn_alloc_lock);
	target_vpfn = (GB02MAC2720 >> GB02MAC313)
	 + gb_hdmac_cpu_vpfn_offset;
	gb_hdmac_cpu_vpfn_offset += nr_pages;
	mutex_unlock(&hdmac_cpu_vpfn_alloc_lock);

	return target_vpfn;
}

void GB02FUNC1800(void)
{
	gb_hdmac_gpu_vpfn_offset = 0;
	gb_hdmac_cpu_vpfn_offset = 0;
}

void GB02FUNC1802(void)
{
	GB02FUNC1751(GB02MAC2781, GB02MAC2777);
	msleep(10);
}

void GB02FUNC1804(void)
{
	GB02FUNC1751(GB02MAC2781, GB02MAC2778);
	msleep(10);
}

int GB02FUNC1805(struct GB02STR228 *gbctx, size_t nr_pages,
	bool is_vram, phys_addr_t *phys, u64 *va)
{
	int err;
	u64 vpfn;

	if (is_vram)
		vpfn = GB02FUNC1796(nr_pages);
	else
		vpfn = GB02FUNC1798(nr_pages);

	*va = vpfn << GB02MAC313;
	/* set pagetable in vram */
	err = GB02FUNC1793(gbctx, vpfn,
			phys,
			nr_pages,
			GB02MAC1240 | GB02MAC1241 |
			 GB02MAC1260 | GB02MAC1259);
	if (err)
		gb_printf(KERN_ERR, "mmap fail vpfn:0x%llx, nr_page:0x%zx, phy_base:0x%llx",
		 vpfn, nr_pages, phys[0]);

	return err;
}

/* mmu set */
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

static inline void GB02FUNC991(u64 *pte, u64 phy)
{
	*pte = phy;
}

static void GB02FUNC1812(struct GB02STR228 *gbctx,
	  struct GB02STR225 *setup)
{
	/* Set up the required caching policies at the correct indices
	 * in the memattr register.
	 */
	setup->memattr = (GB02MAC1199 <<
	 (GB02MAC1204 * 8)) |
		(GB02MAC1200 <<
		 (GB02MAC1205 * 8)) |
		(GB02MAC1201	<<
		 (GB02MAC1206 * 8)) |
		(GB02MAC1202 <<
		 (GB02MAC1207 * 8)) |
		(GB02MAC1203 <<
		 (GB02MAC1208 * 8));

	setup->transtab = (u64)gbctx->pgd & GB02MAC1221;
	hdmac_debug("HDMAC setup->transtab: 0x%llx, as_base_mask:0x%llx\n",
	 setup->transtab, GB02MAC1221);

	setup->transcfg = GB02MAC1216;
}

static void GB02FUNC1815(struct GB02STR228 *gbctx)
{
	struct GB02STR226 as;

	memset(&as, 0, sizeof(struct GB02STR226));
	as.number = gbctx->as_nr;
	GB02FUNC1812(gbctx, &as.current_setup);

	/* Apply the address space setting */
	GB02FUNC1761(&as);
}

static void GB02FUNC992(struct GB02STR228 *gbctx)
{
	struct GB02STR226 as;

	memset(&as, 0, sizeof(struct GB02STR226));
	as.number = gbctx->as_nr;
	GB02FUNC1812(gbctx, &as.current_setup);

	as.current_setup.transtab = 0ULL;
	as.current_setup.transcfg = GB02MAC1212;

	/* Apply the address space setting */
	GB02FUNC1761(&as);
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
	u64 mmu_flags;

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

	if (flags & GB02MAC1256)
		/* inner and outer shareable */
		mmu_flags |= SHARE_BOTH_BITS;
	else if (flags & GB02MAC1254)
		/* inner shareable coherency */
		mmu_flags |= SHARE_INNER_BITS;

	return mmu_flags;
}

static void GB02FUNC1006(u64 *entry, phys_addr_t phy, unsigned long flags)
{
	GB02FUNC991(entry, (phy & ~0xFFF) | GB02FUNC1003(flags) |
	 GB02MAC1272 | GB02MAC1265);
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

static struct GB02STR229 hdmac_mmu64_mode = {
	.update = GB02FUNC1815,
	.get_as_setup = GB02FUNC1812,
	.disable_as = GB02FUNC992,
	.GB02FUNC995 = GB02FUNC995,
	.GB02FUNC998 = GB02FUNC998,
	.GB02FUNC1000 = GB02FUNC1000,
	.GB02FUNC1006 = GB02FUNC1006,
	.GB02FUNC1008 = GB02FUNC1008,
	.GB02FUNC1009 = GB02FUNC1009
};

struct GB02STR229 *GB02FUNC1824(void)
{
	return &hdmac_mmu64_mode;
}

void GB02FUNC1825(void)
{
#ifdef HDMAC_MMU_DUMP
	u64 *mmu_base = NULL;
	u64 mmu_phy;
	int i;

	mmu_base = GB02FUNC1769(GB02MAC2719);
	mmu_phy = GB02MAC2719;
	gb_printf(KERN_ERR, "=========mmu table dump start========\n");
	for (i = 0; i < (0x80000 / 8); i++) {
		if ((*mmu_base != 2) && (*mmu_base != 0))
			gb_printf(KERN_ERR, "0x%llx:0x%llx\n", mmu_phy, *mmu_base);
		mmu_base++;
		mmu_phy += 8;
	}
	gb_printf(KERN_ERR, "=========mmu table dump end========\n");
#endif
}

int GB02FUNC1828(u64 size,
	phys_addr_t *src_phys, phys_addr_t *dst_phys,
	bool is_read, int channel_id, bool is_done, u64 abort_pg_num,
	u64 cpu_virt, u64 gpu_virt)
{
	int ret = 0, retry = 0;
	u64 pg_cnt = size >> GB02MAC313;
	u32 src_phy_l, src_phy_h, dst_phy_l, dst_phy_h, hdmac_int_status;
	u64 src_phy, dst_phy;

	/* config start hdmac */
	mutex_lock(&hdmac_start_lock);
	if (is_read) {
		GB02FUNC1751(GB02MAC2783,
		 (cpu_virt & 0xfffffff) | ((2 * channel_id + 1) << 28));
		GB02FUNC1751(GB02MAC2784,
		 (cpu_virt >> 28)  | ((2 * channel_id + 1) << 28));
		GB02FUNC1751(GB02MAC2785,
		 (gpu_virt & 0xfffffff)  | ((2 * channel_id + 1) << 28));
		GB02FUNC1751(GB02MAC2786,
		 (gpu_virt >> 28)  | ((2 * channel_id + 1) << 28));
		GB02FUNC1751(GB02MAC2787,
		 pg_cnt | ((2 * channel_id + 1) << 28));
		GB02FUNC1751(GB02MAC2788,
		 0 | ((2 * channel_id + 1) << 28));
		GB02FUNC1751(GB02MAC2789,
		 1 | ((2 * channel_id + 1) << 28));
	} else {
		GB02FUNC1751(GB02MAC2783,
		 (gpu_virt & 0xfffffff)  | ((2 * channel_id) << 28));
		GB02FUNC1751(GB02MAC2784,
		 (gpu_virt >> 28)  | ((2 * channel_id) << 28));
		GB02FUNC1751(GB02MAC2785,
		 (cpu_virt & 0xfffffff)  | ((2 * channel_id) << 28));
		GB02FUNC1751(GB02MAC2786,
		 (cpu_virt >> 28)  | ((2 * channel_id) << 28));
		GB02FUNC1751(GB02MAC2787,
		 pg_cnt | ((2 * channel_id) << 28));
		GB02FUNC1751(GB02MAC2788,
		 0 | ((2 * channel_id) << 28));
		GB02FUNC1751(GB02MAC2789,
		 1 | ((2 * channel_id) << 28));
	}
	mutex_unlock(&hdmac_start_lock);
	/* config end */

	mutex_lock(&hdmac_int_status_lock);
retry_get_status:
	if (is_done)
		hdmac_int_status = GB02FUNC1752(GB02MAC2790);
	else
		hdmac_int_status = GB02FUNC1752(GB02MAC2791);

	if (!is_read) {
		if (hdmac_int_status & (1 << (2 * channel_id))) {
			if (is_done) {
				GB02FUNC1751(GB02MAC2792,
				 (1 << (2 * channel_id)));
				GB02FUNC1751(GB02MAC2792, 0);
				GB02FUNC1751(GB02MAC2807, 1);
				ret = 0;
				goto test_done;
			} else {
				src_phy_l =
				 GB02FUNC1752(GB02MAC2797(2
				  * channel_id));
				src_phy_h =
				 GB02FUNC1752(GB02MAC2798(2
				  * channel_id));
				dst_phy_l =
				 GB02FUNC1752(GB02MAC2801(2
				  * channel_id));
				dst_phy_h =
				 GB02FUNC1752(GB02MAC2802(2
				  * channel_id));
				src_phy = ((u64)src_phy_h << 32) | src_phy_l;
				dst_phy = ((u64)dst_phy_h << 32) | dst_phy_l;
				if ((src_phy == src_phys[abort_pg_num])
				 && (dst_phy == dst_phys[abort_pg_num]))
					ret = 0;
				else {
					gb_printf(KERN_ERR, "HDMAC ABORT ERROR!hdmac abort src_phy:0x%llx, user abort src_phy:0x%llx, dst:0x%llx, user dst:0x%llx\n",
					 src_phy, src_phys[abort_pg_num],
					  dst_phy, dst_phys[abort_pg_num]);
					ret = -1;
				}
				GB02FUNC1751(GB02MAC2793,
				 (1 << (2 * channel_id)));
				GB02FUNC1751(GB02MAC2793, 0);
				GB02FUNC1751(GB02MAC2807, 1);
				goto test_done;
			}
		} else {
			if (retry < 10) {
				retry++;
				mdelay(1);
				goto retry_get_status;
			}
			gb_printf(KERN_ERR, "hdmac write test ERR!!!is_read:%d, is_done:%d, channel_id:%d, size:0x%llx, status:0x%x, abort_status:0x%x\n",
			 is_read, is_done, channel_id, size, hdmac_int_status,
			  GB02FUNC1752(GB02MAC2791));
			ret = -1;
			goto test_done;
		}
	} else {
		if (hdmac_int_status & (1 << (2 * channel_id + 1))) {
			if (is_done) {
				GB02FUNC1751(GB02MAC2792,
				 (1 << (2 * channel_id + 1)));
				GB02FUNC1751(GB02MAC2792, 0);
				GB02FUNC1751(GB02MAC2807, 1);
				ret = 0;
				goto test_done;
			} else {
				src_phy_l =
				 GB02FUNC1752(GB02MAC2797(2 *
				  channel_id + 1));
				src_phy_h =
				 GB02FUNC1752(GB02MAC2798(2 *
				  channel_id + 1));
				dst_phy_l =
				 GB02FUNC1752(GB02MAC2801(2 *
				  channel_id + 1));
				dst_phy_h =
				 GB02FUNC1752(GB02MAC2802(2 *
				  channel_id + 1));
				src_phy =
				 ((u64)src_phy_h << 32) | src_phy_l;
				dst_phy =
				 ((u64)dst_phy_h << 32) | dst_phy_l;
				if ((src_phy == src_phys[abort_pg_num])
				 && (dst_phy == dst_phys[abort_pg_num]))
					ret = 0;
				else {
					gb_printf(KERN_ERR, "HDMAC ABORT ERROR!hdmac abort src_phy:0x%llx, user abort src_phy:0x%llx, dst:0x%llx, user dst:0x%llx\n",
					 src_phy, src_phys[abort_pg_num],
					  dst_phy, dst_phys[abort_pg_num]);
					ret = -1;
				}
				GB02FUNC1751(GB02MAC2793,
				 (1 << (2 * channel_id + 1)));
				GB02FUNC1751(GB02MAC2793, 0);
				GB02FUNC1751(GB02MAC2807, 1);
				goto test_done;
			}
		} else {
			if (retry < 10) {
				retry++;
				mdelay(1);
				goto retry_get_status;
			}
			gb_printf(KERN_ERR, "hdmac read test ERR, is_read: %d, is_done: %d, channel_id: %d, size: 0x%llx, status: 0x%x, abort_status: 0x%x\n",
			 is_read, is_done, channel_id, size,
			  hdmac_int_status,
			   GB02FUNC1752(GB02MAC2791));
			ret = -1;
			goto test_done;
		}
	}
test_done:
	mutex_unlock(&hdmac_int_status_lock);
	return ret;
}

void GB02FUNC1836(void)
{
	GB02FUNC1751(GB02MAC1542, 0xffffffff);
	GB02FUNC1751(GB02MAC1544, 0xffffffff);
}

u32 GB02FUNC1838(void)
{
	return GB02FUNC1752(GB02MAC1546);
}
