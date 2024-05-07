// SPDX-License-Identifier: GPL-2.0-only
/*
 * Author: Jackie Liu <liuyun01@kylinos.cn>
 * Copyright (C) 2024, KylinSoft Corporation.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>

#ifdef CONFIG_KYLIN_DIFFERENCES

const char *logo_string[] = {
	"==============================================================================\n",
	" \n",
	"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡀\n",
	"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⢺⠠⣜⣺⠁\n",
	"⠀⠀⠀⠀⠀⠀⠀⠀⡠⡰⣪⢣⣫⡶⣦\n",
	"⠀⠀⠀⠀⠀⠀⡲⣝⢜⡎⢔⢝⣝⡻⡽⡆\n",
	"⠀⠀⠀⠀⠀⢬⢎⢎⢧⢳⢱⡱⠪⣾⠞\n",
	"⠀⠀⠀⠀⡌⡖⡭⡳⡵⣽⡲⣜⢬⢁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡀⡖⣿⢕⠀⠀⠀⢸⣿⠀⠀⢀⣴⠞⠋⠀⠀⠙⢿⣿⣆⠀⠀⢀⡼⠁⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⣿⣿⡇⠀⣿⠹⣿⣆⠀⠀⠀⢸⡇\n",
	"⠀⠀⠀⡪⡣⡫⣪⣷⣟⣯⡿⡜⡎⡇⡗⡄⡀⠀⠀⠀⠀⠀⠀⠀⠀⢤⢳⢙⢜⠕⠀⠀⠀⢸⣿⣠⣴⢿⣷⣄⠀⠀⠀⠀⠀⠹⣿⣦⣴⠟⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⣿⣿⡇⠀⣿⠀⠈⣿⣷⣄⠀⢸⡇\n",
	"⠀⠀⠀⠐⠔⢅⢧⡣⡇⡮⡪⣷⣽⣪⣫⢺⣺⢿⣽⢶⢤⢀⡰⡸⠜⠈⠈⠀⠁⠀⠀⠀⠀⢸⣿⠁⠀⠀⠻⣿⣷⣄⠀⠀⠀⠀⢸⣿⡇⠀⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⣿⣿⡇⠀⣿⠀⠀⠀⠹⣿⣷⣼⡇\n",
	"⠀⠀⠀⠀⢁⢗⢕⡇⡯⣪⡫⡿⡺⠯⡛⡓⣪⢫⢮⢳⢝⢜⠪⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠚⠛⠀⠀⠀⠀⠘⠛⠛⠂⠀⠀⠀⠘⠛⠃⠀⠀⠀⠀⠛⠛⠛⠛⠛⠛⠛⠓⠀⠛⠛⠃⠀⠛⠀⠀⠀⠀⠈⠛⠛⠃\n",
	"⠀⠀⠀⠀⠀⡝⡅⡣⠱⣐⢼⢱⢢⣑⠨⠈⠊⠪⡘⣔⣝⢦⢣⡂⠀⠀⠀⠀⠀⠀⠀⠀⠀⣤⣤⣤⣤⣠⣤⣤⣤⣤⡄⠀⣤⣤⣤⡄⣤⣤⣤⣤⣤⣤⠀⢠⣤⣤⣤⡄⡄⠀⠀⢠⡄⠀⣤⣤⣤⣤⡄⢤⡠⣄⡤\n",
	"⠀⠀⠀⠀⠀⢉⠪⠠⠑⠘⠑⠙⠈⠀⠀⠀⠀⢀⠌⡎⡖⡕⠇⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⣥⣤⣤⡄⣤⣤⣤⣤⣤⡇⠀⠀⠀⠀⠀⣤⠤⠤⣤⢸⡇⠀⠐⣶⢲⠒⠒⡗⠒⣲⢾⡇⠀⣶⣲⣖⣒⡒⢒⡿⣿⡒\n",
	"⠀⠀⠀⠀⠀⠀⡝⠌⠀⠀⠀⠀⠀⠀⠀⢀⢂⠅⠁⠀⠈⡘⡌⡢⠂⠀⠀⠀⠀⠀⠀⠀⠀⢺⠒⠒⠂⣶⠒⠒⠒⠒⠃⠀⠛⠛⡟⠁⣿⠀⠀⣿⢸⡇⠀⢠⣿⣼⣼⡇⡟⣉⡴⢻⡇⠀⣿⢼⡧⠼⣧⠞⢾⣷⣦⠄\n",
	"⠀⠀⠀⠀⢀⠨⢌⠀⠀⠀⠀⠀⠀⠀⠠⠂⠢⠁⠀⠀⡡⢈⠂⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⠀⠀⣿⣤⠞⠳⣄⠀⠀⢀⡾⠃⠀⣿⣀⣀⣿⢸⡇⠀⠀⣿⢾⠶⠶⡿⠷⠶⢾⡷⠀⣿⢸⣗⠞⣹⠞⠫⢿⡷⠄\n",
	"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠊⠐⠐⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠁⠋⠀⠀⠀⠀⠑⠀⠋⠀⠀⠀⠀⠀⠀⠀⠈⠃⠀⠀⠛⠛⠛⠛⠃⠀⠀⠈⠃⠀⠛⠈⠀⠋⠀⠀⠀⠘⠃\n",
	" \n",
	" \n",
};

/* Display a KYLIN logo on the console screen */
static int __init kylin_ascii_logo(char *str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(logo_string); i++)
		pr_info("%s", logo_string[i]);

	return 1;
}
__setup("asciilogo", kylin_ascii_logo);

static int kylin_ascii_logo_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(logo_string); i++)
		seq_printf(m, "%s", logo_string[i]);

	return 0;
}

static int kylin_ascii_logo_open(struct inode *inode, struct file *file)
{
	return single_open(file, kylin_ascii_logo_show, NULL);
}

static const struct proc_ops kylin_ascii_logo_fops = {
	.proc_open	= kylin_ascii_logo_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int __init proc_ascii_logo_init(void)
{
	struct proc_dir_entry *pde;

	pde = proc_mkdir("debug", NULL);
	if (!pde)
		goto err1;

	pde = proc_create("debug/ascii_logo", 0, NULL, &kylin_ascii_logo_fops);
	if (!pde)
		goto err2;

	return 0;

err2:
	remove_proc_entry("debug", NULL);
err1:
	return -ENOMEM;
}
fs_initcall(proc_ascii_logo_init);

static void __exit proc_ascii_logo_exit(void)
{
	remove_proc_entry("debug/ascii_logo", NULL);
	remove_proc_entry("debug", NULL);
}
module_exit(proc_ascii_logo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jackie Liu <liuyun01@kylinos.cn>");

#endif
