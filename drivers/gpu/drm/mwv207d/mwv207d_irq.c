/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>

#include "mwv207d_drv.h"
#include "mwv207d_irq.h"

struct mwv207d_irq_desc {
	u32 reg;
	u32 bit;
};

struct mwv207d_irq {
	void __iomem *mmio;
	struct irq_domain *domain;
	u32 enabled[5];
	u32 stat_reg;
	int stat_reg_nr;
	u32 enable_reg;
	int enable_reg_nr;
	int max_irq;

	u32 enable_status[0x1D];

	spinlock_t lock;
	const struct mwv207d_irq_desc *irq_desc;
	bool is_pf;
	int mode;
};

static const struct mwv207d_irq_desc pf_irq_desc[0x86] = {
	{ 0x14, 16 },
	{ 0x14, 24 },
	{ 0x18, 0  },
	{ 0x18, 8  },
	{ 0x18, 16 },
	{ 0x18, 24 },
	{ 0x1C, 0  },
	{ 0x1C, 8  },
	{ 0x2c, 0  },
	{ 0x2c, 8  },
	{ 0x2c, 16 },
	{ 0x2c, 24 },
	{ 0x04, 0  },
	{ 0x04, 8  },
	{ 0x04, 16 },
	{ 0x04, 24 },
	{ 0x08, 0  },
	{ 0x08, 8  },
	{ 0x08, 16 },
	{ 0x08, 24 },
	{ 0x28, 0  },
	{ 0x28, 8  },
	{ 0x1C, 24 },
	{ 0x28, 24 },
	{ 0x20, 0  },
	{ 0x20, 8  },
	{ 0x20, 16 },
	{ 0x20, 24 },
	{ 0x24, 0  },
	{ 0x24, 8  },
	{ 0x24, 16 },
	{ 0x24, 24 },
	{ 0x28, 16 },
	{ 0x00, 16 },
	{ 0x00, 8  },
	{ 0x00, 0  },
	{ 0x00, 24 },
	{ 0x0C, 0  },
	{ 0x0C, 8  },
	{ 0x0C, 16 },
	{ 0x1C, 16 },
	{ 0x0C, 24 },
	{ 0x14, 0  },
	{ 0x14, 8  },
	{ 0x10, 0  },
	{ 0x10, 8  },
	{ 0x10, 16 },
	{ 0x10, 24 },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x64, 0  },
	{ 0x64, 8  },
	{ 0x64, 16 },
	{ 0x64, 24 },
	{ 0x68, 0  },
	{ 0x68, 8  },
	{ 0x68, 16 },
	{ 0x68, 24 },
	{ 0x44, 0  },
	{ 0x44, 8  },
	{ 0x44, 16 },
	{ 0x44, 24 },
	{ 0x48, 0  },
	{ 0x48, 8  },
	{ 0x48, 16 },
	{ 0x48, 24 },
	{ 0x4C, 0  },
	{ 0x4C, 8  },
	{ 0x4C, 16 },
	{ 0x4C, 24 },
	{ 0x50, 0  },
	{ 0x50, 8  },
	{ 0x50, 16 },
	{ 0x50, 24 },
	{ 0x6C, 0  },
	{ 0x6C, 8  },
	{ 0x6C, 16 },
	{ 0x6C, 24 },
	{ 0x70, 0  },
	{ 0x70, 8  },
	{ 0x70, 16 },
	{ 0x70, 24 },
	{ 0x5C, 0  },
	{ 0x5C, 8  },
	{ 0x5C, 16 },
	{ 0x5C, 24 },
	{ 0x60, 0  },
	{ 0x60, 8  },
	{ 0x60, 16 },
	{ 0x60, 24 },
	{ 0x54, 0  },
	{ 0x54, 8  },
	{ 0x54, 16 },
	{ 0x54, 24 },
	{ 0x58, 0  },
	{ 0x58, 8  },
	{ 0x58, 16 },
	{ 0x58, 24 },
	{ 0x3C, 0  },
	{ 0x3C, 8  },
	{ 0x3C, 16 },
	{ 0x3C, 24 },
	{ 0x40, 0  },
	{ 0x40, 8  },
	{ 0x40, 16 },
	{ 0x40, 24 },
	{ 0x34, 0  },
	{ 0x34, 8  },
	{ 0x34, 16 },
	{ 0x34, 24 },
	{ 0x38, 0  },
	{ 0x38, 8  },
	{ 0x38, 16 },
	{ 0x38, 24 },
	{ 0x30, 0  },
	{ 0x30, 8  },
	{ 0x28, 24 },
	{ 0x28, 24 },
	{ 0x30, 16 },
	{ 0x30, 24 }
};

