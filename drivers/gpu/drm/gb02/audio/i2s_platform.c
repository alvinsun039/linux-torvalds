#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
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
#include "gb02_fpga_v2vdma.h"
#include "local.h"
#include "i2s_platform.h"
#include "common/gb_common.h"
#include "gb_snd_codec.h"
#include "designware_i2s.h"
#include "ip/gb_dp.h"
#include "common/gb_pcie_info.h"

static struct GB02STR86 i2s_infos;
struct GB02STR86 *GB02FUNC505(void)
{
	return &i2s_infos;
}

void GB02FUNC508(void)
{
	struct GB02STR86 *i2s_ctrl;

	i2s_ctrl = GB02FUNC505();
	memset((void *)i2s_ctrl, 0, sizeof(struct GB02STR86));
}

static struct dptx *GB02FUNC510(int dp_index)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];

	return dptx_info;
}

static void GB02FUNC512(struct dptx *dptx, int sample_rate,
	int ch_count, int sample_dep)
{
	struct GB02STR123 *audio_desc;

	audio_desc = &dptx->audio_desc;
	audio_desc->max_sampling_freq = sample_rate;
	audio_desc->max_num_of_channels = ch_count;
	audio_desc->max_bit_per_sample = sample_dep;
}

void GB02FUNC514(int chid, int sample_rate, int ch_count,
	int sample_dep, u64 pll_va, u64 base_switch, u64 dp_va)
{
	uint32_t val = 0, pll_val = 0;
	struct dptx *dptx_info;

	gb_printf(KERN_INFO, "%s-%d: id = %d, sapmle = %d, ch_count = %d\n",
		__func__, __LINE__, chid, sample_rate, ch_count);
	dptx_info = GB02FUNC510(chid);
	GB02FUNC512(dptx_info, sample_rate, ch_count, sample_dep);
	GB02FUNC1440(dptx_info, DPTX_AUDIO_START_PLAY);

	iowrite32(0x60000000, (void *)(base_switch + 0xd0c));

	pll_val = ioread32((void *)(pll_va + 0x154));

	if (chid < 2) {
		val = ioread32((void *)(pll_va + 0x148));
		val &= ~(0xff << (12 + chid * 8));
	} else {
		val = ioread32((void *)(pll_va + 0x14c));
		val &= ~(0xff << (chid - 2) * 8);
	}
	switch (sample_rate) {
	case GB02MAC28:
		pll_val &= ~(1 << (6 + chid)); // pll0
		if (chid < 2)
			val |= GB02MAC911 << (GB02MAC915 + chid * 8);
		else
			val |= GB02MAC911 << (chid - 2) * 8;
		break;
	case GB02MAC30:
		pll_val |= 1 << (6 + chid); // pll1
		if (chid < 2)
			val |= GB02MAC912 << (GB02MAC915 + chid * 8);
		else
			val |= GB02MAC912 << (chid - 2) * 8;
		break;
	case GB02MAC31:
		pll_val &= ~(1 << (6 + chid)); // pll0
		if (chid < 2)
			val |= GB02MAC912 << (GB02MAC915 + chid * 8);
		else
			val |= GB02MAC912 << (chid - 2) * 8;
		break;
	case GB02MAC33:
		pll_val |= 1 << (6 + chid); // pll1
		if (chid < 2)
			val |= GB02MAC913 << (GB02MAC915 + chid * 8);
		else
			val |= GB02MAC913 << (chid - 2) * 8;
		break;
	case GB02MAC35:
		pll_val &= ~(1 << (6 + chid)); // pll0
		if (chid < 2)
			val |= GB02MAC913 << (GB02MAC915 + chid * 8);
		else
			val |= GB02MAC913 << (chid - 2) * 8;
		break;
	case GB02MAC37:
		pll_val |= 1 << (6 + chid); // pll1
		if (chid < 2)
			val |= GB02MAC914 << (GB02MAC915 + chid * 8);
		else
			val |= GB02MAC914 << (chid - 2) * 8;
		break;
	case GB02MAC38:
		pll_val &= ~(1 << (6 + chid)); // pll0
		if (chid < 2)
			val |= GB02MAC914 << (GB02MAC915 + chid * 8);
		else
			val |= GB02MAC914 << (chid - 2) * 8;
		break;
	default:
		pll_val &= ~(1 << (6 + chid)); // 48k pll0
		if (chid < 2)
			val |= GB02MAC912 << (GB02MAC915 + chid * 8);
		else
			val |= GB02MAC912 << (chid - 2) * 8;
		break;
	}

	iowrite32(pll_val, (void *)(pll_va + 0x154));

	if (chid < 2)
		iowrite32(val, (void *)(pll_va + 0x148));
	else
		iowrite32(val, (void *)(pll_va + 0x14c));

	if (ch_count == GB02MAC19) {
		if (sample_dep == GB02MAC41)
			iowrite32(0x13001202,
			(void *)(dp_va + chid * 0x400000 + 0x400));
		else
			iowrite32(0x13001302,
			(void *)(dp_va + chid * 0x400000 + 0x400));
	} else {
		if (sample_dep == GB02MAC41)
			iowrite32(0x1300221e,
			(void *)(dp_va + chid * 0x400000 + 0x400));
		else
			iowrite32(0x1300231e,
			(void *)(dp_va + chid * 0x400000 + 0x400));
	}

	val = ioread32((void *)(dp_va + chid * 0x400000 + 0x0500));
	val &= 0xfffffffc;
	val |= 0x1 << 0;
	val |= 0x1 << 1;
	/* SDP_VERTICAL_CTRL */
	iowrite32(val, (void *)(dp_va + chid * 0x400000 + 0x0500));

	val = ioread32((void *)(dp_va + chid * 0x400000 + 0x0504));
	val &= 0xfffffffc;
	val |= 0x1 << 0;
	val |= 0x1 << 1;
	/* SDP_HORIZONTAL_CTRL */
	iowrite32(val, (void *)(dp_va + chid * 0x400000 + 0x0504));

	iowrite32(0xe08, (void *)(dp_va + chid * 0x400000 + 0x204));
	iowrite32(0x0, (void *)(dp_va + chid * 0x400000 + 0x204));
}

