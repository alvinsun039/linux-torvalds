// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
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
#include <linux/pinctrl/consumer.h>
#include <linux/seq_file.h>
#include "gpu/gb_device.h"
#include "pinctrl-gb02-mcu.h"
#include "common/gb-peripherals-common.h"

//#define PINCTRL_DEBUG
#ifdef PINCTRL_DEBUG
#define GB02MAC412(arg...) gb_printf(KERN_INFO, "[" PLATFORM_LGPIO_DEVICE_NAME "] " arg)
#else
#define GB02MAC412(arg...)
#endif


static DEFINE_MUTEX(sysfs_lock);
static int GB02FUNC236(unsigned int gpio);

struct GB02STR48 {
	const char *name;
	unsigned int start_pin;
	unsigned int npins;
	u32 reg_mask;
	u32 val[GB02MAC543];
	const char *funcs[GB02MAC543];
	unsigned int *pins;
};

struct GB02STR49 {
	int nr_pins;
	char *name;
	struct GB02STR48 *groups;
	int ngroups;
};

struct GB02STR51 {
	const char *name;
	const char **groups;
	unsigned int ngroups;
};

struct GB02STR52 {
	void __iomem *base;
	spinlock_t lock;
};

struct GB02STR53 {
	unsigned int gpio;
	struct mutex mutex;
};

struct GB02STR54 {
	void __iomem *regs;
	void __iomem *sysctl_iofunc_base;			 // bar0
	void __iomem *sysctl_system_peripheral_base; // bar1
	void __iomem *sysctl_cfg_base;				 // bar4
	struct device *dev;
	struct GB02STR48 *groups;
	unsigned int ngroups;
	struct GB02STR51 *funcs;
	unsigned int nfuncs;
	unsigned int nbanks;
	struct GB02STR52 *banks;
	struct GB02STR49 *data;
};

static struct GB02STR54 *g_pinctrl_infos;

static inline int GB02FUNC195(int gpio)
{
	return gpio % GB02MAC538;
}

/* GPIO functions */
static inline void GB02FUNC197(struct GB02STR52 *bank,
	 unsigned int offset, int value)
{
	int dat = 0;

	dat = readl_relaxed(bank->base + GB02MAC527);
	dat &= ~BIT(offset);
	GB02MAC412("offset %u set %d\n", offset, value);
	if (value)
		dat |= BIT(offset);
	writel_relaxed(dat, bank->base + GB02MAC527);
}

static inline u32 GB02FUNC199(struct GB02STR52 *bank,
	unsigned int offset, int mode)
{
	u32 val = 0;

	if (mode == GB02MAC541)
		val = readl_relaxed(bank->base + GB02MAC523);
	else if (mode == GB02MAC542)
		val = readl_relaxed(bank->base + GB02MAC527);
	GB02MAC412("offset %d get %d\n", offset, val);

	return !!(val & BIT(offset));
}

static inline bool GB02FUNC201(unsigned int gpio)
{
	int ngpio;
	struct GB02STR54 *info = g_pinctrl_infos;

	ngpio = info->data->nr_pins;
	if (gpio > ngpio)
		return false;

	return true;
}

static ssize_t direction_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t			status;
	int mode;
	unsigned int gpio;
	struct GB02STR53 *data = dev_get_drvdata(dev);

	gpio = data->gpio;
	mode = GB02FUNC304(gpio);
	status = sprintf(buf, "%s\n", mode ? "out" : "in");

	return status;
}

static ssize_t direction_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	unsigned int gpio;
	struct GB02STR53 *data = dev_get_drvdata(dev);

	gpio = data->gpio;
	if (sysfs_streq(buf, "out"))
		status = GB02FUNC301(gpio, GB02MAC542);
	else if (sysfs_streq(buf, "in"))
		status = GB02FUNC301(gpio, GB02MAC541);
	else
		status = -EINVAL;

	return status ? : size;
}
static DEVICE_ATTR_RW(direction);

