// SPDX-License-Identifier: GPL-2.0-only
/*
 * devfreq: Generic Dynamic Voltage and Frequency Scaling (DVFS) Framework
 *	    for Non-CPU Devices.
 *
 */

#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/overflow.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/hrtimer.h>
#include <linux/version.h>
#include "gb_governor.h"
#include "gb_devfreq.h"

static struct class *gb_devfreq_class;

/*
 * devfreq core provides delayed work based load monitoring helper
 * functions. Governors can use these or can implement their own
 * monitoring mechanism.
 */
static struct workqueue_struct *gb_devfreq_wq;

/* The list of all device-devfreq governors */
static LIST_HEAD(gb_devfreq_governor_list);
/* The list of all device-devfreq */
static LIST_HEAD(gb_devfreq_list);
static DEFINE_MUTEX(gb_devfreq_list_lock);

struct GB02STR98 *GB02FUNC424(struct device *dev, unsigned long freq)
{
	int i = 0;
	struct GB02STR98 *freq_table;
	unsigned long target_freq = freq;
	struct gb_devfreq *devfreq;

	list_for_each_entry(devfreq, &gb_devfreq_list, node) {
		if (dev == devfreq->dev.parent)
			break;
	}

	freq_table = devfreq->profile->freq_table;
	for (i = 0; i < devfreq->profile->max_state; i++) {
		if (target_freq > freq_table->freq)
			freq_table++;
		else
			break;
	}
	return freq_table;
}

void GB02FUNC427(struct GB02STR98 *freq_table, bool enable)
{
	freq_table->available = enable;
}

static int GB02FUNC428(struct gb_devfreq *devfreq, unsigned long *min_freq)
{
	int ret = 0, i = 0;
	struct GB02STR98 *freq_table = devfreq->profile->freq_table;
	int max_state = devfreq->profile->max_state;

	for (i = 0; i < max_state; i++) {
		if (freq_table->available) {
			*min_freq = freq_table->freq;
			break;
		}
		freq_table++;
	}

	for (i += 1; i < max_state; i++) {
		freq_table++;
		if (freq_table->available && *min_freq >freq_table->freq)
			*min_freq = freq_table->freq;
	}

	return ret;
}

static int GB02FUNC436(struct gb_devfreq *devfreq, unsigned long *max_freq)
{
	int ret = 0, i = 0;
	struct GB02STR98 *freq_table = devfreq->profile->freq_table;
	int max_state = devfreq->profile->max_state;

	for (i = 0; i < max_state; i++) {
		if (freq_table->available) {
			*max_freq = freq_table->freq;
			break;
		}
		freq_table++;
	}

	for (i += 1; i < max_state; i++) {
		freq_table++;
		if (freq_table->available && *max_freq < freq_table->freq)
			*max_freq = freq_table->freq;
	}

	return ret;
}

static unsigned long GB02FUNC441(struct gb_devfreq *devfreq)
{
	int ret = 0;
	unsigned long min_freq = 0;

	ret = GB02FUNC428(devfreq, &min_freq);
	if (ret)
		min_freq = 0;

	return min_freq;
}

static unsigned long GB02FUNC446(struct gb_devfreq *devfreq)
{
	int ret = 0;
	unsigned long max_freq = ULONG_MAX;

	ret = GB02FUNC436(devfreq, &max_freq);
	if (ret)
		max_freq = 0;

	return max_freq;
}

/**
 * GB02FUNC447() - Lookup freq_table for the frequency
 * @devfreq:	the devfreq instance
 * @freq:	the target frequency
 */
static int GB02FUNC447(struct gb_devfreq *devfreq, unsigned long freq)
{
	int lev;

	for (lev = 0; lev < devfreq->profile->max_state; lev++)
		if (devfreq->profile->freq_table[lev].available && (freq == devfreq->profile->freq_table[lev].freq))
			return lev;

	return -EINVAL;
}

/**
 * GB02FUNC448() - Update statistics of devfreq behavior
 * @devfreq:	the devfreq instance
 * @freq:	the update target frequency
 */
int GB02FUNC448(struct gb_devfreq *devfreq, unsigned long freq)
{
	int lev, prev_lev, ret = 0;
	unsigned long cur_time;

	lockdep_assert_held(&devfreq->lock);
	cur_time = jiffies;

	/* Immediately exit if previous_freq is not initialized yet. */
	if (!devfreq->previous_freq)
		goto out;

	prev_lev = GB02FUNC447(devfreq, devfreq->previous_freq);
	if (prev_lev < 0) {
		ret = prev_lev;
		goto out;
	}

	devfreq->time_in_state[prev_lev] +=
			 cur_time - devfreq->last_stat_updated;

	lev = GB02FUNC447(devfreq, freq);
	if (lev < 0) {
		ret = lev;
		goto out;
	}

	if (lev != prev_lev) {
		devfreq->trans_table[(prev_lev *
				devfreq->profile->max_state) + lev]++;
		devfreq->total_trans++;
	}

out:
	devfreq->last_stat_updated = cur_time;
	return ret;
}

