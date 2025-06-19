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

#ifndef __DPTX_AVGEN_H__
#define __DPTX_AVGEN_H__

#define GB02MAC1125 0x441B8400
struct dptx;

struct GB02STR122 {
	u8 iec_channel_numcl0;
	u8 iec_channel_numcr0;
	u8 use_lut;
	u8 iec_samp_freq;
	u8 iec_word_length;
	u8 iec_orig_samp_freq;
	u8 data_width;
	u8 num_channels;
	u8 inf_type;
	u8 mute;
	u8 ats_ver;
};

enum audio_sample_freq {
	SAMPLE_FREQ_32 = 0,
	SAMPLE_FREQ_44_1 = 1,
	SAMPLE_FREQ_48 = 2,
	SAMPLE_FREQ_88_2 = 3,
	SAMPLE_FREQ_96 = 4,
	SAMPLE_FREQ_176_4 = 5,
	SAMPLE_FREQ_192 = 6
};

struct GB02STR123 {
	u8 max_num_of_channels;
	enum audio_sample_freq max_sampling_freq;
	u8 max_bit_per_sample;
};

void GB02FUNC792(struct GB02STR122 *aparams);
void GB02FUNC461(struct dptx *dptx);
void GB02FUNC463(struct dptx *dptx);
void GB02FUNC487(struct dptx *dptx);
void GB02FUNC442(struct dptx *dptx, int ch_num, int enable);
void GB02FUNC458(struct dptx *dptx);

void GB02FUNC466(struct dptx *dptx);
void GB02FUNC470(struct dptx *dptx);
void GB02FUNC475(struct dptx *dptx);

enum pixel_enc_type {
	RGB = 0,
	YCBCR420 = 1,
	YCBCR422 = 2,
	YCBCR444 = 3,
	YONLY = 4,
	RAW = 5
};

enum color_depth {
	COLOR_DEPTH_INVALID = 0,
	COLOR_DEPTH_6 = 6,
	COLOR_DEPTH_8 = 8,
	COLOR_DEPTH_10 = 10,
	COLOR_DEPTH_12 = 12,
	COLOR_DEPTH_16 = 16
};

enum pattern_mode {
	TILE = 0,
	RAMP = 1,
	CHESS = 2,
	COLRAMP = 3
};

enum dynamic_range_type {
	CEA = 1,
	VESA = 2
};

enum colorimetry_type {
	ITU601 = 1,
	ITU709 = 2
};

enum video_format_type {
	VCEA = 0,
	CVT = 1,
	DMT = 2
};

struct dtd {
	u16 pixel_repetition_input;
	int pixel_clock;
	/** 1 for interlaced, 0 progressive */
	u8 interlaced;
	u16 h_active;
	u16 h_blanking;
	u16 h_image_size;
	u16 h_sync_offset;
	u16 h_sync_pulse_width;
	u8 h_sync_polarity;
	u16 v_active;
	u16 v_blanking;
	u16 v_image_size;
	u16 v_sync_offset;
	u16 v_sync_pulse_width;
	u8 v_sync_polarity;
};

static inline int GB02FUNC811(struct dtd *mdtd, u8 code, u32 refresh_rate,
	u8 video_format)
{
	return 0;
}

void GB02FUNC798(struct dtd *mdtd);

struct GB02STR124 {
	u8 pix_enc;
	u8 pattern_mode;
	struct dtd mdtd;
	u8 mode;
	u8 bpc;
	u8 colorimetry;
	u8 dynamic_range;
	u8 aver_bytes_per_tu;
	u8 aver_bytes_per_tu_frac;
	u8 init_threshold;
	u32 refresh_rate;
	u8 video_format;
	// DSC staff here TODO: move to special DSC struct
	u16 slice_width;
	u16 chunk_size;
	u16 slice_height;
	int encoders;
	u8 first_line_bpg_offset;
	u32 minRateBufferSize;
	u16 dsc_bpp;
	u16 dsc_bpc;
	u32 hrddelay;
	u32 initial_dec_delay;
	u16 initial_scale_value;
	u32 scale_decrement_interval;
};

int GB02FUNC393(struct dptx *dptx, struct dtd *mdtd, u8 data[18]);
void GB02FUNC795(struct dptx *dptx);

void GB02FUNC492(struct dptx *dptx, int stream);
int GB02FUNC493(struct dptx *dptx, u8 vmode, int stream);
int GB02FUNC496(struct dptx *dptx, int stream);
void GB02FUNC663(struct dptx *dptx, int stream);
void GB02FUNC500(struct dptx *dptx, int stream);
void GB02FUNC708(struct dptx *dptx, int stream);
void GB02FUNC719(struct dptx *dptx, int stream);
void GB02FUNC725(struct dptx *dptx, int stream);
void GB02FUNC720(struct dptx *dpx, int stream);
void GB02FUNC781(struct dptx *dptx, int stream);
void GB02FUNC706(struct dptx *dptx, int stream);
void GB02FUNC576(struct dptx *dptx);
void GB02FUNC504(struct dptx *dptx, int stream);
void GB02FUNC699(struct dptx *dptx, int stream);
int GB02FUNC682(struct dptx *dptx, int lane_num, int rate,
	int bpc, int encoding, int pixel_clock);
void GB02FUNC784(struct dptx *dptx, u8 pattern, int stream);
void GB02FUNC789(struct dptx *dptx, int stream);
void GB02FUNC788(struct dptx *dptx, int stream);
void GB02FUNC454(struct dptx *dptx, int enable, int stream);
void GB02FUNC483(struct dptx *dptx);
void GB02FUNC404(struct dptx *dptx);
void GB02FUNC434(struct dptx *dptx, u8 enable);
#endif