static ssize_t value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t			status;
	unsigned int gpio;
	struct GB02STR53 *data = dev_get_drvdata(dev);

	gpio = data->gpio;
	status = GB02FUNC310(gpio);
	if (status < 0)
		goto err;

	status = sprintf(buf, "%ld\n", status);
err:

	return status;
}

static ssize_t value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status = 0;
	long		value;
	unsigned int gpio;
	struct GB02STR53 *data = dev_get_drvdata(dev);

	gpio = data->gpio;
	if (size <= 2 && isdigit(buf[0]) &&
	    (size == 1 || buf[1] == '\n'))
		value = buf[0] - '0';
	else
		status = kstrtol(buf, 0, &value);

	if (status == 0) {
		GB02FUNC312(gpio, value);
		status = size;
	}

	return status;
}
static DEVICE_ATTR_PREALLOC(value, S_IWUSR | S_IRUGO, value_show, value_store);

static struct attribute *gpio_attrs[] = {
	&dev_attr_direction.attr,
	&dev_attr_value.attr,
	NULL,
};

static const struct attribute_group gpio_group = {
	.attrs = gpio_attrs,
};

static const struct attribute_group *gpio_groups[] = {
	&gpio_group,
	NULL
};

static int GB02FUNC224(struct device *dev, const void *name)
{
	return !strcmp(dev_name(dev), name);
}

/*
 * /sys/class/gpio/export ... write-only
 *	integer N ... number of GPIO to export (full access)
 * /sys/class/gpio/unexport ... write-only
 *	integer N ... number of GPIO to unexport
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t export_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)
#else
static ssize_t export_store(const struct class *class,
				const struct class_attribute *attr,
				const char *buf, size_t len)
#endif	
{
	long			gpio;
	int			status;

	status = kstrtol(buf, 0, &gpio);
	if (status < 0)
		goto done;

	status = GB02FUNC295(gpio);
	if (status < 0) {
		if (status == -EPROBE_DEFER)
			status = -ENODEV;
		goto done;
	}

	status = GB02FUNC236(gpio);
	if (status < 0)
		GB02FUNC300(gpio);
done:
	if (status)
		GB02MAC412("%s: status %d\n", __func__, status);
	return status ? : len;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
static CLASS_ATTR_WO(export);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t unexport_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)
#else
static ssize_t unexport_store(const struct class *class,
				const struct class_attribute *attr,
				const char *buf, size_t len)
#endif	
{
	long			gpio;
	int			status;
	struct device *dev;
	char dev_name[10];

	status = kstrtol(buf, 0, &gpio);
	if (status < 0)
		goto done;

	sprintf(dev_name, "gpio%ld", gpio);
	/* No extra locking here; FLAG_SYSFS just signifies that the
	 * request and export were done by on behalf of userspace, so
	 * they may be undone on its behalf too.
	 */
	dev = class_find_device(class, NULL, dev_name, GB02FUNC224);
	if (!dev)
		gb_printf(KERN_ERR, "can't find dev\n");
	device_destroy(class, dev->devt);
	GB02FUNC300(gpio);
done:
	if (status)
		GB02MAC412("%s: status %d\n", __func__, status);
	return status ? : len;
}
static CLASS_ATTR_WO(unexport);

static struct attribute *gb02_gpio_class_attrs[] = {
	&class_attr_export.attr,
	&class_attr_unexport.attr,
	NULL,
};

ATTRIBUTE_GROUPS(gb02_gpio_class);


static struct class gb02_gpio_class = {
	.name =		"gb02_gpio",
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	.owner =	THIS_MODULE,
#endif
	.class_groups = gb02_gpio_class_groups,
};


