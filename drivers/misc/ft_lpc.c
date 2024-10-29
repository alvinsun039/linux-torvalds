// SPDX-License-Identifier: GPL-2.0
#include <linux/types.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <asm/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>

#include <asm/phytium_platform.h>
#include <linux/acpi.h>
#include <linux/machine_t.h>
#include <linux/i8042.h>
#include <linux/delay.h>
#include <linux/dmi.h>

#define	PHYTIUM_LPC_DRIVER_NAME		"phytium-lpc"

void __iomem *lpc_base;
EXPORT_SYMBOL_GPL(lpc_base);

int lpc_irq = -1;
EXPORT_SYMBOL_GPL(lpc_irq);

/* LPC sync mode */
#define SYNC_CYCLE_SHORT	1
#define SYNC_CYCLE_LONG		0
#define SYNC_PROPERTY_SHORT		"short"
#define SYNC_PROPERTY_LONG		"long"
static int sync_cycle = SYNC_CYCLE_LONG;

/* the lpc-gpio configure address */
#define LPCGPIO_ADDR		0x28100c00

/* LPC register shift with different Phytium CPU */
#define LPC_SHIFT		(is_cpu_fte2000() || is_cpu_ftd3000() ? 0x30 : 0)
#define LPC_IO_OFFSET		0x7FF0000

/* LPC register offset */
#define LPC_INT_MASK		(0xFFD8 - LPC_SHIFT)
#define LPC_SERIRQ_CONFIG	(0xFFE8 - LPC_SHIFT)
#define LPC_INT_STATE		(0xFFF4 - LPC_SHIFT)
#define LPC_CLR_IRQ		(0xFFF0 - LPC_SHIFT)

/*
 * Debug
 */
#define DEBUG
#ifdef DEBUG
static bool ft_lpc_debug;
module_param_named(debug, ft_lpc_debug, bool, 0600);
MODULE_PARM_DESC(debug, "Turn Phytium lpc debugging mode on and off");

#define lpc_dbg(format, arg...)						\
	do {								\
		if (ft_lpc_debug)					\
			pr_debug(KBUILD_MODNAME " " format, ##arg);	\
	} while (0)
#else
#define lpc_dbg(format, arg...)						\
	do {								\
		if (0)							\
			pr_debug(pr_fmt(format), ##arg);		\
	} while (0)
#endif

#ifdef CONFIG_PM_SLEEP
/* Store LPC context across system-wide suspend/resume transitions */
struct phytium_lpc_ctx {
	u8	int_mask;
	u32	sirq_cfg;
	u32	int_state;
	u8	wake_en;
} lpc_ctx;
#endif

struct phytium_lpc {
	struct device		*dev;
	struct fwnode_handle	*fwnode;
	void __iomem		*regs;
	struct irq_domain	*domain;
	struct irq_chip_generic	*gc;
	int			irq;
	int			irq_num;
} *ft_lpc;

static const struct dmi_system_id phytium_machine_table[] = {
	{
		.ident = "706 FT2000/4 laptop",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4251"),
		},
	},
	{
		.ident = "706 FT2000/4 laptop",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4252"),
		},
	},
	{
		.ident = "706 D2000/8 laptop",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4260"),
		},
	},
	{
		.ident = "706 D2000/8 laptop",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4261"),
		},
	},
	{
		.ident = "706 D2000/8 laptop",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4263"),
		},
	},
	{ }
};

#ifdef CONFIG_OF
static void phytium_lpc_probe_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	void __iomem *lpc_gpio_vaddr;

	if (of_property_read_bool(np, "phytium,lpc-gpio")) {
		lpc_gpio_vaddr = ioremap(LPCGPIO_ADDR, 4);
		writel(0x6, lpc_gpio_vaddr);
		iounmap(lpc_gpio_vaddr);
	}
}
#else
static void phytium_lpc_probe_dt(struct platform_device *pdev)
{
}
#endif

/**
 * ft_lpc_read - Read data from LPC.
 * @addr: The offset address of LPC to read.
 *
 * It should use the exported symbol ft_lpc_{read, write} to read and write
 * the LPC, and should not use the lpc_base directly elsewhere.
 *
 * Return: The value it read。
 */
u8 ft_lpc_read(u8 addr)
{
	u8 value;

	if (!lpc_base) {
		lpc_dbg("%s: phytium lpc not prepared!\n", __func__);
		return 0;
	}

	value = readb(lpc_base + addr);
	if (sync_cycle == SYNC_CYCLE_SHORT)
		value = readb(lpc_base + addr);

	return value;
}
EXPORT_SYMBOL(ft_lpc_read);

