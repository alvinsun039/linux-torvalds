// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/gfp.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include "gb_firmware_upgrade.h"
#include "gb_device.h"

#define GB02MAC1025 'U'
#define UPGRADE_MCU_CMD_START _IOW(GB02MAC1025, 1u, int)
#define UPGRADE_MCU_CMD_GET_PROGRESS _IOW(GB02MAC1025, 2u, int)
#define UPGRADE_MCU_CMD_SET_IMAGE_INFO _IOW(GB02MAC1025, 3u, int)

static struct GB02STR115 *mcu_infos;

#define PCIE_UPGRADE_MCU_STR                ("upgrade_mcu")

#define GB02MAC1026 (1024 * 4)    /* 4KB */
//#define GB02MAC501 (0xB0000000 )
#define GB02MAC1027	(0x100000) /* 1MB */
#define GB02MAC1028	(0x20000)
#define GB02MAC1029		(0x20100)
#define GB02MAC1030		(0x20200)
#define GB02MAC1031		(0x20300)
#define GB02MAC1032	(0x20400)
#define GB02MAC1033		(0x20500)

ssize_t GB02FUNC616(struct GB02STR115 *mcu_ctrl, unsigned int offset, void *buffer, size_t len)
{
    struct GB02STR39 *gbdev = mcu_ctrl->gbdev;
    int ddr_bar_id = 2;
    void *base_addr = gbdev->gb_pcie->pci_bars[ddr_bar_id].mmio + GB02MAC501 + offset;

    memcpy(buffer, base_addr, len);

    return len;
}

ssize_t GB02FUNC617(struct GB02STR115 *mcu_ctrl, unsigned int offset, void *buffer, size_t len)
{
    struct GB02STR39 *gbdev = mcu_ctrl->gbdev;
    int ddr_bar_id = 2;
    void *base_addr = gbdev->gb_pcie->pci_bars[ddr_bar_id].mmio + GB02MAC501 + offset;

    memcpy(base_addr, buffer, len);

    return len;
}

static int GB02FUNC620(struct inode *inode, struct file *filp)
{
    filp->private_data = mcu_infos;
	return 0;
}

static int GB02FUNC621(struct inode *inode, struct file *filp)
{
    filp->private_data = NULL;
	return 0;
}
#define GB02MAC1038 (VM_READ|VM_WRITE|VM_EXEC)
static int GB02FUNC622(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long off;
    unsigned long phys;
    unsigned long vsize;
    unsigned long psize;
    int rv;
    struct GB02STR39 *gbdev = mcu_infos->gbdev;
    off = vma->vm_pgoff << PAGE_SHIFT;

    /* BAR physical address */
    phys = pci_resource_start(gbdev->gb_pcie->pdev, 0) + off;
    vsize = vma->vm_end - vma->vm_start;
    /* complete resource */
    psize = pci_resource_end(gbdev->gb_pcie->pdev, 0) -
        pci_resource_start(gbdev->gb_pcie->pdev, 0) + 1 - off;

    // printk("mmap(): mmap->bar:0\n");
    // printk("off = 0x%lx, vsize 0x%lu, psize 0x%lu.\n", off, vsize, psize);
    // printk("start = 0x%llx\n", gbdev->gb_pcie->pci_bars[0].base);
    // printk("phys = 0x%lx\n", phys);

    if (vsize > psize)
        return -EINVAL;
    /*
    * pages must not be cached as this would result in cache line sized
    * accesses to the end point
    */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    /*
    * prevent touching the pages (byte access) for swap-in,
    * and prevent the pages from being swapped out
    */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	vma->vm_flags |= GB02MAC1038;
#else
	vm_flags_set(vma, GB02MAC1038);
#endif
    /* make MMIO accessible to user space */
    rv = io_remap_pfn_range(vma, vma->vm_start, phys >> PAGE_SHIFT,
            vsize, vma->vm_page_prot);
    // printk("vma=0x%p, vma->vm_start=0x%lx, phys=0x%lx, size=%lu = %d\n",
    //     vma, vma->vm_start, phys >> PAGE_SHIFT, vsize, rv);

    if (rv)
        return -EAGAIN;
    return 0;
}

