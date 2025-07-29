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
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/gpio.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/thermal.h>
#include "../gb_pwm.h"
#include "mcu_peripherals/gpio/pinctrl-gb02-mcu.h"

#define GB02MAC1917 30000
#define GB02MAC81 5
//PERIOD use ns
#define GB02MAC1919 30000
#define GB02MAC1921
#define GB02MAC1923 41
//#define FAN_DEBUG
#ifdef FAN_DEBUG
#define GB02MAC85(arg...) gb_printf(KERN_INFO, "[" GB02_PWM_FAN_DEV_NAME "] " arg)
#else
#define GB02MAC85(arg...)
#endif

struct GB02STR157 {
	struct mutex lock;
	struct GB02STR169 *pwm;
	unsigned int pwm_value;
	unsigned int pwm_fan_state;
	unsigned int pwm_fan_max_state;
	unsigned int *pwm_fan_cooling_levels;
	void __iomem *sysctl_iofunc_base; // bar0
	atomic_t pulses;
	unsigned int rpm;
	ktime_t sample_start;
	u8 pulses_per_revolution;
	struct timer_list rpm_timer;
	bool user_mode;
	int irq;
	struct thermal_cooling_device *cdev;
};
static struct GB02STR157 *g_ctx;

static int GB02FUNC1221(struct GB02STR157 *ctx, unsigned long pwm)
{
	unsigned long	 period;
	int				 ret   = 0;
	struct GB02STR168 state = {};

	GB02MAC85("pwm:%ld\n", pwm);
	mutex_lock(&ctx->lock);
	if (ctx->pwm_value == pwm)
		goto exit_set_pwm_err;

	period	= ctx->pwm->period;
	state.duty_cycle = DIV_ROUND_UP(pwm * (period - 1), GB02MAC1917);
	state.period	 = GB02MAC1919;
	state.enable	 = pwm ? true : false;

	ret = GB02FUNC1380(ctx->pwm, &state);
	if (!ret)
		ctx->pwm_value = pwm;
exit_set_pwm_err:
	mutex_unlock(&ctx->lock);
	return ret;
}

static void GB02FUNC1224(struct GB02STR157 *ctx, unsigned long pwm)
{
	int i;

	for (i = 0; i < ctx->pwm_fan_max_state; ++i) {
		if (pwm < ctx->pwm_fan_cooling_levels[i + 1])
			break;
	}

	ctx->pwm_fan_state = i;
}

static ssize_t GB02FUNC37(struct device *dev, struct device_attribute *attr, const char *buf,
					   size_t count)
{
	struct GB02STR157 *ctx = dev_get_drvdata(dev);
	unsigned long		level;
	int					ret;

	if (kstrtoul(buf, 10, &level) || level > GB02MAC81)
		return -EINVAL;

	if (level > 0) {
		ret = GB02FUNC1221(ctx, ctx->pwm_fan_cooling_levels[level-1]);
		if (ret)
			return ret;
		ctx->user_mode = true;
	} else {
		ctx->user_mode = false;
	}

	GB02FUNC1224(ctx, ctx->pwm_fan_cooling_levels[level-1]);
	return count;
}

static ssize_t GB02FUNC41(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct GB02STR157 *ctx = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ctx->pwm_fan_state + 1);
}

static ssize_t GB02FUNC1229(struct device *dev, struct device_attribute *attr, const char *buf,
					   size_t count)
{
	struct GB02STR157 *ctx = dev_get_drvdata(dev);
	unsigned long		pwm;
	int					ret;

	if (kstrtoul(buf, 10, &pwm) || pwm > GB02MAC1917)
		return -EINVAL;

	if (pwm > 0) {
		ret = GB02FUNC1221(ctx, pwm);
		if (ret)
			return ret;
		ctx->user_mode = true;
	} else {
		ctx->user_mode = false;
	}

	GB02FUNC1224(ctx, pwm);
	return count;
}

static ssize_t GB02FUNC1231(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct GB02STR157 *ctx = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ctx->pwm_value);
}

static ssize_t GB02FUNC44(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct GB02STR157 *ctx = dev_get_drvdata(dev);
#ifdef GB02MAC1921
	ctx->rpm = GB02FUNC530(ctx->sysctl_iofunc_base + 0x1000000, 0x0003FC00);
#endif
	return sprintf(buf, "%u\n", ctx->rpm);
}