/**
 * GB02FUNC455() - find devfreq governor from name
 * @name:	name of the governor
 *
 * Search the list of devfreq governors and return the matched
 * governor's pointer. gb_devfreq_list_lock should be held by the caller.
 */
static struct gb_devfreq_governor *GB02FUNC455(const char *name)
{
	struct gb_devfreq_governor *tmp_governor;

	if (IS_ERR_OR_NULL(name)) {
		pr_err("DEVFREQ: %s: Invalid parameters\n", __func__);
		return ERR_PTR(-EINVAL);
	}
	WARN(!mutex_is_locked(&gb_devfreq_list_lock),
	     "gb_devfreq_list_lock must be locked.");

	list_for_each_entry(tmp_governor, &gb_devfreq_governor_list, node) {
		if (!strncmp(tmp_governor->name, name, GB02MAC997))
			return tmp_governor;
	}

	return ERR_PTR(-ENODEV);
}

/**
 * GB02FUNC459() - Try to find the governor and request the
 *                               module if is not found.
 * @name:	name of the governor
 *
 * Search the list of devfreq governors and request the module and try again
 * if is not found. This can happen when both drivers (the governor driver
 * and the driver that call devfreq_add_device) are built as modules.
 * gb_devfreq_list_lock should be held by the caller. Returns the matched
 * governor's pointer or an error pointer.
 */
static struct gb_devfreq_governor *GB02FUNC459(const char *name)
{
	struct gb_devfreq_governor *governor;
	int err = 0;

	if (IS_ERR_OR_NULL(name)) {
		pr_err("DEVFREQ: %s: Invalid parameters\n", __func__);
		return ERR_PTR(-EINVAL);
	}
	WARN(!mutex_is_locked(&gb_devfreq_list_lock),
	     "gb_devfreq_list_lock must be locked.");

	governor = GB02FUNC455(name);
	if (IS_ERR(governor)) {
		mutex_unlock(&gb_devfreq_list_lock);

		if (!strncmp(name, GB_DEVFREQ_GOV_SIMPLE_ONDEMAND,
			     GB02MAC997))
			err = request_module("governor_%s", "gb_simpleondemand");
		else
			err = request_module("governor_%s", name);
		/* Restore previous state before return */
		mutex_lock(&gb_devfreq_list_lock);
		if (err)
			return (err < 0) ? ERR_PTR(err) : ERR_PTR(-EINVAL);

		governor = GB02FUNC455(name);
	}

	return governor;
}

static int GB02FUNC467(struct gb_devfreq *devfreq, unsigned long new_freq)
{
	struct gb_devfreq_freqs freqs;
	unsigned long cur_freq;
	int err = 0;

	if (devfreq->profile->get_cur_freq)
		devfreq->profile->get_cur_freq(devfreq->dev.parent, &cur_freq);
	else
		cur_freq = devfreq->previous_freq;

	freqs.old = cur_freq;
	freqs.new = new_freq;

	err = devfreq->profile->target(devfreq->dev.parent, &new_freq);
	if (err) {
		freqs.new = cur_freq;
		return err;
	}

	freqs.new = new_freq;

	if (GB02FUNC448(devfreq, new_freq))
		dev_err(&devfreq->dev,
			"Couldn't update frequency transition information.\n");

	devfreq->previous_freq = new_freq;

	if (devfreq->suspend_freq)
		devfreq->resume_freq = cur_freq;

	return err;
}

/**
 * GB02FUNC476() - Reevaluate the device and configure frequency.
 * @devfreq:	the devfreq instance.
 *
 * Note: Lock devfreq->lock before calling GB02FUNC476
 *	 This function is exported for governors.
 */
int GB02FUNC476(struct gb_devfreq *devfreq)
{
	unsigned long freq, min_freq, max_freq;
	int err = 0;

	if (!mutex_is_locked(&devfreq->lock)) {
		WARN(true, "devfreq->lock must be locked by the caller.\n");
		return -EINVAL;
	}

	if (!devfreq->governor)
		return -EINVAL;

	/* Reevaluate the proper frequency */
	err = devfreq->governor->get_target_freq(devfreq, &freq);
	if (err)
		return err;

	/*
	 * Adjust the frequency with user freq, QoS and available freq.
	 *
	 * List from the highest priority
	 * max_freq
	 * min_freq
	 */
	max_freq = GB02FUNC446(devfreq);
	min_freq = GB02FUNC441(devfreq);

	if (freq < min_freq)
		freq = min_freq;

	if (freq > max_freq)
		freq = max_freq;

	return GB02FUNC467(devfreq, freq);

}