static const struct mwv207d_irq_desc vf_irq_desc[0x38] = {
	{ 0x00, 0  },
	{ 0x00, 8  },
	{ 0x00, 16 },
	{ 0x04, 16 },
	{ 0x04, 24 },
	{ 0x04, 0  },
	{ 0x04, 8  },
	{ 0x08, 0  },
	{ 0x08, 8  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x00, 0  },
	{ 0x0C, 0  },
	{ 0x0C, 8  },
	{ 0x0C, 16 },
	{ 0x0C, 24 },
	{ 0x10, 0  },
	{ 0x10, 8  },
	{ 0x10, 16 },
	{ 0x10, 24 },
	{ 0x14, 0  },
	{ 0x14, 8  },
	{ 0x14, 16 },
	{ 0x14, 24 },
	{ 0x18, 0  },
	{ 0x18, 8  },
	{ 0x18, 16 },
	{ 0x18, 24 },
	{ 0x1C, 0  },
	{ 0x1C, 8  },
	{ 0x1C, 16 },
	{ 0x1C, 24 },
	{ 0x20, 0  },
	{ 0x20, 8  },
	{ 0x20, 16 },
	{ 0x20, 24 },
};

static inline void irq_write(struct mwv207d_irq *irq, u32 reg, u32 val)
{
	writel_relaxed(val, irq->mmio + reg);
}

static inline u32 irq_read(struct mwv207d_irq *irq, u32 reg)
{
	return readl_relaxed(irq->mmio + reg);
}

static inline void irq_modify(struct mwv207d_irq *irq, u32 reg, u32 mask, u32 val)
{
	u32 new_val = irq_read(irq, reg);

	new_val = (new_val & ~mask) | (val & mask);
	irq_write(irq, reg, new_val);
}

static const u32 irq_masks[2][5] = {

	{ 0x000001ffu, 0x00ffffffu, },

	{ 0xffffffffu, 0x0000ffffu, 0xffffffffu, 0xffffffffu, 0x0000003fu }
};

static irqreturn_t mwv207d_isr(int irq_unused, void *dev_id)
{
	struct mwv207d_irq *irq = dev_id;
	irqreturn_t ret = IRQ_NONE;
	int i, j, hwirq, virq;
	u32 stat;

	for (i = 0; i < irq->stat_reg_nr; i++) {
		stat = irq_read(irq, irq->stat_reg + i * 4);
		stat &= irq_masks[irq->is_pf][i];
		stat &= irq->enabled[i];

		while ((j = ffs(stat))) {
			j--;
			hwirq = i * 32 + j;
			virq = irq_find_mapping(irq->domain, hwirq);
			if (virq) {
				ret = generic_handle_irq(virq);
				if (ret < 0)
					pr_warn("warning, mwv207d hwirq(%d) handled with %d",
					hwirq, ret);
				irq_read(irq, irq->stat_reg);
			} else
				pr_warn("warning, mwv207d no irq mapping set on %d",
				hwirq);

			irq_write(irq, irq->stat_reg + i * 4, (1UL << j));
			stat &= ~(1UL << j);
			ret = IRQ_HANDLED;
		}
	}

	return ret;
}

static void mwv207d_irq_mask(struct irq_data *d)
{
	struct mwv207d_irq *irq = irq_data_get_irq_chip_data(d);
	int hwirq = irqd_to_hwirq(d);
	unsigned long flags;
	u32 reg, mask;

	reg = irq->enable_reg + irq->irq_desc[hwirq].reg;
	mask = 0xffu << irq->irq_desc[hwirq].bit;

	spin_lock_irqsave(&irq->lock, flags);
	irq_modify(irq, reg, mask, 0);
	irq->enabled[hwirq / 32] &= ~(0x1u << (hwirq % 32));
	spin_unlock_irqrestore(&irq->lock, flags);
}

static void mwv207d_irq_unmask(struct irq_data *d)
{
	struct mwv207d_irq *irq = irq_data_get_irq_chip_data(d);
	int hwirq = irqd_to_hwirq(d);
	unsigned long flags;
	u32 reg, mask, val;

	reg = irq->enable_reg + irq->irq_desc[hwirq].reg;
	mask = 0xffu << irq->irq_desc[hwirq].bit;
	val = 0x1u << irq->irq_desc[hwirq].bit;

	spin_lock_irqsave(&irq->lock, flags);
	irq_modify(irq, reg, mask, val);
	irq->enabled[hwirq / 32] |= 0x1u << (hwirq % 32);
	spin_unlock_irqrestore(&irq->lock, flags);
}

static void mwv207d_irq_setup_mode(struct mwv207d_irq *irq)
{
	if (!irq->is_pf)
		return;

	irq_write(irq, 0x190, 0xf);
	irq_write(irq, 0x194, 0x1);
	irq_write(irq, 0x154, 0x0);

	irq_write(irq, 0x158, irq->mode);
}

void mwv207d_irq_suspend(struct mwv207d_device *mdev)
{
	struct mwv207d_irq *irq = mdev->irq;
	int i;

	if (!irq)
		return;

	for (i = 0; i < irq->enable_reg_nr; i++)
		irq->enable_status[i] = irq_read(irq, irq->enable_reg + i * 4);
}

