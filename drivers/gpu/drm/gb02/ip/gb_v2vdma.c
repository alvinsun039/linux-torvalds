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
#include "common/xt.h"
#include "common/gb_irq.h"
#include "common/gb_pcie_info.h"
#include "gb_v2vdma.h"

static struct GB02STR238 *v2vdma_infos;

static inline void GB02FUNC83(struct GB02STR238 *chip)
{
    u32 val;

    val = GB02FUNC170(chip, GB02MAC338);
    val &= ~GB02MAC415;
    GB02FUNC173(chip, GB02MAC338, val);
}

static inline void GB02FUNC88(struct GB02STR238 *chip)
{
    u32 val;

    val = GB02FUNC170(chip, GB02MAC338);
    val |= GB02MAC415;
    GB02FUNC173(chip, GB02MAC338, val);
}

static inline void GB02FUNC92(struct GB02STR238 *chip)
{
    u32 val;

    val = GB02FUNC170(chip, GB02MAC338);
    val &= ~GB02MAC420;
    GB02FUNC173(chip, GB02MAC338, val);
}

static inline void GB02FUNC97(struct GB02STR238 *chip)
{
    u32 val;

    val = GB02FUNC170(chip, GB02MAC338);
    val |= GB02MAC420;
    GB02FUNC173(chip, GB02MAC338, val);
}

static inline void GB02FUNC101(struct GB02STR40 *chan, u32 irq_mask)
{
    u32 val;

    if (likely(irq_mask == DWAXIDMAC_IRQ_ALL)) {
        GB02FUNC15(chan, GB02MAC404, DWAXIDMAC_IRQ_NONE);
    } else {
        val = GB02FUNC13(chan, GB02MAC404);
        val &= ~irq_mask;
        GB02FUNC15(chan, GB02MAC404, val);
    }
}

static inline void GB02FUNC104(struct GB02STR40 *chan, u32 irq_mask)
{
    GB02FUNC15(chan, GB02MAC404, irq_mask);
}

static inline void GB02FUNC105(struct GB02STR40 *chan, u32 irq_mask)
{
    GB02FUNC15(chan, GB02MAC408, irq_mask);
}

static inline void GB02FUNC106(struct GB02STR40 *chan, u32 irq_mask)
{
    GB02FUNC15(chan, GB02MAC410, irq_mask);
}

static inline u32 GB02FUNC109(struct GB02STR40 *chan)
{
    return GB02FUNC13(chan, GB02MAC406);
}

static inline void GB02FUNC111(struct GB02STR40 *chan)
{
    u32 val;

    val = GB02FUNC170(chan->chip, GB02MAC340);
    val &= ~(BIT(chan->id) << GB02MAC424);
    val |=   BIT(chan->id) << GB02MAC427;
    GB02FUNC173(chan->chip, GB02MAC340, val);
}

static inline void GB02FUNC113(struct GB02STR40 *chan)
{
    u32 val;

    val = GB02FUNC170(chan->chip, GB02MAC340);
    val |= BIT(chan->id) << GB02MAC424 |
           BIT(chan->id) << GB02MAC427;
    GB02FUNC173(chan->chip, GB02MAC340, val);
}

static int GB02FUNC114(struct GB02STR238 *chip)
{
	u32 i = 0;

    for (i = 0; i < GB02MAC309; i++) {
		GB02FUNC101(&chip->chan[i], DWAXIDMAC_IRQ_ALL);
		GB02FUNC111(&chip->chan[i]);
	}

	return 0;
}

static inline bool GB02FUNC120(struct GB02STR40 *chan)
{
	u32 val;

	val = GB02FUNC170(chan->chip, GB02MAC340);

	return !!(val & (BIT(chan->id) << GB02MAC424));
}

static noinline void GB02FUNC150(struct GB02STR40 *chan, u32 status)
{
#if 0
    int ret = 0, i = 0;
    struct dma_ctrl_info *chip = chan->chip;
    struct GB02STR38 *desc;

	GB02FUNC111(chan);

    if (status & DWAXIDMAC_IRQ_INVALID_ERR) {
        if (chip->dma_mode_flag == DMA_SHADOW_MODE) {
            /* shadow mode */

            /* prepare for next block's transfer */
            desc = list_next_entry(chan->first_desc, xfer_list);
            for (i = 0;i < chan->shadow_blk_cnt;i++) {
                desc = list_next_entry(desc, xfer_list);
            }

            if (desc) {
                ret = write_chan_shadow(chan, desc);
            }

            GB02FUNC15(chan, GB02MAC385, 1);

            chan->shadow_blk_cnt++;
            GB02FUNC113(chan);
        }
    }
#endif
}

