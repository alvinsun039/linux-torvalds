// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/*
 * Authors:
 *   Jackie Liu <liuyun01@kylinos.cn>
 *   Riwen Lu <luriwen@kylinos.cn>
 *
 * Copyright (C) 2018-2024 KylinSoft Corporation.
 */

#include <linux/machine_t.h>
#include <asm/phytium_platform.h>

bool phytium_check_cpu_type(u32 cpu_type);

static inline bool is_part(u32 cpuid)
{
	return (read_cpuid_id() & MIDR_CPU_MODEL_MASK) == cpuid;
}

static inline bool is_implementer(u32 implementer)
{
	return read_cpuid_implementor() == implementer;
}

bool is_cpu_ft1500a(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return is_part(MIDR_FT_1500A);
}

bool is_cpu_ft2000a4(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return phytium_check_cpu_type(PHYTIUM_CPU_2000_4_X);
}

bool is_cpu_ft2000ahk(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return is_part(MIDR_FT_2000AHK);
}

bool is_cpu_ft2000plus(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return is_part(MIDR_FT_2000PLUS);
}

bool is_cpu_ft2000pc(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return is_part(MIDR_FT_2000A_4) && !is_cpu_ft2500();
}

bool is_cpu_ft2500(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return phytium_check_cpu_type(PHYTIUM_CPU_S2500_64);
}

bool is_cpu_ftd2000(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return phytium_check_cpu_type(PHYTIUM_CPU_D2000_8);
}

bool is_cpu_ftd3000(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return phytium_check_cpu_type(PHYTIUM_CPU_D3000);
}

bool is_cpu_fte2000(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return phytium_check_cpu_type(PHYTIUM_CPU_E2000);
}

bool is_cpu_fts5000(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return phytium_check_cpu_type(PHYTIUM_CPU_S5000);
}

bool is_cpu_kunpeng920(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_HISI))
		return false;

	return is_part(MIDR_HISI_TSV110) || is_part(MIDR_HISI_HIP09);
}

bool is_vendor_phytium(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_PHYTIUM))
		return false;

	return is_implementer(ARM_CPU_IMP_PHYTIUM);
}

bool is_vendor_hisilicon(void)
{
	if (!IS_BUILTIN(CONFIG_ARCH_HISI))
		return false;

	return is_implementer(ARM_CPU_IMP_HISI);
}
