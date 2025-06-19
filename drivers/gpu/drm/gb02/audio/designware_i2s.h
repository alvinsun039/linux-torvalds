/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (ST) 2012 Rajeev Kumar (rajeevkumar.linux@gmail.com)
 */

#ifndef __SOUND_DESIGNWARE_I2S_H
#define __SOUND_DESIGNWARE_I2S_H

#include <linux/dmaengine.h>
#include <linux/types.h>

/*
 * struct GB02STR1 - represent i2s clk configuration data
 * @chan_nr: number of channel
 * @data_width: number of bits per sample (8/16/24/32 bit)
 * @sample_rate: sampling frequency (8Khz, 16Khz, 32Khz, 44Khz, 48Khz)
 */
struct GB02STR1 {
	int chan_nr;
	u32 data_width;
	u32 sample_rate;
};

/* I2S DMA registers */
#define GB02MAC10		0x01C0
#define GB02MAC13		0x01C8

#define GB02MAC19	2	/* up to 2.0 */
#define GB02MAC22	4	/* up to 3.1 */
#define GB02MAC23	6	/* up to 5.1 */
#define GB02MAC25	8	/* up to 7.1 */

#define	GB02MAC28		0
#define	GB02MAC30	1
#define	GB02MAC31		2
#define	GB02MAC33	3
#define	GB02MAC35		4
#define	GB02MAC37	5
#define	GB02MAC38	6

#define GB02MAC41	16
#define GB02MAC43	20
#define GB02MAC45	24
#define GB02MAC47	32

#endif /*  __SOUND_DESIGNWARE_I2S_H */
