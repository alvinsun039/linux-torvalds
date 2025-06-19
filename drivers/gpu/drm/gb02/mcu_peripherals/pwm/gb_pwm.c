/* SPDX-License-Identifier: GPL-2.0 */

#include "gb_pwm.h"

int GB02FUNC1379(struct GB02STR169 *pwm, int duty_ns, int period_ns)
{
	int ret = 0;
	struct GB02STR172 *chip = pwm->chip;

	if (chip->ops->config) {
		ret = chip->ops->config(chip, pwm, duty_ns, period_ns);
		if (ret) {
			pr_err("Failed to configure PWM\n");
			return ret;
		}
	}

	return ret;	
}

int GB02FUNC1380(struct GB02STR169 *pwm, struct GB02STR168 *state)
{
	int ret = 0;
	struct GB02STR172 *chip = pwm->chip;

	if (chip->ops->apply) {
		ret = chip->ops->apply(chip, pwm, state);
		if (ret) {
			pr_err("Failed to configure PWM\n");
			return ret;
		}
	}

	return ret;
}

int GB02FUNC1382(struct GB02STR169 *pwm)
{
	int ret = 0;
	struct GB02STR172 *chip = pwm->chip;

	if (chip->ops->enable) {
		ret = chip->ops->enable(chip, pwm);
		if (ret) {
			pr_err("Failed to enable PWM\n");
			return ret;
		}
	}

	return ret;
}

void GB02FUNC1383(struct GB02STR169 *pwm)
{
	struct GB02STR172 *chip = pwm->chip;

	if (chip->ops->disable)
		chip->ops->disable(chip, pwm);

	return;
}

