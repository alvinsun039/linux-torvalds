/*
 * ALSA SoC Synopsys I2S Audio Layer
 *
 * sound/soc/dwc/designware_i2s.c
 *
 * Copyright (C) 2010 ST Microelectronics
 * Rajeev Kumar <rajeevkumar.linux@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include "audio/designware_i2s.h"
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/delay.h>
#include "local.h"
#include "gb_snd_codec.h"
#include "common/gb_common.h"

static inline void GB02FUNC9(void __iomem *io_base, int reg, u32 val)
{
	writel(val, io_base + reg);
}

u32 GB02FUNC11(void __iomem *io_base, int reg)
{
	udelay(1);
	return readl(io_base + reg);
}

static inline void GB02FUNC14(struct GB02STR96 *dev,
	u32 stream, int ip_num)
{
	u32 i = 0;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0; i < GB02MAC723; i++)
			GB02FUNC9(dev->i2s_base[ip_num], TER(i), 0);
	} else {
		for (i = 0; i < GB02MAC723; i++)
			GB02FUNC9(dev->i2s_base[ip_num], RER(i), 0);
	}
}

void GB02FUNC18(struct GB02STR96 *dev, u32 stream, int which_audio)
{
	u32 i = 0;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0; i < GB02MAC723; i++)
			GB02FUNC11(dev->i2s_base[which_audio], TOR(i));
	} else {
		for (i = 0; i < GB02MAC723; i++)
			GB02FUNC11(dev->i2s_base[which_audio], ROR(i));
	}
}

static inline void GB02FUNC23(struct GB02STR96 *dev, u32 stream,
	int chan_nr, int ip_num)
{
	u32 i, irq;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0; i < (chan_nr / 2); i++) {
			irq = GB02FUNC11(dev->i2s_base[ip_num], IMR(i));
			GB02FUNC9(dev->i2s_base[ip_num],
				IMR(i), irq | 0x30);
		}
	} else {
		for (i = 0; i < (chan_nr / 2); i++) {
			irq = GB02FUNC11(dev->i2s_base[ip_num], IMR(i));
			GB02FUNC9(dev->i2s_base[ip_num],
				IMR(i), irq | 0x03);
		}
	}
}

static inline void GB02FUNC30(struct GB02STR96 *dev, u32 stream,
	int chan_nr, int ip_num)
{
	u32 i, irq;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0; i < (chan_nr / 2); i++) {
			irq = GB02FUNC11(dev->i2s_base[ip_num], IMR(i));
			GB02FUNC9(dev->i2s_base[ip_num],
				IMR(i), irq & ~0x30);
		}
	} else {
		for (i = 0; i < (chan_nr / 2); i++) {
			irq = GB02FUNC11(dev->i2s_base[ip_num], IMR(i));
			GB02FUNC9(dev->i2s_base[ip_num],
				IMR(i), irq & ~0x03);
		}
	}
}

void GB02FUNC31(struct GB02STR96 *dev, int work_mode, int ip_num)
{
	struct GB02STR1 *config = &dev->config[ip_num];

	gb_printf(KERN_INFO, "-i2s start, ip num = %d, i2s base =0x%p\n",
		ip_num, dev->i2s_base[ip_num]);
	GB02FUNC9(dev->i2s_base[ip_num], IER, 1);

	if (work_mode == SNDRV_PCM_STREAM_PLAYBACK)
		GB02FUNC9(dev->i2s_base[ip_num], ITER, 1);
	else
		GB02FUNC9(dev->i2s_base[ip_num], IRER, 1);

	GB02FUNC9(dev->i2s_base[ip_num], GB02MAC983, 0x20000);

	GB02FUNC9(dev->i2s_base[ip_num], CER, 1);
//	GB02FUNC30(dev, work_mode, config->chan_nr, ip_num);

	gb_printf(KERN_INFO, "%s-%d: ip_num = %d, config->chan_nr = %d, finish\n",
		__func__, __LINE__, ip_num, config->chan_nr);
}

void GB02FUNC33(struct GB02STR96 *dev)
{
	GB02FUNC9(dev->i2s_base, CER, 1);
}

void GB02FUNC35(struct GB02STR96 *dev,
		int work_mode, int which_audio)
{
	gb_printf(KERN_INFO, "%s-%d: ----i2s stop ---\n", __func__, __LINE__);
	GB02FUNC18(dev, work_mode, which_audio);
	if (work_mode == SNDRV_PCM_STREAM_PLAYBACK)
		GB02FUNC9(dev->i2s_base[which_audio], ITER, 0);
	else
		GB02FUNC9(dev->i2s_base[which_audio], IRER, 0);

	GB02FUNC23(dev, work_mode, 8, which_audio);

	GB02FUNC9(dev->i2s_base[which_audio], CER, 0);
	GB02FUNC9(dev->i2s_base[which_audio], IER, 0);
}

static void GB02FUNC38(struct GB02STR96 *dev, int stream, int ip_num)
{
	u32 ch_reg, comp1, fifo_depth;
	struct GB02STR1 *config = &dev->config[ip_num];

	dev->i2s_reg_comp1 = GB02MAC977;
	udelay(100);
	comp1 = GB02FUNC11(dev->i2s_base[ip_num], dev->i2s_reg_comp1);
	fifo_depth = 1 << (1 + COMP1_FIFO_DEPTH_GLOBAL(comp1));

	dev->fifo_th = fifo_depth / 2;
	GB02FUNC14(dev, stream, ip_num);

	for (ch_reg = 0; ch_reg < (config->chan_nr / 2); ch_reg++) {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			GB02FUNC9(dev->i2s_base[ip_num], TCR(ch_reg),
				dev->xfer_resolution);
			GB02FUNC9(dev->i2s_base[ip_num], TFCR(ch_reg),
				dev->fifo_th - 3);
			GB02FUNC9(dev->i2s_base[ip_num], TER(ch_reg), 1);
			GB02FUNC9(dev->i2s_base[ip_num], TFF(ch_reg), 1);
		} else {
			GB02FUNC9(dev->i2s_base[ip_num], RCR(ch_reg),
				dev->xfer_resolution);
			GB02FUNC9(dev->i2s_base[ip_num], RFCR(ch_reg),
				dev->fifo_th - 1);
			GB02FUNC9(dev->i2s_base[ip_num], RER(ch_reg), 1);
		}
	}
}

int GB02FUNC46(struct GB02STR96 *dev,
	struct GB02STR94 *param_info, int ip_num)
{
	struct GB02STR1 *config = &dev->config[ip_num];

	switch (param_info->format) {
	case SNDRV_PCM_FORMAT_S16_LE:
		config->data_width = GB02MAC41;
		dev->ccr = 0x00;
		dev->xfer_resolution = 0x02;
		break;

	case SNDRV_PCM_FORMAT_S24_LE:
		config->data_width = GB02MAC45;
		dev->ccr = 0x08;
		dev->xfer_resolution = 0x04;
		break;

	case SNDRV_PCM_FORMAT_S32_LE:
		config->data_width = GB02MAC47;
		dev->ccr = 0x10;
		dev->xfer_resolution = 0x05;
		break;

	default:
		dev_err(dev->dev, "designware-i2s: unsupported PCM fmt");
		return -EINVAL;
	}
	gb_printf(KERN_INFO, "%s-%d: ip num = %d, data width = %d\n",
		__func__, __LINE__, ip_num, config->data_width);
	config->chan_nr = param_info->ch_count;

	switch (config->chan_nr) {
	case GB02MAC25:
	case GB02MAC23:
	case GB02MAC22:
	case GB02MAC19:
		break;
	default:
		dev_err(dev->dev, "channel not supported\n");
		return -EINVAL;
	}

	GB02FUNC38(dev, param_info->work_mode, ip_num);

	GB02FUNC9(dev->i2s_base[ip_num], CCR, dev->ccr);
	return 0;
}

int GB02FUNC57(struct GB02STR96 *dev, int work_mode, int ip_num)
{

	if (work_mode == SNDRV_PCM_STREAM_PLAYBACK)
		GB02FUNC9(dev->i2s_base[ip_num], TXFFR, 1);
	else
		GB02FUNC9(dev->i2s_base[ip_num], RXFFR, 1);

	return 0;
}

static const struct snd_soc_component_driver dw_i2s_component = {
	.name		= "dw-i2s",
};

#define dw_i2s_suspend	NULL
#define dw_i2s_resume	NULL

/*
 * The following tables allow a direct lookup of various parameters
 * defined in the I2S block's configuration in terms of sound system
 * parameters.  Each table is sized to the number of entries possible
 * according to the number of configuration bits describing an I2S
 * block parameter.
 */

