// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Lenovo machine EC driver provides access to nodes for touchpad,
 * performance, backlight, and other addresses.
 *
 * Copyright (C) 2022 Hiwa Chen <chenhaihua@kylinos.cn>
 * Copyright (C) 2023 Ai Chao <aichao@kylinos.cn>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>

#define MODE			0x20
#define BACKLIGHT		0xb9
#define SYSTEM_IDENTY	0xa3
#define KBLIGHT			0x7a
#define TOUCHPAD		0x42

#define BACKLIGHT_DEF		70

static struct platform_device *lenovo_ec_dev;
static struct backlight_device *lenovo_backlight_dev;

static void permit_adjust_backlight(void)
{
	u8 val;

	ec_read(SYSTEM_IDENTY, &val);
	val &= ~0x2;
	val |= 0x2;
	ec_write(SYSTEM_IDENTY, val);
}

static ssize_t mode_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	u8 mode;

	ec_read(MODE, &mode);

	return sprintf(buf, "%u\n", mode);
}
DEVICE_ATTR_RO(mode);

static ssize_t touchpad_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	u8 mode;

	ec_read(TOUCHPAD, &mode);

	return sprintf(buf, "%u\n", mode);
}
DEVICE_ATTR_RO(touchpad);

static ssize_t kblight_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	u8 mode;

	ec_read(KBLIGHT, &mode);

	return sprintf(buf, "%u\n", mode);
}
DEVICE_ATTR_RO(kblight);

static int lenovo_ec_backlight_show(struct backlight_device *blightdev)
{
	u8 brightness;

	if (ec_read(BACKLIGHT, &brightness)) {
		pr_err("get brightness err!\n");
		return -EIO;
	}

	return brightness;
}

static int lenovo_ec_backlight_store(struct backlight_device *blightdev)
{
	if (!blightdev)
		return -EINVAL;

	if (blightdev->props.brightness < 0)
		blightdev->props.brightness = 0;
	else if (blightdev->props.brightness >
			blightdev->props.max_brightness)
		blightdev->props.brightness = blightdev->props.max_brightness;

	if (ec_write(BACKLIGHT, blightdev->props.brightness)) {
		pr_err("set brightness err!\n");
		return -EIO;
	}

	return 0;
}

static const struct backlight_ops lenovo_ec_backlight_ops = {
	.get_brightness = lenovo_ec_backlight_show,
	.update_status = lenovo_ec_backlight_store,
};

static struct attribute *lenovo_ec_attr[] = {
	&dev_attr_mode.attr,
	&dev_attr_touchpad.attr,
	&dev_attr_kblight.attr,
	NULL,
};

static const struct attribute_group lenovo_ec_attr_group = {
	.attrs = lenovo_ec_attr,
};

static void lenovo_ec_enable_f11_hotkey(void)
{
	u8 val;

	ec_read(SYSTEM_IDENTY, &val);
	val &= ~0x1;
	val |= 0x1;
	ec_write(SYSTEM_IDENTY, val);
}

static const struct dmi_system_id lenovo_ec_setting_table[] = {
	{
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "LENOVO"),
			DMI_MATCH(DMI_BOARD_NAME, "LXKT-ZXEG-N6"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "LENOVO"),
			DMI_MATCH(DMI_BOARD_NAME, "LXKT-ZXE-N70"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "KaiTian"),
			DMI_MATCH(DMI_BOARD_NAME, "LXKT-ZXEG-N6"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "KaiTian"),
			DMI_MATCH(DMI_BOARD_NAME, "LXKT-ZXE-N70"),
		},
	},
};

static int lenovo_ec_probe(struct platform_device *pdev)
{
	u8 brightness;
	struct backlight_properties props;

	if (!dmi_check_system(lenovo_ec_setting_table))
		return 0;

	lenovo_ec_dev = platform_device_register_simple("lenovo_ec", -1, NULL, 0);
	if (IS_ERR(lenovo_ec_dev))
		return -ENODEV;

	if (sysfs_create_group(&lenovo_ec_dev->dev.kobj, &lenovo_ec_attr_group)) {
		pr_err("Creat sysfs ec failed\n");
		platform_device_unregister(lenovo_ec_dev);
		return -ENODEV;
	}

	permit_adjust_backlight();
	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_PLATFORM;
	props.max_brightness = 100;
	lenovo_backlight_dev = backlight_device_register("lenovo_backlight",
							 &lenovo_ec_dev->dev, NULL,
							 &lenovo_ec_backlight_ops, &props);

	brightness = lenovo_ec_backlight_show(lenovo_backlight_dev);
	if (brightness > BACKLIGHT_DEF)
		lenovo_backlight_dev->props.brightness = brightness;
	else
		lenovo_backlight_dev->props.brightness = BACKLIGHT_DEF;
	lenovo_ec_backlight_store(lenovo_backlight_dev);
	lenovo_ec_enable_f11_hotkey();

	return 0;
}

static int lenovo_ec_remove(struct platform_device *pdev)
{
	backlight_device_unregister(lenovo_backlight_dev);
	sysfs_remove_group(&lenovo_ec_dev->dev.kobj, &lenovo_ec_attr_group);
	platform_device_unregister(lenovo_ec_dev);
	return 0;
}

static const struct acpi_device_id lenovo_ec_ids[] = {
	{"PNP0C09", 0},
	{"", 0},
};

static struct platform_driver lenovo_ec_driver = {
	.probe = lenovo_ec_probe,
	.remove = lenovo_ec_remove,

	.driver = {
		.name = "lenovo-ec",
		.acpi_match_table = ACPI_PTR(lenovo_ec_ids),
	},
};
module_platform_driver(lenovo_ec_driver);

MODULE_AUTHOR("chenhaihua@kylinos.cn");
MODULE_LICENSE("GPL v2");
