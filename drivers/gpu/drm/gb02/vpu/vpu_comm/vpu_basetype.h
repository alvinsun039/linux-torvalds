/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * jessy 20221019 Sietium
 */

#ifndef __VPU_BASETYPE_H__
#define __VPU_BASETYPE_H__
#include <stddef.h>
#include <linux/types.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#ifndef OK
#define OK 0
#endif
#ifndef NOK
#define NOK -1
#endif

/* Macro to signal unused parameter. */
#define UNUSED(x) (void)(x)

/** \ingroup common_group */
typedef unsigned char u8;
/** \ingroup common_group */
typedef signed char i8;
/** \ingroup common_group */
typedef unsigned short u16;
/** \ingroup common_group */
typedef signed short i16;
/** \ingroup common_group */
typedef unsigned int u32;
/** \ingroup common_group */
typedef signed int i32;
/** \ingroup common_group */
typedef long long i64;
/** \ingroup common_group */
typedef unsigned long long u64;

//! Boolean Type
#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef __FREERTOS__

#ifndef FREERTOS_SIMULATOR
typedef unsigned long long addr_t;
#else
typedef size_t addr_t;
#endif

#else

typedef size_t addr_t;
#endif

#define __u8	u8
#define __i8	i8
#define __u32	u32
#define __i32	i32

#endif /* BASE_TYPE_H */

