#include <linux/types.h>
/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <drm/ttm/ttm_tt.h>
#include <sound/core.h>
#include <sound/asoundef.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <sound/jack.h>
#include <sound/pcm_iec958.h>
#include <sound/pcm_drm_eld.h>

#include "mwv207d_audio.h"
#include "mwv207d_drv.h"
#include "mwv207d_bo.h"
#include "dw-hdmi.h"

#define DRIVER_NAME	"mwv207d"

#define MWV207D_AUDIO_BASE(idx)     (0x400000 + 0x40000 * (idx))
#define MWV207D_CTRL_BASE(idx)      (0x2d0000 + 0x400 + 0x100 * (idx))

#define for_each_audio(_card, _audio_id) \
	for ((_audio_id) = 0; (_audio_id) < (_card)->chanel_nr; (_audio_id)++)

static int enable_audio = 1;
module_param(enable_audio, int, 0644);

static int audio_card_index = SNDRV_DEFAULT_IDX1;
static char *audio_card_id = SNDRV_DEFAULT_STR1;
module_param_named(index, audio_card_index, int, 0444);
MODULE_PARM_DESC(index, "Index value for Jingjia Micro Audio controller.");
module_param_named(id, audio_card_id, charp, 0444);
MODULE_PARM_DESC(id, "ID string for Jingjia Micro Audio controller.");

struct mwv207d_pcm_stream_info {
	struct snd_pcm_substream *substream;
	int substream_refcount;
};

struct mwv207d_audio_channel {
	struct mwv207d_audio_card *mcard;
	struct mwv207d_pcm_stream_info stream_info;
	bool running;
	bool enabled;
	bool has_jack;

	void __iomem *mmio;
	void __iomem *ctrl_mmio;

	uint8_t eld[0x80];
	u32 clock;

	spinlock_t lock;
	struct mutex mutex;

	u32 buf_offset;
	u32 buf_period;
	u32 buf_size;
	u32 channels;
	dma_addr_t dma_addr;

	struct snd_pcm *pcm;
	struct snd_jack *jack;
	struct mwv207d_device *mdev;
	struct mwv207d_bo *bo;

	struct hrtimer fake_dma;
	ktime_t period_time;
	int use_fake_dma;

	struct delayed_work jack_work;
	bool hw_connected;
	int  idx;
};

struct mwv207d_audio_card {
	struct snd_card *card;
	struct device *dev;
	struct mwv207d_audio_channel audio[0x4];
	int    chanel_nr;
};

static inline u8 mwv207d_audio_readb(struct mwv207d_audio_channel *audio, u32 reg)
{
	return readl(audio->mmio + reg * 4);
}

static inline void mwv207d_audio_writeb(struct mwv207d_audio_channel *audio,
		u8 value, u32 reg)
{
	writel_relaxed(value, audio->mmio + reg * 4);
}

static inline u8 mwv207d_ctrl_read(struct mwv207d_audio_channel *audio, u32 reg)
{
	return readl(audio->ctrl_mmio + reg);
}

static inline void mwv207d_ctrl_write(struct mwv207d_audio_channel *audio,
		u32 value, u32 reg)
{
	writel(value, audio->ctrl_mmio + reg);
}

struct channel_config {
	u8 conf1;
	u8 ca;
};

static struct channel_config default_hdmi_channel_config[7] = {
	{ 0x03, 0x00 },
	{ 0x0b, 0x02 },
	{ 0x33, 0x08 },
	{ 0x37, 0x09 },
	{ 0x3f, 0x0b },
	{ 0x7f, 0x0f },
	{ 0xff, 0x13 },
};

enum video_if_reg {
	VIDEO_IF_CTRL = 0x00,
	VIDEO_IF_TMDS_CTRL = 0x04,
	VIDEO_IF_DMA_HADDR_L = 0x08,
	VIDEO_IF_DMA_HADDR_H = 0x0C
};

