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
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0))
#include <linux/timekeeping32.h>
#else
#include <linux/timekeeping.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#include <linux/vmalloc.h>
#endif
#include "gb_mmu.h"
#include "gb_gem.h"
#include "gb_device.h"
#include "gb_pages_alloc.h"
#include "common/gb_common.h"
#ifdef GB_DMA_ON
#include "ip/gb_edma.h"
#endif

#define GB02MAC1303 false
#define GB02MAC1305 false

static size_t GB02FUNC1092(u64 word,
	size_t num_remain, u32 *bit_record, u64 total, size_t nr_pages)
{
	int i;

	if (word == ~0UL)
		return num_remain;

	for (i = 0; i < BITS_PER_LONG; i++) {
		if (!word & 1ULL) {
			bit_record[nr_pages - num_remain] = total + i;
			num_remain--;
			if (!num_remain)
				return 0;
		}
		word >>= 1UL;
	}

	return num_remain;
}

/*
 * find size of size_bit_record zero bits from 0~max_cnt
 * bits in usage and record bits nums in bit_record
 */
static int GB02FUNC1094(volatile u64 *usage,
	u32 max_cnt, size_t size_bit_record, u32 *bit_record)
{
	u64 total;
	size_t num_remain = size_bit_record;
	size_t num_tmp;
	u64 usage_tmp;

	for (total = 0; total < max_cnt; total += 64, ++usage) {
		num_tmp = num_remain;
		usage_tmp = *usage;
		num_remain = GB02FUNC1092(usage_tmp, num_tmp, bit_record, total, size_bit_record);
		if (!num_remain)
			return 0;
	}

	return -ENOMEM;
}

static u32 GB02FUNC1096(volatile u64 *usage, u32 max_cnt)
{
	int i;
	for (i = (max_cnt + 63) / 64 - 1; i >= 0; i--) {
		if (usage[i] != 0) {
			return (i * 64 + fls64(usage[i]));
		}
	}

	return 0;
}

static void GB02FUNC1097(u32 bits, volatile u64 *usage)
{
	u32 idx = bits / BITS_PER_LONG;
	u64 bit = bits % BITS_PER_LONG;
	usage[idx] |= (1UL << bit);
//	gb_verb_printf(KERN_INFO "idx: %d bit: %d usage[0]: 0x%lx, BITS_PER_LONG: %d\n", idx, bit, usage[0], BITS_PER_LONG);
}

static void GB02FUNC1099(int bits, volatile u64 *usage)
{
	int idx = bits / BITS_PER_LONG;
	u64 bit = bits % BITS_PER_LONG;
//	gb_verb_printf("bits: %d idx: %d bit: %d, usage[idx]: %lx\n", bits, idx, bit, usage[idx]);
	u64 tmp = usage[idx];
	usage[idx] &= (~(1UL << bit));
	if (tmp == usage[idx]) {
	//	gb_verb_printf(KERN_INFO "BBBTTTSSS invalid setting!\n");
	}
//	gb_verb_printf("idx: %d bit: %d, usage[idx]: %lx\n", idx, bit, usage[idx]);
}

// GPU device physical address
phys_addr_t GB02FUNC1102(struct GB02STR125 *p)
{
	if (p) {
		return p->gpu_phy;
	} else {
		gb_err(NULL, "Error: %s GB02STR125 is NULL\n", __func__);
	}

	return 0;
}

struct GB02STR125 *GB02FUNC1104(struct GB02STR39 *gbdev, phys_addr_t phys)
{
	int pfn;

	if ((phys < GB02MAC319(gbdev->gb_pcie->GB02STR153)) ||
		(phys >=
		 GB02MAC328(gbdev->gb_pcie->GB02STR153)) ||
		(phys < gbdev->GB02STR35.mmu_lowest_addr &&
		 phys >= gbdev->GB02STR35.page_highest_addr)) {
		gb_printf(KERN_INFO, "AAAAA+++++++ After GB02FUNC1104\n");
		gb_err(gbdev->dev, "Error: %s %d bad mmu phys: 0x%llx mmu_low:0x%llx pg_high:0x%llx\n, normal %llx MMU_TOP %llx ",
			   __func__, __LINE__, phys,
			   gbdev->GB02STR35.mmu_lowest_addr,
			   gbdev->GB02STR35.page_highest_addr,
			   GB02MAC319(gbdev->gb_pcie->GB02STR153),
			   GB02MAC328(gbdev->gb_pcie->GB02STR153));
		return NULL;
	}