static SENSOR_DEVICE_ATTR(fan1_level, 0644, GB02FUNC41, GB02FUNC37, 0);
static SENSOR_DEVICE_ATTR(pwm1, 0644, GB02FUNC1231, GB02FUNC1229, 0);
static SENSOR_DEVICE_ATTR(fan1_input, 0444, GB02FUNC44, NULL, 0);

static struct attribute *pwm_fan_attrs[] = {
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan1_level.dev_attr.attr,
	NULL,
};

ATTRIBUTE_GROUPS(pwm_fan);

/* thermal cooling device callbacks */
static int GB02FUNC1233(struct thermal_cooling_device *cdev, unsigned long *state)
{
	struct GB02STR157 *ctx = cdev->devdata;

	if (!ctx)
		return -EINVAL;

	*state = ctx->pwm_fan_max_state;

	return 0;
}

static int GB02FUNC1235(struct thermal_cooling_device *cdev, unsigned long *state)
{
	struct GB02STR157 *ctx = cdev->devdata;

	if (!ctx)
		return -EINVAL;

	*state = ctx->pwm_fan_state;

	return 0;
}

static int GB02FUNC1239(struct thermal_cooling_device *cdev, unsigned long state)
{
	struct GB02STR157 *ctx = cdev->devdata;
	int					ret;

	if (!ctx || (state > ctx->pwm_fan_max_state))
		return -EINVAL;

	if (state == ctx->pwm_fan_state)
		return 0;

	if (ctx->user_mode)
		return 0;
	ret = GB02FUNC1221(ctx, ctx->pwm_fan_cooling_levels[state]);
	if (ret) {
		dev_err(&cdev->device, "Cannot set pwm!\n");
		return ret;
	}

	ctx->pwm_fan_state = state;

	return ret;
}

static const struct thermal_cooling_device_ops pwm_fan_cooling_ops = {
	.get_max_state = GB02FUNC1233,
	.get_cur_state = GB02FUNC1235,
	.set_cur_state = GB02FUNC1239,
};

static int GB02FUNC63(struct device *dev, struct GB02STR157 *ctx)
{
	int num = GB02MAC81, i;

	ctx->pwm_fan_cooling_levels = devm_kcalloc(dev, GB02MAC81, sizeof(u32), GFP_KERNEL);
	if (!ctx->pwm_fan_cooling_levels)
		return -ENOMEM;
	for (i = 0; i < GB02MAC81; i++) {
		ctx->pwm_fan_cooling_levels[i] = 6000 * (i + 1);
		if (ctx->pwm_fan_cooling_levels[i] > GB02MAC1917) {
			dev_err(dev, "PWM fan state[%d]:%d > %d\n", i, ctx->pwm_fan_cooling_levels[i], GB02MAC1917);
			return -EINVAL;
		}
	}

	ctx->pwm_fan_max_state = num - 1;

	return 0;
}

#ifndef GB02MAC1921
static void GB02FUNC1247(struct timer_list *t)
{
	struct GB02STR157 *ctx = from_timer(ctx, t, rpm_timer);
	int pulses;
	u64 tmp;

	pulses = atomic_read(&ctx->pulses);
	atomic_sub(pulses, &ctx->pulses);
	tmp = (u64)pulses * ktime_ms_delta(ktime_get(), ctx->sample_start) * 60;
	do_div(tmp, ctx->pulses_per_revolution * 1000);
	if (tmp == 0)
		tmp = GB02FUNC530(ctx->sysctl_iofunc_base + 0x1000000, 0x0003FC00);
	ctx->rpm = tmp;

	ctx->sample_start = ktime_get();
	mod_timer(&ctx->rpm_timer, jiffies + HZ);
}

static irqreturn_t GB02FUNC1249(int irq, void *data)
{
	struct GB02STR157 *ctx = data;

	atomic_inc(&ctx->pulses);

	return IRQ_HANDLED;

}
#endif

int GB02FUNC1253(struct GB02STR39 *gbdev,
		struct GB02STR72 *peri_info, struct GB02STR74 *pwm_dev)
{
	struct thermal_cooling_device *cdev;
	struct GB02STR157 *ctx;
	struct device *hwmon;
	int ret;
	struct GB02STR168 state = {};
	struct GB02STR172 *gb02_fan_pwm;

