/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
/*
 *  Copyright (C) 2018-2024 KylinSoft Corporation.
 */

#ifndef __MACHINE_TYPE_H_
#define __MACHINE_TYPE_H_

#include <asm/cputype.h>

void cpu_version_init(void);
bool phytium_check_cpu_type(u32 cpu_type);

/*
 * Phytium CPU VENDOR_ID defined as:
 * SIGNATURE_32 ('P','H','Y','T') in bios
 */
#define PHYTIUM_VENDOR_ID	0x54594850

/* Phytium 2000a4 */
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

/* Phytium 2500 */
#define PHYTIUM_CPU_S2500_64		0x000400
#define PHYTIUM_CPU_S2500_64_C00	0x000401
#define PHYTIUM_CPU_S2500_64_I00	0x000402
#define PHYTIUM_CPU_S2500_56_C00	0x000403
#define PHYTIUM_CPU_S2500_56_I00	0x000404
#define PHYTIUM_CPU_S2500_48_C00	0x000405
#define PHYTIUM_CPU_S2500_48_I00	0x000406

/* Phytium D2000 */
#define PHYTIUM_CPU_D2000_8		0x000500
#define PHYTIUM_CPU_D2000_8_B8C		0x000501
#define PHYTIUM_CPU_D2000_8_E8C		0x000502
#define PHYTIUM_CPU_D2000_8_S8C		0x000503
#define PHYTIUM_CPU_D2000_8_S8I		0x000504
#define PHYTIUM_CPU_D2000_4_S4C		0x000505
#define PHYTIUM_CPU_D2000_4_S4I		0x000506
#define PHYTIUM_CPU_D2000_8_ULP8C	0x000508
#define PHYTIUM_CPU_D2000_8_E8C2	0x000509
#define PHYTIUM_CPU_D2000_8_M8C		0x00050A
#define PHYTIUM_CPU_D2000_8_EN8I	0x00050D

/* Phytium S5000 */
#define PHYTIUM_CPU_S5000		0x000600

/* Phytium E2000 */
#define PHYTIUM_CPU_E2000		0x000700

/* Phytium D3000 */
#define PHYTIUM_CPU_D3000		0x000900

static inline bool is_part(u32 cpuid)
{
	return (read_cpuid_id() & MIDR_CPU_MODEL_MASK) == cpuid;
}

static inline bool is_implementer(u32 implementer)
{
	return read_cpuid_implementor() == implementer;
}

static inline bool is_ft1500a(void)
{
	return is_part(MIDR_FT_1500A);
}

static inline bool is_ft2000a4(void)
{
	return phytium_check_cpu_type(PHYTIUM_CPU_2000_4_X);
}

static inline bool is_ft2000ahk(void)
{
	return is_part(MIDR_FT_2000AHK);
}

static inline bool is_ft2000plus(void)
{
	return is_part(MIDR_FT_2000PLUS);
}

static inline bool is_ft2500(void)
{
	return phytium_check_cpu_type(PHYTIUM_CPU_S2500_64);
}

static inline bool is_ftd2000(void)
{
	return phytium_check_cpu_type(PHYTIUM_CPU_D2000_8);
}

static inline bool is_ft2000_pc(void)
{
	return is_part(MIDR_FT_2000A_4) && !is_ft2500();
}

static inline bool is_ftd3000(void)
{
	return phytium_check_cpu_type(PHYTIUM_CPU_D3000);
}

static inline bool is_fte2000(void)
{
	return phytium_check_cpu_type(PHYTIUM_CPU_E2000);
}

static inline bool is_fts5000(void)
{
	return phytium_check_cpu_type(PHYTIUM_CPU_S5000);
}

static inline bool is_ft_all(void)
{
	return is_implementer(ARM_CPU_IMP_PHYTIUM);
}

static inline bool is_kunpeng920(void)
{
	return is_part(MIDR_HISI_TSV110) || is_part(MIDR_HISI_TSV200);
}

static inline bool is_hisi(void)
{
	return is_implementer(ARM_CPU_IMP_HISI);
}

#endif // __MACHINE_TYPE_H_
