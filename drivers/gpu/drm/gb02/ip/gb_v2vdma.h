/* Copyright (C) Xi'an Sietium Electronics Co.Ltd */
/*
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
#include "common/gb_uk.h"

#ifndef __GB02_FPGA_V2VDMA_H
#define __GB02_FPGA_V2VDMA_H

#define GB02MAC309	(4)
#define GB02MAC310 (1)
#define GB02MAC312 (5) /* DWAXIDMAC_TRANS_WIDTH_256 */
#define GB02MAC314 (255) /* DWAXIDMAC_ARWLEN_256 */
#define GB02MAC316 (0) /* channel 0 */
#define GB02MAC318 (1)

#define GB02MAC320	(0x1000) /* 4KB */
#define GB02MAC2822	(0x100000)

#define GB02MAC2823 (0) /* VRAM ADDR OFFSET view at V2VDMA side */
#define GB02MAC2824	(~0ULL)

#define GB02MAC2825 0x20
#define GB02MAC2826 0x3FFFFF
#define GB02MAC2827 ((GB02MAC2826 + 1) << DWAXIDMAC_TRANS_WIDTH_256)

typedef enum {
	DMA_LL_MODE = 0,
	DMA_SHADOW_MODE,
	DMA_SINGLE_BLK_MODE,
	MULTI_CHNL_DMA_MODE_MAX
} multi_chnl_dma_mode_e;

struct GB02STR237 {
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned int size;
};

struct GB02STR38 {
	struct GB02STR40     *chan;
	struct list_head        xfer_list;
	u64 phys;
};

struct GB02STR40 {
	void __iomem *chan_regs;
	atomic_t stop;
	u8 id;
    struct GB02STR238 *chip;
	atomic_t descs_allocated;
	struct GB02STR38 *first_desc;
    u8 shadow_blk_cnt;
};

struct GB02STR238 {
	struct pci_dev *pdev;
	void __iomem *vram_addr;
	void __iomem *regs;
	struct GB02STR40 chan[GB02MAC309];
	unsigned __iomem *dma_src_addr[GB02MAC309];
	unsigned __iomem *dma_dst_addr[GB02MAC309];
	struct task_struct *thread[GB02MAC309];
	u64 multi_chnl_test_cycle[GB02MAC309];
	dma_addr_t dma_src_handle;
	dma_addr_t dma_dst_handle;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timeval dma_start_tv;
	struct timeval dma_end_tv;
#else
	struct timespec64 dma_start_tv;
	struct timespec64 dma_end_tv;
#endif
	unsigned int irq_cnt;
    struct workqueue_struct *queue;
	int irq;
};

int gb02_dma_single_blk_mode_trans_start(u32, u32, u32);

static inline u32 GB02FUNC170(struct GB02STR238 *chip,
	u32 offset)
{
	u32 val = 0;

	val = ioread32(chip->regs + offset);
	/* gb_printf(KERN_DEBUG, "REG: read 0x%x value: 0x%x\n", offset, val); */

	return val;
}

static inline void GB02FUNC173(struct GB02STR238 *chip,
	u32 offset, u32 val)
{
	iowrite32(val, chip->regs + offset);
	/* gb_printf(KERN_DEBUG, "REG: write 0x%x value: 0x%x\n", offset, val); */
}

static inline u32 GB02FUNC13(struct GB02STR40 *chan,
	u32 offset)
{
	u32 val = 0;

	val = ioread32(chan->chan_regs + offset);
	/* gb_printf(KERN_DEBUG, "CHAN REG: read 0x%x value: 0x%x\n", offset, val); */

	return val;
}

static inline void GB02FUNC15(struct GB02STR40 *chan,
	u32 offset, u32 val)
{
	iowrite32(val, chan->chan_regs + offset);

	/* gb_printf(KERN_DEBUG, "CHAN REG: write 0x%x value: 0x%x\n", offset, val); */
}

static inline void GB02FUNC17(struct GB02STR40 *chan,
	u32 offset, u64 val)
{
	iowrite32(lower_32_bits(val), chan->chan_regs + offset);
	iowrite32(upper_32_bits(val), chan->chan_regs + offset + 4);

	/* gb_printf(KERN_DEBUG, "CHAN REG: write 0x%x value: 0x%llx\n", offset, val); */
}


