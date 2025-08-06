// SPDX-License-Identifier: GPL-2.0
/*
 * CPU feature support for ARM64.
 *
 * Author:
 *   Jackie Liu <liuyun01@kylinos.cn>
 *   Riwen Lu <luriwen@kylinos.cn>
 *
 * Copyright (C) 2024-2025 KylinSoft Corporation.
 */

#include <linux/seq_file.h>
#include <linux/arm-smccc.h>
#include <linux/machine_t.h>
#include <linux/elf.h>
#include <linux/dmi.h>
#include <asm/phytium_platform.h>

#define CPU_VERSION_SMC_FUNC_ID 0xC2000002
#define DMI_PROCESSOR_VERSION_OFFSET 0x10
#define DMI_PROCESSOR_MIN_LENGTH 48
#define MODEL_NAME_LEN 64

static u64 phytium_cpu_version;
static char arm64_model_name[MODEL_NAME_LEN];

/* CPU model name in cpuinfo */
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

/**
 * print_phytium_cpuid_info - Print Phytium CPU ID information
 * @m: seq_file structure to write the information
 *
 * This function retrieves the Phytium CPU ID using an ARM SMC call and
 * prints it to the provided seq_file structure.
 */
static void print_phytium_cpuid_info(struct seq_file *m)
{
	struct arm_smccc_res res;
	struct chip_id chip_id;

	arm_smccc_smc(CPU_VERSION_SMC_FUNC_ID, 0x0, 0x0, 0x0, 0, 0, 0, 0, &res);
	if (res.a0 != 0) // Firmware Call CPU_VERSION SMC Failed
		return;

	chip_id.reg[0] = (uint32_t)res.a2;
	chip_id.reg[1] = (uint32_t)(res.a2 >> 32);
	chip_id.reg[2] = (uint32_t)res.a3;
	chip_id.reg[3] = (uint32_t)(res.a3 >> 32);

	seq_printf(m, "CPUID:\t\t\t%x%x%x%x\n", chip_id.reg[0], chip_id.reg[1],
		   chip_id.reg[2], chip_id.reg[3]);
}

/**
 * print_cpuid_info - Print CPU ID information
 * @m: seq_file structure to write the information
 *
 * This function checks if the CPU vendor is Phytium and prints the
 * corresponding CPU ID information.
 */
void print_cpuid_info(struct seq_file *m)
{
	if (is_vendor_phytium())
		print_phytium_cpuid_info(m);
}

/**
 * phytium_check_cpu_type - Check CPU type for Phytium
 * @cpu_type: CPU type to check
 *
 * This function checks if the provided CPU type matches the Phytium
 * CPU version.
 *
 * Return: true if the CPU type matches, false otherwise.
 */
bool phytium_check_cpu_type(u32 cpu_type)
{
	if (!phytium_cpu_version)
		return false;

	return (cpu_type == (phytium_cpu_version & 0xFFFF00));
}
EXPORT_SYMBOL(phytium_check_cpu_type);

/**
 * phytium_cpu_type_set_default - Set the default Phytium CPU ID with MIDR and CPU count
 * @midr: MIDR value of the CPU
 *
 * This function sets the default Phytium CPU ID based on the MIDR value
 * and the number of possible CPUs.
 *
 * Return: The default Phytium CPU ID.
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

/**
 * phytium_cpu_version_init - Initialize Phytium CPU version
 *
 * This function initializes the Phytium CPU version by reading the
 * CPU ID and making an ARM SMC call.
 *
 * Return: The Phytium CPU version.
 */
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

/**
 * cpu_version_init - Initialize CPU version
 *
 * This function initializes the CPU version if the CPU vendor is Phytium.
 */
void __init cpu_version_init(void)
{
	if (is_vendor_phytium())
		phytium_cpu_version = phytium_cpu_version_init();
}

static struct cpu_mode_desc arm64_cpu_desc[] = {
	{ MIDR_CORTEX_A53, "Cortext-A53" },
	{ MIDR_CORTEX_A57, "Cortext-A57" },
	{ MIDR_THUNDERX, "Cavium,thunder" },
	{ MIDR_FT_1500A, "Phytium,FT-1500A" },
	{ MIDR_FT_2000AHK, "Phytium,FT-2000+/2" },
	{ MIDR_FT_2000PLUS, "Phytium,FT-2000+/64" },
	{ MIDR_FT_2000A_4, "Phytium,FT-2000/4" },
	{ MIDR_FT_E2000_BIG, "Phytium,FT-E2000/Big" },
	{ MIDR_FT_E2000_LITTLE, "Phytium,FT-E2000/Little" },
	{ MIDR_FT_D3000, "Phytium,FT-D3000" },
	{ MIDR_HISI_TSV110, "HUAWEI,Kunpeng 920" },
	{ MIDR_HISI_TSV200, "Kunpeng-920" },
	{}
};

