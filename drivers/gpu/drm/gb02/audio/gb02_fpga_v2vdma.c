/* Copyright (C) Xi'an Sietium Electronics Co.Ltd */
/*
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
#include <linux/io.h>
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
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
#include "local.h"

#include "common/xt.h"
#include "common/gb_common.h"
#include "audio/gb02_fpga_v2vdma.h"

void GB02FUNC83(struct GB02STR42 *chip)
{
	u32 val;

	val = GB02FUNC170(chip, GB02MAC338);
	val &= ~GB02MAC415;
	GB02FUNC173(chip, GB02MAC338, val);
}

void GB02FUNC88(struct GB02STR42 *chip)
{
	u32 val;

	val = GB02FUNC170(chip, GB02MAC338);
	val |= GB02MAC415;
	GB02FUNC173(chip, GB02MAC338, val);
}

void GB02FUNC92(struct GB02STR42 *chip)
{
	u32 val;

	val = GB02FUNC170(chip, GB02MAC338);
	val &= ~GB02MAC420;
	GB02FUNC173(chip, GB02MAC338, val);
}

void GB02FUNC97(struct GB02STR42 *chip)
{
	u32 val;

	val = GB02FUNC170(chip, GB02MAC338);
	val |= GB02MAC420;
	GB02FUNC173(chip, GB02MAC338, val);
}

void GB02FUNC101(struct GB02STR40 *chan, u32 irq_mask)
{
	u32 val;

	if (likely(irq_mask == DWAXIDMAC_IRQ_ALL))
		GB02FUNC15(chan, GB02MAC404, DWAXIDMAC_IRQ_NONE);
	else {
		val = GB02FUNC13(chan, GB02MAC404);
		val &= ~irq_mask;
		GB02FUNC15(chan, GB02MAC404, val);
	}
}

void GB02FUNC104(struct GB02STR40 *chan, u32 irq_mask)
{
	GB02FUNC15(chan, GB02MAC404, irq_mask);
}

void GB02FUNC105(struct GB02STR40 *chan, u32 irq_mask)
{
	GB02FUNC15(chan, GB02MAC408, irq_mask);
}

void GB02FUNC106(struct GB02STR40 *chan, u32 irq_mask)
{
	GB02FUNC15(chan, GB02MAC410, irq_mask);
}

u32 GB02FUNC109(struct GB02STR40 *chan)
{
	return GB02FUNC13(chan, GB02MAC406);
}

void GB02FUNC111(struct GB02STR40 *chan)
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

void GB02FUNC114(struct GB02STR42 *chip)
{
	u32 i;

	for (i = 0; i < GB02MAC309; i++) {
		GB02FUNC101(&chip->chan[i], DWAXIDMAC_IRQ_ALL);
		GB02FUNC111(&chip->chan[i]);
	}
}

static inline bool GB02FUNC120(struct GB02STR40 *chan)
{
	u32 val;

	val = GB02FUNC170(chan->chip, GB02MAC340);

	return !!(val & (BIT(chan->id) << GB02MAC424));
}

int GB02FUNC125(struct GB02STR42 *dma_ctrl,
	u64 addr, int chid)
{
	struct GB02STR40 *chan = &(dma_ctrl->chan[chid]);

#ifdef	GB02MAC296
	/* CHX_SAR */
//	gb_printf(KERN_INFO, "--------dma src addr =%llx--\n", addr);
	GB02FUNC17(chan, GB02MAC358, cpu_to_le64(addr));
#else
	gb_printf(KERN_INFO, "---restart dma---dma src addr =%llx--\n", addr);
	GB02FUNC17(chan, GB02MAC358, cpu_to_le64(addr));

#endif
	GB02FUNC113(chan);
	return 0;
}

void GB02FUNC132(struct GB02STR42 *dma_ctrl,
	int abytes, int chid)
{
	struct GB02STR40 *chan = &(dma_ctrl->chan[chid]);
	u32 val;

	/* CHX_BLOCK_TS */
	val = (abytes >> DWAXIDMAC_TRANS_WIDTH_32) - 1;
	GB02FUNC15(chan, GB02MAC362, cpu_to_le32(val));
}

void GB02FUNC134(struct GB02STR42 *dma_ctrl)
{
	/* DMAC_CFGREG */
	GB02FUNC88(dma_ctrl);
	GB02FUNC97(dma_ctrl);

	/* GB02MAC349 */
	GB02FUNC173(dma_ctrl, GB02MAC349, GENMASK(20, 0));
	/* GB02MAC351 */
	GB02FUNC173(dma_ctrl, GB02MAC351, GENMASK(20, 0));
}

