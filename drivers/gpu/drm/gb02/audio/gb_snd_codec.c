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
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>
#include <sound/pcm.h>
#include <sound/jack.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <sound/dmaengine_pcm.h>

#include "gb_snd_codec.h"
#include "designware_i2s.h"
#include "local.h"
#include "gb02_fpga_v2vdma.h"
#include "i2s_platform.h"
#include "common/xt.h"
#include "common/gb_common.h"
#include "common/gb_pcie_info.h"

static int GB02FUNC326(struct snd_pcm_substream *substream);
static int GB02FUNC306(struct snd_pcm_substream *substream);

static const struct snd_pcm_hardware gb_pcm_hardware_playback = {
	.info = SNDRV_PCM_INFO_DRAIN_TRIGGER|
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_BATCH |
		SNDRV_PCM_INFO_SYNC_START |
		SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min = GB02MAC19,
	.channels_max = GB02MAC25,
	.rates = SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
		SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |
		SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |
		SNDRV_PCM_RATE_192000,
	.rate_min = 32000,
	.rate_max = 192000,
	.buffer_bytes_max = GB02MAC710,
	.period_bytes_min = GB02MAC707,
	.period_bytes_max = GB02MAC705,
	.periods_min = GB02MAC697,
	.periods_max = GB02MAC699,
	.fifo_size = 0,
};

static void GB02FUNC261(struct GB02STR86 *i2s_cinfo,
	int idx, unsigned int count)
{
	i2s_cinfo->g_sample_cnt[idx] = count;
}

static void GB02FUNC265(struct GB02STR86 *i2s_cinfo,
	int idx, int suspend_val)
{
	i2s_cinfo->suspend_play_flag[idx] = suspend_val;
}

static int GB02FUNC266(struct GB02STR86 *i2s_cinfo, int idx)
{
	return i2s_cinfo->suspend_play_flag[idx];
}

void GB02FUNC267(struct snd_pcm_substream *substream)
{
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = substream->pcm;
	int idx = pcm->device;

	i2s_cinfo->playback_substream[idx] = substream;
	i2s_cinfo->audio_data_info[idx].in_buf =
		(unsigned char *)kmalloc(GB02MAC720, GFP_KERNEL);
	if (i2s_cinfo->audio_data_info[idx].in_buf == NULL) {
		gb_printf(KERN_ERR, "%s-%d: malloc inbuf error\n",
			__func__, __LINE__);
		goto err;
	}
	i2s_cinfo->audio_data_info[idx].out_buf =
		(unsigned char *)kmalloc(GB02MAC720 * 2, GFP_KERNEL);
	if (i2s_cinfo->audio_data_info[idx].out_buf == NULL) {
		gb_printf(KERN_ERR, "%s-%d: malloc out_buf error\n",
			__func__, __LINE__);
		goto err;
	}
	memset((void *)i2s_cinfo->audio_data_info[idx].in_buf, 0,
		GB02MAC720);
	memset((void *)i2s_cinfo->audio_data_info[idx].out_buf, 0,
		GB02MAC720 * 2);
	GB02FUNC265(i2s_cinfo, idx, AUDIO_SUSPEND_DISRESTORE);
	return;
err:
	kfree(i2s_cinfo->audio_data_info[idx].in_buf);
	i2s_cinfo->audio_data_info[idx].in_buf = NULL;
	kfree(i2s_cinfo->audio_data_info[idx].out_buf);
	i2s_cinfo->audio_data_info[idx].out_buf = NULL;
}

void GB02FUNC273(int num, struct GB02STR86 *i2s_cinfo)
{
	struct GB02STR96 *i2s_dev = &i2s_cinfo->i2s_info;
	struct GB02STR42 *chip = &i2s_cinfo->dma_info;
	struct GB02STR40 *chan = NULL;
	struct snd_pcm_substream *substream =
		i2s_cinfo->playback_substream[num];
	struct snd_pcm_runtime *runtime = substream->runtime;
	u32 status = 0;

	chan = &chip->chan[num];
	status = GB02FUNC109(chan);
	GB02FUNC106(chan, status);

	GB02FUNC57(i2s_dev, SNDRV_PCM_STREAM_PLAYBACK, num);

#ifndef GB02MAC296
	i2s_cinfo->dma_info.dma_src_paddr[num] = i2s_cinfo->aoddr_pbase
		+ GB02MAC916 * num - i2s_cinfo->vphy_base + GB02MAC302;
#else
	/*config dma src addr */
	gb_printf(KERN_INFO, "%lu\n", runtime->buffer_size);
	i2s_cinfo->dma_info.dma_src_paddr[num] = i2s_cinfo->aoddr_pbase
		+ GB02MAC916 * num - i2s_cinfo->vphy_base;
#endif
	gb_printf(KERN_INFO, "%s-%d: i2s_restart finish!!\n",
		__func__, __LINE__);
}

void GB02FUNC282(struct GB02STR86 *i2s_ctrl)
{
	int i;

	for (i = 0; i < GB02MAC309; i++) {
		if ((i2s_ctrl->GB02STR84[i].pcm_channel == 0)
			&& (i2s_ctrl->GB02STR84[i].audio_sample == 0)
			&& (i2s_ctrl->GB02STR84[i].pcm_width == 0)) {
			i2s_ctrl->GB02STR84[i].pcm_channel =
				GB02MAC19;
			i2s_ctrl->GB02STR84[i].audio_sample =
				GB02MAC31;
			i2s_ctrl->GB02STR84[i].pcm_width = GB02MAC41;
		}
	}
}

