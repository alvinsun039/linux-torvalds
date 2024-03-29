// SPDX-License-Identifier: GPL-2.0-only
/*
 * init/osinfo.c
 * Operating System Information Extensions.
 *
 * Authors: Jackie Liu <liuyun01@kylinos.cn>
 * Copyright (C) 2020-2024 KylinSoft Corporation.
 *
 */
#include <generated/utsrelease.h>
#include <generated/uapi/linux/version.h>
#include <generated/compile.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/mm.h>

static void get_uptime_format_string(char *buf)
{
	struct timespec64 time;
	u64 uptime_secs, minutes, hours, days, pos = 0;

	ktime_get_boottime_ts64(&time);
	uptime_secs = (u64)time.tv_sec;

	days = uptime_secs / (60*60*24);
	hours = (uptime_secs / (60*60)) % 24;
	minutes = (uptime_secs / (60)) % 60;

	if (days)
		pos += sprintf(buf, "%lld %s, ", days, days > 1 ? "days" : "day");
	if (hours)
		pos += sprintf(buf + pos, "%lld %s, ", hours, hours > 1 ? "hours" : "hour");
	if (minutes)
		pos += sprintf(buf + pos, "%lld %s, ", minutes, minutes > 1 ? "minutes" : "minute");

	sprintf(buf + pos, "%llu.%02lu seconds", uptime_secs % 60,
		(time.tv_nsec / (NSEC_PER_SEC / 100)));
}

static int osinfo_proc_show(struct seq_file *m, void *v)
{
	struct sysinfo i;
	char uptime_buf[256] = {0};
	__u64 totalram;

	si_meminfo(&i);
	get_uptime_format_string(uptime_buf);
	totalram = i.totalram << (PAGE_SHIFT - 10);

	seq_printf(m, "Kernel Version:\t\t%s\n", UTS_RELEASE);
	seq_printf(m, "Upstream Version:\t%d.%d.%d\n", LINUX_VERSION_MAJOR,
		   LINUX_VERSION_PATCHLEVEL,
		   LINUX_VERSION_SUBLEVEL);
	seq_printf(m, "Build Time:\t\t%s\n", UTS_VERSION);
	seq_printf(m, "Architecture:\t\t%s\n", UTS_MACHINE);
	seq_printf(m, "Online CPUs:\t\t%d\n", num_online_cpus());
	seq_printf(m, "MemTotal:\t\t%llu.%02llu GB\nPageSize:\t\t%ld KB\n",
		   totalram >> 20, ((totalram & 0xFFFFF) * 100) >> 20,
		   PAGE_SIZE >> 10);
	seq_printf(m, "Uptime:\t\t\t%s\n", uptime_buf);
	seq_printf(m, "Cmdline:\t\t%s\n", saved_command_line);

	return 0;
}

static int __init proc_osinfo_init(void)
{
	proc_create_single("osinfo", 0, NULL, osinfo_proc_show);

	return 0;
}
fs_initcall(proc_osinfo_init);

MODULE_AUTHOR("Jackie Liu <liuyun01@kylinos.cn>");
MODULE_DESCRIPTION("Operating System Information Extensions");
MODULE_LICENSE("GPL");