static void GB02FUNC151(struct GB02STR40 *chan)
{
#if 0
	struct dma_ctrl_info *chip = chan->chip;
	long xfer_period_us = 0L;

    if (unlikely(GB02FUNC120(chan))) {
        gb_printf(KERN_ERR, "BUG: caught DWAXIDMAC_IRQ_DMA_TRF, but channel not idle!\n");
        GB02FUNC111(chan);
    }

	if (chip->invalid_test1_flag) {
		chip->invalid_test1_flag = false;

        do_gettimeofday(&(chip->dma_end_tv));

        xfer_period_us = (chip->dma_end_tv.tv_sec * 1000 * 1000 + chip->dma_end_tv.tv_usec) -
                        (chip->dma_start_tv.tv_sec * 1000 * 1000 + chip->dma_start_tv.tv_usec);
        gb_printf(KERN_INFO, "invalid test1 transfer cost : %ld us.\n", xfer_period_us);
	}
#endif
}

irqreturn_t GB02FUNC1854(int irq, void *arg)
{
    struct GB02STR238 *chip = v2vdma_infos;
    struct GB02STR40 *chan;
    int i = 0;
    u32 status = 0, val = 0;

	/* gb_printf(KERN_DEBUG, "%s:%d:enter.\n", __func__, __LINE__); */

    /* Disable DMAC inerrupts. We'll enable them after processing chanels */
    GB02FUNC92(chip);

    status = GB02FUNC170(chip, GB02MAC353);
    if (status) {
        GB02FUNC173(chip, GB02MAC347, status);
        val = GB02FUNC170(chip, GB02MAC349);
        val &= ~status;
        GB02FUNC173(chip, GB02MAC349, val);
    }

    /* Poll, clear and process every chanel interrupt status */
    for (i = 0; i < GB02MAC309; i++) {
        chan = &chip->chan[i];
        status = GB02FUNC109(chan);
        GB02FUNC106(chan, status);

        gb_printf(KERN_DEBUG, "%s %u IRQ status: 0x%08x\n",
            __func__, i, status);

        if (status & DWAXIDMAC_IRQ_ALL_ERR)
            GB02FUNC150(chan, status);
        else if (status & DWAXIDMAC_IRQ_DMA_TRF) {
            GB02FUNC151(chan);
            atomic_set(&chan->stop, 0);
        }
    }

    /* Re-enable interrupts */
    GB02FUNC97(chip);

	/* TODO: send event to UMD */
    return IRQ_HANDLED;
}

