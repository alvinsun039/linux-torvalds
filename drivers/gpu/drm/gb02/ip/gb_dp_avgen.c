/*
 * Copyright (c) 2016 Synopsys, Inc.
 *
 * Synopsys DP TX Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#include "common/gb_common.h"
#include "gb_dp_dptx.h"
#include "common/gb_common.h"
//#include <asm/fpu/api.h>

u8 GB02FUNC389(const u16 data, u8 shift, u8 width)
{
	return ((data >> shift) & ((((u16)1) << width) - 1));
}

u16 GB02FUNC390(u8 bhi, u8 ohi, u8 nhi, u8 blo, u8 olo, u8 nlo)
{
	return (GB02FUNC389(bhi, ohi, nhi) << nlo) |
		GB02FUNC389(blo, olo, nlo);
}

u16 GB02FUNC391(const u8 hi, const u8 lo)
{
	return GB02FUNC390(hi, 0, 8, lo, 0, 8);
}

u32 GB02FUNC392(u8 b3, u8 b2, u8 b1, u8 b0)
{
	u32 retval = 0;

	retval |= b0 << (0 * 8);
	retval |= b1 << (1 * 8);
	retval |= b2 << (2 * 8);
	retval |= b3 << (3 * 8);
	return retval;
}

int GB02FUNC393(struct dptx *dptx, struct dtd *mdtd, u8 data[18])
{
	mdtd->pixel_repetition_input = 0;

	mdtd->pixel_clock = GB02FUNC391(data[1], data[0]);
	if (mdtd->pixel_clock < 0x01)
		return -EINVAL;

	mdtd->h_active = GB02FUNC390(data[4], 4, 4, data[2], 0, 8);
	mdtd->h_blanking = GB02FUNC390(data[4], 0, 4, data[3], 0, 8);
	mdtd->h_sync_offset = GB02FUNC390(data[11], 6, 2, data[8], 0, 8);
	mdtd->h_sync_pulse_width = GB02FUNC390(data[11], 4, 2, data[9],
							0, 8);
	mdtd->h_image_size = GB02FUNC390(data[14], 4, 4, data[12], 0, 8);

	mdtd->v_active = GB02FUNC390(data[7], 4, 4, data[5], 0, 8);
	mdtd->v_blanking = GB02FUNC390(data[7], 0, 4, data[6], 0, 8);
	mdtd->v_sync_offset = GB02FUNC390(data[11], 2, 2, data[10], 4, 4);
	mdtd->v_sync_pulse_width = GB02FUNC390(data[11], 0, 2,
							data[10], 0, 4);
	mdtd->v_image_size = GB02FUNC390(data[14], 0, 4, data[13], 0, 8);
	if (GB02FUNC389(data[17], 4, 1) != 1)
		return -EINVAL;
	if (GB02FUNC389(data[17], 3, 1) != 1)
		return -EINVAL;

	mdtd->interlaced = GB02FUNC389(data[17], 7, 1) == 1;
	mdtd->v_sync_polarity = GB02FUNC389(data[17], 2, 1) == 0;
	mdtd->h_sync_polarity = GB02FUNC389(data[17], 1, 1) == 0;
	if (mdtd->interlaced == 1)
		mdtd->v_active /= 2;
	dptx_dbg(dptx, "DTD pixel_clock: %d interlaced: %d\n",
		 mdtd->pixel_clock, mdtd->interlaced);
	dptx_dbg(dptx, "h_active: %d h_blanking: %d h_sync_offset: %d\n",
		 mdtd->h_active, mdtd->h_blanking, mdtd->h_sync_offset);
	dptx_dbg(dptx, "h_sync_pulse_width: %d h_image_size: %d h_sync_polarity: %d\n",
		 mdtd->h_sync_pulse_width, mdtd->h_image_size,
		 mdtd->h_sync_polarity);
	dptx_dbg(dptx, "v_active: %d v_blanking: %d v_sync_offset: %d\n",
		 mdtd->v_active, mdtd->v_blanking, mdtd->v_sync_offset);
	dptx_dbg(dptx, "v_sync_pulse_width: %d v_image_size: %d v_sync_polarity: %d\n",
		 mdtd->v_sync_pulse_width, mdtd->v_image_size,
		 mdtd->v_sync_polarity);
	mdtd->pixel_clock *= 10;
	return 0;
}

void GB02FUNC399(struct dptx *dptx)
{
	u32 reg;

	reg = dptx_readl(dptx, GB02MAC2103);
	reg |= GB02MAC2315;
	dptx_writel(dptx, GB02MAC2103, reg);

	reg = dptx_readl(dptx, GB02MAC2104);
	reg |= GB02MAC2315;
	dptx_writel(dptx, GB02MAC2104, reg);
}

void GB02FUNC402(struct dptx *dptx)
{
	u32 reg;

	reg = dptx_readl(dptx, GB02MAC2103);
	reg |= GB02MAC2314;
	dptx_writel(dptx, GB02MAC2103, reg);
}

void GB02FUNC404(struct dptx *dptx)
{
	u32 reg;
	u32 audio_infoframe_header = GB02MAC1125;
	u32 audio_infoframe_data[3] = {0x00000710, 0x0, 0x0};
	u8 orig_sample_freq = 0;
	u8 sample_freq = 0;
	struct GB02STR122 *aparams;

	aparams = &dptx->aparams;
	sample_freq = aparams->iec_samp_freq;
	orig_sample_freq = aparams->iec_orig_samp_freq;

	if (orig_sample_freq == 12 && sample_freq == 3)
		audio_infoframe_data[0] = 0x00000710;
	else if (orig_sample_freq == 15 && sample_freq == 0)
		audio_infoframe_data[0] = 0x00000B10;
	else if (orig_sample_freq == 13 && sample_freq == 2)
		audio_infoframe_data[0] = 0x00000F10;
	else if (orig_sample_freq == 7 && sample_freq == 8)
		audio_infoframe_data[0] = 0x00001310;
	else if (orig_sample_freq == 5 && sample_freq == 10)
		audio_infoframe_data[0] = 0x00001710;
	else if (orig_sample_freq == 3 && sample_freq == 12)
		audio_infoframe_data[0] = 0x00001B10;
	else
		audio_infoframe_data[0] = 0x00001F10;

	audio_infoframe_data[0] |= (aparams->num_channels - 1);
	if (aparams->num_channels == 3)
		audio_infoframe_data[0] |= 0x02000000;
	else if (aparams->num_channels == 4)
		audio_infoframe_data[0] |= 0x03000000;
	else if (aparams->num_channels == 5)
		audio_infoframe_data[0] |= 0x07000000;
	else if (aparams->num_channels == 6)
		audio_infoframe_data[0] |= 0x0b000000;
	else if (aparams->num_channels == 7)
		audio_infoframe_data[0] |= 0x0f000000;
	else if (aparams->num_channels == 8)
		audio_infoframe_data[0] |= 0x13000000;

	//dev_info(dptx->dev, "audio_infoframe_data[0] before = %x\n", audio_infoframe_data[0]);
	switch (aparams->data_width) {
	case 16:
		dev_dbg(dptx->dev, "%s: data_width = 16\n", __func__);
		audio_infoframe_data[0] &= ~GENMASK(9,8);
		audio_infoframe_data[0] |= 1 << 8;
		break;
	case 20:
		dev_dbg(dptx->dev, "%s: data_width = 20\n", __func__);
		audio_infoframe_data[0] &= ~GENMASK(9,8);
		audio_infoframe_data[0] |= 2 << 8;
		break;
	case 24:
		dev_dbg(dptx->dev, "%s: data_width = 24\n", __func__);
		audio_infoframe_data[0] &= ~GENMASK(9,8);
		audio_infoframe_data[0] |= 3 << 8;
		break;
	default:
		dev_dbg(dptx->dev, "%s: data_width not found\n", __func__);
		break;
	}

	//dev_info(dptx->dev, "audio_infoframe_data[0] after = %x\n", audio_infoframe_data[0]);

	dptx->sdp_list[0].payload[0] = audio_infoframe_header;
	dptx_writel(dptx, GB02MAC2106, audio_infoframe_header);
	dptx_writel(dptx, GB02MAC2106 + 4, audio_infoframe_data[0]);
	dptx_writel(dptx, GB02MAC2106 + 8, audio_infoframe_data[1]);
	dptx_writel(dptx, GB02MAC2106 + 12, audio_infoframe_data[2]);
	reg = dptx_readl(dptx, GB02MAC2103);
	reg |= GB02MAC2316;
	dptx_writel(dptx, GB02MAC2103, reg);
}

void GB02FUNC418(struct dptx *dptx, u32 *payload)
{
	int i;

	for (i = 0; i < GB02MAC1326; i++)
		if (!memcmp(dptx->sdp_list[i].payload, payload,
				sizeof(dptx->sdp_list[i].payload)))
			memset(dptx->sdp_list[i].payload, 0,
				sizeof(dptx->sdp_list[i].payload));
}

void GB02FUNC422(struct dptx *dptx, struct GB02STR147 *data)
{
	int i;
	u32 reg;
	int reg_num;
	u32 header;
	int sdp_offset;

	reg_num = 0;
	header = cpu_to_be32(data->payload[0]);
	for (i = 0; i < GB02MAC1326; i++)
		if (dptx->sdp_list[i].payload[0] == 0) {
			dptx->sdp_list[i].payload[0] = header;
			sdp_offset = i * GB02MAC1328;
			reg_num = 0;
			while (reg_num < GB02MAC1327) {
				dptx_writel(dptx, GB02MAC2106 + sdp_offset
					    + reg_num * 4,
					    cpu_to_be32(
							data->payload[reg_num])
					    );
				reg_num++;
			}
			switch (data->blanking) {
			case 0:
				reg = dptx_readl(dptx, GB02MAC2103);
				reg |= (1 << (2 + i));
				dptx_writel(dptx, GB02MAC2103, reg);
				break;
			case 1:
				reg = dptx_readl(dptx,
						 GB02MAC2104);
				reg |= (1 << (2 + i));
				dptx_writel(dptx, GB02MAC2104,
					    reg);
				break;
			case 2:
				reg = dptx_readl(dptx, GB02MAC2103);
				reg |= (1 << (2 + i));
				dptx_writel(dptx, GB02MAC2103, reg);
				reg = dptx_readl(dptx,
						 GB02MAC2104);
				reg |= (1 << (2 + i));
				dptx_writel(dptx, GB02MAC2104,
					    reg);
				break;
			}
			break;
		}
}

void GB02FUNC431(struct dptx *dptx, struct GB02STR147 *data)
{
	if (data->en == 1)
		GB02FUNC422(dptx, data);
	else
		GB02FUNC418(dptx, data->payload);
}

void GB02FUNC434(struct dptx *dptx, u8 enable)
{
        struct GB02STR147 vsc_data;
        int i;

	struct GB02STR124 *vparams;

	vparams = &dptx->vparams;

        vsc_data.en = enable;
        for(i=0 ; i<9 ; i++) {
             if(i == 0)       vsc_data.payload[i] = 0x00070513;
             else if(i == 5)
	       switch(vparams->bpc) {
	        case COLOR_DEPTH_8:
	             vsc_data.payload[i] = 0x30010000;
		     break;
	        case COLOR_DEPTH_10:
		     vsc_data.payload[i] = 0x30020000;
		     break;
	        case COLOR_DEPTH_12:
		     vsc_data.payload[i] = 0x30030000;
		     break;
	        case COLOR_DEPTH_16:
		     vsc_data.payload[i] = 0x30040000;
		     break;
		}
             else      vsc_data.payload[i] = 0x0;
        }
        vsc_data.blanking = 0;
        vsc_data.cont = 1;

        GB02FUNC431(dptx, &vsc_data);
}

void GB02FUNC442(struct dptx *dptx, int ch_num, int enable)
{
	u32 reg = 0;
	u32 data_en = 0;

	reg = dptx_readl(dptx, GB02MAC2095);
	reg &= ~DPTX_AUD_CONFIG1_DATA_EN_IN_MASK;

	if (enable) {
		switch (ch_num) {
		case 1:
			data_en = GB02MAC2317;
			break;
		case 2:
			data_en = GB02MAC2318;
			break;
		case 3:
			data_en = GB02MAC2319;
			break;
		case 4:
			data_en = GB02MAC2320;
			break;
		case 5:
			data_en = GB02MAC2321;
			break;
		case 6:
			data_en = GB02MAC2322;
			break;
		case 7:
			data_en = GB02MAC2323;
			break;
		case 8:
			data_en = GB02MAC2324;
			break;
		}
		reg |= data_en << GB02MAC2265;
	} else {
		switch (ch_num) {
		case 1:
			data_en = ~GB02MAC2317;
			break;
		case 2:
			data_en = ~GB02MAC2318;
			break;
		case 3:
			data_en = ~GB02MAC2319;
			break;
		case 4:
			data_en = ~GB02MAC2320;
			break;
		case 5:
			data_en = ~GB02MAC2321;
			break;
		case 6:
			data_en = ~GB02MAC2322;
			break;
		case 7:
			data_en = ~GB02MAC2323;
			break;
		case 8:
			data_en = ~GB02MAC2324;
			break;
		}
		reg &= data_en << GB02MAC2265;
	}
	dptx_writel(dptx, GB02MAC2095, reg);
}

void GB02FUNC454(struct dptx *dptx, int enable, int stream)
{
	u32 reg;

	dev_dbg(dptx->dev, "%s \n", __func__);
	reg = dptx_readl(dptx, GB02MAC2059);

	if (enable)
		reg |= GB02MAC2170(stream);
	else
		reg &= ~(GB02MAC2170(stream));

	dptx_writel(dptx, GB02MAC2059, reg);
}

void GB02FUNC458(struct dptx *dptx)
{
	u32 reg = 0;
	struct GB02STR122 *aparams;

	aparams = &dptx->aparams;
	reg = dptx_readl(dptx, GB02MAC2095);

	if (aparams->mute == 1)
		reg |= GB02MAC2325;
	else
		reg &= ~GB02MAC2325;
	dptx_writel(dptx, GB02MAC2095, reg);
}

void GB02FUNC461(struct dptx *dptx)
{
	GB02FUNC463(dptx);
//	GB02FUNC487(dptx);
	GB02FUNC399(dptx);
	GB02FUNC402(dptx);

	GB02FUNC404(dptx);
}

void GB02FUNC463(struct dptx *dptx)
{
	struct GB02STR122 *aparams;
	u32 reg;

	aparams = &dptx->aparams;

	GB02FUNC466(dptx);
	GB02FUNC470(dptx);
	GB02FUNC475(dptx);

	reg = dptx_readl(dptx, GB02MAC2095);
	reg &= ~DPTX_AUD_CONFIG1_ATS_VER_MASK;
	reg |= aparams->ats_ver << GB02MAC2266;
	dptx_writel(dptx, GB02MAC2095, reg);

	GB02FUNC442(dptx, aparams->num_channels, 1);
}

void GB02FUNC466(struct dptx *dptx)
{
	u32 reg = 0;
	struct GB02STR122 *aparams;

	aparams = &dptx->aparams;

	reg = dptx_readl(dptx, GB02MAC2095);
	reg &= ~GB02MAC2262;
	reg |= aparams->inf_type << GB02MAC2261;
	dptx_writel(dptx, GB02MAC2095, reg);
}

void GB02FUNC470(struct dptx *dptx)
{
	u32 reg = 0;
	u32 num_ch_map;
	struct GB02STR122 *aparams;

	aparams = &dptx->aparams;

	reg = dptx_readl(dptx, GB02MAC2095);
	reg &= ~DPTX_AUD_CONFIG1_NCH_MASK;

	if (aparams->num_channels == 1)
		num_ch_map = 0;
	else if (aparams->num_channels == 2)
		num_ch_map = 1;
	else
		num_ch_map = aparams->num_channels - 1;

	reg |= num_ch_map << GB02MAC2263;
	dptx_writel(dptx, GB02MAC2095, reg);
}

void GB02FUNC475(struct dptx *dptx)
{
	u32 reg = 0;
	struct GB02STR122 *aparams;
	u8 i2s_word_width = 0;

	aparams = &dptx->aparams;

	reg = dptx_readl(dptx, GB02MAC2095);
	reg &= ~DPTX_AUD_CONFIG1_DATA_WIDTH_MASK;
	reg |= aparams->data_width << GB02MAC2264;
	dptx_writel(dptx, GB02MAC2095, reg);

	switch (aparams->data_width) {
	case 16:
		i2s_word_width = 0;
		break;
	case 17:
		i2s_word_width = 1;
		break;
	case 18:
		i2s_word_width = 2;
		break;
	case 19:
		i2s_word_width = 3;
		break;
	case 20:
		i2s_word_width = 4;
		break;
	case 21:
		i2s_word_width = 5;
		break;
	case 22:
		i2s_word_width = 6;
		break;
	case 23:
		i2s_word_width = 7;
		break;
	case 24:
		i2s_word_width = 8;
		break;
	}
	reg = dptx_readl(dptx, GB02MAC2097);
	reg &= ~DPTX_AG_CONFIG1_WORD_WIDTH_MASK;
	reg |= i2s_word_width << GB02MAC2253;
	dptx_writel(dptx, GB02MAC2097, reg);
}

void GB02FUNC483(struct dptx *dptx)
{
	u32 reg;
	struct GB02STR122 *aparams;

	aparams = &dptx->aparams;
	reg = dptx_readl(dptx, GB02MAC2100);
	reg &= ~DPTX_AG_CONFIG4_SAMP_FREQ_MASK;
	reg |= aparams->iec_samp_freq << GB02MAC2258;
	reg &= ~DPTX_AG_CONFIG4_ORIG_SAMP_FREQ_MASK;
	reg |= aparams->iec_orig_samp_freq <<
		GB02MAC2260;
	dptx_writel(dptx, GB02MAC2100, reg);
}

void GB02FUNC487(struct dptx *dptx)
{
	u32 reg = 0;
	struct GB02STR122 *aparams;

	aparams = &dptx->aparams;

	/* AG_CONFIG1 */
	reg = dptx_readl(dptx, GB02MAC2097);
	reg &= ~GB02MAC2255;
	reg |= aparams->use_lut << GB02MAC2254;
	dptx_writel(dptx, GB02MAC2097, reg);
	GB02FUNC475(dptx);

	/* AG_CONFIG3 */
	reg = dptx_readl(dptx, GB02MAC2099);
	reg &= ~DPTX_AG_CONFIG3_CH_NUMCL0_MASK;
	reg |= aparams->iec_channel_numcl0 << GB02MAC2256;
	reg &= ~DPTX_AG_CONFIG3_CH_NUMCR0_MASK;
	reg |= aparams->iec_channel_numcr0  << GB02MAC2257;
	dptx_writel(dptx, GB02MAC2099, reg);

	/* AG_CONFIG4 */

	GB02FUNC483(dptx);
	reg = dptx_readl(dptx, GB02MAC2100);
	reg &= ~DPTX_AG_CONFIG4_WORD_LENGTH_MASK;
	reg |= aparams->iec_word_length  << GB02MAC2259;
	dptx_writel(dptx, GB02MAC2100, reg);

	/* AG_CONFIG5 */
	reg = dptx_readl(dptx, GB02MAC2101);
	reg &= ~DPTX_AG_CONFIG3_CH_NUMCL0_MASK;
	/*GB02MAC2256;*/
	reg |= aparams->iec_channel_numcl0 << 0;
	reg &= ~DPTX_AG_CONFIG3_CH_NUMCR0_MASK;
	/*GB02MAC2257;*/
	reg |= aparams->iec_channel_numcr0 << 4;
	dptx_writel(dptx, GB02MAC2101, reg);
}