#define GB02MAC32		0x100
#define GB02MAC34		0x100

/* Common registers offset */
#define GB02MAC334			0x000 /* R DMAC ID */
#define GB02MAC336		0x008 /* R DMAC Component Version */
#define GB02MAC338		0x010 /* R/W DMAC Configuration */
#define GB02MAC340		0x018 /* R/W DMAC Channel Enable */
#define GB02MAC342		0x018 /* R/W DMAC Channel Enable 00-31 */
#define GB02MAC343		0x01C /* R/W DMAC Channel Enable 32-63 */
#define GB02MAC345		0x030 /* R DMAC Interrupt Status */
#define GB02MAC347	0x038 /* W DMAC Interrupt Clear */
#define GB02MAC349 0x040 /* R DMAC Interrupt Status Enable */
#define GB02MAC351 0x048 /* R/W DMAC Interrupt Signal Enable */
#define GB02MAC353	0x050 /* R DMAC Interrupt Status */
#define GB02MAC355		0x058 /* R DMAC Reset Register1 */

/* DMA channel registers offset */
#define GB02MAC358			0x000 /* R/W Chan Source Address */
#define GB02MAC360			0x008 /* R/W Chan Destination Address */
#define GB02MAC362		0x010 /* R/W Chan Block Transfer Size */
#define GB02MAC364			0x018 /* R/W Chan Control */
#define GB02MAC366		0x018 /* R/W Chan Control 00-31 */
#define GB02MAC368		0x01C /* R/W Chan Control 32-63 */
#define GB02MAC370			0x020 /* R/W Chan Configuration */
#define GB02MAC372		0x020 /* R/W Chan Configuration 00-31 */
#define GB02MAC374		0x024 /* R/W Chan Configuration 32-63 */
#define GB02MAC375			0x028 /* R/W Chan Linked List Pointer */
#define GB02MAC377		0x030 /* R Chan Status */
#define GB02MAC380		0x038 /* R/W Chan SW Handshake Source */
#define GB02MAC382		0x040 /* R/W Chan SW Handshake Destination */
#define GB02MAC385	0x048 /* W Chan Block Transfer Resume Req */
#define GB02MAC388		0x050 /* R/W Chan AXI ID */
#define GB02MAC391		0x058 /* R/W Chan AXI QOS */
#define GB02MAC394		0x060 /* R Chan Source Status */
#define GB02MAC397		0x068 /* R Chan Destination Status */
#define GB02MAC400		0x070 /* R/W Chan Source Status Fetch Addr */
#define GB02MAC402		0x078 /* R/W Chan Destination Status Fetch Addr */
#define GB02MAC404	0x080 /* R/W Chan Interrupt Status Enable */
#define GB02MAC406		0x088 /* R/W Chan Interrupt Status */
#define GB02MAC408	0x090 /* R/W Chan Interrupt Signal Enable */
#define GB02MAC410		0x098 /* W Chan Interrupt Clear */


/* GB02MAC338 */
#define GB02MAC414			0
#define GB02MAC415			BIT(GB02MAC414)

#define GB02MAC418			1
#define GB02MAC420			BIT(GB02MAC418)

#define GB02MAC424		0
#define GB02MAC427		8

#define GB02MAC429		16
#define GB02MAC431		24

/* GB02MAC368 */
#define GB02MAC434		BIT(6)
#define GB02MAC436		7
#define GB02MAC438		BIT(15)
#define GB02MAC439		16

enum {
	DWAXIDMAC_ARWLEN_1		= 0,
	DWAXIDMAC_ARWLEN_2		= 1,
	DWAXIDMAC_ARWLEN_4		= 3,
	DWAXIDMAC_ARWLEN_8		= 7,
	DWAXIDMAC_ARWLEN_16		= 15,
	DWAXIDMAC_ARWLEN_32		= 31,
	DWAXIDMAC_ARWLEN_64		= 63,
	DWAXIDMAC_ARWLEN_128		= 127,
	DWAXIDMAC_ARWLEN_256		= 255,
	DWAXIDMAC_ARWLEN_MIN		= DWAXIDMAC_ARWLEN_1,
	DWAXIDMAC_ARWLEN_MAX		= DWAXIDMAC_ARWLEN_256
};