static void mwv207d_audio_hdmi_set_sample_rate(struct mwv207d_audio_channel *audio, u32 rate)
{
	u32 val;

	mwv207d_audio_writeb(audio, 0x00, 0x1065);
	mwv207d_audio_writeb(audio, 0x00, 0x1066);
	mwv207d_audio_writeb(audio, 0x00, 0x1067);
	mwv207d_audio_writeb(audio, 0x00, 0x1068);
	mwv207d_audio_writeb(audio, 0x02, 0x1069);
	mwv207d_audio_writeb(audio, 0x02, 0x106A);
	mwv207d_audio_writeb(audio, 0x00, 0x106B);
	mwv207d_audio_writeb(audio, 0x01, 0x106C);
	mwv207d_audio_writeb(audio, 0x00, 0x106D);

	val = (rate == 0xBB80) ? 0xc2 : 0xc0;
	mwv207d_audio_writeb(audio, val, 0x106E);

	val = (rate == 0xBB80) ? 0xd5 : 0xf5;
	mwv207d_audio_writeb(audio, val, 0x106F);
}

static inline bool mwv207d_audio_hdmi_is_enabled(struct mwv207d_audio_channel *audio)
{
	u32 state;

	state = mwv207d_ctrl_read(audio, VIDEO_IF_CTRL);

	return !!(state & 0x1);
}

static void mwv207d_audio_setup_outbound(struct mwv207d_audio_channel *audio)
{
	mwv207d_ctrl_write(audio, (u32)(0x1000000000ULL & 0xffffffff),
			VIDEO_IF_DMA_HADDR_L);
	mwv207d_ctrl_write(audio, (u32)(0x1000000000ULL >> 32),
			VIDEO_IF_DMA_HADDR_H);
}

static void mwv207d_audio_hdmi_dma_init(struct mwv207d_audio_channel *audio)
{
	mwv207d_audio_writeb(audio, HDMI_AHB_DMA_STOP_STOP_MASK, 0x3602);

	mwv207d_audio_setup_outbound(audio);

	mwv207d_audio_writeb(audio, HDMI_IH_AHBDMAAUD_STAT0_MASK,
			0x109);

	mwv207d_audio_writeb(audio, HDMI_IH_MUTE_AHBDMAAUD_STAT0_MASK &
			~HDMI_IH_MUTE_AHBDMAAUD_STAT0_DONE,
			0x189);
}

static void mwv207d_audio_hdmi_close(struct mwv207d_audio_channel *audio)
{

	mwv207d_audio_writeb(audio, HDMI_IH_MUTE_AHBDMAAUD_STAT0_MASK,
			   0x189);

	mwv207d_audio_writeb(audio, HDMI_AHB_DMA_MASK_MASK, 0x3614);

	mwv207d_audio_writeb(audio, HDMI_IH_AHBDMAAUD_STAT0_MASK,
			   0x109);
}

static void mwv207d_audio_hdmi_setup_dma_addr(struct mwv207d_audio_channel *audio,
					u32 start, u32 stop)
{
	mwv207d_audio_writeb(audio, (start & 0xff), 0x3604);
	mwv207d_audio_writeb(audio, ((start >> 8) & 0xff), 0x3605);
	mwv207d_audio_writeb(audio, ((start >> 16) & 0xff), 0x3606);
	mwv207d_audio_writeb(audio, ((start >> 24) & 0xff), 0x3607);

	mwv207d_audio_writeb(audio, (stop & 0xff), 0x3608);
	mwv207d_audio_writeb(audio, ((stop >> 8) & 0xff), 0x3609);
	mwv207d_audio_writeb(audio, ((stop >> 16) & 0xff), 0x360A);
	mwv207d_audio_writeb(audio, ((stop >> 24) & 0xff), 0x360B);
}

static void mwv207d_audio_hdmi_start_dma_xfer(struct mwv207d_audio_channel *audio)
{

	mwv207d_audio_writeb(audio, (u8)(~HDMI_AHB_DMA_DONE), 0x3614);

	mwv207d_audio_writeb(audio, HDMI_AHB_DMA_START_START_MASK, 0x3601);
}

static void mwv207d_audio_hdmi_start_dma(struct mwv207d_audio_channel *audio)
{
	u32 start, stop, period, offset;

	offset = audio->buf_offset;
	period = audio->buf_period;

	start = audio->dma_addr + offset;
	stop = start + period - 1;

	mwv207d_audio_hdmi_setup_dma_addr(audio, start, stop);
	mwv207d_audio_hdmi_start_dma_xfer(audio);

	offset += period;
	if (offset >= audio->buf_size)
		offset = 0;
	audio->buf_offset = offset;
}

