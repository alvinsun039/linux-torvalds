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
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_dp_helper.h>
#else
#include <drm/display/drm_dp.h>
#endif
#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 21)
#include <drm/drm_probe_helper.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#include <linux/vmalloc.h>
#endif
#include "gb_dp.h"
#include "gb_dp_dptx.h"
#include "gb_dp_helper_add.h"
#include "common/gb_pcie_info.h"
#include "common/gb_common.h"
#include "audio/i2s_platform.h"
#include "audio/designware_i2s.h"
#include "gpu/gb_device.h"
#include "gbdc_infinity.h"

static int GB02FUNC1367(struct dptx *dptx);
static int GB02FUNC1396(struct dptx *dptx, u8 port, u8 vcpid, u16 pbn, int port1);
static int GB02FUNC1381(struct dptx *dptx, u8 request_id, u8 *msg_out);
static int GB02FUNC1276(struct dptx *dptx);
int GB02FUNC1316(struct dptx *dptx);

#ifndef GB02MAC1830
#define GB02MAC1830         (0 << 3)
#endif

#ifndef  DP_TEST_LINK_AUDIO_PATTERN
#define DP_TEST_LINK_AUDIO_PATTERN         (1 << 5) /* DPCD >= 1.2 */
#endif
int GB02FUNC1172(struct dptx *dptx, u32 audio_clock_freq,
	u16 oversample_factor)
{
	return 0;
}

#if 1
static int GB02FUNC1198(struct dptx *dptx)
{
	int retval;
	u8 lanes;
	u8 rate;
	u32 phyifctrl;
	struct GB02STR124 *vparams;
	struct dtd *mdtd;

	GB02FUNC1076(dptx);

	/* Move to P0 */
	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	phyifctrl &= ~DPTX_PHYIF_CTRL_LANE_PWRDOWN_MASK;
	dptx_writel(dptx, GB02MAC2107, phyifctrl);

	retval = GB02FUNC1151(dptx, DP_TEST_LINK_RATE, &rate);
	if (retval)
		return retval;

	retval = GB02FUNC1126(rate);
	if (retval < 0)
		return retval;

	rate = retval;

	retval = GB02FUNC1151(dptx, DP_TEST_LANE_COUNT, &lanes);
	if (retval)
		return retval;

	dptx_dbg(dptx, "%s: Strating link training rate=%d, lanes=%d\n",
		 __func__, rate, lanes);

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;

	retval = GB02FUNC682(dptx, lanes, rate,
					 vparams->bpc, vparams->pix_enc,
					 mdtd->pixel_clock);
	if (retval)
		return retval;

	retval = GB02FUNC1567(dptx,
				    rate,
				    lanes);
	if (retval)
		dptx_err(dptx, "Link training failed %d\n", retval);
	else
		dptx_dbg(dptx, "Link training succeeded\n");

	return retval;
}
static int GB02FUNC1202(struct dptx *dptx, int stream)
{
	int retval, i;
	u8 test_h_total_lsb, test_h_total_msb, test_v_total_lsb,
	   test_v_total_msb, test_h_start_lsb, test_h_start_msb,
	   test_v_start_lsb, test_v_start_msb, test_hsync_width_lsb,
	   test_hsync_width_msb, test_vsync_width_lsb, test_vsync_width_msb,
	   test_h_width_lsb, test_h_width_msb, test_v_width_lsb,
	   test_v_width_msb;
	u32 h_total, v_total, h_start, v_start, h_width, v_width,
	    hsync_width, vsync_width, h_sync_pol, v_sync_pol, refresh_rate;
	enum video_format_type video_format;
	u8 vmode;
	u8 test_refresh_rate;
	struct GB02STR124 *vparams;
	struct dtd mdtd;

	vparams = &dptx->vparams;
	retval = 0;
	h_total = 0;
	v_total = 0;
	h_start = 0;
	v_start = 0;
	v_width = 0;
	h_width = 0;
	hsync_width = 0;
	vsync_width = 0;
	h_sync_pol = 0;
	v_sync_pol = 0;
	test_refresh_rate = 0;
	i = 0;

	/* H_TOTAL */
	retval = GB02FUNC1151(dptx, DP_TEST_H_TOTAL_LO, &test_h_total_lsb);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, DP_TEST_H_TOTAL_HI, &test_h_total_msb);
	if (retval)
		return retval;
	h_total |= test_h_total_lsb;
	h_total |= test_h_total_msb << 8;
	dptx_dbg(dptx, "h_total = %d\n", h_total);

	/* V_TOTAL */
	retval = GB02FUNC1151(dptx, DP_TEST_V_TOTAL_LO, &test_v_total_lsb);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, DP_TEST_V_TOTAL_HI, &test_v_total_msb);
	if (retval)
		return retval;
	v_total |= test_v_total_lsb;
	v_total |= test_v_total_msb << 8;
	dptx_dbg(dptx, "v_total = %d\n", v_total);

	/*  H_START */
	retval = GB02FUNC1151(dptx, DP_TEST_H_START_LO, &test_h_start_lsb);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, DP_TEST_H_START_HI, &test_h_start_msb);
	if (retval)
		return retval;
	h_start |= test_h_start_lsb;
	h_start |= test_h_start_msb << 8;
	dptx_dbg(dptx, "h_start = %d\n", h_start);

	/* V_START */
	retval = GB02FUNC1151(dptx, DP_TEST_V_START_LO, &test_v_start_lsb);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, DP_TEST_V_START_HI, &test_v_start_msb);
	if (retval)
		return retval;
	v_start |= test_v_start_lsb;
	v_start |= test_v_start_msb << 8;
	dptx_dbg(dptx, "v_start = %d\n", v_start);

	/* TEST_HSYNC */
	retval = GB02FUNC1151(dptx, DP_TEST_HSYNC_WIDTH_LO,
				&test_hsync_width_lsb);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, DP_TEST_HSYNC_HI,
				&test_hsync_width_msb);
	if (retval)
		return retval;
	hsync_width |= test_hsync_width_lsb;
	hsync_width |= (test_hsync_width_msb & (~(1 << 7))) << 8;
	h_sync_pol |= (test_hsync_width_msb & (1 << 7)) >> 8;
	dptx_dbg(dptx, "hsync_width = %d\n", hsync_width);
	dptx_dbg(dptx, "h_sync_pol = %d\n", h_sync_pol);

	/* TEST_VSYNC */
	retval = GB02FUNC1151(dptx, DP_TEST_VSYNC_WIDTH_LO,
				&test_vsync_width_lsb);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, DP_TEST_VSYNC_HI,
				&test_vsync_width_msb);
	if (retval)
		return retval;
	vsync_width |= test_vsync_width_lsb;
	vsync_width |= (test_vsync_width_msb & (~(1 << 7))) << 8;
	v_sync_pol |= (test_vsync_width_msb & (1 << 7)) >> 8;
	dptx_dbg(dptx, "vsync_width = %d\n", vsync_width);
	dptx_dbg(dptx, "v_sync_pol = %d\n", v_sync_pol);

	/* TEST_H_WIDTH */
	retval = GB02FUNC1151(dptx, DP_TEST_H_WIDTH_LO, &test_h_width_lsb);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, DP_TEST_H_WIDTH_HI, &test_h_width_msb);
	if (retval)
		return retval;
	h_width |= test_h_width_lsb;
	h_width |= test_h_width_msb << 8;
	dptx_dbg(dptx, "h_width = %d\n", h_width);

	/* TEST_V_WIDTH */
	retval = GB02FUNC1151(dptx, DP_TEST_V_HEIGHT_LO, &test_v_width_lsb);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, DP_TEST_V_HEIGHT_HI, &test_v_width_msb);
	if (retval)
		return retval;
	v_width |= test_v_width_lsb;
	v_width |= test_v_width_msb << 8;
	dptx_dbg(dptx, "v_width = %d\n", v_width);

	retval = GB02FUNC1151(dptx, 0x234, &test_refresh_rate);
	if (retval)
		return retval;
	dptx_dbg(dptx, "test_refresh_rate = %d\n", test_refresh_rate);

	video_format = DMT;
	refresh_rate =  test_refresh_rate * 1000;

	if (h_total == 1056 && v_total == 628 && h_start == 216 &&
	    v_start == 27 && hsync_width == 128 && vsync_width == 4 &&
	    h_width == 800 && v_width == 600) {
		vmode = 9;
	} else if (h_total == 1088 && v_total == 517 && h_start == 224 &&
		   v_start == 31 && hsync_width == 112 && vsync_width == 8 &&
		   h_width == 848 && v_width == 480) {
		vmode = 14;
	} else if (h_total == 1344 && v_total == 806 && h_start == 296 &&
		   v_start == 35 && hsync_width == 136 && vsync_width == 6 &&
		   h_width == 1024 && v_width == 768) {
		vmode = 16;
	} else if (h_total == 1440 && v_total == 790 && h_start == 112 &&
		   v_start == 19 && hsync_width == 32 && vsync_width == 7 &&
		   h_width == 1280 && v_width == 768) {
		vmode = 22;
	} else if (h_total == 1664 && v_total == 798 && h_start == 320 &&
		   v_start == 27 && hsync_width == 128 && vsync_width == 7 &&
		   h_width == 1280 && v_width == 768) {
		vmode = 23;
	} else if (h_total == 1440 && v_total == 823 && h_start == 112 &&
		   v_start == 20 && hsync_width == 32 && vsync_width == 6 &&
		   h_width == 1280 && v_width == 800) {
		vmode = 27;
	} else if (h_total == 1800 && v_total == 1000 && h_start == 424 &&
		   v_start == 39 && hsync_width == 112 && vsync_width == 3 &&
		   h_width == 1280 && v_width == 960) {
		vmode = 32;
	} else if (h_total == 1688 && v_total == 1066 && h_start == 360 &&
		   v_start == 41 && hsync_width == 112 && vsync_width == 3 &&
		   h_width == 1280 && v_width  == 1024) {
		vmode = 35;
	} else if (h_total == 1792 && v_total == 795 && h_start == 368 &&
		   v_start == 24 && hsync_width == 112 && vsync_width == 6 &&
		   h_width == 1360 && v_width == 768) {
		vmode = 39;
	} else if (h_total == 1560 && v_total == 1080  && h_start == 112  &&
		   v_start == 27 && hsync_width == 32 && vsync_width == 4 &&
		   h_width == 1400 && v_width == 1050) {
		vmode = 41;
	} else if (h_total == 2160 && v_total == 1250 && h_start == 496 &&
		   v_start == 49 && hsync_width == 192 && vsync_width == 3 &&
		   h_width == 1600 && v_width == 1200) {
		vmode = 51;
	} else if (h_total == 2448 && v_total == 1394 && h_start == 528 &&
		   v_start == 49 && hsync_width == 200 && vsync_width == 3 &&
		   h_width == 1792 && v_width == 1344) {
		vmode = 62;
	} else if (h_total == 2600 && v_total == 1500 && h_start == 552 &&
		   v_start == 59  && hsync_width == 208 && vsync_width == 3 &&
		   h_width == 1920 && v_width == 1440) {
		vmode = 73;
	} else if (h_total == 2200 && v_total == 1125 && h_start == 192 &&
		   v_start == 41 && hsync_width == 44 && vsync_width == 5 &&
		   h_width == 1920 && v_width == 1080) {
		if (refresh_rate == 120000) {
			vmode = 63;
			video_format = VCEA;
		} else {
			vmode = 82;
		}
	} else if (h_total == 800 && v_total == 525 && h_start == 144 &&
		   v_start == 35 && hsync_width == 96 && vsync_width == 2 &&
		   h_width == 640 && v_width == 480) {
		vmode = 1;
		video_format = VCEA;
	} else if (h_total == 1650 && v_total == 750 && h_start == 260 &&
		   v_start == 25 && hsync_width == 40 && vsync_width == 5 &&
		   h_width == 1280  && v_width == 720) {
		vmode = 4;
		video_format = VCEA;
	} else if (h_total == 1680 && v_total == 831 && h_start == 328 &&
		   v_start == 28 && hsync_width == 128 && vsync_width == 6 &&
		   h_width == 1280 && v_width == 800) {
		vmode = 28;
		video_format = CVT;
	} else if (h_total == 1760 && v_total == 1235 && h_start == 112 &&
		   v_start == 32 && hsync_width == 32 && vsync_width == 4 &&
		   h_width == 1600 && v_width == 1200) {
		vmode = 40;
		video_format = CVT;
	} else if (h_total == 2208 && v_total == 1580 && h_start == 112 &&
		   v_start == 41 && hsync_width == 32 &&  vsync_width == 4 &&
		   h_width == 2048 && v_width == 1536) {
		vmode = 41;
		video_format = CVT;
	} else {
		dptx_dbg(dptx, "Unknown video mode\n");
		return -EINVAL;
	}

	if (!GB02FUNC811(&mdtd, vmode, refresh_rate, video_format)) {
		dptx_dbg(dptx, "%s: Invalid video mode value %d\n",
			 __func__, vmode);
		retval = -EINVAL;
		goto fail;
	}
	vparams->mdtd = mdtd;
	vparams->refresh_rate = refresh_rate;
	retval = GB02FUNC682(dptx, dptx->link.lanes,
					 dptx->link.rate, vparams->bpc,
					 vparams->pix_enc, mdtd.pixel_clock);
	if (retval)
		return retval;
	/* MMCM */
	GB02FUNC454(dptx, 1, stream);
	retval = GB02FUNC1171(dptx, mdtd.pixel_clock, stream);
	if (retval) {
		GB02FUNC454(dptx, 0, stream);
		goto fail;
	}
	GB02FUNC454(dptx, 0, stream);

	vparams->mode = vmode;
	vparams->video_format = video_format;
	GB02FUNC492(dptx, stream);
