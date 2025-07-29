// SPDX-License-Identifier: GPL-2.0
/*
 *  Hwmon driver for fans connected to PWM lines.
 *
 */

#include "common/gb-peripherals-common.h"
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/thermal.h>
#include "gpu/gb_device.h"

#define GB02MAC81 5
//#define FAN_DEBUG
#ifdef FAN_DEBUG
#define GB02MAC85(arg...) gb_printf(KERN_INFO, "[" GB02_PWM_FAN_DEV_NAME "] " arg)
#else
#define GB02MAC85(arg...)
#endif

struct GB02STR13 {
	struct mutex lock;
	unsigned int fan_state;
	unsigned int fan_max_state;
	unsigned int *fan_cooling_levels;
	void __iomem *sysctl_iofunc_base; // bar0
	unsigned int rpm;
	bool user_mode;
	unsigned int user_state;
	struct thermal_cooling_device *cdev;
};

static void GB02FUNC34(struct GB02STR13 *ctx, unsigned long pwm)
{
	int i;

	for (i = 0; i < ctx->fan_max_state; ++i) {
		if (pwm < ctx->fan_cooling_levels[i + 1])
			break;
	}

	ctx->fan_state = i;
}

static ssize_t GB02FUNC37(struct device *dev, struct device_attribute *attr, const char *buf,
					   size_t count)
{
	struct GB02STR13 *ctx = dev_get_drvdata(dev);
	unsigned long		level;

	if (kstrtoul(buf, 10, &level) || level > GB02MAC81)
		return -EINVAL;

	if (level > 0) {
		GB02FUNC528(level - 1, ctx->sysctl_iofunc_base + 0x1000000, 0x0003FC04);
		ctx->user_mode = true;
		ctx->user_state = level - 1;
	} else {
		ctx->user_mode = false;
		ctx->user_state = 0xf;
	}

	GB02FUNC34(ctx, ctx->fan_cooling_levels[level-1]);
	return count;
}

static ssize_t GB02FUNC41(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct GB02STR13 *ctx = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ctx->fan_state + 1);
}


static ssize_t GB02FUNC44(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct GB02STR13 *ctx = dev_get_drvdata(dev);

	ctx->rpm = GB02FUNC530(ctx->sysctl_iofunc_base + 0x1000000, 0x0003FC00);

	return sprintf(buf, "%u\n", ctx->rpm);
}

static SENSOR_DEVICE_ATTR(fan1_level, 0644, GB02FUNC41, GB02FUNC37, 0);
static SENSOR_DEVICE_ATTR(fan1_input, 0444, GB02FUNC44, NULL, 0);

static struct attribute *fan_attrs[] = {
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan1_level.dev_attr.attr,
	NULL,
};

ATTRIBUTE_GROUPS(fan);

/* thermal cooling device callbacks */
static int GB02FUNC49(struct thermal_cooling_device *cdev, unsigned long *state)
{
	struct GB02STR13 *ctx = cdev->devdata;

	if (!ctx)
		return -EINVAL;

	*state = ctx->fan_max_state;

	return 0;
}

static int GB02FUNC52(struct thermal_cooling_device *cdev, unsigned long *state)
{
	struct GB02STR13 *ctx = cdev->devdata;

	if (!ctx)
		return -EINVAL;

	*state = ctx->fan_state;

	return 0;
}

static int GB02FUNC55(struct thermal_cooling_device *cdev, unsigned long state)
{
	struct GB02STR13 *ctx = cdev->devdata;

	if (!ctx || (state > ctx->fan_max_state))
		return -EINVAL;

	if (state == ctx->fan_state)
		return 0;

	if (ctx->user_mode)
		return 0;

	GB02FUNC528(state, ctx->sysctl_iofunc_base + 0x1000000, 0x0003FC04);
	ctx->fan_state = state;

	return 0;
}

static const struct thermal_cooling_device_ops fan_cooling_ops = {
	.get_max_state = GB02FUNC49,
	.get_cur_state = GB02FUNC52,
	.set_cur_state = GB02FUNC55,
};

static int GB02FUNC63(struct device *dev, struct GB02STR13 *ctx)
{
	int num = GB02MAC81, i;

	ctx->fan_cooling_levels = devm_kcalloc(dev, GB02MAC81, sizeof(u32), GFP_KERNEL);
	if (!ctx->fan_cooling_levels)
		return -ENOMEM;
	for (i = 0; i < GB02MAC81; i++) {
		ctx->fan_cooling_levels[i] =  i + 1;
		if (ctx->fan_cooling_levels[i] > GB02MAC81) {
			dev_err(dev, "fan state[%d]:%d > %d\n", i, ctx->fan_cooling_levels[i], GB02MAC81);
			return -EINVAL;
		}
	}

	ctx->fan_max_state = num - 1;

	return 0;
}

int GB02FUNC69(struct GB02STR39 *gbdev, struct GB02STR72 *peri_info)
{
	struct thermal_cooling_device *cdev;
	struct GB02STR13 *ctx;
	struct device *hwmon;
	int ret;

	ctx = devm_kzalloc(gbdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mutex_init(&ctx->lock);

	ctx->sysctl_iofunc_base = gbdev->gb_pcie->pci_bars[0].mmio; // bar0

	hwmon = devm_hwmon_device_register_with_groups(gbdev->dev, "pwmfan", ctx, fan_groups);
	if (IS_ERR(hwmon)) {
		dev_err(gbdev->dev, "Failed to register hwmon device\n");
		ret = PTR_ERR(hwmon);
		return ret;
	}

	ret = GB02FUNC63(gbdev->dev, ctx);
	if (ret)
		return ret;

	ctx->fan_state = ctx->fan_max_state;

	if (IS_ENABLED(CONFIG_THERMAL)) {
		cdev = thermal_of_cooling_device_register(gbdev->dev->of_node, "pwm-fan", ctx,
				&fan_cooling_ops);
		if (IS_ERR(cdev)) {
			dev_err(gbdev->dev, "Failed to register pwm-fan as cooling device");
			ret = PTR_ERR(cdev);
			return ret;
		}
		ctx->cdev = cdev;
	}
	peri_info->priv = ctx;

	return ret;
}

void GB02FUNC79(struct GB02STR72 *peri_info)
{
	struct GB02STR13 *ctx = (struct GB02STR13 *)peri_info->priv;

	thermal_cooling_device_unregister(ctx->cdev);
}

int GB02FUNC82(struct GB02STR72 *peri_info)
{
	struct GB02STR13 *ctx = (struct GB02STR13 *)peri_info->priv;

	GB02FUNC528(0, ctx->sysctl_iofunc_base + 0x1000000, 0x0003FC04);
	ctx->fan_state = 0;

	return 0;
}

int GB02FUNC86(struct GB02STR72 *peri_info)
{
	struct GB02STR13 *ctx = (struct GB02STR13 *)peri_info->priv;

	if (ctx->user_mode)
		GB02FUNC528(ctx->user_state, ctx->sysctl_iofunc_base + 0x1000000, 0x0003FC04);

	return 0;
}
