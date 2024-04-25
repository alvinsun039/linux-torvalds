// SPDX-License-Identifier: GPL-2.0
/*
 * cpu feature support for ARM64.
 *
 * Author:
 *   Jackie Liu <liuyun01@kylinos.cn>
 *   Riwen Lu <luriwen@kylinos.cn>
 *
 * Copyright (C) 2024 KylinSoft Corporation.
 */
#include <linux/seq_file.h>
#include <linux/arm-smccc.h>
#include <asm/machine_t.h>
#include <linux/elf.h>
#include <linux/dmi.h>

#define CPU_VERSION_SMC_FUNC_ID		0xC2000002
#define DMI_PROCESSOR_VERSION_OFFSET	0x10
#define DMI_PROCESSOR_MIN_LENGTH	48
#define MODEL_NAME_LEN			64

static u64 phytium_cpu_version;
/* CPU model name in cpuinfo */
static char arm64_model_name[MODEL_NAME_LEN];

const char *get_arm64_model_name(void)
{
	return arm64_model_name;
}

struct cpu_mode_desc {
	u64 model;
	const char *desc;
};

struct chip_id {
	uint32_t reg[4];
};

static void print_phytium_cpuid_info(struct seq_file *m)
{
	struct arm_smccc_res res;
	struct chip_id chip_id;

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
	if (read_cpuid_implementor() == ARM_CPU_IMP_PHYTIUM)
		phytium_cpu_version = phytium_cpu_version_init();
}

static struct cpu_mode_desc arm64_cpu_desc[] = {
	{ MIDR_CORTEX_A53,		"Cortext-A53" },
	{ MIDR_CORTEX_A57,		"Cortext-A57" },
	{ MIDR_THUNDERX,		"Cavium,thunder" },
	{ MIDR_FT_1500A,		"Phytium,FT-1500A" },
	{ MIDR_FT_2000AHK,		"Phytium,FT-2000+/2" },
	{ MIDR_FT_2000PLUS,		"Phytium,FT-2000+/64" },
	{ MIDR_FT_2000A_4,		"Phytium,FT-2000/4" },
	{ MIDR_FT_E2000_BIG,		"Phytium,FT-E2000/Big" },
	{ MIDR_FT_E2000_LITTLE,		"Phytium,FT-E2000/Little" },
	{ MIDR_FT_D3000,		"Phytium,FT-D3000" },
	{ MIDR_HISI_TSV110,		"HUAWEI,Kunpeng 920" },
	{ MIDR_HISI_TSV200,		"Kunpeng-920" },
	{ }
};

static struct cpu_mode_desc phytium_cpu_platform[] = {
	{ PHYTIUM_CPU_2000_4_X,		"Phytium,FT-2000/4-X" },
	{ PHYTIUM_CPU_2000_4,		"Phytium,FT-2000/4" },
	{ PHYTIUM_CPU_2000_4_S,		"Phytium,FT-2000/4-S" },
	{ PHYTIUM_CPU_2000_4_I,		"Phytium,FT-2000/4-I" },
	{ PHYTIUM_CPU_2000_4_SI,	"Phytium,FT-2000/4-SI" },
	{ PHYTIUM_CPU_2000_4_2,		"Phytium,FT-2000/4-2" },
	{ PHYTIUM_CPU_2000_4_I2,	"Phytium,FT-2000/4-I2" },
	{ PHYTIUM_CPU_2000_4_I1,	"Phytium,FT-2000/4-I1" },
	{ PHYTIUM_CPU_2000_4_M,		"Phytium,FT-2000/4-M" },
	{ PHYTIUM_CPU_2000_4_EN,	"Phytium,FT-2000/4-EN" },
	{ PHYTIUM_CPU_2000_4_BC,	"Phytium,FT-2000/4-BC" },
	{ PHYTIUM_CPU_2000_4_ULP4C,	"Phytium,FT-2000/4-ULP4C" },

