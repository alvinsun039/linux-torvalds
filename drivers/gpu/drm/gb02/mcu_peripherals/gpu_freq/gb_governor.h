/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * gb_governor.h - internal header for devfreq governors.
 */

#ifndef _GB_GOVERNOR_H
#define _GB_GOVERNOR_H

#include "gb_devfreq.h"

#define to_gb_devfreq(DEV)	container_of((DEV), struct gb_devfreq, dev)

/* Devfreq events */
#define GB02MAC1044			0x1
#define GB02MAC1045			0x2
#define GB02MAC1047			0x3
#define GB02MAC1049			0x4
#define GB02MAC1051			0x5

#define GB02MAC1054			0
#define GB02MAC1056			ULONG_MAX

/**
 * struct gb_devfreq_governor - Devfreq policy governor
 * @node:		list node - contains registered devfreq governors
 * @name:		Governor's name
 * @immutable:		Immutable flag for governor. If the value is 1,
 *			this govenror is never changeable to other governor.
 * @get_target_freq:	Returns desired operating frequency for the device.
 *			Basically, get_target_freq will run
 *			devfreq_dev_profile.get_dev_status() to get the
 *			status of the device (load = busy_time / total_time).
 *			If no_central_polling is set, this callback is called
 *			only with GB02FUNC476() notified by OPP.
 * @event_handler:      Callback for devfreq core framework to notify events
 *                      to governors. Events include per device governor
 *                      init and exit, opp changes out of devfreq, suspend
 *                      and resume of per device devfreq during device idle.
 *
 * Note that the callbacks are called with devfreq->lock locked by devfreq.
 */
struct gb_devfreq_governor {
	struct list_head node;

	const char name[GB02MAC997];
	const unsigned int immutable;
	int (*get_target_freq)(struct gb_devfreq *this, unsigned long *freq);
	int (*event_handler)(struct gb_devfreq *devfreq,
				unsigned int event, void *data);
};
extern struct gb_devfreq_governor devfreq_performance;
extern struct gb_devfreq_governor devfreq_simple_ondemand;
extern struct gb_devfreq_governor devfreq_userspace;
extern struct gb_devfreq_governor devfreq_powersave;

extern int devfreq_update_status(struct gb_devfreq *devfreq, unsigned long freq);

static inline int GB02FUNC626(struct gb_devfreq *df)
{
	return df->profile->get_dev_status(df->dev.parent, &df->last_status);
}
#endif /* _GB_GOVERNOR_H */