fail:
	return retval;
}
//#ifndef	CONFIG_UOS_20
static int GB02FUNC1218(struct dptx *dptx)
{
	int retval;
	u8 test_audio_mode, test_audio_smaple_range, test_audio_ch_count,
	   audio_ch_count, orig_sample_freq, sample_freq;
	u32 audio_clock_freq;
	struct GB02STR122 *aparams;

	aparams = &dptx->aparams;
	retval = GB02FUNC1151(dptx, GB02MAC1779, &test_audio_mode);
	if (retval)
		return retval;

	dptx_dbg(dptx, "test_audio_mode = %d\n", test_audio_mode);

	test_audio_smaple_range = test_audio_mode &
		DP_TEST_AUDIO_SAMPLING_RATE_MASK;
	test_audio_ch_count = (test_audio_mode & DP_TEST_AUDIO_CH_COUNT_MASK)
		>> GB02MAC1782;

	switch (test_audio_ch_count) {
	case GB02MAC1800:
		dptx_dbg(dptx, "GB02MAC1800\n");
		audio_ch_count = 1;
		break;
	case GB02MAC1802:
		dptx_dbg(dptx, "GB02MAC1802\n");
		audio_ch_count = 2;
		break;
	case GB02MAC1803:
		dptx_dbg(dptx, "GB02MAC1803\n");
		audio_ch_count = 3;
		break;
	case GB02MAC1805:
		dptx_dbg(dptx, "GB02MAC1805\n");
		audio_ch_count = 4;
		break;
	case GB02MAC1807:
		dptx_dbg(dptx, "GB02MAC1807\n");
		audio_ch_count = 5;
		break;
	case GB02MAC1809:
		dptx_dbg(dptx, "GB02MAC1809\n");
		audio_ch_count = 6;
		break;
	case GB02MAC1810:
		dptx_dbg(dptx, "GB02MAC1810\n");
		audio_ch_count = 7;
		break;
	case GB02MAC1812:
		dptx_dbg(dptx, "GB02MAC1812\n");
		audio_ch_count = 8;
		break;
	default:
		dptx_dbg(dptx, "Invalid TEST_AUDIO_CHANNEL_COUNT\n");
		return -EINVAL;
	}
	dptx_dbg(dptx, "test_audio_ch_count = %d\n", audio_ch_count);
	aparams->num_channels = audio_ch_count;

	switch (test_audio_smaple_range) {
	case GB02MAC1785:
		dptx_dbg(dptx, "GB02MAC1785\n");
		orig_sample_freq = 12;
		sample_freq = 3;
		audio_clock_freq = 320;
		break;
	case GB02MAC1787:
		dptx_dbg(dptx, "GB02MAC1787\n");
		orig_sample_freq = 15;
		sample_freq = 0;
		audio_clock_freq = 441;
		break;
	case GB02MAC1789:
		dptx_dbg(dptx, "GB02MAC1789\n");
		orig_sample_freq = 13;
		sample_freq = 2;
		audio_clock_freq = 480;
		break;
	case GB02MAC1791:
		dptx_dbg(dptx, "GB02MAC1791\n");
		orig_sample_freq = 7;
		sample_freq = 8;
		audio_clock_freq = 882;
		break;
	case GB02MAC1793:
		dptx_dbg(dptx, "GB02MAC1793\n");
		orig_sample_freq = 5;
		sample_freq = 10;
		audio_clock_freq = 960;
		break;
	case GB02MAC1795:
		dptx_dbg(dptx, "GB02MAC1795\n");
		orig_sample_freq = 3;
		sample_freq = 12;
		audio_clock_freq = 1764;
		break;
	case GB02MAC1797:
		dptx_dbg(dptx, "GB02MAC1797\n");
		orig_sample_freq = 1;
		sample_freq = 14;
		audio_clock_freq = 1920;
		break;
	default:
		dptx_dbg(dptx, "Invalid TEST_AUDIO_SAMPLING_RATE\n");
		return -EINVAL;
	}
	dptx_dbg(dptx, "sample_freq = %d\n", sample_freq);
	dptx_dbg(dptx, "orig_sample_freq = %d\n", orig_sample_freq);

	retval = GB02FUNC1172(dptx, audio_clock_freq, 512);
	if (retval)
		return retval;

	aparams->iec_samp_freq = sample_freq;
	aparams->iec_orig_samp_freq = orig_sample_freq;

	GB02FUNC470(dptx);
	GB02FUNC483(dptx);
	GB02FUNC404(dptx);

	return retval;
}
//#endif
static int GB02FUNC1225(struct dptx *dptx, int stream)
{
	int retval;
	u8 misc, pattern, bpc, bpc_map, dynamic_range,
	   dynamic_range_map, color_format, color_format_map,
	   ycbcr_coeff,  ycbcr_coeff_map;
	struct GB02STR124 *vparams;
	struct dtd *mdtd;

	vparams = &dptx->vparams;
	mdtd = &vparams->mdtd;
	retval = 0;

	retval = GB02FUNC1151(dptx, DP_TEST_PATTERN, &pattern);
	if (retval)
		return retval;
	retval = GB02FUNC1151(dptx, GB02MAC1749, &misc);
	if (retval)
		return retval;

	dynamic_range = (misc & GB02MAC1754)
			>> GB02MAC1752;
	switch (dynamic_range) {
//#ifndef	CONFIG_UOS_20
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	case GB02MAC1830:
		dptx_dbg(dptx, "GB02MAC1830\n");
		dynamic_range_map = VESA;
		break;
//#endif
//#endif
	case DP_TEST_DYNAMIC_RANGE_CEA:
		dptx_dbg(dptx, "DP_TEST_DYNAMIC_RANGE_CEA\n");
		dynamic_range_map = CEA;
		break;
	default:
		dptx_dbg(dptx, "Invalid TEST_BIT_DEPTH\n");
		return -EINVAL;
	}

	ycbcr_coeff = (misc & GB02MAC1758)
			>> GB02MAC1756;

	switch (ycbcr_coeff) {
	case GB02MAC1772:
		dptx_dbg(dptx, "GB02MAC1772\n");
		ycbcr_coeff_map = ITU601;
		break;
	case GB02MAC1775:
		dptx_dbg(dptx, "GB02MAC1775:\n");
		ycbcr_coeff_map = ITU709;
		break;
	default:
		dptx_dbg(dptx, "Invalid TEST_BIT_DEPTH\n");
		return -EINVAL;
	}
	color_format = misc & DP_TEST_COLOR_FORMAT_MASK;

	switch (color_format) {
	case GB02MAC1763:
		dptx_dbg(dptx, "GB02MAC1763\n");
		color_format_map = RGB;
		break;
	case GB02MAC1766:
		dptx_dbg(dptx, "GB02MAC1766\n");
		color_format_map = YCBCR422;
		break;
	case GB02MAC1769:
		dptx_dbg(dptx, "GB02MAC1769\n");
		color_format_map = YCBCR444;
		break;
	default:
		dptx_dbg(dptx, "Invalid  DP_TEST_COLOR_FORMAT\n");
		return -EINVAL;
	}

	bpc = (misc & DP_TEST_BIT_DEPTH_MASK)
		>> DP_TEST_BIT_DEPTH_SHIFT;

	switch (bpc) {
	case DP_TEST_BIT_DEPTH_6:
		bpc_map = COLOR_DEPTH_6;
		dptx_dbg(dptx, "TEST_BIT_DEPTH_6\n");
		break;
	case DP_TEST_BIT_DEPTH_8:
		bpc_map = COLOR_DEPTH_8;
		dptx_dbg(dptx, "TEST_BIT_DEPTH_8\n");
		break;
	case DP_TEST_BIT_DEPTH_10:
		bpc_map = COLOR_DEPTH_10;
		dptx_dbg(dptx, "TEST_BIT_DEPTH_10\n");
		break;
	case DP_TEST_BIT_DEPTH_12:
		bpc_map = COLOR_DEPTH_12;
		dptx_dbg(dptx, "TEST_BIT_DEPTH_12\n");
		break;
	case DP_TEST_BIT_DEPTH_16:
		bpc_map = COLOR_DEPTH_16;
		dptx_dbg(dptx, "TEST_BIT_DEPTH_16\n");
		break;
	default:
		dptx_dbg(dptx, "Invalid TEST_BIT_DEPTH\n");
		return -EINVAL;
	}

	vparams->dynamic_range = dynamic_range_map;
	dptx_dbg(dptx, "Change video dynamic range to %d\n", dynamic_range_map);

	vparams->colorimetry = ycbcr_coeff_map;
	dptx_dbg(dptx, "Change video colorimetry to %d\n", ycbcr_coeff_map);

	retval = GB02FUNC682(dptx, dptx->link.lanes,
					 dptx->link.rate,
					 bpc_map, color_format_map,
					 mdtd->pixel_clock);
	if (retval)
		return retval;

	vparams->pix_enc = color_format_map;
	dptx_dbg(dptx, "Change pixel encoding to %d\n", color_format_map);

	vparams->bpc = bpc_map;
	GB02FUNC706(dptx, stream);
	dptx_dbg(dptx, "Change bits per component to %d\n", bpc_map);

	GB02FUNC504(dptx, stream);
	GB02FUNC699(dptx, stream);

	switch (pattern) {
	case GB02MAC1697:
		dptx_dbg(dptx, "TEST_PATTERN_NONE %d\n", pattern);
		break;
	case GB02MAC1700:
		dptx_dbg(dptx, "TEST_PATTERN_COLOR_RAMPS %d\n", pattern);
		vparams->pattern_mode = RAMP;
		GB02FUNC784(dptx, RAMP, stream);
		GB02FUNC781(dptx, stream);
		dptx_dbg(dptx, "Change video pattern to RAMP\n");
		break;
	case GB02MAC1703:
		dptx_dbg(dptx, "TEST_PATTERN_BW_VERTICAL_LINES %d\n", pattern);
		break;
	case GB02MAC1706:
		dptx_dbg(dptx, "TEST_PATTERN_COLOR_SQUARE %d\n", pattern);
		vparams->pattern_mode = COLRAMP;
		GB02FUNC784(dptx, COLRAMP, stream);
		GB02FUNC781(dptx, stream);
		dptx_dbg(dptx, "Change video pattern to COLRAMP\n");
		break;
	default:
		dptx_dbg(dptx, "Invalid TEST_PATTERN %d\n", pattern);
		return -EINVAL;
	}

	retval = GB02FUNC1202(dptx, stream);
	if (retval)
		return retval;

	return 0;
}
static int GB02FUNC1241(struct dptx *dptx)
{
	int retval;
	u8 pattern0 , pattern1, pattern2, pattern3, pattern4, pattern5,
	   pattern6, pattern7, pattern8, pattern9;

	u32 custompat0;
	u32 custompat1;
	u32 custompat2;

	retval = GB02FUNC1151(dptx, GB02MAC1709, &pattern0);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1712, &pattern1);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1715, &pattern2);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1718, &pattern3);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1721, &pattern4);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1723, &pattern5);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1725, &pattern6);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1726, &pattern7);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1728, &pattern8);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, GB02MAC1730, &pattern9);
	if (retval)
		return retval;

	/*
	 *  Calculate 30,30 and 20 bits custom patterns depending on TEST_80BIT_CUSTOM_PATTERN sequence
	 */
	custompat0 = ((((((pattern3 & (0xff >> 2)) << 8) | pattern2) << 8) | pattern1) << 8) | pattern0;
	custompat1 = ((((((((pattern7 & (0xf)) << 8) | pattern6) << 8) | pattern5) << 8) | pattern4) << 2) | ((pattern3 >> 6) & 0x3);
	custompat2 = (((pattern9 << 8) | pattern8) << 4) | ((pattern7 >> 4) & 0xf);

	dptx_writel(dptx, GB02MAC2109, custompat0);
	dptx_writel(dptx, GB02MAC2110, custompat1);
	dptx_writel(dptx, GB02MAC2111, custompat2);

	return 0;
}
static int GB02FUNC1250(struct dptx *dptx)
{
	int retval;
	int i;
	u8 lane_01;
	u8 lane_23;

	retval = GB02FUNC1151(dptx, DP_ADJUST_REQUEST_LANE0_1, &lane_01);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, DP_ADJUST_REQUEST_LANE2_3, &lane_23);
	if (retval)
		return retval;

	for (i = 0; i < dptx->link.lanes; i++) {
		u8 pe = 0;
		u8 vs = 0;

		switch (i)
		{
			case 0:
				pe = (lane_01 &  DP_ADJUST_PRE_EMPHASIS_LANE0_MASK)
					>> DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT;
				vs = (lane_01 &  DP_ADJUST_VOLTAGE_SWING_LANE0_MASK)
					>> DP_ADJUST_VOLTAGE_SWING_LANE0_SHIFT;
				break;
			case 1:
				pe = (lane_01 & DP_ADJUST_PRE_EMPHASIS_LANE1_MASK)
					>> DP_ADJUST_PRE_EMPHASIS_LANE1_SHIFT;
				vs = (lane_01 & DP_ADJUST_VOLTAGE_SWING_LANE1_MASK)
					>> DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT;
				break;
			case 2:
				pe = (lane_23 & DP_ADJUST_PRE_EMPHASIS_LANE0_MASK)
					>> DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT;
				vs = (lane_23 & DP_ADJUST_VOLTAGE_SWING_LANE0_MASK)
					>> DP_ADJUST_VOLTAGE_SWING_LANE0_SHIFT;
				break;
			case 3:
				pe = (lane_23 & DP_ADJUST_PRE_EMPHASIS_LANE1_MASK)
					>> DP_ADJUST_PRE_EMPHASIS_LANE1_SHIFT;
				vs = (lane_23 & DP_ADJUST_VOLTAGE_SWING_LANE1_MASK)
					>> DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT;
				break;
		default:
			break;
		}

		GB02FUNC1100(dptx, i, pe);
		GB02FUNC1114(dptx, i, vs);
		GB02FUNC1105(dptx,i,pe,vs);
	}

	return 0;
}
static int GB02FUNC1256(struct dptx *dptx)
{
	u8 pattern;
	int retval;

	retval = GB02FUNC1151(dptx, GB02MAC1695, &pattern);
	if (retval)
		return retval;

	pattern &= DP_TEST_PHY_PATTERN_SEL_MASK;

	switch (pattern) {
	case GB02MAC1733:
		retval = GB02FUNC1250(dptx);
		if (retval)
			return retval;
		dptx_dbg(dptx, "No test pattern selected\n");
		GB02FUNC1118(dptx, GB02MAC2173);
		break;
	case GB02MAC1735:
		retval = GB02FUNC1250(dptx);
		if (retval)
			return retval;
		dptx_dbg(dptx, "D10.2 without scrambling test phy pattern\n");
		GB02FUNC1118(dptx, GB02MAC2174);
		break;
	case GB02MAC1737:
		retval = GB02FUNC1250(dptx);
		if (retval)
			return retval;
		dptx_dbg(dptx, "Symbol error measurement count test phy pattern\n");
		GB02FUNC1118(dptx, GB02MAC2178);
		break;
	case GB02MAC1739:
		retval = GB02FUNC1250(dptx);
		if (retval)
			return retval;
		dptx_dbg(dptx, "PRBS7 test phy pattern\n");
		GB02FUNC1118(dptx, GB02MAC2179);
		break;
	case GB02MAC1741:
		retval = GB02FUNC1250(dptx);
		if (retval)
			return retval;
		dptx_dbg(dptx, "80-bit custom pattern transmitted test phy pattern\n");
		//dptx_writel(dptx, GB02MAC2109, 0x3E0F83E0);
		//dptx_writel(dptx, GB02MAC2110, 0x3E0F83E0);
		//dptx_writel(dptx, GB02MAC2111, 0xF83E0);

		retval = GB02FUNC1241(dptx);
		if (retval)
			return retval;
		GB02FUNC1118(dptx, GB02MAC2180);
		break;
	case GB02MAC1743:
		retval = GB02FUNC1250(dptx);
		if (retval)
			return retval;
		dptx_dbg(dptx, "CP2520_1 - HBR2 Compliance EYE pattern\n");
		GB02FUNC1118(dptx, GB02MAC2181);
		break;
	case GB02MAC1745:
		retval = GB02FUNC1250(dptx);
		if (retval)
			return retval;
		dptx_dbg(dptx, "CP2520_2 - pattern\n");
		GB02FUNC1118(dptx, GB02MAC2182);
		break;
	case GB02MAC1747:
		retval = GB02FUNC1250(dptx);
		if (retval)
			return retval;
		dptx_dbg(dptx, "GB02MAC1747 - pattern\n");
		GB02FUNC1118(dptx, GB02MAC2177);
		break;
	default:
		dptx_dbg(dptx, "Invalid TEST_PHY_PATTERN\n");
		return -EINVAL;
	}
	return retval;
}
#endif
static int GB02FUNC1265(struct dptx *dptx)
{
	int retval;
	u8 test;

	retval = GB02FUNC1151(dptx, DP_TEST_REQUEST, &test);
	if (retval)
		return retval;

	if (test & DP_TEST_LINK_TRAINING) {
		dptx_dbg(dptx, "%s: DP_TEST_LINK_TRAINING\n", __func__);

		retval = GB02FUNC1153(dptx, DP_TEST_RESPONSE, DP_TEST_ACK);
		if (retval)
			return retval;

		retval = GB02FUNC1198(dptx);
		if (retval)
			return retval;
	}

	if (test & DP_TEST_LINK_VIDEO_PATTERN) {
		dptx_dbg(dptx, "%s:DP_TEST_LINK_VIDEO_PATTERN\n", __func__);

		retval = GB02FUNC1153(dptx, DP_TEST_RESPONSE, DP_TEST_ACK);
		if (retval)
			return retval;

		retval = GB02FUNC1225(dptx, 0);
		if (retval)
			return retval;
	}
//#ifndef	CONFIG_UOS_20
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	if (test & DP_TEST_LINK_AUDIO_PATTERN) {
		dptx_dbg(dptx, "%s:DP_TEST_LINK_AUDIO_PATTERN\n", __func__);

		retval = GB02FUNC1153(dptx, DP_TEST_RESPONSE, DP_TEST_ACK);
		if (retval)
			return retval;

		retval = GB02FUNC1218(dptx);
		if (retval)
			return retval;
	}
//#endif
//#endif
	if (test & DP_TEST_LINK_EDID_READ) {
		/* Invalid, this should happen on HOTPLUG */
		dptx_dbg(dptx, "%s:DP_TEST_LINK_EDID_READ\n", __func__);
		return -ENOTSUPP;
	}
	if (test & DP_TEST_LINK_PHY_TEST_PATTERN) {
		dptx_dbg(dptx, "%s:DP_TEST_LINK_PHY_TEST_PATTERN\n", __func__);
		retval = GB02FUNC1256(dptx);
		if (retval)
			return retval;
	}
	return 0;
}
static int GB02FUNC1276(struct dptx *dptx)
{
	int retval;
	u8 vector;
	u8 bytes[1];
	u32 reg;

	gb_printf(KERN_DEBUG, "%s:%d------in sink request---\n", __func__, __LINE__);
	retval = GB02FUNC1585(dptx);
	if (retval)
		return retval;


	gb_printf(KERN_DEBUG, "%s:%d------link status ok---\n", __func__, __LINE__);
    	retval = GB02FUNC1154(dptx,
		DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0,
		bytes, sizeof(bytes)); //sahakyan
    	if (retval)
        	return retval;

	retval = GB02FUNC1151(dptx,
		DP_DEVICE_SERVICE_IRQ_VECTOR,
		&vector);
	if (retval)
		return retval;

	dptx_dbg(dptx, "%s: IRQ_VECTOR: 0x%02x\n", __func__, vector);

    	/* TODO handle sink interrupts */
	if (!vector)
		return 0;

	if ((vector & DP_REMOTE_CONTROL_COMMAND_PENDING) || (bytes[0] & DP_REMOTE_CONTROL_COMMAND_PENDING)) {
		/* TODO */
		dptx_warn(dptx,
			  "%s: DP_REMOTE_CONTROL_COMMAND_PENDING: Not yet implemented",
			  __func__);
	}
#if 1
	if ((vector & DP_AUTOMATED_TEST_REQUEST) || (bytes[0] & DP_AUTOMATED_TEST_REQUEST)) {
		dptx_dbg(dptx, "%s: DP_AUTOMATED_TEST_REQUEST", __func__);
		retval = GB02FUNC1265(dptx);
		if (retval) {
			dptx_err(dptx, "Automated test request failed\n");
			if (retval == -ENOTSUPP) {
				retval = GB02FUNC1153(dptx, DP_TEST_RESPONSE,
							 DP_TEST_NAK);
				if (retval)
					return retval;
			}
		}
	}
#endif
	if ((vector & DP_CP_IRQ) || (bytes[0] & DP_CP_IRQ)) {
		dptx_warn(dptx, "%s: DP_CP_IRQ", __func__);
		retval = GB02FUNC1153(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR,
					 DP_CP_IRQ);
		reg = dptx_readl(dptx, GB02MAC2130);
		reg |= GB02MAC2353;


		dptx_writel(dptx, GB02MAC2130, reg);
		reg = dptx_readl(dptx, GB02MAC2130);
		dptx_warn(dptx, "%s: DP_CP_IRQ1--- 0x%x", __func__, reg);

		reg &= ~GB02MAC2353;
		dptx_writel(dptx, GB02MAC2130, reg);
		if (retval)
			return retval;
        }
	if ((vector & DP_MCCS_IRQ) || (bytes[0] & DP_MCCS_IRQ)) {
		/* TODO */
		dptx_warn(dptx,
			  "%s: DP_MCCS_IRQ: Not yet implemented", __func__);
		retval = -ENOTSUPP;
	}

	if ((vector & DP_UP_REQ_MSG_RDY) || (bytes[0] & DP_UP_REQ_MSG_RDY)) {
        dptx_warn(dptx, "%s: DP_UP_REQ_MSG_RDY", __func__);
        retval = GB02FUNC1367(dptx);
		if (retval) {
			dptx_err(dptx, "%s: Error reading UP REQ (%d)\n", __func__, retval);
			return retval;
		}

    }

	if ((vector & DP_SINK_SPECIFIC_IRQ) || (bytes[0] & DP_SINK_SPECIFIC_IRQ)) {
		/* TODO */
		dptx_warn(dptx, "%s: DP_SINK_SPECIFIC_IRQ: Not yet implemented",
			  __func__);
		retval = -ENOTSUPP;
	}

	return retval;
}
static int GB02FUNC1286(struct dptx *dptx)
{
	u32 phyifctrl;
	u8  retval;
	u32 reg;

	dptx->dummy_dtds_present = false;
	dev_dbg(dptx->dev, "Disabling Forward Error Correction\n");
	// Disable forward error correction
	reg = dptx_readl(dptx, GB02MAC2057);
	reg &= ~GB02MAC2161;
	dptx_writel(dptx, GB02MAC2057, reg);

	/* Clear xmit enables */
	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	phyifctrl &= ~DPTX_PHYIF_CTRL_XMIT_EN_MASK;

	/* Move PHY to P3 state */
	phyifctrl |= (3 << GB02MAC2198);
	dptx_writel(dptx, GB02MAC2107, phyifctrl);

	retval = GB02FUNC1095(dptx, dptx->link.lanes);
	if (retval) {
		dptx_err(dptx, "Timed out waiting for PHY BUSY\n");
		return retval;
	}
	/* Power down all lanes */
	gb_printf(KERN_INFO, "%s:%d-----hot unplug finish--\n",
			__func__, __LINE__);
	atomic_set(&dptx->sink_request, 0);
	dptx->link.trained = false;
	dptx->edid_getted = false;

#ifdef  GB02MAC292
	GB02FUNC547(dptx->index, GB02MAC19,
		GB02MAC31, GB02MAC41, DP_DISCONNECT);
#endif

	return 0;
}
#define GB02MAC1947 1024
static int GB02FUNC1293(u8 *buf, int len)
{
	int i;
	char *str;
	int written = 0;
	str = vmalloc(GB02MAC1947);

	written += snprintf(&str[written], GB02MAC1947 - written, "Buffer:");

	for (i = 0; i < len; i++) {
		if (!(i % 16)) {
			written += snprintf(&str[written],
					    GB02MAC1947 - written,
					    "\n%04x:", i);

			if (written >= GB02MAC1947)
				break;

		}

		written += snprintf(&str[written],
				    GB02MAC1947 - written,
				    " %02x", buf[i]);

		if (written >= GB02MAC1947)
			break;
	}

	gb_printf(KERN_INFO, "%s\n\n", str);
	vfree(str);
	return 0;
}

