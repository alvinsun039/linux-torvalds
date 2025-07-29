/*
 * Copyright (ST) 2012 Rajeev Kumar (rajeevkumar.linux@gmail.com)
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DESIGNWARE_LOCAL_H
#define __DESIGNWARE_LOCAL_H

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/types.h>
//#include <sound/dmaengine_pcm.h>
#include <sound/pcm.h>
#include <sound/jack.h>
#include "designware_i2s.h"


/* common register for all channel */
#define IER		0x000
#define IRER		0x004
#define ITER		0x008
#define CER		0x00C
#define CCR		0x010
#define RXFFR		0x014
#define TXFFR		0x018

#define	GB02MAC944	6

/* Interrupt status register fields */
#define GB02MAC948	BIT(5)
#define GB02MAC951	BIT(4)
#define GB02MAC954	BIT(1)
#define GB02MAC956	BIT(0)

/* I2STxRxRegisters for all channels */
#define GB02MAC961(x)	(0x40 * x + 0x020)
#define GB02MAC963(x)	(0x40 * x + 0x024)
#define RER(x)		(0x40 * x + 0x028)
#define TER(x)		(0x40 * x + 0x02C)
#define RCR(x)		(0x40 * x + 0x030)
#define TCR(x)		(0x40 * x + 0x034)
#define ISR(x)		(0x40 * x + 0x038)
#define IMR(x)		(0x40 * x + 0x03C)
#define ROR(x)		(0x40 * x + 0x040)
#define TOR(x)		(0x40 * x + 0x044)
#define RFCR(x)		(0x40 * x + 0x048)
#define TFCR(x)		(0x40 * x + 0x04C)
#define RFF(x)		(0x40 * x + 0x050)
#define TFF(x)		(0x40 * x + 0x054)

/* I2SCOMPRegisters */
#define GB02MAC975	0x01F0
#define GB02MAC977	0x01F4
#define GB02MAC979	0x01F8
#define GB02MAC981		0x01FC
//#define	GB02MAC13			0x01c8 designware_i2s.h
#define	GB02MAC983			0x200

/*
 * Component parameter register fields - define the I2S block's
 * configuration.
 */
#define	COMP1_TX_WORDSIZE_3(r)	(((r) & GENMASK(27, 25)) >> 25)
#define	COMP1_TX_WORDSIZE_2(r)	(((r) & GENMASK(24, 22)) >> 22)
#define	COMP1_TX_WORDSIZE_1(r)	(((r) & GENMASK(21, 19)) >> 19)
#define	COMP1_TX_WORDSIZE_0(r)	(((r) & GENMASK(18, 16)) >> 16)
#define	COMP1_TX_CHANNELS(r)	(((r) & GENMASK(10, 9)) >> 9)
#define	COMP1_RX_CHANNELS(r)	(((r) & GENMASK(8, 7)) >> 7)
#define	GB02MAC988(r)	(((r) & BIT(6)) >> 6)
#define	GB02MAC990(r)	(((r) & BIT(5)) >> 5)
#define	GB02MAC992(r)	(((r) & BIT(4)) >> 4)
#define	COMP1_FIFO_DEPTH_GLOBAL(r)	(((r) & GENMASK(3, 2)) >> 2)
#define	COMP1_APB_DATA_WIDTH(r)	(((r) & GENMASK(1, 0)) >> 0)

#define	COMP2_RX_WORDSIZE_3(r)	(((r) & GENMASK(12, 10)) >> 10)
#define	COMP2_RX_WORDSIZE_2(r)	(((r) & GENMASK(9, 7)) >> 7)
#define	COMP2_RX_WORDSIZE_1(r)	(((r) & GENMASK(5, 3)) >> 3)
#define	COMP2_RX_WORDSIZE_0(r)	(((r) & GENMASK(2, 0)) >> 0)

/* Number of entries in WORDSIZE and DATA_WIDTH parameter registers */
#define	GB02MAC999	(1 << 3)
#define	GB02MAC1002	(1 << 2)

#define GB02MAC1004		8
#define GB02MAC1007		2

struct GB02STR94 {
	int format;
	//int chan_num;
	int sample_rate;
	int bytes;
	int work_mode;
	int ch_count;
	int	ip_count;
	int evenflag;
};
struct GB02STR96 {
	void __iomem *i2s_base[GB02MAC944];
	unsigned int i2s_reg_comp1;
	unsigned int i2s_reg_comp2;
	struct device *dev;
	u32 ccr;
	u32 xfer_resolution;
	u32 fifo_th;
	struct GB02STR1 config[GB02MAC944];
};

int GB02FUNC46(struct GB02STR96 *dev,
		struct GB02STR94 *param_info, int ip_num);
void GB02FUNC31(struct GB02STR96 *dev,
		      int work_mode, int ip_num);
void GB02FUNC35(struct GB02STR96 *dev,
		int work_mode, int which_audio);
int GB02FUNC57(struct GB02STR96 *dev, int work_mode, int ip_num);
//irqreturn_t i2s_irq_handler(int irq, void *dev_id);

void GB02FUNC18(struct GB02STR96 *dev, u32 stream, int witch_audio);
u32 GB02FUNC11(void __iomem *io_base, int reg);

void GB02FUNC33(struct GB02STR96 *dev);

#define		GB02MAC1019			0x8
#define		GB02MAC1020			0xc
#define		GB02MAC1021		0x30
#define		GB02MAC1022		0x34
#define		GB02MAC1023		0x38
#define		GB02MAC1024		0x3c
#endif