static void GB02FUNC287(struct snd_pcm_runtime *runtime,
	struct GB02STR86 *i2s_cinfo, int idx)
{
	runtime->hw.channels_max = i2s_cinfo->GB02STR84[idx].pcm_channel;
	switch (i2s_cinfo->GB02STR84[idx].audio_sample) {
		case GB02MAC28:
			runtime->hw.rate_max = 32000;
			runtime->hw.rates = SNDRV_PCM_RATE_32000;
			break;
		case GB02MAC30:
			runtime->hw.rate_max = 44100;
			runtime->hw.rates = SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100;
			break;
		case GB02MAC31:
			runtime->hw.rate_max = 48000;
			runtime->hw.rates = SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000;
			break;
		case GB02MAC33:
			runtime->hw.rate_max = 88200;
			runtime->hw.rates = SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |
				SNDRV_PCM_RATE_88200;
			break;
		case GB02MAC35:
			runtime->hw.rate_max = 96000;
			runtime->hw.rates = SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |
				SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000;
			break;
		case GB02MAC37:
			runtime->hw.rate_max = 176400;
			runtime->hw.rates = SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |
				SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 |
				SNDRV_PCM_RATE_176400;
			break;
		case GB02MAC38:
			runtime->hw.rate_max = 192000;
			runtime->hw.rates = SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |
				SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 |
				SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000;
			break;
		default:
			runtime->hw.rate_max = 48000;
			runtime->hw.rates = SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000;
			break;
	}
	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_RATE,
		32000, runtime->hw.rate_max);
	
	switch (i2s_cinfo->GB02STR84[idx].pcm_width) {
	case GB02MAC41:
		runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;
		break;
	case GB02MAC43:
		runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S20_LE;
		break;
	case GB02MAC45:
		runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S20_LE | SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S24_3LE;
		break;
	default:
		runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;
		break;
	}
}

static int GB02FUNC299(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = substream->pcm;
	int idx = pcm->device;
	int ret = 0;

	runtime->hw = gb_pcm_hardware_playback;
	GB02FUNC287(runtime, i2s_cinfo, idx);
	snd_pcm_limit_hw_rates(runtime);

	ret = snd_pcm_hw_constraint_integer(runtime,
		SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		gb_printf(KERN_ERR, "%s-%d: set integer constraint failed\n",
			__func__, __LINE__);
		return ret;
	}
	if (snd_BUG_ON(!runtime->hw.formats))
		gb_printf(KERN_INFO, "error\n");
	snd_pcm_set_sync(substream);
	GB02FUNC267(substream);
	gb_printf(KERN_INFO, "%s, open device %d, runtime->hw.rate_max = %d\n",
		__func__, substream->pcm->device, runtime->hw.rate_max);

	return 0;
}

static void GB02FUNC302(struct snd_pcm_substream *substream)
{
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = substream->pcm;
	int idx = pcm->device;

	i2s_cinfo->playback_substream[idx] = NULL;
	kfree(i2s_cinfo->audio_data_info[idx].in_buf);
	i2s_cinfo->audio_data_info[idx].in_buf = NULL;
	kfree(i2s_cinfo->audio_data_info[idx].out_buf);
	i2s_cinfo->audio_data_info[idx].out_buf = NULL;
	gb_printf(KERN_INFO, "%s, idx = %d\n", __func__, idx);
}

static int GB02FUNC306(struct snd_pcm_substream *substream)
{
#ifdef AUDIO_SOUND_SWITCH_DIFF_A
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = substream->pcm;
	int idx = pcm->device;

	if (i2s_cinfo->jack_stop_pcm[idx] == 1) {
		if ((i2s_cinfo->dp_connect_status[idx] == DP_CONNECT)
		&& (i2s_cinfo->dp_edid_support_audio[idx] == DP_SUPPORT_AUDIO))
			snd_jack_report(i2s_cinfo->jack[idx], SND_JACK_AVOUT);
		i2s_cinfo->jack_stop_pcm[idx] = 0;
	}
#endif
	GB02FUNC302(substream);

	gb_printf(KERN_INFO, "%s\n", __func__);
	return 0;
}

void GB02FUNC311(int dp_num, struct GB02STR86 *i2s_cinfo)
{
	struct GB02STR94 param_info;
	struct GB02STR96 *i2s_dev = &i2s_cinfo->i2s_info;

	memset((void *)&param_info, 0, sizeof(param_info));
	param_info.ch_count = i2s_dev->config[dp_num].chan_nr;
	param_info.sample_rate = i2s_dev->config[dp_num].sample_rate;
	param_info.work_mode = SNDRV_PCM_STREAM_PLAYBACK;
	param_info.ip_count = dp_num;
	GB02FUNC514(dp_num, i2s_dev->config[dp_num].sample_rate,
		i2s_dev->config[dp_num].chan_nr,
		i2s_dev->config[dp_num].data_width,
		i2s_cinfo->pll_va, i2s_cinfo->base_switch, i2s_cinfo->dp_va);
	param_info.format = SNDRV_PCM_FORMAT_S32_LE;
	GB02FUNC46(i2s_dev, &param_info, dp_num);
}

static void GB02FUNC319(struct snd_pcm_runtime *runtime)
{
	gb_printf(KERN_INFO, "%s-%d: printf hw_params start!!\n",
		__func__, __LINE__);
	gb_printf(KERN_INFO, "runtime->access = %d\n", runtime->access);
	gb_printf(KERN_INFO, "runtime->format = %d\n", runtime->format);
	gb_printf(KERN_INFO, "runtime->subformat = %d\n", runtime->subformat);
	gb_printf(KERN_INFO, "runtime->rate = %d\n", runtime->rate);
	gb_printf(KERN_INFO, "runtime->channels = %d\n", runtime->channels);
	gb_printf(KERN_INFO, "runtime->period_size = %ld\n",
		runtime->period_size);
	gb_printf(KERN_INFO, "runtime->periods = %d\n", runtime->periods);
	gb_printf(KERN_INFO, "runtime->buffer_size = %ld\n",
		runtime->buffer_size);
	gb_printf(KERN_INFO, "runtime->min_align = %ld\n", runtime->min_align);
	gb_printf(KERN_INFO, "runtime->byte_align = %ld\n",
		runtime->byte_align);
	gb_printf(KERN_INFO, "runtime->frame_bits = %d\n",
		runtime->frame_bits);
	gb_printf(KERN_INFO, "runtime->sample_bits = %d\n",
		runtime->sample_bits);
	gb_printf(KERN_INFO, "runtime->info = %d\n", runtime->info);
	gb_printf(KERN_INFO, "runtime->rate_num = %d\n", runtime->rate_num);
	gb_printf(KERN_INFO, "runtime->rate_den = %d\n", runtime->rate_den);
	gb_printf(KERN_INFO, "%s-%d: printf hw_params finish!!\n",
		__func__, __LINE__);
}