/**
 * ft_lpc_write - Write data to LPC.
 * @value: The value to write
 * @addr: The offset address of LPC to write.
 *
 * Use ft_lpc_write function to write data to LPC address, avoid use lpc_base
 * directly elsewhere.
 */
u8 ft_lpc_write(u8 value, u8 addr)
{
	if (!lpc_base) {
		lpc_dbg("%s: phytium lpc not prepared!\n", __func__);
		return 0;
	}

	writeb(value, lpc_base + addr);

	return 0;
}
EXPORT_SYMBOL(ft_lpc_write);

#define FTLPC_CTL_TIMEOUT  1000
static int ftlpc_i8042_wait_read(void)
{
	int i = 0;

	while ((~ft_lpc_read(0x64) & I8042_STR_OBF) &&
	       (i < FTLPC_CTL_TIMEOUT)) {
		udelay(50);
		i++;
	}
	return -(i == FTLPC_CTL_TIMEOUT);
}

static int ftlpc_i8042_wait_write(void)
{
	int i = 0;

	while ((ft_lpc_read(0x64) & I8042_STR_IBF) &&
	       (i < FTLPC_CTL_TIMEOUT)) {
		udelay(50);
		i++;
	}
	return -(i == FTLPC_CTL_TIMEOUT);
}

/* The sequence of disabling i8042 interface */
static int ftlpc_disable_i8042_interfaces(void)
{
	u8 i8042_ctr;
	int error;

	error = ftlpc_i8042_wait_write();
	if (error)
		return error;
	ft_lpc_write(I8042_CMD_CTL_RCTR & 0xff, 0x64);
	error = ftlpc_i8042_wait_read();
	if (error) {
		pr_debug("     -- ftlpc (wait read timeout)\n");
		return error;
	}
	i8042_ctr = ft_lpc_read(0x60);
	i8042_ctr |= I8042_CTR_KBDDIS | I8042_CTR_AUXDIS;
	i8042_ctr &= ~(I8042_CTR_KBDINT | I8042_CTR_AUXINT);
	error = ftlpc_i8042_wait_write();
	if (error)
		return error;
	ft_lpc_write(I8042_CMD_CTL_WCTR & 0xff, 0x64);
	error = ftlpc_i8042_wait_write();
	if (error)
		return error;
	ft_lpc_write(i8042_ctr, 0x60);

	return 0;
}

/**
 * ft_lpc_irq_set_type - Set the flow type of LPC irq.
 * @data: Pointer to the irq_data structure that identifies the irq.
 * @type: The irq type (IRQ_TYPE_LEVEL/etc.) to be set.
 */
