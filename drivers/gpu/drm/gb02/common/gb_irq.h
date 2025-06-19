#ifndef __COMM_IRQ_H__
#define __COMM_IRQ_H__

#include <linux/interrupt.h>
#include "xt.h"

#define GB02MAC287
#define GB02MAC549		(GB02MAC1046 + 0x0)
#define GB02MAC550		(GB02MAC1046 + 0x4)
#define GB02MAC551	(GB02MAC1046 + 0x8)
#define GB02MAC552	(GB02MAC1046 + 0xC)
#define GB02MAC553	(GB02MAC1046 + 0x10)
#define GB02MAC554	(GB02MAC1046 + 0x14)
#define GB02MAC555	(GB02MAC1046 + 0x18)
#define GB02MAC556	(GB02MAC1046 + 0x1C)
#define GB02MAC558	(GB02MAC1046 + 0x20)
#define GB02MAC559	(GB02MAC1046 + 0x24)
#define GB02MAC560	(GB02MAC1046 + 0x28)
#define GB02MAC561	(GB02MAC1046 + 0x2C)
#define GB02MAC562		(GB02MAC1046 + 0x30)
#define GB02MAC563		(GB02MAC1046 + 0x34)

struct GB02STR70;

enum gb_irq_ip_type {
	irq_type_comp,
	irq_type_v2vdma,
	irq_type_audio,
	irq_type_dec0,
	irq_type_dec1,
	irq_type_enc,
	irq_type_dc,
	irq_type_gpu,
	irq_type_dp,
	irq_type_hdmac,
	irq_type_total,
};

enum audio_irq {
	AUDIO_IRQ,
	AUDIO_DMA_IRQ,
	AUDIO_IRQ_TOTAL,
};

enum gpu_irq {
	GPU_MMU_IRQ,
	GPU_JOB_IRQ,
	GPU_GPU_IRQ,
	GPU_EVENT_IRQ,
	GPU_IRQ_TOTAL,
};

enum dc_irq {
	DC_DE_IRQ,
	DC_SE_IRQ,
	DC_IRQ_TOTAL,
};

enum gb_irq_bit {
	dc0_irqde		= 0,
	dc0_irqse		= 1,
	dc1_irqde		= 2,
	dc1_irqse		= 3,
	dc2_irqde		= 4,
	dc2_irqse		= 5,
	dc3_irqde		= 6,
	dc3_irqse		= 7,
	dc4_irqde		= 8,
	dc4_irqse		= 9,
	dc5_irqde		= 10,
	dc5_irqse		= 11,
	dp0_intr		= 12,
	dp1_intr		= 13,
	dp2_intr		= 14,
	dp3_intr		= 15,
	dp4_intr		= 16,
	dp5_intr		= 17,
	ddr0_int_o		= 18,
	ddr1_int_o		= 19,
	ddr2_int_o		= 20,
	ddr3_int_o		= 21,
	v2vdma_intr		= 22,
	hdmac_interupt	= 23,
	audio_dma_intr	= 24,
	i2s0_intr		= 25,
	i2s1_intr		= 26,
	i2s2_intr		= 27,
	i2s3_intr		= 28,
	i2s4_intr		= 29,
	i2s5_intr		= 30,
	dec0_INTR		= 31,
	dec1_INTR		= 32,
	enc0_INTR		= 33,
	enc1_INTR		= 34,
	enc2_INTR		= 35,
	gpu_IRQMMU		= 36,
	gpu_IRQJOB		= 37,
	gpu_IRQGPU		= 38,
	gpu_IRQEVENT	= 39,
	sen_volt_int_nomal	= 40,
	sen_volt_int_fatal	= 41,
	sen_temp_int_nomal	= 42,
	sen_temp_int_fatal	= 43,
	gb_irq_bit_total,
};
/*
#define DISABLE_IRQ (GENMASK_ULL(ddr3_int_o, ddr0_int_o) |\
	GENMASK_ULL(dc5_irqse, dc0_irqde) |\
	GENMASK_ULL(sen_temp_int_fatal, enc1_INTR) |\
	GENMASK_ULL(i2s5_intr, i2s0_intr) |\
	BIT(hdmac_interupt))
*/
#define DISABLE_IRQ (GENMASK_ULL(63, gb_irq_bit_total) |\
	GENMASK_ULL(ddr3_int_o, ddr0_int_o) |\
        GENMASK_ULL(sen_temp_int_fatal, enc1_INTR) |\
        GENMASK_ULL(i2s5_intr, i2s0_intr) |\
        GENMASK_ULL(v2vdma_intr, v2vdma_intr) |\
        BIT(hdmac_interupt))
extern int GB02FUNC352(struct GB02STR70 *pcie_info);
extern int GB02FUNC364(struct GB02STR70 *pcie_info);
extern int GB02FUNC370(struct GB02STR70 *pcie_info);
extern int GB02FUNC359(struct GB02STR70 *pcie_info);
extern int GB02FUNC426(struct GB02STR70 *pcie_info);
extern void GB02FUNC423(struct GB02STR70 *pcie_info);
extern int GB02FUNC373(struct GB02STR70 *pcie_info,
	enum gb_irq_ip_type ip_type,
	int ip_irq_index);
extern irqreturn_t GB02FUNC502(int irq, void *data);
extern irqreturn_t GB02FUNC1854(int irq, void *arg);
extern irqreturn_t GB02FUNC1662(int irq, void *arg);
extern irqreturn_t GB02FUNC1511(int irq, void *dev);
extern irqreturn_t GB02FUNC1754(int irq, void *dev, int crtc_id);

extern void GB02FUNC412(void);
#endif