static int GB02FUNC323(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	params_format(params);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	gb_printf(KERN_INFO, "%s-%d: finish!!\n", __func__, __LINE__);
	return 0;
}

static int GB02FUNC326(struct snd_pcm_substream *substream)
{
	gb_printf(KERN_INFO, "%s-%d: gb_card hw_frees\n", __func__, __LINE__);

	return 0;
}

static snd_pcm_uframes_t GB02FUNC328(struct snd_pcm_substream *substream)
{
	struct snd_pcm *pcm = substream->pcm;
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	int idx = pcm->device;
	snd_pcm_uframes_t pos;

	pos = i2s_cinfo->dma_info.pos[idx];

	return pos;
}
#if 0
static int GB02FUNC330(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	gb_printf(KERN_INFO, "%s-%d: gb_card dma_mmap\n", __func__, __LINE__);

	return 0;
}
#endif
static void GB02FUNC333(struct snd_pcm_runtime *runtime,
	struct GB02STR96 *i2s_dev, int dp_num)
{
	switch (runtime->rate) {
		case 32000:
			i2s_dev->config[dp_num].sample_rate = GB02MAC28;
			break;
		case 44100:
			i2s_dev->config[dp_num].sample_rate =
				GB02MAC30;
			break;
		case 48000:
			i2s_dev->config[dp_num].sample_rate = GB02MAC31;
			break;
		case 88200:
			i2s_dev->config[dp_num].sample_rate =
				GB02MAC33;
			break;
		case 96000:
			i2s_dev->config[dp_num].sample_rate = GB02MAC35;
			break;
		case 176400:
			i2s_dev->config[dp_num].sample_rate =
				GB02MAC37;
			break;
		case 192000:
			i2s_dev->config[dp_num].sample_rate = GB02MAC38;
			break;
		default:
			i2s_dev->config[dp_num].sample_rate = GB02MAC31;
			break;
	}
}

void GB02FUNC339(int idx, struct snd_pcm_runtime *runtime,
	struct GB02STR86 *i2s_cinfo)
{
	struct GB02STR96 *i2s_dev = &i2s_cinfo->i2s_info;
	int edid_channels = i2s_cinfo->GB02STR84[idx].pcm_channel;
	int edid_sample = i2s_cinfo->GB02STR84[idx].audio_sample;
	int edid_width = i2s_cinfo->GB02STR84[idx].pcm_width;

	GB02FUNC333(runtime, i2s_dev, idx);
	if (i2s_dev->config[idx].sample_rate > edid_sample)
		i2s_dev->config[idx].sample_rate = edid_sample;

	i2s_dev->config[idx].chan_nr = (runtime->channels > edid_channels) ?
		edid_channels : runtime->channels;
	i2s_dev->config[idx].data_width = (runtime->sample_bits > edid_width) ?
		edid_width : runtime->sample_bits;
}

int GB02FUNC343(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = substream->pcm;
	struct GB02STR96 *i2s_dev = &i2s_cinfo->i2s_info;
	struct GB02STR42 *chip = &i2s_cinfo->dma_info;
	struct GB02STR40 *chan = NULL;
	u32 status = 0;
	int idx = pcm->device;

	chan = &chip->chan[pcm->device];
	status = GB02FUNC109(chan);
	GB02FUNC106(chan, status);

	GB02FUNC319(runtime);
	GB02FUNC339(idx, runtime, i2s_cinfo);

	GB02FUNC311(idx, i2s_cinfo);
	GB02FUNC57(i2s_dev, SNDRV_PCM_STREAM_PLAYBACK, idx);

	i2s_cinfo->dma_info.dma_src_paddr[idx] = i2s_cinfo->aoddr_pbase
		+ GB02MAC916 * idx - i2s_cinfo->vphy_base + GB02MAC306;
	i2s_cinfo->dma_info.pos[idx] = 0;
	gb_printf(KERN_INFO, "%s, finish, dma_src[%d] = 0x%llx\n",
		__func__, idx, i2s_cinfo->dma_info.dma_src_paddr[idx]);

	return 0;
}

static void GB02FUNC345(struct GB02STR86 *i2s_cinfo, int channel)
{
	struct GB02STR42 *chip = &i2s_cinfo->dma_info;
	struct GB02STR40 *chan = NULL;
	u32 status = 0;

	chan = &chip->chan[channel];
	status = GB02FUNC109(chan);
	GB02FUNC106(chan, status);
	GB02FUNC101(&chip->chan[channel], DWAXIDMAC_IRQ_ALL);
	GB02FUNC111(&chip->chan[channel]);
}

static int GB02FUNC347(struct GB02STR86 *i2s_cinfo, int idx)
{
	struct snd_pcm_runtime *runtime;
	int ret = 0, i;
	for (i = 0; i < GB02MAC309; ++i) {
		if (i == idx)
			continue;
		if (i2s_cinfo->playback_substream[i] == NULL)
			continue;
		runtime = i2s_cinfo->playback_substream[i]->runtime;
		if (runtime == NULL)
			continue;
		if (runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
			gb_printf(KERN_INFO, "%s-%d: i = %d, is running\n",
				__func__, __LINE__, i);
			ret = 1;
			break;
		}
	}
	return ret;
}