static ssize_t
gb02_fw_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    char *kbuffer;
    ssize_t bytes_read = 0;
    struct GB02STR115 *mcu_ctrl = (struct GB02STR115 *)(filp->private_data);

    kbuffer = kmalloc(min_t(size_t, count, GB02MAC1026), GFP_KERNEL);
    if (!kbuffer)
        return -ENOMEM;

    while (bytes_read == 0) {
        ssize_t need = min_t(unsigned long, count, GB02MAC1026);

        bytes_read = GB02FUNC616(mcu_ctrl, bytes_read, kbuffer, need);
        if (bytes_read != 0)
            break;

        if (filp->f_flags & O_NONBLOCK) {
            bytes_read = -EAGAIN;
            break;
        }

	    if (bytes_read > 0 && copy_to_user(buf, kbuffer, bytes_read))
    	    bytes_read = -EFAULT;

		bytes_read += bytes_read;
	}

    kfree(kbuffer);
    return bytes_read;
}

static ssize_t
gb02_fw_write(struct file *filp, const char __user *buf,
              size_t count, loff_t *ppos)
{
    char *kbuffer;
    ssize_t bytes_written = 0;
    ssize_t wrote;
    struct GB02STR115 *mcu_ctrl = (struct GB02STR115 *)(filp->private_data);

    kbuffer = kmalloc(min_t(size_t, count, GB02MAC1026), GFP_KERNEL);
    if (!kbuffer)
        return -ENOMEM;

    while (bytes_written < count) {
        ssize_t n = min_t(unsigned long, count - bytes_written, GB02MAC1026);

        if (copy_from_user(kbuffer, buf + bytes_written, n)) {
            bytes_written = -EFAULT;
            break;
        }

        wrote = GB02FUNC617(mcu_ctrl, bytes_written, kbuffer, n);
        if (wrote <= 0) {
            if (!bytes_written)
                bytes_written = wrote;
            break;
        }

        bytes_written += wrote;

        if (filp->f_flags & O_NONBLOCK) {
            if (!bytes_written)
                bytes_written = -EAGAIN;
            break;
        }
    }

    kfree(kbuffer);
    return bytes_written;
}

static int GB02FUNC634(struct GB02STR115 *mcu_ctrl)
{
    u32 val = 0;
    struct GB02STR39 *gbdev = mcu_ctrl->gbdev;

    GB02FUNC528(GB02MAC740, gbdev->gb_pcie->pci_bars[GB02MAC1071].mmio, GB02MAC734);

    /* tell MCU to upgrade sub image */

    val = GB02FUNC530(gbdev->gb_pcie->pci_bars[GB02MAC1069].mmio, GB02MAC855);
    val &= ~0xffff;
    val |= 0x5a;
    GB02FUNC528(val, gbdev->gb_pcie->pci_bars[GB02MAC1069].mmio, GB02MAC855);

    mcu_ctrl->upgrade_result = T_UPGRADE_MCU_RESULT_UPGRADING;
	return 0;
}

static int GB02FUNC638(struct GB02STR115 *mcu_ctrl)
{
    u32 val = 0;
    struct GB02STR39 *gbdev = mcu_ctrl->gbdev;

    /* set SPARE4 register */
    GB02FUNC528(GB02MAC740, gbdev->gb_pcie->pci_bars[GB02MAC1071].mmio, GB02MAC734);

    /* get the progress of upgrade MCU's sub image */
    val = GB02FUNC530(gbdev->gb_pcie->pci_bars[GB02MAC1069].mmio, GB02MAC855);
    mcu_ctrl->upgrade_result = ((val >> 8) & 0xff);
    return mcu_ctrl->upgrade_result;
}

