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
#include "common/gb_common.h"
#include "common/xt.h"
#include "gb_hdmac.h"
#include "gb_hdmac_regmap.h"
#include "common/gb_pcie_info.h"

#if 0
#define GB02MAC2557 'M'
#define HDMAC_TEST _IOW(GB02MAC2557, 1u, unsigned long long)
#define HDMAC_TEST_MMU_SOFT_RESET\
	_IOW(GB02MAC2557, 2u, unsigned long long)
#define HDMAC_TEST_HDMAC_SOFT_RESET\
	_IOW(GB02MAC2557, 3u, unsigned long long)
#endif

static DEFINE_MUTEX(hdmac_channel_id_lock);
struct GB02STR202 *dma_infos;
int hdmac_channel_id;

static struct task_struct *gb_hdmac_thread_0;
static struct task_struct *gb_hdmac_thread_1;
static struct task_struct *gb_hdmac_thread_2;
static struct task_struct *gb_hdmac_thread_3;
static int GB02FUNC1715(struct GB02STR228 *gbctx,
	int channel_id, phys_addr_t *cpu_phys, phys_addr_t *gpu_phys,
	struct page **cpu_pages, u64 *cpu_virt, u64 *gpu_virt, u64 pg_cnt);

static int GB02FUNC1716(void *arg)
{
	int ret = 0;
	u64 size = 0x2000000;
	int channel_id = 0;
	u64 pg_cnt = size >> GB02MAC313;
	phys_addr_t *cpu_phys = NULL;
	phys_addr_t *gpu_phys = NULL;
	struct page **cpu_pages = NULL;
	u64 cpu_virt, gpu_virt;
	struct GB02STR228 gbctx;

	gb_printf(KERN_ERR, "=========Enter %s, channel_id:%d, size:0x%llx==========\n",
		__func__, channel_id, size);

	cpu_phys =
	 (phys_addr_t *)kmalloc(sizeof(phys_addr_t *) * pg_cnt, GFP_KERNEL);
	gpu_phys =
	 (phys_addr_t *)kmalloc(sizeof(phys_addr_t *) * pg_cnt, GFP_KERNEL);
	cpu_pages =
	 (struct page **)kmalloc(sizeof(struct page **) * pg_cnt, GFP_KERNEL);

	if (GB02FUNC1715(&gbctx, channel_id, cpu_phys, gpu_phys,
			cpu_pages, &cpu_virt, &gpu_virt, pg_cnt)) {
		gb_printf(KERN_ERR, "init gbctx failed!!channel_id:%d, size:0x%llx\n",
		 channel_id, size);
		ret = -1;
		goto free;
	}

	GB02FUNC1825();
	while (!kthread_should_stop()) {
		mdelay(1);
		/* DMA READ TEST */
		ret = GB02FUNC1828(size, cpu_phys, gpu_phys, true,
		 channel_id, true, pg_cnt - 1, cpu_virt, gpu_virt);
		if (ret)
			gb_printf(KERN_ERR, "hdmac read test ERR chan_id:%d size:0x%llx\n",
			 channel_id, size);
		mdelay(1);
		/* DMA WRITE TEST */
		ret = GB02FUNC1828(size, gpu_phys, cpu_phys, false,
		 channel_id, true, pg_cnt - 1, cpu_virt, gpu_virt);
		if (ret)
			gb_printf(KERN_ERR, "hdmac write test ERR chan_id:%d size:0x%llx\n",
			 channel_id, size);
	}
free:
	kfree(cpu_phys);
	kfree(gpu_phys);
	kfree(cpu_pages);

	return 0;
}

