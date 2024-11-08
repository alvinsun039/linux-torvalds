/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
/*
 * Machine type interface of KYLINOS.
 *
 * Authors: Jackie Liu <liuyun01@kylinos.cn>
 * Copyright (C) 2018-2024 KylinSoft Corporation.
 */

#ifndef __MACHINE_TYPE_H_
#define __MACHINE_TYPE_H_

#include <linux/types.h>

/* CPU */
bool is_cpu_ft1500a(void);
bool is_cpu_ft2000a4(void);
bool is_cpu_ft2000ahk(void);
bool is_cpu_ft2000plus(void);
bool is_cpu_ft2000pc(void);
bool is_cpu_ft2500(void);
bool is_cpu_ftd2000(void);
bool is_cpu_ftd3000(void);
bool is_cpu_fte2000(void);
bool is_cpu_fts5000(void);
bool is_cpu_kunpeng920(void);

/* Vendor */
bool is_vendor_phytium(void);
bool is_vendor_hisilicon(void);
bool is_vendor_hygon(void);
bool is_vendor_zhaoxin(void);

#endif // __MACHINE_TYPE_H_