	/* for mmu pages */
	if (phys >= gbdev->GB02STR35.mmu_lowest_addr) {
		pfn =
		 (GB02MAC328(gbdev->gb_pcie->GB02STR153)
		  - 1 - phys) >> GB02MAC313;
		return gbdev->mmu_top_page + pfn;
	}

	/* for normal pages */
	pfn =
	 (phys - GB02MAC319(gbdev->gb_pcie->GB02STR153)) >>
	  PAGE_SHIFT;
	return gbdev->global_gb_page + pfn;
}

u64 *GB02FUNC1107(struct GB02STR125 *p)
{
	struct GB02STR67 *GB02STR153 = p->gbdev->gb_pcie->GB02STR153;
	int ddr_bar_id = GB02FUNC468(GB02STR153);
	u64 *ret_p = NULL;

	if (p == NULL) {
		gb_err(NULL, "ERROR: %s p is NULL\n", __func__);
		return NULL;
	}
	if (p->pfn == -1 || p->gpu_phy == -1) {
		gb_err(NULL, "Error: %s invalid pfn.", __func__);
		return NULL;
	}

	if (p->gpu_phy >= p->gbdev->GB02STR35.mmu_lowest_addr) {
		// stand for mmu page
		ret_p =
		 (u64 *)((void *)p->gbdev->gb_pcie->pci_bars[ddr_bar_id].mmio +
		  GB02MAC328(GB02STR153) -
		   (((p->pfn)+1)<<GB02MAC313));

		return ret_p;
	} else {
		ret_p =
		 (u64 *)((void *)p->gbdev->gb_pcie->pci_bars[ddr_bar_id].mmio +
		  GB02MAC319(GB02STR153) +
		   ((p->pfn)<<PAGE_SHIFT));
		return ret_p;
	}
}

/*
 * Set info for struct GB02STR125
 * nr_pages: number of pages in gpu pg size
 * bit_record: pfn bit record in gb_find_first_zero_bit
 */
static void GB02FUNC1108(struct GB02STR39 *gbdev,
	size_t nr_pages, phys_addr_t *pages, u32 *bit_record)
{
	struct GB02STR125 *GB02STR125 = NULL;
	u32 gpu_pfn;
	size_t i;
	int k;

	spin_lock(&gbdev->GB02STR35.ddr_lock);

	/*
	 * nr_pages is number of page in gpu pg size, but GB02STR125
	 * is for page in cpu page size, so use GB02MAC317 to
	 * control loop
	 */
	for (i = 0; i < nr_pages; i += GB02MAC317) {
		gpu_pfn = bit_record[i/GB02MAC317];

		GB02FUNC1097(gpu_pfn, gbdev->GB02STR35.usage);
		atomic_add_return(1, &(gbdev->GB02STR35.used_pages));

		smp_mb();

		GB02STR125 = gbdev->global_gb_page + gpu_pfn;
		GB02STR125->pfn = gpu_pfn;
		GB02STR125->gbdev = gbdev;
		/* one page 16K on MIPS */
		GB02STR125->gpu_phy =
		 GB02MAC319(gbdev->gb_pcie->GB02STR153) +
		  ((GB02STR125->pfn) << PAGE_SHIFT);
		for (k = 0; k < GB02MAC317; k++){
			pages[i+k] = GB02STR125->gpu_phy + k * GB02MAC311;
		}
	}

	/*
	 * refresh page_highest_addr, the last pfn in bit_record
	 * must be the highest address in pfns of bit_record, so
	 * use it directly to compare with old page_highest_addr
	 */
	if (GB02STR125->gpu_phy + PAGE_SIZE > gbdev->GB02STR35.page_highest_addr)
		gbdev->GB02STR35.page_highest_addr = GB02STR125->gpu_phy + PAGE_SIZE;

	spin_unlock(&gbdev->GB02STR35.ddr_lock);
}

