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
#include <linux/shrinker.h>
#include <linux/atomic.h>
#include <linux/version.h>
#include <linux/irqreturn.h>
#include <linux/pci.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0))
#include <linux/timekeeping32.h>
#else
#include <linux/timekeeping.h>
#endif
#include <linux/vmalloc.h>
#include "gb_edma.h"
#include "edma_reg.h"
#include "gpu/gb_device.h"
#include "common/xt.h"
#include "common/gb_common.h"

static void GB02FUNC1660(struct GB02STR6 *chan, u32 status)
{
    GB02FUNC15(chan, GB02MAC110, status);
}

static void GB02FUNC1661(struct GB02STR6 *chan)
{
    struct GB02STR8 *chip = chan->chip;

    GB02FUNC21(GB02MAC21, chip->hdmac_regs, GB02MAC18);;
}

irqreturn_t GB02FUNC1662(int irq, void *arg)
{
    struct GB02STR8 *dma_ctrl = (struct GB02STR8 *)arg;
    u32 status = 0;
    u32 i = 0;
    struct GB02STR6 *chan;

    gb_printf(KERN_DEBUG, "%s:%d:enter.\n", __func__, __LINE__);

    /* Poll, clear and process every read chanel interrupt status */
    for (i = 0; i < GB02MAC1; i++) {
        chan = &(dma_ctrl->rd_chan[i]);
        status = GB02FUNC13(chan, GB02MAC95);
        /* gb_printf(KERN_INFO, "%s %u HDMA_IRQ status: 0x%04x\n",
            __func__, i, status); */

        GB02FUNC1660(chan, status);
        if (status) {
            GB02FUNC1661(chan);
        }

        if (status & GB02MAC101) {
            /* error */
            gb_printf(KERN_ERR, "HDMA read channel %u error : 0x%x\n", i,
                (status & GB02MAC101) >> 3);
        }
        if (status & GB02MAC100) {
            /* abort */
            gb_printf(KERN_ERR, "HDMA read channel %u abort\n", i);
        }
        if (status & GB02MAC99) {
            /* watermark */
            gb_printf(KERN_ERR, "HDMA read channel %u watermark\n", i);
        }
        if (status & GB02MAC97) {
            /* done */
           gb_printf(KERN_ERR, "HDMA read channel %u done\n", i);
        }
    }

    for (i = 0; i < GB02MAC1; i++) {
        chan = &(dma_ctrl->wr_chan[i]);
        status = GB02FUNC13(chan, GB02MAC95);
        /* gb_printf(KERN_INFO, "%s %u HDMA_IRQ status: 0x%04x\n",
            __func__, i, status); */

        GB02FUNC1660(chan, status);
        if (status) {
            GB02FUNC1661(chan);
        }

        if (status & GB02MAC101) {
            /* error */
            gb_printf(KERN_ERR, "HDMA write channel %u error : 0x%x\n", i,
                (status & GB02MAC101) >> 3);
        }
        if (status & GB02MAC100) {
            /* abort */
            gb_printf(KERN_ERR, "HDMA write channel %u abort\n", i);
        }
        if (status & GB02MAC99) {
            /* watermark */
            gb_printf(KERN_ERR, "HDMA write channel %u watermark\n", i);
        }
        if (status & GB02MAC97) {
            /* done */
           gb_printf(KERN_ERR, "HDMA write channel %u done\n", i);
        }
    }

	/* TODO: send event to UMD */
    return IRQ_HANDLED;
}

static void __iomem *GB02FUNC1664(struct GB02STR70 *pcie_info,
	unsigned long num_dma_entry, phys_addr_t *ll_table_addr)
{
	unsigned long bitmap_no;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3, t4;
#else
	struct timespec64 t1, t2, t3, t4;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2, time_use3;
#endif
	void __iomem *ll_va = NULL;
	int ddr_bar_id;
	int bitmap_cnt;
	unsigned long *bitmap;
	unsigned long bitmap_maxno;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	ddr_bar_id = pcie_info->GB02STR153->ddr_bar_id;
	bitmap_cnt = 1 + num_dma_entry *
		GB02MAC26 / GB02MAC311;
	bitmap = pcie_info->edma_para.ll_bitmap.dma_ll_mem_bitmap;
	bitmap_maxno = pcie_info->edma_para.ll_bitmap.dma_ll_mem_bitmap_maxno;

	mutex_lock(&pcie_info->edma_para.ll_bitmap.lock);
	bitmap_no = bitmap_find_next_zero_area(bitmap,
					bitmap_maxno,
					0, bitmap_cnt, 0);

	//print(KERN_INFO "lldebug max:%lu, no:%lu, num_dma_entry:%lu, cnt:%d\n",
	//		bitmap_maxno, bitmap_no, num_dma_entry, bitmap_cnt);
	if (bitmap_no >= bitmap_maxno) {
			mutex_unlock(&pcie_info->edma_para.ll_bitmap.lock);
			return NULL;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	bitmap_set(bitmap, bitmap_no, bitmap_cnt);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
	mutex_unlock(&pcie_info->edma_para.ll_bitmap.lock);

	*ll_table_addr = GB02MAC506 +
		(bitmap_no << GB02MAC313);
	ll_va = (void *)pcie_info->pci_bars[ddr_bar_id].mmio +
		GB02MAC506 +
		(bitmap_no << GB02MAC313);
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
#endif
	memset_io(ll_va, 0, GB02MAC311 * bitmap_cnt);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t4);
#else
	ktime_get_real_ts64(&t4);
#endif
#ifdef GB_TIME_DEBUG
	time_use3=(t4.tv_sec-t3.tv_sec)*1000000+(t4.tv_nsec-t3.tv_nsec)/1000;
	/* gb_printf(KERN_INFO, "%s cost:1:%ld 2:%ld 3:%ld\n",
		__func__, time_use1, time_use2, time_use3); */
#endif
	return ll_va;
}