static struct cpu_mode_desc phytium_cpu_platform[] = {
	{ PHYTIUM_CPU_2000_4_X, "Phytium,FT-2000/4-X" },
	{ PHYTIUM_CPU_2000_4, "Phytium,FT-2000/4" },
	{ PHYTIUM_CPU_2000_4_S, "Phytium,FT-2000/4-S" },
	{ PHYTIUM_CPU_2000_4_I, "Phytium,FT-2000/4-I" },
	{ PHYTIUM_CPU_2000_4_SI, "Phytium,FT-2000/4-SI" },
	{ PHYTIUM_CPU_2000_4_2, "Phytium,FT-2000/4-2" },
	{ PHYTIUM_CPU_2000_4_I2, "Phytium,FT-2000/4-I2" },
	{ PHYTIUM_CPU_2000_4_I1, "Phytium,FT-2000/4-I1" },
	{ PHYTIUM_CPU_2000_4_M, "Phytium,FT-2000/4-M" },
	{ PHYTIUM_CPU_2000_4_EN, "Phytium,FT-2000/4-EN" },
	{ PHYTIUM_CPU_2000_4_BC, "Phytium,FT-2000/4-BC" },
	{ PHYTIUM_CPU_2000_4_ULP4C, "Phytium,FT-2000/4-ULP4C" },
	{ PHYTIUM_CPU_S2500_64, "Phytium,S2500/64" },
	{ PHYTIUM_CPU_S2500_64_C00, "Phytium,S2500/64 C00" },
	{ PHYTIUM_CPU_S2500_64_I00, "Phytium,S2500/64 I00" },
	{ PHYTIUM_CPU_S2500_56_C00, "Phytium,S2500/56 C00" },
	{ PHYTIUM_CPU_S2500_56_I00, "Phytium,S2500/56 I00" },
	{ PHYTIUM_CPU_S2500_48_C00, "Phytium,S2500/48 C00" },
	{ PHYTIUM_CPU_S2500_48_I00, "Phytium,S2500/48 I00" },
	{ PHYTIUM_CPU_D2000_8, "Phytium,D2000/8" },
	{ PHYTIUM_CPU_D2000_8_B8C, "Phytium,D2000/8 B8C" },
	{ PHYTIUM_CPU_D2000_8_E8C, "Phytium,D2000/8 E8C" },
	{ PHYTIUM_CPU_D2000_8_S8C, "Phytium,D2000/8 S8C" },
	{ PHYTIUM_CPU_D2000_8_S8I, "Phytium,D2000/8 S8I" },
	{ PHYTIUM_CPU_D2000_4_S4C, "Phytium,D2000/4 S4C" },
	{ PHYTIUM_CPU_D2000_4_S4I, "Phytium,D2000/4 S4I" },
	{ PHYTIUM_CPU_D2000_8_ULP8C, "Phytium,D2000/8 ULP8C" },
	{ PHYTIUM_CPU_D2000_8_E8C2, "Phytium,D2000/8 E8C" },
	{ PHYTIUM_CPU_D2000_8_M8C, "Phytium,D2000/8 M8C" },
	{ PHYTIUM_CPU_D2000_8_EN8I, "Phytium,D2000/8 EN8I" },
	{ PHYTIUM_CPU_S5000, "Phytium,S5000" },
	{ PHYTIUM_CPU_E2000, "Phytium,FT-E2000" },
	{ PHYTIUM_CPU_D3000, "Phytium,FT-D3000" },
	{}
};

/**
 * arm64_get_cpu_desc_from_array - Get CPU description from array
 * @midr: MIDR value of the CPU
 *
 * This function retrieves the CPU description from the array based on
 * the MIDR value.
 *
 * Return: The CPU description string if found, NULL otherwise.
 */
static const char *arm64_get_cpu_desc_from_array(u32 midr)
{
	if (is_vendor_phytium() && phytium_cpu_version) {
		for (int i = 0; phytium_cpu_platform[i].desc; i++) {
			if (phytium_cpu_platform[i].model ==
			    phytium_cpu_version)
				return phytium_cpu_platform[i].desc;
		}
	}

	for (int i = 0; arm64_cpu_desc[i].desc; i++) {
		if (arm64_cpu_desc[i].model == (midr & MIDR_CPU_MODEL_MASK))
			return arm64_cpu_desc[i].desc;
	}

	return NULL;
}

/**
 * find_dmi_processor_version - Callback function used to get processor version from DMI
 * @dm: DMI header structure
 * @private: Private data (processor version string)
 *
 * This function is a callback used to retrieve the processor version
 * from the DMI table.
 */
static void find_dmi_processor_version(const struct dmi_header *dm,
				       void *private)
{
	const char *d = (const char *)dm;
	char *dmi_processor_ver = (char *)private;

	if (dm->type == DMI_ENTRY_PROCESSOR &&
	    dm->length >= DMI_PROCESSOR_MIN_LENGTH) {
		const char *bp = (const char *)dm + dm->length;
		const char *nsp;
		u8 s = d[DMI_PROCESSOR_VERSION_OFFSET];

		if (s) {
			while (--s > 0 && *bp)
				bp += strlen(bp) + 1;

			nsp = bp;
			while (*nsp == ' ')
				nsp++;

			if (*nsp != '\0')
				strscpy_pad(dmi_processor_ver, bp,
					    MODEL_NAME_LEN);
		}
	}
}

/**
 * arm64_init_model_name - Initialize model name
 *
 * This function initializes the model name by reading the CPU ID and
 * retrieving the description from the DMI table or the CPU description array.
 *
 * Return: 0 on success.
 */
static int __init arm64_init_model_name(void)
{
	const char *name;
	u32 midr = read_cpuid_id();

	if (is_vendor_phytium() &&
	    !dmi_walk(find_dmi_processor_version, arm64_model_name) &&
	    arm64_model_name[0])
		return 0;

	name = arm64_get_cpu_desc_from_array(midr);
	if (name) {
		sprintf(arm64_model_name, "%s", name);
		return 0;
	}

	sprintf(arm64_model_name, "ARMv8 Processor rev %d (%s)",
		MIDR_REVISION(midr), COMPAT_ELF_PLATFORM);
	return 0;
}
arch_initcall(arm64_init_model_name);