static void mwv207d_audio_hdmi_stop_dma(struct mwv207d_audio_channel *audio)
{

	mwv207d_audio_writeb(audio, (u8)~0U, 0x3614);

	mwv207d_audio_writeb(audio, HDMI_AHB_DMA_STOP_STOP_MASK, 0x3602);
}

static int mwv207d_audio_hdmi_calculate_n(u32 rate, u32 *n)
{
	int n_val, ret = 0;

	switch (rate) {
	case 0x7D00:
		n_val = 4096;
		break;
	case 0xAC44:
		n_val = 6272;
		break;
	case 0xBB80:
		n_val = 6144;
		break;
	case 0x15888:
		n_val = 12544;
		break;
	case 0x17700:
		n_val = 12288;
		break;
	case 0x2B110:
		n_val = 25088;
		break;
	case 0x2EE00:
		n_val = 24576;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	*n = n_val;

	return ret;
}

static void mwv207d_audio_hdmi_set_cts_n(struct mwv207d_audio_channel *audio, u32 cts, u32 n)
{
	u32 value;

	value = mwv207d_audio_readb(audio, 0x3205);
	value &= (u32) (~HDMI_AUD_CTS3_CTS_MANUAL);
	mwv207d_audio_writeb(audio, value, 0x3205);

	value = mwv207d_audio_readb(audio, 0x3205);
	value &= (u32) (~HDMI_AUD_CTS3_N_SHIFT_MASK);
	mwv207d_audio_writeb(audio, value, 0x3205);

	mwv207d_audio_writeb(audio, ((n >> 16) & 0xff) | 0x80, 0x3202);
	mwv207d_audio_writeb(audio, ((cts >> 16) &
			   HDMI_AUD_CTS3_AUDCTS19_16_MASK) |
			   HDMI_AUD_CTS3_CTS_MANUAL,
			   0x3205);
	mwv207d_audio_writeb(audio, (cts >> 8) & 0xff, 0x3204);
	mwv207d_audio_writeb(audio, cts & 0xff, 0x3203);
	mwv207d_audio_writeb(audio, ((n >> 16) & 0xff) | 0x80, 0x3202);
	mwv207d_audio_writeb(audio, (n >> 8) & 0xff, 0x3201);
	mwv207d_audio_writeb(audio, n & 0xff, 0x3200);
}

static void mwv207d_audio_hdmi_calculate_cts(struct mwv207d_audio_channel *audio, u32 rate,
			    u32 clock, u32 *cts, u32 n_val)
{
	u32 cts_val;
	u64 dividend, divisor;
	u32 config3 = 0;

	dividend = (u64) clock * n_val * 1000;
	divisor = 128 * rate;
	cts_val = div64_u64(dividend, divisor);

	config3 = mwv207d_audio_readb(audio, 0x7);

	if (!(config3 & HDMI_CONFIG3_AHBAUDDMA))
		cts_val = 0;

	*cts = cts_val;
}

static int mwv207d_audio_hdmi_calculate_cts_n(struct mwv207d_audio_channel *audio, u32 rate,
						u32 *cts, u32 *n)
{
	int ret;

	ret = mwv207d_audio_hdmi_calculate_n(rate, n);
	if (ret)
		return ret;

	mwv207d_audio_hdmi_calculate_cts(audio, rate, audio->clock, cts, *n);

	return 0;
}

static void mwv207d_audio_hdmi_dip_config(struct mwv207d_audio_channel *audio, u32 channels)
{
	u8 ca, conf0, conf1, layout;
	u32 value = 0;

	conf0 = HDMI_AHB_DMA_CONF0_SW_FIFO_RST |
		HDMI_AHB_DMA_CONF0_INSERT_PCUV |
		HDMI_AHB_DMA_CONF0_INCR8 | HDMI_AHB_DMA_CONF0_BURST_MODE;

	if (channels <= 2) {
		ca = 0x00;
		layout = HDMI_FC_AUDSCONF_AUD_PACKET_LAYOUT_LAYOUT0;
		conf1 = 0x03;
	} else {
		ca = default_hdmi_channel_config[channels - 2].ca;
		layout = HDMI_FC_AUDSCONF_AUD_PACKET_LAYOUT_LAYOUT1;
		conf1 = default_hdmi_channel_config[channels - 2].conf1;
	}

	mwv207d_audio_writeb(audio, 0x40, 0x3603);
	mwv207d_audio_writeb(audio, conf0, 0x3600);
	mwv207d_audio_writeb(audio, conf1, 0x3616);

	value = mwv207d_audio_readb(audio, 0x1063);
	value &= (u32) (~HDMI_FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK);
	value |= (layout & HDMI_FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK);
	mwv207d_audio_writeb(audio, value, 0x1063);

	value = mwv207d_audio_readb(audio, 0x1025);
	value &= (u32) (~HDMI_FC_AUDICONF0_CC_MASK);
	value |= (((channels - 1) << HDMI_FC_AUDICONF0_CC_OFFSET) &
		 HDMI_FC_AUDICONF0_CC_MASK);
	mwv207d_audio_writeb(audio, value, 0x1025);

	mwv207d_audio_writeb(audio, ca, 0x1027);
}

static int mwv207d_audio_hdmi_prepare(struct mwv207d_audio_channel *audio, u32 rate)
{
	u32 cts, n;
	int ret;

	mwv207d_audio_hdmi_dip_config(audio, audio->channels);

	ret = mwv207d_audio_hdmi_calculate_cts_n(audio, rate, &cts, &n);
	if (ret)
		return ret;

	mwv207d_audio_hdmi_set_cts_n(audio, cts, n);

	mwv207d_audio_hdmi_set_sample_rate(audio, rate);

	return 0;
}

static struct snd_pcm_substream *mwv207d_audio_substream_get(
		struct mwv207d_audio_channel *audio)
{
	struct snd_pcm_substream *substream;
	unsigned long flags;

	spin_lock_irqsave(&audio->lock, flags);
	substream = audio->stream_info.substream;
	if (substream)
		audio->stream_info.substream_refcount++;
	spin_unlock_irqrestore(&audio->lock, flags);

	return substream;
}

static void mwv207d_audio_substream_put(struct mwv207d_audio_channel *audio)
{
	unsigned long flags;

	spin_lock_irqsave(&audio->lock, flags);
	audio->stream_info.substream_refcount--;
	spin_unlock_irqrestore(&audio->lock, flags);
}

static const struct snd_pcm_hardware mwv207d_pcm_hardware = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID),
	.formats = SNDRV_PCM_FMTBIT_S24_LE,
	.rates = (SNDRV_PCM_RATE_32000 |
		  SNDRV_PCM_RATE_44100 |
		  SNDRV_PCM_RATE_48000 |
		  SNDRV_PCM_RATE_88200 |
		  SNDRV_PCM_RATE_96000 |
		  SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000),
	.channels_min = 0x2,
	.channels_max = 0x8,
	.rate_min = 0x7D00,
	.rate_max = 0x2EE00,
	.buffer_bytes_max = (1024*1024),
	.period_bytes_min = 0x100,
	.period_bytes_max = 0x2000,
	.periods_min = 0x2,
	.periods_max = 0x10,
	.fifo_size = 0x0,
};

