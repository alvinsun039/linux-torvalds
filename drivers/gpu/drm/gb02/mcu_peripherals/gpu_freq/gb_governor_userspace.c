// SPDX-License-Identifier: GPL-2.0-only
/*
 */

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include "gb_governor.h"

#define CUSTOM_MODE_PERF "perf"
#define CUSTOM_MODE_NORMAL "normal"
#define CUSTOM_MODE_POWERSAVE "powersave"
#define CUSTOM_MODE_DEEPPOWERSAVE "deeppowersave"

static int GB02FUNC666(struct gb_devfreq *df, unsigned long *freq)
{
	struct GB02STR103 *data = df->data;

	if (data->valid)
		*freq = data->user_frequency;
	else
		*freq = df->previous_freq; /* No user freq specified yet */

	return 0;
}

static ssize_t GB02FUNC668(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	ssize_t count = 0;

	count = sprintf(buf, "%s %s %s %s\n",
			CUSTOM_MODE_PERF, CUSTOM_MODE_NORMAL,
			CUSTOM_MODE_POWERSAVE, CUSTOM_MODE_DEEPPOWERSAVE);

	return count;
}
static ssize_t GB02FUNC671(struct device *dev, struct device_attribute *attr, char *buf)
{
	int err = 0;
	char *name = to_gb_devfreq(dev)->custom_mode.name;
	if (name)
		err = sprintf(buf, "%s\n", to_gb_devfreq(dev)->custom_mode.name);
	else
		err = sprintf(buf, "undefined\n");

	return err;
}

static ssize_t GB02FUNC673(struct device *dev, struct device_attribute *attr, const char *buf,
			size_t count)
{
	int ret;
	char str_mode[GB02MAC1000 + 1];
	struct GB02STR103 *data;
	struct gb_devfreq *df = to_gb_devfreq(dev);

	mutex_lock(&df->lock);
	data = df->data;
	ret = sscanf(buf, "%" __stringify(GB02MAC1000) "s", str_mode);
	if (ret != 1)
		return -EINVAL;

	df->custom_mode.is_custom = true;
	strcpy(df->custom_mode.name, str_mode);

	if (strcmp(str_mode, CUSTOM_MODE_PERF) == 0) {
		data->user_frequency = high_freq;
		data->tpu_cnt = 32;
		data->valid = true;
	} else if (strcmp(str_mode, CUSTOM_MODE_POWERSAVE) == 0) {
		data->user_frequency = low_freq;
		data->tpu_cnt = 8;
		data->valid = true;
	} else if (strcmp(str_mode, CUSTOM_MODE_DEEPPOWERSAVE) == 0) {
		data->user_frequency = lowest_freq;
		data->tpu_cnt = 1;
		data->valid = true;
	} else {
		data->user_frequency = mid_freq;
		data->tpu_cnt = 16;
		data->valid = true;
	}
	ret = GB02FUNC476(df);
	if (ret == 0)
		ret = count;
	mutex_unlock(&df->lock);

	return count;
}

static ssize_t GB02FUNC678(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct gb_devfreq *devfreq = to_gb_devfreq(dev);
	struct GB02STR103 *data;
	unsigned long wanted;
	int err = 0;

	mutex_lock(&devfreq->lock);
	data = devfreq->data;

	sscanf(buf, "%lu", &wanted);
	data->user_frequency = wanted;
	data->valid = true;

	err = GB02FUNC476(devfreq);
	if (err == 0)
		err = count;
	mutex_unlock(&devfreq->lock);
	return err;
}

static ssize_t GB02FUNC680(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct gb_devfreq *devfreq = to_gb_devfreq(dev);
	struct GB02STR103 *data;
	int err = 0;

	mutex_lock(&devfreq->lock);
	data = devfreq->data;

	if (data->valid)
		err = sprintf(buf, "%lu\n", data->user_frequency);
	else
		err = sprintf(buf, "undefined\n");
	mutex_unlock(&devfreq->lock);
	return err;
}

static DEVICE_ATTR(set_freq, 0644, GB02FUNC680, GB02FUNC678);
static DEVICE_ATTR(custom_mode, 0644, GB02FUNC671, GB02FUNC673);
static DEVICE_ATTR(available_custom_mode, 0644, GB02FUNC668, NULL);
static struct attribute *dev_entries[] = {
	&dev_attr_set_freq.attr,
	&dev_attr_custom_mode.attr,
	&dev_attr_available_custom_mode.attr,
	NULL,
};
static const struct attribute_group dev_attr_group = {
	.name	= GB_DEVFREQ_GOV_USERSPACE,
	.attrs	= dev_entries,
};

static int GB02FUNC683(struct gb_devfreq *devfreq)
{
	int err = 0;
	struct GB02STR103 *data = kzalloc(sizeof(struct GB02STR103),
					      GFP_KERNEL);
	struct GB02STR98 *freq_table = GB02FUNC424(devfreq->dev.parent, lowest_freq);

	if (!data) {
		err = -ENOMEM;
		goto out;
	}
	data->valid = false;
	devfreq->data = data;

	err = sysfs_create_group(&devfreq->dev.kobj, &dev_attr_group);
	GB02FUNC427(freq_table, true);
out:
	return err;
}

static void GB02FUNC685(struct gb_devfreq *devfreq)
{
	struct GB02STR98 *freq_table = GB02FUNC424(devfreq->dev.parent, lowest_freq);

	GB02FUNC427(freq_table, false);
	/*
	 * Remove the sysfs entry, unless this is being called after
	 * device_del(), which should have done this already via kobject_del().
	 */
	if (devfreq->dev.kobj.sd)
		sysfs_remove_group(&devfreq->dev.kobj, &dev_attr_group);

	kfree(devfreq->data);
	devfreq->data = NULL;
	devfreq->custom_mode.is_custom = false;
}

static int GB02FUNC687(struct gb_devfreq *devfreq,
			unsigned int event, void *data)
{
	int ret = 0;

	switch (event) {
	case GB02MAC1044:
		ret = GB02FUNC683(devfreq);
		break;
	case GB02MAC1045:
		GB02FUNC685(devfreq);
		break;
	default:
		break;
	}

	return ret;
}

struct gb_devfreq_governor devfreq_userspace = {
	.name = GB_DEVFREQ_GOV_USERSPACE,
	.get_target_freq = GB02FUNC666,
	.event_handler = GB02FUNC687,
};
