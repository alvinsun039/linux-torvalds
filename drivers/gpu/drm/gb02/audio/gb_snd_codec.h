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
#ifndef __GB_SND_CODEC_H__
#define __GB_SND_CODEC_H__
#include <linux/platform_device.h>
#include "i2s_platform.h"

#define GB02MAC697	4
#define GB02MAC699	256
#define GB02MAC701	16000
#define GB02MAC703	48000
#define GB02MAC705	(4096)
#define GB02MAC707	(256)
#define GB02MAC710	(4096)
#define GB02MAC713	(32 * 1024)

#define GB02MAC717	(192 * 4)
#define GB02MAC720	32768
#define GB02MAC723	4

/* Sample index defination depends on IEC60958-3@1999 */
#define GB02MAC728                    1
#define GB02MAC731                   2
#define GB02MAC733                5
#define GB02MAC736               6
#define GB02MAC739                    41
#define GB02MAC742                   44
#define GB02MAC744   49
#define GB02MAC747  50
#define GB02MAC750   51
#define GB02MAC753  52
#define GB02MAC756   53
#define GB02MAC759  54
#define GB02MAC762   55
#define GB02MAC765  56
#define GB02MAC768   65
#define GB02MAC770  66
#define GB02MAC772         67
#define GB02MAC774        68
#define GB02MAC776         69
#define GB02MAC779        70
#define GB02MAC782         71
#define GB02MAC785        72

#define GB02MAC789                  26
#define GB02MAC792                   28
#define GB02MAC793                     384

int GB02FUNC486(struct device *dev);
void GB02FUNC273(int num, struct GB02STR86 *i2s_cinfo);
void GB02FUNC311(int dp_num, struct GB02STR86 *i2s_cinfo);
u32 GB02FUNC415(struct snd_pcm_runtime *runtime, u64 pos);
#ifdef AUDIO_SOUND_SWITCH_DIFF_A
int GB02FUNC360(struct snd_pcm_substream *substream, int cmd);
#endif
void GB02FUNC478(struct GB02STR83 *pcie_info);
#endif