static int GB02FUNC640(struct GB02STR115 *mcu_ctrl, struct GB02STR114 *image_info)
{
	u32 blk_nbr = 0;
    struct GB02STR39 *gbdev = mcu_ctrl->gbdev;
    int ddr_bar_id = 2;
	void __iomem *base_addr = gbdev->gb_pcie->pci_bars[ddr_bar_id].mmio + GB02MAC501;

	if (image_info == NULL)
		return -1;

	mcu_ctrl->upgrade_result = T_UPGRADE_MCU_RESULT_NO_UPGRADE;
	/* FILENAME */
	gb_printf(KERN_DEBUG, "filename : %s, addr : 0x%llx, size : 0x%x, type : %d\n",
			image_info->filename, image_info->addr, image_info->size, image_info->type);

	memcpy(base_addr + GB02MAC1028, image_info->filename, strlen(image_info->filename));
	*((char *)(base_addr + GB02MAC1028 + strlen(image_info->filename))) = '\0';

	/* ADDR */
	GB02FUNC528(image_info->addr & 0x1FFFFFFF, base_addr, GB02MAC1029);

	blk_nbr = (image_info->addr)/(512*MB);

	gb_printf(KERN_DEBUG, "512M_blk_number:%d--0x%x",blk_nbr, blk_nbr);

	/* write 512M block index to sysctrl_SPARE3 6000020c register spare3*/
	GB02FUNC528(blk_nbr, gbdev->gb_pcie->pci_bars[GB02MAC1069].mmio, GB02MAC46);

	/* SIZE */
	GB02FUNC528(image_info->size, base_addr, GB02MAC1030);

	/* CRC16 */
	GB02FUNC528(image_info->crc16, base_addr, GB02MAC1031);

	/* CHECKSUM */
	GB02FUNC528(image_info->checksum, base_addr, GB02MAC1032);

	/* IMAGE TYPE */
	GB02FUNC528(image_info->type, base_addr, GB02MAC1033);

	return 0;
}

static long GB02FUNC646(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
	struct GB02STR115 *mcu_ctrl = filp->private_data;
	struct GB02STR114 image_info;

	gb_printf(KERN_INFO, "enter ioctrl!\n");

    switch (cmd) {
    case UPGRADE_MCU_CMD_START:
		gb_printf(KERN_INFO, "=====Upgrade MCU's SUB start!======\n");
        ret = GB02FUNC634(mcu_ctrl);
        break;
    case UPGRADE_MCU_CMD_GET_PROGRESS:
		gb_printf(KERN_INFO, "=====Get the progress of upgrade MCU's sub start!======\n");
        ret = GB02FUNC638(mcu_ctrl);
        break;
	case UPGRADE_MCU_CMD_SET_IMAGE_INFO:
		gb_printf(KERN_INFO, "=====Set the upgrade image info of MCU start!======\n");
		memset(&image_info, 0, sizeof(image_info));
		if (copy_from_user(&image_info, (struct GB02STR114 *)arg,
			sizeof(struct GB02STR114)) ) {
			gb_printf(KERN_ERR, "%s:%d:copy_from_user() invalid!\n", __func__, __LINE__);
			return -EFAULT;
		}

        ret = GB02FUNC640(mcu_ctrl, &image_info);
		break;
    default:
        break;
    }

	return 0;
}

static __poll_t GB02FUNC651(struct file *filp, poll_table *wait)
{
    struct GB02STR115 *mcu_ctrl = filp->private_data;
	__poll_t mask = 0;
    int val = 0;

    /* for FPGA, comment out such codes */
    val = GB02FUNC638(mcu_ctrl);
    gb_printf(KERN_INFO, "%s upgrade state : %d\n",__func__,val);
    if(val == T_UPGRADE_MCU_RESULT_OK) {
        mcu_ctrl->upgrade_flag = 0;
        mcu_ctrl->upgrade_result = T_UPGRADE_MCU_RESULT_OK;
        mask |= EPOLLIN | EPOLLRDNORM;
    }

    return mask;
}