static int GB02FUNC236(unsigned int gpio)
{
	int			status;
	const char		*ioname = NULL;
	struct device		*dev;
	struct GB02STR53 *data;

	mutex_lock(&sysfs_lock);

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	data->gpio = gpio;
	mutex_init(&data->mutex);
	dev = device_create_with_groups(&gb02_gpio_class, g_pinctrl_infos->dev,
					MKDEV(0, 0), data, gpio_groups,
					ioname ? ioname : "gpio%u",
					gpio);
	if (IS_ERR(dev)) {
		status = PTR_ERR(dev);
		goto err_unlock;
	}

	mutex_unlock(&sysfs_lock);
	return 0;

err_unlock:
	mutex_unlock(&sysfs_lock);
	return status;
}

static int GB02FUNC245(void)
{
	int status = 0;

	status = class_register(&gb02_gpio_class);
	if (status < 0)
		return status;

	return status;
}

#ifdef TEST_MISC_GPIO
static int GB02FUNC246(struct GB02STR54 *info, u32 gpio)
{
	/* GPIO range judge */
	if ((gpio < GB02MAC546) ||
	 (gpio > GB02MAC547)) {
		gb_printf(KERN_ERR, "GPIO IRQ : unsupported gpio(%d)\n", gpio);
		return -1;
	}

	/* GPIO output mode */
	GB02FUNC312(gpio, 1);
	udelay(10);
	GB02FUNC312(gpio, 0);
	udelay(10);
	GB02FUNC312(gpio, 1);

	return 0;
}

static int GB02FUNC251(struct inode *inode, struct file *filp)
{
	filp->private_data = g_pinctrl_infos;
	return 0;
}

static int GB02FUNC253(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static long GB02FUNC255(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct GB02STR54 *info = filp->private_data;

	GB02MAC412("enter ioctrl!\n");

	switch (cmd) {
	case PINCTRL_MCU_CMD_GPIO40:
		GB02MAC412("=====MCU's GPIO40 start!======\n");
		ret = GB02FUNC246(info, 40);
		break;
	case PINCTRL_MCU_CMD_GPIO41:
		GB02MAC412("=====MCU's GPIO41 start!======\n");
		ret = GB02FUNC246(info, 41);
		break;
	case PINCTRL_MCU_CMD_GPIO42:
		GB02MAC412("=====MCU's GPIO42 start!======\n");
		ret = GB02FUNC246(info, 42);
		break;
	case PINCTRL_MCU_CMD_GPIO43:
		GB02MAC412("=====MCU's GPIO43 start!======\n");
		ret = GB02FUNC246(info, 43);
		break;
	default:
		break;
	}

	return 0;
}

static const struct file_operations mmap_fops = {
	.owner = THIS_MODULE,
	.open = GB02FUNC251,
	.release = GB02FUNC253,
	.unlocked_ioctl = GB02FUNC255,
};

static struct miscdevice gbfpga_misc = {
	.fops = &mmap_fops,
	.name = "gbfpgagpio",
	.minor = MISC_DYNAMIC_MINOR,
};
#endif

static void GB02FUNC262(struct GB02STR52 *bank,
	int pin, u32 mode)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&bank->lock, flags);

	if (!mode) {
		/* GPIO input mode */
		val = readl_relaxed(bank->base + 0x4);
		val |= BIT(pin);
		writel_relaxed(val, bank->base + 0x4);
		val = readl_relaxed(bank->base + GB02MAC525);
		val &= ~BIT(pin);
		writel_relaxed(val, bank->base + GB02MAC525);

		val = readl_relaxed(bank->base + GB02MAC526);
		val &= ~BIT(pin);
		writel_relaxed(val, bank->base + GB02MAC526);
	} else {
		/* GPIO output mode */
		val = readl_relaxed(bank->base + 0x4);
		val &= ~BIT(pin);
		writel_relaxed(val, bank->base + 0x4);

		val = readl_relaxed(bank->base + GB02MAC525);
		val &= ~BIT(pin);
		val |= BIT(pin);
		writel_relaxed(val, bank->base + GB02MAC525);

		val = readl_relaxed(bank->base + GB02MAC526);
		val &= ~BIT(pin);
		writel_relaxed(val, bank->base + GB02MAC526);
	}

	spin_unlock_irqrestore(&bank->lock, flags);
}