static int GB02FUNC1720(void *arg)
{
	int ret = 0;
	u64 size = 0x2000000;
	int channel_id = 1;
	u64 pg_cnt = size >> GB02MAC313;
	phys_addr_t *cpu_phys = NULL;
	phys_addr_t *gpu_phys = NULL;
	struct page **cpu_pages = NULL;
	u64 cpu_virt, gpu_virt;
	struct GB02STR228 gbctx;

	gb_printf(KERN_ERR, "=========Enter %s, channel_id:%d, size:0x%llx==========\n",
		__func__, channel_id, size);

	cpu_phys =
	 (phys_addr_t *)kmalloc(sizeof(phys_addr_t *) * pg_cnt, GFP_KERNEL);
	gpu_phys =
	 (phys_addr_t *)kmalloc(sizeof(phys_addr_t *) * pg_cnt, GFP_KERNEL);
	cpu_pages =
	 (struct page **)kmalloc(sizeof(struct page **) * pg_cnt, GFP_KERNEL);

	if (GB02FUNC1715(&gbctx, channel_id, cpu_phys, gpu_phys,
			cpu_pages, &cpu_virt, &gpu_virt, pg_cnt)) {
		gb_printf(KERN_ERR, "init gbctx failed!!channel_id:%d, size:0x%llx\n",
		 channel_id, size);
		ret = -1;
		goto free;
	}

	GB02FUNC1825();
	while (!kthread_should_stop()) {
		mdelay(1);
		/* DMA READ TEST */
		ret = GB02FUNC1828(size, cpu_phys, gpu_phys, true,
		 channel_id, true, pg_cnt - 1, cpu_virt, gpu_virt);
		if (ret)
			gb_printf(KERN_ERR, "hdmac read test ERR!!!chan_id:%d size:0x%llx\n",
			 channel_id, size);
		mdelay(1);
		/* DMA WRITE TEST */
		ret = GB02FUNC1828(size, gpu_phys, cpu_phys, false, channel_id,
			true, pg_cnt - 1, cpu_virt, gpu_virt);
		if (ret)
			gb_printf(KERN_ERR, "hdmac write test ERR chan_id:%d size:0x%llx\n",
			 channel_id, size);
	}
free:
	kfree(cpu_phys);
	kfree(gpu_phys);
	kfree(cpu_pages);

	return 0;
}

static int GB02FUNC1724(void *arg)
{
	int ret = 0;
	u64 size = 0x2000000;
	int channel_id = 2;
	u64 pg_cnt = size >> GB02MAC313;
	phys_addr_t *cpu_phys = NULL;
	phys_addr_t *gpu_phys = NULL;
	struct page **cpu_pages = NULL;
	u64 cpu_virt, gpu_virt;
	struct GB02STR228 gbctx;

	gb_printf(KERN_ERR, "=========Enter %s, channel_id:%d, size:0x%llx==========\n",
		__func__, channel_id, size);

	cpu_phys =
	 (phys_addr_t *)kmalloc(sizeof(phys_addr_t *) * pg_cnt, GFP_KERNEL);
	gpu_phys =
	 (phys_addr_t *)kmalloc(sizeof(phys_addr_t *) * pg_cnt, GFP_KERNEL);
	cpu_pages =
	 (struct page **)kmalloc(sizeof(struct page **) * pg_cnt, GFP_KERNEL);

	if (GB02FUNC1715(&gbctx, channel_id, cpu_phys, gpu_phys,
			cpu_pages, &cpu_virt, &gpu_virt, pg_cnt)) {
		gb_printf(KERN_ERR, "init gbctx failed!!channel_id:%d, size:0x%llx\n",
		 channel_id, size);
		ret = -1;
		goto free;
	}

	GB02FUNC1825();

	while (!kthread_should_stop()) {
		mdelay(1);
		/* DMA READ TEST */
		ret = GB02FUNC1828(size, cpu_phys, gpu_phys, true,
		 channel_id, true, pg_cnt - 1, cpu_virt, gpu_virt);
		if (ret)
			gb_printf(KERN_ERR, "hdmac read test ERR chan_id:%d size:0x%llx\n",
			 channel_id, size);
		mdelay(1);
		/* DMA WRITE TEST */
		ret = GB02FUNC1828(size, gpu_phys, cpu_phys, false,
		 channel_id, true, pg_cnt - 1, cpu_virt, gpu_virt);
		if (ret)
			gb_printf(KERN_ERR, "hdmac write test ERR chan_id:%d size:0x%llx\n",
			 channel_id, size);
	}
free:
	kfree(cpu_phys);
	kfree(gpu_phys);
	kfree(cpu_pages);

	return 0;
}