int GB02FUNC1111(struct GB02STR39 *gbdev, size_t nr_pages, phys_addr_t *pages)
{
	u64 search_max_pfn;
	u64 vram_end_pfn;
	u64 vram_end_addr;
	u32 *bit_record = NULL;
	int vmalloc_flag = false;
	int ret = 0;
	size_t nr_pg_in_cpu_pgsz = (nr_pages + GB02MAC317 - 1) / GB02MAC317;

	if (nr_pages == 0) {
		gb_printf(KERN_DEBUG, "%s alloc 0 page, return", __func__);
		return 0;
	}

	/* record allocated pfns in gb_find_first_zero_bit */
	bit_record = (u32 *)kmalloc(nr_pg_in_cpu_pgsz * sizeof(u32), GFP_KERNEL);
	if (unlikely(!bit_record)) {
		bit_record = (u32 *)vmalloc(nr_pg_in_cpu_pgsz * sizeof(u32));
		if (!bit_record) {
			gb_err(gbdev->dev, "ERROR: %s %d no enough cpu ddr space:alloc size: 0x%zx\n",
				__func__, __LINE__, nr_pg_in_cpu_pgsz * sizeof(u32));
			return -ENOMEM;
		}

		vmalloc_flag = true;
	}

	mutex_lock(&gbdev->vram_mutex);
	/*
	 * pfn for vram pg must be less than lowest pfn of mmu pg,
	 * they can't be overlaped
	 */
	search_max_pfn = (gbdev->GB02STR35.mmu_lowest_addr -
	 GB02MAC319(gbdev->gb_pcie->GB02STR153)) / PAGE_SIZE;
	ret = GB02FUNC1094(gbdev->GB02STR35.usage,
		search_max_pfn, nr_pg_in_cpu_pgsz, bit_record);
	if (ret) {
		gb_printf(KERN_ERR, "ERROR in %s mmu_lowest_addr:0x%llx\n",
		 __func__, gbdev->GB02STR35.mmu_lowest_addr);
		gb_printf(KERN_ERR, "ERROR pg_highest_addr:0x%llx, 0x%llx, 0x%llx\n",
		 gbdev->GB02STR35.page_highest_addr,
		  GB02MAC328(gbdev->gb_pcie->GB02STR153) -
		   ((GB02FUNC1096(gbdev->GB02STR35.mmu_usage,
		    GB02MAC330(gbdev->gb_pcie->GB02STR153) * 64))
			 << GB02MAC313),
			GB02MAC319(gbdev->gb_pcie->GB02STR153) +
			 ((GB02FUNC1096(gbdev->GB02STR35.usage,
			  GB02MAC331(gbdev->gb_pcie->GB02STR153)
			   * 64)) << PAGE_SHIFT));
		goto out;
	}

	vram_end_pfn = bit_record[nr_pages - 1];
	vram_end_addr = GB02MAC319(gbdev->gb_pcie->GB02STR153) +
	 (vram_end_pfn << PAGE_SHIFT);
	if (vram_end_addr >= gbdev->GB02STR35.mmu_lowest_addr) {
		gb_printf(KERN_ERR, "%s mmu_lowest_addr:0x%llx vram_end_addr:0x%llx \n", __func__,
			gbdev->GB02STR35.mmu_lowest_addr, vram_end_addr);
		ret = -ENOMEM;
		goto out;
	}

	GB02FUNC1108(gbdev, nr_pages, pages, bit_record);

out:
	mutex_unlock(&gbdev->vram_mutex);

	if (vmalloc_flag)
		vfree(bit_record);
	else
		kfree(bit_record);

	return ret;
}