static int GB02FUNC1300(struct dptx *dptx,
				unsigned int block)
{
	int retval;
	int retry = 0;
	int i;
	int j;
	u8 offset = block * GB02MAC1428;
	u8 segment = block >> 1;
	u8 tmp_offset = offset;
	int cnt = 1;
	int retry_head = 0;
#if 0
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);
	int cnt = 1;
	/*HMDI reading 16 byte edid data through aux needs to be repeated twice*/
	if (gb_type == PCIE_LPDDR4) {
		if (dptx->index == 2)
			cnt = 2;
	} else if (gb_type == PCIE_FULL_LPDDR4) {
		if (dptx->index == 5)
			cnt = 2;
	} else if (gb_type == PCIE_M6FL8G_LPDDR4) {
		if ((dptx->index != 0) && (dptx->index != 2))
			cnt = 2;
	}	else if (gb_type == PCIE_HIE1LP4_LPDDR4) {
		if ((dptx->index != 1) && (dptx->index != 4))
			cnt = 2;
	}
#endif
	dptx_dbg(dptx, "%s: block=%d\n",
		 __func__, block);
again:
#if 0
	retval = GB02FUNC376(dptx, 0x30, &segment, 1);
	/* TODO Skip if no E-DDC */
	retval = GB02FUNC376(dptx, 0x50, &offset, 1);
//	if (retval)
//		return retval;
	retval = GB02FUNC372(dptx, 0x50,
				&dptx->edid[block * GB02MAC1428],
				GB02MAC1428);
	if(retval && !retry) {
		retry = 1;
		goto again;
	}
#else
	for (i = 0; i < GB02MAC1428; i += 16) {
		retval = GB02FUNC376(dptx, 0x30, &segment, 1);
		/* TODO Skip if no E-DDC */
		tmp_offset = offset + i;
		retval = GB02FUNC376(dptx, 0x50, &tmp_offset, 1);
		for (j = 0; j < cnt; j++) {
			retval = GB02FUNC372(dptx, 0x50,
				&dptx->edid[block * GB02MAC1428 + i],
				16);
		}
		if ((block == 0) && (i == 0) && GB02FUNC1316(dptx)) {
			retry_head++;
			gb_printf(KERN_ERR, "%s: retry_head %d\n", __func__, retry_head);
			if (retry_head == 1) {
				//HDMI_power_reset(GB02FUNC518(),dptx->index);
				goto again;
			} else if (retry_head == 2)
				return -EINVAL;

		}
		if (retval && !retry) {
			retry = 1;
			goto again;
		}
	}
#endif
	retval = GB02FUNC375(dptx, 0x50);
//	if (retval)
//		return retval;

	if ((retval == -EINVAL) || (block == 0))
		for (i = 0; i < GB02MAC1428; i++)
			dptx->edid_second[i] = 0x00;
	else
		for (i = 0; i < GB02MAC1428; i++)
			dptx->edid_second[i] =
				dptx->edid[GB02MAC1428 + i];
	pr_info("%s:%d----dptx index=%x---block id = %x-\n",
			__func__, __LINE__, dptx->index, block);
	GB02FUNC1293(&dptx->edid[block * GB02MAC1428],
		GB02MAC1428);

	return 0;
}

static int GB02FUNC1306(struct dptx *dptx)
{
	int retval = 0;

	dptx_dbg(dptx, "%s:\n", __func__);
	memset(dptx->edid, 0, GB02MAC1428 * 3);

	retval = GB02FUNC1300(dptx, 0);
	if (retval)
		return retval;

	dptx_dbg(dptx, "dptx->edid[126] %d\n", dptx->edid[126]);

	if (dptx->edid[126] == 1)
		retval = GB02FUNC1300(dptx, 1);
	else if (dptx->edid[126] == 2) {
		retval = GB02FUNC1300(dptx, 1);
		retval |= GB02FUNC1300(dptx, 2);
	} else
		gb_printf(KERN_WARNING, "edid[126] > 2\n");

	return retval;
}
static int GB02FUNC1311(const u8 *raw_edid)
{
	int i;
	u8 csum = 0;

	for (i = 0; i < EDID_LENGTH; i++)
		csum += raw_edid[i];

	return csum;
}
int GB02FUNC1313(struct dptx *dptx)
{
	int i;
	u32 edid_sum = 0;
	struct edid *monitor_edid = (struct edid *)dptx->edid;

	const u8 edid_header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	for (i = 0; i < sizeof(edid_header); i++) {
		if (dptx->edid[i] != edid_header[i]) {
			dptx_err(dptx, "Invalid EDID header\n");
			return -EINVAL;
		}
	}

	for (i = 0; i <= monitor_edid->extensions; i++) {
		edid_sum = GB02FUNC1311(dptx->edid + i * EDID_LENGTH);
		if (edid_sum & 0xFF) {
			dptx_err(dptx, "Block %d Invalid EDID checksum\n", i);
			return -EINVAL;
		}
	}
	return 0;
}
int GB02FUNC1316(struct dptx *dptx)
{
	int i;
	const u8 edid_header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	for (i = 0; i < sizeof(edid_header); i++) {
		if (dptx->edid[i] != edid_header[i]) {
			dptx_err(dptx, "%s:EDID head err,edid[%d]=0x%x\n", __func__,
				 i, dptx->edid[i]);
			return -EINVAL;
		}
	}

	return 0;
}

static void GB02FUNC1318(struct dptx *dptx)
{
	int i;

	for (i = 0; i < 8; i++)
		dptx_writel(dptx, GB02MAC2061(i), 0);
}

static void GB02FUNC1319(struct dptx *dptx,
				      u32 slot, u32 stream)
{
	u32 offset;
	u32 reg;
	u32 lsb;
	u32 mask;

	if (slot > 63) {
		dptx_err(dptx, "Invalid slot number > 63\n");
		return;
	}

	offset = GB02MAC2061(slot >> 3);
	reg = dptx_readl(dptx, offset);

	lsb = (slot & 0x7) * 4;
	mask = GENMASK(lsb + 3, lsb);

	reg &= ~mask;
	reg |= (stream << lsb) & mask;

	dptx_dbg(dptx, "%s: Writing 0x%08x val=0x%08x\n", __func__, offset, reg);
	dptx_writel(dptx, offset, reg);
}

static void GB02FUNC1321(struct dptx *dptx,
				       u32 start, u32 count,
				       u32 stream)
{
	int i;

	if ((start + count) > 64) {
		dptx_err(dptx, "Invalid slot number > 63\n");
		return;
	}

	for (i = 0; i < count; i++) {
		dptx_dbg(dptx, "--------- %s: setting slot %d for stream %d\n",
			 __func__, start + i, stream);
		GB02FUNC1319(dptx, start + i, stream);
	}
}

static void GB02FUNC1322(struct dptx *dptx)
{
	u32 reg;
	int count = 0;

	reg = dptx_readl(dptx, GB02MAC2057);
	reg |= GB02MAC2162;
	dptx_writel(dptx, GB02MAC2057, reg);

	while (1) {
		reg = dptx_readl(dptx, GB02MAC2057);
		if (!(reg & GB02MAC2162))
			break;

		count ++;
		if (count > 100) {
			dptx_err(dptx, "CCTL.ACT timeout\n");
			break;
		}

		mdelay(10);
	}
}

