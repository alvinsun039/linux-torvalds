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
