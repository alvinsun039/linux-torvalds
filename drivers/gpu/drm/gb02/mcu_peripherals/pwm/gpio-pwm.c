// SPDX-License-Identifier: GPL-2.0-only
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/pwm.h>
#include <linux/timekeeping.h>
#include "gb_pwm.h"
#include "mcu_peripherals/gpio/pinctrl-gb02-mcu.h"

//#define GPIO_PWM_DEBUG
#ifdef GPIO_PWM_DEBUG
#define GB02MAC2036(arg...) gb_printf(KERN_INFO, "[" PLATFORM_GPIO_PWM_DEVICE_NAME "] " arg)
#else
#define GB02MAC2036(arg...)
#endif

static inline struct GB02STR174 *GB02FUNC1426(struct GB02STR172 *c)
{
	return container_of(c, struct GB02STR174, chip);
}

static void GB02FUNC1427(struct GB02STR174 *GB02STR174)
{
	GB02FUNC312(GB02STR174->gpio, 0);
}

static void GB02FUNC1428(struct GB02STR174 *GB02STR174)
{
	GB02FUNC312(GB02STR174->gpio, 1);
}

static int GB02FUNC1429(struct GB02STR172 *chip, struct GB02STR169 *pwm)
{
	struct GB02STR174 *GB02STR174 = GB02FUNC1426(chip);

	if (GB02STR174->off_time)
		hrtimer_start(&GB02STR174->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
	else
		GB02FUNC1428(GB02STR174);

	return 0;
}

static void GB02FUNC1432(struct GB02STR172 *chip, struct GB02STR169 *pwm)
{
	struct GB02STR174 *GB02STR174 = GB02FUNC1426(chip);

	if (!GB02STR174->off_time)
		GB02FUNC1427(GB02STR174);
}

static enum hrtimer_restart GB02FUNC1434(struct hrtimer *timer)
{
	struct GB02STR174 *GB02STR174 = container_of(timer,
					struct GB02STR174, timer);

	if (!GB02STR174->pin_on) {
		hrtimer_forward_now(&GB02STR174->timer, ns_to_ktime(GB02STR174->on_time));

		if (GB02STR174->on_time) {
			GB02MAC2036("--%lld us--", ktime_to_ns(ktime_get()));
			GB02FUNC1428(GB02STR174);
			GB02STR174->pin_on = true;
		}

	} else {
		hrtimer_forward_now(&GB02STR174->timer, ns_to_ktime(GB02STR174->off_time));

		if (GB02STR174->off_time) {
			GB02MAC2036("***%lld us***", ktime_to_ns(ktime_get()));
			GB02FUNC1427(GB02STR174);
			GB02STR174->pin_on = false;
		}
	}

	return HRTIMER_RESTART;
}

static int GB02FUNC1436(struct GB02STR172 *chip, struct GB02STR169 *pwm,
						   int duty_ns, int period_ns)
{
	struct GB02STR174 *GB02STR174 = GB02FUNC1426(chip);

	GB02STR174->on_time = duty_ns;
	GB02STR174->off_time = period_ns - duty_ns;
	GB02MAC2036("config duty:%d, period:%d\n", duty_ns, period_ns);
	return 0;
}

static int GB02FUNC1438(struct GB02STR172 *chip, struct GB02STR169 *pwm,
				struct GB02STR168 *state)
{
	bool enabled;
	int ret;

	enabled = pwm->state.enable;

	if (enabled && !state->enable) {
		GB02FUNC1432(chip, pwm);
		return 0;
	}

	ret = GB02FUNC1436(chip, pwm,
				   state->duty_cycle, state->period);
	if (ret)
		return ret;

	if (!enabled && state->enable)
		ret = GB02FUNC1429(chip, pwm);

	return ret;
}

static struct GB02STR171 gpio_pwm_ops = {
	.config = GB02FUNC1436,
	.enable = GB02FUNC1429,
	.disable = GB02FUNC1432,
	.apply = GB02FUNC1438,
};

static int GB02FUNC1441(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info, int device_id)
{
	int ret, i;
	struct GB02STR174 *GB02STR174;
	struct GB02STR169 *pwm;

	GB02STR174 = devm_kzalloc(gb_dev->dev, sizeof(*GB02STR174), GFP_KERNEL);
	if (!GB02STR174)
		return -ENOMEM;

	GB02STR174->chip.dev = gb_dev->dev;
	GB02STR174->chip.ops = &gpio_pwm_ops;
	GB02STR174->chip.npwm = 1;
	GB02STR174->gpio = peri_info->gpio_num;
	GB02STR174->chip.pwm_type = GB_GPIO_PWM;
	GB02STR174->chip.driver_data = GB02STR174;
	ret = GB02FUNC295(GB02STR174->gpio);
	if (ret) {
		GB02MAC2036("gpio%u request failed\n", GB02STR174->gpio);
		return ret;
	}
	ret = GB02FUNC301(GB02STR174->gpio, GB02MAC542);
	if (ret) {
		GB02MAC2036("gpio%u set_direction failed\n", GB02STR174->gpio);
		return ret;
	}
	hrtimer_init(&GB02STR174->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	GB02STR174->timer.function = &GB02FUNC1434;
	GB02STR174->pin_on = false;

	GB02STR174->chip.pwms = kzalloc(sizeof(struct GB02STR169) * GB02STR174->chip.npwm, GFP_KERNEL);
	if (!GB02STR174->chip.pwms)
		return -ENOMEM;

	for (i = 0; i < GB02STR174->chip.npwm; i++) {
		pwm = &GB02STR174->chip.pwms[i];
		pwm->chip = &GB02STR174->chip;
		pwm->hwpwm = i;
		pwm->state.polarity = PWM_POLARITY_NORMAL;
	}

	if (!hrtimer_is_hres_active(&GB02STR174->timer))
		dev_warn(gb_dev->dev, "HR timer unavailable, restricting to low resolution\n");


	peri_info->priv = &GB02STR174->chip;
	return 0;
}

static void GB02FUNC1445(struct GB02STR72 *peri_info)
{
	struct GB02STR172 *GB02STR172 = (struct GB02STR172 *)peri_info->priv;
	struct GB02STR174 *GB02STR174 = (struct GB02STR174 *)GB02STR172->driver_data;

	GB02FUNC300(peri_info->gpio_num);
	hrtimer_cancel(&GB02STR174->timer);

	kfree(GB02STR172->pwms);
	GB02STR172->pwms = NULL;
}

int GB02FUNC1415(struct GB02STR72 *peri_info)
{
	unsigned gpio = peri_info->gpio_num;
	int ret = -1;

	ret = GB02FUNC295(gpio);
	if (ret)
		GB02MAC2036("gpio%u request failed\n", gpio);
	ret = GB02FUNC301(gpio, GB02MAC542);
	if (ret)
		GB02MAC2036("gpio%u set_direction failed\n", gpio);
	return ret;
}

int GB02FUNC1412(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	int ret;
	int device_id = 0;

	device_id = peri_info->mcu_peripherals_device_id;
	ret = GB02FUNC1441(gb_dev, peri_info, device_id);

	return ret;
}

void GB02FUNC1414(struct GB02STR72 *peri_info)
{
	GB02FUNC1445(peri_info);
}
