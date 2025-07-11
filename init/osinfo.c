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
#include <linux/utsname.h>

static void get_uptime_format_string(char *buf)
{
	struct timespec64 time;
	u64 nseconds, seconds, minutes, hours, days;
	u64 remainder, pos = 0;

	ktime_get_boottime_ts64(&time);

	/* days = tv_sec / (60 * 60 * 24) */
	days = (u64)time.tv_sec;
	remainder = do_div(days, 60 * 60 * 24);

	/* hours = (tc_sec / (60 * 60)) % 24 */
	hours = remainder;
	remainder = do_div(hours, 60 * 60);

	/* minutes = (tv_sec / 60) % 60 */
	minutes = remainder;
	seconds = do_div(minutes, 60);

	if (days) {
		pos += sprintf(buf, "%lld %s, ", days,
			       days > 1 ? "days" : "day");
	}
	if (hours) {
		pos += sprintf(buf + pos, "%lld %s, ", hours,
			       hours > 1 ? "hours" : "hour");
	}
	if (minutes) {
		pos += sprintf(buf + pos, "%lld %s, ", minutes,
			       minutes > 1 ? "minutes" : "minute");
	}

	/* nseconds = tv_nsec / (NSEC_PER_SEC / 100) */
	nseconds = (u64)time.tv_nsec;
	do_div(nseconds, NSEC_PER_SEC / 100);

	sprintf(buf + pos, "%llu.%02llu seconds", seconds, nseconds);
}

void __weak print_cpuid_info(struct seq_file *m)
{
}

static int osinfo_proc_show(struct seq_file *m, void *v)
{
	struct sysinfo i;
	char uptime_buf[256] = {0};
	__u64 totalram;

	si_meminfo(&i);
	get_uptime_format_string(uptime_buf);
	totalram = i.totalram << (PAGE_SHIFT - 10);

	seq_printf(m, "Kernel Version:\t\t%s\n", utsname()->release);
	seq_printf(m, "Source Version:\t\t%s\n", SOURCE_VERSION);
	seq_printf(m, "Upstream Version:\t%d.%d.%d\n", LINUX_VERSION_MAJOR,
		   LINUX_VERSION_PATCHLEVEL,
		   LINUX_VERSION_SUBLEVEL);
	seq_printf(m, "Build Time:\t\t%s\n", utsname()->version);
	seq_printf(m, "Architecture:\t\t%s\n", utsname()->machine);
	seq_printf(m, "Online CPUs:\t\t%d\n", num_online_cpus());
	seq_printf(m, "MemTotal:\t\t%llu.%02llu GB\nPageSize:\t\t%ld KB\n",
		   totalram >> 20, ((totalram & 0xFFFFF) * 100) >> 20,
		   PAGE_SIZE >> 10);
	seq_printf(m, "Uptime:\t\t\t%s\n", uptime_buf);
	seq_printf(m, "Cmdline:\t\t%s\n", saved_command_line);

	print_cpuid_info(m);

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