static void GB02FUNC1324(struct dptx *dptx)
{
	u8 bytes[] = { 0x00, 0x00, 0x3f, };
	u8 status;
	int count = 0;

	dptx_dbg(dptx, "%s:\n", __func__);

	GB02FUNC1151(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
	GB02FUNC1153(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, DP_PAYLOAD_TABLE_UPDATED);
	GB02FUNC1151(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);

	GB02FUNC1156(dptx, DP_PAYLOAD_ALLOCATE_SET, bytes, 3);

	status = 0;
	while (!(status & DP_PAYLOAD_TABLE_UPDATED)) {
		GB02FUNC1151(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
		count ++;
		if (count > 2000) {
			dptx_err(dptx, "Timeout waiting for DPCD VCPID table update\n");
			break;
		}

		udelay(1000);
	}

	GB02FUNC1153(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, DP_PAYLOAD_TABLE_UPDATED);
	GB02FUNC1151(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
}

static void GB02FUNC1327(struct dptx *dptx,
				      u32 start, u32 count,
				      u32 stream)
{
	u8 bytes[3];
	u8 status;
	int tries = 0;

	bytes[0] = stream;
	bytes[1] = start;
	bytes[2] = count;

	GB02FUNC1156(dptx, DP_PAYLOAD_ALLOCATE_SET, bytes, 3);

	while (!(status & DP_PAYLOAD_TABLE_UPDATED)) {
		GB02FUNC1151(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
		tries ++;
		if (tries > 2000) {
			dptx_err(dptx, "Timeout waiting for DPCD VCPID table update\n");
			break;
		}
	}

	GB02FUNC1153(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, DP_PAYLOAD_TABLE_UPDATED);
	GB02FUNC1151(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
}

static void GB02FUNC1329(struct dptx *dptx)
{
	u8 bytes[64] = { 0, };

	GB02FUNC1154(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, bytes, 64);
	GB02FUNC1293(bytes, 64);
}

#if 1

/* TODO these are kernel functions. Need to make them accessible. */

static u8 GB02FUNC1330(const uint8_t *data, size_t num_nibbles)
{
	u8 bitmask = 0x80;
	u8 bitshift = 7;
	u8 array_index = 0;
	int number_of_bits = num_nibbles * 4;
	u8 remainder = 0;

	while (number_of_bits != 0) {
		number_of_bits--;
		remainder <<= 1;
		remainder |= (data[array_index] & bitmask) >> bitshift;
		bitmask >>= 1;
		bitshift--;
		if (bitmask == 0) {
			bitmask = 0x80;
			bitshift = 7;
			array_index++;
		}
		if ((remainder & 0x10) == 0x10)
			remainder ^= 0x13;
	}

	number_of_bits = 4;
	while (number_of_bits != 0) {
		number_of_bits--;
		remainder <<= 1;
		if ((remainder & 0x10) != 0)
			remainder ^= 0x13;
	}

	return remainder;
}

static u8 GB02FUNC1332(const uint8_t *data, u8 number_of_bytes)
{
	u8 bitmask = 0x80;
	u8 bitshift = 7;
	u8 array_index = 0;
	int number_of_bits = number_of_bytes * 8;
	u16 remainder = 0;

	while (number_of_bits != 0) {
		number_of_bits--;
		remainder <<= 1;
		remainder |= (data[array_index] & bitmask) >> bitshift;
		bitmask >>= 1;
		bitshift--;
		if (bitmask == 0) {
			bitmask = 0x80;
			bitshift = 7;
			array_index++;
		}
		if ((remainder & 0x100) == 0x100)
			remainder ^= 0xd5;
	}

	number_of_bits = 8;
	while (number_of_bits != 0) {
		number_of_bits--;
		remainder <<= 1;
		if ((remainder & 0x100) != 0)
			remainder ^= 0xd5;
	}

	return remainder & 0xff;
}

static void GB02FUNC1337(struct drm_dp_sideband_msg_hdr *hdr,
					   u8 *buf, int *len)
{
	int idx = 0;
	int i;
	u8 crc4;
	buf[idx++] = ((hdr->lct & 0xf) << 4) | (hdr->lcr & 0xf);
	for (i = 0; i < (hdr->lct / 2); i++)
		buf[idx++] = hdr->rad[i];
	buf[idx++] = (hdr->broadcast << 7) | (hdr->path_msg << 6) |
		(hdr->msg_len & 0x3f);
	buf[idx++] = (hdr->somt << 7) | (hdr->eomt << 6) | (hdr->seqno << 4);

	crc4 = GB02FUNC1330(buf, (idx * 2) - 1);
	buf[idx - 1] |= (crc4 & 0xf);
	*len = idx;
}

static void GB02FUNC1340(u8 *msg, u8 len)
{
	u8 crc4;
	crc4 = GB02FUNC1332(msg, len);
	msg[len] = crc4;
}

static bool GB02FUNC1342(struct drm_dp_sideband_msg_hdr *hdr,
					   u8 *buf, int buflen, u8 *hdrlen)
{
	u8 crc4;
	u8 len;
	int i;
	u8 idx;
	if (buf[0] == 0)
		return false;
	len = 3;
	len += ((buf[0] & 0xf0) >> 4) / 2;
	if (len > buflen)
		return false;
	crc4 = GB02FUNC1330(buf, (len * 2) - 1);

	if ((crc4 & 0xf) != (buf[len - 1] & 0xf)) {
		//DRM_DEBUG_KMS("crc4 mismatch 0x%x 0x%x\n", crc4, buf[len - 1]);
		return false;
	}

	hdr->lct = (buf[0] & 0xf0) >> 4;
	hdr->lcr = (buf[0] & 0xf);
	idx = 1;
	for (i = 0; i < (hdr->lct / 2); i++)
		hdr->rad[i] = buf[idx++];
	hdr->broadcast = (buf[idx] >> 7) & 0x1;
	hdr->path_msg = (buf[idx] >> 6) & 0x1;
	hdr->msg_len = buf[idx] & 0x3f;
	idx++;
	hdr->somt = (buf[idx] >> 7) & 0x1;
	hdr->eomt = (buf[idx] >> 6) & 0x1;
	hdr->seqno = (buf[idx] >> 4) & 0x1;
	idx++;
	*hdrlen = idx;
	return true;
}

#endif

static char const *GB02FUNC1348(struct drm_dp_sideband_msg_hdr *header)
{

	if (header->lct >= 1)
		return "TODO";

	return "none";
}

static void GB02FUNC1351(struct dptx *dptx,
				       struct drm_dp_sideband_msg_hdr *header)
{
	dptx_dbg(dptx, "SIDEBAND_MSG_HEADER: "
		 "lct=%d, lcr=%d, rad=%s, bcast=%d, "
		 "path=%d, msglen=%d, somt=%d, eomt=%d, seqno=%d\n",
		 header->lct, header->lcr, GB02FUNC1348(header),
		 header->broadcast, header->path_msg, header->msg_len,
		 header->somt, header->eomt, header->seqno);
}
static void GB02FUNC1354(struct drm_dp_sideband_msg_rx *raw,
							   struct drm_dp_sideband_msg_req_body *msg)
{
    int idx = 1;

	msg->u.conn_stat.port_number = (raw->msg[idx] & 0xf0) >> 4;
	idx++;

	memcpy(msg->u.conn_stat.guid, &raw->msg[idx], 16);
	idx += 16;

	msg->u.conn_stat.legacy_device_plug_status = (raw->msg[idx] >> 6) & 0x1;
	msg->u.conn_stat.displayport_device_plug_status = (raw->msg[idx] >> 5) & 0x1;
	msg->u.conn_stat.message_capability_status = (raw->msg[idx] >> 4) & 0x1;
	msg->u.conn_stat.input_port = (raw->msg[idx] >> 3) & 0x1;
	msg->u.conn_stat.peer_device_type = (raw->msg[idx] & 0x7);
	idx++;
}
static int GB02FUNC1357(struct dptx *dptx, u8 port, int port1)
{
	struct drm_dp_sideband_msg_hdr header;
	u8 buf[256];
	int len = 256;
	u8 *msg;

	memset(&header, 0, sizeof(struct drm_dp_sideband_msg_hdr));

    	header.lct = 1;
	header.lcr = 0;
	header.rad[0] = 0;
	header.broadcast = false;
	header.path_msg = 0;
	header.msg_len = 9;
	header.somt = 1;
	header.eomt = 1;
	header.seqno = 0;

	if ((port1 >= 0) && dptx->need_rad) {
		header.lct = 2;
		header.lcr = 1;
		header.rad[0] |= ((dptx->rad_port << 4) & 0xf0);
		header.rad[0] |= (0 & 0xf);
	}


	GB02FUNC1337(&header, buf, &len);

	GB02FUNC1293(buf, len);
	msg = &buf[len];
	msg[0] = DP_REMOTE_I2C_READ;
	msg[1] = ((port & 0xf) << 4);
	msg[1] |= 1 & 0x3;
	msg[2] = 0x50 & 0x7f;
	msg[3] = 1;
	msg[4] = 0 << 5;
	msg[5] = 0 & 0xf;
	msg[6] = 0x50 & 0x7f;
	msg[7] = 0x80;

	GB02FUNC1340(msg, 8);

	len += 9;

	dptx_dbg(dptx, "%s: Sending DOWN_REQ\n", __func__);
	GB02FUNC1156(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE,
				 buf, len);

	GB02FUNC1381(dptx, DP_REMOTE_I2C_READ, NULL);
	return 0;
}

static int GB02FUNC1362(struct dptx *dptx, u8 port, int port1)
{
	struct drm_dp_sideband_msg_hdr header;
	u8 buf[256];
	int len = 256;
	u8 *msg;

	memset(&header, 0, sizeof(struct drm_dp_sideband_msg_hdr));

    	header.lct = 1;
	header.lcr = 0;
	header.rad[0] = 0;
	header.broadcast = false;
	header.path_msg = 0;
	header.msg_len = 3;
	header.somt = 1;
	header.eomt = 1;
	header.seqno = 0;

	if ((port1 >= 0) && dptx->need_rad) {
		header.lct = 2;
		header.lcr = 1;
		header.rad[0] |= ((dptx->rad_port << 4) & 0xf0);
		header.rad[0] |= (0 & 0xf);
	}

	GB02FUNC1337(&header, buf, &len);

	GB02FUNC1293(buf, len);
	msg = &buf[len];
	msg[0] = DP_ENUM_PATH_RESOURCES;
	msg[1] = ((port & 0xf) << 4);

	GB02FUNC1340(msg, 2);

	len += 3;

	dptx_dbg(dptx, "%s: Sending DOWN_REQ\n", __func__);
	GB02FUNC1156(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE,
				 buf, len);

	GB02FUNC1381(dptx, DP_ENUM_PATH_RESOURCES, NULL);
	return 0;
}

static int GB02FUNC1367(struct dptx *dptx)
{
	struct drm_dp_sideband_msg_hdr header;
	u8 buf[256];
	u8 header_len;
	int retval;
	int first = 1;
	u8 *msg;
	u8 msg_len;
	int retries = 0;

	struct drm_dp_sideband_msg_rx raw;
	struct drm_dp_sideband_msg_req_body req_msg;
	msg = vmalloc(1024);
	memset(&raw, 0, sizeof(raw));
	memset(&req_msg, 0, sizeof(req_msg));

again:
	memset(msg, 0, 1024);
	msg_len = 0;

	while (1) {
		retval = GB02FUNC1154(dptx, DP_SIDEBAND_MSG_UP_REQ_BASE, buf, 256);
		if (retval) {
			dptx_err(dptx, "%s: Error reading up req (%d)\n", __func__, retval);
			vfree(msg);
			return retval;
		}
		if (!GB02FUNC1342(&header, buf, 256, &header_len)) {
			dptx_err(dptx, "%s: Error decoding sideband header (%d)\n", __func__, retval);
			vfree(msg);
			return -EINVAL;
		}

		GB02FUNC1351(dptx, &header);

		header.msg_len -= 1;
		memcpy(&msg[msg_len], &buf[header_len], header.msg_len);
		msg_len += header.msg_len;

		if (first && !header.somt) {
			dptx_err(dptx, "%s: SOMT not set\n", __func__);
			vfree(msg);
			return -EINVAL;
		}
		first = 0;

		GB02FUNC1153(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, DP_UP_REQ_MSG_RDY);

		if (header.eomt)
			break;
	}

	GB02FUNC1293(msg, msg_len);
	if ((msg[0] & 0x7f) != DP_CONNECTION_STATUS_NOTIFY) {
		if (retries < 3) {
			dptx_err(dptx, "%s: request_id %d does not match expected %d, retrying\n", __func__, msg[0] & 0x7f, DP_UP_REQ_MSG_RDY);
			retries ++;
			goto again;
		} else {
			dptx_err(dptx, "%s: request_id %d does not match expected %d, giving up\n", __func__, msg[0] & 0x7f, DP_UP_REQ_MSG_RDY);
			vfree(msg);
			return -EINVAL;
		}
	}

	memcpy(raw.msg, &msg, msg_len);

	GB02FUNC1293(buf, 256);
	GB02FUNC1293(raw.msg, 256);
	GB02FUNC1354(&raw, &req_msg);

	gb_printf(KERN_ERR, "sahakyan: %d", req_msg.u.conn_stat.port_number);

	GB02FUNC1293(msg, msg_len);
	if (req_msg.u.conn_stat.displayport_device_plug_status) {
		dptx->vcp_id++;
		gb_printf(KERN_ERR, "sahakyan %s:%d", __func__, __LINE__);
		GB02FUNC1396(dptx, req_msg.u.conn_stat.port_number, dptx->vcp_id, dptx->pbn, -1);
	} else
		dptx->vcp_id--;
	vfree(msg);
	return 0;
}
static int GB02FUNC1374(struct dptx *dptx)
{
	int count = 0;
	u8 vector;
	u8 bytes[1];

	dptx_dbg(dptx, "%s:\n", __func__);
	while (1) {

	GB02FUNC1154(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0, bytes, sizeof(bytes)); //sahakyan
    GB02FUNC1151(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, &vector);

    if (vector & DP_DOWN_REP_MSG_RDY || bytes[0] & DP_DOWN_REP_MSG_RDY) {
			dptx_dbg(dptx, "%s: vector set\n", __func__);
			break;
		}

		count ++;
		if (count > 1000) {
			dptx_dbg(dptx, "%s: Timed out\n", __func__);
			return -ETIMEDOUT;
		}

		udelay(100);
	}

	return 0;
}

static int GB02FUNC1377(struct dptx *dptx)
{
	int count = 0;
	u8 vector;
    u8 bytes[1];

	dptx_dbg(dptx, "%s:\n", __func__);

	while (1) {
		GB02FUNC1151(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, &vector);
        GB02FUNC1154(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0, bytes, sizeof(bytes)); //sahakyan

        if (!(vector & DP_DOWN_REP_MSG_RDY || bytes[0] & DP_DOWN_REP_MSG_RDY)) {
			dptx_dbg(dptx, "%s: vector clear\n", __func__);
			break;
		}

		GB02FUNC1153(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, DP_DOWN_REP_MSG_RDY);
        GB02FUNC1156(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR_ESI0, bytes, sizeof(bytes)); //sahakyan

		count ++;
		if (count > 2000) {
			dptx_dbg(dptx, "%s: Timed out\n", __func__);
			return -ETIMEDOUT;
		}

		udelay(1000);
	}

	return 0;
}

static int GB02FUNC1381(struct dptx *dptx, u8 request_id, u8 *msg_out)
{
	struct drm_dp_sideband_msg_hdr header;
	u8 buf[256];
	u8 header_len;
	int retval;
	int first = 1;
	u8 *msg;
	u8 msg_len;
	int retries = 0;
	msg = vmalloc(1024);
again:
	memset(msg, 0, 1024);
	msg_len = 0;

	dptx_err(dptx, "%s:\n", __func__);
	while (1) {

		retval = GB02FUNC1374(dptx);
		if (retval) {
			dptx_err(dptx, "%s: Error waiting down rep (%d)\n", __func__, retval);
			vfree(msg);
			return retval;
		}

		retval = GB02FUNC1154(dptx, DP_SIDEBAND_MSG_DOWN_REP_BASE, buf, 256);
		if (retval) {
			dptx_err(dptx, "%s: Error reading down rep (%d)\n", __func__, retval);
			vfree(msg);
			return retval;
		}
		if (!GB02FUNC1342(&header, buf, 256, &header_len)) {
			dptx_err(dptx, "%s: Error decoding sideband header (%d)\n", __func__, retval);
			vfree(msg);
			return -EINVAL;
		}

		GB02FUNC1351(dptx, &header);

	/* TODO check sideband msg body crc */
		header.msg_len -= 1;
		memcpy(&msg[msg_len], &buf[header_len], header.msg_len);
		msg_len += header.msg_len;

		if (first && !header.somt) {
			dptx_err(dptx, "%s: SOMT not set\n", __func__);
			vfree(msg);
			return -EINVAL;
		}
		first = 0;

		GB02FUNC1153(dptx, DP_DEVICE_SERVICE_IRQ_VECTOR, DP_DOWN_REP_MSG_RDY);

		if (header.eomt)
			break;
	}

    gb_printf(KERN_ERR, "sahakyan: >>>>>>>>>>>>>>>>>>>>>>>>>\n\n\n\n\n\n\n\n %s:%d", __func__, __LINE__);
	GB02FUNC1293(msg, msg_len);
	if ((msg[0] & 0x7f) != request_id) {
		if (retries < 3) {
			dptx_err(dptx, "%s: request_id %d does not match expected %d, retrying\n", __func__, msg[0] & 0x7f, request_id);
			retries ++;
			goto again;
		} else {
			dptx_err(dptx, "%s: request_id %d does not match expected %d, giving up\n", __func__, msg[0] & 0x7f, request_id);
			vfree(msg);
			return -EINVAL;
		}
	}

	retval = GB02FUNC1377(dptx);
	if (retval) {
		dptx_err(dptx, "%s: Error waiting down rep clear (%d)\n", __func__, retval);
		vfree(msg);
		return retval;
	}

//	return 0;
	if (msg_out)
		memcpy(msg_out, msg, msg_len);
	vfree(msg);
	return msg_len;
}

static int GB02FUNC1385(struct dptx *dptx)
{
    //dp1.4 spec 2.11.6.1
	struct drm_dp_sideband_msg_hdr header = {
		.lct = 1,
		.lcr = 6,
		.rad = { 0, },
		.broadcast = true,
		.path_msg = 1,
		.msg_len = 2,
		.somt = 1,
		.eomt = 1,
		.seqno = 0,
	};

	u8 buf[256];
	int len = 256;
	u8 *msg;


	GB02FUNC1337(&header, buf, &len);

	msg = &buf[len];
	msg[0] = DP_CLEAR_PAYLOAD_ID_TABLE;
	GB02FUNC1340(msg, 1);

	len += 2;

	dptx_dbg(dptx, "%s: Sending DOWN_REQ\n", __func__);
	GB02FUNC1156(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE,
				 buf, len);

	GB02FUNC1381(dptx, DP_CLEAR_PAYLOAD_ID_TABLE, NULL);

	return 0;
}




static bool GB02FUNC1388(struct drm_dp_sideband_msg_rx *raw,
					       struct drm_dp_sideband_msg_reply_body *repmsg)
{
	int idx = 1;
	int i;
	memcpy(repmsg->u.link_addr.guid, &raw->msg[idx], 16);
	idx += 16;
	repmsg->u.link_addr.nports = raw->msg[idx] & 0xf;
	idx++;
	if (idx > raw->curlen)
		goto fail_len;
	for (i = 0; i < repmsg->u.link_addr.nports; i++) {
		if (raw->msg[idx] & 0x80)
			repmsg->u.link_addr.ports[i].input_port = 1;

		repmsg->u.link_addr.ports[i].peer_device_type = (raw->msg[idx] >> 4) & 0x7;
		repmsg->u.link_addr.ports[i].port_number = (raw->msg[idx] & 0xf);

		idx++;
		if (idx > raw->curlen)
			goto fail_len;
		repmsg->u.link_addr.ports[i].mcs = (raw->msg[idx] >> 7) & 0x1;
		repmsg->u.link_addr.ports[i].ddps = (raw->msg[idx] >> 6) & 0x1;
		if (repmsg->u.link_addr.ports[i].input_port == 0)
			repmsg->u.link_addr.ports[i].legacy_device_plug_status = (raw->msg[idx] >> 5) & 0x1;
		idx++;
		if (idx > raw->curlen)
			goto fail_len;
		if (repmsg->u.link_addr.ports[i].input_port == 0) {
			repmsg->u.link_addr.ports[i].dpcd_revision = (raw->msg[idx]);
			idx++;
			if (idx > raw->curlen)
				goto fail_len;
			memcpy(repmsg->u.link_addr.ports[i].peer_guid, &raw->msg[idx], 16);
			idx += 16;
			if (idx > raw->curlen)
				goto fail_len;
			repmsg->u.link_addr.ports[i].num_sdp_streams = (raw->msg[idx] >> 4) & 0xf;
			repmsg->u.link_addr.ports[i].num_sdp_stream_sinks = (raw->msg[idx] & 0xf);
			idx++;

		}
		if (idx > raw->curlen)
			goto fail_len;
	}

	return true;
fail_len:
	gb_printf(KERN_INFO, "link address reply parse length fail %d %d\n", idx, raw->curlen);
	return false;
}


static int GB02FUNC1393(struct dptx *dptx, struct drm_dp_sideband_msg_rx *raw, struct drm_dp_sideband_msg_reply_body *rep, int port)
{
	struct drm_dp_sideband_msg_hdr header;
	u8 buf[256];
	int len = 256;

	u8 *msg;

    memset(&header, 0, sizeof(struct drm_dp_sideband_msg_hdr));

	header.lct = 1;
	header.lcr = 0;
	header.rad[0] = 0;
	header.broadcast = false;
	header.path_msg = 0;
	header.msg_len = 2;
	header.somt = 1;
	header.eomt = 1;
	header.seqno = 0;

	if (port >= 0) {
	   header.lct = 2;
	   header.lcr = 1;
       header.rad[0] |= ((port << 4) & 0xf0);
       header.rad[0] |= (0 & 0xf);
	}

	GB02FUNC1337(&header, buf, &len);

	msg = &buf[len];
	msg[0] = DP_LINK_ADDRESS;


	GB02FUNC1340(msg, 1);

	len += 2;
    GB02FUNC1293(buf, len);
	dptx_dbg(dptx, "%s: Sending DOWN_REQ\n", __func__);
	GB02FUNC1156(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE,
				 buf, len);

	len = GB02FUNC1381(dptx, DP_LINK_ADDRESS, raw->msg);
	raw->curlen = len;
	dptx_dbg(dptx, "%s: rawlen = %d\n", __func__, len);

	GB02FUNC1293(raw->msg, len);
	GB02FUNC1388(raw, rep);

		return 0;
}


//	for(i = 0; i < len-1; i++)
//		gb_printf(KERN_INFO, "Byte %d of Link address response: %x", i, buf[i]);

static int GB02FUNC1396(struct dptx *dptx, u8 port,
					 u8 vcpid, u16 pbn, int port1)
{
	struct drm_dp_sideband_msg_hdr header;
	u8 buf[256];
	int len = 256;
	u8 *msg;
//    int i;

    gb_printf(KERN_ERR, "\n\n\nsahakyan: port for allocate = %d, VCPID = %d, PBN = %d \n\n\n", port, vcpid, pbn);
    memset(&header, 0, sizeof(struct drm_dp_sideband_msg_hdr));
#if 0
    if (port1 >= 0) {
        header.lct = 1;
        header.lcr = 0;
        header.rad[0] = 0;
        header.broadcast = false;
        header.path_msg = 1;
        header.msg_len = 6;
        header.somt = 1;
        header.eomt = 1;
        header.seqno = 0;
        gb_printf(KERN_ERR, "sahakyan: RAD = %x", header.rad[0]);
    } else {
        header.lct = 1;
        header.lcr = 0;
        header.rad[0] = 0;
        header.broadcast = false;
        header.path_msg = 1;
        header.msg_len = 6;
        header.somt = 1;
        header.eomt = 1;
        header.seqno = 0;
        gb_printf(KERN_ERR, "sahakyan: RAD = %x", header.rad[0]);
    }
#endif
#if 1
    header.lct = 1;
	header.lcr = 0;
    header.rad[0] = 0;
	header.broadcast = false;
	header.path_msg = 1;
	header.msg_len = 6;
	header.somt = 1;
	header.eomt = 1;
	header.seqno = 0;

	dptx_dbg(dptx, "%s: PBN=%d\n", __func__, pbn);
    if ((port1 >= 0) && dptx->need_rad) {
        header.lct = 2;
        header.lcr = 1;
        header.rad[0] |= ((dptx->rad_port << 4) & 0xf0);
        header.rad[0] |= (0 & 0xf);
        gb_printf(KERN_ERR, "sahakyan: RAD = %x", header.rad[0]);
    }
#endif

	GB02FUNC1337(&header, buf, &len);

	GB02FUNC1293(buf, len);
    msg = &buf[len];
	msg[0] = DP_ALLOCATE_PAYLOAD;
	msg[1] = ((port & 0xf) << 4);
    gb_printf(KERN_ERR, "sahakyan: msg_port = %x", msg[1]);
	msg[2] = vcpid & 0x7f;
	msg[3] = pbn >> 8;
	msg[4] = pbn & 0xff;
	GB02FUNC1340(msg, 5);

	len += 6;

	dptx_dbg(dptx, "%s: Sending DOWN_REQ\n", __func__);
	GB02FUNC1156(dptx, DP_SIDEBAND_MSG_DOWN_REQ_BASE,
				 buf, len);

	GB02FUNC1381(dptx, DP_ALLOCATE_PAYLOAD, NULL);
	return 0;
}

static u32 GB02FUNC1400(struct dptx *dptx, int pbn)
{
	int div;
	int dp_link_bw = GB02FUNC1125(dptx->link.rate);
	int dp_link_count = dptx->link.lanes;

	switch (dp_link_bw) {
	case DP_LINK_BW_1_62:
		div = 3 * dp_link_count;
		break;
	case DP_LINK_BW_2_7:
		div = 5 * dp_link_count;
		break;
	case DP_LINK_BW_5_4:
		div = 10 * dp_link_count;
		break;
    case DP_LINK_BW_8_1:
		div = 15 * dp_link_count;
		break;
	default:
		return 0;
	}

	return DIV_ROUND_UP(pbn, div);
}

#ifdef DPTX_TYPE_C
void GB02FUNC1402(struct dptx *dptx)
{
	u32 reg;

	// Disable apb-i2c
	dptx_writel(dptx, GB02MAC2132, 0);
	udelay(200);

	// Enable master mode, disable slave mode
	dptx_writel(dptx, GB02MAC2133, 0x45);
	udelay(200);

	// Write target slave address
	dptx_writel(dptx, GB02MAC2134, 0x38);
	udelay(200);

	// Enable apb-i2c
	dptx_writel(dptx, GB02MAC2132, 1);
	udelay(200);

	// Write Data command (slave register address)
	dptx_writel(dptx, GB02MAC2135, 0x21A);
	udelay(200);

	// Write target slave address
	dptx_writel(dptx, GB02MAC2134, 0x39);
	udelay(200);

	// Write Read command and read first byte
	dptx_writel(dptx, GB02MAC2135, 0x300);
	udelay(200);

	reg = dptx_readl(dptx, GB02MAC2135);
	dptx_dbg(dptx, "%s : first byte %d", __func__, reg);
	udelay(200);

	// Write Read command and read second byte
	dptx_writel(dptx, GB02MAC2135, 0x300);
	udelay(200);

	reg = dptx_readl(dptx, GB02MAC2135);
	udelay(200);

	dptx_dbg(dptx, "%s : second byte %x", __func__, reg);
	dptx_writel(dptx, GB02MAC2132, 0);

	// Detect Plug orientation
	if (reg & GB02MAC2136)
		dptx_writel(dptx, 0x800f0, 0);
	else
		dptx_writel(dptx, 0x800f0, 1);
}
#else
void GB02FUNC1402(struct dptx *dptx) {}
#endif


static void GB02FUNC1407(struct dptx* dptx, int edid_index)
{
	u8 sample_freq;
	struct GB02STR123* audio_desc;

	audio_desc = &dptx->audio_desc;

	sample_freq = dptx->edid_second[edid_index + 2] & GENMASK(6, 0);

	if (sample_freq & BIT(0)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 32khz audio\n");
		audio_desc->max_sampling_freq = SAMPLE_FREQ_32;
	}

	if (sample_freq & BIT(1)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 44.1khz audio\n");
		audio_desc->max_sampling_freq = SAMPLE_FREQ_44_1;
	}

	if (sample_freq & BIT(2)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 48khz audio\n");
		audio_desc->max_sampling_freq = SAMPLE_FREQ_48;
	}

	if (sample_freq & BIT(3)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 88.2khz audio\n");
		audio_desc->max_sampling_freq = SAMPLE_FREQ_88_2;
	}

	if (sample_freq & BIT(4)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 96khz audio\n");
		audio_desc->max_sampling_freq = SAMPLE_FREQ_96;
	}

	if (sample_freq & BIT(5)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 176.4khz audio\n");
		audio_desc->max_sampling_freq = SAMPLE_FREQ_176_4;
	}
	if (sample_freq & BIT(6)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 192khz audio\n");
		audio_desc->max_sampling_freq = SAMPLE_FREQ_192;
	}
//	audio_desc->max_sampling_freq = SAMPLE_FREQ_48;
	gb_printf(KERN_INFO, "%s_%d: AUDIO EDID: sample_freq = 0x%x\n",
		__func__, __LINE__, sample_freq);
}

static void GB02FUNC1411(struct dptx* dptx, int edid_index)
{
	u8 bpsample;
	struct GB02STR123* audio_desc;

	audio_desc = &dptx->audio_desc;
	bpsample = dptx->edid_second[edid_index + 3] & GENMASK(2, 0);

	if (bpsample & BIT(0)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 16 bit audio\n");
		audio_desc->max_bit_per_sample = 16;
	}

	if (bpsample & BIT(1)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 20 bit audio\n");
		audio_desc->max_bit_per_sample = 20;
	}

	if (bpsample & BIT(2)) {
		dev_dbg(dptx->dev, "AUDIO EDID: Sink supports 24 bit audio\n");
		audio_desc->max_bit_per_sample = 24;
	}
//	audio_desc->max_bit_per_sample = 16;
	gb_printf(KERN_INFO, "%s_%d: AUDIO EDID: bit bpsample = 0x%x\n",
		__func__, __LINE__, bpsample);
}

static void GB02FUNC1416(struct dptx* dptx, int edid_index,
	int dp_edid_support_status)
{
	struct GB02STR123* audio_desc;
//	u8 audio_block[4];
	u8 audio_data_size;
//	u8 num_of_channels, bpsample, sampleFreq;

	if (edid_index > 126 || edid_index < 4)
		dp_edid_support_status = DP_DISSUPPORT_AUDIO;

	GB02FUNC554(dptx->index, dp_edid_support_status);
	if (dp_edid_support_status == DP_DISSUPPORT_AUDIO) {
		gb_printf(KERN_INFO, "%s-%d: dissupport audio, num = %d, edid_index = %d\n",
			__func__, __LINE__, dptx->index, edid_index);
		GB02FUNC547(dptx->index, GB02MAC19,
			SAMPLE_FREQ_48, GB02MAC41, DP_CONNECT);
		return;
	}
	audio_desc = &dptx->audio_desc;

	audio_data_size = (dptx->edid_second[edid_index] & EDID_SIZE_MASK) >> GB02MAC1527;

	audio_desc->max_num_of_channels = (dptx->edid_second[edid_index + 1] & GENMASK(2, 0)) + 1;
	dev_dbg(dptx->dev,"AUDIO EDID: Sink supports up to %d channels\n", audio_desc->max_num_of_channels);
	gb_printf(KERN_INFO, "%s-%d: edid_index = %d, audio_data_size = %d\n",
		__func__, __LINE__, edid_index, audio_data_size);

	/* Detect audio sampling frequency supported by the sink*/
	GB02FUNC1407(dptx, edid_index);

	/* Detect bit per sample supported by the sink */
	GB02FUNC1411(dptx, edid_index);
}

static void GB02FUNC1418(struct dptx* dptx)
{
	u8 byte1, byte2, byte3;

	byte1 = dptx->edid[35];
	byte2 = dptx->edid[36];
	byte3 = dptx->edid[37];

#ifndef GB02MAC1329

	// Parsing BYTE 1
	if (byte1 & GB02MAC1528) {
		dev_dbg(dptx->dev, "Sink supports GB02MAC1528\n");
		dptx->selected_est_timing = DMT_800x600_60hz;
		return;
	}

	if (byte1 & GB02MAC1529) {
		dev_dbg(dptx->dev, "Sink supports GB02MAC1529, but we dont\n");
	}

	if (byte1 & GB02MAC1531)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1531, but we dont\n");

	if (byte1 & GB02MAC1533)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1533, but we dont\n");

	if (byte1 & GB02MAC1534)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1534, but we dont\n");

	if (byte1 & GB02MAC1535) {
		dev_dbg(dptx->dev, "Sink supports GB02MAC1535\n");
		dptx->selected_est_timing = DMT_640x480_60hz;
		return;
	}

	if (byte1 & GB02MAC1537)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1537, but we dont\n");

	if (byte1 & GB02MAC1539)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1539, but we dont\n");

	// Parsing BYTE 2
	if (byte2 & GB02MAC1541)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1541, but we dont\n");

	if (byte2 & GB02MAC1543)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1543, but we dont\n");

	if (byte2 & GB02MAC1545)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1545, but we dont\n");

	if (byte2 & GB02MAC1547) {
		dev_dbg(dptx->dev, "Sink supports GB02MAC1547\n");
		dptx->selected_est_timing = DMT_1024x768_60hz;
		return;
	}

	if (byte2 & GB02MAC1548)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1548, but we dont\n");

	if (byte2 & GB02MAC1550)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1550, but we dont\n");

	if (byte2 & GB02MAC1552)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1552, but we dont\n");

	if (byte2 & GB02MAC1554)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1554, but we dont\n");

	// Parsing BYTE 3
	if (byte3 & GB02MAC1557)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1557, but we dont\n");

#else

	// Parsing BYTE 3
	if (byte3 & GB02MAC1557)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1557, but we dont\n");

	// Parsing BYTE 2
	if (byte2 & GB02MAC1554)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1554, but we dont\n");


	if (byte2 & GB02MAC1552)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1552, but we dont\n");

	if (byte2 & GB02MAC1550)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1550, but we dont\n");

	if (byte2 & GB02MAC1548)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1548, but we dont\n");

	if (byte2 & GB02MAC1547) {
		dev_dbg(dptx->dev, "Sink supports GB02MAC1547\n");
		dptx->selected_est_timing = DMT_1024x768_60hz;
		return;
	}

	if (byte2 & GB02MAC1545)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1545, but we dont\n");

	if (byte2 & GB02MAC1543)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1543, but we dont\n");

	if (byte2 & GB02MAC1541)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1541, but we dont\n");

	// Parsing BYTE 1
	if (byte1 & GB02MAC1539)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1539, but we dont\n");

	if (byte1 & GB02MAC1537)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1537, but we dont\n");

	if (byte1 & GB02MAC1535) {
		dev_dbg(dptx->dev, "Sink supports GB02MAC1535\n");
		dptx->selected_est_timing = DMT_640x480_60hz;
		return;
	}

	if (byte1 & GB02MAC1534)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1534, but we dont\n");

	if (byte1 & GB02MAC1533)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1533, but we dont\n");

	if (byte1 & GB02MAC1531)
		dev_dbg(dptx->dev, "Sink supports GB02MAC1531, but we dont\n");

	if (byte1 & GB02MAC1529) {
		dev_dbg(dptx->dev, "Sink supports GB02MAC1529, but we dont\n");
	}

	if (byte1 & GB02MAC1528) {
		dev_dbg(dptx->dev, "Sink supports GB02MAC1528\n");
		dptx->selected_est_timing = DMT_800x600_60hz;
		return;
	}
#endif
}

static void GB02FUNC1435(struct dptx* dptx)
{
	if (GB02FUNC1313(dptx) != 0)
		return;
	//dev_info(dptx->dev, "dptx->edid[54] = %d, dptx->edid[55] = %d,\n", dptx->edid[54], dptx->edid[55]);
	//dev_info(dptx->dev, "dptx->edid[72] = %d, dptx->edid[73] = %d,\n", dptx->edid[72], dptx->edid[73]);
	if ((dptx->edid[54] == 0 && dptx->edid[55] == 0)
		&& (dptx->edid[72] == 0 && dptx->edid[73] == 0)) {
		//dev_info(dptx->dev, "%s FOUND EDID DUMMY BLOCKS\n",__func__);
		//dev_info(dptx->dev, "%s: Going to parse established timings\n", __func__);
		dptx->dummy_dtds_present = true;
		GB02FUNC1418(dptx);
	}
	else {
		//dev_info(dptx->dev, "%s: EDID Dummy blocks not found, continuing with usual way\n",	 __func__);
	}
}

static void GB02FUNC1439(struct dptx* dptx)
{
	u8 byte;
	u8 tag, size;
//	u8 audio_desc[3];
	u8 edid_block1[128];
	int i, index = 0, dp_edid_support_status = DP_DISSUPPORT_AUDIO;

	if (dptx->edid[126] > 0) { // Means EDID has extension block
		for (i = 0; i < 128; i++)
			edid_block1[i] = dptx->edid_second[i];

		byte = edid_block1[4];
		index = 4;
		tag = (byte & EDID_TAG_MASK) >> GB02MAC1525;
		size = (byte & EDID_SIZE_MASK) >> GB02MAC1527;

		/* find the audio tag  containing byte */
		while (tag != GB02MAC1521)
		{
			size = (byte & EDID_SIZE_MASK) >> GB02MAC1527;
			index = index + size +1;
        		if (index > 127)
				break;
	 		byte = dptx->edid_second[index];
			tag = (byte & EDID_TAG_MASK) >> GB02MAC1525;
		}
		if (tag == GB02MAC1521)
	 		dp_edid_support_status = DP_SUPPORT_AUDIO;
	}

	GB02FUNC1416(dptx, index, dp_edid_support_status);
}

int GB02FUNC1440(struct dptx *dptx, int set_info)
{
	int retval;
//	u8 test_audio_mode;
	u8 audio_ch_count, orig_sample_freq, sample_freq, desc_audio_ch_count;
	u32 audio_clock_freq;
	struct GB02STR123 *audio_desc;
	enum audio_sample_freq audio_smaple_range;
	struct GB02STR122 *aparams;

	audio_desc = &dptx->audio_desc;
	aparams = &dptx->aparams;

	audio_smaple_range = audio_desc->max_sampling_freq;
	desc_audio_ch_count = audio_desc->max_num_of_channels;

	/* Configure channel count */
	switch (desc_audio_ch_count) {
	case 1:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_CHANNEL1\n");
		audio_ch_count = 1;
		break;
	case 2:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_CHANNEL2\n");
		audio_ch_count = 2;
		break;
	case 3:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_CHANNEL3\n");
		audio_ch_count = 3;
		break;
	case 4:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_CHANNEL4\n");
		audio_ch_count = 4;
		break;
	case 5:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_CHANNEL5\n");
		audio_ch_count = 5;
		break;
	case 6:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_CHANNEL6\n");
		audio_ch_count = 6;
		break;
	case 7:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_CHANNEL7\n");
		audio_ch_count = 7;
		break;
	case 8:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_CHANNEL8\n");
		audio_ch_count = 8;
		break;
	default:
		dptx_dbg(dptx, "Invalid SHORT_AUDIO_DESC AUDIO_CHANNEL_COUNT\n");
		return -EINVAL;
	}
	dptx_dbg(dptx, "audio_ch_count = %d\n", audio_ch_count);
	aparams->num_channels = audio_ch_count;

	/* Configure sampling frequency */
	switch (audio_smaple_range) {
	case SAMPLE_FREQ_32:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_SAMPLING_RATE_32\n");
		orig_sample_freq = 12;
		sample_freq = 3;
		audio_clock_freq = 320;
		break;
	case SAMPLE_FREQ_44_1:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_SAMPLING_RATE_44_1\n");
		orig_sample_freq = 15;
		sample_freq = 0;
		audio_clock_freq = 441;
		break;
	case SAMPLE_FREQ_48:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_SAMPLING_RATE_48\n");
		orig_sample_freq = 13;
		sample_freq = 2;
		audio_clock_freq = 480;
		break;
	case SAMPLE_FREQ_88_2:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_SAMPLING_RATE_88_2\n");
		orig_sample_freq = 7;
		sample_freq = 8;
		audio_clock_freq = 882;
		break;
	case SAMPLE_FREQ_96:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_SAMPLING_RATE_96\n");
		orig_sample_freq = 5;
		sample_freq = 10;
		audio_clock_freq = 960;
		break;
	case SAMPLE_FREQ_176_4:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_SAMPLING_RATE_176_4\n");
		orig_sample_freq = 3;
		sample_freq = 12;
		audio_clock_freq = 1764;
		break;
	case SAMPLE_FREQ_192:
		dptx_dbg(dptx, "SHORT AUDIO DESC AUDIO_SAMPLING_RATE_192\n");
		orig_sample_freq = 1;
		sample_freq = 14;
		audio_clock_freq = 1920;
		break;
	default:
		dptx_dbg(dptx, "Invalid SHORT AUDIO DESC AUDIO_SAMPLING_RATE\n");
		return -EINVAL;
	}
	dptx_dbg(dptx, "sample_freq = %d\n", sample_freq);
	dptx_dbg(dptx, "orig_sample_freq = %d\n", orig_sample_freq);

	/* Configure audio data width */
	aparams->data_width = audio_desc->max_bit_per_sample;
	dptx_dbg(dptx, "SHORT AUDIO DATA WIDTH = %d\n", aparams->data_width);
	GB02FUNC475(dptx);


	retval = GB02FUNC1172(dptx, audio_clock_freq, 512);
	if (retval)
		return retval;

	aparams->iec_samp_freq = sample_freq;
	aparams->iec_orig_samp_freq = orig_sample_freq;

	GB02FUNC470(dptx);
	GB02FUNC483(dptx);
	GB02FUNC404(dptx);
	if (set_info == 1) {
#ifdef  GB02MAC292
	GB02FUNC547(dptx->index, desc_audio_ch_count,
		audio_smaple_range, aparams->data_width, DP_CONNECT);
	GB02FUNC556(dptx->index, AUDIO_NEED_RESTART);
#endif
	}
	return retval;
}

static int GB02FUNC1455(struct dptx *dptx) {

	int retval;
	int i;
	u16 pbn;
	u32 slots;
	int port_count = 0;
	struct drm_dp_sideband_msg_rx *raw;
	struct drm_dp_sideband_msg_reply_body rep;

	memset(&rep, 0, sizeof(rep));
	raw = kzalloc(sizeof(*raw), GFP_KERNEL);
	if (!raw)
		return -ENOMEM;

	dptx->need_rad = false;

	dptx_err(dptx, "%s: --------------- Sending sideband message Clear Payload\n", __func__);

	retval = GB02FUNC1385(dptx);
	if (retval) {
		kfree(raw);
		return retval;
	}
	//LINK_ADDRESS FIRST MONITOR

	dptx_err(dptx, "%s: --------------- Sending link_address 0\n", __func__);
	retval = GB02FUNC1393(dptx, raw, &rep, -1);
	if (retval) {
		kfree(raw);
		return retval;
	}

	dptx_dbg(dptx, "%s: NPORTS = %d\n", __func__, rep.u.link_addr.nports);
	for (i = 0; i < rep.u.link_addr.nports; i++) {
		dptx_dbg(dptx, "%s: input=%d, pdt=%d, pnum=%d, mcs=%d, ldps = %d, ddps=%d\n", __func__,
					rep.u.link_addr.ports[i].input_port,
					rep.u.link_addr.ports[i].peer_device_type,
					rep.u.link_addr.ports[i].port_number,
					rep.u.link_addr.ports[i].mcs,
					rep.u.link_addr.ports[i].legacy_device_plug_status,
					rep.u.link_addr.ports[i].ddps);
		}

	for (i = 0; i < rep.u.link_addr.nports; i++) {
		if (rep.u.link_addr.ports[i].peer_device_type == 3 && !rep.u.link_addr.ports[i].mcs && rep.u.link_addr.ports[i].ddps && port_count < 2) {
			dptx->logic_port = true;
			dptx->port[port_count] = rep.u.link_addr.ports[i].port_number;
			port_count++;
		}

		if (rep.u.link_addr.ports[i].peer_device_type == 2 && rep.u.link_addr.ports[i].mcs && rep.u.link_addr.ports[i].ddps) {
			dptx->need_rad = true;
			dptx->rad_port = rep.u.link_addr.ports[i].port_number;
			dptx_err(dptx, "%s: --------------- Sending link_address %d\n", __func__, i);
			retval = GB02FUNC1393(dptx, raw, &rep, rep.u.link_addr.ports[i].port_number);
			if (retval) {
				kfree(raw);
				return retval;
			}
			dptx_dbg(dptx, "%s: NPORTS = %d\n", __func__, rep.u.link_addr.nports);
			for (i = 0; i < rep.u.link_addr.nports; i++) {
				dptx_dbg(dptx, "%s: input=%d, pdt=%d, pnum=%d, mcs=%d, ldps = %d, ddps=%d\n", __func__,
						rep.u.link_addr.ports[i].input_port,
						rep.u.link_addr.ports[i].peer_device_type,
						rep.u.link_addr.ports[i].port_number,
						rep.u.link_addr.ports[i].mcs,
						rep.u.link_addr.ports[i].legacy_device_plug_status,
						rep.u.link_addr.ports[i].ddps);
				if (rep.u.link_addr.ports[i].peer_device_type == 3 && !rep.u.link_addr.ports[i].mcs && rep.u.link_addr.ports[i].ddps && port_count < 2) {
					dptx->port[port_count] = rep.u.link_addr.ports[i].port_number;
					port_count++;
				}
			}
		}
	}

	GB02FUNC1362(dptx, dptx->port[0], -1);
	GB02FUNC1357(dptx, dptx->port[0], -1);

	GB02FUNC1357(dptx, dptx->port[1], 1);

	#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
	dptx->pbn = (u16)drm_dp_calc_pbn_mode(74250, 18);
	#else
	dptx->pbn = (u16)DIV_ROUND_UP_ULL(mul_u32_u32(74250 * 18, 64 * 1006),
                                8 * 54 * 1000 * 1000);
	#endif

	pbn = dptx->pbn;
	slots = GB02FUNC1400(dptx, pbn);

	dptx_dbg(dptx, "%s: NUM SLOTS = %d\n", __func__, slots);

	dptx_err(dptx, "%s: --------------- Clearing DPCD VCPID table\n", __func__);
	GB02FUNC1324(dptx);

	dptx_err(dptx, "%s: --------------- Clearing DPTX VCPID table\n", __func__);
	GB02FUNC1318(dptx);

	for (i = 0; i < dptx->streams; i++) {
		GB02FUNC1321(dptx, slots * i + 1, slots, i + 1);
	}

	for (i = 0; i < dptx->streams; i++) {
		GB02FUNC1327(dptx, slots * i + 1, slots, i + 1);
	}

	GB02FUNC1329(dptx);

	GB02FUNC1322(dptx);

	{
		int tries = 0;
		u8 status = 0;

		while (!(status & DP_PAYLOAD_ACT_HANDLED)) {
			GB02FUNC1151(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);
			tries++;
			if (tries > 100) {
				dptx_err(dptx, "Timeout waiting for ACT_HANDLED\n");
				break;
			}

			mdelay(20);
		}

		GB02FUNC1153(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, 0x3);
		GB02FUNC1151(dptx, DP_PAYLOAD_TABLE_UPDATE_STATUS, &status);

	}

	dptx->vcp_id = 1;

	if (dptx->logic_port)
		GB02FUNC1396(dptx, dptx->port[0], dptx->vcp_id, pbn, -1);
	else
		GB02FUNC1396(dptx, dptx->port[0], dptx->vcp_id, pbn, 1);
	dptx->vcp_id++;
	GB02FUNC1362(dptx, dptx->port[1], 1);
	GB02FUNC1396(dptx, dptx->port[1], dptx->vcp_id, pbn, 1);

	kfree(raw);
	return retval;
}


static int GB02FUNC1465(struct dptx* dptx, struct dtd *mdtd)
{
	struct GB02STR124* vparams = &dptx->vparams;

	switch (dptx->selected_est_timing)
	{
	case DMT_640x480_60hz:
		dev_err(dptx->dev, "Set Video mode to DMT 640x480\n");
		vparams->video_format = DMT;
		GB02FUNC811(mdtd, 4, vparams->refresh_rate,
			vparams->video_format);
		return 0;
	case DMT_800x600_60hz:
		dev_err(dptx->dev, "Set Video mode to DMT 800x600\n");
		vparams->video_format = DMT;
		GB02FUNC811(mdtd, 9, vparams->refresh_rate,
			vparams->video_format);
		return 0;
	case DMT_1024x768_60hz:
		dev_err(dptx->dev, "Set Video mode to DMT 1024x768\n");
		vparams->video_format = DMT;
		GB02FUNC811(mdtd, 16, vparams->refresh_rate,
			vparams->video_format);
		return 0;
	case NONE:
	default:
		dev_err(dptx->dev, "%s: Not Found selected timing in Established timings\n", __func__);
		return -EINVAL;
	}
}
static int GB02FUNC1469(struct dptx *dptx)
{
	u8 rev;
	int retval;
	u32 phyifctrl;
	u32 reg;
	u8 vector;
	u8 preferred_vic[18];
	struct GB02STR124 *vparams;
	struct GB02STR150 *hparams;
	struct dtd mdtd;
	u8 result,read_cnt = 0;

	struct drm_dp_sideband_msg_rx raw;
	struct drm_dp_sideband_msg_reply_body rep;

	memset(&raw, 0, sizeof(raw));
	memset(&rep, 0, sizeof(rep));

	GB02FUNC1402(dptx);
	vparams = &dptx->vparams;
	hparams = &dptx->hparams;

	if (dptx->ssc_en)
		GB02FUNC1076(dptx);
#ifdef CONFIG_GB02_CTS
        dptx_writel(dptx, 0xc14, 0x00000001);
        dptx_writel(dptx, 0xc18, 0x00500051);
	dptx_writel(dptx, 0xc1c, 0x001e600a);
	dptx_writel(dptx, 0xc20, 0x00000000);
//	dptx_writel(dptx, 0xc3c, 0x00000006);
        dptx_writel(dptx, 0xc3c, 0x00000000);
//	dptx_writel(dptx, 0xc40, 0x01010014);
	dptx_writel(dptx, 0xc40, 0x0101002f);
	dptx_writel(dptx, 0xc44, 0x00000777);
#endif
	GB02FUNC1059(dptx,
		GB02MAC2166 |
		//GB02MAC2167 |
		GB02MAC2169);

	GB02FUNC1064(dptx);
	gb_printf(KERN_DEBUG, "%s:%d------dptx index =%x------\n",
			__func__, __LINE__, dptx->index);
	/* Move to P0 */
	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	phyifctrl &= ~DPTX_PHYIF_CTRL_LANE_PWRDOWN_MASK;
	dptx_writel(dptx, GB02MAC2107, phyifctrl);

        GB02FUNC1153(dptx, DP_SET_POWER,  DP_SET_POWER_D0);

	if (dptx->mst) {
		u8 byte;

		retval = GB02FUNC1151(dptx, DP_MSTM_CAP, &byte);
		if (retval)
			return retval;
		if (byte & DP_MST_CAP) {
			retval = GB02FUNC1153(dptx, DP_MSTM_CTRL, 0x07);
			if (retval)
				return retval;
		} else {
			dptx->mst = false;
			pr_err("sahakyan: mst = %d", dptx->mst);
		}
	} else {
		u8 byte;

		retval = GB02FUNC1151(dptx, DP_MSTM_CAP, &byte);
		if (retval)
			return retval;
		if (byte & DP_MST_CAP) {
			retval = GB02FUNC1153(dptx, DP_MSTM_CTRL, 0x00);
			if (retval)
				return retval;
		}
	}

	/* Ensure streams = 1 if no MST */
	if (!dptx->mst)
		dptx->streams = 1;

	retval = GB02FUNC1151(dptx,
		DP_DEVICE_SERVICE_IRQ_VECTOR, &vector);
	if (retval)
		return retval;
RETRY_EDID:
	retval = GB02FUNC1306(dptx);
	if (retval)
		return retval;

	if (!GB02FUNC1313(dptx))
		pr_debug("%s-%d: dp %d edid is valid\n", __func__, __LINE__, dptx->index);
	else {
		gb_printf(KERN_ERR, "--edid is invalid,retry---\n");
		read_cnt++;
		if (read_cnt < 2) {
			gb_printf(KERN_WARNING, "try dp %d next edid\n", dptx->index);
			goto RETRY_EDID;
		} else
			return -EINVAL;
	}

	/* Check and parse Established Timings */
	GB02FUNC1435(dptx);

	/* Configure audio params based on sinks EDID */
	GB02FUNC1439(dptx);
	GB02FUNC1440(dptx, DPTX_AUDIO_EDID_CONFIGURE);

	if (dptx->mst) {
	vparams->video_format = VCEA;
// in case of 1080p is should be 16 instead of 1
		GB02FUNC811(&mdtd, 34, vparams->refresh_rate,
			vparams->video_format);
	} else {
		if (dptx->dummy_dtds_present) {
			retval = GB02FUNC1465(dptx, &mdtd);
			if (retval) {
				gb_printf(KERN_ERR, "%s: Selecting  default video mode\n",
					__func__);
				vparams->video_format = VCEA;
				GB02FUNC811(&mdtd, 1, vparams->refresh_rate,
					vparams->video_format);
			}
		} else {
			retval = GB02FUNC1313(dptx);
			if (retval) {
				vparams->video_format = VCEA;
				GB02FUNC811(&mdtd, 1, vparams->refresh_rate,
				vparams->video_format);
				dptx->edid_getted = false;
				gb_printf(KERN_INFO, "%s:%d------check edid fail false--\n",
					__func__, __LINE__);
			} else {
				dptx->edid_getted = true;
				gb_printf(KERN_INFO, "%s:%d------check edid sucess true--\n",
					__func__, __LINE__);
				memcpy(preferred_vic, dptx->edid + 0x36, 0x12);
				retval = GB02FUNC393(dptx, &mdtd, preferred_vic);
				if (retval) {
					vparams->video_format = VCEA;
					GB02FUNC811(&mdtd, 1, vparams->refresh_rate,
					vparams->video_format);
				}
			}
		}
	}
	retval = GB02FUNC1151(dptx, DP_DPCD_REV, &rev);
	if (retval) {
                /* Abort bringup */
                /* Reset core and try again */
                /* Abort all aux, and other work, reset the core */
		return retval;
	}

	dptx_dbg(dptx, "DP Revision %x.%x\n",
		(rev & 0xf0) >> 4, rev & 0xf);

	memset(dptx->rx_caps, 0, GB02MAC1325);
	retval = GB02FUNC1154(dptx, DP_DPCD_REV,
		dptx->rx_caps, GB02MAC1325);
	if (retval)
		return retval;

	if (dptx->rx_caps[DP_TRAINING_AUX_RD_INTERVAL] &
		GB02MAC1819) {
		retval = GB02FUNC1154(dptx, 0x2200,
			dptx->rx_caps, GB02MAC1325);
		if (retval)
			return retval;
	}
        /*
         * The TEST_EDID_READ is asserted on HOTPLUG. Check for it and
         * handle it here.
         */
	if (vector & DP_AUTOMATED_TEST_REQUEST) {
		u8 test;

		dptx_dbg(dptx, "%s: DP_AUTOMATED_TEST_REQUEST", __func__);

		retval = GB02FUNC1151(dptx, DP_TEST_REQUEST, &test);
		if (retval)
			return retval;

		if (test & DP_TEST_LINK_EDID_READ) {
			u8 checksum = 0;
			u8 blocks = 0;

			blocks = dptx->edid[126];
			checksum = dptx->edid[127 + 128 * blocks];

			retval = GB02FUNC1153(dptx,
			DP_TEST_EDID_CHECKSUM, checksum);
			if (retval)
				return retval;

			retval = GB02FUNC1153(dptx, DP_TEST_RESPONSE,
                                                 DP_TEST_EDID_CHECKSUM_WRITE);
			if (retval)
				return retval;
		}
	}

        /* TODO No other IRQ should be set on hotplug */
        /* Forward Error Correction flow */
        if(dptx->fec) {
		reg = dptx_readl(dptx, GB02MAC2057);
		reg |= GB02MAC2163;
		dptx_writel(dptx, GB02MAC2057, reg);

		// Set FEC_READY on the sink side
		retval = GB02FUNC1153(dptx,
			DP_FEC_CONFIGURATION, DP_FEC_READY);
		if (retval)
			return retval;
	} else {
		reg = dptx_readl(dptx, GB02MAC2057);
		reg &= (~GB02MAC2161);
		dptx_writel(dptx, GB02MAC2057, reg);
	}
	retval = GB02FUNC1567(dptx, dptx->max_rate, dptx->max_lanes);
	if (retval)
                return retval;
	else {
		dptx->link.trained = true;
		gb_printf(KERN_DEBUG, "%s:%d-*******---wake up xorg-------\n",
				__func__, __LINE__);
		wake_up_interruptible(&dptx->waitq);
	}
	//msleep(1);
	if (dptx->fec) {
                // Enable forward error correction
                reg = dptx_readl(dptx, GB02MAC2057);
		reg |= GB02MAC2161;
		dptx_writel(dptx, GB02MAC2057, reg);

		dev_dbg(dptx->dev,
			"%s: Enabling Forward Error Correction\n", __func__);

		retval = GB02FUNC1151(dptx, 0x280, &result);
		if (retval)
			dev_dbg(dptx->dev, "DPCD read failed\n");
		dev_dbg(dptx->dev, "fec status = %x\n", result);

		retval = GB02FUNC1151(dptx, 0x281, &result);
		if (retval)
			dev_dbg(dptx->dev, "DPCD read failed\n");

		dev_dbg(dptx->dev, "fec error count %x\n", result);
        }

	return 0;

}
int GB02FUNC1479(struct dptx *dptx, u8 enable)
{
	int retval;
    u32 reg;
	u8 result;

    if(!enable) {
       dptx->fec = true;
    }
    else {
        dptx->fec = false;
    }

    reg = dptx_readl(dptx, GB02MAC2057);
    reg |= GB02MAC2163;
    dptx_writel(dptx, GB02MAC2057, reg);

    // Set FEC_READY on the sink side
    retval = GB02FUNC1153(dptx, DP_FEC_CONFIGURATION, DP_FEC_READY);
	  if (retval)
		return retval;

     retval = GB02FUNC1567(dptx, dptx->max_rate, dptx->max_lanes);
     if (retval)
        return retval;

     msleep(1);

	// Enable forward error correction
	reg = dptx_readl(dptx, GB02MAC2057);
	reg |= GB02MAC2161;
	dptx_writel(dptx, GB02MAC2057, reg);

	dev_dbg(dptx->dev, "%s: Enabling Forward Error Correction\n", __func__);

	retval = GB02FUNC1151(dptx, 0x280, &result);
	if (retval)
		dev_dbg(dptx->dev, "DPCD read failed\n");
	dev_dbg(dptx->dev,"fec status = %x\n", result);

	retval = GB02FUNC1151(dptx, 0x281, &result);
	if (retval)
		dev_dbg(dptx->dev, "DPCD read failed\n");

	dev_dbg(dptx->dev,"fec error count %x\n", result);

    return 0;
}

int GB02FUNC1481(struct dptx *dptx, u8 enable)
{
	int retval;
    u32 reg;

    retval = GB02FUNC1479(dptx, enable);
    if (retval)
        return retval;

    if(!enable) {
       dptx->dsc = true;
    }
    else {
        dptx->dsc = false;
    }

    reg = dptx_readl(dptx, GB02MAC2066(0));
			reg |= GB02MAC2269;
			dptx_writel(dptx, GB02MAC2066(0), reg);

    GB02FUNC1286(dptx);
	GB02FUNC1469(dptx);

    gb_printf(KERN_ERR, "sahakyan dsc = %d - - - %s:%d", enable, __func__, __LINE__);
    return 0;
}

int GB02FUNC1483(struct dptx *dptx, u8 enable)
{

    struct dtd mdtd;
    struct GB02STR124 *vparams;
    u8 preferred_vic[18];
    int retval;
    u32 reg;
    int i;

    if(enable) {
       dptx->mst = true;
       dptx->streams = 2;
    }
    else {
        dptx->mst = false;
        dptx->streams = 1;
    }
    GB02FUNC788(dptx, (dptx->streams));
    GB02FUNC1567(dptx, dptx->max_rate, dptx->max_lanes);
    dptx_writel(dptx, GB02MAC2057, GB02MAC2157
            | (dptx->mst ? GB02MAC2160 : 0));
    GB02FUNC789(dptx, (dptx->streams));

    vparams = &dptx->vparams;

    retval = GB02FUNC1306(dptx);
    if (retval)
        return retval;

    /* Check and parse Established Timings */
    GB02FUNC1435(dptx);

    /* Configure audio params based on sinks EDID */
    GB02FUNC1439(dptx);
    GB02FUNC1440(dptx, DPTX_AUDIO_EDID_CONFIGURE);

    if (dptx->mst) {
        u8 byte;

        retval = GB02FUNC1151(dptx, DP_MSTM_CAP, &byte);
        if (retval)
            return retval;
        if (byte & DP_MST_CAP) {
            retval = GB02FUNC1153(dptx, DP_MSTM_CTRL, 0x07);
            if (retval)
                return retval;
        } else {
            dptx->mst = false;
            gb_printf(KERN_ERR, "sahakyan: mst = %d", dptx->mst);
        }
    } else {
        u8 byte;

        retval = GB02FUNC1151(dptx, DP_MSTM_CAP, &byte);
        if (retval)
            return retval;

        if (byte & DP_MST_CAP) {
            retval = GB02FUNC1153(dptx, DP_MSTM_CTRL, 0x00);
            if (retval)
                return retval;
        }
    }

    /* Ensure streams = 1 if no MST */
    if (!dptx->mst)
        dptx->streams = 1;

    if (dptx->mst) {
        vparams->video_format = VCEA;
        GB02FUNC811(&mdtd, 34, vparams->refresh_rate,
                vparams->video_format); // in case of 1080p is should be 16 instead of 1
    } else {
        if (dptx->dummy_dtds_present) {
            retval = GB02FUNC1465(dptx, &mdtd);
            if (retval) {
                dev_err(dptx->dev,"%s: Selecting  default video mode\n", __func__);
                vparams->video_format = VCEA;
                GB02FUNC811(&mdtd, 1, vparams->refresh_rate,
                        vparams->video_format);
            }
        }
        else {

            retval = GB02FUNC1313(dptx);
            if (retval) {
                vparams->video_format = VCEA;
                GB02FUNC811(&mdtd, 1, vparams->refresh_rate,
                        vparams->video_format);
            }
            else {
                memcpy(preferred_vic, dptx->edid + 0x36, 0x12);
                retval = GB02FUNC393(dptx, &mdtd, preferred_vic);
                if (retval) {
                    vparams->video_format = VCEA;
                    GB02FUNC811(&mdtd, 1, vparams->refresh_rate,
                            vparams->video_format);
                }
            }
        }
    }

    if(dptx->mst) {
        vparams->video_format = VCEA;
        for (i = 0; i < dptx->streams; i++) {
            retval = GB02FUNC493(dptx, 34, i);
            if (retval)
                return retval;
        }
    }
    else {
        if (retval) {
            dptx_dbg(dptx, "%s: Can't change to the preferred video mode: frequency = %d\n",
                    __func__, mdtd.pixel_clock);
            dptx_dbg(dptx, "%s: Changing to the default video mode\n",
                    __func__);
            vparams->video_format = VCEA;
            retval = GB02FUNC493(dptx, 1, 0);
            if (retval)
                return retval;
        } else {
            vparams->mdtd = mdtd;
            dptx_dbg(dptx, "%s: pixel_frequency - %d\n", __func__,
                    mdtd.pixel_clock);

            /* MMCM */

            GB02FUNC454(dptx, 1, 0);
            retval = GB02FUNC1171(dptx, mdtd.pixel_clock, 0);
            if (retval) {
                GB02FUNC454(dptx, 0, 0);
                return retval;
            }
            GB02FUNC454(dptx, 0, 0);
            retval = GB02FUNC682(dptx, dptx->link.lanes,
                    dptx->link.rate, vparams->bpc,
                    vparams->pix_enc, mdtd.pixel_clock);
	if (retval)
		return retval;
            GB02FUNC492(dptx, 0);

            if(dptx->dsc) {
                /* Enable compression */
                reg = dptx_readl(dptx, GB02MAC2066(0));
                reg |= GB02MAC2269;
                dptx_writel(dptx, GB02MAC2066(0), reg);
            }
        }
    }


    if (dptx->mst) {
        retval = GB02FUNC1455(dptx);
        if (retval)
            return retval;
    }

       // Enable audio SDP
       GB02FUNC399(dptx);
       GB02FUNC402(dptx);

    return 0;
}
#if 0
static void GB02FUNC1496(struct dptx *dptx)
{
	u32 hdcpintsts;
	u32 hdcpgpiostchg;
	struct GB02STR150 *hparams;

	hparams = &dptx->hparams;
	hdcpintsts = dptx_readl(dptx, GB02MAC2333);
	dptx_dbg_irq(dptx, "%s: >>>> HDCP_INT_STS=0x%08x\n", __func__,
		     hdcpintsts);

	if (hdcpintsts & GB02MAC2340)
		dptx_dbg(dptx, "%s: KSV memory access guaranteed for read, write access\n",
			 __func__);

	if (hdcpintsts & GB02MAC2342)
		dptx_dbg(dptx, "%s: SHA1 verification has been done\n",
			 __func__);

	if (hdcpintsts & GB02MAC2341) {
		dptx_en_dis_hdcp13(dptx, 0);
		dptx_dbg(dptx, "%s: AUXRESPTIMEOUT\n", __func__);
	}

	if (hdcpintsts & GB02MAC2343) {
		hparams->auth_fail_count++;
		if (hparams->auth_fail_count > GB02MAC1400) {
			dptx_en_dis_hdcp13(dptx, 0);
			dptx_dbg(dptx, "%s: Reach max allowed retries count %d\n",
				 __func__, hparams->auth_fail_count);
		}
		dptx_dbg(dptx, "%s: HDCP authentication process was failed\n",
			 __func__);
	}

	if (hdcpintsts & GB02MAC2344) {
		if (hparams->hdcp13_is_en)
			hparams->auth_fail_count = 0;
		dptx_dbg(dptx, "%s: HDCP authentication process was successful\n",
			 __func__);
	}

	if (hdcpintsts & GB02MAC2345) {
		dptx_dbg(dptx, "%s: HDCP22_GPIOINT\n", __func__);
		//GB02MAC2336
		hdcpgpiostchg = dptx_readl(dptx, GB02MAC2337);
		dptx_dbg(dptx, "%s: HDCP2.2 GPIO status changed %0x ", __func__,hdcpgpiostchg);
       		dptx_writel(dptx, GB02MAC2337, hdcpgpiostchg);

		if((hdcpgpiostchg & 56)) {
                gb_printf(KERN_INFO, "harutk bstat is updated \n");
		dptx->bstatus = 1;
                 }
	       else
                dptx->bstatus = 0;
        }
	dptx_writel(dptx, GB02MAC2334, hdcpintsts);
}
#endif

bool GB02FUNC1500(int dp_index)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];
	bool hpd_status = false;
	u32 rdata;

	/* PCIE_LPDDR4 only has DP2 & DP3 */
	if (GB02FUNC503(pcie_info) == PCIE_LPDDR4 ||
		GB02FUNC503(pcie_info) == PCIE_C0_200) {
		if (dp_index != 2 && dp_index != 3)
			return false;
	}

	/* PCIE_FULL_LPDDR4 DP1(lvds) have no use */
	if (GB02FUNC503(pcie_info) == PCIE_FULL_LPDDR4 &&
		dp_index == 1)
		return false;
	/* PCIE_HIE1LP4_LPDDR4 DP-4-5 have no use */
	if (GB02FUNC503(pcie_info) == PCIE_HIE1LP4_LPDDR4 &&
		((dp_index == 1) || (dp_index == 4)))
		return false;
	/* PCIE_M4HL8G_LPDDR4 DP-1-4 have no use */
	if (GB02FUNC503(pcie_info) == PCIE_M4HL8G_LPDDR4 &&
		((dp_index == 1) || (dp_index == 4)))
		return false;
	rdata = dptx_readl(dptx_info, GB02MAC2128);
	hpd_status = (rdata & (1 << 8)) >> 8;

	return hpd_status;
}

bool GB02FUNC1503(int dp_index)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];
	return dptx_info->edid_getted;
}

unsigned char *GB02FUNC1504(int dp_index)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];

	return dptx_info->edid;
}

