/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * gb_devfreq: Generic Dynamic Voltage and Frequency Scaling (DVFS) Framework
 *	    for GenBu.
 *
 */

#ifndef __GB_DEVFREQ_H__
#define __GB_DEVFREQ_H__

#include <linux/device.h>

#define GB02MAC997 20
#define GB02MAC1000 16

/* DEVFREQ governor name */
#define GB_DEVFREQ_GOV_SIMPLE_ONDEMAND	"gb_simple_ondemand"
#define GB_DEVFREQ_GOV_PERFORMANCE		"gb_performance"
#define GB_DEVFREQ_GOV_POWERSAVE		"gb_powersave"
#define GB_DEVFREQ_GOV_USERSPACE		"gb_userspace"

/* DEVFREQ notifier interface */
#define GB02MAC1006	(0)

/* Transition notifiers of DEVFREQ_TRANSITION_NOTIFIER */
#define	GB02MAC1010		(0)
#define	GB02MAC1012		(1)

struct gb_devfreq;
struct gb_devfreq_governor;

/**
 * struct gb_devfreq_dev_status - Data given from devfreq user device to
 *			     governors. Represents the performance
 *			     statistics.
 * @total_time:		The total time represented by this instance of
 *			gb_devfreq_dev_status
 * @busy_time:		The time that the device was working among the
 *			total_time.
 * @current_frequency:	The operating frequency.
 * @private_data:	An entry not specified by the devfreq framework.
 *			A device and a specific governor may have their
 *			own protocol with private_data. However, because
 *			this is governor-specific, a governor using this
 *			will be only compatible with devices aware of it.
 */
struct gb_devfreq_dev_status {
	/* both since the last measure */
	unsigned long total_time;
	unsigned long busy_time;
	unsigned long current_frequency;
	void *private_data;
};

struct GB02STR98 {
	bool available;
	unsigned long freq;
	unsigned long volt;
};

/**
 * struct gb_devfreq_dev_profile - Devfreq's user device profile
 * @initial_freq:	The operating frequency when devfreq_add_device() is
 *			called.
 * @polling_ms:		The polling interval in ms. 0 disables polling.
 * @target:		The device should set its operating frequency at
 *			freq or lowest-upper-than-freq value. If freq is
 *			higher than any operable frequency, set maximum.
 *			Before returning, target function should set
 *			freq at the current frequency.
 *			The "flags" parameter's possible values are
 *			explained above with "DEVFREQ_FLAG_*" macros.
 * @get_dev_status:	The device should provide the current performance
 *			status to devfreq. Governors are recommended not to
 *			use this directly. Instead, governors are recommended
 *			to use GB02FUNC626() along with
 *			devfreq.last_status.
 * @get_cur_freq:	The device should provide the current frequency
 *			at which it is operating.
 * @exit:		An optional callback that is called when devfreq
 *			is removing the devfreq object due to error or
 *			from devfreq_remove_device() call. If the user
 *			has registered devfreq->nb at a notifier-head,
 *			this is the time to unregister it.
 * @freq_table:		Optional list of frequencies to support statistics
 *			and freq_table must be generated in ascending order.
 * @max_state:		The size of freq_table.
 */
struct gb_devfreq_dev_profile {
	unsigned long initial_freq;
	unsigned int polling_ms;

	int (*target)(struct device *dev, unsigned long *freq);
	int (*get_dev_status)(struct device *dev,
			      struct gb_devfreq_dev_status *stat);
	int (*get_cur_freq)(struct device *dev, unsigned long *freq);
	void (*exit)(struct device *dev);

	struct GB02STR98 *freq_table;
	unsigned int max_state;
};

struct GB02STR99 {
	bool is_custom;
	char name[GB02MAC1000];
};
/**
 * struct gb_devfreq - Device devfreq structure
 * @node:	list node - contains the devices with devfreq that have been
 *		registered.
 * @lock:	a mutex to protect accessing devfreq.
 * @dev:	device registered by devfreq class. dev.parent is the device
 *		using devfreq.
 * @profile:	device-specific devfreq profile
 * @governor:	method how to choose frequency based on the usage.
 * @governor_name:	devfreq governor name for use with this devfreq
 * @nb:		notifier block used to notify devfreq object that it should
 *		reevaluate operable frequencies. Devfreq users may use
 *		devfreq.nb to the corresponding register notifier call chain.
 * @work:	delayed work for load monitoring.
 * @previous_freq:	previously configured frequency value.
 * @data:	Private data of the governor. The devfreq framework does not
 *		touch this.
 * @min_freq:	Limit minimum frequency requested by user (0: none)
 * @max_freq:	Limit maximum frequency requested by user (0: none)
 * @scaling_min_freq:	Limit minimum frequency requested by OPP interface
 * @scaling_max_freq:	Limit maximum frequency requested by OPP interface
 * @stop_polling:	 devfreq polling status of a device.
 * @suspend_freq:	 frequency of a device set during suspend phase.
 * @resume_freq:	 frequency of a device set in resume phase.
 * @suspend_count:	 suspend requests counter for a device.
 * @total_trans:	Number of devfreq transitions
 * @trans_table:	Statistics of devfreq transitions
 * @time_in_state:	Statistics of devfreq states
 * @last_stat_updated:	The last time stat updated
 * @transition_notifier_list: list head of DEVFREQ_TRANSITION_NOTIFIER notifier
 *
 * This structure stores the devfreq information for a give device.
 *
 * Note that when a governor accesses entries in struct gb_devfreq in its
 * functions except for the context of callbacks defined in struct
 * devfreq_governor, the governor should protect its access with the
 * struct mutex lock in struct gb_devfreq. A governor may use this mutex
 * to protect its own private data in void *data as well.
 */