/**
 * GB02FUNC481() - Periodically poll devfreq objects.
 * @work:	the work struct used to run GB02FUNC481 periodically.
 *
 */
static void GB02FUNC481(struct work_struct *work)
{
	int err;
	struct gb_devfreq *devfreq = container_of(work,
					struct gb_devfreq, work.work);

	mutex_lock(&devfreq->lock);
	err = GB02FUNC476(devfreq);
	if (err)
		dev_err(&devfreq->dev, "dvfs failed with (%d) error\n", err);

	queue_delayed_work(gb_devfreq_wq, &devfreq->work,
				msecs_to_jiffies(devfreq->profile->polling_ms));
	mutex_unlock(&devfreq->lock);
}

/**
 * GB02FUNC485() - Start load monitoring of devfreq instance
 * @devfreq:	the devfreq instance.
 *
 * Helper function for starting devfreq device load monitoring. By
 * default delayed work based monitoring is supported. Function
 * to be called from governor in response to GB02MAC1044
 * event when device is added to devfreq framework.
 */
void GB02FUNC485(struct gb_devfreq *devfreq)
{
	INIT_DEFERRABLE_WORK(&devfreq->work, GB02FUNC481);
	if (devfreq->profile->polling_ms)
		queue_delayed_work(gb_devfreq_wq, &devfreq->work,
			msecs_to_jiffies(devfreq->profile->polling_ms));
}

/**
 * GB02FUNC488() - Stop load monitoring of a devfreq instance
 * @devfreq:	the devfreq instance.
 *
 * Helper function to stop devfreq device load monitoring. Function
 * to be called from governor in response to GB02MAC1045
 * event when device is removed from devfreq framework.
 */
void GB02FUNC488(struct gb_devfreq *devfreq)
{
	cancel_delayed_work_sync(&devfreq->work);
}

/**
 * GB02FUNC489() - Suspend load monitoring of a devfreq instance
 * @devfreq:	the devfreq instance.
 *
 * Helper function to suspend devfreq device load monitoring. Function
 * to be called from governor in response to GB02MAC1049
 * event or when polling interval is set to zero.
 *
 * Note: Though this function is same as GB02FUNC488(),
 * intentionally kept separate to provide hooks for collecting
 * transition statistics.
 */
void GB02FUNC489(struct gb_devfreq *devfreq)
{
	mutex_lock(&devfreq->lock);
	if (devfreq->stop_polling) {
		mutex_unlock(&devfreq->lock);
		return;
	}

	GB02FUNC448(devfreq, devfreq->max_freq);
	devfreq->stop_polling = true;
	mutex_unlock(&devfreq->lock);
	cancel_delayed_work_sync(&devfreq->work);
}

/**
 * GB02FUNC491() - Resume load monitoring of a devfreq instance
 * @devfreq:    the devfreq instance.
 *
 * Helper function to resume devfreq device load monitoring. Function
 * to be called from governor in response to GB02MAC1051
 * event or when polling interval is set to non-zero.
 */
void GB02FUNC491(struct gb_devfreq *devfreq)
{
	unsigned long freq;

	mutex_lock(&devfreq->lock);
	if (!devfreq->stop_polling)
		goto out;

	if (!delayed_work_pending(&devfreq->work) &&
			devfreq->profile->polling_ms)
		queue_delayed_work(gb_devfreq_wq, &devfreq->work,
			msecs_to_jiffies(devfreq->profile->polling_ms));

	devfreq->last_stat_updated = jiffies;
	devfreq->stop_polling = false;

	if (devfreq->profile->get_cur_freq &&
		!devfreq->profile->get_cur_freq(devfreq->dev.parent, &freq))
		devfreq->previous_freq = freq;

out:
	mutex_unlock(&devfreq->lock);
}

/**
 * GB02FUNC494() - Update device devfreq monitoring interval
 * @devfreq:    the devfreq instance.
 * @delay:      new polling interval to be set.
 *
 * Helper function to set new load monitoring polling interval. Function
 * to be called from governor in response to GB02MAC1047 event.
 */
