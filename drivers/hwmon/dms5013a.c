// SPDX-License-Identifier: GPL-2.0
/*
 * dms5013a.c - Phytium cpu temperature monitoring module
 *
 * Inspired from many hwmon drivers especially from coretemp
 *
 * Copyright (C) 2019-2020 Zhengyuan Liu <liuzhengyuan@kylinos.cn>
 * Copyright (C) 2019-2024 Jackie Liu <liuyun01@kylinos.cn>
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/cleanup.h>
#include <linux/machine_t.h>

#define DRVNAME			"dms5013a"
#undef pr_fmt
#define pr_fmt(fmt)		DRVNAME ": " fmt

#define TEMP_NAME_LENGTH	19
#define TOTAL_ATTRS		2
#define MAX_CORE_DATA		128
#define DMS_BASE_ADDR		0x80028780000
#define IO_NODE_OFFSET		0x10000

struct temp_data {
	unsigned int temp;
	unsigned long last_updated;
	unsigned int numa_node_id;
	void *base_addr;
	void *data_addr;
	bool valid;
	struct sensor_device_attribute sd_attrs[TOTAL_ATTRS];
	char attr_name[TOTAL_ATTRS][TEMP_NAME_LENGTH];
	struct attribute *attrs[TOTAL_ATTRS + 1];
	struct attribute_group attr_group;
	struct mutex update_lock;
};

struct platform_data {
	struct device *hwmon_dev;
	struct temp_data *core_data[MAX_CORE_DATA];
	struct device_attribute name_attr;
	u64 temp_base;
};

static void writel_delay(void *addr, unsigned int val)
{
	writel(val, addr);
	mdelay(1);
}

static unsigned int get_temperature(struct temp_data *tdata)
{
	writel_delay(tdata->base_addr, 0x30);
	writel_delay(tdata->data_addr, 0x200 | 0x100 | 0x1);
	writel_delay(tdata->data_addr, 0x200 | 0x1);
	writel_delay(tdata->data_addr, 0x400 | 0x200 | 0x1);
	writel_delay(tdata->data_addr, 0x200 | 0x1);
	writel_delay(tdata->base_addr, 0x110);

	return ((readl(tdata->data_addr) & 0xfff) * 0.0489 - 40.0 + 0.625) * 1000;
}

static ssize_t show_label(struct device *dev,
			  struct device_attribute *devattr, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct platform_data *pdata = dev_get_drvdata(dev);
	struct temp_data *tdata = pdata->core_data[attr->index];

	return sprintf(buf, "Node %u\n", tdata->numa_node_id);
}

static ssize_t show_temp(struct device *dev,
			 struct device_attribute *devattr, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct platform_data *pdata = dev_get_drvdata(dev);
	struct temp_data *tdata = pdata->core_data[attr->index];

	guard(mutex)(&tdata->update_lock);
	if (!tdata->valid || time_after(jiffies, tdata->last_updated + HZ)) {
		tdata->temp = get_temperature(tdata);
		tdata->valid = 1;
		tdata->last_updated = jiffies;
	}
	return sprintf(buf, "%d\n", tdata->temp);
}

static int create_core_attrs(struct temp_data *tdata, struct device *dev,
			     int attr_no)
{
	int i;

	static ssize_t (*const rd_ptrs[TOTAL_ATTRS])(struct device *dev,
			struct device_attribute *devattr, char *buf) = {
		show_label, show_temp };
	static const char *const suffixes[TOTAL_ATTRS] = {
		"label", "input"
	};

	/*
	 *since command sensors will only take sys attr that with a suffix great
	 *than 0 into account, we simply make attr_no++ here. Otherwise sensors
	 *will only show 7 nodes, although there is 8.
	 */
	for (i = 0; i < ARRAY_SIZE(suffixes); i++) {
		snprintf(tdata->attr_name[i], TEMP_NAME_LENGTH,
			 "temp%d_%s", attr_no + 1, suffixes[i]);
		sysfs_attr_init(&tdata->sd_attrs[i].dev_attr.attr);
		tdata->sd_attrs[i].dev_attr.attr.name = tdata->attr_name[i];
		tdata->sd_attrs[i].dev_attr.attr.mode = 0444;
		tdata->sd_attrs[i].dev_attr.show = rd_ptrs[i];
		tdata->sd_attrs[i].index = attr_no;
		tdata->attrs[i] = &tdata->sd_attrs[i].dev_attr.attr;
	}

	tdata->attr_group.attrs = tdata->attrs;
	return sysfs_create_group(&dev->kobj, &tdata->attr_group);
}