static void GB02FUNC351(struct GB02STR86 *i2s_cinfo, int dp_num)
{
	struct GB02STR96 *i2s_dev = &i2s_cinfo->i2s_info;
	struct GB02STR42 *chip = &i2s_cinfo->dma_info;
	int ret = -1;


	if ((i2s_cinfo->audio_data_info[dp_num].in_buf == NULL) ||
		(i2s_cinfo->audio_data_info[dp_num].out_buf == NULL))
		return;

	GB02FUNC343(i2s_cinfo->playback_substream[dp_num]);
	memset((void *)i2s_cinfo->audio_data_info[dp_num].in_buf, 0,
		GB02MAC720);
	memset((void *)i2s_cinfo->audio_data_info[dp_num].out_buf, 0,
		GB02MAC720 * 2);
	GB02FUNC345(i2s_cinfo, dp_num);
	ret = GB02FUNC347(i2s_cinfo, dp_num);
	if (ret == 0)
		GB02FUNC134(chip);
	GB02FUNC147(&i2s_cinfo->dma_info,
		i2s_cinfo->pcm_bytes[dp_num], dp_num);
	GB02FUNC31(i2s_dev, SNDRV_PCM_STREAM_PLAYBACK, dp_num);
	gb_printf(KERN_INFO, "%s-%d: resume handle\n", __func__, __LINE__);
}

static void GB02FUNC357(struct GB02STR86 *i2s_cinfo, int mute_flag)
{
	uint32_t val = 0;
	u64 dp_va = i2s_cinfo->dp_va;
	int i;

	for (i = 0; i < GB02MAC309; ++i) {
		val = ioread32((void *)(dp_va + i * 0x400000 + 0x400));
		if (mute_flag == AUDIO_SET_MUTE) {
			val |= (0x01 << 15);
			val |= 0x01;
		} else if (mute_flag == AUDIO_SET_UNMUTE)
			val &= 0xffff7ffe;

		iowrite32(val, (void *)(dp_va + i * 0x400000 + 0x400));
	}
}
#ifdef AUDIO_SOUND_SWITCH_DIFF_A
int GB02FUNC360(struct snd_pcm_substream *substream, int cmd)
#else
static int GB02FUNC360(struct snd_pcm_substream *substream, int cmd)
#endif
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct GB02STR96 *i2s_dev = &i2s_cinfo->i2s_info;
	struct GB02STR42 *chip = &i2s_cinfo->dma_info;
//	struct GB02STR40 *chan = NULL;
	struct snd_pcm *pcm = substream->pcm;
	int idx = pcm->device;
	int ret;

#if 0
	if (i2s_cinfo->dp_connect_status[idx] == DP_DISCONNECT) {
		GB02FUNC326(substream);
		GB02FUNC306(substream);
		gb_printf(KERN_INFO, "%s-%d: the dp = %d is DP_DISCONNECT\n",
			__func__, __LINE__, idx);
		return 0;
	}
#endif
	switch (runtime->sample_bits) {
	case 16:
		i2s_cinfo->pcm_bytes[idx] =
			frames_to_bytes(runtime, runtime->period_size) * 2;
		break;
	case 20:
		i2s_cinfo->pcm_bytes[idx] =
			frames_to_bytes(runtime, runtime->period_size) * 2;
		break;
	case 24:
		i2s_cinfo->pcm_bytes[idx] =
			frames_to_bytes(runtime, runtime->period_size) / 3 * 4;
		break;
	default:
		i2s_cinfo->pcm_bytes[idx] =
			frames_to_bytes(runtime, runtime->period_size) * 2;
		break;
	}

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = GB02FUNC347(i2s_cinfo, idx);
		if (ret == 0)
			GB02FUNC134(chip);
		GB02FUNC147(&i2s_cinfo->dma_info,
			i2s_cinfo->pcm_bytes[idx], idx);
		GB02FUNC31(i2s_dev, SNDRV_PCM_STREAM_PLAYBACK, idx);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
//		chan = &chip->chan[idx];
//		GB02FUNC101(chan, DWAXIDMAC_IRQ_ALL);
//		GB02FUNC111(chan);
		GB02FUNC345(i2s_cinfo, idx);
		ret = GB02FUNC347(i2s_cinfo, idx);
		if (ret == 0) {
			GB02FUNC92(chip);
			GB02FUNC83(chip);
		}
		GB02FUNC261(i2s_cinfo, idx, 0); // IEC60958 block start
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		if ((runtime->status->state != SNDRV_PCM_STATE_SUSPENDED)
		|| (GB02FUNC266(i2s_cinfo, idx) !=
			AUDIO_SUSPEND_RESTORE))
			break;
		GB02FUNC351(i2s_cinfo, idx);
		GB02FUNC357(i2s_cinfo, AUDIO_SET_UNMUTE);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
			GB02FUNC345(i2s_cinfo, idx);
			ret = GB02FUNC347(i2s_cinfo, idx);
			if (ret == 0) {
				GB02FUNC92(chip);
				GB02FUNC83(chip);
			}
			GB02FUNC357(i2s_cinfo, AUDIO_SET_MUTE);
			GB02FUNC265(i2s_cinfo,  idx, AUDIO_SUSPEND_RESTORE);
		}
		break;
	}
	gb_printf(KERN_INFO, "%s-%d: cmd = %d, runtime->status->state = %d\n",
		__func__, __LINE__, cmd, runtime->status->state);

	return 0;
}

static int bitcount(unsigned int adata)
{
	int count = 0;

	while (adata) {
		count++;
		adata &= (adata - 1);
	}
	return count;
}

static void GB02FUNC381(struct GB02STR86 *i2s_cinfo,
	unsigned int **pbuf, int idx)
{
	unsigned int g_sample_cnt = i2s_cinfo->g_sample_cnt[idx];

	if (g_sample_cnt == 1) {
		**pbuf = (**pbuf & (~((unsigned int)0x3 << GB02MAC792)));
	} else if (g_sample_cnt == GB02MAC793) { // 2 sample * 192 frames
		g_sample_cnt = 0; // transport one complete block, reset the counter
	} else {
		// do nothing
	}
	i2s_cinfo->g_sample_cnt[idx] = g_sample_cnt;
}