	{ PHYTIUM_CPU_S2500_64,		"Phytium,S2500/64" },
	{ PHYTIUM_CPU_S2500_64_C00,	"Phytium,S2500/64 C00" },
	{ PHYTIUM_CPU_S2500_64_I00,	"Phytium,S2500/64 I00" },
	{ PHYTIUM_CPU_S2500_56_C00,	"Phytium,S2500/56 C00" },
	{ PHYTIUM_CPU_S2500_56_I00,	"Phytium,S2500/56 I00" },
	{ PHYTIUM_CPU_S2500_48_C00,	"Phytium,S2500/48 C00" },
	{ PHYTIUM_CPU_S2500_48_I00,	"Phytium,S2500/48 I00" },
	{ PHYTIUM_CPU_D2000_8,		"Phytium,D2000/8" },
	{ PHYTIUM_CPU_D2000_8_B8C,	"Phytium,D2000/8 B8C" },
	{ PHYTIUM_CPU_D2000_8_E8C,	"Phytium,D2000/8 E8C" },
	{ PHYTIUM_CPU_D2000_8_S8C,	"Phytium,D2000/8 S8C" },
	{ PHYTIUM_CPU_D2000_8_S8I,	"Phytium,D2000/8 S8I" },
	{ PHYTIUM_CPU_D2000_4_S4C,	"Phytium,D2000/4 S4C" },
	{ PHYTIUM_CPU_D2000_4_S4I,	"Phytium,D2000/4 S4I" },
	{ PHYTIUM_CPU_D2000_8_ULP8C,	"Phytium,D2000/8 ULP8C" },
	{ PHYTIUM_CPU_D2000_8_E8C2,	"Phytium,D2000/8 E8C" },
	{ PHYTIUM_CPU_D2000_8_M8C,	"Phytium,D2000/8 M8C" },
	{ PHYTIUM_CPU_D2000_8_EN8I,	"Phytium,D2000/8 EN8I" },

	{ PHYTIUM_CPU_S5000,		"Phytium,S5000" },

	{ PHYTIUM_CPU_E2000,		"Phytium,FT-E2000" },
	{ PHYTIUM_CPU_D3000,		"Phytium,FT-D3000" },
	{ }
};

static const char *arm64_get_cpu_desc_from_array(u32 midr)
{
	int i;
	u32 model = midr & MIDR_CPU_MODEL_MASK;

	/* use phytium cpu version */
	if (is_ft_all() && phytium_cpu_version) {
		for (i = 0; phytium_cpu_platform[i].desc; i++) {
			if (phytium_cpu_platform[i].model == phytium_cpu_version)
				return phytium_cpu_platform[i].desc;
		}
	}

	/* use MIDR */
	for (i = 0; arm64_cpu_desc[i].desc; i++)
		if (arm64_cpu_desc[i].model == model)
			return arm64_cpu_desc[i].desc;

	return NULL;
}

/* Callback function used to get processor version from DMI */
static void find_dmi_processor_version(const struct dmi_header *dm, void *private)
{
	const char *d = (const char *)dm;
	char *dmi_processor_ver = (char *)private;

	if (dm->type == DMI_ENTRY_PROCESSOR &&
	    dm->length >= DMI_PROCESSOR_MIN_LENGTH) {
		const char *bp = (const char *)dm + dm->length;
		const char *nsp;
		/* String order in a group of strings */
		u8 s = d[DMI_PROCESSOR_VERSION_OFFSET];

		if (s) {
			/* Find the string position according to the string
			 * order and the total length of the preceding string.
			 */
			while (--s > 0 && *bp)
				bp += strlen(bp) +  1;
			/* String containing only spaces are considered empty */
			nsp = bp;
			while (*nsp == ' ')
				nsp++;
			if (*nsp != '\0')
				strscpy_pad(dmi_processor_ver, bp, MODEL_NAME_LEN);
		}
	}
}

/* Should init after arm dmi init, before proc cpuinfo init. */
static int __init arm64_init_model_name(void)
{
	const char *name;
	u32 midr = read_cpuid_id();

	/* Use dmi information default in phytium platform. */
	if (is_ft_all() && !dmi_walk(find_dmi_processor_version, arm64_model_name))
		return 0;

	/* get desc from defined array */
	name = arm64_get_cpu_desc_from_array(midr);
	if (name) {
		sprintf(arm64_model_name, "%s", name);
		return 0;
	}

	/* normal arm64 */
	sprintf(arm64_model_name, "ARMv8 Processor rev %d (%s)",
		MIDR_REVISION(midr), COMPAT_ELF_PLATFORM);

	return 0;
}
arch_initcall(arm64_init_model_name);