static void GB02FUNC1666(struct GB02STR70 *pcie_info,
	unsigned long num_dma_entry, phys_addr_t ll_table_addr)
{
	int bitmap_cnt = 1 + num_dma_entry *
		GB02MAC26 / GB02MAC311;
	unsigned long *bitmap =
		pcie_info->edma_para.ll_bitmap.dma_ll_mem_bitmap;
	unsigned long bitmap_no = (ll_table_addr -
		GB02MAC506) >>
		GB02MAC313;

	mutex_lock(&pcie_info->edma_para.ll_bitmap.lock);
	bitmap_clear(bitmap, bitmap_no, bitmap_cnt);
	mutex_unlock(&pcie_info->edma_para.ll_bitmap.lock);
}

static int GB02FUNC1668(struct GB02STR70 *pcie_info,
	bool dma_write)
{
	int i = 0;
	int timeout = 0;
	unsigned int *channel_usage;
	unsigned int GB02FUNC386 = 0;
	unsigned int int_status = 0;
	struct GB02STR8 *dma_ctrl = pcie_info->edma_para.dma_ctrl;
	struct GB02STR6 *chan = NULL;

	if (dma_write) {
		chan = dma_ctrl->wr_chan;
		channel_usage = &pcie_info->edma_para.wr_channel_usage;
	} else {
		chan = dma_ctrl->rd_chan;
		channel_usage = &pcie_info->edma_para.rd_channel_usage;
	}

	do {
		for (i = 0; i < GB02MAC1; i++) {
			GB02FUNC386 = GB02FUNC13(&chan[i], GB02MAC89);
			GB02FUNC386 &= GB02MAC90;
			int_status = GB02FUNC13(&chan[i], GB02MAC95);

			if ((GB02FUNC386 == GB02MAC60) ||
				(GB02FUNC386 == GB02MAC63) ||
				!(int_status & (GB02MAC100 | GB02MAC101))) {
				/*
				 * If channel i was occupied by other process,
				 * once we get this channel, there is at least
				 * 1 process will lose channel i stop sign, this
				 * process would assume DMA timeout, something
				 * goes wrong.
				 */
				if (!(*channel_usage & (1 << i))) {
					*channel_usage |= (1 << i);
					return i;
				}
			}
		}

		usleep_range(50, 100);
		timeout++;
	} while (timeout < 5000);

	return GB02MAC1;
}

void GB02FUNC1669(struct GB02STR6 *chan, bool dma_write)
{
	gb_printf(KERN_ERR, "====== HDMA %s REGISTER DUMP =====\n", dma_write? "WRCH" : "RDCH");
	gb_printf(KERN_ERR, "GB02MAC71:    0x%08x\n", GB02FUNC13(chan, GB02MAC71));
	gb_printf(KERN_ERR, "GB02MAC72:   0x%08x\n", GB02FUNC13(chan, GB02MAC72));
	gb_printf(KERN_ERR, "GB02MAC76:   0x%08x\n", GB02FUNC13(chan, GB02MAC76));
	gb_printf(KERN_ERR, "GB02MAC77:    0x%08x\n", GB02FUNC13(chan, GB02MAC77));
	gb_printf(KERN_ERR, "GB02MAC78:   0x%08x\n", GB02FUNC13(chan, GB02MAC78));
	gb_printf(KERN_ERR, "GB02MAC79:    0x%08x\n", GB02FUNC13(chan, GB02MAC79));
	gb_printf(KERN_ERR, "GB02MAC80:   0x%08x\n", GB02FUNC13(chan, GB02MAC80));
	gb_printf(KERN_ERR, "GB02MAC83:   0x%08x\n", GB02FUNC13(chan, GB02MAC83));
	gb_printf(KERN_ERR, "GB02MAC89:     0x%08x\n", GB02FUNC13(chan, GB02MAC89));
	gb_printf(KERN_ERR, "GB02MAC95: 0x%08x\n", GB02FUNC13(chan, GB02MAC95));
	gb_printf(KERN_ERR, "=====================================\n");
}