void GB02FUNC1505(struct work_struct *work)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = container_of(work, struct dptx,
						  hotplug_work.work);
	int retry_times = 0;

	atomic_set(&dptx_info->aux.abort, 0);

	if (GB02FUNC503(pcie_info) == PCIE_LPDDR4 ||
		GB02FUNC503(pcie_info) == PCIE_C0_200) {
		if (dptx_info->index != 2 && dptx_info->index != 3)
			return ;
	}
	/* PCIE_FULL_LPDDR4 DP1(lvds) have no use */
	if (GB02FUNC503(pcie_info) == PCIE_FULL_LPDDR4 &&
			dptx_info->index == 1)
			return ;

	if (GB02FUNC503(pcie_info) == PCIE_M4HL8G_LPDDR4 &&
			(dptx_info->index == 1 || dptx_info->index == 4))
			return ;

	if (atomic_read(&dptx_info->c_connect)) {
		atomic_set(&dptx_info->c_connect, 0);
		if (GB02FUNC1500(dptx_info->index)) {

			if (GB02FUNC1469(dptx_info)) {
				dptx_err(dptx_info, "%s:GB02FUNC1469 fail!\n",
					__func__);
				HDMI_power_reset(pcie_info, dptx_info->index);
				return;
			}
			pr_info("%s:%d----*****--hotplug--id=%x-\n",
					__func__, __LINE__, dptx_info->index);
		} else {
			if (GB02FUNC1286(dptx_info)) {
				dptx_err(dptx_info, "%s: GB02FUNC1286 fail!\n", __func__);
				return;
			}
			pr_info("%s:%d---***---hotunplug done idx=%x-\n",
					__func__, __LINE__, dptx_info->index);
		}
	}
	if (atomic_read(&dptx_info->sink_request)) {
		atomic_set(&dptx_info->sink_request, 0);
		pr_info("%s:%d------sink request---\n",
				__func__, __LINE__);
		if (GB02FUNC1276(dptx_info)) {
			dptx_err(dptx_info, "%s: unable to handle sink request fail!\n",
				__func__);
			return;
		}
	}
	while (!dptx_info->drm_dev) {
		if (retry_times > 1000) {
			dptx_err(dptx_info, "%s: err!dp%d no drm_dev\n",
				__func__, dptx_info->index);
			return;
		}
		msleep(10);
		retry_times++;
	}
	if (!GB02FUNC1500(dptx_info->index))
		HDMI_power_reset(pcie_info, dptx_info->index);
	if (!GB02FUNC1691(dptx_info->drm_dev,
                            dptx_info->index, NULL, false))
		drm_helper_hpd_irq_event(dptx_info->drm_dev);
	else if (GB02FUNC1691(dptx_info->drm_dev,
        dptx_info->index, NULL, false) /*&& GB02FUNC1500(dptx_info->index)*/
		&& !dptx_info->drm_dev->mode_config.suspend_state) {
		GB02FUNC1689(dptx_info->drm_dev, dptx_info->index);
		if (GB02FUNC1500(dptx_info->index))
			GB02FUNC1676(dptx_info->drm_dev);
		else
			GB02FUNC1677(dptx_info->drm_dev, dptx_info->index);
	}
	gb_printf(KERN_INFO, "%s-%d: board_type = %d, dp_index = %d hpd_work_func finish\n",
	__func__, __LINE__, GB02FUNC503(pcie_info), dptx_info->index);
}