static int mwv207d_pcm_open(struct snd_pcm_substream *substream)
{
	struct mwv207d_audio_channel *audio;
	struct snd_pcm_runtime *runtime;
	unsigned long flags;
	int ret;

	audio = snd_pcm_substream_chip(substream);
	runtime = substream->runtime;
	runtime->hw = mwv207d_pcm_hardware;

	ret = snd_pcm_limit_hw_rates(runtime);
	if (ret < 0)
		return ret;

	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		return ret;

	ret = snd_pcm_hw_constraint_minmax(runtime,
					SNDRV_PCM_HW_PARAM_BUFFER_SIZE,
					0, substream->dma_buffer.bytes);
	if (ret < 0)
		return ret;

	mwv207d_audio_hdmi_dma_init(audio);

	spin_lock_irqsave(&audio->lock, flags);
	audio->stream_info.substream = substream;
	audio->stream_info.substream_refcount++;
	spin_unlock_irqrestore(&audio->lock, flags);

	return 0;
}

static int mwv207d_pcm_close(struct snd_pcm_substream *substream)
{
	struct mwv207d_audio_channel *audio;
	unsigned long flags;

	audio = snd_pcm_substream_chip(substream);

	spin_lock_irqsave(&audio->lock, flags);
	audio->stream_info.substream_refcount--;
	while (audio->stream_info.substream_refcount > 0) {
		spin_unlock_irqrestore(&audio->lock, flags);
		cpu_relax();
		spin_lock_irqsave(&audio->lock, flags);
	}
	audio->stream_info.substream = NULL;
	spin_unlock_irqrestore(&audio->lock, flags);

	mwv207d_audio_hdmi_close(audio);

	hrtimer_cancel(&audio->fake_dma);

	return 0;
}