	gb02_fan_pwm = (struct GB02STR172 *)peri_info->priv;
	ctx = devm_kzalloc(gbdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mutex_init(&ctx->lock);

	ctx->pwm = (struct GB02STR169 *)&gb02_fan_pwm->pwms[GB02MAC2035];
	if (IS_ERR(ctx->pwm)) {
		ret = PTR_ERR(ctx->pwm);
		if (ret != -EPROBE_DEFER)
			dev_err(gbdev->dev, "Could not get PWM: %d\n", ret);
		return ret;
	}

	ctx->pwm->period = GB02MAC1919;
	ctx->pwm_value = GB02MAC1917;
	ctx->sysctl_iofunc_base = gbdev->gb_pcie->pci_bars[0].mmio; // bar0
	/* Set duty cycle to maximum allowed and enable PWM output */
	state.duty_cycle = GB02MAC1919 - 1;
	state.enable	 = true;
	state.period	 = GB02MAC1919;
	ret = GB02FUNC1380(ctx->pwm, &state);
	if (!ctx->pwm)
		pr_err("%s pwm is NULL\n", __func__);
	if (ret)
		return ret;

	hwmon = devm_hwmon_device_register_with_groups(gbdev->dev, "pwmfan", ctx, pwm_fan_groups);
	if (IS_ERR(hwmon)) {
		dev_err(gbdev->dev, "Failed to register hwmon device\n");
		ret = PTR_ERR(hwmon);
		goto err_pwm_disable;
	}

	ret = GB02FUNC63(gbdev->dev, ctx);
	if (ret)
		goto err_pwm_disable;

	ctx->pwm_fan_state = ctx->pwm_fan_max_state;
	ctx->pulses_per_revolution = 2;

#ifndef GB02MAC1921
	ret = GB02FUNC295(GB02MAC1923);
	if (ret) {
		dev_err(gbdev->dev, "Failed to request  gpio\n");
		return ret;
	}
	ret = GB02FUNC301(GB02MAC1923, GB02MAC541);
	if (ret) {
		dev_err(gbdev->dev, "Failed to set  gpio direction\n");
		return ret;
	}

	GB02FUNC292(GB02MAC1923, true);
	//register irq
	ctx->irq = pci_irq_vector(gbdev->gb_pcie->pdev, 6);
	if (ctx->irq > 0) {
		ret = devm_request_threaded_irq(gbdev->dev, ctx->irq, NULL, GB02FUNC1249, IRQF_SHARED | IRQF_TRIGGER_RISING,
				       "fan_fg", ctx);
		if (ret) {
			dev_err(gbdev->dev, "Failed to request interrupt: %d\n", ret);
			return ret;
		}
		ctx->sample_start = ktime_get();
		timer_setup(&ctx->rpm_timer, GB02FUNC1247, 0);
		mod_timer(&ctx->rpm_timer, jiffies + HZ);
	}
#endif

	if (IS_ENABLED(CONFIG_THERMAL)) {
		cdev = thermal_of_cooling_device_register(gbdev->dev->of_node, "pwm-fan", ctx,
				&pwm_fan_cooling_ops);
		if (IS_ERR(cdev)) {
			dev_err(gbdev->dev, "Failed to register pwm-fan as cooling device");
			ret = PTR_ERR(cdev);
			goto err_pwm_disable;
		}
		ctx->cdev = cdev;
	}

	g_ctx = ctx;

	return 0;

err_pwm_disable:
	state.enable = false;
	GB02FUNC1380(ctx->pwm, &state);

	return ret;
}

void GB02FUNC1262(struct GB02STR72 *peri_info)
{
	struct GB02STR157 *ctx = g_ctx;

	thermal_cooling_device_unregister(ctx->cdev);
	if (ctx->pwm_value)
		GB02FUNC1383(ctx->pwm);
	del_timer_sync(&ctx->rpm_timer);
}

int GB02FUNC1264(struct GB02STR72 *peri_info)
{
	struct GB02STR157 *ctx;
	int ret;

	ctx = g_ctx;

	ret = GB02FUNC1221(ctx, ctx->pwm_fan_cooling_levels[0]);
	if (ret)
		return ret;
	ctx->pwm_fan_state = 0;

	return 0;
}

int GB02FUNC1268(struct GB02STR39 *gbdev, struct GB02STR72 *peri_info)
{
	struct GB02STR157 *ctx;
	int ret;

	ctx = g_ctx;

	if (ctx->pwm_value == 0)
		return 0;
#ifndef GB02MAC1921
	//restore gpio status
	ret = GB02FUNC295(GB02MAC1923);
	if (ret) {
		dev_err(gbdev->dev, "Failed to request  gpio\n");
		return ret;
	}
	ret = GB02FUNC301(GB02MAC1923, GB02MAC541);
	if (ret) {
		dev_err(gbdev->dev, "Failed to set  gpio direction\n");
		return ret;
	}

	GB02FUNC292(GB02MAC1923, true);
#endif

	return ret;
}
