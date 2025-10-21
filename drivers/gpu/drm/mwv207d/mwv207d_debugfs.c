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
#include <linux/debugfs.h>
#include <linux/vmalloc.h>

#include "mwv207d_bo.h"
#include "mwv207d_vbios.h"

#ifdef CONFIG_DEBUG_FS

struct debugfs_entry {
	const char *name;
	const struct file_operations *fops;
};

static int mwv207d_debugfs_vram_size_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%llu\n", mdev->hw.vram_size);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_vram_size);

static int mwv207d_debugfs_gtt_size_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%llu\n", mdev->gtt_size);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_gtt_size);

static int mwv207d_debugfs_moved_bytes_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%llu\n", atomic64_read(&mdev->moved_bytes));

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_moved_bytes);

static int mwv207d_debugfs_vram_used_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%llu\n", atomic64_read(&mdev->vram_used_bytes));

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_vram_used);

static int mwv207d_debugfs_visible_vram_used_show(struct seq_file *m,
						  void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%llu\n",
		   atomic64_read(&mdev->visible_vram_used_bytes));

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_visible_vram_used);

static int mwv207d_debugfs_gtt_used_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%llu\n", atomic64_read(&mdev->gtt_used_bytes));

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_gtt_used);

static int mwv207d_debugfs_vram_pinned_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%llu\n", mdev->vram_pinned_bytes);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_vram_pinned);

static int mwv207d_debugfs_gtt_pinned_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%llu\n", mdev->gtt_pinned_bytes);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_gtt_pinned);

static int mwv207d_debugfs_3d_clusters_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%d\n", mdev->hw.nr_3d_clusters);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_3d_clusters);

static int mwv207d_debugfs_2d_clusters_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%d\n", mdev->hw.nr_2d_clusters);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_2d_clusters);

static int mwv207d_debugfs_fus_clusters_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%d\n", mdev->hw.nr_fus_clusters);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_fus_clusters);

static int mwv207d_debugfs_num_dec_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%d\n", mdev->hw.nr_pipe[0x1]);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_num_dec);

static int mwv207d_debugfs_num_enc_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;

	seq_printf(m, "%d\n", mdev->hw.nr_pipe[0x2]);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_num_enc);

static int mwv207d_debugfs_vbios_log_show(struct seq_file *m, void *unused)
{
	struct mwv207d_device *mdev = m->private;
	int read_size;
	char *buf;

	buf = vmalloc((1024*5-4));
	if (!buf)
		return -ENOMEM;

	read_size = mwv207d_read_vlog(mdev, buf, (1024*5-4));
	if (!read_size)
		goto out;

	seq_printf(m, "%.*s\n", read_size, buf);

out:
	vfree(buf);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mwv207d_debugfs_vbios_log);

static const struct debugfs_entry debugfs_array[] = {
	{ "vram_size", &mwv207d_debugfs_vram_size_fops },
	{ "gtt_size", &mwv207d_debugfs_gtt_size_fops },
	{ "moved_byte", &mwv207d_debugfs_moved_bytes_fops },
	{ "vram_used_bytes", &mwv207d_debugfs_vram_used_fops },
	{ "visible_vram_used_bytes", &mwv207d_debugfs_visible_vram_used_fops },
	{ "gtt_used_bytes", &mwv207d_debugfs_gtt_used_fops },
	{ "vram_pinned_bytes", &mwv207d_debugfs_vram_pinned_fops },
	{ "gtt_pinned_bytes", &mwv207d_debugfs_gtt_pinned_fops },
	{ "3d_clusters", &mwv207d_debugfs_3d_clusters_fops },
	{ "2d_clusters", &mwv207d_debugfs_2d_clusters_fops },
	{ "fus_clusters", &mwv207d_debugfs_fus_clusters_fops },
	{ "num_dec", &mwv207d_debugfs_num_dec_fops },
	{ "num_enc", &mwv207d_debugfs_num_enc_fops },
};

void mwv207d_debugfs_init(struct mwv207d_device *mdev)
{
	struct dentry *root = mdev->base.primary->debugfs_root;
	struct dentry *ent;
	int i;

	for (i = 0; i < ARRAY_SIZE(debugfs_array); i++) {
		ent = debugfs_create_file(debugfs_array[i].name, 0444,
					  root, mdev, debugfs_array[i].fops);
		if (IS_ERR(ent))
			DRM_ERROR("unable to create debugsfs file：%s\n", debugfs_array[i].name);
	}

	if (mdev->hw.is_pf) {
		ent = debugfs_create_file("vbios_log", 0444, root, mdev,
					  &mwv207d_debugfs_vbios_log_fops);
		if (IS_ERR(ent))
			DRM_ERROR("unable to create debugsfs file: vlog data\n");
	}
}

#endif