void GB02FUNC1119(struct GB02STR125 *GB02STR125, struct GB02STR39 *gbdev)
{
	/*
	 * this pfn is the highest pfn for vram page plus 1
	 * for example if vram page only used pfn 0,
	 * then this pfn is 1
	 */
	u32 pfn;
	void *ptr;

	ptr = GB02FUNC1107(GB02STR125);
	if (ptr) {
		// gb_printf(KERN_INFO, "%s page free GB02STR125->pfn: 0x%x\n",__func__, GB02STR125->pfn);
	} else {
		gb_err(gbdev->dev, "Error:%s GB02STR125 is invalid!\n", __func__);

		return;
	}
	memset(ptr, 0, GB02MAC311);

	spin_lock(&gbdev->GB02STR35.ddr_lock);
	GB02FUNC1099(GB02STR125->pfn, gbdev->GB02STR35.usage);
	atomic_sub_return(1, &(gbdev->GB02STR35.used_pages));

	/* if highest addr is freed, then refresh it */
	if (GB02STR125->gpu_phy + PAGE_SIZE == gbdev->GB02STR35.page_highest_addr) {
		pfn = GB02FUNC1096(gbdev->GB02STR35.usage, GB02STR125->pfn);

		gbdev->GB02STR35.page_highest_addr =
			GB02MAC319(gbdev->gb_pcie->GB02STR153) +
			 (pfn << PAGE_SHIFT);
	}
	spin_unlock(&gbdev->GB02STR35.ddr_lock);

	memset(GB02STR125, 0, sizeof(struct GB02STR125));
	GB02STR125->pfn = -1;
	GB02STR125->gpu_phy = -1;
}

struct GB02STR125 *GB02FUNC1123(struct GB02STR39 *gbdev)
{
	struct GB02STR125 *GB02STR125;
	u32 gpu_pfn;
	u64 search_max_pfn;
	u64 mmu_end_addr;
	int ret;

	mutex_lock(&gbdev->vram_mutex);

	search_max_pfn =
	 (GB02MAC328(gbdev->gb_pcie->GB02STR153) -
	  gbdev->GB02STR35.page_highest_addr) / GB02MAC311;
	ret = GB02FUNC1094(gbdev->GB02STR35.mmu_usage,
		search_max_pfn, 1, &gpu_pfn);
	if (ret) {
		gb_printf(KERN_ERR, "ERROR in %s!!! mmu_lowest_addr:0x%llx, pg_highest_addr:0x%llx, 0x%llx, 0x%llx\n",
		 __func__, gbdev->GB02STR35.mmu_lowest_addr,
		  gbdev->GB02STR35.page_highest_addr,
		   GB02MAC328(gbdev->gb_pcie->GB02STR153) -
			 ((GB02FUNC1096(gbdev->GB02STR35.mmu_usage,
			  GB02MAC330(gbdev->gb_pcie->GB02STR153) * 64))
			   << GB02MAC313),
			GB02MAC319(gbdev->gb_pcie->GB02STR153) +
			 ((GB02FUNC1096(gbdev->GB02STR35.usage,
			  GB02MAC331(gbdev->gb_pcie->GB02STR153)
			   * 64)) << PAGE_SHIFT));
		mutex_unlock(&gbdev->vram_mutex);
		return NULL;
	}

	mmu_end_addr =
	 GB02MAC328(gbdev->gb_pcie->GB02STR153) -
	  ((gpu_pfn + 1) << GB02MAC313);
	if (mmu_end_addr < gbdev->GB02STR35.page_highest_addr) {
		gb_printf(KERN_ERR, "%s mmu alloc err, page_highest_addr:0x%llx mmu_end_addr:0x%llx\n", __func__,
			gbdev->GB02STR35.page_highest_addr, mmu_end_addr);
		mutex_unlock(&gbdev->vram_mutex);
		return NULL;
	}

	spin_lock(&gbdev->GB02STR35.ddr_lock);

	GB02FUNC1097(gpu_pfn, gbdev->GB02STR35.mmu_usage);
	GB02STR125 = gbdev->mmu_top_page + gpu_pfn;
	memset(GB02STR125, 0, sizeof(struct GB02STR125));
	atomic_add_return(1, &(gbdev->GB02STR35.used_mmu_pages));

	smp_mb();

