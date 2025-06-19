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

#ifndef _EDMA_REG_H
#define _EDMA_REG_H

/* EDMA */
#define GB02MAC1 (4) /* max 4 channel for both read & write*/
/* from vram to ram, DMA can transfer 256B one step in ll mode */
#define GB02MAC3	(PAGE_SIZE)
#define GB02MAC5	(PAGE_SIZE)
#define GB02MAC6	0x60000
#define GB02MAC7	0x1000
#define GB02MAC8		0x800000
#define GB02MAC11	0x21c4
#define GB02MAC15	BIT(1)
#define GB02MAC18	0x21c8
#define GB02MAC21	BIT(0)

#define GB02MAC24				(0x4000) /* 16KB */
#define GB02MAC26	(24) /* 6 32-bit double words */
#define GB02MAC27		(0x2000) /* maximum 8192 LL data element */
#define GB02MAC29	(2) /* 2 link element */

#define GB02MAC32						(0)
#define GB02MAC34						(0x200)
#define GB02MAC36					(0x100)

/* BAR0 register */
#define GB02MAC39     0x200
#define GB02MAC40     0x204
#define GB02MAC42     0x208
#define GB02MAC44     0x5a
#define GB02MAC46     0x20c
#define GB02MAC48  0x5a
#define GB02MAC49     0x210
#define GB02MAC50     0x55

struct GB02STR4 {
	unsigned int dma_chnl; /* 0 --- 3 */
	unsigned int rd_wr_flag; /* 0 : write channel; 1 : read channel */
};

typedef enum {
    DMA_LL_MODE = 0,
    DMA_NON_LL_MODE,
    MULTI_CHNL_DMA_MODE_MAX
} multi_chnl_dma_mode_e;

struct GB02STR6 {
	void __iomem *chan_regs;
	u8 id;
	u8 rd_chnl_flag; /* 0 : write channel; 1 : read channel */
    struct GB02STR8 *chip;
};

struct GB02STR8 {
	struct pci_dev *pdev;
	unsigned long long *ll_structure[GB02MAC27 + GB02MAC29];
	unsigned int ll_data_element_num_max;
	unsigned int ll_link_element_num;
	unsigned int dma_data_element_index;
	unsigned int irq_cnt;
	void __iomem *regs;
	void __iomem *hdmac_regs;
	struct GB02STR6 wr_chan[GB02MAC1];
	struct GB02STR6 rd_chan[GB02MAC1];
	struct task_struct *thread[2];
};

static inline u32 GB02FUNC13(struct GB02STR6 *chan, u32 offset)
{
	u32 val = 0;

	val = ioread32(chan->chan_regs + offset);
	/* gb_printf(KERN_DEBUG, "%s CHAN%d REG: read 0x%x value: 0x%x\n",
		chan->rd_chnl_flag ? "read" : "write", chan->id, offset, val); */

	return val;
}

static inline void GB02FUNC15(struct GB02STR6 *chan, u32 offset, u32 val)
{
	iowrite32(val, chan->chan_regs + offset);

	/* gb_printf(KERN_DEBUG, "%s CHAN%d REG: write 0x%x value: 0x%x\n",
		chan->rd_chnl_flag ? "read" : "write", chan->id, offset, val); */
}

static inline void GB02FUNC17(struct GB02STR6 *chan, u32 offset, u64 val)
{
	iowrite32(lower_32_bits(val), chan->chan_regs + offset);
	iowrite32(upper_32_bits(val), chan->chan_regs + offset + 4);

	gb_printf(KERN_DEBUG, "%s CHAN%d REG: write 0x%x value: 0x%llx\n",
		chan->rd_chnl_flag ? "read" : "write", chan->id, offset, val);
}

static inline void GB02FUNC21(u32 val, void __iomem *regbase, u32 offset)
{
	iowrite32(val, regbase + offset);
	/* gb_printf(KERN_DEBUG, "REG: write 0x%x value: 0x%x\n", offset, val); */
}

static inline u32 GB02FUNC25(void __iomem *regbase, u32 offset)
{
	u32 val;

	val = ioread32(regbase + offset);
	/* gb_printf(KERN_DEBUG, "REG: read 0x%x value: 0x%x\n", offset, val); */

	return val;
}

/* HDMA */
#define GB02MAC60			(0)
#define GB02MAC61				(1)
#define GB02MAC62				(2)
#define GB02MAC63				(3)
#define GB02MAC64					(0x00)
#define GB02MAC65			(1 << 0)
#define GB02MAC66			(0x04)
#define GB02MAC67		(1 << 0)
#define GB02MAC68		(1 << 1)
#define GB02MAC69			(0x08)
#define GB02MAC70			(0x0c)
#define GB02MAC71			(0x10)
#define GB02MAC72			(0x14)
#define GB02MAC73				(0x18)
#define GB02MAC74			(1 << 0)
#define GB02MAC75		(1 << 1)
#define GB02MAC76			(0x1c)
#define GB02MAC77			(0x20)
#define GB02MAC78			(0x24)
#define GB02MAC79			(0x28)
#define GB02MAC80			(0x2c)
#define GB02MAC82		(0x30)
#define GB02MAC83			(0x34)
#define GB02MAC84		(1 << 0)
#define GB02MAC86	(1 << 1)
#define GB02MAC87			(0x38)
#define GB02MAC88				(0x3c)

#define GB02MAC89				(0x80)
#define GB02MAC90		(7 << 0)
#define GB02MAC91			(1)
#define GB02MAC92			(2)
#define GB02MAC94			(3)
#define GB02MAC95			(0x84)
#define GB02MAC97		BIT(0)
#define GB02MAC99	BIT(1)
#define GB02MAC100		BIT(2)
#define GB02MAC101		(0xf << 3)
#define GB02MAC102			(0x88)
#define GB02MAC103		(1 << 0)
#define GB02MAC104	(1 << 1)
#define GB02MAC105		(1 << 2)
#define GB02MAC106				(1 << 3)
#define GB02MAC107				(1 << 4)
#define GB02MAC108				(1 << 5)
#define GB02MAC109				(1 << 6)
#define GB02MAC110			(0x8c)
#define GB02MAC111		(0x90)
#define GB02MAC112		(0x94)
#define GB02MAC113		(0x98)
#define GB02MAC114		(0x9c)
#define GB02MAC115	(0xa0)
#define GB02MAC116	(0xa4)
#define GB02MAC117			(0xa8)


/* PCIE DMA LL mode */
#define GB02MAC118		(0x0)
#define GB02MAC119	(0x4)
#define GB02MAC120		(0x8)
#define GB02MAC121	(0xc)
#define GB02MAC122		(0x10)
#define GB02MAC123	(0x14)
#define GB02MAC124		(0x8)
#define GB02MAC125	(0xc)

#define GB02MAC126			(1 << 0)
#define GB02MAC127		(1 << 1)
#define GB02MAC128		(1 << 2)
#define GB02MAC129		(1 << 3)
#define GB02MAC130		(1 << 4)

#endif