void GB02FUNC494(struct gb_devfreq *devfreq, unsigned int *delay)
{
	unsigned int cur_delay = devfreq->profile->polling_ms;
	unsigned int new_delay = *delay;

	mutex_lock(&devfreq->lock);
	devfreq->profile->polling_ms = new_delay;
	pr_err("polling_ms = %d\n", devfreq->profile->polling_ms);

	if (devfreq->stop_polling)
		goto out;

	/* if new delay is zero, stop polling */
	if (!new_delay) {
		mutex_unlock(&devfreq->lock);
		cancel_delayed_work_sync(&devfreq->work);
		return;
	}

	/* if current delay is zero, start polling with new delay */
	if (!cur_delay) {
		queue_delayed_work(gb_devfreq_wq, &devfreq->work,
			msecs_to_jiffies(devfreq->profile->polling_ms));
		goto out;
	}

	/* if current delay is greater than new delay, restart polling */
	if (cur_delay > new_delay) {
		mutex_unlock(&devfreq->lock);
		cancel_delayed_work_sync(&devfreq->work);
		mutex_lock(&devfreq->lock);
		if (!devfreq->stop_polling)
			queue_delayed_work(gb_devfreq_wq, &devfreq->work,
				msecs_to_jiffies(devfreq->profile->polling_ms));
	}
out:
	mutex_unlock(&devfreq->lock);
}

/**
 * GB02FUNC498() - Callback for struct device to release the device.
 * @dev:	the devfreq device
 *
 * Remove devfreq from the list and release its resources.
 */
static void GB02FUNC498(struct device *dev)
{
	struct gb_devfreq *devfreq = to_gb_devfreq(dev);

	mutex_lock(&gb_devfreq_list_lock);
	list_del(&devfreq->node);
	mutex_unlock(&gb_devfreq_list_lock);

	if (devfreq->profile->exit)
		devfreq->profile->exit(devfreq->dev.parent);

	mutex_destroy(&devfreq->lock);
	kfree(devfreq);
}

/**
 * GB02FUNC499() - Remove devfreq feature from a device.
 * @devfreq:	the devfreq instance to be removed
 *
 * The opposite of GB02FUNC501().
 */
static int GB02FUNC499(struct gb_devfreq *devfreq)
{
	if (!devfreq)
		return -EINVAL;

	if (devfreq->governor)
		devfreq->governor->event_handler(devfreq,
						 GB02MAC1045, NULL);
	device_unregister(&devfreq->dev);

	return 0;
}

/**
 * GB02FUNC501() - Add devfreq feature to the device
 * @dev:	the device to add devfreq feature.
 * @profile:	device-specific profile to run devfreq.
 * @governor_name:	name of the policy to choose frequency.
 * @data:	private data for the governor. The devfreq framework does not
 *		touch this value.
 */
