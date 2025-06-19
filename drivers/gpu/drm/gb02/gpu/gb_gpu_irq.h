
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __GB_IRQ_H__
#define __GB_IRQ_H__

#include <linux/bitfield.h>
#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/pci.h>

irqreturn_t GB02FUNC831(int irq, void *data);
irqreturn_t GB02FUNC833(struct GB02STR39 *gbdev, u32 status);

irqreturn_t GB02FUNC1334(int irq, void *data);
irqreturn_t GB02FUNC1336(int irq, void *data);
irqreturn_t GB02FUNC1339(int irq, void *data);
void GB02FUNC1331(struct GB02STR39 *gbdev);

irqreturn_t GB02FUNC961(int irq, void *data);
irqreturn_t GB02FUNC959(int irq, void *data);

int GB02FUNC886(struct GB02STR39 *gbdev);
int GB02FUNC893(struct GB02STR39 *gbdev);
void GB02FUNC882(struct GB02STR39 *gbdev);
irqreturn_t GB02FUNC877(int irq, void *data);

void GB02FUNC895(struct GB02STR39 *gbdev);
#endif
