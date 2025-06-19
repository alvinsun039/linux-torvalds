// SPDX-License-Identifier: GPL-2.0

#include <linux/clk.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "gpu/gb_device.h"
#include "gb02_pcie_msg.h"
#include "gb_pcie_info.h"

int g_ddr_ecc_flag = -1;
struct GB02STR39 *g_gbdev = NULL;

static pcie_cmd_t pcie_cmd_g[] = {
	{
		"ddrtest",
		{
		.cmd = PCIE_DDR_TEST,
		},
	},
	{
		"read",
		{
		.cmd = PCIE_DDR_READ,
		},
	},
	{
		"write",
		{
		.cmd = PCIE_DDR_WRITE,
		},
	},
	{
		"setecc",
		{
		.cmd = PCIE_ECC_SET,
		},
	},
	{
		"getecc",
		{
		.cmd = PCIE_ECC_GET,
		},
	},
};

unsigned int GB02FUNC1110(struct GB02STR39 *gbdev)
{
	unsigned int val;

	iowrite32(0x92000000, gbdev->gb_pcie->pci_bars[4].mmio + 0xd0c);

	msleep(1);
	val = ioread32(gbdev->gb_pcie->pci_bars[0].mmio + 0x70);
	val &= 7;

	printk(KERN_INFO "ecc val is 0x%x\n", val);

	if(val == 4 || val == 5){
		g_ddr_ecc_flag = 1;
	}else{
		g_ddr_ecc_flag = 0;
	}

	return g_ddr_ecc_flag;
}

u64 GB02FUNC1112(void)
{
	u64 size = 0;
	if(g_ddr_ecc_flag == 1)
	{
		size = GB02FUNC464()/8;
	}
	else
	{
		size = 0;
	}
	return size;
}

static int GB02FUNC1115(struct inode *inode, struct file *filp)
{
	return 0;
}

static int GB02FUNC1117(struct inode *inode, struct file *filp)
{
	return 0;
}


static ssize_t
gb02_pcie_msg_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{

	struct GB02STR39 *gbdev = g_gbdev;
	int ret;
	void *base_addr;
	pcie_msg_t pcie_msg;
	int mcu_peri_bar_id = GB02FUNC474(gbdev->gb_pcie->GB02STR153);

	if(*ppos)
		return 0;

	base_addr = gbdev->gb_pcie->pci_bars[mcu_peri_bar_id].mmio + GB02MAC1322;

	memcpy_fromio(&pcie_msg, base_addr, sizeof(pcie_msg_t));

	if(pcie_msg.magic != GB02MAC1318)
	{
		gb_printf(KERN_ERR, "NO msg, magic not GB02MAC1318\n");
		return -EFAULT;
	}

	if(pcie_msg.len != 0)
	{
		gb_printf(KERN_INFO, "%s:len %d\n", __func__, pcie_msg.len);
		pcie_msg.data = kmalloc(pcie_msg.len, GFP_KERNEL);
		memcpy_fromio(pcie_msg.data, base_addr+4+4+4, pcie_msg.len);

		ret = copy_to_user(buf, pcie_msg.data, pcie_msg.len);
		if (ret) {
			gb_printf(KERN_ERR, "%s:copy_to_user failed, returned %d\n", __func__, ret);

			kfree(pcie_msg.data);
			return -EFAULT;
		}

		*ppos += pcie_msg.len;
		kfree(pcie_msg.data);

	}

	return pcie_msg.len;
}

static ssize_t
gb02_pcie_msg_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	char *kbuffer;
	int i;
	struct GB02STR39 *gbdev = g_gbdev;
	void *base_addr;
	pcie_msg_t pcie_msg_s;
	int mcu_peri_bar_id = GB02FUNC474(gbdev->gb_pcie->GB02STR153);

	base_addr = gbdev->gb_pcie->pci_bars[mcu_peri_bar_id].mmio + GB02MAC1322;

	kbuffer = kmalloc(count + 1, GFP_KERNEL);
	if (kbuffer == NULL)
		return -ENOMEM;
	memset(kbuffer, 0, count + 1);

	if (copy_from_user(kbuffer, buf, count)) {
		return -ENOMEM;
	}

	gb_printf(KERN_INFO, "kbuffer is %s\n",kbuffer);

	pcie_msg_s.magic = ioread32(base_addr);

	if(pcie_msg_s.magic == GB02MAC1317)
	{
		gb_printf(KERN_ERR, "msg process, magic is GB02MAC1317\n");
		return -ENOMEM;
	}

	for(i=0;i<sizeof(pcie_cmd_g)/sizeof(pcie_cmd_t);i++)
	{
		if(strstr(kbuffer, pcie_cmd_g[i].cmd_str))
		{
			pcie_msg_s.magic = GB02MAC1317;
			pcie_msg_s.cmd = pcie_cmd_g[i].pcie_msg.cmd;
			pcie_msg_s.len = strlen(kbuffer) + 1;

			gb_printf(KERN_DEBUG, "cmd is %d, len is %d\n", pcie_msg_s.cmd, pcie_msg_s.len);

			memcpy_toio(base_addr, &pcie_msg_s, sizeof(uint32_t)*3);
			memcpy_toio(base_addr + sizeof(uint32_t)*3, kbuffer, pcie_msg_s.len);
			break;
		}
	}

	kfree(kbuffer);

	return count;
}

static struct file_operations gb02_pcie_msg_fops = {
	.owner = THIS_MODULE,
	.open = GB02FUNC1115,
	.read = gb02_pcie_msg_read,
	.write = gb02_pcie_msg_write,
	.release = GB02FUNC1117,
//	.unlocked_ioctl = gb02_pcie_msg_ioctl,
//	.poll = gb02_pcie_msg_poll,
};
static struct miscdevice gb02_pcie_msg_misc =
{
	.fops       = &gb02_pcie_msg_fops,
	.name       = "gb02_pcie_msg",
	.minor      = MISC_DYNAMIC_MINOR,
};

int GB02FUNC1128(struct GB02STR39 *gbdev)
{
	int ret = 0;

	gb_printf(KERN_INFO, "%s\n", __func__);

	g_gbdev = gbdev;
	GB02FUNC1110(gbdev);

	ret = misc_register(&gb02_pcie_msg_misc);
	if (ret < 0) {
		gb_printf(KERN_ERR, "%s:Failed to register as misc device:%d\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

void GB02FUNC1131(struct GB02STR39 *gbdev)
{
	misc_deregister(&gb02_pcie_msg_misc);
}