static int GB02FUNC269(struct GB02STR54 *info,
	const char *name,
	struct GB02STR48 *grp)
{
	unsigned int mask = grp->reg_mask;
	int func = 0, val = 0, dat = 0;

	func = match_string(grp->funcs, GB02MAC543, name);
	if (func < 0)
		return -ENOTSUPP;

	val = grp->val[func];
	dat = GB02FUNC530(info->sysctl_iofunc_base, GB02MAC851);
	dat &= ~mask;
	dat |= val;
	GB02FUNC528(dat, info->sysctl_iofunc_base, GB02MAC851);

	return 0;
}

static struct GB02STR48 *GB02FUNC276(
	struct GB02STR54 *info, int pin, int *grp)
{
	while (*grp < info->ngroups) {
		struct GB02STR48 *group = &info->groups[*grp];
		int j;

		*grp = *grp + 1;
		for (j = 0; j < group->npins; j++)
			if (group->pins[j] == pin)
				return group;
	}
	return NULL;
}

static void GB02FUNC280(struct GB02STR52 *bank, int pin, u32 *mode)
{
	u32 val = 0, mode0 = 0, mode1 = 0;
	unsigned long flags;

	spin_lock_irqsave(&bank->lock, flags);

	val = readl_relaxed(bank->base + GB02MAC525);
	val &= BIT(pin);
	mode0 = !!val;

	val = readl_relaxed(bank->base + GB02MAC526);
	val &= BIT(pin);
	mode1 = !!val;

	*mode = (mode1 << 1) | mode0;

	spin_unlock_irqrestore(&bank->lock, flags);
}

static int GB02FUNC286(struct GB02STR54 *info,
	struct GB02STR48 *grp)
{
	unsigned int mask = grp->reg_mask;
	int func = 0, val = 0, dat = 0;

	if (strcmp(grp->name, "test") == 0)
		func = 1;

	GB02MAC412("enable function %s group %s\n",
	  grp->funcs[func], grp->name);

	val = grp->val[func];
	dat = GB02FUNC530(info->sysctl_iofunc_base, GB02MAC851);
	dat &= ~mask;
	dat |= val;
	GB02MAC412("sysctl_iofunc_set default %x\n", dat);
	GB02FUNC528(dat, info->sysctl_iofunc_base, GB02MAC851);

	return 0;
}

static struct GB02STR52 *GB02FUNC291(unsigned int gpio)
{
	int nbank;
	struct GB02STR54 *info = g_pinctrl_infos;

	nbank = gpio / GB02MAC538;

	return &info->banks[nbank];
}

void GB02FUNC292(unsigned int gpio, bool enable)
{
	u32 val = 0;
	/* enable or disable LGPIO5_0---LGPIO5_3 sw interrupt */
	if (gpio < 40) {
		GB02MAC412("unsupport gpio%d to irq\n", gpio);
		return;
	}
	GB02FUNC528(GB02MAC740, g_pinctrl_infos->sysctl_cfg_base, GB02MAC734);
	GB02FUNC528(GB02MAC743, g_pinctrl_infos->sysctl_cfg_base, GB02MAC737);
	val = GB02FUNC530(g_pinctrl_infos->sysctl_system_peripheral_base, GB02MAC745 + GB02MAC859);
	if (enable)
		val |= BIT(gpio - 40) << (45 - 32);
	else
		val &= ~BIT(gpio - 40) << (45 - 32);
	GB02FUNC528(val, g_pinctrl_infos->sysctl_system_peripheral_base, GB02MAC745 + GB02MAC859);
}

int GB02FUNC295(unsigned int gpio)
{
	struct GB02STR54 *info = g_pinctrl_infos;
	struct GB02STR48 *group;
	int grp = 0;
	bool valid;

	GB02MAC412("requesting gpio %d\n", gpio);

	valid = GB02FUNC201(gpio);
	if (!valid) {
		gb_printf(KERN_ERR,"gpio%u is valid\n", gpio);
		return -1;
	}

	while ((group = GB02FUNC276(info, gpio, &grp)))
		GB02FUNC269(info, "gpio", group);

	return 0;
}