static int GB02FUNC1670(struct GB02STR70 *pcie_info,
	int channel_id, bool dma_write)
{
	int timeout = 0;
	unsigned int GB02FUNC386 = 0;
	struct GB02STR8 *dma_ctrl = pcie_info->edma_para.dma_ctrl;
	struct GB02STR6 *chan = NULL;
	u32 status = 0;

	if (dma_write)
		chan = &(dma_ctrl->wr_chan[channel_id]);
	else
		chan = &(dma_ctrl->rd_chan[channel_id]);

	do {
		GB02FUNC386 = GB02FUNC13(chan, GB02MAC89);
		GB02FUNC386 &= GB02MAC90;
		if (GB02FUNC386 == GB02MAC63)
			return 0;

		if (GB02FUNC386 == GB02MAC62) {
			gb_printf(KERN_ERR, "%s %d channel_id:%d dma trans aborted!\n",
				__func__, __LINE__, channel_id);
			goto err_handle;
		}

		usleep_range(50, 100);
		timeout++;
	} while (timeout < 5000);

	gb_printf(KERN_ERR, "%s %d channel_id:%d dma trans timeout!\n",
		__func__, __LINE__, channel_id);

err_handle:
	GB02FUNC1669(chan, dma_write);
	status = GB02FUNC13(chan, GB02MAC95);
	GB02FUNC1660(chan, status);
	if (status)
		GB02FUNC1661(chan);

	return -1;
}

static int GB02FUNC1673(struct GB02STR70 *pcie_info,
	struct GB02STR188 *dma_info)
{
	unsigned int value = 0, i;
	unsigned int channel_id = 0;
	int ret = 0;
	u64 ll_addr;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3, t4, t5;
#else
	struct timespec64 t1, t2, t3, t4, t5;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2, time_use3, time_use4;
#endif
	bool dma_write;
	struct GB02STR6 *chan = NULL;
	struct GB02STR8 *dma_ctrl = pcie_info->edma_para.dma_ctrl;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	dma_write = dma_info->dma_dir == GB_DMA_FROM_DEVICE ? true : false;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	mutex_lock(dma_write ? &pcie_info->edma_para.channel_wr_lock :
				&pcie_info->edma_para.channel_rd_lock);
	channel_id = GB02FUNC1668(pcie_info, dma_write);
	if (channel_id >= GB02MAC1) {
		mutex_unlock(dma_write ? &pcie_info->edma_para.channel_wr_lock :
						&pcie_info->edma_para.channel_rd_lock);
		gb_err(&pcie_info->pdev->dev, "%s %d dma get free channel failed!\n",
			__func__, __LINE__);
		return -1;
	}
	ll_addr = 0x800000000 + dma_info->ll_table_addr;
	/* HDMAC interrupt pass through */
	value = GB02FUNC25(dma_ctrl->hdmac_regs, GB02MAC11);
	value |= GB02MAC15;
	GB02FUNC21(value, dma_ctrl->hdmac_regs, GB02MAC11);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
	if (dma_write) {
		/* DMA write channel */
		chan = &(dma_ctrl->wr_chan[channel_id]);
		/* DMA Write Interrupt Enable */
		value = GB02FUNC13(chan, GB02MAC102);
		value &= ~(GB02MAC103 | GB02MAC105);
		value &= ~(GB02MAC106 | GB02MAC108);
		value |= (GB02MAC107 | GB02MAC109);
		GB02FUNC15(chan, GB02MAC102, value);

		/* DMA Write Channel Enable */
		GB02FUNC15(chan, GB02MAC64, GB02MAC65);

		/* DMA Channel Control 1 */
		GB02FUNC15(chan, GB02MAC83, GB02MAC84);
		GB02FUNC15(chan, GB02MAC73, GB02MAC75);

		/* DMA Linked List Pointer */
		GB02FUNC15(chan, GB02MAC71, lower_32_bits(ll_addr));
		GB02FUNC15(chan, GB02MAC72, upper_32_bits(ll_addr));

		/* DMA Write Doorbell */
		GB02FUNC15(chan, GB02MAC66, GB02MAC67);
	} else {
		/* DMA read channel */
		chan = &(dma_ctrl->rd_chan[channel_id]);
		/* DMA read Interrupt Enable */
		value = GB02FUNC13(chan, GB02MAC102);
		value &= ~(GB02MAC103 | GB02MAC105);
		value &= ~(GB02MAC106 | GB02MAC108);
		value |= (GB02MAC107 | GB02MAC109);
		GB02FUNC15(chan, GB02MAC102, value);

		/* DMA Read Channel Enable */
		GB02FUNC15(chan, GB02MAC64, GB02MAC65);

		/* DMA Channel Control 1 */
		GB02FUNC15(chan, GB02MAC83, GB02MAC84);
		GB02FUNC15(chan, GB02MAC73, GB02MAC75);

		/* DMA Linked List Pointer */
		GB02FUNC15(chan, GB02MAC71, lower_32_bits(ll_addr));
		GB02FUNC15(chan, GB02MAC72, upper_32_bits(ll_addr));

		/* DMA Read Doorbell */
		GB02FUNC15(chan, GB02MAC66, GB02MAC67);
	}

	mutex_unlock(dma_write ? &pcie_info->edma_para.channel_wr_lock :
				 &pcie_info->edma_para.channel_rd_lock);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t4);
#else
	ktime_get_real_ts64(&t4);
#endif
	ret = GB02FUNC1670(pcie_info, channel_id, dma_write);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t5);