static int GB02FUNC1727(void *arg)
{
	int ret = 0;
	u64 size = 0x2000000;
	int channel_id = 3;
	u64 pg_cnt = size >> GB02MAC313;
	phys_addr_t *cpu_phys = NULL;
	phys_addr_t *gpu_phys = NULL;
	struct page **cpu_pages = NULL;
	u64 cpu_virt, gpu_virt;
	struct GB02STR228 gbctx;

	gb_printf(KERN_ERR, "=========Enter %s, channel_id:%d, size:0x%llx========\n",
		__func__, channel_id, size);

	cpu_phys =
	 (phys_addr_t *)kmalloc(sizeof(phys_addr_t *) * pg_cnt, GFP_KERNEL);
	gpu_phys =
	 (phys_addr_t *)kmalloc(sizeof(phys_addr_t *) * pg_cnt, GFP_KERNEL);
	cpu_pages =
	 (struct page **)kmalloc(sizeof(struct page **) * pg_cnt, GFP_KERNEL);

	if (GB02FUNC1715(&gbctx, channel_id, cpu_phys, gpu_phys,
			cpu_pages, &cpu_virt, &gpu_virt, pg_cnt)) {
		gb_printf(KERN_ERR, "init gbctx failed!!channel_id:%d, size:0x%llx\n",
		 channel_id, size);
		ret = -1;
		goto free;
	}

	GB02FUNC1825();

	while (!kthread_should_stop()) {
		mdelay(1);
		/* DMA READ TEST */
		ret = GB02FUNC1828(size, cpu_phys, gpu_phys, true,
		 channel_id, true, pg_cnt - 1, cpu_virt, gpu_virt);
		if (ret)
			gb_printf(KERN_ERR, "hdmac read test ERR chan_id:%d size:0x%llx\n",
				channel_id, size);
		mdelay(1);
		/* DMA WRITE TEST */
		ret = GB02FUNC1828(size, gpu_phys, cpu_phys, false,
		 channel_id, true, pg_cnt - 1, cpu_virt, gpu_virt);
		if (ret)
			gb_printf(KERN_ERR, "hdmac write test ERR chan_id:%d size:0x%llx\n",
				channel_id, size);
	}
free:
	kfree(cpu_phys);
	kfree(gpu_phys);
	kfree(cpu_pages);

	return 0;
}

static int GB02FUNC1730(void)
{
	gb_hdmac_thread_0 =
	 kthread_run(GB02FUNC1716, NULL, "gb_hdmac_thread_0");
	gb_hdmac_thread_1 =
	 kthread_run(GB02FUNC1720, NULL, "gb_hdmac_thread_1");
	gb_hdmac_thread_2 =
	 kthread_run(GB02FUNC1724, NULL, "gb_hdmac_thread_2");
	gb_hdmac_thread_3 =
	 kthread_run(GB02FUNC1727, NULL, "gb_hdmac_thread_3");
	if (IS_ERR(gb_hdmac_thread_0) | IS_ERR(gb_hdmac_thread_1) |
		IS_ERR(gb_hdmac_thread_2) | IS_ERR(gb_hdmac_thread_3)) {
		gb_printf(KERN_ERR, "%s-%d create thread fail 0x%p, 0x%p, 0x%p, 0x%p\n",
		 __func__, __LINE__, gb_hdmac_thread_0,
		  gb_hdmac_thread_1, gb_hdmac_thread_2,
		   gb_hdmac_thread_3);
		return -EFAULT;
	}
	return 0;
}

static void GB02FUNC1732(void)
{
	if (gb_hdmac_thread_0)
		kthread_stop(gb_hdmac_thread_0);
	if (gb_hdmac_thread_1)
		kthread_stop(gb_hdmac_thread_1);
	if (gb_hdmac_thread_2)
		kthread_stop(gb_hdmac_thread_2);
	if (gb_hdmac_thread_3)
		kthread_stop(gb_hdmac_thread_3);
}

static void GB02FUNC1734(struct GB02STR228 *gbctx, int as_nr)
{
	/* init gbctx */
	memset(gbctx, 0, sizeof(struct GB02STR228));
	gbctx->mmu_mode = GB02FUNC1824();
	gbctx->as_nr = as_nr;
	gbctx->pgd = GB02FUNC1778(gbctx);

	gb_printf(KERN_ERR, "%s as_nr:%d, pgd:0x%llx\n",
		__func__, gbctx->as_nr, gbctx->pgd);
	/* config mmu hw */
	gbctx->mmu_mode->update(gbctx);
}

