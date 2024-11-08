// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/*
 * Machine type interface of KYLINOS.
 *
 * Authors: Jackie Liu <liuyun01@kylinos.cn>
 * Copyright (C) 2018-2024 KylinSoft Corporation.
 */
#include <linux/export.h>

bool __weak is_cpu_ft1500a(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_ft1500a);

bool __weak is_cpu_ft2000a4(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_ft2000a4);

bool __weak is_cpu_ft2000ahk(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_ft2000ahk);

bool __weak is_cpu_ft2000plus(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_ft2000plus);

bool __weak is_cpu_ft2000pc(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_ft2000pc);

bool __weak is_cpu_ft2500(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_ft2500);

bool __weak is_cpu_ftd2000(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_ftd2000);

bool __weak is_cpu_ftd3000(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_ftd3000);

bool __weak is_cpu_fte2000(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_fte2000);

bool __weak is_cpu_fts5000(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_fts5000);

bool __weak is_cpu_kunpeng920(void)
{
	return false;
}
EXPORT_SYMBOL(is_cpu_kunpeng920);

bool __weak is_vendor_phytium(void)
{
	return false;
}
EXPORT_SYMBOL(is_vendor_phytium);

bool __weak is_vendor_hisilicon(void)
{
	return false;
}
EXPORT_SYMBOL(is_vendor_hisilicon);

bool __weak is_vendor_hygon(void)
{
	return false;
}
EXPORT_SYMBOL(is_vendor_hygon);

bool __weak is_vendor_zhaoxin(void)
{
	return false;
}
EXPORT_SYMBOL(is_vendor_zhaoxin);