static struct temp_data *init_temp_data(u64 temp_base, unsigned int node_id)
{
	struct temp_data *tdata;

	tdata = kzalloc(sizeof(struct temp_data), GFP_KERNEL);
	if (!tdata)
		return NULL;

	tdata->numa_node_id = node_id;
	mutex_init(&tdata->update_lock);
	/* temperature address register */
	tdata->base_addr = ioremap(temp_base + node_id * IO_NODE_OFFSET, 4);
	/* temperature data register */
	tdata->data_addr = ioremap(temp_base + 8 + node_id * IO_NODE_OFFSET, 4);
	return tdata;
}

static int create_core_data(struct platform_device *pdev, unsigned int node_id)
{
	struct temp_data *tdata;
	struct platform_data *pdata = platform_get_drvdata(pdev);
	int err;

	if (node_id > MAX_CORE_DATA - 1)
		return -ERANGE;

	if (pdata->core_data[node_id] != NULL)
		return 0;

	tdata = init_temp_data(pdata->temp_base, node_id);
	if (!tdata)
		return -ENOMEM;

	pdata->core_data[node_id] = tdata;

	err = create_core_attrs(tdata, pdata->hwmon_dev, node_id);
	if (err)
		goto exit_free;

	return 0;

exit_free:
	pdata->core_data[node_id] = NULL;
	kfree(tdata);
	return err;
}

static struct platform_device *pdev;

static int dms5013a_init(void)
{
	struct device *dev;
	struct platform_data *pdata;
	int ret, node_id;

	if (!is_cpu_ft2000plus())
		return -ENODEV;

	pdev = platform_device_alloc(DRVNAME, -1);
	if (!pdev) {
		pr_err("platform_device_alloc return error!\n");
		return -ENOMEM;
	}

	dev = &pdev->dev;
	pdata = devm_kzalloc(dev, sizeof(struct platform_data), GFP_KERNEL);
	if (!pdata) {
		pr_err("devm_kzalloc return error!\n");
		ret = -ENOMEM;
		goto exit_device_put;
	}

	pdata->temp_base = DMS_BASE_ADDR;

	platform_set_drvdata(pdev, pdata);
	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("platform_device_add return error\n");
		goto exit_device_put;
	}

	pdata->hwmon_dev = devm_hwmon_device_register_with_groups(dev, DRVNAME,
								  pdata, NULL);
	ret = PTR_ERR_OR_ZERO(pdata->hwmon_dev);
	if (ret) {
		pr_err("devm_hwmon_device_register_with_groups return error!\n");
		goto exit_device_put;
	}

	for_each_online_node(node_id)
		create_core_data(pdev, node_id);

	pr_debug("PHYTIUM CPU sensors driver registered!\n");
	return 0;

exit_device_put:
	platform_device_unregister(pdev);
	pdev = NULL;

	return ret;
}

static void dms5013a_remove_core(struct platform_data *pdata,
				 int index)
{
	struct temp_data *tdata = pdata->core_data[index];

	/* Remove the sysfs attributes */
	sysfs_remove_group(&pdata->hwmon_dev->kobj, &tdata->attr_group);

	iounmap(tdata->base_addr);
	iounmap(tdata->data_addr);

	kfree(pdata->core_data[index]);
	pdata->core_data[index] = NULL;
}

static void dms5013a_exit(void)
{
	struct platform_data *pdata;
	int i;

	if (!pdev)
		return;

	pdata = platform_get_drvdata(pdev);
	for_each_online_node(i)
		if (pdata->core_data[i])
			dms5013a_remove_core(pdata, i);

	platform_device_unregister(pdev);
}

module_init(dms5013a_init)
module_exit(dms5013a_exit)

MODULE_AUTHOR("Jackie Liu <liuyun01@kylinos.cn>");
MODULE_AUTHOR("Zhengyuan Liu <liuzhengyuan@kylinos.cn>");
MODULE_DESCRIPTION("Moortec Embedded Temperature Sensor Monitor");
MODULE_LICENSE("GPL");
