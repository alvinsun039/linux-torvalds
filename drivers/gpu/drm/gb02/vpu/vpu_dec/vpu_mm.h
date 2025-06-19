/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * jessy 20221019 Sietium
 */
#ifndef __VPU_MM_H__
#define __VPU_MM_H__

#include <linux/delay.h>
#include <linux/version.h>

#define GB02MAC1066  'm'

#define HANTRO_IOCS_MMU_MEM_MAP _IOWR(GB02MAC1066, 1, struct GB02STR33 *)
#define HANTRO_IOCS_MMU_MEM_UNMAP _IOWR(GB02MAC1066, 2, struct GB02STR33 *)
#define HANTRO_IOCS_MMU_FLUSH _IOWR(GB02MAC1066, 3, unsigned int *)
#define GB02MAC1067 3

#endif