irqreturn_t GB02FUNC1511(int irq, void *dev)
{
	irqreturn_t retval = IRQ_HANDLED;
	struct dptx *dptx = dev;
	u32 ists;
	u32 reg;
	u32 audio_fifo_en;
	u32 video_fifo_en;

	ists = dptx_readl(dptx, GB02MAC2126);
	dptx_info(dptx, "%s: >>>> ISTS=0x%08x\n", __func__, ists);

	if (!(ists & GB02MAC2231)) {
		retval = IRQ_NONE;
		dptx_dbg(dptx, "%s: IRQ_NONE\n", __func__);
		goto done;
	}

	if (ists & GB02MAC2225) {
		dptx_dbg(dptx, "%s: ERR!!GB02MAC2225\n", __func__);
		dptx_writel(dptx, GB02MAC2126,
					GB02MAC2225);
	}

	if (ists & GB02MAC2227) {
		dptx_dbg(dptx, "%s: GB02MAC2227\n", __func__);
		dptx_writel(dptx, GB02MAC2126,
					GB02MAC2227);
	}
	if (ists & GB02MAC2226) {
		dptx_dbg(dptx, "%s: GB02MAC2226\n", __func__);
		dptx_writel(dptx, GB02MAC2126,
					GB02MAC2226);
	}
	if (ists & GB02MAC2224) {
		dptx_dbg(dptx, "%s: GB02MAC2224\n", __func__);
		dptx_writel(dptx, GB02MAC2126,
					GB02MAC2224);
	}

	if (ists & GB02MAC2228) {
		reg = dptx_readl(dptx, GB02MAC2127);
		audio_fifo_en = reg & GB02MAC2237;
		if (audio_fifo_en) {
			dptx_dbg(dptx, "%s: GB02MAC2228\n",
				 __func__);
			dptx_writel(dptx, GB02MAC2126,
					GB02MAC2228);
		}
	}

	if (ists & GB02MAC2229) {
		reg = dptx_readl(dptx, GB02MAC2127);
		video_fifo_en = reg & GB02MAC2238;
		if (video_fifo_en) {
			dptx_dbg(dptx, "%s: GB02MAC2229\n",
				 __func__);
			dptx_writel(dptx, GB02MAC2126,
					GB02MAC2229);
		}
	}

	if (ists & GB02MAC2230) {
		dptx_dbg(dptx, "%s: GB02MAC2230\n", __func__);
		dptx_writel(dptx, GB02MAC2126,
					GB02MAC2230);
	}

	if (ists & GB02MAC2223) {
		u32 hpdsts;

		dptx_dbg(dptx, "%s: GB02MAC2223\n", __func__);
		hpdsts = dptx_readl(dptx, GB02MAC2128);

		dptx_dbg(dptx, "%s: HPDSTS = 0x%08x\n", __func__, hpdsts);

		if (hpdsts & GB02MAC2241) {
			dptx_dbg(dptx, "%s: GB02MAC2241\n", __func__);
			dptx_writel(dptx, GB02MAC2128, GB02MAC2241);
			atomic_set(&dptx->sink_request, 1);
		}

		if (hpdsts & GB02MAC2242) {
			dptx_dbg(dptx, "%s: GB02MAC2242 id=%x\n",
					__func__, dptx->index);
			dptx_writel(dptx, GB02MAC2128, GB02MAC2242);
			atomic_set(&dptx->aux.abort, 1);
			atomic_set(&dptx->c_connect, 1);
		}

		if (hpdsts & GB02MAC2243) {
			dptx_dbg(dptx, "%s: GB02MAC2243 id=%x\n",
				 __func__, dptx->index);
			dptx_writel(dptx, GB02MAC2128, GB02MAC2243);
			atomic_set(&dptx->aux.abort, 1);
			atomic_set(&dptx->c_connect, 1);
		}
		schedule_delayed_work(&dptx->hotplug_work, 0);
	}

done:
	dptx_info(dptx, "%s: <<<<\n", __func__);
	return retval;
}

