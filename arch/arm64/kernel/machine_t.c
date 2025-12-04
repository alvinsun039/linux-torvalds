// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/*
 * Machine type interface of KYLINOS.
 *
 * Copyright (C) 2018 - 2025 KylinSoft Co., Ltd. All rights reserved.
 * Copyright (C) 2018 - 2025 Jackie Liu <liuyun01@kylinos.cn>
 * Copyright (C) 2024 Riwen Lu <luriwen@kylinos.cn>
 */

#include <linux/machine_t.h>
#include <linux/jump_label.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/cache.h>
#include <linux/arm-smccc.h>
#include <linux/cpumask.h>
#include <asm/phytium_platform.h>

u64 phytium_cpu_version;
EXPORT_SYMBOL(phytium_cpu_version);

/*
 * Define CPU type check functions with static_key optimization.
 * Each function returns true only when the specific CPU type is detected at boot.
 */
#define DEFINE_CPU_CHECK_FUNC(name, config)                                 \
	DEFINE_STATIC_KEY_FALSE_RO(machine_t_cpu_##name##_key);             \
	bool is_cpu_##name(void)                                            \
	{                                                                   \
		if (!IS_BUILTIN(config))                                    \
			return false;                                       \
		return static_branch_unlikely(&machine_t_cpu_##name##_key); \
	}

/* CPU type check functions */
DEFINE_CPU_CHECK_FUNC(ft1500a, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(ft2000a4, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(ft2000ahk, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(ft2000plus, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(ft2000pc, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(ft2500, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(ftd2000, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(ftd3000, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(fte2000, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(fts5000, CONFIG_ARCH_PHYTIUM);
DEFINE_CPU_CHECK_FUNC(kunpeng920, CONFIG_ARCH_HISI);

/* Vendor check functions without static_key (CPU register read is already fast) */
#define DEFINE_VENDOR_CHECK_FUNC(name, config, implementer)       \
	bool is_vendor_##name(void)                               \
	{                                                         \
		if (!IS_BUILTIN(config))                          \
			return false;                             \
		return read_cpuid_implementor() == (implementer); \
	}

DEFINE_VENDOR_CHECK_FUNC(phytium, CONFIG_ARCH_PHYTIUM, ARM_CPU_IMP_PHYTIUM);
DEFINE_VENDOR_CHECK_FUNC(hisilicon, CONFIG_ARCH_HISI, ARM_CPU_IMP_HISI);

static u32 phytium_cpu_type_set_default(u32 midr)
{
	u32 cpuid = 0;
	unsigned int cpu_count = num_possible_cpus();

	switch (midr) {
	case MIDR_FT_2000A_4:
		if (cpu_count == 8)
			cpuid = PHYTIUM_CPU_D2000_8;
		else if (cpu_count > 8)
			cpuid = PHYTIUM_CPU_S2500_64;
		else
			cpuid = PHYTIUM_CPU_2000_4_X;
		break;
	case MIDR_FT_E2000_BIG:
	case MIDR_FT_E2000_LITTLE:
		cpuid = PHYTIUM_CPU_E2000;
		break;
	case MIDR_FT_D3000:
		cpuid = PHYTIUM_CPU_D3000;
		break;
	}

	return cpuid;
}

static u32 phytium_cpu_version_init(void)
{
	struct arm_smccc_res res;
	u32 midr = read_cpuid_id() & MIDR_CPU_MODEL_MASK;

	if (midr == MIDR_FT_1500A || midr == MIDR_FT_2000AHK ||
	    midr == MIDR_FT_2000PLUS)
		return 0;

	arm_smccc_smc(CPU_VERSION_SMC_FUNC_ID, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0) {
		pr_debug("It's not support arm smc service!\n");
		return phytium_cpu_type_set_default(midr);
	}
	return res.a1;
}

static int __init detect_phytium_cpu_type(void)
{
	u32 midr = read_cpuid_id() & MIDR_CPU_MODEL_MASK;

	switch (midr) {
	case MIDR_FT_1500A:
		static_branch_enable(&machine_t_cpu_ft1500a_key);
		return 0;
	case MIDR_FT_2000AHK:
		static_branch_enable(&machine_t_cpu_ft2000ahk_key);
		return 0;
	case MIDR_FT_2000PLUS:
		static_branch_enable(&machine_t_cpu_ft2000plus_key);
		return 0;
	}

	phytium_cpu_version = phytium_cpu_version_init();
	switch (phytium_cpu_version & 0xFFFF00) {
	case PHYTIUM_CPU_2000_4_X:
		static_branch_enable(&machine_t_cpu_ft2000a4_key);
		static_branch_enable(&machine_t_cpu_ft2000pc_key);
		return 0;
	case PHYTIUM_CPU_S2500_64:
		static_branch_enable(&machine_t_cpu_ft2500_key);
		return 0;
	case PHYTIUM_CPU_D2000_8:
		static_branch_enable(&machine_t_cpu_ftd2000_key);
		static_branch_enable(&machine_t_cpu_ft2000pc_key);
		return 0;
	case PHYTIUM_CPU_D3000:
		static_branch_enable(&machine_t_cpu_ftd3000_key);
		return 0;
	case PHYTIUM_CPU_E2000:
		static_branch_enable(&machine_t_cpu_fte2000_key);
		return 0;
	case PHYTIUM_CPU_S5000:
		static_branch_enable(&machine_t_cpu_fts5000_key);
		return 0;
	default:
		return -ENODATA;
	}
}

static int __init detect_hisi_cpu_type(void)
{
	u32 midr = read_cpuid_id() & MIDR_CPU_MODEL_MASK;

	switch (midr) {
	case MIDR_HISI_TSV110:
	case MIDR_HISI_HIP09:
		static_branch_enable(&machine_t_cpu_kunpeng920_key);
		return 0;
	default:
		return -ENODATA;
	}
}

/*
 * Detect CPU type at boot time and enable corresponding static_key.
 * This allows is_cpu_*() functions to have near-zero overhead.
 */
static int __init machine_t_init(void)
{
	if (!IS_ENABLED(CONFIG_KYLIN_DIFFERENCES))
		return 0;

	if (IS_BUILTIN(CONFIG_ARCH_PHYTIUM) &&
	    (read_cpuid_implementor() == ARM_CPU_IMP_PHYTIUM))
		return detect_phytium_cpu_type();

	if (IS_BUILTIN(CONFIG_ARCH_HISI) &&
	    (read_cpuid_implementor() == ARM_CPU_IMP_HISI))
		return detect_hisi_cpu_type();

	return 0;
}
arch_initcall(machine_t_init);