/*
 * Video Generation
 */

void GB02FUNC492(struct dptx *dptx, int stream)
{
	dev_dbg(dptx->dev, "%s \n", __func__);
	GB02FUNC788(dptx, stream);
	if(dptx->dsc)
		GB02FUNC659(dptx);
	/*
	int i = 0;
	for (i = 0; i < 32; i++)
	{
		u32 pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, i));
		dev_err(dptx->dev, "DSC_ PPS[%d] = %x\n", i, pps);

	}
	*/
	GB02FUNC663(dptx, stream);
	GB02FUNC699(dptx, stream);
	GB02FUNC789(dptx, stream);
	GB02FUNC500(dptx, stream);
	dptx_writel(dptx, GB02MAC2089(stream), 0);
}

int GB02FUNC493(struct dptx *dptx, u8 vmode, int stream)
{
	int retval;
	struct GB02STR124 *vparams;
	struct dtd mdtd;

	vparams = &dptx->vparams;
	if (!GB02FUNC811(&mdtd, vmode, vparams->refresh_rate,
			   vparams->video_format)) {
		dptx_dbg(dptx, "%s: Invalid video mode value %d\n",
			 __func__, vmode);
		return -EINVAL;
	}
	retval = GB02FUNC682(dptx, dptx->link.lanes,
					 dptx->link.rate, vparams->bpc,
					 vparams->pix_enc, mdtd.pixel_clock);
	if (retval)
		return retval;
	vparams->mdtd = mdtd;
	vparams->mode = vmode;

	/* MMCM */
	GB02FUNC454(dptx, 1, stream);
	retval = GB02FUNC1171(dptx, mdtd.pixel_clock, stream);
	if (retval) {
		GB02FUNC454(dptx, 0, stream);
		return retval;
	}
	GB02FUNC454(dptx, 0, stream);
	GB02FUNC492(dptx, stream);
	dptx_dbg(dptx, "%s: Change video mode to %d\n",
		 __func__, vmode);

	return retval;
}

int GB02FUNC496(struct dptx *dptx, int stream)
{
	struct GB02STR124 *vparams;
	struct dtd *mdtd;
    int i = 0;

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;

	dev_dbg(dptx->dev, "%s \n", __func__);

	if (!GB02FUNC811(mdtd, vparams->mode,
			   vparams->refresh_rate, vparams->video_format))
		return -EINVAL;

	if(dptx->dsc) {
		GB02FUNC659(dptx);
        for (i = 0; i < 32; i++)
		{
			u32 pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, i));
			dev_err(dptx->dev, "DSC_ PPS[%d] = %x\n", i, pps);
		}
	}
	GB02FUNC663(dptx, stream);
	//GB02FUNC500(dptx, stream);

	return 0;
}

void GB02FUNC500(struct dptx *dptx, int stream)
{
	u32 reg = 0;
	struct GB02STR124 *vparams;
	struct dtd *mdtd;
	u8 vmode;

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;
	vmode = vparams->mode;

	reg |= GB02MAC2302;

	/* Configure video polarity   */
	if (mdtd->h_sync_polarity == 1)
		reg |= GB02MAC2303;

	if (mdtd->v_sync_polarity == 1)
		reg |= GB02MAC2305;

	/* Configure Interlaced or Prograssive video  */
	if (mdtd->interlaced == 1)
		reg |= GB02MAC2306;

	/* Configure BLANK  */

	if (vparams->video_format == VCEA) {
		if (vmode == 5 || vmode == 6 || vmode == 7 ||
		    vmode == 10 || vmode == 11 || vmode == 20 ||
		    vmode == 21 || vmode == 22 || vmode == 39 ||
		    vmode == 25 || vmode == 26 || vmode == 40 ||
		    vmode == 44 || vmode == 45 || vmode == 46 ||
		    vmode == 50 || vmode == 51 || vmode == 54 ||
		    vmode == 55 || vmode == 58 || vmode == 59)
			reg |= GB02MAC2307;
	}

	/* Single, dual, or quad pixel */
	reg &= ~DPTX_VG_CONFIG1_MULTI_PIXEL_MASK;
	reg |= dptx->multipixel << GB02MAC2298;
	dptx_writel(dptx, GB02MAC2084(stream), reg);

	GB02FUNC504(dptx, stream);
	GB02FUNC719(dptx, stream);
	GB02FUNC781(dptx, stream);

	/* Configure video_gen2 register */

         reg = 0;
        if(vparams->pix_enc == YCBCR420) {
             reg |= (mdtd->h_active/2) << GB02MAC2312;
             reg |= (mdtd->h_blanking/2) << GB02MAC2313;
        } else {
             reg |= mdtd->h_active << GB02MAC2312;
             reg |= mdtd->h_blanking << GB02MAC2313;
        }

	dptx_writel(dptx, GB02MAC2085(stream), reg);

	/* Configure video_gen3 register */
	reg = 0;
	reg |= mdtd->h_sync_offset << GB02MAC2286;
	reg |= mdtd->h_sync_pulse_width << GB02MAC2287;
	dptx_writel(dptx, GB02MAC2086(stream), reg);

	/* Configure video_gen4 register */
	reg = 0;
	reg |= mdtd->v_active << GB02MAC2285;
	reg |= mdtd->v_blanking << GB02MAC2284;
	dptx_writel(dptx, GB02MAC2087(stream), reg);

	/* Configure video_gen5 register */
	reg = 0;
	reg |= mdtd->v_sync_offset << GB02MAC2288;
	reg |= mdtd->v_sync_pulse_width << GB02MAC2289;
	dptx_writel(dptx, GB02MAC2088(stream), reg);
}

void GB02FUNC504(struct dptx *dptx, int stream)
{
	u32 reg = 0;
	enum pixel_enc_type pix_enc;
	struct GB02STR124 *vparams;

	vparams = &dptx->vparams;
	pix_enc = vparams->pix_enc;
	reg = dptx_readl(dptx, GB02MAC2084(stream));

	if (pix_enc == YCBCR420 || pix_enc  == YCBCR422) {
		reg |= GB02MAC2309;
		if (pix_enc == YCBCR420) {
		//	reg |= GB02MAC2310; //sahakyan
			reg &= ~GB02MAC2308;
		} else if (pix_enc == YCBCR422) {
			reg |= GB02MAC2308;
			reg &= ~GB02MAC2310;
		}
	} else {
		reg &= ~GB02MAC2309;
		reg &= ~GB02MAC2310;
		reg &= ~GB02MAC2308;
	}
	dptx_writel(dptx, GB02MAC2084(stream), reg);
}

int GB02FUNC509(struct dptx* dptx)
{
	struct GB02STR124 *vparams;
	int pixel_clk;
	u16 h_blank;
	u32 link_clk;
	u8 rate;
	int hblank_interval;

	vparams = &dptx->vparams;
	pixel_clk = vparams->mdtd.pixel_clock;
	h_blank = vparams->mdtd.h_blanking;
	rate = dptx->link.rate;

	switch (rate) {
	case GB02MAC2185:
		link_clk = 40500;
		break;
	case GB02MAC2186:
		link_clk = 67500;
		break;
	case GB02MAC2187:
		link_clk = 135000;
		break;
	case GB02MAC2188:
		link_clk = 202500;
		break;
	case GB02MAC2189:
		link_clk = 54000;
		break;
	case GB02MAC2190:
		link_clk = 60750;
		break;
	case GB02MAC2191:
		link_clk = 81000;
		break;
	case GB02MAC2192:
		link_clk = 108000;
		break;
	default:
		dptx_err(dptx, "Invalid rate 0x%x\n", rate);
		return -EINVAL;
	}

	hblank_interval = h_blank * link_clk / pixel_clk;

	return hblank_interval;
}

