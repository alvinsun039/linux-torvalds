// SPDX-License-Identifier: GPL-2.0-only
/*
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/math64.h>
#include "gb_governor.h"

/* Default constants for DevFreq-Simple-Ondemand (DFSO) */
#define GB02MAC1063	(70)
#define GB02MAC1064	(20)

/* static inline bool devfreq_check_decreasing(unsigned long current)
{
	static unsigned long prev = 0;
	static unsigned long count = 0;

	if (current < prev) {
		count = 0;
		prev = current;
		return false;
	}

	prev = current;
	if (++count == 3) {
		count = 0;
		return true;
	}

	return false;
} */

static int GB02FUNC644(struct gb_devfreq *df,
					unsigned long *freq)
{
	int err;
	struct gb_devfreq_dev_status *stat;
	unsigned long long a, b;
	unsigned int dfso_upthreshold = GB02MAC1063;
	unsigned int dfso_downdifferential = GB02MAC1064;
	struct gb_devfreq_simple_ondemand_data *data = df->data;
	static unsigned long prev = 800;
	static unsigned long count = 0;

	err = GB02FUNC626(df);
	if (err)
		return err;

	stat = &df->last_status;

	if (data) {
		if (data->upthreshold)
			dfso_upthreshold = data->upthreshold;
		if (data->downdifferential)
			dfso_downdifferential = data->downdifferential;
	}
	if (dfso_upthreshold > 100 ||
	    dfso_upthreshold < dfso_downdifferential)
		return -EINVAL;

	/* Assume MAX if it is going to be divided by zero */
	if (stat->total_time == 0) {
		*freq = GB02MAC1056;
		return 0;
	}

	/* Prevent overflow */
	if (stat->busy_time >= (1 << 24) || stat->total_time >= (1 << 24)) {
		stat->busy_time >>= 7;
		stat->total_time >>= 7;
	}

	/* Set MAX if it's busy enough */
	if (stat->busy_time * 100 >
	    stat->total_time * dfso_upthreshold) {
		*freq = GB02MAC1056;
		return 0;
	}

	/* Set MAX if we do not know the initial frequency */
	if (stat->current_frequency == 0) {
		*freq = GB02MAC1056;
		return 0;
	}

	/* Keep the current frequency */
	if (stat->busy_time * 100 >
		stat->total_time * (dfso_upthreshold - dfso_downdifferential)) {
		*freq = stat->current_frequency;
		return 0;
	}

	/* Set the desired frequency based on the load */
	a = stat->busy_time;
	a *= stat->current_frequency * 1000000;
	b = div_u64(a, stat->total_time);
	b *= 100;
	b = div_u64(b, (dfso_upthreshold - dfso_downdifferential / 2));
	*freq = (unsigned long) b / 1000000;

	if (*freq < prev) {
		prev = *freq ;
		count++;
		if (count < 4) {
			*freq = stat->current_frequency;
			return 0;
		}
	}

	count = 0;
	prev = *freq ;

/* 	if (stat->current_frequency - *freq > 400)
		*freq = stat->current_frequency - 200; */

	return 0;
}

static int GB02FUNC657(struct gb_devfreq *devfreq,
				unsigned int event, void *data)
{
	switch (event) {
	case GB02MAC1044:
		GB02FUNC485(devfreq);
		break;

	case GB02MAC1045:
		GB02FUNC488(devfreq);
		break;

	case GB02MAC1047:
		GB02FUNC494(devfreq, (unsigned int *)data);
		break;

	case GB02MAC1049:
		GB02FUNC489(devfreq);
		break;

	case GB02MAC1051:
		GB02FUNC491(devfreq);
		break;

	default:
		break;
	}

	return 0;
}

struct gb_devfreq_governor devfreq_simple_ondemand = {
	.name = GB_DEVFREQ_GOV_SIMPLE_ONDEMAND,
	.get_target_freq = GB02FUNC644,
	.event_handler = GB02FUNC657,
};

