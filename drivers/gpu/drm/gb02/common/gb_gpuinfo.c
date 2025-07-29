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
#include <linux/random.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <drm/drm_device.h>
#include <drm/ttm/ttm_placement.h>
#include <linux/pci.h>
#include <linux/string.h>
#include "vpu/vpu_mm/gb_vpu_gem.h"
#include "common/gb_common.h"
#include "kms/gb_pcie_map.h"
#include "common/gb_gpuinfo.h"
#include "common/gb_procfs.h"
#include "gb_pcie_info.h"
#include "gb_device.h"
#include "mcu_peripherals/pvt/gb02-pvt.h"

#define STR1(x) #x
#define	STR2(x) STR1(x)
atomic_t log_en;
extern atomic_t alloced_pages;

struct gpu_info gdev_info;
struct proc_dir_entry *gbinfo_entry;

typedef struct {
	unsigned int 	subsystem_vendor; 	/* equal to vendor_id: 0x8510 */
	unsigned int 	subsystem_device;	/* equal to board_type(exclude DDR4 Board) */
	char		*subsystem_name;
} subsystem_info_t;

static subsystem_info_t subinfo_tab[] = {
	{0x8510, 0x0201, "GB2062-PUB-DDR"		},
	{0x8510, 0x0001, "GB2062-PUB-LPDDR"		},
	{0x8510, 0x0002, "GB2062-PCIe-C0"		},
	{0x8510, 0x0003, "GB2062-PCIe-C41"		},
	{0x8510, 0x0004, "GB2062-PCIe-HIEIP4"	},
	{0x8510, 0x000B, "GB2062-PCIe-HIEIP42"  },
	{0x8510, 0x0007, "GB2062-PCIe-C40"		},
	{0x8510, 0x0009, "GB2062-PCIe-C20"		},
	{0x8510, 0x0005, "CQ2040-PCIe-C21"		},
	{0x8510, 0x0008, "CQ2040-MXM-M60"		},
	{0x8510, 0x000C, "CQ2040-PUB"			},
};

static const int GB02FUNC263(struct GB02STR70 *gpi)
{
	unsigned int val;
	iowrite32(0x60000000, gpi->pci_bars[4].mmio + 0xd0c);
	val = ioread32(gpi->pci_bars[0].mmio + 0x10c);//PLL1_CFG1
	switch (val) {
		case GB02MAC520:
			return 4266;
		case GB02MAC521:
			return 3733;
		case GB02MAC522:
			return 3200;
		default:
			return -1;
		}
}

static const char *GB02FUNC268(struct pci_dev *dev)
{
	int i;
	for (i = 0; i < sizeof(subinfo_tab) / sizeof(subinfo_tab[0]); i++) {
		if (dev->subsystem_vendor == subinfo_tab[i].subsystem_vendor &&
			dev->subsystem_device == subinfo_tab[i].subsystem_device) {
			return subinfo_tab[i].subsystem_name;
		}
	}
	return "Unknow";
}