int GB02FUNC520(struct dptx *dptx)
{
	int retval;
	u8 dsc_color_format;

	dev_dbg(dptx->dev, "%s \n", __func__);

	retval = GB02FUNC1151(dptx, DP_DSC_DEC_COLOR_FORMAT_CAP,
				&dsc_color_format);
	if (retval) {
		dev_err(dptx->dev,"%s : DPCD read failed\n", __func__);
		//return retval;
	}

	if(dsc_color_format & DP_DSC_RGB)
		dptx_dbg(dptx, "%s: Sink supports RGB color format\n", __func__);

	if (dsc_color_format & DP_DSC_YCbCr444)
		dptx_dbg(dptx, "%s: Sink supports YCbCr 4:4:4 color format\n", __func__);

	if (dsc_color_format & DP_DSC_YCbCr422_Simple)
		dptx_dbg(dptx, "%s: Sink supports YCbCr 4:2:2 SIMPLE color format\n", __func__);

	if (dsc_color_format & DP_DSC_YCbCr422_Native)
		dptx_dbg(dptx, "%s: Sink supports YCbCr 4:2:2 NATIVE color format\n", __func__);

	if (dsc_color_format & DP_DSC_YCbCr420_Native)
		dptx_dbg(dptx, "%s: Sink supports YCbCr 4:2:0 NATIVE color format\n", __func__);

	return 0;
}

int GB02FUNC523(struct dptx *dptx)
{
	int retval;
	u8 dsc_color_depth;

	dev_dbg(dptx->dev, "%s \n", __func__);

	retval = GB02FUNC1151(dptx, DP_DSC_DEC_COLOR_DEPTH_CAP,
				&dsc_color_depth);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
		//return retval;
	}

	if (dsc_color_depth & DP_DSC_8_BPC)
		dptx_dbg(dptx, "%s: Sink supports 8 bpc\n", __func__);

	if (dsc_color_depth & DP_DSC_10_BPC)
		dptx_dbg(dptx, "%s: Sink supports 10 bpc\n", __func__);

	if (dsc_color_depth & DP_DSC_12_BPC)
		dptx_dbg(dptx, "%s: Sink supports 12 bpc\n", __func__);

	return 0;
}

int GB02FUNC527(int ppr)
{
	gb_printf(KERN_INFO, "%s \n", __func__);

	if (ppr <= 340)
		return 1;

	if (ppr > 340 && ppr <= 680)
		return 2;

	if (ppr > 680 && ppr <= 1360)
		return 4;

	if (ppr > 1360 && ppr <= 3200)
		return 8;

	if (ppr > 3200 && ppr <= 4800)
		return 12;

	if (ppr > 4800 && ppr <= 6400)
		return 16;

	if (ppr > 6400 && ppr <= 8000)
		return 20;

	if (ppr > 8000 && ppr <= 9600)
		return 24;

	return -EINVAL;
}

bool GB02FUNC531(struct dptx* dptx, int slice_count)
{
	u8 slice_cap1;
	u8 slice_cap2;
	int retval = 0;

	dev_dbg(dptx->dev, "%s \n", __func__);

	retval = GB02FUNC1151(dptx, DP_DSC_SLICE_CAP_1, &slice_cap1);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed 1\n", __func__);
	}
	retval = GB02FUNC1151(dptx, DP_DSC_SLICE_CAP_2, &slice_cap2);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed 2\n", __func__);
	}

	if ((slice_cap1 & DP_DSC_1_PER_DP_DSC_SINK) && (slice_count == 1))
		return true;

	if ((slice_cap1 & DP_DSC_2_PER_DP_DSC_SINK) && (slice_count == 2))
		return true;

	if ((slice_cap1 & DP_DSC_4_PER_DP_DSC_SINK) && (slice_count == 4))
		return true;

	if ((slice_cap1 & DP_DSC_6_PER_DP_DSC_SINK) && (slice_count == 6))
		return true;

	if ((slice_cap1 & DP_DSC_8_PER_DP_DSC_SINK) && (slice_count == 8))
		return true;

	if ((slice_cap1 & DP_DSC_10_PER_DP_DSC_SINK) && (slice_count == 10))
		return true;

	if ((slice_cap1 & DP_DSC_12_PER_DP_DSC_SINK) && (slice_count == 12))
		return true;

	if ((slice_cap2 & DP_DSC_16_PER_DP_DSC_SINK) && (slice_count == 16))
		return true;

	if ((slice_cap2 & DP_DSC_20_PER_DP_DSC_SINK) && (slice_count == 20))
		return true;

	if ((slice_cap2 & DP_DSC_24_PER_DP_DSC_SINK) && (slice_count == 24))
		return true;

	return false;
}

int  GB02FUNC540(struct dptx* dptx, int slice_count)
{
	dev_dbg(dptx->dev, "%s \n", __func__);

	if (slice_count == 24)
		return 1;

	if (slice_count == 1)
		slice_count = 2;
	else
		if (slice_count == 2)
			slice_count = 4;
		else
			slice_count += 4;

	return slice_count;
}

/*
 *  Get the slice width based on slice count, which is supported by the sink
 */
u16 GB02FUNC543(struct dptx* dptx, int* slice_count)
{
	struct GB02STR124 *vparams;
	u16 h_active;
	u8 max_slice_width;
	u16 slice_width;
	int ppr;
	int retval = 0;

	dev_dbg(dptx->dev, "%s \n", __func__);

	vparams = &dptx->vparams;
	ppr = vparams->mdtd.pixel_clock;
	h_active = vparams->mdtd.h_active;

	//TODO max_slice_width is u8, should be u16
	retval = GB02FUNC1151(dptx, DP_DSC_MAX_SLICE_WIDTH, &max_slice_width);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
	}

	// Initialize slice width
	slice_width = 4096;

	if (h_active <= 4096)
		slice_width = 2048;

	if (slice_width > max_slice_width)
		slice_width = max_slice_width;

	// Adjust slice count and recalculate again, if slice width > max slice width
	slice_width = (h_active / (*slice_count));
	//while (slice_width > max_slice_width) {
	//	*slice_count = GB02FUNC540(dptx, *slice_count);
	//	slice_width = (h_active / *slice_count);
	//}

	return slice_width;
}

int GB02FUNC548(struct dptx *dptx)
{
	struct GB02STR124 *vparams;
	int retval;
	int ppr;
//	u16 chunk_size;

	vparams = &dptx->vparams;
	ppr = vparams->mdtd.pixel_clock;

	dev_dbg(dptx->dev, "%s \n", __func__);
	retval = GB02FUNC520(dptx);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
	}


	retval = GB02FUNC523(dptx);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
	}

	gb_printf(KERN_INFO, "%s ppr = %d\n",__func__, ppr);
	// Get the initial slice count based on ppr
//	vparams->encoders = GB02FUNC527(ppr/1000); //sahakyan
	vparams->encoders = 2;
	if (vparams->encoders < 0) {
		dptx_err(dptx, "Could not determine slice count based on PPR!!!\n");
		//return vparams->encoders;
	}

	dev_dbg(dptx->dev, "Initial slice count based on PPR is %d\n", vparams->encoders);

	vparams->slice_width = GB02FUNC543(dptx, &vparams->encoders);
	vparams->chunk_size = vparams->slice_width;

	dev_dbg(dptx->dev, "Slice count after calculating supported slice width is %d\n", vparams->encoders);

	//// Check the slice count is supported or not
	//while (!GB02FUNC531(dptx, vparams->encoders)) {
	//	vparams->encoders = GB02FUNC540(dptx, vparams->encoders);
	//	vparams->slice_width = GB02FUNC543(dptx, &vparams->encoders);
	//}

	dev_dbg(dptx->dev, "%s: Slice count = %d, slice width = %d, vparams->chunk_size = %d\n",
		__func__, vparams->encoders, vparams->slice_width, vparams->chunk_size);

	return vparams->encoders;
}

void GB02FUNC553(struct dptx* dptx)
{
	u32 reg;
	u8 pixels_per_pixelclk;
	int encoders = dptx->vparams.encoders;

	dev_dbg(dptx->dev, "%s \n", __func__);

	// Determine sampled pixel count based on pixel clock
	switch (dptx->multipixel)
	{
	case GB02MAC2044:
		pixels_per_pixelclk = 1;
		break;
	case GB02MAC2045:
		pixels_per_pixelclk = 2;
		break;
	case GB02MAC2046:
		pixels_per_pixelclk = 4;
		break;
	default:
		break;
	}
	pixels_per_pixelclk = 2;
	// Program DSC_CTL.STREAMn_ENC_CLK_DIVIDED bit
	if (encoders > pixels_per_pixelclk) {
		// Divide pixel clock for DSC encoder
		reg = dptx_readl(dptx, GB02MAC2356);
		reg |= 1 << GB02MAC2358;
		dptx_writel(dptx, GB02MAC2356, reg);
	}
}

void GB02FUNC559(struct dptx *dptx)
{
	int encoders = dptx->vparams.encoders;
	u32 reg;
	u8 stream = 0; //TODO SST only

	dev_dbg(dptx->dev, "%s \n", __func__);

	// Change pixel mode based on encoders count
	switch (encoders)
	{
	case 8:
		dptx->multipixel = GB02MAC2046;
		break;
	case 4:
		dptx->multipixel = GB02MAC2045;
		//dptx->multipixel = GB02MAC2046;
		break;
	default:
		break;
	}

	/* Single, dual, or quad pixel */
	reg = dptx_readl(dptx, GB02MAC2066(stream));
	reg &= ~DPTX_VSAMPLE_CTRL_MULTI_PIXEL_MASK;
	reg |= dptx->multipixel << GB02MAC2268;
	dptx_writel(dptx, GB02MAC2066(stream), reg);

	/* Divide pixel clock, if needed */
	GB02FUNC553(dptx);
}

int GB02FUNC562(struct dptx *dptx)
{
	u8 available_encoders;
	u8 dsc_hwcfg;
	u32 dsc_ctl;
	int encoders;
//	int i;

	dev_dbg(dptx->dev, "%s \n", __func__);

	encoders = dptx->vparams.encoders;
	dsc_hwcfg = dptx_readl(dptx, GB02MAC2355);
	dsc_ctl = dptx_readl(dptx, GB02MAC2356);
	available_encoders = dsc_hwcfg & DPTX_DSC_NUM_ENC_MSK;

	dsc_ctl = dptx_readl(dptx, GB02MAC2356);  //sahakyan
	dsc_ctl |= 1<<22;
	dptx_writel(dptx, GB02MAC2356, dsc_ctl);

	dev_dbg(dptx->dev, "Calculated encoder count = %d\n", encoders);
	dev_dbg(dptx->dev, "Available encoders count = %d\n", available_encoders);

	//RAZ TODO check this case
	if (encoders > available_encoders) {
		dev_err(dptx->dev, "Encoder count is greather than available encoders\n");
		return -EINVAL;
	}

	//// Write stream number (0 for SST mode) for encoders to use
	//for (i = 1; i <= encoders; ++i) {
	//	dsc_ctl &= ~(1 << GB02MAC2359(i - 1));
	//}

	return 0;
}

void GB02FUNC566(struct dptx* dptx)
{
	u8 dsc_rev;
	u8 dsc_rev_min;
	u8 dsc_rev_maj;
	u32 pps;
	int retval = 0;

	retval = GB02FUNC1151(dptx, DP_DSC_REV, &dsc_rev);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
	}

	/*dsc_rev_min = dsc_rev & DP_DSC_MINOR_MASK;
	dsc_rev_maj = (dsc_rev & DP_DSC_MAJOR_MASK) >> DP_DSC_MAJOR_SHIFT;*/

	/* Hardcode, DPCD not present */
	dsc_rev_min = 2;
	dsc_rev_maj = 1 >> DP_DSC_MAJOR_SHIFT;

	dev_dbg(dptx->dev, "%s  PPS dsc_rev = %d\n", __func__, dsc_rev);
	dev_dbg(dptx->dev, "%s  PPS dsc_rev_min = %d  dsc_rev_maj = %d\n", __func__, dsc_rev_min, dsc_rev_maj);

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 0));
	pps |= dsc_rev_min;
	pps |= (dsc_rev_maj << GB02MAC2363);
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 0),pps);
}

void GB02FUNC569(struct dptx* dptx)
{
	u8 buf_bit_depth;
	u8 pps_buf_bit_depth;
	u32 pps;
	int retval = 0;

	retval = GB02FUNC1151(dptx, DP_DSC_LINE_BUF_BIT_DEPTH, &buf_bit_depth);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
	}
	pps_buf_bit_depth = (buf_bit_depth & DP_DSC_LINE_BUF_BIT_DEPTH_MASK);

    dev_dbg(dptx->dev, "%s PPS pps_buf_bit_depth = %d\n", __func__, pps_buf_bit_depth);
	/* Hardcode, DPCD not present */
	pps_buf_bit_depth = 0xE;

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 0));
	pps |= (pps_buf_bit_depth << GB02MAC2364);
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 0), pps);
}

void GB02FUNC571(struct dptx* dptx)
{
	u8 block_pred;
	u32 pps;
	int retval = 0;

	retval = GB02FUNC1151(dptx, DP_DSC_BLK_PREDICTION_SUPPORT, &block_pred);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
	}

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 1));
	//pps |= (block_pred & 1) << GB02MAC2365;
	// Hardcode, DPCD not present
	//pps |= (1 & 1) << GB02MAC2365;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 1), pps);

	dev_dbg(dptx->dev, "%s PPS block_pred = %d \n", __func__, block_pred);
}

static u8 GB02FUNC573(struct dptx* dptx)
{

    u8 bpc;
    u8 port_cap[4];
    int retval = 0;

    retval = GB02FUNC1154(dptx, DP_DOWNSTREAM_PORT_0,
					   port_cap,
					   sizeof(port_cap));
	if (retval)
		return retval;

    bpc = port_cap[2] & DP_DS_MAX_BPC_MASK;

	gb_printf(KERN_ERR, "Sahakyan bpc = %x\n", bpc);
		switch (bpc) {
		case DP_DS_8BPC:
			return 8;
		case DP_DS_10BPC:
			return 10;
		case DP_DS_12BPC:
			return 12;
		case DP_DS_16BPC:
			return 16;
		default:
	    	return 0;
	}
}

