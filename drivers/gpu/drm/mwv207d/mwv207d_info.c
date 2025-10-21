/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include "mwv207d_drv.h"
#include "mwv207d_debugfs.h"
#include "mwv207d_sched.h"
#include "mwv207d_vbios.h"

static int ddr_monitor_duration = 200;

static ssize_t fb_memory_total_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", mdev->hw.vram_size);
}
static DEVICE_ATTR_RO(fb_memory_total);

static ssize_t fb_memory_used_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", atomic64_read(&mdev->vram_used_bytes));
}
static DEVICE_ATTR_RO(fb_memory_used);

static ssize_t bar_memory_total_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", mdev->visible_vram_size);
}
static DEVICE_ATTR_RO(bar_memory_total);

static ssize_t bar_memory_used_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n",
		       atomic64_read(&mdev->visible_vram_used_bytes));
}
static DEVICE_ATTR_RO(bar_memory_used);

static ssize_t gtt_memory_total_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", mdev->gtt_size);
}
static DEVICE_ATTR_RO(gtt_memory_total);

static ssize_t gtt_memory_used_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", atomic64_read(&mdev->gtt_used_bytes));
}
static DEVICE_ATTR_RO(gtt_memory_used);

static ssize_t fb_memory_pinned_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", mdev->vram_pinned_bytes);
}
static DEVICE_ATTR_RO(fb_memory_pinned);

static ssize_t gtt_memory_pinned_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", mdev->gtt_pinned_bytes);
}
static DEVICE_ATTR_RO(gtt_memory_pinned);

static ssize_t memory_moved_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%llu\n", atomic64_read(&mdev->moved_bytes));
}
static DEVICE_ATTR_RO(memory_moved);

static ssize_t gpu_mode_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	int mode = mdev->hw.nr_3d_clusters;

	if (mdev->hw.is_pf)
		mode = 0;

	return sprintf(buf, "%d\n", mode);
}
static DEVICE_ATTR_RO(gpu_mode);

static ssize_t usage_2d0_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 usage = mwv207d_pipe_get_usage(mdev, 0x3, 0);

	return sprintf(buf, "%d\n", usage);
}
static DEVICE_ATTR_RO(usage_2d0);

static ssize_t usage_3d0_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 usage = mwv207d_pipe_get_usage(mdev, 0x0, 0);

	return sprintf(buf, "%d\n", usage);
}
static DEVICE_ATTR_RO(usage_3d0);

static ssize_t usage_fus0_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 usage = mwv207d_pipe_get_usage(mdev, 0x5, 0);

	return sprintf(buf, "%d\n", usage);
}
static DEVICE_ATTR_RO(usage_fus0);

static ssize_t usage_enc0_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 usage = mwv207d_pipe_get_usage(mdev, 0x2, 0);

	return sprintf(buf, "%d\n", usage);
}
static DEVICE_ATTR_RO(usage_enc0);

static ssize_t usage_dec0_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 usage = mwv207d_pipe_get_usage(mdev, 0x1, 0);

	return sprintf(buf, "%d\n", usage);
}
static DEVICE_ATTR_RO(usage_dec0);

static ssize_t usage_enc1_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 usage = mwv207d_pipe_get_usage(mdev, 0x2, 1);

	return sprintf(buf, "%d\n", usage);
}
static DEVICE_ATTR_RO(usage_enc1);

static ssize_t usage_dec1_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 usage = mwv207d_pipe_get_usage(mdev, 0x1, 1);

	return sprintf(buf, "%d\n", usage);
}
static DEVICE_ATTR_RO(usage_dec1);

static ssize_t kfreq_3d_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_CORE_3D, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_3d);

static ssize_t kfreq_2d_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_CORE_2D, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_2d);

static ssize_t kfreq_hd_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_CORE_HD, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_hd);

static ssize_t kfreq_fus_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_CORE_FUS, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_fus);

static ssize_t kfreq_ddr0_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_DDR0, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_ddr0);

static ssize_t kfreq_ddr1_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_DDR1, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_ddr1);

static ssize_t kfreq_ddr2_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_DDR2, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_ddr2);

static ssize_t kfreq_ddr3_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_DDR3, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_ddr3);

static ssize_t kfreq_pcie_dm_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 kfreq = 0;
	int ret;

	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_PCIE_DM, &kfreq);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", kfreq);
}
static DEVICE_ATTR_RO(kfreq_pcie_dm);

static ssize_t vbios_version_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", mwv207d_vbios_get_version(mdev));
}
static DEVICE_ATTR_RO(vbios_version);

static ssize_t fan_speed_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", mwv207d_vbios_get_fan_speed(mdev));
}
static DEVICE_ATTR_RO(fan_speed);

static ssize_t temperature_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", mwv207d_vbios_get_temp(mdev));
}
static DEVICE_ATTR_RO(temperature);

static ssize_t power_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{

	return sprintf(buf, "%u\n", 0xffffffffu);
}
static DEVICE_ATTR_RO(power);

