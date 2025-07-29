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


#ifndef __VPU_SUBSYS1_H__
#define __VPU_SUBSYS1_H__

#include "vpu/vpu_comm/vpu_vcmd_registers.h"
#include "vpu_dec.h"

void CheckSubsysCoreArray1(struct GB02STR23 *subsys, int *vcmd,
	int *total_vcmd_core_num);

#endif