static int mwv207d_audio_buffer_new(struct mwv207d_audio_channel *audio,
				    int size)
{
	struct snd_pcm_substream *substream = audio->pcm->streams[0].substream;
	struct mwv207d_bo *mbo;
	u64 gpu_addr;
	void *vaddr;
	int ret;

	ret = mwv207d_bo_create_pin_mapped(audio->mdev, size, SZ_4K,
			0x4, 0, &mbo,
			&gpu_addr, &vaddr);
	if (ret)
		return ret;

	audio->bo = mbo;

	substream->dma_buffer.addr = gpu_addr;
	substream->dma_buffer.bytes = size;
	substream->dma_buffer.area = vaddr;
	substream->dma_buffer.dev.type = SNDRV_DMA_TYPE_DEV;
	substream->dma_buffer.dev.dev = audio->mdev->dev;
	substream->buffer_bytes_max = size;
	substream->dma_max = size;

	return 0;
}

static int mwv207d_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct mwv207d_audio_channel *audio = snd_pcm_substream_chip(substream);
	unsigned int size = params_buffer_bytes(params);

	if (!audio->mdev->gart)
		return snd_pcm_lib_malloc_pages(substream, size);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	return 0;
}

static int mwv207d_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct mwv207d_audio_channel *audio;
	audio = snd_pcm_substream_chip(substream);

	if (!audio->mdev->gart)
		return snd_pcm_lib_free_pages(substream);

	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int mwv207d_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct mwv207d_audio_channel *audio;
	unsigned long flags;
	int ret = 0;

	audio = snd_pcm_substream_chip(substream);

	spin_lock_irqsave(&audio->lock, flags);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		audio->buf_offset = 0;
		audio->running = true;
		if (!audio->use_fake_dma)
			mwv207d_audio_hdmi_start_dma(audio);

		substream->runtime->delay = substream->runtime->period_size;
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
		audio->running = false;
		if (!audio->use_fake_dma)
			mwv207d_audio_hdmi_stop_dma(audio);

		break;
	default:
		ret = -EINVAL;
		break;
	}
	spin_unlock_irqrestore(&audio->lock, flags);

	return ret;
}

static snd_pcm_uframes_t mwv207d_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct mwv207d_audio_channel *audio;

	audio = substream->private_data;

	return bytes_to_frames(substream->runtime, audio->buf_offset);
}

static void mwv207d_fake_dma_prepare(struct mwv207d_audio_channel *audio,
				struct snd_pcm_runtime *runtime)
{
	unsigned int period, rate;
	unsigned long nsecs;
	long sec;

	period = runtime->period_size;
	rate = runtime->rate;
	sec = period / rate;
	period %= rate;
	nsecs = div_u64((u64) period * 1000000000UL + rate - 1, rate);

	audio->period_time = ktime_set(sec, nsecs);
}

static int mwv207d_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct mwv207d_audio_channel *audio;
	struct snd_pcm_runtime *runtime;
	unsigned long flags;
	int ret;

	runtime = substream->runtime;
	audio = snd_pcm_substream_chip(substream);

	spin_lock_irqsave(&audio->lock, flags);
	audio->use_fake_dma = !audio->enabled;
	spin_unlock_irqrestore(&audio->lock, flags);

	if (audio->use_fake_dma)
		hrtimer_start(&audio->fake_dma, ms_to_ktime(4),
			      HRTIMER_MODE_REL);
	else
		hrtimer_cancel(&audio->fake_dma);

	if (runtime->format != SNDRV_PCM_FORMAT_S24) {
		pr_err("unsupported pcm foramt %d on audio chanel %d\n",
		       runtime->format, audio->idx);
		return -EINVAL;
	}

	audio->dma_addr = runtime->dma_addr;
	audio->channels = runtime->channels;
	audio->buf_size = snd_pcm_lib_buffer_bytes(substream);
	audio->buf_period = snd_pcm_lib_period_bytes(substream);

	if (audio->use_fake_dma) {
		mwv207d_fake_dma_prepare(audio, runtime);
		return 0;
	}

	ret = snd_pcm_hw_constraint_eld(substream->runtime, audio->eld);
	if (ret)
		return ret;

	runtime->hw.fifo_size = 0x40 * 32;

	ret = mwv207d_audio_hdmi_prepare(audio, runtime->rate);
	if (ret)
		return ret;

	return 0;
}

