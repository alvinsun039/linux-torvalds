// SPDX-License-Identifier: GPL-2.0
/*
 * cpuid support for Arm64.
 *
 * Author: Jackie Liu <liuyun01@kylinos.cn>
 * Copyright (C) 2024 KylinSoft Corporation.
 */
#include <linux/seq_file.h>
#include <linux/arm-smccc.h>
#include <asm/machine_t.h>

#define CPU_VERSION_SMC_FUNC_ID		0xC2000002

static u64 phytium_cpu_version;

typedef struct {
	uint32_t reg[4];
} chip_id_t;

static void print_phytium_cpuid_info(struct seq_file *m)
{
	struct arm_smccc_res res;
	chip_id_t chip_id;

	arm_smccc_smc(CPU_VERSION_SMC_FUNC_ID, 0x0, 0x0, 0x0, 0, 0, 0, 0, &res);

	/* Firmware Call CPU_VERSION SMC Failed */
	if (res.a0 != 0)
		return;

	chip_id.reg[0] = (uint32_t)res.a2;
	chip_id.reg[1] = (uint32_t)(res.a2 >> 32);
	chip_id.reg[2] = (uint32_t)res.a3;
	chip_id.reg[3] = (uint32_t)(res.a3 >> 32);

	seq_printf(m, "CPUID:\t\t\t%x%x%x%x\n", chip_id.reg[0], chip_id.reg[1],
		   chip_id.reg[2], chip_id.reg[3]);
}

void print_cpuid_info(struct seq_file *m)
{
	if (is_ft_all())
		return print_phytium_cpuid_info(m);
}

/*
 * phytium_check_cpu_type - Check cpu type for Phytium.
 * @cpu_type: Phytium cpu type series
 */
bool phytium_check_cpu_type(u32 cpu_type)
{
	if (!phytium_cpu_version)
		return false;

	/* phytium_cpu_version & 0xFFFF00: Phytium cpu series */
	if (cpu_type == (phytium_cpu_version & 0xFFFF00))
		return true;
	else
		return false;
}
EXPORT_SYMBOL(phytium_check_cpu_type);

/*
 * Set the default Phytium cpuid with midr and cpu_count.
 */
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

	/*
	 * 1500a\2000ahk\2000plus implement is_part(midr) func in machine_t.h.
	 */
	if (midr == MIDR_FT_1500A || midr == MIDR_FT_2000AHK ||
	    midr == MIDR_FT_2000PLUS)
		return 0;

	arm_smccc_smc(CPU_VERSION_SMC_FUNC_ID, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0) {
		pr_debug("It's not support arm smc service!\n");
		/* Old Phytium bpf do not support SMC service */
		return phytium_cpu_type_set_default(midr);
	}
	return res.a1;
}

void __init cpu_version_init(void)
{
	u32 implementer = read_cpuid_implementor();

	if (implementer == ARM_CPU_IMP_PHYTIUM)
		phytium_cpu_version = phytium_cpu_version_init();
}