#else
	ktime_get_real_ts64(&t5);
#endif
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
	time_use3 = (t4.tv_sec - t3.tv_sec) * 1000000 +
		(t4.tv_nsec - t3.tv_nsec)/1000;
	time_use4 = (t5.tv_sec - t4.tv_sec) * 1000000 +
		(t5.tv_nsec - t4.tv_nsec)/1000;
#endif
	//clear channel usage since CPU got dma done status
	mutex_lock(dma_write ? &pcie_info->edma_para.channel_wr_lock :
				 &pcie_info->edma_para.channel_rd_lock);
	if (dma_write)
		pcie_info->edma_para.wr_channel_usage &= (0 << channel_id);
	else
		pcie_info->edma_para.rd_channel_usage &= (0 << channel_id);
	mutex_unlock(dma_write ? &pcie_info->edma_para.channel_wr_lock :
				 &pcie_info->edma_para.channel_rd_lock);

	/* DMA Write Channel Disable */
	GB02FUNC15(chan, GB02MAC64, ~GB02MAC65);
#ifdef GB_TIME_DEBUG
	gb_printf(KERN_INFO, "%s, get channel time:%ld per register time:%ld  trans time: %ld %s\n", __func__, time_use2, time_use3, time_use4, dma_write ? "aaa" : "bbb");
#endif
	GB02FUNC1666(pcie_info, dma_info->num_dma_entry,
		dma_info->ll_table_addr);
	for (i = 0; i < dma_info->num_dma_pages; i++) {
		dma_unmap_page(&pcie_info->pdev->dev, dma_info->dma_buffer[i],
			PAGE_SIZE, !dma_info->dma_dir);
	}
	vfree(dma_info->dma_buffer);
	return ret;
}

static void GB02FUNC1678(void __iomem *ll_table_addr,
	struct GB02STR70 *pcie_info, struct GB02STR188 *dma_info)
{
	int offset_base, offset, i, j;
	dma_addr_t src_addr, src_addr_base;
	phys_addr_t dst_addr, dst_addr_base;
	int last_page_num = dma_info->num_dma_pages - 1;
	u64 table_size = dma_info->num_dma_pages * PAGE_SIZE / GB02MAC5 *
		GB02MAC26;
	void *table_addr = kvmalloc(table_size, GFP_KERNEL);
	memset(table_addr, 0x0, table_size);

	if (!table_addr)
		table_addr = ll_table_addr;
	dma_info->dma_buffer = vmalloc(sizeof(dma_addr_t) * dma_info->num_dma_pages);
	for (i = 0; i < dma_info->num_dma_pages; i++) {
		src_addr_base = dma_map_page(&pcie_info->pdev->dev,
			dma_info->cpu_pages[i],
			0, PAGE_SIZE, DMA_TO_DEVICE);
		dma_info->dma_buffer[i] = src_addr_base;
		dst_addr_base = 0x800000000 + dma_info->gpu_phys[GB02MAC317 * i];
		offset_base = i * PAGE_SIZE / GB02MAC5 *
			GB02MAC26;

		for (j = 0; j < PAGE_SIZE / GB02MAC5; j++) {
			src_addr = src_addr_base + j * GB02MAC5;
			dst_addr = dst_addr_base + j * GB02MAC5;
			offset = offset_base + j *
				GB02MAC26;
			/* enable interrupt for the last element */
			*(u32 *)(table_addr + offset +
				GB02MAC118) =
				((i == (dma_info->num_dma_pages - 1)) && (j == (PAGE_SIZE / GB02MAC5 - 1))) ?
				(GB02MAC126 | GB02MAC129) :
					GB02MAC74;
			if ((i == last_page_num) && (dma_info->last_page_size != 0))
				*(u32 *)(table_addr + offset +
					GB02MAC119) =
					dma_info->last_page_size;
			else
				*(u32 *)(table_addr + offset +
					GB02MAC119) =
					GB02MAC5;
			*(u32 *)(table_addr + offset +
				GB02MAC120) =
				lower_32_bits(src_addr);
			*(u32 *)(table_addr + offset +
				GB02MAC121) =
				upper_32_bits(src_addr);
			*(u32 *)(table_addr + offset +
				GB02MAC122) =
				lower_32_bits(dst_addr);
			*(u32 *)(table_addr + offset +
				GB02MAC123) =
				upper_32_bits(dst_addr);
		}
	}

	if (table_addr != ll_table_addr) {
		memcpy_toio(ll_table_addr, table_addr, table_size);
		kvfree(table_addr);
	}
}