int GB02FUNC532(struct GB02STR86 *i2s_cinfo, int idx)
{
	int ret = DP_DISSUPPORT_AUDIO;

	if ((i2s_cinfo->dp_connect_status[idx] == DP_CONNECT)
	&& (i2s_cinfo->dp_edid_support_audio[idx] == DP_SUPPORT_AUDIO))
		ret = DP_SUPPORT_AUDIO;
	return ret;
}

#ifdef AUDIO_SOUND_SWITCH_DIFF_A
static void GB02FUNC534(struct GB02STR86 *i2s_ctrl, int dp_num)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;

	if (i2s_ctrl->playback_substream[dp_num] == NULL)
		return;
	substream = i2s_ctrl->playback_substream[dp_num];
	runtime = substream->runtime;
	if (runtime == NULL)
		return;
	if (runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
		GB02FUNC360(substream, SNDRV_PCM_TRIGGER_STOP);
//		GB02FUNC326(substream);
//		GB02FUNC306(substream);
		i2s_ctrl->jack_stop_pcm[dp_num] = 1;
	}
}
#endif

static int GB02FUNC536(struct GB02STR86 *i2s_infos)
{
	int i, ret_num = 0;

	for (i = 0; i < GB02MAC309; ++i) {
		if (i2s_infos->dp_connect_status[i] == DP_CONNECT)
			ret_num++;
	}
	return ret_num;
}

static void GB02FUNC539(struct GB02STR86 *i2s_infos,
	int dp_num)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);
	int ret;

	if ((gb_type == PCIE_LPDDR4) || (gb_type == PCIE_C0_200)) {
		if (i2s_infos->jack_init_flag[dp_num] == AUDIO_JACK_INIT) {
			// switch the audio cards, when GVA keeping CONNECT.
			ret = GB02FUNC536(i2s_infos);
			if (ret > 0)
				snd_jack_report(i2s_infos->jack[dp_num], 0);
		}
	}
}

// determine if only the VGA port is connected
static int GB02FUNC542(enum gb_board_type gb_type,
	struct GB02STR86 *i2s_infos)
{
	int ret = -1;

	if ((gb_type != PCIE_LPDDR4) && (gb_type != PCIE_C0_200))
		return ret;
	if ((i2s_infos->dp_connect_status[GB02MAC909] == DP_DISCONNECT)
	&& (i2s_infos->dp_connect_status[GB02MAC910] == DP_CONNECT))
		ret = 0;
	return ret;
}