static int ft_lpc_irq_set_type(struct irq_data *data, u32 type)
{
	if (type & IRQ_TYPE_LEVEL_MASK)
		irq_set_handler_locked(data, handle_level_irq);
	else if (type & IRQ_TYPE_EDGE_BOTH)
		irq_set_handler_locked(data, handle_edge_irq);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
/**
 * ft_lpc_irq_set_wake - Enable/disable power-management wake-on of LPC irq.
 * @data: Pointer to the irq_data structure that identifies the irq.
 * @enable: Enable or disable the irq power-management wake-on
 *
 * Enable or disable the power-management wake-on of LPC irq. According to
 * Phytium Spec, the LPC irq can only be masked as a whole, not bit mask.
 */
static int ft_lpc_irq_set_wake(struct irq_data *data, unsigned int enable)
{
	if (enable)
		lpc_ctx.wake_en = 0x3;
	else
		lpc_ctx.wake_en = 0x0;

	return 0;
}
#else
#define ft_lpc_irq_set_wake	NULL
#endif

/* LPC power-management wake-on */
static const struct irq_chip ft_lpc_irq_chip = {
	.name		= PHYTIUM_LPC_DRIVER_NAME,
	.irq_set_type	= ft_lpc_irq_set_type,
	.irq_set_wake	= ft_lpc_irq_set_wake,
};

/**
 * phytium_lpc_irq_handler - Handle the LPC irq.
 * @desc: Pointer to the irq_desc structure that identifies the irq chip.
 *
 * Invoke the handler for a particular irq number read from LPC irq status
 * register.
 */
static void phytium_lpc_irq_handler(struct irq_desc *desc)
{
	struct phytium_lpc *lpc = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned long sts;
	irq_hw_number_t hwirq;

	chained_irq_enter(chip, desc);

	/* read irq status */
	sts = irq_reg_readl(lpc->gc, LPC_INT_STATE);
	lpc_dbg("Interrupt %d, with irq status: %#lx\n", lpc->irq, sts);

	/* clear lpc irq */
	irq_reg_writel(lpc->gc, 0, LPC_CLR_IRQ);

	for_each_set_bit(hwirq, &sts, lpc->irq_num) {
		int sirq = irq_find_mapping(lpc->domain, hwirq);

		generic_handle_irq(sirq);
	}

	chained_irq_exit(chip, desc);
}

/**
 * phytium_lpc_irq_find_mapping- Find a linux irq from a LPC irq number.
 * @offset: Hardware irq number in LPC domain sapce.
 */
int phytium_lpc_irq_find_mapping(u32 offset)
{
	if (!ft_lpc || !ft_lpc->domain)
		return 0;

	return irq_find_mapping(ft_lpc->domain, offset);
}

/**
 * phytium_lpc_configure_irqs - Configure the LPC irq.
 * @ft_lpc: Pointer to the phytium_lpc structure that represent the LPC.
 * @pdev: Pointer to the platform device of LPC.
 *
 * Configure the LPC irq according to Phytium Spec.
 */
static int phytium_lpc_configure_irqs(struct phytium_lpc *ft_lpc,
				     struct platform_device *pdev)
{

	struct irq_chip_generic *gc = NULL;
	irq_hw_number_t hwirq;
	int ret;
	u32 sirq_config;

	/*
	 * NU_SERIRQ_CONFIG:
	 * bit 3~4: 2b01: Max serial irq number is 32, else 16.
	 */
	sirq_config = (readb(ft_lpc->regs + LPC_SERIRQ_CONFIG) >> 3) & 0x3;
	if (sirq_config == 0x1)
		ft_lpc->irq_num = 32;
	else
		ft_lpc->irq_num = 16;

	ft_lpc->domain = irq_domain_create_linear(dev_fwnode(&pdev->dev),
						  ft_lpc->irq_num,
						  &irq_generic_chip_ops,
						  ft_lpc);
	if (!ft_lpc->domain) {
		dev_err(&pdev->dev, "Cannot add Phytium irq domain.\n");
		return -EINVAL;
	}

	ret = irq_alloc_domain_generic_chips(ft_lpc->domain, ft_lpc->irq_num, 1,
					     PHYTIUM_LPC_DRIVER_NAME,
					     handle_level_irq, 0,
					     IRQ_TYPE_DEFAULT, 0);
	if (ret) {
		dev_err(&pdev->dev, "Unable to register irq generic chip\n");
		goto out_free_domain;
	}

	gc = irq_get_domain_generic_chip(ft_lpc->domain, 0);
	if (!gc) {
		dev_err(&pdev->dev, "Unable to get domain irq generic chip.\n");
		ret = -ENODEV;
		goto out_free_domain;
	}

	gc->private = ft_lpc;
	gc->reg_base = ft_lpc->regs;
	gc->domain = ft_lpc->domain;
	gc->chip_types->chip = ft_lpc_irq_chip;

	ft_lpc->gc = gc;

	irq_set_chained_handler_and_data(ft_lpc->irq,
					 phytium_lpc_irq_handler, ft_lpc);

	/* Map a LPC interrupt into linux irq space */
	for (hwirq = 0; hwirq < ft_lpc->irq_num; hwirq++)
		irq_create_mapping(ft_lpc->domain, hwirq);

	return 0;

out_free_domain:
	irq_domain_remove(ft_lpc->domain);
	ft_lpc->domain = NULL;

	return ret;
}

static int phytium_lpc_probe(struct platform_device *pdev)
{
	struct resource *mem = NULL;
	const char *lpc_sync = SYNC_PROPERTY_LONG;
	int err;

	ft_lpc = devm_kzalloc(&pdev->dev, sizeof(*ft_lpc), GFP_KERNEL);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		pr_info("Phytium lpc get resource failed.\n");
		return -ENOMEM;
	}

	lpc_base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(lpc_base)) {
		pr_err("Phytium lpc ioremap base failed.\n");
		err = -ENOMEM;
		goto err_resource;
	}

	ft_lpc->regs = ioremap(mem->start + LPC_IO_OFFSET, 0xFFFD);
	if (IS_ERR(ft_lpc->regs)) {
		pr_err("Phytium lpc ioremap regs failed.\n");
		err = -ENOMEM;
		goto err_ioremap;
	}

	platform_set_drvdata(pdev, ft_lpc);
	ft_lpc->dev = &pdev->dev;
	ft_lpc->fwnode = dev_fwnode(&pdev->dev);

	ft_lpc->irq = platform_get_irq(pdev, 0);
	if (ft_lpc->irq < 0) {
		pr_err("Get Phytium lpc irq failed.\n");
		goto irq_failed;
	}

	lpc_irq = ft_lpc->irq;

	err = phytium_lpc_configure_irqs(ft_lpc, pdev);
	if (err)
		dev_err(&pdev->dev, "Configure Phytium lpc irq failed %d\n", err);

	if (!is_cpu_ft2000pc())
		phytium_lpc_probe_dt(pdev);

	/* The 'Synchronize' property should be declared in ACPI or device-tree. */
	if (!device_property_read_string(&pdev->dev, "Synchronize", &lpc_sync))
		if (!strcmp(lpc_sync, SYNC_PROPERTY_SHORT))
			sync_cycle = SYNC_CYCLE_SHORT;

	/* Whether the Phytium EC acpi id present */
	if (acpi_dev_found("FTEC0001") || acpi_dev_found("PHYT000B")) {
		/*
		 * Disable EC acpi irq.
		 * The LPC won't work normally, when enabled the ec apci
		 * without the ec driver to handle the irq.
		 * It enabled in the ec_sci event driver.
		 */
		ft_lpc_write(0x87, 0x66);
	}

	/*
	 * Disable i8042 kbd/aux port in some phytium machines such as 706,
	 * enable it when i8042 present and i8042 driver probe.
	 */
	if (acpi_dev_found("KBCI8042") &&
			dmi_check_system(phytium_machine_table))
		ftlpc_disable_i8042_interfaces();

	pr_info("phytium lpc probe\n");

	return 0;