//static u16 dptx_calculate_bpp(struct dptx* dptx)
//{
//    u8 bpc;
//    u8 bpp_high, bpp_low;
//    u16 bpp;
//    int retval = 0;


//    retval = GB02FUNC1151(dptx, DP_DSC_MAX_BITS_PER_PIXEL_LOW, &bpp_low);
//	if (retval)
//		return retval;

//    retval = GB02FUNC1151(dptx, DP_DSC_MAX_BITS_PER_PIXEL_HI, &bpp_high);
//	if (retval)
//		return retval;

//    bpp =  (bpp_low << 8) & bpp_high;

//    if(!bpp) {
//        bpc = GB02FUNC573(dptx);
//        if(bpc > 0)
//            bpp = 3*bpc;
//    }
//
//    return bpp;
//
//}

void GB02FUNC576(struct dptx* dptx)
{
	struct GB02STR124 *vparams;
	enum pixel_enc_type;// pix_enc;
	u8 bpp_high, bpp_low;
	u16 bpp;
	u32 pps;
	u32 bpp1;

	vparams = &dptx->vparams;
	bpp1=128;

	bpp = GB02MAC2373; // dptx_calculate_bpp(dptx);
	dptx->vparams.dsc_bpp = bpp;

	/* Get high and low parts of bpp (10 bits) */
	bpp_high = (bpp & DPTX_DSC_BPP_HIGH_MASK) >> 8;
	bpp_low = 128 & DPTX_DSC_BPP_LOW_MASK;

	gb_printf(KERN_ERR, "RAZ bpp_low = %x\n",bpp_low);
	gb_printf(KERN_ERR, "RAZ bpp_high = %x\n", bpp_high);

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 1));
	pps |= bpp_high;
	pps |= bpp_low <<8 ;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 1), pps);

	dev_dbg(dptx->dev, "%s PPS  bpp = %d\n", __func__, bpp);
}

void GB02FUNC579(struct dptx* dptx)
{
	struct GB02STR124 *vparams;
	enum pixel_enc_type pix_enc;
	u8 bpc;
	u32 pps;


	vparams = &dptx->vparams;
	vparams->dsc_bpc =  GB02FUNC573(dptx);
	bpc = vparams->dsc_bpc;
	pix_enc = vparams->pix_enc;

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 0));
	switch (bpc)
	{
	case COLOR_DEPTH_8:
		pps |= bpc << GB02MAC2366;
		break;
	case COLOR_DEPTH_10:
		pps |= bpc << GB02MAC2366;
		break;
	case COLOR_DEPTH_12:
		pps |= bpc << GB02MAC2366;
		break;
	case COLOR_DEPTH_16:
		pps |= bpc << GB02MAC2366;
		break;
	default:
		dev_err(dptx->dev, "Unsupported Color depth by DSC spec\n");
		break;
	}

	dev_dbg(dptx->dev, "%s PPS bpc = %d\n", __func__, bpc);
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 0), pps);

	switch (pix_enc)
	{
	case RGB:
		pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 1));
		pps |= 1 << 4;
		dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 1), pps);
		break;
	case YCBCR420:
		pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 23));
		pps |= 1 << 1;
		dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 23), pps);
		break;
	case YCBCR422:
		// TODO check with bala
		break;
	default:
		break;
	}
}

void GB02FUNC586(struct dptx* dptx)
{
	u32 pps;
	struct GB02STR124 *vparams;

	dev_dbg(dptx->dev, "%s \n", __func__);
	vparams = &dptx->vparams;

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 3));
	pps |= dptx->vparams.slice_width >> 8;
	pps |= (dptx->vparams.slice_width & GENMASK(7, 0)) << 8;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 3), pps);

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 3));
	pps |= (dptx->vparams.chunk_size >> 8) << 16;
	pps |= (dptx->vparams.chunk_size & GENMASK(7, 0)) << 24;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 3), pps);

	dev_dbg(dptx->dev, "%s PPS slice_width = %d\n", __func__, dptx->vparams.slice_width);
	dev_dbg(dptx->dev, "%s PPS chunk_size = %d\n", __func__, dptx->vparams.chunk_size);
	dev_dbg(dptx->dev, "%s PPS vbr_enable = %d\n", __func__, 0);

	//vbr_enable = 0
}


void GB02FUNC592(struct dptx* dptx)
{
	struct GB02STR124 *vparams;
	u32 pps;
	u32 pic_width;
	u32 pic_height;

	vparams = &dptx->vparams;
	pic_width = vparams->mdtd.h_active;
	pic_height = vparams->mdtd.v_active;

	gb_printf(KERN_INFO, "RAZ pic_height=%d\n", pic_height);
	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 2));

        pps = 0;

	pps |= (pic_width >> 8);
	pps |= (pic_width & GENMASK(7,0)) << 8;
	gb_printf(KERN_INFO, "RAZ pps=0x%0x -pic_width", pps);
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 2), pps);


	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 1));

	pps &= ~GENMASK(31,16);
	pps |= (pic_height >> 8) << 16;

	gb_printf(KERN_INFO, "RAZ pps=0x%0x -pic_height", pps);

	pps |= (pic_height & GENMASK(7,0)) << 24;

	gb_printf(KERN_INFO, "RAZ pps=0x%0x -pic_height", pps);

	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 1), pps);

	dev_dbg(dptx->dev, "%s PPS pic_width = %d\n", __func__, pic_width);
	dev_dbg(dptx->dev, "%s PPS pic_height = %d\n", __func__, pic_height);
}


void GB02FUNC596(struct dptx* dptx)
{
	struct GB02STR124 *vparams;
//	u32 pps;
	u16 pic_height;
	u16 slice_height;
	u16 dsc_max_num_lines;
	u32 reg;
	u8 first_line_bpg_offset;
	u8 second_line_bpg_offset;

	dev_dbg(dptx->dev, "%s \n", __func__);

	reg = dptx_readl(dptx, GB02MAC2367);
	dsc_max_num_lines = (reg & DSC_MAX_NUM_LINES_MASK) >> GB02MAC2368;

	dev_dbg(dptx->dev, "%s dsc_max_num_lines = %d\n", __func__, dsc_max_num_lines);

	vparams = &dptx->vparams;
	pic_height = vparams->mdtd.v_active;

	if(pic_height < dsc_max_num_lines)
		slice_height = pic_height;
	else {
		slice_height = pic_height;
		while (!(slice_height < dsc_max_num_lines))
		{
			slice_height = pic_height >> 2; // divide to 2
		}
	}

	dptx->vparams.slice_height = slice_height;
	dev_dbg(dptx->dev, "%s PPS slice_height = %d\n", __func__, slice_height);
	reg = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 2));
	reg |= (slice_height >> 8) << 16;
	reg |= (slice_height & GENMASK(7, 0)) << 24;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 2), reg);


	//Calculate first_line_bpg_offset based on slice height
	if (slice_height >= 8)
		first_line_bpg_offset = 12 +  ((9 * min(34, slice_height - 8)) / 100);
		//first_line_bpg_offset = 12 + min(34, slice_height - 8);
	else
		first_line_bpg_offset = 2 * (slice_height - 1);

	reg = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 6));
	reg |= first_line_bpg_offset << 24;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 6), reg);

	dev_dbg(dptx->dev, "%s PPS first_line_bpg_offset = %d\n", __func__, first_line_bpg_offset);
	gb_printf(KERN_ERR, "RAZ first_line_bpg_offset = %d\n", first_line_bpg_offset);

	//Calculate second_line_bpg_offset based on slice height only in YcBcR420
	if ((dptx->vparams.pix_enc == YCBCR420) &&
		(slice_height < 8))
		second_line_bpg_offset = 2 * (slice_height - 1);
	else
		// Generic value for second_line_bpg_offset
		second_line_bpg_offset = 12;

	reg = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 22));
	reg |= second_line_bpg_offset << 8;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 22), reg);

	dptx->vparams.first_line_bpg_offset = first_line_bpg_offset;
}

u8 GB02FUNC601(struct dptx* dptx)
{
	u8 groups_per_line;
	u16 slice_width;
	enum pixel_enc_type pix_enc;
	struct GB02STR124 *vparams;

	vparams = &dptx->vparams;
	slice_width = dptx->vparams.slice_width;
	pix_enc = vparams->pix_enc;

	gb_printf(KERN_ERR, "%s: RAZ slice_width = %d ",__func__, slice_width);
	if (pix_enc == RGB)
		groups_per_line = slice_width / GB02MAC1335;
	else
		groups_per_line = (slice_width >> 1) / GB02MAC1335;

	gb_printf(KERN_ERR, "%s: RAZ groups_per_line = %d ", __func__, groups_per_line);

	return groups_per_line;
}

void GB02FUNC604(struct dptx* dptx)
{
	struct GB02STR124 *vparams;
	enum pixel_enc_type pix_enc;
	u32 minRateBufferSize = 0, pps;
	u8 group_pl;
	u8 rc_buf_size;
	u8 rc_buf_block_size;
	u8 first_line_bpg_offset;
	u8 bpc;
	u16 bpp;
	u16 rc_model_size = GB02MAC1330;
	u16 initial_offset = GB02MAC1332;
	u16 initial_delay = GB02MAC1333;
	int retval = 0;

	dev_dbg(dptx->dev, "%s \n", __func__);

	vparams = &dptx->vparams;
	bpc = vparams->dsc_bpc;
	bpp = vparams->dsc_bpp;
	pix_enc = vparams->pix_enc;
	first_line_bpg_offset = dptx->vparams.first_line_bpg_offset;
	group_pl = GB02FUNC601(dptx);

	retval = GB02FUNC1151(dptx, DP_DSC_RC_BUF_SIZE, &rc_buf_size);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
	}

	retval = GB02FUNC1151(dptx, DP_DSC_RC_BUF_BLK_SIZE, &rc_buf_block_size);
	if (retval) {
		dev_err(dptx->dev, "%s : DPCD read failed\n", __func__);
	}

	switch (pix_enc)
	{
	case RGB:
		minRateBufferSize = (rc_model_size - initial_offset) +
				    (initial_delay * bpp) +
				    (group_pl * first_line_bpg_offset);
		gb_printf(KERN_ERR, "RAZ minRateBufferSize=%d\n", minRateBufferSize);
		dptx->vparams.minRateBufferSize = minRateBufferSize;
		break;
	case YCBCR420:
		//TODO
		break;
	case YCBCR422:
		//TODO
		break;
	default:
		break;
	}


	//if (minRateBufferSize > rc_buf_size) {
	//	// TODO reduce silce height by half and continue from 5.12 step 8
	//	dev_err(dptx->dev, " Min rate buf size is greater than sinks rc buf size\n");
	//}
	//else {

		dev_dbg(dptx->dev, " PPS DSC minRateBufferSize is %d\n", minRateBufferSize);
		dev_dbg(dptx->dev, " PPS DSC rc_model_size is %d\n", rc_model_size);
		dev_dbg(dptx->dev, " PPS DSC initial_offset is %d\n", initial_offset);
		dev_dbg(dptx->dev, " PPS DSC initial_delay is %d\n", initial_delay);

		// Program rc_model_size pps
		pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 9));
		pps |= (rc_model_size >> 8) << 16;
		pps |= (rc_model_size & GENMASK(7,0)) << 24;
		dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 9), pps);

		// Program initial_offset pps
		pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 8));
		pps |= ((initial_offset & GENMASK(15,8))>> 8);
		pps |= (initial_offset & GENMASK(7, 0)) << 8;
		dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 8), pps);

		// Program initial_delay pps
		gb_printf(KERN_ERR, "initial_delay = %d\n", initial_delay);
		pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 4));
		pps |= ((initial_delay & GENMASK(9, 8)) >> 8);
		pps |= (initial_delay & GENMASK(7, 0)) << 8;
		dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 4), pps);
	//}
}

u8 GB02FUNC609(struct dptx* dptx)
{
	u8 bpc;
	u8 muxWordSize = 0;

	bpc = dptx->vparams.dsc_bpc;

	switch (bpc)
	{
	case 12:
	case 14:
	case 16:
		muxWordSize = 64;
		break;
	case 8:
	case 10:
		muxWordSize = 48;
		break;
	default:
		dev_err(dptx->dev, "Cant get muxWordSize based on bpc\n");
		break;
	}

	return muxWordSize;
}

