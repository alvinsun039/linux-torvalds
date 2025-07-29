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

/* Table of contents*/
/* 1. Include headers 2. External compiler flags 3. Module defines */
#ifndef __VPU_VCMD_REGISTERS__H__
#define __VPU_VCMD_REGISTERS__H__

#ifdef __cplusplus
extern "C" {
#endif
/* 1. Include headers */
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
typedef size_t    addr_t;
typedef addr_t    ptr_t;

/* 2. External compiler flags  3. Module defines */
#define GB02MAC513            0X40
#define GB02MAC514         0X44
#define GB02MAC515            0X48
#define GB02MAC516       0X64

#ifdef HANTROVCMD_ENABLE_IP_SUPPORT
#define GB02MAC2     32
#define GB02MAC4   64
#else
#define GB02MAC4   27
#endif
/* HW Register field names */
typedef enum {
#include "vpu_vcmd_registerenum.h"
	VcmdRegisterAmount
} regVcmdName;

/* HW Register field descriptions */
typedef struct {
	u32 name;               /* Register name and index  */
	int base;               /* Register base address  */
	u32 mask;               /* Bitmask for this field */
	int lsb;                /* LSB for this field [31..0] */
	int trace;         /* Enable/disable writing in swreg_params.trc */
	int rw;                 /* 1=Read-only 2=Write-only 3=Read-Write */
	char *description;      /* Field description */
} regVcmdField_s;

/* Flags for read-only, write-only and read-write */
#define RO 1
#define WO 2
#define RW 3

#define REGBASE(reg) (asicVcmdRegisterDesc[reg].base)

/* Description field only needed for system model build. */
#ifdef TEST_DATA
#define VCMDREG(name, base, mask, lsb, trace, rw, desc) \
	{name, base, mask, lsb, trace, rw, desc}
#else
#define VCMDREG(name, base, mask, lsb, trace, rw, desc) \
	{name, base, mask, lsb, trace, rw, ""}
#endif


/* 4. Function prototypes */
extern const regVcmdField_s asicVcmdRegisterDesc[];

/* EncAsicSetRegisterValue Set a value into a defined register field */
static inline void GB02FUNC342(u32 *reg_mirror,
	regVcmdName name, u32 value)
{
	const regVcmdField_s *field;
	u32 regVal;

	field = &asicVcmdRegisterDesc[name];

#ifdef DEBUG_PRINT_REGS
	gb_printf(KERN_INFO, "%s: 0x%2x  0x%08x  Value: %10d  %s\n", __func__,
		field->base, field->mask, value, field->description);
#endif

	/* Clear previous value of field in register */
	regVal = reg_mirror[field->base / 4] & ~(field->mask);

	/* Put new value of field in register */
	reg_mirror[field->base / 4] = regVal
	 | ((value << field->lsb) & field->mask);
}

static inline u32 GB02FUNC344(u32 *reg_mirror,
	regVcmdName name)
{
	const regVcmdField_s *field;
	u32 regVal;

	field = &asicVcmdRegisterDesc[name];

	/* Check that value fits in field */
	regVal = reg_mirror[field->base / 4];
	regVal = (regVal & field->mask) >> field->lsb;

#ifdef DEBUG_PRINT_REGS
	gb_printf(KERN_INFO, "%s: 0x%2x  0x%08x  Value: %10d  %s\n", __func__,
		field->base, field->mask, regVal, field->description);
#endif
	return regVal;
}

u32 GB02FUNC313(const void *hwregs, u32 offset);

void GB02FUNC318(const void *hwregs, u32 offset, u32 val);
void GB02FUNC321(const void *hwregs, u32 *reg_mirror,
	regVcmdName name, u32 value);
u32 GB02FUNC325(const void *hwregs, u32 *reg_mirror,
	regVcmdName name);

#define vcmd_set_addr_register_value(reg_base, reg_mirror, name, value) do {\
	if (sizeof(addr_t) == 8) {\
	  GB02FUNC321((reg_base), (reg_mirror), name,\
	   (u32)((addr_t)value));\
	  GB02FUNC321((reg_base), (reg_mirror), name##_MSB,\
	   (u32)(((addr_t)value) >> 32));\
	} else {\
		GB02FUNC321((reg_base), (reg_mirror), name,\
		 (u32)((addr_t)value));\
	}\
} while (0)

#define VCMDGetAddrRegisterValue(reg_base, reg_mirror, name)\
	((sizeof(addr_t) == 8) ?\
	((((addr_t)GB02FUNC325((reg_base), (reg_mirror), name))\
	| (((addr_t)GB02FUNC325((reg_base), (reg_mirror),\
	name##_MSB)) << 32))) :\
	((addr_t)GB02FUNC325((reg_base), (reg_mirror), (name))))

#ifdef __cplusplus
}
#endif

#endif /* VCMD_SWHWREGISTERS_H */
