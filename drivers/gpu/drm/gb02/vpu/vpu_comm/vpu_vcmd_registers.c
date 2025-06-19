// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <linux/kernel.h>
#include <linux/module.h>
#include "asm/io.h"
#include "vpu_vcmd_registers.h"
#include "vpu_io.h"
#include "common/gb_common.h"

const regVcmdField_s asicVcmdRegisterDesc[] =
{
#include "vpu_vcmd_registertable.h"
};

/* 2. External compiler flags  3. Module defines */
u32 GB02FUNC313(const void *hwregs, u32 offset)
{
	u32 val;

	val = (u32)ioread32((void *)hwregs + offset);
	gb_printf(KERN_INFO, "%s: 0x%02x --> %08x\n", __func__, offset, val);
	return val;
}

void GB02FUNC318(const void *hwregs, u32 offset, u32 val)
{
	iowrite32(val, (void *)hwregs + offset);
	gb_printf(KERN_INFO, "%s: vcmd write reg offset: 0x%02x with value %08x\n",
			__func__, offset, val);
}

/*  GB02FUNC321 */
/* Write a value into a defined register field (write will happens) */
void GB02FUNC321(const void *hwregs, u32 *reg_mirror,
	regVcmdName name, u32 value)
{
	const regVcmdField_s *field;
	u32 regVal;

	field = &asicVcmdRegisterDesc[name];
#ifdef DEBUG_PRINT_REGS
	gb_printf(KERN_INFO, "%s: 0x%2x  0x%08x  Value: %10d  %s\n",
		__func__, field->base, field->mask, value, field->description);
#endif

	/* Check that value fits in field */
	gb_printf(KERN_INFO, "%s: field->name = %d, name = %d\n",
	 __func__, field->name, name);
	gb_printf(KERN_INFO, "((field->mask >> field->lsb) << field->lsb)=%d, field->mask=%d\n",
	 ((field->mask >> field->lsb) << field->lsb), field->mask);
	gb_printf(KERN_INFO, "(field->mask >> field->lsb) >= value=%d\n",
	 (field->mask >> field->lsb) >= value);
	gb_printf(KERN_INFO, "field->base < GB02MAC4*4=%d\n",
	 field->base < GB02MAC4 * 4);

	/* Clear previous value of field in register */
	regVal = reg_mirror[field->base/4] & ~(field->mask);

	/* Put new value of field in register */
	reg_mirror[field->base / 4] = regVal
	 | ((value << field->lsb) & field->mask);

	/* write it into HW registers */
	GB02FUNC318(hwregs, field->base, reg_mirror[field->base / 4]);
}

/*  GB02FUNC325 */
/*	Get an unsigned value from the ASIC registers */
u32 GB02FUNC325(const void *hwregs, u32 *reg_mirror,
	regVcmdName name)
{
	const regVcmdField_s *field;
	u32 value;

	field = &asicVcmdRegisterDesc[name];
	gb_printf(KERN_INFO, "%s: field->base = %d, GB02MAC4 * 4 = %d\n",
	 __func__, field->base, GB02MAC4 * 4);
	value = reg_mirror[field->base / 4] =
	 GB02FUNC313(hwregs, field->base);
	value = (value & field->mask) >> field->lsb;

	return value;
}