	GB02STR125->gpu_phy = mmu_end_addr;
	GB02STR125->pfn = gpu_pfn;
	GB02STR125->gbdev = gbdev;
	//pr_err("[%s] phy %llx low %llx\n", __func__, GB02STR125->gpu_phy, gbdev->GB02STR35.mmu_lowest_addr);
	if (GB02STR125->gpu_phy < gbdev->GB02STR35.mmu_lowest_addr)
		gbdev->GB02STR35.mmu_lowest_addr = GB02STR125->gpu_phy;

	spin_unlock(&gbdev->GB02STR35.ddr_lock);

	// gb_verb_printf(KERN_INFO "%s gpu_phy: 0x%llx pfn: %u\n", __func__, GB02STR125->gpu_phy, GB02STR125->pfn);
	mutex_unlock(&gbdev->vram_mutex);

	return GB02STR125;
}

void GB02FUNC1130(struct GB02STR125 *GB02STR125, struct GB02STR39 *gbdev)
{
	void *ptr;
	u32 pfn;

	ptr = GB02FUNC1107(GB02STR125);
	if (!ptr) {
		gb_err(gbdev->dev, "Error:%s GB02STR125 is invalid!\n", __func__);

		return;
	}
	memset_io(ptr, 0, GB02MAC311);
	//dump_stack();
	spin_lock(&gbdev->GB02STR35.ddr_lock);
	GB02FUNC1099(GB02STR125->pfn, gbdev->GB02STR35.mmu_usage);
	atomic_sub_return(1, &(gbdev->GB02STR35.used_mmu_pages));

	/* if lowest addr is freed, then refresh it */
	if (GB02STR125->gpu_phy == gbdev->GB02STR35.mmu_lowest_addr) {
		pfn = GB02FUNC1096(gbdev->GB02STR35.mmu_usage, GB02STR125->pfn);
		//pr_err("[%s] low %llx phy %llx\n", __func__, gbdev->GB02STR35.mmu_lowest_addr, GB02STR125->gpu_phy);
		gbdev->GB02STR35.mmu_lowest_addr =
		 GB02MAC328(gbdev->gb_pcie->GB02STR153) -
			 (pfn << GB02MAC313);
	}
	spin_unlock(&gbdev->GB02STR35.ddr_lock);
	memset(GB02STR125, 0, sizeof(struct GB02STR125));
	GB02STR125->pfn = -1;
	GB02STR125->gpu_phy = -1;
}

int GB02FUNC1132(struct GB02STR39 *gbdev, size_t nr_pages, phys_addr_t *pages)
{
	return GB02FUNC1111(gbdev, nr_pages, pages);
}

void GB02FUNC1133(struct GB02STR39 *gbdev, size_t nr_pages, phys_addr_t *pages)
{
	struct GB02STR125 *p;
	size_t i = 0;
	size_t k;

	/* Free any remaining pages to kernel */
	for (; i < nr_pages; i+=GB02MAC317) {
		if (unlikely(!pages[i])) {
			gb_err(gbdev->dev, "BUG.... in %s\n", __func__);

			continue;
		}

		p = GB02FUNC1104(gbdev, pages[i]);
		if (!p) {
			gb_err(gbdev->dev, "Error: GB02STR125 is NULL %s\n", __func__);
			return;
		}
		GB02FUNC1119(p, gbdev);

		pages[i] = 0;
		for (k = 0; k < GB02MAC317; k++) {
			pages[i+k] = 0;
		}
	}
}

#ifdef GB_DMA_ON
int GB02FUNC1135(struct gb_context *gbctx, struct gb_uk_get_mem_status *mem)
{
	struct GB02STR35 *page_ctrl;

	page_ctrl = &gbctx->gbdev->GB02STR35;

	mem->used_pages = atomic_read(&page_ctrl->used_pages);
	mem->used_mmu_pages = atomic_read(&page_ctrl->used_mmu_pages);
	gb_verb_printf(KERN_INFO "used_pages: %d used_mmu_pages: 0x%x pg_high: 0x%llx mmu_low: 0x%llx\n",
		mem->used_pages, mem->used_mmu_pages,
		page_ctrl->page_highest_addr, page_ctrl->mmu_lowest_addr);

	return 0;
}
GB_EXPORT_TEST_API(GB02FUNC1135);
#endif