static int mwv207d_pcm_mmap(struct snd_pcm_substream *substream,
			    struct vm_area_struct *vma)
{
	struct mwv207d_audio_channel *audio = snd_pcm_substream_chip(substream);
	struct ttm_tt *ttm = audio->bo->tbo.ttm;
	unsigned long address = vma->vm_start;
	unsigned long page_offset;
	unsigned long page_last;
	unsigned long pfn;
	int ret;

	vm_flags_set(vma, VM_PFNMAP);
	vm_flags_set(vma, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);

	page_offset = vma->vm_pgoff;
	page_last = vma_pages(vma) + vma->vm_pgoff;
	if (unlikely(page_last > ttm->num_pages))
		return VM_FAULT_SIGBUS;

	for (; page_offset < page_last; page_offset++, address += PAGE_SIZE) {
		pfn = page_to_pfn(ttm->pages[page_offset]);
		ret = remap_pfn_range(vma, address, pfn, PAGE_SIZE, vma->vm_page_prot);
		if (ret)
			return ret;
	}

	return 0;
}

static struct snd_pcm_ops mwv207d_pcm_ops = {
	.open = mwv207d_pcm_open,
	.close = mwv207d_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = mwv207d_pcm_hw_params,
	.hw_free = mwv207d_pcm_hw_free,
	.prepare = mwv207d_pcm_prepare,
	.trigger = mwv207d_pcm_trigger,
	.pointer = mwv207d_pcm_pointer,
};

static void mwv207d_jack_switch(struct mwv207d_audio_channel *audio, bool on)
{
	if (on && !audio->has_jack)
		snd_jack_report(audio->jack, SND_JACK_AVOUT);
	if (!on && audio->has_jack)
		snd_jack_report(audio->jack, 0);
	audio->has_jack = on;
}

static void mwv207d_jack_work(struct work_struct *work)
{
	struct mwv207d_audio_channel *audio = container_of(to_delayed_work(work),
							struct mwv207d_audio_channel,
							jack_work);
	if (!audio->jack)
		return;

	mutex_lock(&audio->mutex);
	if (audio->enabled) {
		BUG_ON(!audio->hw_connected);
		mwv207d_jack_switch(audio, true);
	} else {
		bool new_state = audio->hw_connected ?
		    mwv207d_audio_hdmi_is_enabled(audio) : false;
		mwv207d_jack_switch(audio, new_state);
	}
	mutex_unlock(&audio->mutex);
}

static void mwv207d_enable_audio_chanel(struct mwv207d_audio_channel *audio)
{
	struct snd_pcm_substream *substream;
	unsigned long flags;

	spin_lock_irqsave(&audio->lock, flags);
	if (audio->enabled) {
		spin_unlock_irqrestore(&audio->lock, flags);
		return;
	}
	audio->enabled = true;
	spin_unlock_irqrestore(&audio->lock, flags);

	substream = mwv207d_audio_substream_get(audio);
	if (substream) {
		snd_pcm_stop_xrun(substream);
		mwv207d_audio_substream_put(audio);
	}

	schedule_delayed_work(&audio->jack_work, msecs_to_jiffies(1000));
}

static void mwv207d_disable_audio_chanel(struct mwv207d_audio_channel *audio)
{
	struct snd_pcm_substream *substream;
	unsigned int delayed_time;
	unsigned long flags;

	spin_lock_irqsave(&audio->lock, flags);
	if (!audio->enabled) {
		spin_unlock_irqrestore(&audio->lock, flags);
		return;
	}

	mwv207d_audio_hdmi_stop_dma(audio);

	audio->enabled = false;
	spin_unlock_irqrestore(&audio->lock, flags);

	substream = mwv207d_audio_substream_get(audio);
	if (substream) {
		snd_pcm_stop_xrun(substream);
		mwv207d_audio_substream_put(audio);
	}

	delayed_time = audio->hw_connected ? 3000 : 0;
	schedule_delayed_work(&audio->jack_work, msecs_to_jiffies(delayed_time));
}

