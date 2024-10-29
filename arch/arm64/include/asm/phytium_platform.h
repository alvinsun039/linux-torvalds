/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
/*
 * This file is mainly used to match the Model provided by PHYTIUM firmware
 * and obtain the corresponding CPU type through the `arm_smccc_smc` interface.
 * It is different from MDIR. On the Phytium platform, the model obtained
 * through this interface will be preferred.
 *
 * Authors:
 *   Jackie Liu <liuyun01@kylinos.cn>
 *   Riwen Lu <luriwen@kylinos.cn>
 *
 * Copyright (C) 2018-2024 KylinSoft Corporation.
 */

#ifndef __PHYTIUM_PLATFORM_H_
#define __PHYTIUM_PLATFORM_H_

#include <asm/cputype.h>

#define PHYTIUM_CPU_2000_4_X		0x000300
#define PHYTIUM_CPU_2000_4		0x000301
#define PHYTIUM_CPU_2000_4_S		0x000302
#define PHYTIUM_CPU_2000_4_I		0x000303
#define PHYTIUM_CPU_2000_4_SI		0x000304
#define PHYTIUM_CPU_2000_4_2		0x000305
#define PHYTIUM_CPU_2000_4_I2		0x000306
#define PHYTIUM_CPU_2000_4_I1		0x000307
#define PHYTIUM_CPU_2000_4_M		0x000308
#define PHYTIUM_CPU_2000_4_EN		0x000311
#define PHYTIUM_CPU_2000_4_BC		0x000313
#define PHYTIUM_CPU_2000_4_ULP4C	0x000318

#define PHYTIUM_CPU_S2500_64		0x000400
#define PHYTIUM_CPU_S2500_64_C00	0x000401
#define PHYTIUM_CPU_S2500_64_I00	0x000402
#define PHYTIUM_CPU_S2500_56_C00	0x000403
#define PHYTIUM_CPU_S2500_56_I00	0x000404
#define PHYTIUM_CPU_S2500_48_C00	0x000405
#define PHYTIUM_CPU_S2500_48_I00	0x000406

#define PHYTIUM_CPU_D2000_8		0x000500
#define PHYTIUM_CPU_D2000_8_B8C		0x000501
#define PHYTIUM_CPU_D2000_8_E8C		0x000502
#define PHYTIUM_CPU_D2000_8_S8C		0x000503
#define PHYTIUM_CPU_D2000_8_EN8I	0x000504
#define PHYTIUM_CPU_D2000_4_S4C		0x000505
#define PHYTIUM_CPU_D2000_4_S4I		0x000506
#define PHYTIUM_CPU_D2000_8_ULP8C	0x000508
#define PHYTIUM_CPU_D2000_8_E8C2	0x000509
#define PHYTIUM_CPU_D2000_8_M8C		0x00050A
#define PHYTIUM_CPU_D2000_8_S8I		0x00050D

#define PHYTIUM_CPU_S5000		0x000600
#define PHYTIUM_CPU_E2000		0x000700
#define PHYTIUM_CPU_D3000		0x000900

#define PHYTIUM_LPC_SIRQ_BIT_KBD	1
#define PHYTIUM_LPC_SIRQ_BIT_EC		11
#define PHYTIUM_LPC_SIRQ_BIT_AUX	12

u8 ft_lpc_read(u8 addr);
u8 ft_lpc_write(u8 value, u8 addr);
int phytium_lpc_irq_find_mapping(u32 offset);

#endif // __PHYTIUM_PLATFORM_H_