/* VRAM ---> DDR */
static void GB02FUNC1679(void __iomem *ll_table_addr,
	struct GB02STR70 *pcie_info, struct GB02STR188 *dma_info)
{
	int offset_base, offset, i, j;
	dma_addr_t src_addr, src_addr_base;
	phys_addr_t dst_addr, dst_addr_base;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3;
#else
	struct timespec64 t1, t2, t3;
#endif
	int last_page_num = dma_info->num_dma_pages - 1;
	u64 table_size = dma_info->num_dma_pages * PAGE_SIZE / GB02MAC3 *
		GB02MAC26;
	void *table_addr = kvmalloc(table_size, GFP_KERNEL);
	memset(table_addr, 0x0, table_size);

#ifdef GB_TIME_DEBUG
	long time_use1, time_use2;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	if (!table_addr)
		table_addr = ll_table_addr;
	dma_info->dma_buffer = vmalloc(sizeof(dma_addr_t) * dma_info->num_dma_pages);
	for (i = 0; i < dma_info->num_dma_pages; i++) {
		src_addr_base = 0x800000000 + dma_info->gpu_phys[GB02MAC317 * i];
		dst_addr_base = dma_map_page(&pcie_info->pdev->dev,
			dma_info->cpu_pages[i],
			0, PAGE_SIZE, DMA_FROM_DEVICE);
		dma_info->dma_buffer[i] = dst_addr_base;
		offset_base = i * PAGE_SIZE / GB02MAC3 *
			GB02MAC26;

		for (j = 0; j < PAGE_SIZE / GB02MAC3; j++) {
			src_addr = src_addr_base + j * GB02MAC3;
			dst_addr = dst_addr_base + j * GB02MAC3;
			offset = offset_base + j *
				GB02MAC26;

			/* enable interrupt for the last element */
			*(u32 *)(table_addr + offset +
				GB02MAC118) =
				((i == (dma_info->num_dma_pages - 1)) && (j == (PAGE_SIZE / GB02MAC3 - 1))) ?
				(GB02MAC126 | GB02MAC129) :
					GB02MAC74;
			if ((i == last_page_num) && (dma_info->last_page_size != 0))
				*(u32 *)(table_addr + offset +
					GB02MAC119) =
					dma_info->last_page_size;
			else
				*(u32 *)(table_addr + offset +
					GB02MAC119) =
					GB02MAC3;
			*(u32 *)(table_addr + offset +
				GB02MAC120) =
				lower_32_bits(src_addr);
			*(u32 *)(table_addr + offset +
				GB02MAC121) =
				upper_32_bits(src_addr);
			*(u32 *)(table_addr + offset +
				GB02MAC122) =
				lower_32_bits(dst_addr);
			*(u32 *)(table_addr + offset +
				GB02MAC123) =
				upper_32_bits(dst_addr);
		}
	}
#ifdef GB_TIME_DEBUG
	for (i = 0; i < table_size / 24 ; i++) {
			gb_printf(KERN_INFO, "flag %lx", ((u32 *)table_addr)[i * 6]);
			gb_printf(KERN_INFO, "szie %lx", ((u32 *)table_addr)[i * 6 + 1]);
			gb_printf(KERN_INFO, "sar1 %lx", ((u32 *)table_addr)[i * 6 + 2]);
			gb_printf(KERN_INFO, "sar2 %lx", ((u32 *)table_addr)[i * 6 + 3]);
			gb_printf(KERN_INFO, "dar1 %lx", ((u32 *)table_addr)[i * 6 + 4]);
			gb_printf(KERN_INFO, "dar2 %lx", ((u32 *)table_addr)[i * 6 + 5]);
	}
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	if (table_addr != ll_table_addr) {
		memcpy_toio(ll_table_addr, table_addr, table_size);
		kvfree(table_addr);
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
	/* gb_printf(KERN_INFO, "%s cost:1:%ld 2:%ld\n",
		__func__, time_use1, time_use2); */
#endif
}

static int GB02FUNC1682(struct GB02STR70 *pcie_info,
	struct GB02STR188 *dma_info)
{
	void __iomem *base_addr;
	phys_addr_t ll_table_addr = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3;
#else
	struct timespec64 t1, t2, t3;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2;
#endif
	unsigned long num_dma_entry;