irq_failed:
	iounmap(ft_lpc->regs);
	ft_lpc->regs = NULL;

err_ioremap:
	if (lpc_base) {
		iounmap(lpc_base);
		lpc_base = NULL;
	}

err_resource:
	kfree(ft_lpc);

	return err;
}

static int phytium_lpc_remove(struct platform_device *pdev)
{
	u32 offset;
	int irq;

	iounmap(lpc_base);
	lpc_base = NULL;

	iounmap(ft_lpc->regs);
	ft_lpc->regs = NULL;

	if (ft_lpc->domain) {
		for (offset = 0; offset < ft_lpc->irq_num; offset++) {
			irq = irq_find_mapping(ft_lpc->domain, offset);
			irq_dispose_mapping(irq);
		}

		irq_destroy_generic_chip(ft_lpc->gc, 0, 0, 0);
		irq_domain_remove(ft_lpc->domain);
	}

	platform_set_drvdata(pdev, NULL);
	kfree(ft_lpc);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int phytium_lpc_suspend(struct device *dev)
{
	if (ft_lpc && ft_lpc->regs) {
		/* Read the LPC context */
		lpc_ctx.int_mask = readb(ft_lpc->regs + LPC_INT_MASK);
		lpc_ctx.sirq_cfg = readl(ft_lpc->regs + LPC_SERIRQ_CONFIG);

		writeb(~lpc_ctx.wake_en & 0x3, ft_lpc->regs + LPC_INT_MASK);
	}

	return 0;
}

static int phytium_lpc_resume(struct device *dev)
{
	if (ft_lpc && ft_lpc->regs) {
		/* Restore the LPC context */
		writeb(lpc_ctx.int_mask, ft_lpc->regs + LPC_INT_MASK);
		writel(lpc_ctx.sirq_cfg, ft_lpc->regs + LPC_SERIRQ_CONFIG);
		lpc_ctx.int_state = readl(ft_lpc->regs + LPC_INT_STATE);
	}

	lpc_dbg("phytium LPC resume int mask: %X, mode: %X, status: %X\n",
 		lpc_ctx.int_mask, lpc_ctx.sirq_cfg, lpc_ctx.int_state);

	return 0;
}

static SIMPLE_DEV_PM_OPS(phytium_lpc_pm_ops,
			 phytium_lpc_suspend,
			 phytium_lpc_resume);
#endif

#ifdef CONFIG_OF
static const struct of_device_id phytium_lpc_of_match[] = {
	{ .compatible = "phytium,lpc", },
	{},
};
MODULE_DEVICE_TABLE(of, phytium_lpc_of_match);
#endif

static const struct acpi_device_id lpc_acpi_match[] = {
	{ "PHYT0007", 0 },
	{ "LPC0001", 0 },
	{}
};
MODULE_DEVICE_TABLE(acpi, lpc_acpi_match);

static struct platform_driver phytium_lpc_driver = {
	.probe = phytium_lpc_probe,
	.remove = phytium_lpc_remove,
	.driver = {
		.name = PHYTIUM_LPC_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(phytium_lpc_of_match),
		.acpi_match_table = ACPI_PTR(lpc_acpi_match),
#ifdef CONFIG_PM_SLEEP
		.pm = &phytium_lpc_pm_ops,
#endif
	}
};

static int __init phytium_lpc_init(void)
{
	return platform_driver_register(&phytium_lpc_driver);
}
subsys_initcall(phytium_lpc_init);

MODULE_DESCRIPTION("Phytium Platform Spectic lpc Driver");
MODULE_LICENSE("GPL");
