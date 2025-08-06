// SPDX-License-Identifier: GPL-2.0-only
/*
 * init/osinfo.c
 * Operating System Information Extensions.
 *
 * Authors: Jackie Liu <liuyun01@kylinos.cn>
 * Copyright (C) 2020-2025 KylinSoft Corporation.
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
#include <linux/timekeeping.h>

#define FORMAT_TIME_UNIT(buf, buf_size, pos, value)                      \
	do {                                                             \
		if (value) {                                             \
			int ret = snprintf(buf + pos, buf_size - pos,    \
					   "%llu " #value "%s, ", value, \
					   value > 1 ? "s" : "");        \
			if (ret >= 0 && ret < buf_size - pos) {          \
				pos += ret;                              \
			} else {                                         \
				pos = buf_size;                          \
			}                                                \
		}                                                        \
	} while (0)

/**
 * format_uptime - Formats the system uptime into a human-readable string.
 * @buf: Buffer to store the formatted uptime string.
 * @buf_size: Size of the buffer.
 */
static void format_uptime(char *buf, size_t buf_size)
{
	struct timespec64 boot_time;
	u64 nanoseconds, seconds, minute, hour, day;
	u64 buffer_pos = 0;

	ktime_get_boottime_ts64(&boot_time);

	/* Calculate days, hours, minutes, and seconds */
	day = (u64)boot_time.tv_sec;
	hour = do_div(day, 60 * 60 * 24);
	minute = do_div(hour, 60 * 60);
	seconds = do_div(minute, 60);

	/* Format the uptime string */
	FORMAT_TIME_UNIT(buf, buf_size, buffer_pos, day);
	FORMAT_TIME_UNIT(buf, buf_size, buffer_pos, hour);
	FORMAT_TIME_UNIT(buf, buf_size, buffer_pos, minute);

	/* Calculate and format nanoseconds */
	nanoseconds = (u64)boot_time.tv_nsec;
	do_div(nanoseconds, NSEC_PER_SEC / 100);
	snprintf(buf + buffer_pos, buf_size - buffer_pos, "%llu.%02llu seconds",
		 seconds, nanoseconds);
}

/**
 * print_cpuid_info - Weak function to print CPU ID information.
 * @m: seq_file structure to write the information.
 */
void __weak print_cpuid_info(struct seq_file *m)
{
	/* This function can be overridden by other modules */
}

/**
 * show_osinfo - Show function for the /proc/osinfo file.
 * @m: seq_file structure to write the information.
 * @v: Unused parameter.
 *
 * Returns 0 on success.
 */
static int show_osinfo(struct seq_file *m, void *v)
{
	struct sysinfo sys_info;
	char uptime_buf[256] = { 0 };
	__u64 total_ram;

	si_meminfo(&sys_info);
	format_uptime(uptime_buf, sizeof(uptime_buf));
	total_ram = sys_info.totalram << (PAGE_SHIFT - 10);

	seq_printf(m, "Kernel Version:\t\t%s\n", utsname()->release);
	seq_printf(m, "Source Version:\t\t%s\n", SOURCE_VERSION);
	seq_printf(m, "Upstream Version:\t%d.%d.%d\n", LINUX_VERSION_MAJOR,
		   LINUX_VERSION_PATCHLEVEL, LINUX_VERSION_SUBLEVEL);
	seq_printf(m, "Build Time:\t\t%s\n", utsname()->version);
	seq_printf(m, "Architecture:\t\t%s\n", utsname()->machine);
	seq_printf(m, "Online CPUs:\t\t%d\n", num_online_cpus());
	seq_printf(m, "MemTotal:\t\t%llu.%02llu GB\n", total_ram >> 20,
		   ((total_ram & 0xFFFFF) * 100) >> 20);
	seq_printf(m, "PageSize:\t\t%ld KB\n", PAGE_SIZE >> 10);
	seq_printf(m, "Uptime:\t\t\t%s\n", uptime_buf);
	seq_printf(m, "Cmdline:\t\t%s\n", saved_command_line);

	print_cpuid_info(m);

	return 0;
}

/**
 * proc_osinfo_init - Initialization function for the osinfo module.
 *
 * Returns 0 on success.
 */
static int __init proc_osinfo_init(void)
{
	proc_create_single("osinfo", 0, NULL, show_osinfo);
	return 0;
}
fs_initcall(proc_osinfo_init);

MODULE_AUTHOR("Jackie Liu <liuyun01@kylinos.cn>");
MODULE_DESCRIPTION("Operating System Information Extensions");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");