static enum hrtimer_restart mwv207d_audio_fake_dma_worker(struct hrtimer *timer)
{
	struct mwv207d_audio_channel *audio = container_of(timer,
							struct mwv207d_audio_channel,
							fake_dma);
	struct snd_pcm_substream *substream;
	unsigned long flags;
	int elapse = 0;
	u32 offset;

	spin_lock_irqsave(&audio->lock, flags);
	if (audio->running) {
		offset = audio->buf_offset + audio->buf_period;
		if (offset >= audio->buf_size)
			offset = 0;
		audio->buf_offset = offset;
		elapse = 1;
	}
	spin_unlock_irqrestore(&audio->lock, flags);

	substream = mwv207d_audio_substream_get(audio);
	if (substream) {
		if (elapse)
			snd_pcm_period_elapsed(substream);
		mwv207d_audio_substream_put(audio);
	}

	hrtimer_forward_now(timer, audio->period_time);
	return HRTIMER_RESTART;
}

static void mwv207d_private_free(struct snd_card *card)
{
	struct mwv207d_audio_card *mcard;
	struct mwv207d_audio_channel *audio;
	int audio_id;

	mcard = card->private_data;
	for_each_audio(mcard, audio_id) {
		audio = &mcard->audio[audio_id];
		hrtimer_cancel(&audio->fake_dma);
		cancel_delayed_work_sync(&audio->jack_work);
	}
}