/* Maximum bit resolution of a channel - not uniformly spaced */
static const u32 fifo_width[GB02MAC999] = {
	12, 16, 20, 24, 32, 0, 0, 0
};

/* Width of (DMA) bus */
static const u32 bus_widths[GB02MAC1002] = {
	DMA_SLAVE_BUSWIDTH_1_BYTE,
	DMA_SLAVE_BUSWIDTH_2_BYTES,
	DMA_SLAVE_BUSWIDTH_4_BYTES,
	DMA_SLAVE_BUSWIDTH_UNDEFINED
};

/* PCM format to support channel resolution */
static const u32 formats[GB02MAC999] = {
	SNDRV_PCM_FMTBIT_S16_LE,
	SNDRV_PCM_FMTBIT_S16_LE,
	SNDRV_PCM_FMTBIT_S24_LE,
	SNDRV_PCM_FMTBIT_S24_LE,
	SNDRV_PCM_FMTBIT_S32_LE,
	0,
	0,
	0
};

#define		GB02MAC157		0xC00
void GB02FUNC71(struct GB02STR96 *dev,
	struct GB02STR94 *parm_info)
{
	int i = 0;
	void __iomem *tbase = dev->i2s_base + GB02MAC157;

	GB02FUNC9(tbase, GB02MAC1019, 7);
	GB02FUNC9(tbase, GB02MAC1020, 1);
	for (i = 0; i < parm_info->ch_count / 2; i++)
		GB02FUNC9(tbase, GB02MAC1021 + i * 4, 1);

	for (i = 0; i < 4; i++)
		GB02FUNC9(tbase, GB02MAC1021 + i * 4, 0);
}
