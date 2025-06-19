// SPDX-License-Identifier: GPL-2.0-only
/*
 */

#include <linux/module.h>
#include "gb_governor.h"

static int GB02FUNC628(struct gb_devfreq *df,
				    unsigned long *freq)
{
	/*
	 * target callback should be able to get floor value as
	 * said in devfreq.h
	 */
	*freq = GB02MAC1056;
	return 0;
}

static int GB02FUNC630(struct gb_devfreq *devfreq,
				unsigned int event, void *data)
{
	int ret = 0;

	if (event == GB02MAC1044) {
		mutex_lock(&devfreq->lock);
		ret = GB02FUNC476(devfreq);
		mutex_unlock(&devfreq->lock);
	}

	return ret;
}

struct gb_devfreq_governor devfreq_performance = {
	.name = GB_DEVFREQ_GOV_PERFORMANCE,
	.get_target_freq = GB02FUNC628,
	.event_handler = GB02FUNC630,
};