void GB02FUNC300(unsigned int gpio)
{
	struct GB02STR54 *info = g_pinctrl_infos;
	struct GB02STR48 *group;
	int grp = 0;

	GB02MAC412("set pin %u as default\n", gpio);

	/* Set the pin to some default state */
	while ((group = GB02FUNC276(info, gpio, &grp)))
		GB02FUNC286(info, group);
}

int GB02FUNC301(unsigned int gpio, bool direction)
{
	struct GB02STR52 *bank;
	int pin;
	bool valid;

	valid = GB02FUNC201(gpio);
	if (!valid) {
		gb_printf(KERN_ERR,"gpio%u is valid\n", gpio);
		return -1;
	}

	bank = GB02FUNC291(gpio);
	pin = GB02FUNC195(gpio);

	GB02FUNC262(bank, pin, direction);

	return 0;
}

int GB02FUNC304(unsigned int gpio)
{
	struct GB02STR52 *bank = GB02FUNC291(gpio);
	int pin = GB02FUNC195(gpio);
	int ret = 0;
	u32 mode;

	GB02FUNC280(bank, pin, &mode);
	if (mode == GB02MAC541)
		ret = 0;
	else if (mode == GB02MAC542)
		ret = 1;
	else if (mode == 2) {
		gb_printf(KERN_ERR, "pin %u is in open-drain mode\n", gpio);
		ret = -EINVAL;
	} else
		ret = -EINVAL;

	return ret;
}

int GB02FUNC310(unsigned int gpio)
{
	struct GB02STR52 *bank = GB02FUNC291(gpio);
	int pin = GB02FUNC195(gpio);
	int mode = GB02FUNC304(gpio);

	return GB02FUNC199(bank, pin, mode);
}

int GB02FUNC312(unsigned int gpio, int value)
{
	int ret = 0;
	u32 mode;
	struct GB02STR52 *bank = GB02FUNC291(gpio);
	int pin = GB02FUNC195(gpio);

	GB02FUNC280(bank, pin, &mode);
	if (mode != GB02MAC542) {
		ret = GB02FUNC301(gpio, GB02MAC542);
		if (ret)
			gb_printf(KERN_ERR,"GB02FUNC312 gpio%u set %d falile\n", gpio, value);
	}
	GB02FUNC197(bank, pin, value);

	return ret;
}

static int GB02FUNC320(struct GB02STR51 *funcs,
	int *funcsize, const char *name)
{
	int i = 0;

	if (*funcsize <= 0)
		return -EOVERFLOW;

	while (funcs->ngroups) {
		/* function already there */
		if (strcmp(funcs->name, name) == 0) {
			funcs->ngroups++;

			return -EEXIST;
		}
		funcs++;
		i++;
	}

	/* append new unique function */
	funcs->name = name;
	funcs->ngroups = 1;
	(*funcsize)--;

	return 0;
}

static int GB02FUNC324(struct GB02STR54 *info)
{
	int n, num = 0, funcsize = info->data->nr_pins;

	for (n = 0; n < info->ngroups; n++) {
		struct GB02STR48 *grp = &info->groups[n];
		int i, f;

		grp->pins = devm_kcalloc(info->dev,
			grp->npins, sizeof(*grp->pins),
			GFP_KERNEL);
		if (!grp->pins)
			return -ENOMEM;

		for (i = 0; i < grp->npins; i++)
			grp->pins[i] = grp->start_pin + i;

		for (f = 0; (f < GB02MAC543) && grp->funcs[f]; f++) {
			int ret;
			/* check for unique functions and count groups */
			ret = GB02FUNC320(info->funcs, &funcsize,
			   grp->funcs[f]);
			if (ret == -EOVERFLOW)
				dev_err(info->dev,
				  "More functions than pins(%d)\n",
				  info->data->nr_pins);
			if (ret < 0)
				continue;
			num++;
		}
	}

	info->nfuncs = num;

	return 0;
}

