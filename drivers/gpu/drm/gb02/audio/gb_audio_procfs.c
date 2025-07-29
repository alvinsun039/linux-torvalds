/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>

#include "gb_audio_procfs.h"
#include "gb02_fpga_v2vdma.h"
#include "i2s_platform.h"
#include "common/gb_common.h"
#include "common/gb_pcie_info.h"

struct proc_dir_entry *audio_proc_parent;

static int GB02FUNC227(struct seq_file *m, void *v)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type;

	gb_type = GB02FUNC503(pcie_info);
	seq_printf(m, "%d\n", gb_type);
	return 0;
}

static int GB02FUNC229(struct inode *inode, struct file *file)
{
	struct GB02STR254 *gbpcie_info;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	gbpcie_info = PDE_DATA(inode);
#else
	gbpcie_info = pde_data(inode);
#endif	
	gb_printf(KERN_INFO, "chen gb open proc--\n");

	return single_open(file, GB02FUNC227, gbpcie_info);

}
static int GB02FUNC231(struct inode *inode, struct file *file)
{
	int res = single_release(inode, file);

	return res;

}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
static const struct file_operations audio_proc_ops = {
	.open = GB02FUNC229,
	.read = seq_read,
	.release = GB02FUNC231,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops audio_proc_ops = {
	.proc_open = GB02FUNC229,
	.proc_read = seq_read,
	.proc_release = GB02FUNC231,
};
#endif

static int GB02FUNC232(struct seq_file *m, void *v)
{
	int i, support_audio = 0;
	struct GB02STR86 *i2s_infos = GB02FUNC505();

	for (i = 0; i < GB02MAC309; i++) {
		support_audio = GB02FUNC532(i2s_infos, i);
		seq_printf(m, "HDMI: %d, edid_audio_support: %d\n", i, support_audio);
	}
	return 0;
}

static int GB02FUNC234(struct inode *inode, struct file *file)
{
	gb_printf(KERN_INFO, "GB02FUNC234 proc--\n");

	return single_open(file, GB02FUNC232, NULL);

}
static int GB02FUNC235(struct inode *inode, struct file *file)
{
	int res = single_release(inode, file);

	return res;

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
static const struct file_operations audio_edid_ops = {
	.open = GB02FUNC234,
	.read = seq_read,
	.release = GB02FUNC235,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops audio_edid_ops = {
	.proc_open = GB02FUNC234,
	.proc_read = seq_read,
	.proc_release = GB02FUNC235,
};
#endif

int GB02FUNC240(void)
{
	struct proc_dir_entry *return_parent;

	gb_printf(KERN_INFO, "gb audio enter proc entry\n");

	remove_proc_subtree("gb02_audio", NULL);
	audio_proc_parent = proc_mkdir("gb02_audio", NULL);
	if (!audio_proc_parent) {
		gb_printf(KERN_ERR, "error creating proc entry\n");
		return -ENOMEM;
	}

	return_parent = proc_create_data("gb02_board_type", 0,
		audio_proc_parent, &audio_proc_ops, NULL);
	if (!return_parent) {
		remove_proc_subtree("gb02_audio", NULL);
		return -ENOMEM;
	}

	return_parent = proc_create_data("EDID_audio", 0,
		audio_proc_parent, &audio_edid_ops, NULL);
	if (!return_parent) {
		remove_proc_subtree("gb02_audio", NULL);
		return -ENOMEM;
	}

	gb_printf(KERN_INFO, "gb audio create proc entry\n");
	return 0;
}

void GB02FUNC248(void)
{
	remove_proc_subtree("gb02_audio", NULL);
}