void GB02FUNC614(struct dptx* dptx)
{
	struct GB02STR124 *vparams;
	u32 minRateBufferSize;
	u8 muxWordSize, maxSeSize_Y, maxSeSize_C;
	u16 bpp;
	u8 bpc;
	u8 group_per_line;
	u32 pps;
	u32 hrddelay;
	u32 initial_dec_delay;
	u32 slice_bpg_offset;
	u16 initial_scale_value;
	u16 rc_model_size = GB02MAC1330;
	u16 initial_offset = GB02MAC1332;
	u32 scale_decrement_interval;
	u32 scale_increment_interval;
	u32 numExtraMuxBits;
	int rcxformoffset;
	int final_offset;
	int final_scale_value;
	int nfl_bpg_offset;
	int groupsTotal;

	dev_dbg(dptx->dev, "%s \n", __func__);

	vparams = &dptx->vparams;
	minRateBufferSize = dptx->vparams.minRateBufferSize;
	bpp = dptx->vparams.dsc_bpp;
	bpc = dptx->vparams.dsc_bpc;

 	hrddelay = minRateBufferSize / bpp;

	dev_dbg(dptx->dev, "%s PPS hrddelay = %d\n", __func__, hrddelay);
	gb_printf(KERN_ERR, "RAZ hrddelay = %d\n", hrddelay);
	dptx->vparams.hrddelay = hrddelay;

	// Calculate initial_dec_delay and program PPS
	initial_dec_delay = hrddelay - GB02MAC1333;
	gb_printf(KERN_ERR, "RAZ initial_dec_delay = %d\n", initial_dec_delay);
	//initial_dec_delay = 658;
	dptx->vparams.initial_dec_delay = initial_dec_delay;

	dev_dbg(dptx->dev, "%s PPS initial_dec_delay = %d\n", __func__, initial_dec_delay);

	gb_printf(KERN_ERR, "RAZ initial_dec_delay = %d\n", initial_dec_delay);
	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 4));
	gb_printf(KERN_ERR, "RAZ high part = %x\n", (initial_dec_delay >> 8));
	gb_printf(KERN_ERR, "RAZ low part = %lx\n", initial_dec_delay & GENMASK(7, 0));
	pps |= (initial_dec_delay >> 8) << 16;
	pps |= (initial_dec_delay & GENMASK(7, 0)) << 24;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 4), pps); //Bala changed 4 to 5

	// Calculate initial_scale_value and program PPS
	initial_scale_value = (rc_model_size / (rc_model_size - initial_offset));
	gb_printf(KERN_ERR, "RAZ initial_scale_value = %d (%x)\n", initial_scale_value, initial_scale_value);
	dptx->vparams.initial_scale_value = initial_scale_value;
	dev_dbg(dptx->dev, "%s PPS initial_scale_value = %d\n", __func__, initial_scale_value);

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 5));
	pps |= (initial_scale_value << GB02MAC1368)<< 8;
	//pps |= initial_scale_value ;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 5), pps); //Bala

 	//Calculate scale_decrement_interval and program PPS
	group_per_line = GB02FUNC601(dptx);

	gb_printf(KERN_ERR, "RAZ  group_per_line =%d\n", group_per_line);
	gb_printf(KERN_ERR, "RAZ  initial_scale_value =%d\n", initial_scale_value);


	scale_decrement_interval = group_per_line / (8 * (initial_scale_value - 1));
	dev_dbg(dptx->dev, "%s PPS scale_decrement_interval = %d\n", __func__, scale_decrement_interval);
	dptx->vparams.scale_decrement_interval = scale_decrement_interval;
	gb_printf(KERN_ERR, "RAZ  scale_decrement_interval =%d\n", scale_decrement_interval);

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 6));
	pps |= (scale_decrement_interval & GENMASK(11,9)) >> 9;
	pps |= (scale_decrement_interval & GENMASK(7, 0)) << 8;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 6), pps);

	//Calculate scale_increment_interval and program PPS
	muxWordSize = GB02FUNC609(dptx);
	rcxformoffset = GB02MAC1332 - GB02MAC1330;
	gb_printf(KERN_ERR, "RAZ first_line_bpg_offset=%d\n", dptx->vparams.first_line_bpg_offset);
	gb_printf(KERN_ERR, "RAZ slice_height=%d\n", dptx->vparams.slice_height);

	nfl_bpg_offset = (dptx->vparams.first_line_bpg_offset << GB02MAC1370)/ (dptx->vparams.slice_height - 1);
	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 7));
	pps |= ((nfl_bpg_offset & GENMASK(15, 8)) >> 8);
	pps |= ((nfl_bpg_offset & GENMASK(7, 0))) << 8;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 7), pps);
	gb_printf(KERN_ERR, "RAZ nfl_bpg_offset=%d\n", nfl_bpg_offset);

	if (bpc == 16) {
		maxSeSize_Y = maxSeSize_C = 64;
	}
	else {
		maxSeSize_Y = bpc * 4 + 4;
		maxSeSize_C = (bpc + 1) * 4; //1 is convert_rgb
	}

	gb_printf(KERN_ERR, "RAZ muxWordSize=%d\n", muxWordSize);
	gb_printf(KERN_ERR, "RAZ maxSeSize_Y=%d\n", maxSeSize_Y);
	gb_printf(KERN_ERR, "RAZ maxSeSize_C=%d\n", maxSeSize_C);

	numExtraMuxBits = (muxWordSize + maxSeSize_Y - 2) + 2 * (muxWordSize + maxSeSize_C - 2);
	gb_printf(KERN_ERR, "RAZ numExtraMuxBits = %d \n", numExtraMuxBits);
	final_offset = GB02MAC1330 - GB02MAC1333 * bpp + numExtraMuxBits;

	gb_printf(KERN_ERR, "RAZ  final_offset = %d\n", final_offset);
	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 8));
	pps |= ((final_offset & GENMASK(15, 8)) >> 8) <<16;
	pps |= ((final_offset & GENMASK(7, 0))) << 24;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 8), pps);

	final_scale_value = GB02MAC1330 / (GB02MAC1330 - final_offset);
	gb_printf(KERN_ERR, "RAZ  final_scale_value = %d\n", final_scale_value);
	groupsTotal = group_per_line * dptx->vparams.slice_height;
	gb_printf(KERN_ERR, "RAZ groupsTotal=%d\n", groupsTotal);
	slice_bpg_offset = ((1 << GB02MAC1370) *(GB02MAC1330 - GB02MAC1332 + numExtraMuxBits) / groupsTotal);
	gb_printf(KERN_ERR, "RAZ slice_bpg_offset=%d\n", slice_bpg_offset);

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 7));
	pps |= ((slice_bpg_offset & GENMASK(15, 8)) >> 8) << 16;
	pps |= ((slice_bpg_offset & GENMASK(7, 0))) << 24;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 7), pps);

	scale_increment_interval = (final_offset / nfl_bpg_offset + slice_bpg_offset) / (8 * (final_scale_value -1));
	//gb_printf(KERN_ERR, "RAZ scale_increment_interval = %d\n", scale_increment_interval);
	scale_increment_interval = 11428;

	dev_dbg(dptx->dev,"%s PPS scale_increment_interval = %d\n", __func__, scale_increment_interval);

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 5));
	pps |= (scale_increment_interval >> 8) << 16;
	pps |= ((scale_increment_interval & GENMASK(7, 0))) << 24;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 5), pps); //Bala

	gb_printf(KERN_ERR, "RAZ scale_increment_interval = %x\n", scale_increment_interval);
	if (scale_increment_interval > 65535)
		dev_err(dptx->dev, "%s Scale increment interval exceeds 65535\n", __func__); // TODO check with Bala
}

void GB02FUNC627(struct dptx* dptx)
{
	u32 pps;

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 9));
	pps |= GB02MAC1351;
	pps |= GB02MAC1353 << 8;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 9), pps);
}

/*
 *  Returns 2's complement representation of negative number
 *  Number should be 5 bits in this function
 *  Number passed as integer, for being able have negative values
 */
u8 GB02FUNC629(int number)
{
	u8 res;

	if (number >= 0)
		return number;

	number = -number;

	// Calculate 2's complement representation
	res = (number ^ 0x1f) + 1;

	// Set the sign bit before returning
	return (res | (1 << 5));
}

static struct GB02STR141 rc_range_params[15] = {
       {0, 4, 2},
       {0, 4, 0},
       {1, 5, 0},
       {1, 6, -2},
       {3, 7, -4},
       {3, 7, -6},
       {3, 7, -8},
       {3, 8, -8},
       {3, 9, -8},
       {3, 10, -10},
       {5, 10, -10},
       {5, 11, -12},
       {5, 11, -12},
       {9, 12, -12},
       {12, 13, -12},
};


void GB02FUNC636(struct dptx* dptx)
{
	u32 pps;
	int i;
	int pps_index;

	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, (14)));
	pps |= ((rc_range_params[0].maxQP & GENMASK(4, 2)) >> 2) << 16;
	pps |= rc_range_params[0].minQP << 19;
	pps |= GB02FUNC629(rc_range_params[0].offset) << 24;
	pps |= (rc_range_params[0].maxQP & GENMASK(1, 0)) << 29;
	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 14), pps);

	// Read once 4 byte and program in there 2  rc_range_parameters
	for (i = 1, pps_index = 60; i < 15; i+=2, pps_index+=4)
	{
		pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, (pps_index / 4)));
		pps |= ((rc_range_params[i].maxQP & GENMASK(4, 2)) >> 2);
		pps |= (rc_range_params[i].minQP) << 3;
		pps |= GB02FUNC629(rc_range_params[i].offset) << 8;
		pps |= (rc_range_params[i].maxQP & GENMASK(1, 0)) << 14 ;

		pps |= ((rc_range_params[i+1].maxQP & GENMASK(4, 2)) >> 2) << 16;
		pps |= (rc_range_params[i+1].minQP) << 19;
		pps |= GB02FUNC629(rc_range_params[i+1].offset) << 24;
		pps |= (rc_range_params[i+1].maxQP & GENMASK(1, 0)) << 30;

		dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, pps_index / 4), pps);
	}
}

static u32 RC_BUF_THRESHOLD[14] = {
       896,1792,2688,3584,4480,5376,6272,6720,7168,7616,7744,7872,8000,8064,
};

void GB02FUNC642(struct dptx* dptx)
{
	u32 pps;
	int i;
	int pps_index;

	// rc_edge_factor
	pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, 10));
	pps |= GB02MAC1355;

	// rc_quant_incr_limit0
	pps |= GB02MAC1357 << 8;

	// rc_quant_incr_limit1
	pps |= GB02MAC1359 << 16;

	// rc_tgt_offset_hi
	pps |= GB02MAC1362 << 28;

	// rc_tgt_offset_lo
	pps |= GB02MAC1365 << 24;

	dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 10), pps);

	// PPS44 - PPS57 rc_buf_threshold values
	for (i = 0, pps_index = 44; i < 14; ++i, ++pps_index)
	{
		pps = dptx_readl(dptx, DPTX_DSC_PPS(GB02MAC2361, (pps_index / 4)));
		gb_printf(KERN_ERR, "RAZ %d = %x\n", RC_BUF_THRESHOLD[i], RC_BUF_THRESHOLD[i] >> 6);
		pps |= (RC_BUF_THRESHOLD[i] >> 6) << (pps_index % 4) * 8;
		dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, pps_index / 4), pps);
	}

	// RC_RANGE_PARAMETERS from DSC 1.2 spec
	GB02FUNC636(dptx);
}

void GB02FUNC645(struct dptx* dptx)
{
	dev_dbg(dptx->dev, "%s \n", __func__);

	GB02FUNC566(dptx);
	GB02FUNC569(dptx);
	GB02FUNC571(dptx);
	GB02FUNC576(dptx);
	GB02FUNC579(dptx);
	GB02FUNC586(dptx);
	GB02FUNC592(dptx);
	GB02FUNC596(dptx);
	GB02FUNC604(dptx);
	GB02FUNC614(dptx);
	GB02FUNC627(dptx);
	GB02FUNC642(dptx);

	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 0), 0x8e000012);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 1), 0xe0018010);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 2), 0xe0018002);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 3), 0x80028002);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 4), 0x92020002);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 5), 0xa42c2000);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 6), 0x0f000800);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 7), 0x2e004100);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 8), 0xf0100018);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 9), 0x00200c03);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 10), 0x330b0b06);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 11), 0x382a1c0e);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 12), 0x69625446);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 13), 0x7b797770);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 14), 0x02017e7d);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 15), 0x40090001);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 16), 0xfc19be09);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 17), 0xf819fa19);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 18), 0x781a381a);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 19), 0x62ab61a);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 20), 0xf42af42a);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 21), 0x7463344b);
	//dptx_writel(dptx, DPTX_DSC_PPS(GB02MAC2361, 22), 0x00000000);
}

void GB02FUNC649(struct dptx* dptx)
{
	int encoder_delay;
	u32 mux_word_size;
	u32 muxer_initial_delay;
	u32 reg;
	u16 h_active;
	u16 h_blanking;
	int lsteer_xmit_delay;
	u8 horizontal_slices;
	u8 vertical_slices;
	u8 bpc;
	u8 multipixel;

	bpc = dptx->vparams.dsc_bpc;
	mux_word_size = (bpc < 12) ? 48 : 64;
	muxer_initial_delay = (mux_word_size + (4 * bpc + 4) - 3 + 32) * 3;
	horizontal_slices = dptx->vparams.encoders;
	multipixel = dptx->multipixel;
	h_active = dptx->vparams.mdtd.h_active;
	h_blanking = dptx->vparams.mdtd.h_blanking;
	vertical_slices = horizontal_slices; // TODO check with Bala

	encoder_delay = (((GB02MAC1338 + GB02MAC1349 +
			   GB02MAC1341 + muxer_initial_delay +
		           GB02MAC1344) * horizontal_slices)) /
		           (1 << multipixel);

	lsteer_xmit_delay = encoder_delay + (((h_blanking + h_active) * vertical_slices) +
		h_blanking + h_active +  (GB02MAC1333 * horizontal_slices)) / (1 << multipixel);

	dev_dbg(dptx->dev, "%s muxer inital delay  = %d\n", __func__, muxer_initial_delay);
	dev_dbg(dptx->dev, "%s DSC encoder delay = %d\n", __func__, encoder_delay);
	dev_dbg(dptx->dev, "%s DSC XMIT delay = %d\n", __func__, lsteer_xmit_delay);

//	switch (multipixel)
//	{
//	case GB02MAC2045:
//		lsteer_xmit_delay /= 2;
//		break;
//	case GB02MAC2046:
//		lsteer_xmit_delay /= 4;
//		break;
//	default:
//		break;
//	}
//
	//lsteer_xmit_delay = 2492;
	lsteer_xmit_delay = 1692;
	dev_dbg(dptx->dev, "%s DSC XMIT delay = %d\n", __func__, lsteer_xmit_delay);

	reg = dptx_readl(dptx, GB02MAC2369);
	reg &=~DPTX_DSC_LSTEER_XMIT_DELAY_MASK;
	reg &= ~DPTX_DSC_LSTEER_FRAC_SHIFT_MASK;
	reg |= lsteer_xmit_delay << GB02MAC2372;
	dev_dbg(dptx->dev, "%s DSCCFG is  = %x\n", __func__, reg);
	dptx_writel(dptx, GB02MAC2369, reg);
}