void mwv207d_irq_resume(struct mwv207d_device *mdev)
{
	struct mwv207d_irq *irq = mdev->irq;
	int i;

	if (!irq)
		return;

	mwv207d_irq_setup_mode(irq);

	for (i = 0; i < irq->stat_reg_nr; i++)
		irq_write(irq, irq->stat_reg + i * 4, 0xffffffff);
	for (i = 0; i < irq->enable_reg_nr; ++i)
		irq_write(irq, irq->enable_reg + i * 4, irq->enable_status[i]);
}

static struct irq_chip mwv207d_irq_chip = {
	.name           = "mwv207d",
	.irq_mask       = mwv207d_irq_mask,
	.irq_unmask     = mwv207d_irq_unmask,
};

static int mwv207d_irq_domain_map(struct irq_domain *d, unsigned int virq,
				  irq_hw_number_t hwirq)
{
	struct mwv207d_irq *irq = d->host_data;

	if (hwirq >= irq->max_irq)
		return -EPERM;

	irq_set_chip_and_handler(virq, &mwv207d_irq_chip, handle_simple_irq);
	irq_set_chip_data(virq, irq);

	return 0;
}

static const struct irq_domain_ops mwv207d_irq_domain_ops = {
	.map = mwv207d_irq_domain_map,
};

static void mwv207d_irq_regs_init(struct mwv207d_irq *irq)
{
	int i;

	if (irq->is_pf) {
		irq->max_irq = 0x86;
		irq->stat_reg = 0x78;
		irq->stat_reg_nr = 0x5;
		irq->enable_reg = 0x0;
		irq->enable_reg_nr = 0x1D;
		irq->irq_desc = pf_irq_desc;
	} else {
		irq->max_irq = 0x38;
		irq->stat_reg = 0x24;
		irq->stat_reg_nr = 0x2;
		irq->enable_reg = 0x0;
		irq->enable_reg_nr = 0x9;
		irq->irq_desc = vf_irq_desc;
	}

	for (i = 0; i < irq->stat_reg_nr; i++)
		irq_write(irq, irq->stat_reg + i * 4, 0xffffffff);
	for (i = 0; i < irq->enable_reg_nr; i++)
		irq_write(irq, irq->enable_reg + i * 4, 0x0);
}

int mwv207d_irq_init(struct mwv207d_device *mdev)
{
	struct pci_dev *pdev = to_pci_dev(mdev->dev);
	struct mwv207d_irq *irq;
	int ret, i;

	if (mdev->isr_poll)
		return 0;

	irq = devm_kzalloc(mdev->dev, sizeof(*irq), GFP_KERNEL);
	if (!irq)
		return -ENOMEM;
	mdev->irq = irq;
	irq->is_pf = mdev->hw.is_pf;
	irq->mmio = mdev->mmio +
		(irq->is_pf ? 0x2EF800 : 0x10000);
	spin_lock_init(&irq->lock);

	mwv207d_irq_regs_init(irq);

	irq->domain = irq_domain_add_linear(NULL, irq->max_irq,
					    &mwv207d_irq_domain_ops, irq);
	if (!irq->domain)
		return -ENODEV;

	for (i = 0; i < irq->max_irq; ++i)
		irq_create_mapping(irq->domain, i);

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_LEGACY);
	if (ret < 1) {
		ret = -ENODEV;
		goto free_mapping;
	}

	irq->mode = pdev->msi_enabled ? 1 : 2;
	mwv207d_irq_setup_mode(irq);

	ret = request_irq(pdev->irq, mwv207d_isr, IRQF_SHARED, "mwv207d_isr", irq);
	if (ret) {
		dev_err(mdev->dev, "error, request irq failed");
		goto free_vec;
	}

	return 0;

free_vec:
	pci_free_irq_vectors(pdev);
free_mapping:
	for (i = 0; i < irq->max_irq; ++i)
		irq_dispose_mapping(irq_find_mapping(irq->domain, i));
	irq_domain_remove(irq->domain);
	devm_kfree(mdev->dev, irq);
	mdev->irq = NULL;
	return ret;
}

void mwv207d_irq_fini(struct mwv207d_device *mdev)
{
	struct pci_dev *pdev = to_pci_dev(mdev->dev);
	struct mwv207d_irq *irq = mdev->irq;
	int i;

	if (!irq)
		return;

	free_irq(pdev->irq, irq);
	pci_free_irq_vectors(pdev);

	for (i = 0; i < irq->max_irq; ++i)
		irq_dispose_mapping(irq_find_mapping(irq->domain, i));
	irq_domain_remove(irq->domain);
	devm_kfree(mdev->dev, irq);
	mdev->irq = NULL;
}

u32 mwv207d_irq_find(struct mwv207d_device *mdev, u32 pf_irq, u32 vf_irq)
{
	struct mwv207d_irq *irq = mdev->irq;

	return irq_find_mapping(irq->domain, irq->is_pf ? pf_irq : vf_irq);
}