static void GB02FUNC544(struct GB02STR86 *i2s_infos,
	int dp_num)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);
	int ret;

	if ((gb_type == PCIE_LPDDR4) || (gb_type == PCIE_C0_200)) {
		if (dp_num != GB02MAC910)
			return;
		ret = GB02FUNC542(gb_type, i2s_infos);
		if (ret == 0) {
			if (i2s_infos->jack_init_flag[GB02MAC909] ==
				AUDIO_JACK_INIT)
				snd_jack_report(i2s_infos->jack[GB02MAC909], 0);
			gb_printf(KERN_INFO, "%s-%d: vga = %d, delete jack\n",
				__func__, __LINE__, dp_num);
		}
	}
}

void GB02FUNC547(int dp_num, int pcm_channel,
	int audio_sample, int pcm_width, int hotplug_status)
{
	struct GB02STR86 *i2s_ctrl;

	i2s_ctrl = GB02FUNC505();
	i2s_ctrl->dp_connect_status[dp_num] = hotplug_status;
	if (i2s_ctrl->dp_connect_status[dp_num] == DP_DISCONNECT) {
		GB02FUNC554(dp_num, 0);
		GB02FUNC539(i2s_ctrl, dp_num);
		i2s_ctrl->GB02STR84[dp_num].pcm_channel =
			GB02MAC19;
		i2s_ctrl->GB02STR84[dp_num].audio_sample = GB02MAC31;
		i2s_ctrl->GB02STR84[dp_num].pcm_width = GB02MAC41;
	} else {
		if (i2s_ctrl->dp_edid_support_audio[dp_num] == DP_SUPPORT_AUDIO) {
			if (pcm_channel > GB02MAC25)
				i2s_ctrl->GB02STR84[dp_num].pcm_channel =
					GB02MAC25;
			else if (pcm_channel <= GB02MAC19)
				i2s_ctrl->GB02STR84[dp_num].pcm_channel =
					GB02MAC19;
			else
				i2s_ctrl->GB02STR84[dp_num].pcm_channel =
					pcm_channel / 2 * 2;

			if (audio_sample < GB02MAC28
				|| audio_sample > GB02MAC38)
				i2s_ctrl->GB02STR84[dp_num].audio_sample =
					GB02MAC31;
			else
				i2s_ctrl->GB02STR84[dp_num].audio_sample =
					audio_sample;
			i2s_ctrl->GB02STR84[dp_num].pcm_width = pcm_width;
			gb_printf(KERN_INFO, "dp_num=%d, jack on\n", dp_num);
			if (i2s_ctrl->jack_init_flag[dp_num] ==
				AUDIO_JACK_INIT) {
#ifdef AUDIO_SOUND_SWITCH_DIFF_A
				GB02FUNC534(i2s_ctrl, dp_num);
				if (i2s_ctrl->jack_stop_pcm[dp_num] == 0)
#endif
				snd_jack_report(i2s_ctrl->jack[dp_num],
					SND_JACK_AVOUT);
			}
		} else
			GB02FUNC544(i2s_ctrl, dp_num);
	}
	gb_printf(KERN_INFO, "%s-%d: i = %d, channel=%d, sample=%d,\
		connect_status = %d, pcm_width = %d\n",
		__func__, __LINE__, dp_num,
		i2s_ctrl->GB02STR84[dp_num].pcm_channel,
		i2s_ctrl->GB02STR84[dp_num].audio_sample,
		i2s_ctrl->dp_connect_status[dp_num], pcm_width);
}

void GB02FUNC554(int dp_num, int audio_support_status)
{
	struct GB02STR86 *i2s_ctrl;

	i2s_ctrl = GB02FUNC505();
	i2s_ctrl->dp_edid_support_audio[dp_num] = audio_support_status;
	gb_printf(KERN_INFO, "%s-%d: dp_num = %d, audio_support_status = %d\n",
		__func__, __LINE__, dp_num, audio_support_status);
}

void GB02FUNC556(int dp_num, int restart_val)
{
	struct GB02STR86 *i2s_cinfo = GB02FUNC505();
	struct snd_pcm_substream *substream =
		i2s_cinfo->playback_substream[dp_num];
	struct snd_pcm_runtime *runtime;

	if ((i2s_cinfo == NULL) || (substream == NULL)) {
		gb_printf(KERN_INFO, "%s-%d: dp_num = %d, NULL, so return\n",
			__func__, __LINE__, dp_num);
		return;
	}
	if ((i2s_cinfo->dp_connect_status[dp_num] == DP_DISCONNECT)
	|| (i2s_cinfo->dp_edid_support_audio[dp_num] == DP_DISSUPPORT_AUDIO)) {
		gb_printf(KERN_INFO, "%s-%d: dp_num = %d, return\n",
			__func__, __LINE__, dp_num);
		return;
	}
	runtime = substream->runtime;
	if (runtime == NULL) {
		gb_printf(KERN_INFO, "%s-%d: runtime = NULL, so return\n",
			__func__, __LINE__);
		return;
	}

	if (runtime->status->state == SNDRV_PCM_STATE_RUNNING) // running
		i2s_cinfo->audio_reset_parameters[dp_num] = restart_val;
	gb_printf(KERN_INFO, "%s-%d: audio audio_reset_parameters[%d] = %d\n",
		__func__, __LINE__, dp_num,
		i2s_cinfo->audio_reset_parameters[dp_num]);
}