static struct gb_devfreq *GB02FUNC501(struct device *dev,
				   struct gb_devfreq_dev_profile *profile,
				   const char *governor_name,
				   void *data)
{
	struct gb_devfreq *devfreq;
	struct gb_devfreq_governor *governor;
	static atomic_t devfreq_no = ATOMIC_INIT(-1);
	int err = 0;

	if (!dev || !profile || !governor_name) {
		dev_err(dev, "%s: Invalid parameters.\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	devfreq = kzalloc(sizeof(struct gb_devfreq), GFP_KERNEL);
	if (!devfreq) {
		err = -ENOMEM;
		goto err_out;
	}

	mutex_init(&devfreq->lock);
	mutex_lock(&devfreq->lock);
	devfreq->dev.parent = dev;
	devfreq->dev.class = gb_devfreq_class;
	devfreq->dev.release = GB02FUNC498;
	INIT_LIST_HEAD(&devfreq->node);
	devfreq->profile = profile;
	strncpy(devfreq->governor_name, governor_name, GB02MAC997);
	devfreq->previous_freq = profile->initial_freq;
	devfreq->last_status.current_frequency = profile->initial_freq;
	devfreq->data = data;

	if (!devfreq->profile->freq_table) {
		mutex_unlock(&devfreq->lock);
		goto err_dev;
	}

	devfreq->scaling_min_freq = GB02FUNC441(devfreq);
	if (!devfreq->scaling_min_freq) {
		mutex_unlock(&devfreq->lock);
		err = -EINVAL;
		goto err_dev;
	}
	devfreq->min_freq = devfreq->scaling_min_freq;

	devfreq->scaling_max_freq = GB02FUNC446(devfreq);
	if (!devfreq->scaling_max_freq) {
		mutex_unlock(&devfreq->lock);
		err = -EINVAL;
		goto err_dev;
	}
	devfreq->max_freq = devfreq->scaling_max_freq;

	dev_set_name(&devfreq->dev, "gb_devfreq%d",
				atomic_inc_return(&devfreq_no));
	err = device_register(&devfreq->dev);
	if (err) {
		mutex_unlock(&devfreq->lock);
		put_device(&devfreq->dev);
		goto err_out;
	}

	devfreq->trans_table = devm_kzalloc(&devfreq->dev,
			array3_size(sizeof(unsigned int),
				    devfreq->profile->max_state,
				    devfreq->profile->max_state),
			GFP_KERNEL);
	if (!devfreq->trans_table) {
		mutex_unlock(&devfreq->lock);
		err = -ENOMEM;
		goto err_devfreq;
	}

	devfreq->time_in_state = devm_kcalloc(&devfreq->dev,
			devfreq->profile->max_state,
			sizeof(unsigned long),
			GFP_KERNEL);
	if (!devfreq->time_in_state) {
		mutex_unlock(&devfreq->lock);
		err = -ENOMEM;
		goto err_devfreq;
	}

	devfreq->last_stat_updated = jiffies;

	mutex_unlock(&devfreq->lock);

	mutex_lock(&gb_devfreq_list_lock);
	governor = GB02FUNC459(devfreq->governor_name);
	if (IS_ERR(governor)) {
		dev_err(dev, "%s: Unable to find governor for the device\n",
			__func__);
		err = PTR_ERR(governor);
		goto err_devfreq;
	}

	devfreq->governor = governor;
	list_add(&devfreq->node, &gb_devfreq_list);
	err = devfreq->governor->event_handler(devfreq, GB02MAC1044,
						NULL);
	if (err) {
		dev_err(dev, "%s: Unable to start governor for the device\n",
			__func__);
		goto err_devfreq;
	}
	mutex_unlock(&gb_devfreq_list_lock);

	return devfreq;

err_devfreq:
	GB02FUNC499(devfreq);
	devfreq = NULL;
err_dev:
	kfree(devfreq);
err_out:
	return ERR_PTR(err);
}

static int GB02FUNC519(struct device *dev, void *res, void *data)
{
	struct gb_devfreq **r = res;

	if (WARN_ON(!r || !*r))
		return 0;

	return *r == data;
}

static void GB02FUNC521(struct device *dev, void *res)
{
	GB02FUNC499(*(struct gb_devfreq **)res);
}

/**
 * GB02FUNC522() - Resource-managed GB02FUNC501()
 * @dev:	the device to add devfreq feature.
 * @profile:	device-specific profile to run devfreq.
 * @governor_name:	name of the policy to choose frequency.
 * @data:	private data for the governor. The devfreq framework does not
 *		touch this value.
 *
 * This function manages automatically the memory of devfreq device using device
 * resource management and simplify the free operation for memory of devfreq
 * device.
 */
struct gb_devfreq *GB02FUNC522(struct device *dev,
					struct gb_devfreq_dev_profile *profile,
					const char *governor_name,
					void *data)
{
	struct gb_devfreq **ptr, *devfreq;

	ptr = devres_alloc(GB02FUNC521, sizeof(*ptr), GFP_KERNEL);
	if (!ptr)
		return ERR_PTR(-ENOMEM);

	devfreq = GB02FUNC501(dev, profile, governor_name, data);
	if (IS_ERR(devfreq)) {
		devres_free(ptr);
		return devfreq;
	}

	*ptr = devfreq;
	devres_add(dev, ptr);

	return devfreq;
}

/**
 * GB02FUNC524() - Resource-managed GB02FUNC499()
 * @dev:	the device from which to remove devfreq feature.
 * @devfreq:	the devfreq instance to be removed
 */
void GB02FUNC524(struct device *dev, struct gb_devfreq *devfreq)
{
	WARN_ON(devres_release(dev, GB02FUNC521,
			       GB02FUNC519, devfreq));
}

/**
 * GB02FUNC526() - Add devfreq governor
 * @governor:	the devfreq governor to be added
 */
static int GB02FUNC526(struct gb_devfreq_governor *governor)
{
	struct gb_devfreq_governor *g;
	struct gb_devfreq *devfreq;
	int err = 0;

	if (!governor) {
		pr_err("%s: Invalid parameters.\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&gb_devfreq_list_lock);
	g = GB02FUNC455(governor->name);
	if (!IS_ERR(g)) {
		pr_err("%s: governor %s already registered\n", __func__,
		       g->name);
		err = -EINVAL;
		goto err_out;
	}

	list_add(&governor->node, &gb_devfreq_governor_list);

	list_for_each_entry(devfreq, &gb_devfreq_list, node) {
		int ret = 0;
		struct device *dev = devfreq->dev.parent;

		if (!strncmp(devfreq->governor_name, governor->name,
			     GB02MAC997)) {
			/* The following should never occur */
			if (devfreq->governor) {
				dev_warn(dev,
					 "%s: Governor %s already present\n",
					 __func__, devfreq->governor->name);
				ret = devfreq->governor->event_handler(devfreq,
							GB02MAC1045, NULL);
				if (ret) {
					dev_warn(dev,
						 "%s: Governor %s stop = %d\n",
						 __func__,
						 devfreq->governor->name, ret);
				}
				/* Fall through */
			}
			devfreq->governor = governor;
			ret = devfreq->governor->event_handler(devfreq,
						GB02MAC1044, NULL);
			if (ret) {
				dev_warn(dev, "%s: Governor %s start=%d\n",
					 __func__, devfreq->governor->name,
					 ret);
			}
		}
	}
err_out:
	mutex_unlock(&gb_devfreq_list_lock);

	return err;
}

int GB02FUNC533(void)
{
	int ret = 0;

	ret = GB02FUNC526(&devfreq_performance);
	if (ret)
		pr_err("GB02FUNC526 %s failed\n", devfreq_performance.name);
	ret = GB02FUNC526(&devfreq_simple_ondemand);
	if (ret)
		pr_err("GB02FUNC526 %s failed\n", devfreq_simple_ondemand.name);
	ret = GB02FUNC526(&devfreq_userspace);
	if (ret)
		pr_err("GB02FUNC526 %s failed\n", devfreq_userspace.name);
	ret = GB02FUNC526(&devfreq_powersave);
	if (ret)
		pr_err("GB02FUNC526 %s failed\n", devfreq_powersave.name);

	return ret;
}

static int GB02FUNC537(struct gb_devfreq_governor *governor)
{
	struct gb_devfreq_governor *g;
	struct gb_devfreq *devfreq;
	int err = 0;

	if (!governor) {
		pr_err("%s: Invalid parameters.\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&gb_devfreq_list_lock);
	g = GB02FUNC455(governor->name);
	if (IS_ERR(g)) {
		pr_err("%s: governor %s not registered\n", __func__,
		       governor->name);
		err = PTR_ERR(g);
		goto err_out;
	}
	list_for_each_entry(devfreq, &gb_devfreq_list, node) {
		int ret;
		struct device *dev = devfreq->dev.parent;

		if (!strncmp(devfreq->governor_name, governor->name,
			     GB02MAC997)) {
			/* we should have a devfreq governor! */
			if (!devfreq->governor) {
				dev_warn(dev, "%s: Governor %s NOT present\n",
					 __func__, governor->name);
				continue;
				/* Fall through */
			}
			ret = devfreq->governor->event_handler(devfreq,
						GB02MAC1045, NULL);
			if (ret) {
				dev_warn(dev, "%s: Governor %s stop=%d\n",
					 __func__, devfreq->governor->name,
					 ret);
			}
			devfreq->governor = NULL;
		}
	}

	list_del(&governor->node);
err_out:
	mutex_unlock(&gb_devfreq_list_lock);

	return err;
}

int GB02FUNC546(void)
{
	int ret = 0;

	ret = GB02FUNC537(&devfreq_performance);
	if (ret)
		pr_err("GB02FUNC537 %s failed\n", devfreq_performance.name);
	ret = GB02FUNC537( &devfreq_simple_ondemand);
	if (ret)
		pr_err("GB02FUNC537 %s failed\n", devfreq_simple_ondemand.name);
	ret = GB02FUNC537(&devfreq_userspace);
	if (ret)
		pr_err("GB02FUNC537 %s failed\n", devfreq_userspace.name);
	ret = GB02FUNC537(&devfreq_powersave);
	if (ret)
		pr_err("GB02FUNC537 %s failed\n", devfreq_powersave.name);

	return ret;
}

static ssize_t name_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct gb_devfreq *devfreq = to_gb_devfreq(dev);
	return sprintf(buf, "%s\n", dev_name(devfreq->dev.parent));
}
static DEVICE_ATTR_RO(name);

static ssize_t governor_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	if (!to_gb_devfreq(dev)->governor)
		return -EINVAL;

	return sprintf(buf, "%s\n", to_gb_devfreq(dev)->governor->name);
}

static ssize_t governor_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct gb_devfreq *df = to_gb_devfreq(dev);
	int ret;
	char str_governor[GB02MAC997 + 1];
	const struct gb_devfreq_governor *governor, *prev_governor;

	ret = sscanf(buf, "%" __stringify(GB02MAC997) "s", str_governor);
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&gb_devfreq_list_lock);
	governor = GB02FUNC459(str_governor);
	if (IS_ERR(governor)) {
		ret = PTR_ERR(governor);
		goto out;
	}
	if (df->governor == governor) {
		ret = 0;
		goto out;
	} else if ((df->governor && df->governor->immutable) ||
					governor->immutable) {
		ret = -EINVAL;
		goto out;
	}

	if (df->governor) {
		ret = df->governor->event_handler(df, GB02MAC1045, NULL);
		if (ret) {
			dev_warn(dev, "%s: Governor %s not stopped(%d)\n",
				 __func__, df->governor->name, ret);
			goto out;
		}
	}
	prev_governor = df->governor;
	df->governor = governor;
	strncpy(df->governor_name, governor->name, GB02MAC997);
	ret = df->governor->event_handler(df, GB02MAC1044, NULL);
	if (ret) {
		dev_warn(dev, "%s: Governor %s not started(%d)\n",
			 __func__, df->governor->name, ret);
		df->governor = prev_governor;
		strncpy(df->governor_name, prev_governor->name,
			GB02MAC997);
		ret = df->governor->event_handler(df, GB02MAC1044, NULL);
		if (ret) {
			dev_err(dev,
				"%s: reverting to Governor %s failed (%d)\n",
				__func__, df->governor_name, ret);
			df->governor = NULL;
		}
	}
out:
	mutex_unlock(&gb_devfreq_list_lock);

	if (!ret)
		ret = count;
	return ret;
}
static DEVICE_ATTR_RW(governor);

static ssize_t available_governors_show(struct device *d,
					struct device_attribute *attr,
					char *buf)
{
	struct gb_devfreq *df = to_gb_devfreq(d);
	ssize_t count = 0;

	mutex_lock(&gb_devfreq_list_lock);

	/*
	 * The devfreq with immutable governor (e.g., passive) shows
	 * only own governor.
	 */
	if (df->governor && df->governor->immutable) {
		count = scnprintf(&buf[count], GB02MAC997,
				  "%s ", df->governor_name);
	/*
	 * The devfreq device shows the registered governor except for
	 * immutable governors such as passive governor .
	 */
	} else {
		struct gb_devfreq_governor *governor;

		list_for_each_entry(governor, &gb_devfreq_governor_list, node) {
			if (governor->immutable)
				continue;
			count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
					   "%s ", governor->name);
		}
	}

	mutex_unlock(&gb_devfreq_list_lock);

	/* Truncate the trailing space */
	if (count)
		count--;

	count += sprintf(&buf[count], "\n");

	return count;
}
static DEVICE_ATTR_RO(available_governors);

static ssize_t cur_freq_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	unsigned long freq;
	struct gb_devfreq *devfreq = to_gb_devfreq(dev);

	if (devfreq->profile->get_cur_freq &&
		!devfreq->profile->get_cur_freq(devfreq->dev.parent, &freq))
		return sprintf(buf, "%lu\n", freq);

	return sprintf(buf, "%lu\n", devfreq->previous_freq);
}
static DEVICE_ATTR_RO(cur_freq);

