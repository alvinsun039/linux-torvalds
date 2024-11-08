// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/*
 * Authors:
 *   Jackie Liu <liuyun01@kylinos.cn>
 *   Shi Tao <shitao@kylinos.cn>
 *
 * Copyright (C) 2018-2024 KylinSoft Corporation.
 */

#include <linux/machine_t.h>
#include <linux/processor.h>

bool is_vendor_hygon(void)
{
	if (!IS_BUILTIN(CONFIG_CPU_SUP_HYGON))
		return false;

	return boot_cpu_data.x86_vendor == X86_VENDOR_HYGON;
}

bool is_vendor_zhaoxin(void)
{
	if (!IS_BUILTIN(CONFIG_CPU_SUP_ZHAOXIN) && !IS_BUILTIN(CONFIG_CPU_SUP_CENTAUR))
		return false;

	return (boot_cpu_data.x86_vendor == X86_VENDOR_ZHAOXIN ||
		boot_cpu_data.x86_vendor == X86_VENDOR_CENTAUR);
}