int GB02FUNC560(int dp_num)
{
	struct GB02STR86 *i2s_cinfo;

	i2s_cinfo = GB02FUNC505();
	return i2s_cinfo->audio_reset_parameters[dp_num];
}

irqreturn_t GB02FUNC561(int irq, void *arg)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	struct GB02STR86 *i2s_cinfo = GB02FUNC505();
	struct GB02STR42 *chip = &i2s_cinfo->dma_info;
	struct GB02STR40 *chan = NULL;
	u32 status = 0, val = 0;
	u64 bus_addr = 0, bus_vaddr = 0;
	int i = 0;

	if (chip->regs == NULL) {
		gb_printf(KERN_ERR, "%s-%d: chip is NULL\n",
			__func__, __LINE__);
		return IRQ_NONE;
	}
	GB02FUNC92(chip);
	status = GB02FUNC170(chip, GB02MAC353);
	if (status) {
		GB02FUNC173(chip, GB02MAC347, status);
		val = GB02FUNC170(chip, GB02MAC349);
		val &= ~status;
		GB02FUNC173(chip, GB02MAC349, val);
	}

	for (i = 0; i < GB02MAC309; i++) {
		chan = &chip->chan[i];
		if (chan->chan_regs == NULL) {
			gb_printf(KERN_ERR, "%s-%d: chan is NULL\n",
				__func__, __LINE__);
			return IRQ_NONE;
		}
		status = GB02FUNC109(chan);
		GB02FUNC106(chan, status);
		if (status & DWAXIDMAC_IRQ_ALL_ERR) {
			gb_printf(KERN_ERR, "%s-%d: err id =%d, status=0x%x\n",
				__func__, __LINE__, i, status);
			return IRQ_NONE;
		} else if (status & DWAXIDMAC_IRQ_DMA_TRF) {
			substream = i2s_cinfo->playback_substream[i];
			if (substream == NULL) {
				gb_printf(KERN_ERR, "%s-%d: substream is NULL\n",
					__func__, __LINE__);
				GB02FUNC97(chip);
				continue;
			}

			runtime = substream->runtime;
			if (runtime == NULL) {
				gb_printf(KERN_ERR, "%s-%d: runtime is NULL\n",
					__func__, __LINE__);
				GB02FUNC97(chip);
				continue;
			}

			GB02FUNC151(chan);
			chip->pos[i] = (chip->pos[i]
				+ runtime->period_size) % runtime->buffer_size;
			val = GB02FUNC415(runtime,
				frames_to_bytes(runtime, chip->pos[i]));
			bus_addr = i2s_cinfo->aoddr_pbase + GB02MAC916 * i
				+ val
				- i2s_cinfo->vphy_base + GB02MAC306;
//			gb_printf(KERN_INFO,"bus_addr = 0x%llx, point = %llu, hw_ptr = %lu, hw_base = %lu\n",
//				bus_addr, chip->pos[i],
//				runtime->status->hw_ptr, runtime->hw_ptr_base);
			/* Solve problem VPU-1438:Begin */
			if (runtime->control->appl_ptr < runtime->status->hw_ptr) {
				bus_vaddr = (u64)i2s_cinfo->ddrwr_va + i2s_cinfo->aoddr_pbase - i2s_cinfo->vphy_base +
							GB02MAC916 * i + frames_to_bytes(runtime, val);
				memset((void *)bus_vaddr, 0, i2s_cinfo->pcm_bytes[i]);
			}
			/* Solve problem VPU-1438:End */
			GB02FUNC97(chip);
			GB02FUNC125(chip, bus_addr, i);

			if (i2s_cinfo->playback_substream[i])
				snd_pcm_period_elapsed(i2s_cinfo->playback_substream[i]);
		}
	}
	return IRQ_HANDLED;
}