void GB02FUNC654(struct dptx* dptx)
{
	u32 reg;
	int encoders;
	u16 bpp;
	u32 wait_cnt_int;
	u32 wait_cnt_frac;
	s64 fixp;

	bpp = dptx->vparams.dsc_bpp;
	encoders = dptx->vparams.encoders;

	// Get the integer part
	fixp = drm_fixp_from_fraction(128, (bpp * encoders));
	wait_cnt_int = drm_fixp2int(fixp);
	//wait_cnt_int = 16;

	// Get the fractional part
	fixp &= DRM_FIXED_DECIMAL_MASK;
	fixp *= 64;
	wait_cnt_frac = drm_fixp2int(fixp);
	wait_cnt_frac = 0;

	dev_dbg(dptx->dev, "%s wait_cnt_int = %u, wait_cnt_frac = %u\n",
		__func__, wait_cnt_int, wait_cnt_frac);

	reg = dptx_readl(dptx, GB02MAC2369);
	reg &=~DPTX_DSC_LSTEER_INT_SHIFT_MASK;
	reg |= wait_cnt_int << GB02MAC2370;
	reg |= wait_cnt_frac << GB02MAC2371;
	reg &=~DPTX_DSC_LSTEER_XMIT_DELAY_MASK;
	dptx_writel(dptx, GB02MAC2369, reg);

	GB02FUNC649(dptx);

	/* Program pps repeat count */
	//reg = dptx_readl(dptx, GB02MAC2356);
	//reg |= 3 << 16;
	//dptx_writel(dptx, GB02MAC2356, reg);

	/* Enable compression */
	reg = dptx_readl(dptx, GB02MAC2066(0));
	reg |= GB02MAC2269;
	//dptx_writel(dptx, GB02MAC2066(0), reg); BB

}

void GB02FUNC659(struct dptx* dptx)
{
	int encoders = 0;

	dev_dbg(dptx->dev, "%s\n",__func__);
	encoders = GB02FUNC548(dptx);
	GB02FUNC562(dptx);

	// Apply soft reset - stream 0 for SST mode
	GB02FUNC1059(dptx, GB02MAC2170(0));

	GB02FUNC559(dptx);
	GB02FUNC645(dptx);
	GB02FUNC654(dptx);


}

void GB02FUNC663(struct dptx *dptx, int stream)
{
	u32 reg = 0;
	u8 vmode;

	struct GB02STR124 *vparams;
	struct dtd *mdtd;

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;
	vmode = vparams->mode;

	GB02FUNC708(dptx, stream);

	/* Single, dual, or quad pixel */
	reg = dptx_readl(dptx, GB02MAC2066(stream));
	reg &= ~DPTX_VSAMPLE_CTRL_MULTI_PIXEL_MASK;
	reg |= dptx->multipixel << GB02MAC2268;
	dptx_writel(dptx, GB02MAC2066(stream), reg);

	/* Configure DPTX_VSAMPLE_POLARITY_CTRL register */
	reg = 0;

	if (mdtd->h_sync_polarity == 1)
		reg |= GB02MAC2278;
	if (mdtd->v_sync_polarity == 1)
		reg |= GB02MAC2277;

	dptx_writel(dptx, GB02MAC2072(stream), reg);

	reg = 0;

	/* Configure video_config1 register */
	if (vparams->video_format == VCEA) {
		if (vmode == 5 || vmode == 6 || vmode == 7 ||
		    vmode == 10 || vmode == 11 || vmode == 20 ||
		    vmode == 21 || vmode == 22 || vmode == 39 ||
		    vmode == 25 || vmode == 26 || vmode == 40 ||
		    vmode == 44 || vmode == 45 || vmode == 46 ||
		    vmode == 50 || vmode == 51 || vmode == 54 ||
		    vmode == 55 || vmode == 58 || vmode  == 59)
			reg |= GB02MAC2280;
	}

	if (mdtd->interlaced == 1)
		reg |= GB02MAC2281;

    reg |= mdtd->h_active << GB02MAC2283;
    reg |= mdtd->h_blanking << GB02MAC2282;
	dptx_writel(dptx, GB02MAC2074(stream), reg);

	/* Configure video_config2 register */
	reg = 0;
	reg |= mdtd->v_active << GB02MAC2285;
	reg |= mdtd->v_blanking << GB02MAC2284;
	dptx_writel(dptx, GB02MAC2076(stream), reg);

	/* Configure video_config3 register */
	reg = 0;
        reg |= mdtd->h_sync_offset << GB02MAC2286;
        reg |= mdtd->h_sync_pulse_width << GB02MAC2287;
	dptx_writel(dptx, GB02MAC2078(stream), reg);

	/* Configure video_config4 register */
	reg = 0;
	reg |= mdtd->v_sync_offset << GB02MAC2288;
	reg |= mdtd->v_sync_pulse_width << GB02MAC2289;
	dptx_writel(dptx, GB02MAC2079(stream), reg);

	/* Configure video_config5 register */
	GB02FUNC699(dptx, stream);

	/* Configure video_msa1 register */
	reg = 0;
	reg |= (mdtd->h_blanking - mdtd->h_sync_offset)
		<< GB02MAC2290;
	reg |= (mdtd->v_blanking - mdtd->v_sync_offset)
		<< GB02MAC2291;
	dptx_writel(dptx, GB02MAC2081(stream), reg);

	GB02FUNC725(dptx, stream);

////
//       reg = 0x10104;
//       dptx_writel(dptx, 0x330,reg);
////
	reg = GB02FUNC509(dptx);
	reg |= (GB02MAC2094 << GB02MAC2093);
	dptx_writel(dptx, GB02MAC2091, reg);
}

u8 GB02FUNC675(struct dptx* dptx)
{
	int link_pixel_clock_ratio;
	int pixle_push_rate;
	int lanes;
	int tu;
	int slot_count;
	int fec_slot_count;
	int link_clk;
	int pixel_clk;
	u16 dsc_bpp;
	u8 rate;

	tu = dptx->vparams.aver_bytes_per_tu;
	lanes = dptx->link.lanes;
	dsc_bpp = dptx->vparams.dsc_bpp;


    if(dptx->fec) {
        if(lanes == 1)
            fec_slot_count = 13;
        else
            fec_slot_count = 7;
    } else {
        fec_slot_count = 0;
    }

	pixle_push_rate = (8 / dsc_bpp) * lanes;
	if(tu > 0)
        slot_count = tu + 1 + fec_slot_count;
    else
        slot_count = tu + fec_slot_count;

    slot_count = ROUND_UP_TO_NEAREST(slot_count, 4);

	pixel_clk = dptx->vparams.mdtd.pixel_clock;

	rate = dptx->link.rate;

	switch (rate) {
	case GB02MAC2185:
		link_clk = 40500;
		break;
	case GB02MAC2186:
		link_clk = 67500;
		break;
	case GB02MAC2187:
		link_clk = 135000;
		break;
	case GB02MAC2188:
		link_clk = 202500;
		break;
	case GB02MAC2189:
		link_clk = 54000;
		break;
	case GB02MAC2190:
		link_clk = 60750;
		break;
	case GB02MAC2191:
		link_clk = 81000;
		break;
	case GB02MAC2192:
		link_clk = 108000;
		break;
	default:
		dptx_err(dptx, "Invalid rate 0x%x\n", rate);
		return -EINVAL;
	}

	link_pixel_clock_ratio = link_clk / pixel_clk;

	return (pixle_push_rate * link_pixel_clock_ratio * slot_count);
}

