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
#include <drm/drm_modes.h>
#include "common/gb_common.h"
#include "kms/gb_pcie_map.h"
#include "kms/device/gb_ip.h"
#include "common/gb_procfs.h"
#include "ip/gb_dp.h"

/* version format(3 sections): xx.xx.xx (major.minor.build). e.g., v0.1.20 */
#define DIGIT_TOTAL_NUM    6  /* version data in register. e.g., 0x000120 */
#define GB02MAC868   2  /* number of hex digit in each section */
#define GB02MAC869    3
#define GB02MAC870    4
#define GB02MAC871 10

struct proc_dir_entry *proc_parent;

static int GB02FUNC541(uint32_t reg_val, char* version)
{
	uint8_t sect_arr[GB02MAC869] = {0, 0, 0};
	uint8_t sect = 0;
	int i,j = 0;
	if(version==NULL) {
		return -1;
	}
	for(i=0; i < DIGIT_TOTAL_NUM; i++) {
		/*get 2-bit dec digits from 2-bit hex digits of each section*/
		if(i%GB02MAC868) {
			sect = sect + ((reg_val>>(i*GB02MAC870))&0xf) * 10;
			sect_arr[j++] = sect;
		} else {
			sect = 0;
			sect = ((reg_val>>(i*GB02MAC870))&0xf);
		}
	}
	sprintf(version, "%d.%d.%d", sect_arr[2], sect_arr[1], sect_arr[0]);
	gb_printf(KERN_INFO, "verr:%s\n", version);
	return 0;
}

static int GB02FUNC545(struct seq_file *m, void *v)
{
	char mcu_version[GB02MAC871] = {0,};
	char bios_version[GB02MAC871] = {0,};
	struct GB02STR117 gb_version;
	struct GB02STR254 *gbpcie_info = m->private;
	void __iomem *pcie_base = gbpcie_info->pci_mmio_bar[0];
	struct GB02STR70 *gb_pcie = GB02FUNC518();
	iowrite32(0x60000000, gbpcie_info->pci_mmio_bar[4] + 0xd0c);
	gbpcie_info->gb_cops->get_version(pcie_base, &gb_version);
	GB02FUNC541(gb_version.mcu_vid, mcu_version);
	GB02FUNC541(gb_version.bios_vid, bios_version);
	GB02FUNC1041(gb_pcie);
	seq_printf(m, "driver\t: %s\n", STR2(GB_VERSION));
	seq_printf(m, "vbios\t: V%s-%x\n", mcu_version, gb_version.mcu_vdata);
	seq_printf(m, "option rom\t: V%s-%x\n", bios_version, gb_version.bios_vdata);
      if (gb_pcie->f_info.version_info & GB02MAC1595)
                seq_printf(m, "HDMI firmware\t: V%x.%x\n",
                           gb_pcie->f_info.hdmi_firmware_maj,
                           gb_pcie->f_info.hdmi_firmware_min);
        if (gb_pcie->f_info.version_info & GB02MAC1596)
                seq_printf(m, "VGA firmware\t: V%x.%x\n",
                           gb_pcie->f_info.vga_firmware_maj,
                           gb_pcie->f_info.vga_firmware_min);
	return 0;
}

static int GB02FUNC550(struct inode *inode, struct file *file)
{
	struct GB02STR254 *gbpcie_info;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	gbpcie_info = PDE_DATA(inode);
#else
	gbpcie_info = pde_data(inode);
#endif
	gb_printf(KERN_INFO, "chen gb open proc--\n");

	return single_open(file, GB02FUNC545, gbpcie_info);

}
static int GB02FUNC551(struct inode *inode, struct file *file)
{
	int res = single_release(inode, file);
	return res;

}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
static const struct file_operations exports_proc_ops = {
	.open = GB02FUNC550,
	.read = seq_read,
	.release=GB02FUNC551,
	.owner= THIS_MODULE,
};
#else
static const struct proc_ops exports_proc_ops = {
	.proc_open = GB02FUNC550,
	.proc_read = seq_read,
	.proc_release = GB02FUNC551,
};
#endif

int GB02FUNC552(struct GB02STR254 *gbpcie_info)
{
	gb_printf(KERN_INFO, "gb enter proc entry-\n");

	remove_proc_subtree("gb", NULL);
	proc_parent = proc_mkdir("gb", NULL);
	if(!proc_parent){
		gb_printf(KERN_ERR, "error creating proc entry\n");
		return -ENOMEM;
	}

	proc_parent = proc_create_data("version", 0, proc_parent, &exports_proc_ops, gbpcie_info);
	if(!proc_parent){
		remove_proc_subtree("gb", NULL);
		return -ENOMEM;
	}

	gb_printf(KERN_INFO, "gb create proc entry-\n");
	return 0;
}

void GB02FUNC557(void)
{
	remove_proc_subtree("gb", NULL);
}