	if (dma_info->dma_dir == GB_DMA_TO_DEVICE) {
		num_dma_entry = dma_info->num_dma_pages *
			PAGE_SIZE / GB02MAC5;
		num_dma_entry++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
		getnstimeofday(&t1);
#else
		ktime_get_real_ts64(&t1);
#endif
		base_addr = GB02FUNC1664(pcie_info,
			num_dma_entry, &ll_table_addr);
		if (!base_addr) {
			gb_printf(KERN_ERR, "%s %d alloc ll table failed!\n", __func__, __LINE__);
			return -1;
		}
		dma_info->num_dma_entry = num_dma_entry;
		dma_info->ll_table_addr = ll_table_addr;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
		getnstimeofday(&t2);
#else
		ktime_get_real_ts64(&t2);
#endif
		GB02FUNC1678(base_addr, pcie_info, dma_info);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
		getnstimeofday(&t3);
#else
		ktime_get_real_ts64(&t3);
#endif
	} else {
		num_dma_entry = dma_info->num_dma_pages * PAGE_SIZE / GB02MAC3;
		num_dma_entry++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
		getnstimeofday(&t1);
#else
		ktime_get_real_ts64(&t1);
#endif
		base_addr = GB02FUNC1664(pcie_info, num_dma_entry, &ll_table_addr);
		if (!base_addr) {
			gb_printf(KERN_ERR, "%s %d alloc ll table failed!\n", __func__, __LINE__);
			return -1;
		}
		dma_info->num_dma_entry = num_dma_entry;
		dma_info->ll_table_addr = ll_table_addr;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
		getnstimeofday(&t2);
#else
		ktime_get_real_ts64(&t2);
#endif
		GB02FUNC1679(base_addr, pcie_info, dma_info);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
		getnstimeofday(&t3);
#else
		ktime_get_real_ts64(&t3);
#endif
	}

#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
	/* gb_printf(KERN_INFO, "%s cost:1:%ld 2:%ld\n",
		__func__, time_use1, time_use2); */
#endif
	return 0;
}

int GB02FUNC1688(struct device *dev,
	struct GB02STR188 *dma_info,
	struct GB02STR70 *pcie_info)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3;
#else
	struct timespec64 t1, t2, t3;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	/* first, prepare the memory of SAR */
	if (GB02FUNC1682(pcie_info, dma_info)) {
		gb_err(dev, "%s %d dma prepare failed!\n",
			__func__, __LINE__);
		return -1;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	/* second, start DMA transferring */
	mutex_lock(&pcie_info->edma_para.dma_lock);
	if (GB02FUNC1673(pcie_info, dma_info)) {
		mutex_unlock(&pcie_info->edma_para.dma_lock);
		gb_err(dev, "%s %d dma transfer failed!\n",
			__func__, __LINE__);
		return -1;
	}
	mutex_unlock(&pcie_info->edma_para.dma_lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
gb_printf(KERN_INFO, "%s cost GB02FUNC1682:%ld  GB02FUNC1673:%ld size %lld\n",
                __func__, time_use1, time_use2, (long long)(dma_info->num_dma_pages * 4096));
#endif
	return 0;
}

static void GB02FUNC1690(void __iomem *ll_table_va,
	phys_addr_t ll_table_pa,
	struct GB02STR70 *pcie_info,
	struct GB02STR105 *dma_info,
	struct page **user_pages, phys_addr_t gpu_phy)
{
	int i, j, ll_cnt;
	dma_addr_t src_addr;
	phys_addr_t dst_addr, dst_addr_base;
	u64 cpu_va_row_start, row_pg_offset, ll_offset, ll_offset_base = 0;
	u32 width_sz, height_sz, plane_width_sz, pg_num;
	u32 size, cur_size, ll_total_cnt = 0;
	u64 table_size = dma_info->num_dma_entry *
		GB02MAC26;
	void *table_addr = kzalloc(table_size, GFP_KERNEL);

	if (!table_addr)
		table_addr = ll_table_va;

	width_sz = 4 * dma_info->width;
	height_sz = 4 * dma_info->height;
	plane_width_sz = 4 * dma_info->plane_width;

	for (i = 0; i < dma_info->height; i++) {
		cpu_va_row_start = dma_info->cpu_va + i * plane_width_sz;
		row_pg_offset = (cpu_va_row_start & (PAGE_SIZE - 1));
		ll_cnt = DIV_ROUND_UP(row_pg_offset + width_sz, PAGE_SIZE);
		pg_num = (cpu_va_row_start - (dma_info->cpu_va & PAGE_MASK)) /
					PAGE_SIZE;

		src_addr = row_pg_offset + dma_map_page(&pcie_info->pdev->dev,
			user_pages[pg_num], 0, PAGE_SIZE, DMA_TO_DEVICE);
		dst_addr = 0x800000000 + gpu_phy + i * plane_width_sz;
		ll_offset = ll_offset_base;

		*(u32 *)(table_addr + ll_offset +
			GB02MAC118) =
			GB02MAC126;
		*(u32 *)(table_addr + ll_offset +
			GB02MAC120) =
			lower_32_bits(src_addr);
		*(u32 *)(table_addr + ll_offset +
			GB02MAC121) =
			upper_32_bits(src_addr);
		*(u32 *)(table_addr + ll_offset +
			GB02MAC122) =
			lower_32_bits(dst_addr);
		*(u32 *)(table_addr + ll_offset +
			GB02MAC123) =
			upper_32_bits(dst_addr);
		if (ll_cnt == 1) {
			*(u32 *)(table_addr + ll_offset +
				GB02MAC119) = width_sz;
			/* enable interrupt for the last element */
			*(u32 *)(table_addr + ll_offset +
				GB02MAC118) =
				(i == (dma_info->height - 1)) ?
				(GB02MAC126 | GB02MAC129) :
				GB02MAC74;
		} else {
			*(u32 *)(table_addr + ll_offset +
				GB02MAC119) =
				PAGE_SIZE - row_pg_offset;
			size = width_sz - (PAGE_SIZE - row_pg_offset);
			dst_addr_base = dst_addr + (PAGE_SIZE - row_pg_offset);
			for (j = 1; j < ll_cnt; j++) {
				cur_size = size;
				if (size > PAGE_SIZE)
					cur_size = PAGE_SIZE;
				size -= cur_size;

				src_addr = dma_map_page(pcie_info->gbdev->dev,
					user_pages[pg_num + j], 0,
					PAGE_SIZE, DMA_TO_DEVICE);
				dst_addr = dst_addr_base + (j - 1) * PAGE_SIZE;
				ll_offset = ll_offset_base +
					j * GB02MAC26;
				/* enable interrupt for the last element */
				*(u32 *)(table_addr + ll_offset +
					GB02MAC118) =
					((i == (dma_info->height - 1)) && (j == (ll_cnt - 1))) ?
					(GB02MAC126 | GB02MAC129) :
					GB02MAC74;
				*(u32 *)(table_addr + ll_offset +
					GB02MAC119) =
					cur_size;
				*(u32 *)(table_addr + ll_offset +
					GB02MAC120) =
					lower_32_bits(src_addr);
				*(u32 *)(table_addr + ll_offset +
					GB02MAC121) =
					upper_32_bits(src_addr);
				*(u32 *)(table_addr + ll_offset +
					GB02MAC122) =
					lower_32_bits(dst_addr);
				*(u32 *)(table_addr + ll_offset +
					GB02MAC123) =
					upper_32_bits(dst_addr);
			}
		}
		ll_total_cnt += ll_cnt;
		ll_offset_base += ll_cnt * GB02MAC26;
	}
/*
	for (i = 0; i < ll_total_cnt ; i++) {
			gb_printf(KERN_INFO, "flag %lx", ((u32 *)table_addr)[i * 6]);
			gb_printf(KERN_INFO, "szie %lx", ((u32 *)table_addr)[i * 6 + 1]);
			gb_printf(KERN_INFO, "sar1 %lx", ((u32 *)table_addr)[i * 6 + 2]);
			gb_printf(KERN_INFO, "sar2 %lx", ((u32 *)table_addr)[i * 6 + 3]);
			gb_printf(KERN_INFO, "dar1 %lx", ((u32 *)table_addr)[i * 6 + 4]);
			gb_printf(KERN_INFO, "dar2 %lx", ((u32 *)table_addr)[i * 6 + 5]);
	}
*/
	if (table_addr != ll_table_va) {
		memcpy_toio(ll_table_va, table_addr,
			ll_total_cnt * GB02MAC26);
		kfree(table_addr);
	}
}

static int GB02FUNC1693(struct GB02STR70 *pcie_info,
	struct GB02STR105 *dma_info,
	struct page **user_pages, phys_addr_t gpu_phy)
{
	void __iomem *ll_table_va;
	phys_addr_t ll_table_pa = 0;
	u32 entry_num_one_row;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3;
#else
	struct timespec64 t1, t2, t3;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2;
#endif
	unsigned long num_dma_entry;

	entry_num_one_row = DIV_ROUND_UP(4 * dma_info->width, PAGE_SIZE) + 1;
	num_dma_entry = dma_info->height * entry_num_one_row + 1;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	ll_table_va = GB02FUNC1664(pcie_info, num_dma_entry, &ll_table_pa);
	if (!ll_table_va) {
		gb_printf(KERN_ERR, "%s %d alloc ll table failed!\n", __func__, __LINE__);
		return -1;
	}

	dma_info->num_dma_entry = num_dma_entry;
	dma_info->ll_table_addr = ll_table_pa;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	GB02FUNC1690(ll_table_va, ll_table_pa,
		pcie_info, dma_info, user_pages, gpu_phy);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif

#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
	/* gb_printf(KERN_INFO, "%s cost:1:%ld 2:%ld\n",
		__func__, time_use1, time_use2); */
#endif
	return 0;
}

int GB02FUNC1699(struct device *dev,
	struct GB02STR105 *dma_info,
	phys_addr_t gpu_phy,
	struct page **user_pages,
	struct GB02STR70 *pcie_info)
{
	struct GB02STR188 dma_start_info;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3;
#else
	struct timespec64 t1, t2, t3;
#endif
#ifdef GB_TIME_DEBUG
	long time_use1, time_use2;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif
	/* first, prepare the memory of SAR */
	if (GB02FUNC1693(pcie_info, dma_info, user_pages, gpu_phy)) {
		gb_err(pcie_info->gbdev->dev, "%s %d dma prepare failed!\n",
			__func__, __LINE__);
		return -1;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	/* second, start DMA transferring */
	mutex_lock(&pcie_info->edma_para.dma_lock);
	dma_start_info.dma_dir = dma_info->dma_direction;
	dma_start_info.ll_table_addr = dma_info->ll_table_addr;
	dma_start_info.num_dma_entry = dma_info->num_dma_entry;

	if (GB02FUNC1673(pcie_info, &dma_start_info)) {
		mutex_unlock(&pcie_info->edma_para.dma_lock);
		gb_err(pcie_info->gbdev->dev, "%s %d dma transfer failed!\n",
			__func__, __LINE__);
		return -1;
	}
	mutex_unlock(&pcie_info->edma_para.dma_lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
	gb_printf(KERN_INFO, "%s GB02FUNC1693:%ld GB02FUNC1673:%ld size %lld, %dx*%d\n",
                __func__, time_use1, time_use2, (long long)(dma_info->height * dma_info->width * 4), dma_info->width, dma_info->height);
#endif
	return 0;
}

struct page **gb_edma_get_user_cpu_pages(
	u64 cpu_va, enum gb_dma_data_direction dma_dir,
	unsigned long num_dma_pages)
{
	struct page **user_pages = NULL;
	unsigned long getted_num = 0;
	int i, cnt;

	user_pages = kvzalloc(sizeof(struct page *) * num_dma_pages,
			GFP_KERNEL);
	if (!user_pages) {
		gb_printf(KERN_ERR, "%s %d alloc dma %lu pages failed!\n",
			__func__, __LINE__, num_dma_pages);
		return NULL;
	}

	do {
		unsigned num_pages = num_dma_pages - getted_num;
		u64 userptr = cpu_va + getted_num * PAGE_SIZE;
		struct page **pages = user_pages + getted_num;

		cnt = get_user_pages_fast(userptr, num_pages,
				dma_dir == GB_DMA_FROM_DEVICE ? FOLL_WRITE : 0,
				pages);
		if (cnt < 0)
			goto err_release_pages;

		getted_num += cnt;

	} while (getted_num < num_dma_pages);

	return user_pages;

err_release_pages:
	for (i = 0; i < getted_num; i++)
		put_page(user_pages[i]);
	kvfree(user_pages);

	return NULL;
}

int GB02FUNC1703(struct GB02STR70 *pcie_info)
{
	int result = 0;
	struct GB02STR8 *dma_ctrl = NULL;
	struct GB02STR6 *chan = NULL;
	int hdmac_reg_bar_id = GB02FUNC465(pcie_info->GB02STR153);
	int hdma_reg_bar_id = GB02FUNC477(pcie_info->GB02STR153);
	int i, ll_bitmap_sz;

	gb_printf(KERN_INFO, "Start to init PCIE HDMA:\n");
	dma_ctrl = (struct GB02STR8 *)devm_kzalloc(&pcie_info->pdev->dev,
		sizeof(struct GB02STR8), GFP_KERNEL);
	if (!dma_ctrl) {
		gb_printf(KERN_ERR, "Error: %s, kmalloc failed\n", __func__);
		return -ENOMEM;
	}

	//init dma_channel_usgae
	pcie_info->edma_para.rd_channel_usage = 0;
	pcie_info->edma_para.wr_channel_usage = 0;

	dma_ctrl->pdev = pcie_info->pdev;
	dma_ctrl->hdmac_regs = pcie_info->pci_bars[hdmac_reg_bar_id].mmio + GB02MAC1048;
	dma_ctrl->regs = pcie_info->pci_bars[hdma_reg_bar_id].mmio + GB02MAC7;
	/* write channel */
	for (i = 0; i < GB02MAC1; i++) {
		chan = &(dma_ctrl->wr_chan[i]);
		chan->id = i;
		chan->rd_chnl_flag = 0;
		chan->chan_regs = dma_ctrl->regs + GB02MAC32 + i * GB02MAC34;
		chan->chip = dma_ctrl;
	}
	/* read channel */
	for (i = 0; i < GB02MAC1; i++) {
		chan = &(dma_ctrl->rd_chan[i]);
		chan->id = i;
		chan->rd_chnl_flag = 1;
		chan->chan_regs = dma_ctrl->regs + GB02MAC32 + i * GB02MAC34 + GB02MAC36;
		chan->chip = dma_ctrl;
	}

	dma_ctrl->ll_data_element_num_max = GB02MAC27;
	dma_ctrl->ll_link_element_num = GB02MAC29;

	pcie_info->edma_para.dma_ctrl = dma_ctrl;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
	result = dma_set_mask(&pcie_info->pdev->dev, DMA_BIT_MASK(64));
#else
	result = pci_set_dma_mask(pcie_info->pdev, DMA_BIT_MASK(64));
#endif
	if(result) {
		gb_printf(KERN_ERR, "%s:%d:No suitable PCI mapping available(%d).\n", __func__, __LINE__, result);
		goto err;
	}

	pcie_info->edma_para.ll_bitmap.dma_ll_mem_bitmap_maxno =
		GB02MAC494 >> GB02MAC313;
	ll_bitmap_sz = BITS_TO_LONGS(GB02MAC494 >> GB02MAC313) *
		sizeof(long);
	pcie_info->edma_para.ll_bitmap.dma_ll_mem_bitmap =
		devm_kzalloc(&pcie_info->pdev->dev, ll_bitmap_sz, GFP_KERNEL);

	if (!pcie_info->edma_para.ll_bitmap.dma_ll_mem_bitmap) {
		gb_printf(KERN_ERR, "%s %d gb alloc 0x%lx dma_ll_mem_bitmap failed!\n",
			__func__, __LINE__,
			pcie_info->edma_para.ll_bitmap.dma_ll_mem_bitmap_maxno);
		goto err;
	}

	mutex_init(&pcie_info->edma_para.channel_rd_lock);
	mutex_init(&pcie_info->edma_para.channel_wr_lock);
	mutex_init(&pcie_info->edma_para.dma_lock);
	mutex_init(&pcie_info->edma_para.ll_bitmap.lock);

	return 0;
err:
	devm_kfree(&pcie_info->pdev->dev, dma_ctrl);
	return -1;
}