static ssize_t voltage_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{

	return sprintf(buf, "%u\n", 0xffffffffu);
}
static DEVICE_ATTR_RO(voltage);

static ssize_t ddr_bandwidth_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	u32 rbw, wbw, tbw;
	int ret;

	ret = mwv207d_vbios_get_ddr_bandwidth(mdev, ddr_monitor_duration, &rbw, &wbw, &tbw);
	if (ret)
		return ret;
	return sprintf(buf, "read:%uMBps write:%uMBps total:%uMBps\n",
			rbw, wbw, tbw);
}
static DEVICE_ATTR_RO(ddr_bandwidth);

static ssize_t ddr_bandwidth_duration_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", ddr_monitor_duration);
}

static ssize_t ddr_bandwidth_duration_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	int ret, duration, max;

	ret = kstrtoint(buf, 10, &duration);
	if (ret)
		return ret;

	max = mwv207d_vbios_get_max_duration(mdev);
	if (duration <= 0 || duration > max)
		return -EINVAL;

	ddr_monitor_duration =  duration;

	return count;
}
static DEVICE_ATTR_RW(ddr_bandwidth_duration);

static struct attribute *mwv207d_pf_attrs[] = {
	&dev_attr_fb_memory_total.attr,
	&dev_attr_fb_memory_used.attr,
	&dev_attr_bar_memory_total.attr,
	&dev_attr_bar_memory_used.attr,
	&dev_attr_usage_2d0.attr,
	&dev_attr_usage_3d0.attr,
	&dev_attr_usage_fus0.attr,
	&dev_attr_usage_enc0.attr,
	&dev_attr_usage_dec0.attr,
	&dev_attr_usage_enc1.attr,
	&dev_attr_usage_dec1.attr,
	&dev_attr_gtt_memory_total.attr,
	&dev_attr_gtt_memory_used.attr,
	&dev_attr_fb_memory_pinned.attr,
	&dev_attr_gtt_memory_pinned.attr,
	&dev_attr_memory_moved.attr,
	&dev_attr_gpu_mode.attr,
	&dev_attr_vbios_version.attr,
	&dev_attr_fan_speed.attr,
	&dev_attr_temperature.attr,
	&dev_attr_power.attr,
	&dev_attr_voltage.attr,
	&dev_attr_kfreq_3d.attr,
	&dev_attr_kfreq_2d.attr,
	&dev_attr_kfreq_hd.attr,
	&dev_attr_kfreq_fus.attr,
	&dev_attr_kfreq_ddr0.attr,
	&dev_attr_kfreq_ddr1.attr,
	&dev_attr_kfreq_ddr2.attr,
	&dev_attr_kfreq_ddr3.attr,
	&dev_attr_kfreq_pcie_dm.attr,
	&dev_attr_ddr_bandwidth.attr,
	&dev_attr_ddr_bandwidth_duration.attr,
	NULL,
};

static struct attribute *mwv207d_vf_attrs[] = {
	&dev_attr_fb_memory_total.attr,
	&dev_attr_fb_memory_used.attr,
	&dev_attr_bar_memory_total.attr,
	&dev_attr_bar_memory_used.attr,
	&dev_attr_usage_2d0.attr,
	&dev_attr_usage_3d0.attr,
	&dev_attr_usage_fus0.attr,
	&dev_attr_usage_enc0.attr,
	&dev_attr_usage_dec0.attr,
	&dev_attr_usage_enc1.attr,
	&dev_attr_usage_dec1.attr,
	&dev_attr_gtt_memory_total.attr,
	&dev_attr_gtt_memory_used.attr,
	&dev_attr_fb_memory_pinned.attr,
	&dev_attr_gtt_memory_pinned.attr,
	&dev_attr_memory_moved.attr,
	&dev_attr_gpu_mode.attr,
	NULL,
};

static const struct attribute_group mwv207d_pf_group = {
	.name  = "mwv207d_hwmon",
	.attrs = mwv207d_pf_attrs,
};

static const struct attribute_group *mwv207d_pf_groups[] = {
	&mwv207d_pf_group,
	NULL,
};
static const struct attribute_group mwv207d_vf_group = {
	.name  = "mwv207d_hwmon",
	.attrs = mwv207d_vf_attrs,
};

static const struct attribute_group *mwv207d_vf_groups[] = {
	&mwv207d_vf_group,
	NULL,
};

int mwv207d_sysfs_init(struct mwv207d_device *mdev)
{
	int ret;

	ret = sysfs_create_groups(&mdev->dev->kobj, mdev->hw.is_pf ?
						    mwv207d_pf_groups :
						    mwv207d_vf_groups);
	if (ret) {
		dev_err(mdev->dev, "failed to create sysfs groups");
		return ret;
	}
	return 0;
}

void mwv207d_sysfs_fini(struct mwv207d_device *mdev)
{
	sysfs_remove_groups(&mdev->dev->kobj, mdev->hw.is_pf ?
					      mwv207d_pf_groups :
					      mwv207d_vf_groups);
}