static ssize_t target_freq_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", to_gb_devfreq(dev)->previous_freq);
}
static DEVICE_ATTR_RO(target_freq);

static ssize_t polling_interval_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", to_gb_devfreq(dev)->profile->polling_ms);
}

static ssize_t polling_interval_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct gb_devfreq *df = to_gb_devfreq(dev);
	unsigned int value;
	int ret;

	if (!df->governor)
		return -EINVAL;

	ret = sscanf(buf, "%u", &value);
	if (ret != 1)
		return -EINVAL;

	df->governor->event_handler(df, GB02MAC1047, &value);
	ret = count;

	return ret;
}
static DEVICE_ATTR_RW(polling_interval);

static ssize_t min_freq_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct gb_devfreq *df = to_gb_devfreq(dev);
	struct GB02STR98 *freq_table;
	unsigned long value;
	int ret;

	ret = sscanf(buf, "%lu", &value);
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&df->lock);

	if (value) {
		if (value > df->max_freq) {
			ret = -EINVAL;
			goto unlock;
		}
	} else {
		freq_table = df->profile->freq_table;

		/* Get minimum frequency according to sorting order */
		if (freq_table[0].freq < freq_table[df->profile->max_state - 1].freq)
			value = freq_table[0].freq;
		else
			value = freq_table[df->profile->max_state - 1].freq;
	}

	df->min_freq = value;
	GB02FUNC476(df);
	ret = count;