static int GB02FUNC331(struct GB02STR54 *info)
{
	struct GB02STR51 *funcs = info->funcs;
	int n;

	for (n = 0; n < info->nfuncs; n++) {
		const char *name = funcs[n].name;
		const char **groups;
		int g;

		funcs[n].groups = devm_kcalloc(info->dev,
			funcs[n].ngroups,
			sizeof(*(funcs[n].groups)),
			GFP_KERNEL);
		if (!funcs[n].groups)
			return -ENOMEM;

		groups = funcs[n].groups;

		for (g = 0; g < info->ngroups; g++) {
			struct GB02STR48 *gp = &info->groups[g];
			int f;

			f = match_string(gp->funcs, GB02MAC543, name);
			if (f < 0)
				continue;

			*groups = gp->name;
			groups++;
		}
	}
	return 0;
}

static int GB02FUNC338(struct GB02STR54 *info)
{
	int ret = 0, j = 0;
	struct GB02STR52 *bank;

	info->banks = devm_kcalloc(info->dev, GB02MAC539, sizeof(*info->banks),
	   GFP_KERNEL);
	if (!info->banks)
		return -ENOMEM;

	for (j = 0; j < GB02MAC539; j++) {
		bank = &info->banks[info->nbanks];
		bank->base = info->regs + info->nbanks * 0x1000;
		spin_lock_init(&bank->lock);
		info->nbanks++;
	}

	return ret;
}

static int GB02FUNC341(struct device *dev,
	struct GB02STR54 *info)
{
	const struct GB02STR49 *pin_data = info->data;
	int ret;

	info->groups = pin_data->groups;
	info->ngroups = pin_data->ngroups;

	/*
	 * we allocate functions for number of pins and hope there are
	 * fewer unique functions than pins available
	 */
	info->funcs = devm_kcalloc(dev,
	   pin_data->nr_pins,
	   sizeof(struct GB02STR51),
	   GFP_KERNEL);
	if (!info->funcs)
		return -ENOMEM;

	ret = GB02FUNC338(info);
	if(ret)
		return ret;

	ret = GB02FUNC324(info);
	if (ret)
		return ret;

	ret = GB02FUNC331(info);
	if (ret)
		return ret;

	return 0;
}

static int GB02FUNC346(struct GB02STR54 *info)
{
#ifdef TEST_MISC_GPIO
	int ret = 0;
	int i = 0;
	u32 gpio = 0;

	for (i = 0; i < 4; i++) {
		gpio = 40 + i;

		/* GPIO request */
		ret = GB02FUNC295(gpio);
		if (ret) {
			gb_printf(KERN_ERR, "Failed to request gpio(%d)\n", gpio);
			return ret;
		}

		/* GPIO input mode */
		GB02FUNC301(gpio, GB02MAC541);
	}
#endif
	return 0;
}

#define PIN_GRP_GPIO(_name, _start, _nr, _mask, _func1) \
	{	  \
		.name = _name,	\
		.start_pin = _start,	\
		.npins = _nr,	 \
		.reg_mask = _mask,   \
		.val = {0, _mask},   \
		.funcs = { _func1,   \
				   "gpio" }  \
	}

#define PIN_GRP_GPIO_2(_name, _start, _nr, _mask, _val1, _val2, _func1) \
	{	   \
		.name = _name,	 \
		.start_pin = _start,	 \
		.npins = _nr,	  \
		.reg_mask = _mask,	\
		.val = {_val1, _val2},   \
		.funcs = { _func1,	\
				   "gpio" }   \
	}