#define GB02MAC446		BIT(30)
#define GB02MAC449		BIT(31)

/* GB02MAC366 */
#define GB02MAC452		BIT(30)

#define GB02MAC455		18
#define GB02MAC457		14

enum {
	DWAXIDMAC_BURST_TRANS_LEN_1	= 0,
	DWAXIDMAC_BURST_TRANS_LEN_4,
	DWAXIDMAC_BURST_TRANS_LEN_8,
	DWAXIDMAC_BURST_TRANS_LEN_16,
	DWAXIDMAC_BURST_TRANS_LEN_32,
	DWAXIDMAC_BURST_TRANS_LEN_64,
	DWAXIDMAC_BURST_TRANS_LEN_128,
	DWAXIDMAC_BURST_TRANS_LEN_256,
	DWAXIDMAC_BURST_TRANS_LEN_512,
	DWAXIDMAC_BURST_TRANS_LEN_1024
};

#define GB02MAC467		11
#define GB02MAC468		8

#define GB02MAC471		6
#define GB02MAC472		4
enum {
	DWAXIDMAC_CH_CTL_L_INC		= 0,
	DWAXIDMAC_CH_CTL_L_NOINC
};

#define GB02MAC473		BIT(2)
#define GB02MAC474		BIT(0)

/* GB02MAC374 */
#define GB02MAC475		17
#define GB02MAC476		4
#define GB02MAC477		3
enum {
	DWAXIDMAC_HS_SEL_HW		= 0,
	DWAXIDMAC_HS_SEL_SW
};

#define GB02MAC478		0
enum {
	DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC	= 0,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_SRC,
	DWAXIDMAC_TT_FC_PER_TO_PER_SRC,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DST,
	DWAXIDMAC_TT_FC_PER_TO_PER_DST
};

/* GB02MAC372 */
#define GB02MAC479	2
#define GB02MAC480	0
enum {
	DWAXIDMAC_MBLK_TYPE_CONTIGUOUS	= 0,
	DWAXIDMAC_MBLK_TYPE_RELOAD,
	DWAXIDMAC_MBLK_TYPE_SHADOW_REG,
	DWAXIDMAC_MBLK_TYPE_LL
};

/**
 * DW AXI DMA channel interrupts
 *
 * @DWAXIDMAC_IRQ_NONE: Bitmask of no one interrupt
 * @DWAXIDMAC_IRQ_BLOCK_TRF: Block transfer complete
 * @DWAXIDMAC_IRQ_DMA_TRF: Dma transfer complete
 * @DWAXIDMAC_IRQ_SRC_TRAN: Source transaction complete
 * @DWAXIDMAC_IRQ_DST_TRAN: Destination transaction complete
 * @DWAXIDMAC_IRQ_SRC_DEC_ERR: Source decode error
 * @DWAXIDMAC_IRQ_DST_DEC_ERR: Destination decode error
 * @DWAXIDMAC_IRQ_SRC_SLV_ERR: Source slave error
 * @DWAXIDMAC_IRQ_DST_SLV_ERR: Destination slave error
 * @DWAXIDMAC_IRQ_LLI_RD_DEC_ERR: LLI read decode error
 * @DWAXIDMAC_IRQ_LLI_WR_DEC_ERR: LLI write decode error
 * @DWAXIDMAC_IRQ_LLI_RD_SLV_ERR: LLI read slave error
 * @DWAXIDMAC_IRQ_LLI_WR_SLV_ERR: LLI write slave error
 * @DWAXIDMAC_IRQ_INVALID_ERR: LLI invalid error or Shadow register error
 * @DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR: Slave Interface Multiblock type error
 * @DWAXIDMAC_IRQ_DEC_ERR: Slave Interface decode error
 * @DWAXIDMAC_IRQ_WR2RO_ERR: Slave Interface write to read only error
 * @DWAXIDMAC_IRQ_RD2RWO_ERR: Slave Interface read to write only error
 * @DWAXIDMAC_IRQ_WRONCHEN_ERR: Slave Interface write to channel error
 * @DWAXIDMAC_IRQ_SHADOWREG_ERR: Slave Interface shadow reg error
 * @DWAXIDMAC_IRQ_WRONHOLD_ERR: Slave Interface hold error
 * @DWAXIDMAC_IRQ_LOCK_CLEARED: Lock Cleared Status
 * @DWAXIDMAC_IRQ_SRC_SUSPENDED: Source Suspended Status
 * @DWAXIDMAC_IRQ_SUSPENDED: Channel Suspended Status
 * @DWAXIDMAC_IRQ_DISABLED: Channel Disabled Status
 * @DWAXIDMAC_IRQ_ABORTED: Channel Aborted Status
 * @DWAXIDMAC_IRQ_ALL_ERR: Bitmask of all error interrupts
 * @DWAXIDMAC_IRQ_ALL: Bitmask of all interrupts
 */