int GB02FUNC682(struct dptx *dptx, int lane_num, int rate,
			    int bpc, int encoding, int pixel_clock)
{
	struct GB02STR124 *vparams;
	struct dtd *mdtd;
	int link_rate;
	int link_clk;
	int retval = 0;
	int ts;
	int T1;
	int T2;
	int tu;
	int tu_frac;
	int color_dep;

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;
	if (dptx->dsc) {
		dptx_err(dptx, "%s %d we have no dsc!return\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	if (dptx->multipixel != GB02MAC2044) {
		dptx_err(dptx, "%s %d we have no multipixel!return\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	if (dptx->mst) {
		dptx_err(dptx, "%s %d we have no mst!return\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	switch (rate) {
		case GB02MAC2185:
			link_rate = 162;
			link_clk = 40500;
			break;
		case GB02MAC2186:
			link_rate = 270;
			link_clk = 67500;
			break;
		case GB02MAC2187:
			link_rate = 540;
			link_clk = 135000;
			break;
		case GB02MAC2188:
			link_rate = 810;
			link_clk = 202500;
			break;
		case GB02MAC2189:
			link_rate = 216;
			link_clk = 54000;
			break;
		case GB02MAC2190:
			link_rate = 243;
			link_clk = 60750;
			break;
		case GB02MAC2191:
			link_rate = 324;
			link_clk = 81000;
			break;
		case GB02MAC2192:
			link_rate = 432;
			link_clk = 108000;
			break;
		default:
	        dptx_dbg(dptx, "Invalid rate param = %d\n" , rate);
	        return -EINVAL;
	        break;
	}

	switch (bpc) {
		case COLOR_DEPTH_6:
			color_dep = 18;
			break;
		case COLOR_DEPTH_8:
			if (encoding == YCBCR420)
				color_dep  = 12;
			else if (encoding == YCBCR422)
				color_dep = 16;
			else if (encoding == YONLY)
				color_dep = 8;
			else
				color_dep = 24;
			break;
		case COLOR_DEPTH_10:
			if (encoding == YCBCR420)
				color_dep = 15;
			else if (encoding == YCBCR422)
				color_dep = 20;
			else if (encoding  == YONLY)
				color_dep = 10;
			else
				color_dep = 30;
			break;

		case COLOR_DEPTH_12:
			if (encoding == YCBCR420)
				color_dep = 18;
			else if (encoding == YCBCR422)
				color_dep = 24;
			else if (encoding == YONLY)
				color_dep = 12;
			else
				color_dep = 36;
			break;

		case COLOR_DEPTH_16:
			if (encoding == YCBCR420)
				color_dep = 24;
			else if (encoding == YCBCR422)
				color_dep = 32;
			else if (encoding == YONLY)
				color_dep = 16;
			else
				color_dep = 48;
			break;

		default:
			color_dep = 18;
			break;
	}
	// Calculate average_bytes_per_tu based on compressed bpp
	if (dptx->dsc)
		color_dep = dptx->vparams.dsc_bpp;

	ts = (8 * color_dep * pixel_clock) / (lane_num * link_rate);

	tu  = ts / 1000;
	if (tu >= 65) {
		dptx_dbg(dptx, "%s: tu(%d) > 65",
		__func__, tu);
		return -EINVAL;
	}

	tu_frac = ts / 100 - tu * 10;

	// Calculate init_threshold for DSC mode
	if (dptx->dsc) {
		vparams->init_threshold = GB02FUNC675(dptx);
		dptx_dbg(dptx, "calculated init_threshold for dsc = %d\n", vparams->init_threshold);
		if (vparams->init_threshold < 32) {
			vparams->init_threshold = 32;
			dptx_dbg(dptx, "Set init_threshold for dsc to %d\n", vparams->init_threshold);
		}
		// Calculate init_threshold for non DSC mode
	} else {
		T1 = 0;
		T2 = 0;
		//Single Pixel Mode
		if (dptx->multipixel == GB02MAC2044) {
			if (tu < 6)
				vparams->init_threshold = 32;
			else if (mdtd->h_blanking <= 40 && encoding == YCBCR420)
				vparams->init_threshold = 3;
			else if (mdtd->h_blanking <= 80  && encoding != YCBCR420)
				vparams->init_threshold = 12;
			else
				vparams->init_threshold = 16;
			//Multiple Pixel Mode
		} else {
			gb_printf(KERN_INFO, "harutk---- enter MP switch \n");
			switch (bpc) {
				case COLOR_DEPTH_6:
					T1 = (4*1000/9)*lane_num;
					gb_printf(KERN_INFO, "harutk---- enter MP switch T1= %d\n", T1);
					break;
				case COLOR_DEPTH_8:
					if (encoding == YCBCR422)
						T1 = (1000/2)*lane_num;
					else if (encoding == YONLY)
						T1 = lane_num*1000;
					else if(dptx->multipixel == GB02MAC2045)
						T1 = (1000/3)*lane_num;
					else
						T1 = (3000/16)*lane_num;
					break;
				case COLOR_DEPTH_10:
					if (encoding == YCBCR422)
						T1 = (2000/5)*lane_num;
					else if (encoding == YONLY)
						T1 = (4000/5)*lane_num;
					else
						T1 = (4000/15)*lane_num;
					break;
				case COLOR_DEPTH_12:
					if(encoding == YCBCR422) {
						if(dptx->multipixel == GB02MAC2045)
							T1 = (1000/6)*lane_num;
						else
							T1 = (1000/3)*lane_num;
					} else if (encoding == YONLY)
						T1 = (2000/3)*lane_num;
					else
						T1 = (2000/9)*lane_num;
					break;
				case COLOR_DEPTH_16:
					if(encoding == YONLY)
						T1 = (1000/2)*lane_num;
					if((encoding != YONLY) && (encoding != YCBCR422) && (dptx->multipixel == GB02MAC2045))
						T1 = (1000/6)*lane_num;
					else
						T1 = (1000/4)*lane_num;
					break;
				default:
					dptx_dbg(dptx, "Invalid param BPC = %d\n" , bpc);
					return -EINVAL;
			}

			if (encoding == YCBCR420)
				pixel_clock = pixel_clock /2;

			T2 = (link_clk*1000 /  pixel_clock);

			vparams->init_threshold = T1 * T2 * tu / (1000*1000);
		}

	}

	vparams->aver_bytes_per_tu = (u8)tu;
	vparams->aver_bytes_per_tu_frac = (u8)tu_frac;
	dptx_dbg(dptx, "%s %d tu:%u, tu_frac:%u, init_threshold:%u \n",
		__func__, __LINE__,
		vparams->aver_bytes_per_tu,
		vparams->aver_bytes_per_tu_frac,
		vparams->init_threshold);

	return retval;
}

void GB02FUNC699(struct dptx *dptx, int stream)
{
	u32 reg;
	struct GB02STR124 *vparams;

	vparams = &dptx->vparams;

	reg = dptx_readl(dptx, GB02MAC2080(stream));
	reg = reg & (~DPTX_VIDEO_CONFIG5_TU_MASK);
	reg = reg | (vparams->aver_bytes_per_tu <<
			GB02MAC2292);
	if(dptx->mst) {
		reg = reg & (~DPTX_VIDEO_CONFIG5_TU_FRAC_MASK_MST);
		reg = reg | (vparams->aver_bytes_per_tu_frac <<
			     GB02MAC2293);
	}
	else {
		reg = reg & (~DPTX_VIDEO_CONFIG5_TU_FRAC_MASK_SST);
		reg = reg | (vparams->aver_bytes_per_tu_frac <<
			     GB02MAC2294);
	}
	reg = reg & (~DPTX_VIDEO_CONFIG5_INIT_THRESHOLD_MASK);
	reg = reg | (vparams->init_threshold <<
		      GB02MAC2295);
	dptx_writel(dptx, GB02MAC2080(stream), reg);
}

void GB02FUNC706(struct dptx *dptx, int stream)
{
	GB02FUNC708(dptx, stream);
	GB02FUNC725(dptx, stream);
	GB02FUNC719(dptx, stream);
}

void GB02FUNC708(struct dptx *dptx, int stream)
{
	u32 reg;
	u8 bpc_mapping = 0, bpc = 0;
	enum pixel_enc_type pix_enc;
	struct GB02STR124 *vparams;

	vparams = &dptx->vparams;
	bpc = vparams->bpc;
	pix_enc = vparams->pix_enc;

	reg = dptx_readl(dptx, GB02MAC2066(stream));
	reg &= ~DPTX_VSAMPLE_CTRL_VMAP_BPC_MASK;

	switch (pix_enc) {
	case RGB:
		if (bpc == COLOR_DEPTH_6)
			bpc_mapping = 0;
		else if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 1;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 2;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 3;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 4;
		break;
	case YCBCR444:
		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 5;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 6;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 7;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 8;
		break;
	case YCBCR422:
		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 9;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 10;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 11;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 12;
		break;
	case YCBCR420:
		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 13;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 14;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 15;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 16;
		break;
	case YONLY:
		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 17;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 18;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 19;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 20;
		break;
	case RAW:
		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 23;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 24;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 25;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 27;
		break;
	}

	/* TODO only for RGB 8bpc */
	if (dptx->dsc)
		reg |= (1 << GB02MAC2296); //sahakyan
//		reg |= 2 << GB02MAC2267; // check DSC-2slices for 10bpc -harutk
	else
		reg |= (bpc_mapping << GB02MAC2267);
	dptx_writel(dptx, GB02MAC2066(stream), reg);
}

void GB02FUNC719(struct dptx *dptx, int stream)
{
	u32 reg;
	u8 bpc_mapping = 0, bpc = 0;
	struct GB02STR124 *vparams;

	vparams = &dptx->vparams;
	bpc = vparams->bpc;
	reg = dptx_readl(dptx, GB02MAC2084(stream));
	reg &= ~DPTX_VG_CONFIG1_BPC_MASK;

	switch (bpc) {
	case COLOR_DEPTH_6:
		bpc_mapping = 0;
		break;
	case COLOR_DEPTH_8:
		bpc_mapping = 1;
		break;
	case COLOR_DEPTH_10:
		bpc_mapping = 2;
		break;
	case COLOR_DEPTH_12:
		bpc_mapping = 3;
		break;
	case COLOR_DEPTH_16:
		bpc_mapping = 4;
		break;
	}
	if(dptx->dsc)
		reg |= (1 << GB02MAC2296); // 10 bpc DSC harutk
	else
		reg |= (bpc_mapping << GB02MAC2296);

	dptx_writel(dptx, GB02MAC2084(stream), reg);
}

void GB02FUNC720(struct dptx *dptx, int stream)
{
	u32 reg_msa2;
	u8 col_mapping;
	u8 colorimetry;
	u8 dynamic_range;
	struct GB02STR124 *vparams;
	enum pixel_enc_type pix_enc;

	vparams = &dptx->vparams;
	pix_enc = vparams->pix_enc;
	colorimetry = vparams->colorimetry;
	dynamic_range = vparams->dynamic_range;

	reg_msa2 = dptx_readl(dptx, GB02MAC2082(stream));
	reg_msa2 &= ~DPTX_VIDEO_VMSA2_COL_MASK;

	col_mapping = 0;

	/* According to Table 2-94 of DisplayPort spec 1.3 */
	switch (pix_enc) {
	case RGB:
		if (dynamic_range == CEA)
			col_mapping = 4;
		else if (dynamic_range == VESA)
			col_mapping = 0;
		break;
	case YCBCR422:
		if (colorimetry == ITU601)
			col_mapping = 5;
		else if (colorimetry == ITU709)
			col_mapping = 13;
		break;
	case YCBCR444:
		if (colorimetry == ITU601)
			col_mapping = 6;
		else if (colorimetry == ITU709)
			col_mapping = 14;
		break;
	case RAW:
		col_mapping = 1;
		break;
	case YCBCR420:
	case YONLY:
		break;
	}

	reg_msa2 |= (col_mapping << GB02MAC2274);
	dptx_writel(dptx, GB02MAC2082(stream), reg_msa2);
}

void GB02FUNC725(struct dptx *dptx, int stream)
{
	u32 reg_msa2, reg_msa3;
	u8 bpc_mapping = 0, bpc = 0;
	struct GB02STR124 *vparams;
	enum pixel_enc_type pix_enc;

	vparams = &dptx->vparams;
	pix_enc = vparams->pix_enc;
	bpc = vparams->bpc;

	reg_msa2 = dptx_readl(dptx, GB02MAC2082(stream));
	reg_msa3 = dptx_readl(dptx, GB02MAC2083(stream));

#ifdef GB02_PAL_DP
	/* SET nvid, BIT[23:0], If the clocks are asynchronous,
	controller automatically sets this value to 32768 */
	reg_msa3 &= GENMASK(31, 24);
	reg_msa3 |= 0x8000;
#endif

	reg_msa2 &= ~DPTX_VIDEO_VMSA2_BPC_MASK;
	reg_msa3 &= ~DPTX_VIDEO_VMSA3_PIX_ENC_MASK;

	switch (pix_enc) {
	case RGB:

		if (bpc == COLOR_DEPTH_6)
			bpc_mapping = 0;
		else if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 1;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 2;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 3;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 4;
		break;
	case YCBCR444:

		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 1;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 2;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 3;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 4;
		break;
	case YCBCR422:

		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 1;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 2;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 3;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 4;
		break;
	case YCBCR420:
                reg_msa3 |= 1 << GB02MAC2276;
		break;
	case YONLY:

		/* According to Table 2-94 of DisplayPort spec 1.3 */
		reg_msa3 |= 1 << GB02MAC2275;

		if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 1;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 2;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 3;
		if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 4;
		break;
	case RAW:

		 /* According to Table 2-94 of DisplayPort spec 1.3 */
		reg_msa3 |= (1 << GB02MAC2275);

		if (bpc == COLOR_DEPTH_6)
			bpc_mapping = 1;
		else if (bpc == COLOR_DEPTH_8)
			bpc_mapping = 3;
		else if (bpc == COLOR_DEPTH_10)
			bpc_mapping = 4;
		else if (bpc == COLOR_DEPTH_12)
			bpc_mapping = 5;
		else if (bpc == COLOR_DEPTH_16)
			bpc_mapping = 7;
		break;
	}

	if(dptx->dsc)
		reg_msa2 |= (1 << GB02MAC2273); // DSC 10bpc harutk
	else
		reg_msa2 |= (bpc_mapping << GB02MAC2273);

	dptx_writel(dptx, GB02MAC2082(stream), reg_msa2);
	dptx_writel(dptx, GB02MAC2083(stream), reg_msa3);
	GB02FUNC720(dptx, stream);
}

void GB02FUNC737(struct dptx *dptx, int stream)
{
	u32 reg;

	reg = dptx_readl(dptx, GB02MAC2137(stream));
	reg &= ~DPTX_VG_RAM_ADDR_START_MASK;
	dptx_writel(dptx, GB02MAC2137(stream), reg);

	reg = dptx_readl(dptx, GB02MAC2138(stream));
	reg |= GB02MAC2142;
	dptx_writel(dptx, GB02MAC2138(stream), reg);

	reg = dptx_readl(dptx, GB02MAC2138(stream));
	reg &= ~GB02MAC2142;
	dptx_writel(dptx, GB02MAC2138(stream), reg);
}

void GB02FUNC741(struct dptx *dptx, u32 value, int count, int stream)
{
	int i;

	for (i = 0; i < count; i++)
		dptx_writel(dptx, GB02MAC2139(stream), value);
}

void GB02FUNC742(struct dptx *dptx, int column_num, int stream)
{
	int i, step, range, shift1, shift2, value;
	u8 bpc, mult;
	struct GB02STR124 *vparams;

	vparams = &dptx->vparams;
	bpc = vparams->bpc;
	dev_dbg(dptx->dev, "%s \n", __func__);

	GB02FUNC737(dptx, stream);

	switch (dptx->multipixel) {
	default:
	case GB02MAC2044:
 		GB02FUNC741(dptx, 0, 6, stream);
		break;
	case GB02MAC2045:
		GB02FUNC741(dptx, 0, 12, stream);
		break;
	case GB02MAC2046:
		GB02FUNC741(dptx, 0, 24, stream);
		break;
	}

	range = 0;
	shift1 = 0;
	shift2 = 0;
	step = 0;
	value = 0;
	mult = 1;

	if (bpc == COLOR_DEPTH_6 || bpc == COLOR_DEPTH_8) {
	   if(!dptx->dsc){
	 	if (bpc == COLOR_DEPTH_6)
			mult = 4;
	   }

		/* Program RED color */
		for (i = 0; i < column_num; i++) {
			GB02FUNC741(dptx, i * mult, 1, stream);
			GB02FUNC741(dptx, 0, 5, stream);
		}
		/* Program GREEN color */
		for (i = 0; i < column_num; i++) {
			GB02FUNC741(dptx, 0, 2, stream);
			GB02FUNC741(dptx, i * mult, 1, stream);
			GB02FUNC741(dptx, 0, 3, stream);
		}
		/* Program BLUE color */
		for (i = 0; i < column_num; i++) {
			GB02FUNC741(dptx, 0, 4, stream);
			GB02FUNC741(dptx, i * mult, 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
		}
		/* Program WHITE color */
		for (i = 0; i < column_num; i++) {
			GB02FUNC741(dptx, i * mult, 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
			GB02FUNC741(dptx, i * mult, 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
			GB02FUNC741(dptx, i * mult, 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
		}
	} else if (bpc == COLOR_DEPTH_10 || bpc == COLOR_DEPTH_12 ||
		   bpc == COLOR_DEPTH_16) {
		if (bpc == COLOR_DEPTH_10) {
			range = 384;
			shift1 = 2;
			shift2 = 6;
			step = 4;
		} else if (bpc == COLOR_DEPTH_12) {
			range = 1920;
			shift1 = 1;
			shift2 = 7;
			step = 16;
		} else if (bpc == COLOR_DEPTH_16) {
			range = 32640;
			shift1 = 0;
			shift2 = 8;
			step = 256;
		}

		for (i = 0; i < column_num; i++) {
			value = range + i;
			GB02FUNC741(dptx, (value >> shift1), 1, stream);
			GB02FUNC741(dptx, (value << shift2), 1, stream);
			GB02FUNC741(dptx, 0, 4, stream);
		}
		for (i = 0; i < column_num * step; i = i + step) {
			GB02FUNC741(dptx, (i >> shift1), 1, stream);
			GB02FUNC741(dptx, (i << shift2), 1, stream);
			GB02FUNC741(dptx, 0, 4, stream);
		}
		for (i = 0; i < column_num; i++) {
			value = range + i;
			GB02FUNC741(dptx, 0, 2, stream);
			GB02FUNC741(dptx, (value >> shift1), 1, stream);
			GB02FUNC741(dptx, (value << shift2), 1, stream);
			GB02FUNC741(dptx, 0, 2, stream);
		}
		for (i = 0; i < column_num * step; i = i + step) {
			GB02FUNC741(dptx, 0, 2, stream);
			GB02FUNC741(dptx, (i >> shift1), 1, stream);
			GB02FUNC741(dptx, (i << shift2), 1, stream);
			GB02FUNC741(dptx, 0, 2, stream);
		}
		for (i = 0; i < column_num; i++) {
			value = range + i;
			GB02FUNC741(dptx, 0, 4, stream);
			GB02FUNC741(dptx, (value >> shift1), 1, stream);
			GB02FUNC741(dptx, (value << shift2), 1, stream);
		}
		for (i = 0; i < column_num * step; i = i + step) {
			GB02FUNC741(dptx, 0, 4, stream);
			GB02FUNC741(dptx, (i >> shift1), 1, stream);
			GB02FUNC741(dptx, (i << shift2), 1, stream);
		}
		for (i = 0; i < column_num; i++) {
			value = range + i;
			GB02FUNC741(dptx, (value >> shift1), 1, stream);
			GB02FUNC741(dptx, (value << shift2), 1, stream);
			GB02FUNC741(dptx, (value >> shift1), 1, stream);
			GB02FUNC741(dptx, (value << shift2), 1, stream);
			GB02FUNC741(dptx, (value >> shift1), 1, stream);
			GB02FUNC741(dptx, (value << shift2), 1, stream);
		}
		for (i = 0; i < column_num * step; i = i + step) {
			GB02FUNC741(dptx, (i >> shift1), 1, stream);
			GB02FUNC741(dptx, (i << shift2), 1, stream);
			GB02FUNC741(dptx, (i >> shift1), 1, stream);
			GB02FUNC741(dptx, (i << shift2), 1, stream);
			GB02FUNC741(dptx, (i >> shift1), 1, stream);
			GB02FUNC741(dptx, (i << shift2), 1, stream);
		}
	}
}

void GB02FUNC756(struct dptx *dptx, int stream)
{
	int i, j;
	u8 ram_pix_cnt;
	static int white[3] = {235, 235, 235};
	static int yellow[3] = {235, 235, 16};
	static int cyan[3] = {16, 235, 235};
	static int green[3] = {16, 235, 16};
	static int magenta[3] = {235, 16, 235};
	static int red[3] = {235, 16, 16};
	static int blue[3] = {16, 16, 235};
	static int black[3] = {16, 16, 16};
	static int *colors_row1[8] = {
		white, yellow, cyan, green,
		magenta, red, blue, black
	};
	static int *colors_row2[8] = {
		blue, red, magenta, green,
		cyan, yellow, white, black
	};

	GB02FUNC737(dptx, stream);

	switch (dptx->multipixel) {
	default:
	case GB02MAC2044:
		GB02FUNC741(dptx, 0, 6, stream);
		ram_pix_cnt = 1;
		break;
	case GB02MAC2045:
		GB02FUNC741(dptx, 0, 12, stream);
		ram_pix_cnt = 2;
		break;
	case GB02MAC2046:
		GB02FUNC741(dptx, 0, 24, stream);
		ram_pix_cnt = 4;
		break;
	}

	for (i = 0; i < 8; i++) {
		for (j = 0; j < ram_pix_cnt; j++) {
			GB02FUNC741(dptx, colors_row1[i][0], 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
			GB02FUNC741(dptx, colors_row1[i][1], 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
			GB02FUNC741(dptx, colors_row1[i][2], 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
		}
	}
	for (i = 0; i < 8; i++) {
		for (j = 0; j < ram_pix_cnt; j++) {
			GB02FUNC741(dptx, colors_row2[i][0], 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
			GB02FUNC741(dptx, colors_row2[i][1], 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
			GB02FUNC741(dptx, colors_row2[i][2], 1, stream);
			GB02FUNC741(dptx, 0, 1, stream);
		}
	}
}

void GB02FUNC762(struct dptx *dptx, int stream)
{
	struct GB02STR124 *vparams;
	u8 colorimetry, bpc, shift1, shift2;
	int i, j, mult;
	u8 ram_pix_cnt;

	static int white[3] = {235, 128, 128};
	static int yellow[3] = {210, 16, 146};
	static int cyan[3] = {170, 166, 16};
	static int green[3] = {145, 54, 34};
	static int magenta[3] = {106, 202, 222};
	static int red[3] = {81, 90, 240};
	static int blue[3] = {41, 240, 110};
	static int black[3] = {16, 128, 128};
	static int *colors_row1[8] = {
		white, yellow, cyan, green,
		magenta, red, blue, black
	};

	GB02FUNC737(dptx, stream);

	switch (dptx->multipixel) {
	default:
	case GB02MAC2044:
		GB02FUNC741(dptx, 0, 6, stream);
		ram_pix_cnt = 1;
		break;
	case GB02MAC2045:
		GB02FUNC741(dptx, 0, 12, stream);
		ram_pix_cnt = 2;
		break;
	case GB02MAC2046:
		GB02FUNC741(dptx, 0, 24, stream);
		ram_pix_cnt = 4;
		break;
	}

	vparams = &dptx->vparams;
	colorimetry = vparams->colorimetry;
	bpc = vparams->bpc;

	if (colorimetry == ITU601) {
		if (bpc == COLOR_DEPTH_10) {
			white[0] = 940;
			white[1] = 512;
			white[2] = 512;
			yellow[0] = 840;
			yellow[1] = 64;
			yellow[2] = 585;
			cyan[0] = 678;
			cyan[1] = 663;
			cyan[2] = 64;
			green[0] = 578;
			green[1] = 215;
			green[2] = 137;
			magenta[0] = 426;
			magenta[1] = 809;
			magenta[2] = 887;
			red[0] = 326;
			red[1] = 361;
			red[2] = 960;
			blue[0] = 164;
			blue[1] = 960;
			blue[2] = 439;
			black[0] = 64;
			black[1] = 512;
			black[2] = 512;
		} else {
			white[0] = 235;
			white[1] = 128;
			white[2] = 128;
			yellow[0] = 210;
			yellow[1] = 16;
			yellow[2] = 146;
			cyan[0] = 170;
			cyan[1] = 166;
			cyan[2] = 16;
			green[0] = 145;
			green[1] = 54;
			green[2] = 34;
			magenta[0] = 106;
			magenta[1] = 202;
			magenta[2] = 222;
			red[0] = 81;
			red[1] = 90;
			red[2] = 240;
			blue[0] = 41;
			blue[1] = 240;
			blue[2] = 110;
			black[0] = 16;
			black[1] = 128;
			black[2] = 128;
		}
	} else {
		if (bpc == COLOR_DEPTH_10) {
			white[0] = 940;
			white[1] = 512;
			white[2] = 512;
			yellow[0] = 877;
			yellow[1] = 64;
			yellow[2] = 553;
			cyan[0] = 753;
			cyan[1] = 614;
			cyan[2] = 64;
			green[0] = 690;
			green[1] = 167;
			green[2] = 106;
			magenta[0] = 314;
			magenta[1] = 857;
			magenta[2] = 918;
			red[0] = 251;
			red[1] = 410;
			red[2] = 960;
			blue[0] = 127;
			blue[1] = 960;
			blue[2] = 471;
			black[0] = 64;
			black[1] = 512;
			black[2] = 512;
		} else {
		        white[0] = 235;
			white[1] = 128;
			white[2] = 128;
			yellow[0] = 219;
			yellow[1] = 16;
			yellow[2] = 138;
			cyan[0] = 188;
			cyan[1] = 154;
			cyan[2] = 16;
			green[0] = 173;
			green[1] = 42;
			green[2] =  26;
			magenta[0] = 78;
			magenta[1] = 214;
			magenta[2] = 230;
			red[0] = 63;
			red[1] = 102;
			red[2] = 240;
			blue[0] = 32;
			blue[1] = 240;
			blue[2] = 118;
			black[0] = 16;
			black[1] = 128;
			black[2] = 128;
		}
	}

	if (bpc == COLOR_DEPTH_10) {
		shift1 = 2;
		shift2 = 6;
		mult = 1;
	} else if (bpc == COLOR_DEPTH_12) {
		shift1 = 4;
		shift2 = 4;
		mult = 16;
	} else if (bpc == COLOR_DEPTH_16) {
		shift1 = 8;
		shift2 = 0;
		mult = 16 * 16;
	} else {
		shift1 = 0;
		shift2 = 0;
		mult = 1;
	}

	colors_row1[0] = white;
	colors_row1[1] = yellow;
	colors_row1[2] = cyan;
	colors_row1[3] = green;
	colors_row1[4] = magenta;
	colors_row1[5] = red;
	colors_row1[6] = blue;
	colors_row1[7] = black;

	for (i = 0; i < 8; i++) {
		for(j = 0; j< ram_pix_cnt; j++) { // for quad/dual pixel the same pixel should be programmed 4/2 times
			GB02FUNC741(dptx, (colors_row1[i][0]*mult) >> shift1, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][0]*mult) << shift2, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][1]*mult) >> shift1, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][1]*mult) << shift2, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][2]*mult) >> shift1, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][2]*mult) << shift2, 1, stream);
		}
	}

	colors_row1[0] = blue;
	colors_row1[1] = red;
	colors_row1[2] = magenta;
	colors_row1[4] = cyan;
	colors_row1[5] = yellow;
	colors_row1[6] = white;
	colors_row1[7] = black;

	for (i = 0; i < 8; i++) {
		for(j = 0; j< ram_pix_cnt; j++) { // for quad/dual pixel the same pixel should be programmed 4/2 times
			GB02FUNC741(dptx, (colors_row1[i][0]*mult) >> shift1, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][0]*mult) << shift2, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][1]*mult) >> shift1, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][1]*mult) << shift2, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][2]*mult) >> shift1, 1, stream);
			GB02FUNC741(dptx, (colors_row1[i][2]*mult) << shift2, 1, stream);
		}
	}
}