/* single block mode */
int GB02FUNC1855(phys_addr_t src_addr,
	phys_addr_t dst_addr, uint32_t size)
{
	struct GB02STR40 *chan = NULL;
	u32 dma_chnl = 0, val = 0, irq_mask = 0;
	unsigned long long addr = 0LL;
	struct GB02STR238 *dma_ctrl = v2vdma_infos;
	u64 timeout = 0;
	u32 status;

	if (dma_chnl >= GB02MAC309) {
		gb_printf(KERN_ERR, "%s:%d: can not get DMA channel(%d).\n",
			__func__, __LINE__, dma_chnl);
		return -1;
	}
	chan = &(dma_ctrl->chan[dma_chnl]);
	atomic_set(&chan->stop, 1);
	/* DMAC_CFGREG */
	GB02FUNC88(dma_ctrl);
	GB02FUNC97(dma_ctrl);

	/* GB02MAC349 */
	GB02FUNC173(dma_ctrl, GB02MAC349, GENMASK(20, 0));
	/* GB02MAC351 */
	GB02FUNC173(dma_ctrl, GB02MAC351, GENMASK(20, 0));

	/* CHX_SAR */
	addr = GB02MAC2823 + src_addr;
	GB02FUNC17(chan, GB02MAC358, cpu_to_le64(addr));
	/* CHX_DAR */
	addr = GB02MAC2823 + dst_addr;
	GB02FUNC17(chan, GB02MAC360, cpu_to_le64(addr));
	/* CHX_BLOCK_TS */
	val = (size >> DWAXIDMAC_TRANS_WIDTH_256) - 1;
	GB02FUNC15(chan, GB02MAC362, cpu_to_le32(val));
	/* CHX_CTL */
	val = (DWAXIDMAC_BURST_TRANS_LEN_16 << GB02MAC455 |
            DWAXIDMAC_BURST_TRANS_LEN_16 << GB02MAC457 |
            GB02MAC312 << GB02MAC467 |
            GB02MAC312 << GB02MAC468 |
            DWAXIDMAC_CH_CTL_L_INC << GB02MAC471 |
            DWAXIDMAC_CH_CTL_L_INC << GB02MAC472);
	val &= ~(GB02MAC474 | GB02MAC473);
	GB02FUNC15(chan, GB02MAC366, val);

	/* CHX_CFG */
	val = (DWAXIDMAC_MBLK_TYPE_CONTIGUOUS << GB02MAC479 |
           DWAXIDMAC_MBLK_TYPE_CONTIGUOUS << GB02MAC480);
	GB02FUNC15(chan, GB02MAC372, val);
	val = (DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC << GB02MAC478 |
           GB02MAC318 << GB02MAC475 |
           DWAXIDMAC_HS_SEL_SW << GB02MAC476 |
           DWAXIDMAC_HS_SEL_SW << GB02MAC477);
	GB02FUNC15(chan, GB02MAC374, val);

	/* GB02MAC404 */
	irq_mask = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_ALL_ERR;
	GB02FUNC105(chan, irq_mask);

	/* Generate 'suspend' status but don't generate interrupt */
	irq_mask |= DWAXIDMAC_IRQ_SUSPENDED;
	GB02FUNC104(chan, irq_mask);

	/* DMAC_CHENREG */
	GB02FUNC113(chan);
	/* wait v to v dma trans end */
	do {
		status = GB02FUNC109(chan);
		GB02FUNC106(chan, status);
		if (status & DWAXIDMAC_IRQ_ALL_ERR) {
			GB02FUNC150(chan, status);
			break;
		} else if (status & DWAXIDMAC_IRQ_DMA_TRF) {
			GB02FUNC151(chan);
			return 0;
		}
		timeout++;
		udelay(2);
	} while (timeout < 30000);
	gb_printf(KERN_INFO, "%s IRQ status: 0x%08x timeout\n", __func__, atomic_read(&chan->stop));

	return -1;
}

int GB02FUNC1856(struct GB02STR70 *pcie_info)
{
	int result = 0;
	struct GB02STR238 *dma_ctrl;
	int i = 0;
	int ddr_bar_id = GB02FUNC468(pcie_info->GB02STR153);
	int reg_bar_id = GB02FUNC465(pcie_info->GB02STR153);

	dma_ctrl = (struct GB02STR238 *)kzalloc(sizeof(struct GB02STR238), GFP_KERNEL);
	if (dma_ctrl == NULL) {
		gb_printf(KERN_ERR, "Error: %s, kmalloc failed\n", __func__);
		return -ENOMEM;
	}

	dma_ctrl->regs = pcie_info->pci_bars[reg_bar_id].mmio + GB02MAC1058;
	dma_ctrl->vram_addr = pcie_info->pci_bars[ddr_bar_id].mmio;
	for (i = 0; i < GB02MAC309; i++) {
		struct GB02STR40 *chan = &(dma_ctrl->chan[i]);
		chan->id = i;
		chan->chan_regs = dma_ctrl->regs + GB02MAC32 + i * GB02MAC34;
    	chan->chip = dma_ctrl;
    	atomic_set(&chan->descs_allocated, 0);
	}

	result = GB02FUNC114(dma_ctrl);
	if (result)
		goto err_kfree;

	v2vdma_infos = dma_ctrl;

	return result;
err_kfree:
	kfree(dma_ctrl);
	gb_printf(KERN_ERR, "%s %d fail!\n", __func__, __LINE__);
	return result;
}

void GB02FUNC1857(struct GB02STR70 *pcie_info)
{
	gb_printf(KERN_ERR, "%s %d v2vdma_info:0x%pK\n",
		__func__, __LINE__, v2vdma_infos);

	if (v2vdma_infos) {
		kfree(v2vdma_infos);
		v2vdma_infos = NULL;
	}
}