static void mwv207d_audio_init(struct mwv207d_audio_channel *audio)
{
	audio->running = false;
	audio->enabled = false;
	audio->has_jack = false;
	audio->hw_connected = false;
	audio->stream_info.substream_refcount = 0;

	spin_lock_init(&audio->lock);
	mutex_init(&audio->mutex);

	hrtimer_init(&audio->fake_dma, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	audio->fake_dma.function = mwv207d_audio_fake_dma_worker;
	INIT_DELAYED_WORK(&audio->jack_work, mwv207d_jack_work);
}

static int mwv207d_create_jack(struct snd_card *card,
		struct mwv207d_audio_channel *audio, int idx)
{
	char hdmi_str[32];
	int ret;

	snprintf(hdmi_str, sizeof(hdmi_str), "HDMI/DP,pcm=%d", idx);

	ret = snd_jack_new(card, hdmi_str,
			   SND_JACK_AVOUT, &audio->jack, true, false);
	if (ret)
		return ret;

	if (audio->jack == NULL)
		pr_info("sound jack is not supported.");

	return 0;
}

void mwv207d_audio_isr(struct mwv207d_device *mdev, int idx)
{
	struct mwv207d_audio_channel *audio;
	struct snd_pcm_substream *substream;

	if (!mdev->mcard)
		return;

	audio = &mdev->mcard->audio[idx];

	substream = mwv207d_audio_substream_get(audio);
	if (!substream)
		return;

	if (snd_pcm_playback_empty(substream))
		snd_pcm_stop_xrun(substream);

	spin_lock(&audio->lock);
	if (audio->enabled && audio->running)
		mwv207d_audio_hdmi_start_dma(audio);
	spin_unlock(&audio->lock);

	snd_pcm_period_elapsed(substream);

	mwv207d_audio_substream_put(audio);

	return;
}

struct mwv207d_audio_card *mwv207d_audio_create(struct mwv207d_device *mdev)
{
	struct snd_card *card;
	struct mwv207d_audio_card *mcard;
	struct mwv207d_audio_channel *audio;
	int ret, audio_id;

	if (!enable_audio)
		return NULL;

	ret = snd_card_new(mdev->dev, audio_card_index,
			   audio_card_id, THIS_MODULE,
			   sizeof(struct mwv207d_audio_card), &card);
	if (ret) {
		pr_err("failed to create snd card, ret = %d\n", ret);
		return NULL;
	}

	strcpy(card->driver, DRIVER_NAME);
	strcpy(card->shortname, DRIVER_NAME);
	strcpy(card->longname, "Jingjia Micro MWV207D HDMI/DP Audio");
	card->private_free = mwv207d_private_free;

	mcard = card->private_data;
	mcard->chanel_nr = 0x4;
	mcard->card = card;

	for_each_audio(mcard, audio_id) {
		audio = &mcard->audio[audio_id];
		audio->mmio = mdev->mmio + MWV207D_AUDIO_BASE(audio_id);
		audio->ctrl_mmio = mdev->mmio + MWV207D_CTRL_BASE(audio_id);
		audio->idx = audio_id;
		audio->mdev = mdev;
		mwv207d_audio_init(audio);

		ret = snd_pcm_new(card, DRIVER_NAME, audio_id,
				  0x1,
				  0x0, &audio->pcm);
		if (ret) {
			pr_err("snd_pcm_new failed %d\n", ret);
			goto err_out;
		}

		audio->pcm->private_data = audio;
		audio->pcm->info_flags = 0;
		snprintf(audio->pcm->name, sizeof(audio->pcm->name), "%s %d",
			 card->shortname, audio->idx);

		if (mdev->gart)
			mwv207d_pcm_ops.mmap = mwv207d_pcm_mmap;
		snd_pcm_set_ops(audio->pcm, SNDRV_PCM_STREAM_PLAYBACK, &mwv207d_pcm_ops);

		if (mdev->gart) {
			ret = mwv207d_audio_buffer_new(audio, (1024*1024));
			if (ret)
				goto err_out;
		} else
			snd_pcm_lib_preallocate_pages_for_all(audio->pcm, SNDRV_DMA_TYPE_DEV,
					mdev->dev, 1024 * 1024,
					1024 * 1024);

		if (mwv207d_create_jack(card, audio, audio->idx))
			goto err_out;
	}

	ret = snd_card_register(card);
	if (ret) {
		pr_err("snd_card_register failed %d\n", ret);
		goto err_out;
	}

	dev_info(mdev->dev, "mwv207d audio create done\n");

	return mcard;

err_out:
	snd_card_free(card);
	dev_info(mdev->dev, "mwv207d audio create failed\n");
	return NULL;
}

void mwv207d_audio_suspend(struct mwv207d_device *mdev)
{
	struct mwv207d_audio_card *mcard = mdev->mcard;
	struct mwv207d_audio_channel *audio;
	int audio_id;

	if (!mcard)
		return;

	snd_power_change_state(mcard->card, SNDRV_CTL_POWER_D3cold);
	for_each_audio(mcard, audio_id) {
		audio = &mcard->audio[audio_id];

		mwv207d_disable_audio_chanel(audio);
		hrtimer_cancel(&audio->fake_dma);
		snd_pcm_suspend_all(audio->pcm);
		cancel_delayed_work_sync(&audio->jack_work);
	}

	return;
}

void mwv207d_audio_resume(struct mwv207d_device *mdev)
{
	if (!mdev->mcard)
		return;

	snd_power_change_state(mdev->mcard->card, SNDRV_CTL_POWER_D0);

	return;
}

void mwv207d_audio_destroy(struct mwv207d_device *mdev)
{
	struct mwv207d_audio_card *mcard = mdev->mcard;
	struct mwv207d_audio_channel *audio;
	int audio_id;

	if (!enable_audio || !mcard)
		return;

	for_each_audio(mcard, audio_id) {
		if (!mdev->gart)
			break;
		audio = &mcard->audio[audio_id];
		mwv207d_bo_destroy_pin_mapped(audio->bo);
	}

	snd_card_free(mdev->mcard->card);
}

void mwv207d_audio_switch(struct mwv207d_device *mdev, int idx,
		uint8_t *eld, u32 clock, bool active, bool hw_connected)
{
	struct mwv207d_audio_channel *audio;

	if (!enable_audio || !mdev->mcard)
		return;

	audio = &mdev->mcard->audio[idx];

	mutex_lock(&audio->mutex);
	audio->clock = clock;
	audio->hw_connected = hw_connected;
	if (active) {
		if (!eld)
			return;
		memcpy(audio->eld, eld, sizeof(audio->eld));
		mwv207d_enable_audio_chanel(audio);
	} else {
		memset(audio->eld, 0, sizeof(audio->eld));
		mwv207d_disable_audio_chanel(audio);
	}
	mutex_unlock(&audio->mutex);
}