void GB02FUNC781(struct dptx *dptx, int stream)
{
	struct GB02STR124 *vparams;
	u32 reg;
	u8 pattern, bpc, encoding;
	int column_num;

	vparams = &dptx->vparams;
	pattern = vparams->pattern_mode;
	bpc = vparams->bpc;
	encoding = vparams->pix_enc;

	column_num = 256;

#if 0
	/* TODO - This is temporary code for mst. Just program the pattern
	 * code and not the pattern in the VG RAM. */
	reg = dptx_readl(dptx, GB02MAC2084(stream));
	reg &= ~DPTX_VG_CONFIG1_PATTERN_MASK;
	reg |= (0) << GB02MAC2297;
	dptx_writel(dptx, GB02MAC2084(stream), reg);

	return;
#else

	if (pattern == RAMP) {
	   if(!dptx->dsc) {
 	     if (bpc == COLOR_DEPTH_6)
	        column_num = 64;
	   }
		GB02FUNC742(dptx, column_num, stream);
	} else if (pattern == COLRAMP) {
		if (encoding == YCBCR422 || encoding == YCBCR444)
			GB02FUNC762(dptx, stream);
		else
			GB02FUNC756(dptx, stream);
	}

	reg = dptx_readl(dptx, GB02MAC2084(stream));
	reg &= ~DPTX_VG_CONFIG1_PATTERN_MASK;
	reg |= (vparams->pattern_mode << GB02MAC2297);
	dptx_writel(dptx, GB02MAC2084(stream), reg);
#endif
}

void GB02FUNC784(struct dptx *dptx, u8 pattern, int stream)
{
	struct GB02STR124 *vparams;

	vparams = &dptx->vparams;
	vparams->pattern_mode = pattern;

	GB02FUNC788(dptx, stream);
	GB02FUNC781(dptx, stream);
	dptx_writel(dptx, GB02MAC2089(stream), 0);
	GB02FUNC789(dptx, stream);
	dptx_dbg(dptx, "%s: Change video pattern to %d\n",
		 __func__, pattern);
}

void GB02FUNC788(struct dptx *dptx, int stream)
{
	u32 vsamplectrl;

	vsamplectrl = dptx_readl(dptx, GB02MAC2066(stream));
	vsamplectrl &= ~GB02MAC2252;
	dptx_writel(dptx, GB02MAC2066(stream), vsamplectrl);
}

void GB02FUNC789(struct dptx *dptx, int stream)
{
	u32 vsamplectrl;

	vsamplectrl = dptx_readl(dptx, GB02MAC2066(stream));
	vsamplectrl |= GB02MAC2252;
	dptx_writel(dptx, GB02MAC2066(stream), vsamplectrl);
}

/*
 * Audio/Video Parameters
 */

void GB02FUNC792(struct GB02STR122 *params)
{
	params->iec_channel_numcl0 = 8;
	params->iec_channel_numcr0 = 4;
	params->use_lut = 1;
	params->iec_samp_freq = 3;
	params->iec_word_length = 11;
	params->iec_orig_samp_freq = 12;
	params->data_width = 24;
	params->num_channels = 2;
	params->inf_type = 0;
	params->ats_ver = 17;
	params->mute = 0;
}

void GB02FUNC795(struct dptx *dptx)
{
	struct GB02STR124 *params = &dptx->vparams;

	params->pix_enc = RGB;
	params->mode = 1;

	if (dptx->mst || dptx->dsc) {
		/* TODO 6 bpc should be default - use 8 bpc for MST calculation */
		params->bpc = COLOR_DEPTH_6;
	} else {
		params->bpc = COLOR_DEPTH_6;
	}

	params->colorimetry = ITU601;
	params->dynamic_range = CEA;
	params->aver_bytes_per_tu = 30;
	params->aver_bytes_per_tu_frac = 0;
	params->init_threshold = 15;
	params->pattern_mode = RAMP;
	params->refresh_rate = 60000;
	params->video_format = VCEA;
}

/*
 * DTD
 */

void GB02FUNC798(struct dtd *mdtd)
{
	mdtd->pixel_repetition_input = 0;
	mdtd->pixel_clock  = 0;
	mdtd->h_active = 0;
	mdtd->h_blanking = 0;
	mdtd->h_sync_offset = 0;
	mdtd->h_sync_pulse_width = 0;
	mdtd->h_image_size = 0;
	mdtd->v_active = 0;
	mdtd->v_blanking = 0;
	mdtd->v_sync_offset = 0;
	mdtd->v_sync_pulse_width = 0;
	mdtd->v_image_size = 0;
	mdtd->interlaced = 0;
	mdtd->v_sync_polarity = 0;
	mdtd->h_sync_polarity = 0;
}