static void GB02FUNC135(struct GB02STR42 *dma_ctrl,
	int abytes, int chid)
{
	struct GB02STR40 *chan = &(dma_ctrl->chan[chid]);
	u32 val = 0, irq_mask = 0;

#ifdef	GB02MAC296
	/* CHX_SAR */
	gb_printf(KERN_INFO, "dma src addr = %llx\n",
		dma_ctrl->dma_src_paddr[chid]);
	GB02FUNC17(chan, GB02MAC358,
		cpu_to_le64(dma_ctrl->dma_src_paddr[chid]));
#else
	gb_printf(KERN_INFO, "dma src addr = %llx\n",
		dma_ctrl->dma_src_paddr[chid]);
	GB02FUNC17(chan, GB02MAC358,
		cpu_to_le64(dma_ctrl->dma_src_paddr[chid]));

#endif
	/* CHX_DAR */
#ifdef	GB02MAC296
	gb_printf(KERN_INFO, "--dma dts addr =%llx--\n",
		dma_ctrl->dma_dst_paddr + chid * GB02MAC299);
	val = dma_ctrl->dma_dst_paddr;
	GB02FUNC17(chan, GB02MAC360,
		cpu_to_le64(dma_ctrl->dma_dst_paddr + chid * GB02MAC299));
#else
	gb_printf(KERN_INFO, "--------dma dts addr =%llx--\n",
		dma_ctrl->dma_dst_paddr + chid * GB02MAC299);
	val = dma_ctrl->dma_dst_paddr;
	GB02FUNC17(chan, GB02MAC360,
		cpu_to_le64(dma_ctrl->dma_dst_paddr + chid * GB02MAC299));
#endif
	/* CHX_BLOCK_TS */
	val = (abytes >> DWAXIDMAC_TRANS_WIDTH_32) - 1;
	gb_printf(KERN_INFO, "%s-%d: i2s start  abytes = 0x%x, val = 0x%x\n",
		__func__, __LINE__,  abytes, val);
	GB02FUNC15(chan, GB02MAC362, cpu_to_le32(val));
	/* CHX_CTL */
	val = (DWAXIDMAC_BURST_TRANS_LEN_8 << GB02MAC455 |
		DWAXIDMAC_BURST_TRANS_LEN_8 << GB02MAC457 |
		GB02MAC312 << GB02MAC467 |
		GB02MAC312 << GB02MAC468 |
		DWAXIDMAC_CH_CTL_L_NOINC << GB02MAC471 |
		DWAXIDMAC_CH_CTL_L_INC << GB02MAC472 |
		GB02MAC473);
	val &= ~(GB02MAC474);
	GB02FUNC15(chan, GB02MAC366, val);
	GB02FUNC15(chan, GB02MAC368, 0x48240);

	/* CHX_CFG */
	val = (DWAXIDMAC_MBLK_TYPE_CONTIGUOUS
		<< GB02MAC479 |
		DWAXIDMAC_MBLK_TYPE_CONTIGUOUS
		<< GB02MAC480);
	GB02FUNC15(chan, GB02MAC372, val);
	val = (DWAXIDMAC_TT_FC_MEM_TO_PER_DST << GB02MAC478 |
		GB02MAC318 << GB02MAC475 |
		DWAXIDMAC_HS_SEL_HW << GB02MAC476 |
		DWAXIDMAC_HS_SEL_HW << GB02MAC477);
	switch (chid) {
	case 0:
		GB02FUNC15(chan, GB02MAC374, 0x7f8a0009);
		break;
	case 1:
		GB02FUNC15(chan, GB02MAC374, 0x7f8a1089);
		break;
	case 2:
		GB02FUNC15(chan, GB02MAC374, 0x7f8a2109);
		break;
	case 3:
		GB02FUNC15(chan, GB02MAC374, 0x7f8a3189);
		break;
	case 4:
		GB02FUNC15(chan, GB02MAC374, 0x7f8a4209);
		break;
	case 5:
		GB02FUNC15(chan, GB02MAC374, 0x7f8a5289);
		break;
	default:
		break;
	}
	GB02FUNC15(chan, GB02MAC380, 0x00000003);

	/* GB02MAC404 */
	irq_mask = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_ALL_ERR;
	GB02FUNC105(chan, irq_mask);

	/* Generate 'suspend' status but don't generate interrupt */
	irq_mask |= DWAXIDMAC_IRQ_SUSPENDED;
	GB02FUNC104(chan, irq_mask);

	/* DMAC_CHENREG */
	GB02FUNC113(chan);
}

int GB02FUNC147(struct GB02STR42 *dma_ctrl,
	int data_bytes, int chid)
{
	int ret = 0;

	/* second, prepare for DMA transferring */
	GB02FUNC135(dma_ctrl, data_bytes, chid);
	return ret;
}

void GB02FUNC150(struct GB02STR40 *chan, u32 status)
{
	 GB02FUNC111(chan);
}

void GB02FUNC151(struct GB02STR40 *chan)
{
	if (unlikely(GB02FUNC120(chan))) {
		gb_printf(KERN_ERR, "BUG: channel not idle!\n");
		GB02FUNC111(chan);
	}
}