static struct GB02STR48 gb02_mcu_pin_groups[] = {
	PIN_GRP_GPIO_2("test", 0, 8, BIT(11), BIT(11), 0, "plltest"),
	PIN_GRP_GPIO("spis", 8, 4, BIT(0), "spis"),
	PIN_GRP_GPIO("uart0", 12, 2, BIT(1), "uart"),
	PIN_GRP_GPIO("uart1", 14, 2, BIT(2), "uart"),
	PIN_GRP_GPIO("qspi0", 16, 6, BIT(3), "qspi"),
	PIN_GRP_GPIO("qspi1", 22, 6, BIT(4), "qspi"),
	PIN_GRP_GPIO("i2c0", 28, 2, BIT(5), "i2c"),
	PIN_GRP_GPIO("i2c1", 30, 2, BIT(6), "i2c"),
	PIN_GRP_GPIO("i2c2", 32, 2, BIT(7), "i2c"),
	PIN_GRP_GPIO("i2c3", 34, 2, BIT(8), "i2c"),
	PIN_GRP_GPIO("timer0", 36, 4, BIT(9), "timer"),
	PIN_GRP_GPIO("timer1", 40, 4, BIT(10), "timer"),
};

static const struct GB02STR49 gb02_pin_data = {
	.nr_pins = 44,
	.name = "GB02-GPIO",
	.groups = gb02_mcu_pin_groups,
	.ngroups = ARRAY_SIZE(gb02_mcu_pin_groups),
};

static int GB02FUNC354(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info, unsigned int device_id)
{
	struct GB02STR54 *GB02STR54;
	struct GB02STR70 *pcie_info;
	int mcu_peri_bar_id, peri_base_bar_id;
	int id, ret = 0;

	GB02MAC412("Start to probe GB02 pinctrl MCU:\n");
	pcie_info = gb_dev->gb_pcie;
	id = device_id;

	GB02STR54 = devm_kzalloc(gb_dev->dev, sizeof(struct GB02STR54), GFP_KERNEL);
	if (GB02STR54 == NULL)
		return -ENOMEM;

	mcu_peri_bar_id = GB02FUNC474(pcie_info->GB02STR153);
	peri_base_bar_id = GB02FUNC477(pcie_info->GB02STR153);

	GB02STR54->dev = gb_dev->dev;
	GB02STR54->data = (struct GB02STR49 *)&gb02_pin_data;
	GB02STR54->sysctl_iofunc_base = gb_dev->gb_pcie->pci_bars[mcu_peri_bar_id].mmio; // bar0
	GB02STR54->sysctl_cfg_base =  gb_dev->gb_pcie->pci_bars[peri_base_bar_id].mmio; //bar4
	GB02STR54->sysctl_system_peripheral_base = gb_dev->gb_pcie->pci_bars[GB02MAC721].mmio; //bar1
	GB02STR54->regs = GB02STR54->sysctl_iofunc_base + GB02MAC748 + GB02MAC834;

	ret = GB02FUNC341(gb_dev->dev, GB02STR54);
	if (ret)
		return ret;

	ret = GB02FUNC346(GB02STR54);
	if (ret)
		return ret;
	g_pinctrl_infos = GB02STR54;
	peri_info->priv = GB02STR54;
	GB02FUNC245();
	dev_info(GB02STR54->dev, "Pinctrl GB02 initialized\n");

	return ret;
}

static void GB02FUNC363(struct GB02STR72 *peri_info)
{
#ifdef TEST_MISC_GPIO
	int i = 0;

	for (i = 0; i < 4; i++)
		GB02FUNC300(40 + i);
#endif
	g_pinctrl_infos = NULL;
}

int GB02FUNC366(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	int ret;
	int device_id = 0;

#ifdef TEST_MISC_GPIO
	ret = misc_register(&gbfpga_misc);
	if (ret)
		dev_err(gb_dev->dev, "unable to register gpio misc device(%d)\n", ret);
#endif
	GB02MAC412("platform gpio register\n");
	device_id = peri_info->mcu_peripherals_device_id;
	ret = GB02FUNC354(gb_dev, peri_info, device_id);
	return ret;
}

void GB02FUNC371(struct GB02STR72 *peri_info)
{

	GB02FUNC363(peri_info);
#ifdef TEST_MISC_GPIO
	misc_deregister(&gbfpga_misc);
#endif
}