static int GB02FUNC1715(struct GB02STR228 *gbctx,
	int channel_id, phys_addr_t *cpu_phys, phys_addr_t *gpu_phys,
	struct page **cpu_pages, u64 *cpu_virt, u64 *gpu_virt, u64 pg_cnt)
{
	int i;

	/*
	 * we have 8 AddrSpaces and 8 channels, one as for one channel,
	 * so treat channel_id as as_id
	 */
	GB02FUNC1734(gbctx, channel_id);

	for (i = 0; i < pg_cnt; i++) {
		cpu_pages[i] = alloc_page(GFP_KERNEL);
		cpu_phys[i] = page_to_phys(cpu_pages[i]);
		hdmac_debug("cpu_phy[%d]:0x%llx\n", i, cpu_phys[i]);
	}

	if (GB02FUNC1805(gbctx, pg_cnt, false, cpu_phys, cpu_virt)) {
		gb_printf(KERN_ERR, "map cpu failed!pg_cnt:%llu\n", pg_cnt);
		return -1;
	}

	for (i = 0; i < pg_cnt; i++)
		gpu_phys[i] = GB02FUNC1772();

	if (GB02FUNC1805(gbctx, pg_cnt, true, gpu_phys, gpu_virt)) {
		gb_printf(KERN_ERR, "map gpu failed!pg_cnt:%llu\n", pg_cnt);
		return -1;
	}

	return 0;
}

irqreturn_t GB02FUNC1739(int irq, void *arg)
{
	u32 hdmac_int_status;
	u32 hdmac_abort_status;
	u32 hdmac_mmu_status;
	u64 fault_addr_lo, fault_addr_hi;
	u64 fault_addr;

	hdmac_int_status = GB02FUNC1752(GB02MAC2790);
	hdmac_abort_status = GB02FUNC1752(GB02MAC2791);
	hdmac_mmu_status = GB02FUNC1752(MMU_AS_REG(0, GB02MAC1582));

	if (hdmac_mmu_status) {
		fault_addr_lo = GB02FUNC1752(MMU_AS_REG(0,
		 GB02MAC2815));
		fault_addr_hi = GB02FUNC1752(MMU_AS_REG(0,
		 GB02MAC2816));
		fault_addr = fault_addr_lo | (fault_addr_hi << 32);
		hdmac_debug("mmu interrupt!status:0x%x, fault_addr:0x%llx\n",
			hdmac_mmu_status, fault_addr);
	}

	if (hdmac_abort_status)
		hdmac_debug("abort interrupt!status:0x%x\n",
		 hdmac_abort_status);

	if (hdmac_int_status)
		hdmac_debug("done interrupt!status:0x%x\n", hdmac_int_status);

	return IRQ_HANDLED;
}

int GB02FUNC1742(struct GB02STR70 *pcie_info)
{
	int result = 0;
	struct GB02STR202 *dma_ctrl;
	int ddr_bar_id = GB02FUNC468(pcie_info->GB02STR153);
	int reg_bar_id = GB02FUNC465(pcie_info->GB02STR153);

	dma_ctrl =
	 (struct GB02STR202 *)kzalloc(sizeof(struct GB02STR202),
	  GFP_KERNEL);
	if (dma_ctrl == NULL) {
		gb_printf(KERN_ERR, "Error: %s, kmalloc failed\n", __func__);
		return -ENOMEM;
	}

	dma_ctrl->vram_addr = pcie_info->pci_bars[ddr_bar_id].mmio;
	dma_ctrl->regs = pcie_info->pci_bars[reg_bar_id].mmio
	 + GB02MAC1048;
	memset_io(dma_ctrl->vram_addr + GB02MAC2719, 0, GB02MAC2718);

	dma_infos = dma_ctrl;

	GB02FUNC1836();
	GB02FUNC1751(GB02MAC2805, 0x40000000);
	GB02FUNC1751(GB02MAC2806, 0);
	GB02FUNC1751(GB02MAC2808, (1 << 0) | (1 << 31));

	result = GB02FUNC1730();
	if (result)
		gb_printf(KERN_ERR, "%s %d fail!\n", __func__, __LINE__);

	return result;
}

void GB02FUNC1745(void)
{
	GB02FUNC1732();
	kfree(dma_infos);
	dma_infos = NULL;
}
