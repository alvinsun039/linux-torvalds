// SPDX-License-Identifier: GPL-2.0
/*
 * Serial Port driver for Aspeed SIO UART device
 *
 * Author:
 * Hu Hai <huhai@kylinos.cn>
 *
 * Copyright (C) 2019-2024 KylinSoft Corporation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/serial.h>
#include <linux/kthread.h>
#include <linux/serial_8250.h>
#include <linux/platform_device.h>

struct aspeed_suart_data {
	struct uart_8250_port uart;
	int line;
};

struct uart_8250_port *serial8250_get_port(int line);

static unsigned int aspeed_suart_in(struct uart_port *p, int offset)
{
	u8 reg_ret;

	offset <<= p->regshift;
	reg_ret = readb(p->membase + offset);

	return reg_ret;
}

static void aspeed_suart_out(struct uart_port *p,
			int offset, int value)
{
	offset <<= p->regshift;
	writeb(value, p->membase + offset);
}

static int aspeed_logical_device_config(u64 suart_addr, bool io_cycle,
					u8 logical_device, u16 suart_port)
{
	unsigned char *__iomem suart_base = ioremap(suart_addr, 0x100);

	if (!suart_base)
		return -ENOMEM;

	if (io_cycle) {
		/* password */
		writeb(0xA5, suart_base + 0x2E);
		writeb(0xA5, suart_base + 0x2E);

		/* select logical device */
		writeb(0x07, suart_base + 0x2E);
		writeb(logical_device, suart_base + 0x2F);

		/* enabled device */
		writeb(0x30, suart_base + 0x2E);
		writeb(0x01, suart_base + 0x2F);

		/* set device addr */
		writeb(0x60, suart_base + 0x2E);
		writeb((suart_port >> 8) & 0xff,
				suart_base + 0x2F);

		writeb(0x61, suart_base + 0x2E);
		writeb(suart_port & 0xff,
				suart_base + 0x2F);

		/* exit */
		writeb(0xAA, suart_base + 0x2E);
	} else {
		/* password */
		writeb(0xA5, suart_base + 0x4E);
		writeb(0xA5, suart_base + 0x4E);

		/* select logical device */
		writeb(0x07, suart_base + 0x4E);
		writeb(logical_device, suart_base + 0x4F);

		/* enabled device */
		writeb(0x30, suart_base + 0x4E);
		writeb(0x01, suart_base + 0x4F);

		/* set device addr */
		writeb(0x60, suart_base + 0x4E);
		writeb((suart_port >> 8) & 0xff,
				suart_base + 0x4F);

		writeb(0x61, suart_base + 0x4E);
		writeb(suart_port & 0xff,
				suart_base + 0x4F);

		/* exit */
		writeb(0xAA, suart_base + 0x4E);
	}

	iounmap(suart_base);
	return 0;
}

static int aspeed_suart_config(struct platform_device *pdev,
			       u64 suart_addr, u16 suart_port)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	if (of_property_read_bool(np, "aspeed,suart1,2e2f"))
		ret = aspeed_logical_device_config(suart_addr, true, 0x2, suart_port);
	else if (of_property_read_bool(np, "aspeed,suart1,4e4f"))
		ret = aspeed_logical_device_config(suart_addr, false, 0x2, suart_port);
	else if (of_property_read_bool(np, "aspeed,suart2,2e2f"))
		ret = aspeed_logical_device_config(suart_addr, true, 0x3, suart_port);
	else if (of_property_read_bool(np, "aspeed,suart2,4e4f"))
		ret = aspeed_logical_device_config(suart_addr, false, 0x3, suart_port);

	return ret;
}

static int aspeed_suart_probe(struct platform_device *pdev)
{
	struct uart_8250_port *port;
	struct aspeed_suart_data *data;
	struct resource *res;
	void *__iomem sio_base;
	int ret = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "memory resource not found\n");
		return -EINVAL;
	}

	sio_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!sio_base) {
		dev_err(&pdev->dev, "devm_ioremap error\n");
		return -ENOMEM;
	}

	data = devm_kcalloc(&pdev->dev, 1,
			sizeof(struct aspeed_suart_data),
			GFP_KERNEL);
	if (!data) {
		dev_err(&pdev->dev, "Failed to alloc private mem struct.\n");
		return -ENOMEM;
	}

	ret = aspeed_suart_config(pdev, res->start & (~0x3FFLL), res->start & 0x3F8);
	if (ret) {
		dev_err(&pdev->dev, "ioremap error\n");
		return ret;
	}

	memset(&data->uart, 0, sizeof(data->uart));

	spin_lock_init(&data->uart.port.lock);
	data->uart.port.dev = &pdev->dev;
	data->uart.port.regshift = 0;
	data->uart.port.irq = 0;
	data->uart.port.iotype = UPIO_MEM;
	data->uart.port.type = PORT_16550A;
	data->uart.port.membase = sio_base;
	data->uart.port.mapbase = res->start;
	data->uart.port.uartclk = 1843200;
	data->uart.port.serial_in = aspeed_suart_in;
	data->uart.port.serial_out = aspeed_suart_out;
	data->uart.port.flags = UPF_FIXED_PORT | UPF_FIXED_TYPE | UPF_SKIP_TEST;

	data->line = serial8250_register_8250_port(&data->uart);
	if (data->line < 0) {
		dev_err(&pdev->dev,
			"unable to resigter 8250 port (MEM%llx): %d\n",
			(unsigned long long)res->start, ret);
		return data->line;
	}

	port = serial8250_get_port(data->line);
	dev_set_drvdata(&pdev->dev, data);

	return 0;
}

static int aspeed_suart_remove(struct platform_device *pdev)
{
	struct aspeed_suart_data *data = dev_get_drvdata(&pdev->dev);

	if (!data)
		return -ENOMEM;

	serial8250_unregister_port(data->line);

	return 0;
}

static const struct of_device_id aspeed_suart_table[] = {
	{ .compatible = "aspeed,ast2500-suart" },
	{ },
};
MODULE_DEVICE_TABLE(of, aspeed_suart_table);

static struct platform_driver aspeed_suart_driver = {
	.probe	= aspeed_suart_probe,
	.remove = aspeed_suart_remove,
	.driver = {
		.name = "aspeed-suart",
		.of_match_table = aspeed_suart_table,
	},
};

module_platform_driver(aspeed_suart_driver);

MODULE_DESCRIPTION("Driver for Aspeed SIO UART device");
MODULE_AUTHOR("Hu Hai <huhai@kylinos.cn>");
MODULE_LICENSE("GPL");