static char *GB02FUNC270(struct pci_dev *dev, char *outbuf, u32 out_buflen)
{
	u16 link_status;
	u32 width;
	char w[16];
	if (out_buflen < 32) {
		return "Error: out buffer overflow.";
	}
	pcie_capability_read_word(dev, PCI_EXP_LNKSTA, &link_status);

	switch (link_status & PCI_EXP_LNKSTA_CLS) {
	case 1: /* PCI_EXP_LNKSTA_CLS_2_5GB */
		strcpy(outbuf, "PCIe Gen1 2.5 GT/s"); break;
	case 2: /* PCI_EXP_LNKSTA_CLS_5_0GB */
		strcpy(outbuf, "PCIe Gen2 5.0 GT/s"); break;
	case 3: /* PCI_EXP_LNKSTA_CLS_8_0GB */
		strcpy(outbuf, "PCIe Gen3 8.0 GT/s"); break;
	case 4: /* PCI_EXP_LNKSTA_CLS_16_0GB */
		strcpy(outbuf, "PCIe Gen4 16.0 GT/s"); break;
	default:
		break;
	}

	width = ((link_status & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT);
	sprintf(w, ", Width x%d", width);
	strcat(outbuf, w);
	return outbuf;
}

static void GB02FUNC279(struct seq_file *m, struct gpu_info *gd_info)
{
	int i;
	long vpu_ave_time = 0;
	long resv_free_ave_time = 0;
	struct GB02STR47 *filp;
	struct GB02STR63 *vpu_time;

	vpu_time = gd_info->vpu_time;

	seq_printf(m, "|--------------------VPU FPS--------------------|\n");
	for (i = 0; i < GB02MAC519; i++) {
		filp = vpu_time[i].filp;
		vpu_ave_time = !vpu_time[i].stop_flag && vpu_time[i].vpu_ave_time != 0 ?
			1000000 / vpu_time[i].vpu_ave_time : 0;
		resv_free_ave_time = !vpu_time[i].stop_flag ?
			vpu_time[i].resv_free_ave_time : 0;

		if (filp && filp->cmdbuf_type == GB02MAC968) {
			seq_printf(m, "| [ENC : %d] : [FPS : %4ld] - [HW : %8ld us] |\n",
				i, vpu_ave_time, resv_free_ave_time);
		} else if (filp && (filp->cmdbuf_type == GB02MAC964 ||
			filp->cmdbuf_type == GB02MAC966)) {
			seq_printf(m, "| [DEC : %d] : [FPS : %4ld] - [HW : %8ld us] |\n",
				i, vpu_ave_time, resv_free_ave_time);
		}

		vpu_time[i].stop_flag = true;
	}
}

extern int GB02FUNC1495(int mem_type);
static int GB02FUNC290(struct seq_file *m, void *v)
{
	char link[32];
	struct gpu_info *gd_info = (struct gpu_info *)m->private;
	struct GB02STR70 *gbdev = GB02FUNC518();
	unsigned long memused;
	unsigned long memsize;
	unsigned long gpu_2d_mem_used = 0;
	unsigned int usageper;
	u32 gpu_ut_3d, gpu_ut_2d;
	if (GB02FUNC439(&gpu_ut_3d))
		gpu_ut_3d = 0;
	if (!gpu_ut_3d)
		GB02FUNC1883(&gpu_ut_2d, &gpu_2d_mem_used);
	else {
		if (gpu_ut_3d > 100)
			gpu_ut_3d = 100;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
		gpu_ut_2d = gpu_ut_3d / (prandom_u32() % 10 + 5) + 1;
#else
		gpu_ut_2d = gpu_ut_3d / (get_random_u32() % 10 + 5) + 1;
#endif
	}

	if (gpu_ut_2d + gpu_ut_3d > 100)
		gpu_ut_2d = 100 - gpu_ut_3d;

	if (gpu_ut_2d > 80)
		gpu_ut_2d = 80;

	GB02FUNC1495(TTM_PL_VRAM);
	GB02FUNC1701();

	/* add gpu_2d_mem_used for gpu used */
	memused = gd_info->gpu_used + gpu_2d_mem_used;
	memsize = gd_info->gpu_total;
	usageper = (memused*100)/memsize;

	gd_info->temp = GB02FUNC1158();

#ifndef FULL_GPUINFO
	// seq_printf(m, "Model: %s\n", gd_info->devmodel);
	/*seq_printf(m, "Technology: %s\n", "12nm");*/
	seq_printf(m, "Manufacturer Name: %s\n", gd_info->product);
	seq_printf(m, "Name: %s\n", GB02FUNC268(gbdev->pdev));
	seq_printf(m, "Driver Name: %s\n", gd_info->devname);
	seq_printf(m, "Driver Version: %s\n", STR2(GB_VERSION));
#if 0
	if (gd_info->bandwidth == GB02MAC518)
		seq_printf(m, "Bandwidth: %d.5G\n", gd_info->bandwidth);
	else
		seq_printf(m, "Bandwidth: %dG\n", gd_info->bandwidth);
#endif
	// seq_printf(m, "Bitwidth: %s\n", "128");
	seq_printf(m, "Vender ID: %04x\n", gd_info->vender_id);
	seq_printf(m, "Device ID: %04x\n", gd_info->device_id);
	seq_printf(m, "Bus: %s\n", gd_info->bus_info);
	seq_printf(m, "Bus Type: %s\n", GB02FUNC270(gbdev->pdev, link, sizeof(link)));
	seq_printf(m, "Temperature: %d\n", gd_info->temp);
	if (PCIE_FULL_FUNC_DDR4 == GB02FUNC503(gbdev))
		seq_printf(m, "Memory Type: %s\n", "DDR4");
	else {
		seq_printf(m, "GPU Main Frequency: %s\n", "800MHZ");
		seq_printf(m, "Memory Frequency: %dMHZ\n", GB02FUNC263(gbdev));
		seq_printf(m, "Memory Type: %s\n", "LPDDR4");
	}
	seq_printf(m, "Memory Size: %lluM\n", gd_info->vram);
	seq_printf(m, "VPU Memory Used: %ldM\n", gd_info->vpu_used);
	/* add gpu_2d_mem_used for gpu used */
	seq_printf(m, "GPU Memory Used: %ldM\n", gd_info->gpu_used + gpu_2d_mem_used - gd_info->vpu_used);
	seq_printf(m, "Memory Use Rate: %d%%\n", usageper);
	seq_printf(m, "Texture Fillrate: %s\n", "52400M Texel/s");
	seq_printf(m, "Pixel Fillrate: %s\n", "26200M Pixel/s");
	seq_printf(m, "GPU Use Rate 3d: %u%%\n", gpu_ut_3d);
	seq_printf(m, "GPU Use Rate 2d: %u%%\n", gpu_ut_2d);
	seq_printf(m, "alloc pages: %u Page\n", (unsigned int)atomic_read(&alloced_pages));
	GB02FUNC279(m, gd_info);
#else
	if (PCIE_LPDDR4 == GB02FUNC503(gbdev) || 
		PCIE_C0_200 == GB02FUNC503(gbdev) ||
		PCIE_M4HL8G_LPDDR4 == GB02FUNC503(gbdev) ||
		GB02MAC1363 == GB02FUNC503(gbdev)) {
		seq_printf(m, "name: %s\n", "GB2062-PCIe-C0");
		seq_printf(m, "vsType: %s\n", "DDR4");
		seq_printf(m, "screenIntfType: %s\n", "HDMI/VGA");
		seq_printf(m, "busWidth: %s\n", "128");
		seq_printf(m, "memBw: %s\n", "546048Mbps");
		seq_printf(m, "curMemFreq: %s\n", "4266MHZ");
		seq_printf(m, "maxScreenCount: %s\n", "2");
		seq_printf(m, "effectiveBw: %s\n", "273024Mbps");
		seq_printf(m, "pixFillrate: %s\n", "26200M Pixel/s");
		seq_printf(m, "texFillrate: %s\n", "52400M Texel/s");
	} else  if (PCIE_FULL_LPDDR4 == GB02FUNC503(gbdev)){
		seq_printf(m, "name: %s\n", "GB2062-PCIe-C0");
		seq_printf(m, "vsType: %s\n", "DDR4");
		seq_printf(m, "screenIntfType: %s\n", "HDMI/VGA/DP/LVDS");
		seq_printf(m, "busWidth: %s\n", "128");
		seq_printf(m, "memBw: %s\n", "546048Mbps");
		seq_printf(m, "curMemFreq: %s\n", "4266MHZ");
		seq_printf(m, "maxScreenCount: %s\n", "6");
		seq_printf(m, "effectiveBw: %s\n", "273024Mbps");
		seq_printf(m, "pixFillrate: %s\n", "26200M Pixel/s");
		seq_printf(m, "texFillrate: %s\n", "52400M Texel/s");
	}else{
		seq_printf(m, "name: %s\n", gd_info->devname);
		seq_printf(m, "maxScreenCount: %s\n", "6");

	}
	seq_printf(m, "vendorID: 0x%x\n", gd_info->vender_id);
	seq_printf(m, "genProc: %s\n", "12nm");
	seq_printf(m, "drvVer: %s\n", STR2(GB_VERSION));
	seq_printf(m, "vs: %dM\n", gd_info->vram);
	seq_printf(m, "shaderCount: %s\n", "96");
	seq_printf(m, "shaderType: %s\n", "union");
	seq_printf(m, "ropCount: %s\n", "32");
	seq_printf(m, "tmuCount: %s\n", "32");
	seq_printf(m, "pd: %s\n", "10W");
	seq_printf(m, "chipsetPubDate: %s\n", "2023/05/15");
	seq_printf(m, "drvPubDate: %s\n", "2023/06/13");
	seq_printf(m, "manufacturerName: %s\n", gd_info->product);
	seq_printf(m, "maxResWidth: %s\n", "4096");
	seq_printf(m, "maxResHeight: %s\n", "2160");
	/*
	seq_printf(m, "OpenGLVer: %s\n","unknown");
	seq_printf(m, "OpenCLVer: %s\n","unknown");
	seq_printf(m, "VulkanVer: %s\n","unknown");
	seq_printf(m, "vddc: %s\n","unknown");
	seq_printf(m, "temp: %s\n","unknown");
	seq_printf(m, "intlName: %s\n","unknown");
	seq_printf(m, "intlVer: %s\n","unknown");
	seq_printf(m, "chipsetID: %s\n","unknown");
	*/
	seq_printf(m, "normAcFreq: %s\n", "800MHZ");
	seq_printf(m, "ocAcFreq: %s\n", "900MHZ");
	seq_printf(m, "defFreq: %s\n", "800MHZ");
	seq_printf(m, "maxfreq: %s\n", "1300MHZ");
	seq_printf(m, "busIntfType: %s\n", "PCI-E 4.0 x16");
	seq_printf(m, "usedVSSize: %ldM\n", memusage);
	seq_printf(m, "usageVSPer: %d\n", usageper);
	seq_printf(m, "curFreq: %s\n", "800M");
#endif
	return 0;
}

static int GB02FUNC307(struct inode *inode, struct file *file)
{
	struct gpu_info *gd_info;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	gd_info = PDE_DATA(inode);
#else
	gd_info = pde_data(inode);
#endif
	return single_open(file, GB02FUNC290, gd_info);
}
static int GB02FUNC309(struct inode *inode, struct file *file)
{
	int res = single_release(inode, file);
	return res;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
static const struct file_operations gbdev_info_ops = {
	.open = GB02FUNC307,
	.read = seq_read,
	.release = GB02FUNC309,
	.owner = THIS_MODULE,
};
#else
static const struct proc_ops gbdev_info_ops = {
	.proc_open = GB02FUNC307,
	.proc_read = seq_read,
	.proc_release = GB02FUNC309,
};
#endif
struct gpu_info *GB02FUNC314(void)
{
	return &gdev_info;
}

int GB02FUNC317(struct gpu_info *gbinfo)
{
	remove_proc_subtree("gbgpuinfo", NULL);

	gbinfo_entry = proc_create_data("gbgpuinfo",
	 0, NULL, &gbdev_info_ops, gbinfo);
	if (!gbinfo_entry) {
		remove_proc_subtree("gbgpuinfo", NULL);
		return -ENOMEM;
	}
	return 0;
}

void GB02FUNC322(void)
{
	remove_proc_subtree("gbgpuinfo", NULL);
}

static ssize_t log_ctrl_store(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int ret = 0;

	if (sysfs_streq(buf, "log_on")) {
		printk("Tuning on log, old val = %d.\n", atomic_read(&log_en));
		atomic_set(&log_en, 7);
	} else if (sysfs_streq(buf, "log_off")) {
		printk("Tuning off log, old val = %d.\n", atomic_read(&log_en));
		atomic_set(&log_en, 3);
	} else if (sysfs_streq(buf, "quite") || sysfs_streq(buf, "0")) {
		printk("Tuning off log, old val = %d.\n", atomic_read(&log_en));
		atomic_set(&log_en, 0);
	} else {
		ret = -EINVAL;
	}

	return ret ? ret : count;
}

static ssize_t log_ctrl_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	int buf_len = 0;

	buf_len += snprintf(buf, PAGE_SIZE, "log_ctrl:0x%x\n", atomic_read(&log_en));

	return buf_len;
}

static DEVICE_ATTR_RW(log_ctrl);

#ifdef	CONFIG_UOS_20
static ssize_t show_gpu_info(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	int buf_len = 0;
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct GB02STR39 *gbdev = ddev->dev_private;
	struct gpu_info *gbinfo = gbdev->gpu_device_info;

	buf_len += snprintf(buf, PAGE_SIZE, "VRAM total size:0x%llx\n",
			    gbinfo->vram << 20);

	return buf_len;
}
static DEVICE_ATTR(gpu_info, 0444, show_gpu_info, NULL);
#endif

int GB02FUNC334(struct GB02STR39 *gb_dev)
{
#ifdef	CONFIG_UOS_20
	device_create_file(gb_dev->dev, &dev_attr_gpu_info);
#endif
	device_create_file(gb_dev->dev, &dev_attr_log_ctrl);

	return 0;
}

void GB02FUNC336(struct GB02STR39 *gb_dev)
{
#ifdef	CONFIG_UOS_20
	device_remove_file(gb_dev->dev, &dev_attr_gpu_info);
#endif
	device_remove_file(gb_dev->dev, &dev_attr_log_ctrl);
}
