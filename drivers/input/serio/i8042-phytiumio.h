/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _I8042_PHYTIUMIO_H
#define _I8042_PHYTIUMIO_H

#include <linux/of_irq.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/acpi.h>
#include <asm/phytium_platform.h>

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

/*
 * Names.
 */

#define I8042_KBD_PHYS_DESC "isa0060/serio0"
#define I8042_AUX_PHYS_DESC "isa0060/serio1"
#define I8042_MUX_PHYS_DESC "isa0060/serio%d"

/*
 * IRQs.
 */
static int kbd_irq;
static int aux_irq;

#define I8042_KBD_IRQ  kbd_irq
#define I8042_AUX_IRQ  aux_irq

static u32 i8042_data_port = 0x60UL;
static u32 i8042_command_port = 0x64UL;
static u32 i8042_status_port = 0x64UL;

/*
 * Register numbers.
 */
#define I8042_COMMAND_REG	i8042_command_port
#define I8042_STATUS_REG	i8042_status_port
#define I8042_DATA_REG		i8042_data_port

#define LPC_STATUS_REG		0xFFF4
#define LPC_INTERRUPT_REG	0xFFF0
#define LPC_IRQMODE_REG		0xFFE8

static inline int i8042_read_data(void)
{
	return ft_lpc_read(I8042_DATA_REG);
}

static inline int i8042_read_status(void)
{
	return ft_lpc_read(I8042_STATUS_REG);
}

static inline void i8042_write_data(int val)
{
	ft_lpc_write(val, I8042_DATA_REG);
}

static inline void i8042_write_command(int val)
{
	ft_lpc_write(val, I8042_COMMAND_REG);
}

static const struct acpi_device_id i8042_acpi_match[] = {
	{ "KBCI8042", 0 },
	{}
};
MODULE_DEVICE_TABLE(acpi, i8042_acpi_match);

static inline int i8042_platform_init(void)
{
	if (!acpi_dev_present(i8042_acpi_match[0].id, NULL, -1) &&
	    !of_find_compatible_node(NULL, NULL, "phytium,i8042"))
		return -ENODEV;

	kbd_irq = phytium_lpc_irq_find_mapping(PHYTIUM_LPC_SIRQ_BIT_KBD);
	aux_irq = phytium_lpc_irq_find_mapping(PHYTIUM_LPC_SIRQ_BIT_AUX);

	if (!kbd_irq && !aux_irq) {
		pr_err("Can't get i8042 kbd/aux irq\n");
		return -EINVAL;
	}

	if (of_find_compatible_node(NULL, NULL, "czc,laptop")) {
		i8042_data_port = 0x180UL;
		i8042_command_port = 0x190UL;
		i8042_status_port = 0x190UL;
	}

	i8042_reset = 1;

	return 0;
}

static inline void i8042_platform_exit(void)
{
	return;
}

#endif /* _I8042_FT1500A_H */