static struct file_operations gb02_fw_fops = {
    .owner = THIS_MODULE,
    .open = GB02FUNC620,
    .read = gb02_fw_read,
    .write = gb02_fw_write,
    .release = GB02FUNC621,
    .unlocked_ioctl = GB02FUNC646,
    .mmap = GB02FUNC622,
    .poll = GB02FUNC651,
};
static struct miscdevice gb02_fw_misc =
{
	.fops       = &gb02_fw_fops,
	.name       = "gb02_fw",
	.minor      = MISC_DYNAMIC_MINOR,
	.mode       = S_IRWXUGO,
};

irqreturn_t GB02FUNC656(int irq, void *arg)
{
    struct GB02STR115 *mcu_ctrl = mcu_infos;
    struct GB02STR39 *gbdev = mcu_ctrl->gbdev;
    u32 status = 0, val = 0;

	gb_printf(KERN_DEBUG, "%s:%d:enter.\n", __func__, __LINE__);

    GB02FUNC528(GB02MAC743, gbdev->gb_pcie->pci_bars[GB02MAC1071].mmio, GB02MAC737);
    status = GB02FUNC530(gbdev->gb_pcie->pci_bars[GB02MAC1070].mmio, GB02MAC745 + GB02MAC865);
    if (status & (1 << (44 - 32))) {
        /* MCU upgrade finished */
        val = GB02FUNC530(gbdev->gb_pcie->pci_bars[GB02MAC1069].mmio, GB02MAC852);
        val &= ~GB02MAC853;
        GB02FUNC528(val, gbdev->gb_pcie->pci_bars[GB02MAC1069].mmio, GB02MAC852);

        val = GB02FUNC530(gbdev->gb_pcie->pci_bars[GB02MAC1069].mmio, GB02MAC855);
        val &= ~0xffff;
        val |= (2 << 8);
        GB02FUNC528(val, gbdev->gb_pcie->pci_bars[GB02MAC1069].mmio, GB02MAC855);
    }

    mcu_ctrl->upgrade_flag = 1;
    wake_up_interruptible(&mcu_ctrl->wait);

    return IRQ_HANDLED;
}


static int GB02FUNC660(struct seq_file *m, void *v)
{
	struct GB02STR115 *mcu_ctrl = (struct GB02STR115 *)m->private;
	upgrade_mcu_result_e result = mcu_ctrl->upgrade_result;

	seq_printf(m, "%d\n", result);

	return 0;
}