enum {
	DWAXIDMAC_IRQ_NONE		= 0,
	DWAXIDMAC_IRQ_BLOCK_TRF		= BIT(0),
	DWAXIDMAC_IRQ_DMA_TRF		= BIT(1),
	DWAXIDMAC_IRQ_SRC_TRAN		= BIT(3),
	DWAXIDMAC_IRQ_DST_TRAN		= BIT(4),
	DWAXIDMAC_IRQ_SRC_DEC_ERR	= BIT(5),
	DWAXIDMAC_IRQ_DST_DEC_ERR	= BIT(6),
	DWAXIDMAC_IRQ_SRC_SLV_ERR	= BIT(7),
	DWAXIDMAC_IRQ_DST_SLV_ERR	= BIT(8),
	DWAXIDMAC_IRQ_LLI_RD_DEC_ERR	= BIT(9),
	DWAXIDMAC_IRQ_LLI_WR_DEC_ERR	= BIT(10),
	DWAXIDMAC_IRQ_LLI_RD_SLV_ERR	= BIT(11),
	DWAXIDMAC_IRQ_LLI_WR_SLV_ERR	= BIT(12),
	DWAXIDMAC_IRQ_INVALID_ERR	= BIT(13),
	DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR	= BIT(14),
	DWAXIDMAC_IRQ_DEC_ERR		= BIT(16),
	DWAXIDMAC_IRQ_WR2RO_ERR		= BIT(17),
	DWAXIDMAC_IRQ_RD2RWO_ERR	= BIT(18),
	DWAXIDMAC_IRQ_WRONCHEN_ERR	= BIT(19),
	DWAXIDMAC_IRQ_SHADOWREG_ERR	= BIT(20),
	DWAXIDMAC_IRQ_WRONHOLD_ERR	= BIT(21),
	DWAXIDMAC_IRQ_LOCK_CLEARED	= BIT(27),
	DWAXIDMAC_IRQ_SRC_SUSPENDED	= BIT(28),
	DWAXIDMAC_IRQ_SUSPENDED		= BIT(29),
	DWAXIDMAC_IRQ_DISABLED		= BIT(30),
	DWAXIDMAC_IRQ_ABORTED		= BIT(31),
	DWAXIDMAC_IRQ_ALL_ERR		= (GENMASK(21, 16) | GENMASK(14, 5)),
	DWAXIDMAC_IRQ_ALL		= GENMASK(31, 0)
};

enum {
	DWAXIDMAC_TRANS_WIDTH_8		= 0,
	DWAXIDMAC_TRANS_WIDTH_16,
	DWAXIDMAC_TRANS_WIDTH_32,
	DWAXIDMAC_TRANS_WIDTH_64,
	DWAXIDMAC_TRANS_WIDTH_128,
	DWAXIDMAC_TRANS_WIDTH_256,
	DWAXIDMAC_TRANS_WIDTH_512,
	DWAXIDMAC_TRANS_WIDTH_MAX	= DWAXIDMAC_TRANS_WIDTH_512
};
int GB02FUNC1856(struct GB02STR70 *pcie_info);
void GB02FUNC1857(struct GB02STR70 *pcie_info);
int GB02FUNC1855(phys_addr_t src_addr,
	phys_addr_t dst_addr, uint32_t size);

#endif