unlock:
	mutex_unlock(&df->lock);
	return ret;
}

static ssize_t min_freq_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct gb_devfreq *df = to_gb_devfreq(dev);

	return sprintf(buf, "%lu\n", min(df->scaling_min_freq, df->min_freq));
}

static ssize_t max_freq_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct gb_devfreq *df = to_gb_devfreq(dev);
	struct GB02STR98 *freq_table;
	unsigned long value;
	int ret;

	ret = sscanf(buf, "%lu", &value);
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&df->lock);

	if (value) {
		if (value < df->min_freq) {
			ret = -EINVAL;
			goto unlock;
		}
	} else {
		freq_table = df->profile->freq_table;

		/* Get maximum frequency according to sorting order */
		if (freq_table[0].freq < freq_table[df->profile->max_state - 1].freq)
			value = GB02FUNC446(df);
		else
			value = freq_table[0].freq;
	}

	df->max_freq = value;
	GB02FUNC476(df);
	ret = count;
unlock:
	mutex_unlock(&df->lock);
	return ret;
}
static DEVICE_ATTR_RW(min_freq);

static ssize_t max_freq_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct gb_devfreq *df = to_gb_devfreq(dev);

	return sprintf(buf, "%lu\n", GB02FUNC446(df));
}
static DEVICE_ATTR_RW(max_freq);