static int GB02FUNC662(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	struct GB02STR115 *mcu_ctrl = (struct GB02STR115 *)PDE_DATA(file_inode(file));
#else
	struct GB02STR115 *mcu_ctrl = (struct GB02STR115 *)pde_data(file_inode(file));
#endif

    return single_open(file, GB02FUNC660, mcu_ctrl);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
static const struct file_operations upgrade_mcu_proc_fops = {
    .owner      = THIS_MODULE,
    .open       = GB02FUNC662,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};
#else
static const struct proc_ops upgrade_mcu_proc_fops = {
    .proc_open       = GB02FUNC662,
    .proc_read       = seq_read,
    .proc_lseek      = seq_lseek,
    .proc_release    = single_release,
};
#endif

static int GB02FUNC667(struct GB02STR115 *mcu_ctrl)
{
	char proc_name[15] = {0};

	snprintf(proc_name, sizeof(proc_name), "%s", "upgrade_mcu");

	mcu_ctrl->procdir = proc_mkdir(proc_name, NULL);
	if(!mcu_ctrl->procdir) {
		gb_printf(KERN_ERR, "create the procfs node(%s) failed.\n", proc_name);
		return -1;
	}

	proc_create_data("upgrade_result",  S_IRUGO, mcu_ctrl->procdir, &upgrade_mcu_proc_fops, (void *)mcu_ctrl);

	return 0;
}

int GB02FUNC670(struct GB02STR39 *gbdev, struct GB02STR67 *GB02STR153)
{
	int ret = 0;
	struct GB02STR115 *mcu_ctrl;
	struct device *dev = gbdev->dev;

	gb_printf(KERN_INFO, "Start to init GB02 firmware's upgrade\n");

	mcu_ctrl = devm_kzalloc(dev, sizeof(*mcu_ctrl), GFP_KERNEL);
	if (!mcu_ctrl) {
		gb_printf(KERN_ERR, "%s:kmalloc failed\n", __func__);
		return -ENOMEM;
	}

	mcu_ctrl->gbdev = gbdev;
    mcu_ctrl->dev_info = GB02STR153;
    init_waitqueue_head(&mcu_ctrl->wait);
    mcu_ctrl->upgrade_flag = 0;
    mcu_ctrl->upgrade_result = T_UPGRADE_MCU_RESULT_NO_UPGRADE;

	ret = misc_register(&gb02_fw_misc);
	if (ret < 0) {
		gb_printf(KERN_ERR, "%s:Failed to register as misc device:%d\n",
			__func__, ret);
		return ret;
	}

	/* create procfs node */
	ret = GB02FUNC667(mcu_ctrl);
	if(ret) {
		gb_printf(KERN_ERR, "GB02FUNC667() failed:%d\n", ret);
		goto err_misc_deregister;
	}

	/* enable mcu2pcie sw interrupt */
    /* for FPGA, disable such codes */
#if 0
    GB02FUNC528(GB02MAC743, gbdev->gb_pcie->pci_bars[GB02MAC1071].mmio, GB02MAC737);
	val = GB02FUNC530(gbdev->gb_pcie->pci_bars[GB02MAC1070].mmio, GB02MAC745 + GB02MAC859);
	val |= 1 << (44 - 32);
	GB02FUNC528(val, gbdev->gb_pcie->pci_bars[GB02MAC1070].mmio, GB02MAC745 + GB02MAC859);
#endif

    mcu_infos = mcu_ctrl;

    return 0;

err_misc_deregister:
	misc_deregister(&gb02_fw_misc);

	return ret;
}

void GB02FUNC677(struct GB02STR39 *gbdev)
{
	struct GB02STR115 *mcu_ctrl = mcu_infos;

	if (mcu_ctrl->procdir) {
		proc_remove(mcu_ctrl->procdir);
	}

	misc_deregister(&gb02_fw_misc);
    mcu_infos = NULL;
}

void GB02FUNC679(struct GB02STR39 *gbdev)
{
	//struct GB02STR115 *mcu_ctrl = mcu_infos;
#if 0
    u32 val = 0, i = 0;
#endif
    gb_printf(KERN_DEBUG, "%s:%d:enter\n", __func__, __LINE__);

    /* tell MCU to reboot */
    //GB02FUNC528(GB02MAC740, mcu_ctrl->pci_mmio_bar[DMA_CFG_REG_BAR4], GB02MAC734);
    /* hot reset */
    //GB02FUNC528(GB02MAC44, mcu_ctrl->pci_mmio_bar[GB02MAC1069], GB02MAC42);
    /* MCU reset */
    //GB02FUNC528(GB02MAC48, mcu_ctrl->pci_mmio_bar[GB02MAC1069], GB02MAC46);

#if 0
    /* set SPARE4 register */
    GB02FUNC528(0, mcu_ctrl->pci_mmio_bar[GB02MAC1069], GB02MAC49);
    mdelay(100);

    for(i = 0;i < 1000;i++) {
        val = GB02FUNC530(mcu_ctrl->pci_mmio_bar[GB02MAC1069], GB02MAC49);
        if (val == GB02MAC50)
            break;
        mdelay(10);
        gb_printf(KERN_INFO, "%s:%d:val = 0x%x\n", __func__, __LINE__, val);
    }
#endif
}