struct gb_devfreq {
	struct list_head node;

	struct mutex lock;
	struct device dev;
	struct gb_devfreq_dev_profile *profile;
	const struct gb_devfreq_governor *governor;
	char governor_name[GB02MAC997];
	struct delayed_work work;

	unsigned long previous_freq;
	struct gb_devfreq_dev_status last_status;

	void *data; /* private data for governors */

	unsigned long min_freq;
	unsigned long max_freq;
	unsigned long scaling_min_freq;
	unsigned long scaling_max_freq;
	bool stop_polling;

	unsigned long suspend_freq;//reserve
	unsigned long resume_freq;//reserve
	atomic_t suspend_count;//reserve

	/* information for device frequency transition */
	unsigned int total_trans;
	unsigned int *trans_table;
	unsigned long *time_in_state;
	unsigned long last_stat_updated;
	struct GB02STR99 custom_mode;
};

struct gb_devfreq_freqs {
	unsigned long old;
	unsigned long new;
};

enum freq_e {
	perf_freq = 900,
	high_freq = 800,
	mid_freq = 700,
	low_freq = 600,
	lowest_freq = 400,
};

#ifdef CONFIG_MCU_FREQ
extern int GB02FUNC595(void);
extern int GB02FUNC533(void);
extern int GB02FUNC546(void);
extern void GB02FUNC597(void);
extern struct GB02STR98 *GB02FUNC424(struct device *dev, unsigned long freq);
extern struct gb_devfreq *GB02FUNC522(struct device *dev,
				  struct gb_devfreq_dev_profile *profile,
				  const char *governor_name,
				  void *data);
extern void GB02FUNC524(struct device *dev,
				  struct gb_devfreq *devfreq);

/**
 * GB02FUNC476() - Reevaluate the device and configure frequency
 * @devfreq:	the devfreq device
 *
 * Note: devfreq->lock must be held
 */
extern int GB02FUNC476(struct gb_devfreq *devfreq);

/**
 * struct gb_devfreq_simple_ondemand_data - void *data fed to struct gb_devfreq
 *	and devfreq_add_device
 * @upthreshold:	If the load is over this value, the frequency jumps.
 *			Specify 0 to use the default. Valid value = 0 to 100.
 * @downdifferential:	If the load is under upthreshold - downdifferential,
 *			the governor may consider slowing the frequency down.
 *			Specify 0 to use the default. Valid value = 0 to 100.
 *			downdifferential < upthreshold must hold.
 *
 * If the fed gb_devfreq_simple_ondemand_data pointer is NULL to the governor,
 * the governor uses the default values.
 */
struct gb_devfreq_simple_ondemand_data {
	unsigned int upthreshold;
	unsigned int downdifferential;
};

struct GB02STR103 {
	unsigned long user_frequency;
	unsigned long tpu_cnt;
	bool valid;
};

extern void GB02FUNC427(struct GB02STR98 *freq_table, bool enable);
extern void GB02FUNC485(struct gb_devfreq *devfreq);
extern void GB02FUNC488(struct gb_devfreq *devfreq);
extern void GB02FUNC489(struct gb_devfreq *devfreq);
extern void GB02FUNC491(struct gb_devfreq *devfreq);
extern void GB02FUNC494(struct gb_devfreq *devfreq,
					unsigned int *delay);

#else /* !CONFIG_GB_DEVFREQ */
static inline int GB02FUNC595(void)
{
	return -ENODEV;
}

static inline void GB02FUNC597(void)
{
}

static inline int GB02FUNC533(void)
{
	return 0;
}

static inline int GB02FUNC546(void)
{
	return 0;
}

struct GB02STR98 *GB02FUNC424(struct device *dev, unsigned long freq)
{
	return ERR_PTR(-EINVAL);
}

static inline struct gb_devfreq *GB02FUNC522(struct device *dev,
					struct gb_devfreq_dev_profile *profile,
					const char *governor_name,
					void *data)
{
	return ERR_PTR(-ENOSYS);
}

static inline void GB02FUNC524(struct device *dev,
					struct gb_devfreq *devfreq)
{
}

static inline void GB02FUNC427(struct GB02STR98 *freq_table, bool enable)
{
}

static inline void GB02FUNC485(struct gb_devfreq *devfreq)
{
}
static inline void GB02FUNC488(struct gb_devfreq *devfreq)
{
}
static inline void GB02FUNC489(struct gb_devfreq *devfreq)
{
}
static inline void GB02FUNC491(struct gb_devfreq *devfreq)
{
}
static inline void GB02FUNC494(struct gb_devfreq *devfreq,
					unsigned int *delay)
{
}
#endif /* CONFIG_MCU_FREQ */

#endif /* __GB_DEVFREQ_H__ */