static ssize_t available_frequencies_show(struct device *d,
					  struct device_attribute *attr,
					  char *buf)
{
	struct gb_devfreq *df = to_gb_devfreq(d);
	ssize_t count = 0;
	int i;

	mutex_lock(&df->lock);

	for (i = 0; i < df->profile->max_state; i++) {
		if (df->profile->freq_table[i].available)
			count += scnprintf(&buf[count], (PAGE_SIZE - count - 2),
					"%lu ", df->profile->freq_table[i].freq);
	}

	mutex_unlock(&df->lock);
	/* Truncate the trailing space */
	if (count)
		count--;

	count += sprintf(&buf[count], "\n");

	return count;
}
static DEVICE_ATTR_RO(available_frequencies);

static ssize_t trans_stat_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct gb_devfreq *devfreq = to_gb_devfreq(dev);
	ssize_t len;
	int i, j;
	unsigned int max_state = devfreq->profile->max_state;

	if (max_state == 0)
		return sprintf(buf, "Not Supported.\n");

	mutex_lock(&devfreq->lock);
	if (!devfreq->stop_polling &&
			GB02FUNC448(devfreq, devfreq->previous_freq)) {
		mutex_unlock(&devfreq->lock);
		return 0;
	}
	mutex_unlock(&devfreq->lock);

	len = sprintf(buf, "     From  :   To\n");
	len += sprintf(buf + len, "           :");
	for (i = 0; i < max_state; i++)
		len += sprintf(buf + len, "%10lu",
				devfreq->profile->freq_table[i].freq);

	len += sprintf(buf + len, "   time(ms)\n");

	for (i = 0; i < max_state; i++) {
		if (devfreq->profile->freq_table[i].freq
					== devfreq->previous_freq) {
			len += sprintf(buf + len, "*");
		} else {
			len += sprintf(buf + len, " ");
		}
		len += sprintf(buf + len, "%10lu:",
				devfreq->profile->freq_table[i].freq);
		for (j = 0; j < max_state; j++)
			len += sprintf(buf + len, "%10u",
				devfreq->trans_table[(i * max_state) + j]);
		len += sprintf(buf + len, "%10u\n",
			jiffies_to_msecs(devfreq->time_in_state[i]));
	}

	len += sprintf(buf + len, "Total transition : %u\n",
					devfreq->total_trans);
	return len;
}
static DEVICE_ATTR_RO(trans_stat);

static struct attribute *gb_devfreq_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_governor.attr,
	&dev_attr_available_governors.attr,
	&dev_attr_cur_freq.attr,
	&dev_attr_available_frequencies.attr,
	&dev_attr_target_freq.attr,
	&dev_attr_polling_interval.attr,
	&dev_attr_min_freq.attr,
	&dev_attr_max_freq.attr,
	&dev_attr_trans_stat.attr,
	NULL,
};
ATTRIBUTE_GROUPS(gb_devfreq);

int GB02FUNC595(void)
{
	int ret = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	gb_devfreq_class = class_create(THIS_MODULE, "gb_devfreq");
#else
	gb_devfreq_class = class_create("gb_devfreq");
#endif
	if (IS_ERR(gb_devfreq_class)) {
		pr_err("%s: couldn't create class\n", __FILE__);
		return PTR_ERR(gb_devfreq_class);
	}

	gb_devfreq_wq = alloc_workqueue("gb_devfreq_wq", WQ_HIGHPRI | WQ_SYSFS | WQ_FREEZABLE, 1);
	if (!gb_devfreq_wq) {
		class_destroy(gb_devfreq_class);
		pr_err("%s: couldn't create workqueue\n", __FILE__);
		return -ENOMEM;
	}
	gb_devfreq_class->dev_groups = gb_devfreq_groups;

	return ret;
}

void GB02FUNC597(void)
{
	class_destroy(gb_devfreq_class);
	destroy_workqueue(gb_devfreq_wq);
}