/* IEC60958-3 */
static void GB02FUNC386(struct GB02STR86 *i2s_cinfo,
	unsigned int **pbuf, int idx)
{
	struct snd_pcm_substream *substream =
		i2s_cinfo->playback_substream[idx];
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int g_sample_cnt = i2s_cinfo->g_sample_cnt[idx];

	g_sample_cnt++;
	/* Software for which no copyright is asserted. */
	if ((g_sample_cnt == GB02MAC733) ||
	    (g_sample_cnt == GB02MAC736)) { // Bit 2 for both left & right channel
		**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
	}

	/* channel number set, 2 channel mode */
	if (g_sample_cnt == GB02MAC739) { // Bit 20 for left channel
		**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
	}
	if (g_sample_cnt == GB02MAC742) { // Bit 21 for right channel
		**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
	}

	/* word length set, max length is 24bit */
	switch (runtime->sample_bits) {
		case 16:
			if ((g_sample_cnt == GB02MAC772) ||
			    (g_sample_cnt == GB02MAC774)) { // Bit 33 for both left & right channel
				**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
			break;
		case 24:
			if ((g_sample_cnt == GB02MAC768) ||
			    (g_sample_cnt == GB02MAC770)) { // Bit 32 for both left & right channel
				**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
			if ((g_sample_cnt == GB02MAC772) ||
			    (g_sample_cnt == GB02MAC774) || // Bit 33 for both left & right channel
			    (g_sample_cnt == GB02MAC782) ||
				(g_sample_cnt == GB02MAC785)) { // Bit 35 for both left & right channel
				**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
		    break;
		default:
			gb_printf(KERN_INFO, "%s-L%d: unsupport sample bits format:%d\n", __func__, __LINE__, runtime->sample_bits);
	}

	/* sample frequence set */
	switch (runtime->rate) {
		case 32000:
			if ((g_sample_cnt == GB02MAC744) ||
			    (g_sample_cnt == GB02MAC747) || // Bit 24 for both left & right channel
			    (g_sample_cnt == GB02MAC750) ||
				(g_sample_cnt == GB02MAC753)) { // Bit 25 for both left & right channel
					**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
			break;
		case 44100:
			/* default 0 for bit 24~27 */
			break;
		case 48000:
			if ((g_sample_cnt == GB02MAC750) ||
			    (g_sample_cnt == GB02MAC753)) { // Bit 25 for both left & right channel
					**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
			break;
		case 88200:
			if ((g_sample_cnt == GB02MAC762) ||
			    (g_sample_cnt == GB02MAC765)) { // Bit 27 for both left & right channel
					**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
			break;
		case 96000:
			if ((g_sample_cnt == GB02MAC750) ||
			    (g_sample_cnt == GB02MAC753) || // Bit 25 for both left & right channel
				(g_sample_cnt == GB02MAC762) ||
				(g_sample_cnt == GB02MAC765)) { // Bit 27 for both left & right channel
					**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
			break;
		case 176400:
			if ((g_sample_cnt == GB02MAC756) ||
			    (g_sample_cnt == GB02MAC759) || // Bit 26 for both left & right channel
				(g_sample_cnt == GB02MAC762) ||
				(g_sample_cnt == GB02MAC765)) { // Bit 27 for both left & right channel
					**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
			break;
		case 192000:
		    if ((g_sample_cnt == GB02MAC750) ||
			    (g_sample_cnt == GB02MAC753) || // Bit 25 for both left & right channel
				(g_sample_cnt == GB02MAC756) ||
				(g_sample_cnt == GB02MAC759) || // Bit 26 for both left & right channel
			    (g_sample_cnt == GB02MAC762) ||
				(g_sample_cnt == GB02MAC765)) { // Bit 27 for both left & right channel
					**pbuf = (**pbuf | ((unsigned int)0x1 << GB02MAC789));
			}
			break;
		default:
			gb_printf(KERN_INFO, "%s-L%d: unsupport sample rate format:%d\n", __func__, __LINE__, runtime->rate);
	}
	i2s_cinfo->g_sample_cnt[idx] = g_sample_cnt;
}

static int GB02FUNC400(struct GB02STR86 *i2s_cinfo,
	int sample_dep, int len, int idx)
{
	int i = 0, j = 0;
	unsigned int *vbuf = NULL;
	int bcount = 0;
	enum gb_board_type gb_type;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	unsigned char *aubuf = i2s_cinfo->audio_data_info[idx].in_buf;
	unsigned char *vrambuf = i2s_cinfo->audio_data_info[idx].out_buf;

	gb_type = GB02FUNC503(pcie_info);
	if (sample_dep == GB02MAC45) {
		for (i = 0; i < len; ) {
			vbuf = (int *)(&vrambuf[j]);
			*vbuf =
			aubuf[i + 2] << 16 | aubuf[i + 1] << 8 | aubuf[i];
			GB02FUNC386(i2s_cinfo, &vbuf, idx);
			bcount = bitcount(*vbuf);
			*vbuf &= ~(1 << 24);
			if (i % 6 == 0) {
			//	gb_printf(KERN_INFO, "data is left---\n");
				*vbuf = (*vbuf | 1 << 28);
			} else {
			//	gb_printf(KERN_INFO, "data is right\n");
				*vbuf = (*vbuf | 1 << 29);
			}

			if (bcount % 2)
				*vbuf = (*vbuf | 1 << 27);
		//	gb_printf(KERN_INFO, "data =%x--\n", *vbuf);
			i += 3;
			j += 4;
			GB02FUNC381(i2s_cinfo, &vbuf, idx);
			*vbuf = *vbuf << 2;
		}
	} else {
		for (i = 0; i < len;) {
			vbuf = (unsigned int *)(&vrambuf[j]);
			#ifdef I2S_PORT
			*vbuf =  (unsigned char)aubuf[i + 1]<<8 |
				(unsigned char)aubuf[i];
			#else
			*vbuf =  (unsigned char)aubuf[i + 1] << 16 |
				(unsigned char)aubuf[i] << 8;
			#endif
			GB02FUNC386(i2s_cinfo, &vbuf, idx);
			bcount = bitcount(*vbuf);
			*vbuf &= ~(1 << 24);
			if (i % 4 == 0)
				*vbuf = (*vbuf | 1 << 28);
			else
				*vbuf = (*vbuf | 1 << 29);

			if (bcount % 2)
				*vbuf = (*vbuf | 1 << 27);
			i += 2;
			j += 4;
			GB02FUNC381(i2s_cinfo, &vbuf, idx);
			*vbuf = *vbuf << 2;
		}
	}

	return j;
}


static void GB02FUNC410(struct GB02STR86 *i2s_cinfo, int idx)
{
	if (GB02FUNC560(idx) == AUDIO_NEED_RESTART) {
		GB02FUNC311(idx, i2s_cinfo);
		GB02FUNC556(idx, AUDIO_DISNEED_RESTART);
	}

	if (GB02FUNC266(i2s_cinfo, idx) != AUDIO_SUSPEND_RESTORE)
		return;
	GB02FUNC311(idx, i2s_cinfo);
	GB02FUNC265(i2s_cinfo, idx, AUDIO_SUSPEND_DISRESTORE);
}

u32 GB02FUNC415(struct snd_pcm_runtime *runtime, u64 pos)
{
	int ret_val;

	switch (runtime->sample_bits) {
	case GB02MAC41:
		ret_val = pos * 2;
		break;
	case GB02MAC43:
		ret_val = pos * 2;
		break;
	case GB02MAC45:
		ret_val = pos / 3 * 4;
		break;
	default:
		ret_val = pos * 2;
		break;
	}
	return ret_val;
}

#ifdef DEBUG_SAVE_FILE
static void GB02FUNC419(struct snd_pcm_substream *substream,
	unsigned long pos, unsigned long count, int flag)
#else
static void GB02FUNC419(struct snd_pcm_substream *substream,
	unsigned long pos, unsigned long count)
#endif
{
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_pcm *pcm = substream->pcm;
	int PCM_bits = GB02MAC41;
	int idx = pcm->device;
	unsigned char *out_buf = i2s_cinfo->audio_data_info[idx].out_buf;
	u64 vaddr;
	int pos_offset, out_buf_len;
#ifdef DEBUG_SAVE_FILE
	unsigned char *t_buf = i2s_cinfo->audio_data_info[idx].in_buf;
	static loff_t fp_pos = 0;
#endif

#ifdef DEBUG_SAVE_FILE
	if (flag == 1)
		kernel_write(i2s_cinfo->fp, t_buf, count, &fp_pos);
#endif

	GB02FUNC410(i2s_cinfo, idx);
	/*packet user data for IEC60958*/

	memset((void *)out_buf, 0, count * 2);
	PCM_bits = runtime->sample_bits;
	out_buf_len = GB02FUNC400(i2s_cinfo, PCM_bits, count, idx);

	pos_offset = GB02FUNC415(runtime, pos);
	vaddr = (u64)i2s_cinfo->ddrwr_va + i2s_cinfo->aoddr_pbase
		- i2s_cinfo->vphy_base + GB02MAC916 * idx
		+ pos_offset;
/*	gb_printf(KERN_INFO, "pos = %lu, vaddr = 0x%llx, count=%lu, ps =%lu, \
		idx = %d, pos_offset = %d, out_buf_len = %d\n", \
		pos, vaddr, count, runtime->period_size, \
		idx, pos_offset, out_buf_len);*/
#ifndef CONFIG_SW64
	memset((void *)vaddr, 0, out_buf_len);
#else
	memset_io((void *)vaddr, 0, out_buf_len);
#endif
	memcpy_toio((void *)vaddr, (void *)out_buf, out_buf_len);
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static int gb_snd_playback_copy_user(struct snd_pcm_substream *substream,
	int channel, unsigned long pos, void __user *src, unsigned long count)
{
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = substream->pcm;
	int idx = pcm->device;
	unsigned char *t_buf = i2s_cinfo->audio_data_info[idx].in_buf;
	unsigned char *out_buf = i2s_cinfo->audio_data_info[idx].out_buf;
	int ret;

	if (t_buf == NULL || out_buf == NULL) {
		gb_printf(KERN_ERR, "%s-%d: tbuf or out_buf = NUll, error\n",
			__func__, __LINE__);
		return 0;
	}
	if (count > GB02MAC720) {
		gb_printf(KERN_ERR, "%s-%d: copy_user count = %ld, error\n",
			__func__, __LINE__, count);
		return 0;
	}

	memset((void *)t_buf, 0, GB02MAC720);
	ret = copy_from_user((void *)t_buf, src, count);
	if (ret) {
		gb_printf(KERN_ERR, "%s-%d: ret = %d, error copy_from_user\n",
			__func__, __LINE__, ret);
		return -EFAULT;
	}
#ifdef DEBUG_SAVE_FILE
	GB02FUNC419(substream, pos, count, 0);
#else
	GB02FUNC419(substream, pos, count);
#endif

	return 0;
}

static int GB02FUNC432(struct snd_pcm_substream *substream,
	int channel, unsigned long pos, void *src, unsigned long count)
{
	gb_printf(KERN_INFO, "%s-%d: gb-card copy_kernel finish!!\n",
		__func__, __LINE__);
	return 0;
}
#else
static int GB02FUNC435(struct snd_pcm_substream *substream,
			  int voice, unsigned long pos,
			  struct iov_iter *src, unsigned long count)
{
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = substream->pcm;
	int idx = pcm->device;
	unsigned char *t_buf = i2s_cinfo->audio_data_info[idx].in_buf;
	unsigned char *out_buf = i2s_cinfo->audio_data_info[idx].out_buf;
	int ret;

	if (t_buf == NULL || out_buf == NULL) {
		gb_printf(KERN_ERR, "%s-%d: tbuf or out_buf = NUll, error\n",
			__func__, __LINE__);
		return 0;
	}
	if (count > GB02MAC720) {
		gb_printf(KERN_ERR, "%s-%d: copy_user count = %ld, error\n",
			__func__, __LINE__, count);
		return 0;
	}

	memset((void *)t_buf, 0, GB02MAC720);
	ret = copy_from_iter((void *)t_buf, count, src);
	if (ret != count) {
		gb_printf(KERN_ERR, "%s-%d: ret = %d, error copy_from_iter_toio\n",
			__func__, __LINE__, ret);
		return -EFAULT;
	}
#ifdef DEBUG_SAVE_FILE
	GB02FUNC419(substream, pos, count, 1);
#else
	GB02FUNC419(substream, pos, count);
#endif
	return 0;
}
#endif

static int GB02FUNC445(struct snd_pcm_substream *substream,
	int channel, unsigned long pos, unsigned long count)
{
	struct GB02STR86 *i2s_cinfo = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = substream->pcm;
	int idx = pcm->device;
	unsigned char *t_buf = i2s_cinfo->audio_data_info[idx].in_buf;
	unsigned char *out_buf = i2s_cinfo->audio_data_info[idx].out_buf;

	if (t_buf == NULL || out_buf == NULL) {
		gb_printf(KERN_ERR, "%s-%d: tbuf or out_buf = NUll, error\n",
			__func__, __LINE__);
		return 0;
	}
	if (count > GB02MAC720) {
		gb_printf(KERN_ERR, "%s-%d: copy_user count = %ld, error\n",
			__func__, __LINE__, count);
		return 0;
	}

	memset((void *)t_buf, 0, count);
#ifdef DEBUG_SAVE_FILE
	GB02FUNC419(substream, pos, count, 0);
#else
	GB02FUNC419(substream, pos, count);
#endif
	return 0;
}

static struct snd_pcm_ops gb_dma_ops_playback = {
	.open = GB02FUNC299,
	.close = GB02FUNC306,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = GB02FUNC323,
	.hw_free = GB02FUNC326,
	.prepare = GB02FUNC343,
	.trigger = GB02FUNC360,
	.pointer = GB02FUNC328,
//	.mmap = GB02FUNC330,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	.copy_user =    gb_snd_playback_copy_user,
	.copy_kernel =  GB02FUNC432,
#else
	.copy =    GB02FUNC435,
#endif
	.fill_silence = GB02FUNC445,
};

static int GB02FUNC450(struct snd_device *device)
{
	return 0;
}

static int GB02FUNC452(struct snd_device *device)
{
	return 0;
}

int GB02FUNC453(struct device *dev, struct snd_card *card,
	struct GB02STR86 *i2s_infos)
{
	static struct snd_device_ops ops = {
		.dev_disconnect = GB02FUNC450,
		.dev_free = GB02FUNC452,
	};
	int err;

	i2s_infos->card = card;
	err = snd_device_new(card, SNDRV_DEV_LOWLEVEL,
		(void *)i2s_infos, &ops);
	if (err < 0) {
		dev_err(card->dev, "Error creating device [card]!\n");
		return err;
	}
	return 0;
}

static int GB02FUNC457(struct GB02STR86 *i2s_infos)
{
	struct snd_pcm *pcm;
	int err, i = 0;
	int dev_num;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type;
	char name[] = "HDMI 0";
	char hdmi_str[32];
	int hdmi_id = 0;

	static int audio_idx[BOARD_TYPE_MAX][6] = {
		[PCIE_LPDDR4]  = {2},
		[PCIE_C0_200] = {2},
		[PCIE_FULL_LPDDR4] = {0, 2, 3, 5},
		[PCIE_FULL_FUNC_DDR4] = {0, 1, 2, 3, 4, 5},
		[PCIE_M6FL8G_LPDDR4] = {0, 1, 2, 3, 4, 5},
		[PCIE_HIE1LP4_LPDDR4] = {0, 2, 3, 5},
		[PCIE_M4HL8G_LPDDR4] = {0, 2, 3, 5},
		[GB02MAC1363] = {0, 1, 2, 4, 5},
	};

	gb_type = GB02FUNC503(pcie_info);

	switch (gb_type) {
	case PCIE_LPDDR4:
	case PCIE_C0_200:
		dev_num = 1;
	break;
	case PCIE_FULL_LPDDR4:
		dev_num = 4;
	break;
	case PCIE_FULL_FUNC_DDR4:
		dev_num = 6;
	break;
	case PCIE_M6FL8G_LPDDR4:
		dev_num = 6;
	break;
	case PCIE_HIE1LP4_LPDDR4:
		dev_num = 4;
	break;
	case PCIE_M4HL8G_LPDDR4:
		dev_num = 4;
	break;
	case GB02MAC1363:
		dev_num = 5;
	break;
	default:
		dev_num = 6;
	break;
	}
	gb_printf(KERN_INFO, "gb_type = %d, dev_num = %d\n", gb_type, dev_num);
	for (i = 0; i < dev_num; i++) {
		sprintf(name, "HDMI %d", i);
		hdmi_id = audio_idx[gb_type][i];
		err = snd_pcm_new(i2s_infos->card, name,
			hdmi_id, 1, 0, &pcm);
		if (err < 0) {
			gb_printf(KERN_ERR, "%s-%d: err = %d, audio snd pcm new failed!\n",
				__func__, __LINE__, err);
			return err;
		}
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&gb_dma_ops_playback);

		strlcpy(pcm->name, name, sizeof(pcm->name));
		pcm->private_data = i2s_infos;
		i2s_infos->pcm = pcm;
		gb_printf(KERN_INFO, "%s-%d: i = %d, dma_size = %d, max_size = %d\n",
			__func__, __LINE__, i, GB02MAC713, GB02MAC713);

		gb_printf(KERN_INFO, "yth*gb_type=%d, i=%d, **%d\n", gb_type, i, audio_idx[gb_type][i]);
		snprintf(hdmi_str, sizeof(hdmi_str), "HDMI/DP,pcm=%d", audio_idx[gb_type][i]);
		err = snd_jack_new(i2s_infos->card, hdmi_str, 
			SND_JACK_AVOUT, &i2s_infos->jack[hdmi_id], true, false);
		if (err < 0) {
			gb_printf(KERN_ERR, "%s-%d: err = %d, audo snd_jack_new failed!\n",
				__func__, __LINE__, err);
			return err;	
		}

		if ((i2s_infos->dp_connect_status[hdmi_id] == DP_CONNECT)
		&& (i2s_infos->dp_edid_support_audio[hdmi_id] == DP_SUPPORT_AUDIO))
			snd_jack_report(i2s_infos->jack[hdmi_id], SND_JACK_AVOUT);
		else
			snd_jack_report(i2s_infos->jack[hdmi_id], 0);
		i2s_infos->jack_init_flag[hdmi_id] = AUDIO_JACK_INIT;
	}

	return err;
}

void GB02FUNC473(u64 pll_va)
{
	/*set i2s0 pll is 98.304M / 512 = 192khz */
	iowrite32(0x4188905, (void *)(pll_va + 0x120));
	iowrite32(0x4dd2f2, (void *)(pll_va + 0x124));

	/*set i2s1 pll is 90.3168M / 512 = 176.4khz */
	iowrite32(0x4168905, (void *)(pll_va + 0x128));
	iowrite32(0x5119ce, (void *)(pll_va + 0x12c));
}

void GB02FUNC478(struct GB02STR83 *pcie_info)
{
	struct GB02STR40 *chan;
	struct GB02STR86 *i2s_ctrl = GB02FUNC505();
	int i;

#ifndef GB02MAC296
	i2s_ctrl->i2s_info.i2s_base[0] = pcie_info->hdwr_va + GB02MAC301;
	i2s_ctrl->dma_info.regs = pcie_info->hdwr_va;
	i2s_ctrl->aoddr_pbase = GB02MAC502 + pcie_info->ddrwr_pa;
	i2s_ctrl->vphy_base = pcie_info->ddrwr_pa;
	i2s_ctrl->dma_info.dma_src_paddr[0] =
		(long long unsigned)(GB02MAC302);
	i2s_ctrl->dma_info.dma_dst_paddr =
		GB02MAC301 + GB02MAC13 + GB02MAC303;
#else
	i2s_ctrl->aoddr_pbase = pcie_info->ddrwr_pa + GB02MAC502;
	i2s_ctrl->i2s_info.i2s_base[0] = pcie_info->hdwr_va + GB02MAC304;
	i2s_ctrl->dma_info.regs = pcie_info->hdwr_va  + GB02MAC305;
	i2s_ctrl->vphy_base     = pcie_info->ddrwr_pa;
	i2s_ctrl->pll_va        = pcie_info->pll_va;
	i2s_ctrl->base_switch   = pcie_info->base_switch;
	i2s_ctrl->dp_va = (u64)pcie_info->hdwr_va + GB02MAC1060;
	i2s_ctrl->ddrwr_va = pcie_info->ddrwr_va;
	i2s_ctrl->dma_info.dma_src_paddr[0] =
		(long long unsigned)(GB02MAC306);
	i2s_ctrl->dma_info.dma_dst_paddr = GB02MAC304 + GB02MAC13 + GB02MAC307;

	gb_printf(KERN_INFO, "%s-%d: i2s_base = 0x%p, ddrwr_va = 0x%p\n",
	__func__, __LINE__, i2s_ctrl->i2s_info.i2s_base, i2s_ctrl->ddrwr_va);
#endif
	for (i = 0; i < GB02MAC309; i++) {
		chan = &(i2s_ctrl->dma_info.chan[i]);
		chan->id = i;
		chan->chan_regs = i2s_ctrl->dma_info.regs
			+ GB02MAC32 + i * GB02MAC34;
		chan->chip = &i2s_ctrl->dma_info;
		chan->first_desc = NULL;
	}
	for (i = 1; i < GB02MAC309; i++) {
	#ifndef GB02MAC296
		i2s_ctrl->i2s_info.i2s_base[i] =
			i2s_ctrl->i2s_info.i2s_base[i - 1]
			+ 5 * GB02MAC300;
	#else
		i2s_ctrl->i2s_info.i2s_base[i] =
			i2s_ctrl->i2s_info.i2s_base[i - 1] + GB02MAC299;
	#endif
		gb_printf(KERN_INFO, "%s-%d: ip num = %d,base = 0x%p\n",
			__func__, __LINE__, i, i2s_ctrl->i2s_info.i2s_base[i]);
	}

	GB02FUNC473(i2s_ctrl->pll_va);
	GB02FUNC282(i2s_ctrl);
	GB02FUNC114(&i2s_ctrl->dma_info);
	GB02FUNC92(&i2s_ctrl->dma_info);
	GB02FUNC83(&i2s_ctrl->dma_info);
}

int GB02FUNC486(struct device *dev)
{
	struct snd_card *gb_card;
	struct GB02STR86 *i2s_infos = GB02FUNC505();
	int err;

	err = snd_card_new(dev, -1, NULL, THIS_MODULE, 0, &gb_card);
	if (err < 0) {
		gb_printf(KERN_ERR, "%s-%d: err = %d, gb audio new card failed\n",
			__func__, __LINE__, err);
		return err;
	}

	err = GB02FUNC453(dev, gb_card, i2s_infos);
	if (err < 0) {
		gb_printf(KERN_ERR, "%s-%d: err = %d, create audio failed!\n",
			__func__, __LINE__, err);
		return err;
	}

	strcpy(gb_card->shortname, "GB HDMI");
	sprintf(gb_card->longname, "%s at gb alsa", gb_card->shortname);
	strcpy(gb_card->driver, "GB-Audio");

	err = GB02FUNC457(i2s_infos);
	if (err < 0) {
		gb_printf(KERN_ERR, "%s-%d: err = %d, audio card pcm failed\n",
			__func__, __LINE__, err);
		return err;
	}

#ifdef DEBUG_SAVE_FILE
	i2s_infos->fp = filp_open("/dev/test.wav",
		O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (IS_ERR(i2s_infos->fp))
		gb_printf(KERN_ERR, "%s-%d: err = %d, filp_open error\n",
			__func__, __LINE__, err);
#endif
	err = snd_card_register(gb_card);
	if (err < 0) {
		gb_printf(KERN_ERR, "%s-%d: err = %d, card register error\n",
			__func__, __LINE__, err);
		return err;
	}

	gb_printf(KERN_INFO, "%s-%d: gb audio init finish!\n",
		__func__, __LINE__);
	return 0;
}

