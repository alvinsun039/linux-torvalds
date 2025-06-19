/* SPDX-License-Identifier: GPL-3.0-or-later */
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
#ifndef __GB_IP_OFFSET_H__
#define __GB_IP_OFFSET_H__
#include "common/gb_common.h"

#ifdef GB02MAC482
#define GB02MAC1142 0x1400000
#define GB02MAC1143 0x1800000
#define GB02MAC1144  0x1C00000
#endif

#ifdef FPGA_MODE
#define GB02MAC1142 0x81400000
#define GB02MAC1143 0x81800000
#define GB02MAC1144  0x81C00000
#endif

#define GB02MAC1145 0x80400000
#define GB02MAC1146 0x800
#define GB02MAC1147  6


struct GB02STR126 {
	u32	dec0_offset_base;
	u32 dec1_offset_base;
	u32 enc_offset_base;
};

struct GB02STR127 {
	u32	GB02STR127;
	u32 audio_interval;
	u32 audio_num;
};

struct GB02STR128 {

};

struct GB02STR129 {
	u32	GB02STR129;
	u32 gpu_interval;
	u32 gpu_num;
};



struct GB02STR130 {
	struct GB02STR126	vpu_offset;
	struct GB02STR127 audio_offset;
	struct GB02STR128 dcdp_offset;
	struct GB02STR129 gpu_offset;
};


#endif
