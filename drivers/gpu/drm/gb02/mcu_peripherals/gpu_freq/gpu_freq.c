// SPDX-License-Identifier: GPL-2.0
/*
 *  Devfreq driver for freqs.
 *
 */

#include "common/gb-peripherals-common.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/random.h>
#include <linux/delay.h>
#include "gpu/gb_device.h"
#include "gb_devfreq.h"
#include "gpu_freq.h"
#include "gb_regs.h"

#define GB02MAC1076 900
#define GB02MAC1077 800
#define GB02MAC1078 700
#define GB02MAC1079 600
#define GB02MAC1080 400
#define GB02MAC1081 50
#define GB02MAC1082 70
#define GB02MAC1083 0x130
#define GB02_FREQ_DEV_NAME "gpu_freq"
#define GB02MAC1084 (sizeof(GB02STR98) / sizeof(struct GB02STR98))

//#define FREQ_DEBUG
#ifdef FREQ_DEBUG
#define GB02MAC1085(arg...) gb_printf(KERN_INFO, "[" PLATFORM_FREQ_DEVICE_NAME "] " arg)
#else
#define GB02MAC1085(arg...)
#endif

union gpupll_cfg {
	uint32_t val;
	struct {
		uint32_t gpu_pll_pllen:1;
		uint32_t gpu_pll_bypass:1;
		uint32_t gpu_pll_refdiv:6;
		uint32_t gpu_pll_postdiv1:3;
		uint32_t gpu_pll_postdiv2:3;
		uint32_t gpu_pll_fbdiv:12;
		uint32_t gpu_pll_dsmen:1;
		uint32_t reserve:5;
	} type;
};

struct GB02STR116 {
	struct GB02STR39 *gbdev;
	void __iomem *sysctl_cfg_base;    // bar4
	void __iomem *sysctl_iofunc_base; // bar0
	union gpupll_cfg cfg;
	spinlock_t lock;
	struct gb_devfreq *devfreq;
	unsigned long cur_freq;
	unsigned long cur_volt;
	unsigned long last_freq;
	struct notifier_block nb;
	bool limit_freq;
};

BLOCKING_NOTIFIER_HEAD(limit_freq_notifier_chain);
static struct GB02STR116 *g_ctx;
static struct mutex slot_lock[GB02MAC288];
static struct GB02STR98 GB02STR98[] = {
	{
		.available = false,
		.freq = GB02MAC1080,
		.volt = 0,
	},
	{
		.available = true,
		.freq = GB02MAC1079,
		.volt = 0,
	},
	{
		.available = true,
		.freq = GB02MAC1078,
		.volt = 0,
	},
	{
		.available = true,
		.freq = GB02MAC1077,
		.volt = 0,
	},
	{
		.available = false,
		.freq = GB02MAC1076,
		.volt = 0,
	},
};

u32 GB02FUNC693(void)
{
	return ((g_ctx->cur_freq) << 20);
}

static void GB02FUNC695(struct GB02STR39 *gb_dev, int slot)
{
	struct gb_devfreq_slot *devfreq_slot = &gb_dev->devfreq.slot[slot];
	ktime_t now;
	ktime_t last;

	if (!gb_dev->devfreq.devfreq)
		return;

	now = ktime_get();
	last = gb_dev->devfreq.slot[slot].time_last_update;

	/* If we last recorded a transition to busy, we have been idle since */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
	if (devfreq_slot->busy)
		gb_dev->devfreq.slot[slot].busy_time += ktime_sub(now, last);
	else
		gb_dev->devfreq.slot[slot].idle_time += ktime_sub(now, last);
#else
	if (devfreq_slot->busy)
		gb_dev->devfreq.slot[slot].busy_time.tv64 += ktime_sub(now, last).tv64;
	else
		gb_dev->devfreq.slot[slot].idle_time.tv64 += ktime_sub(now, last).tv64;
#endif
	gb_dev->devfreq.slot[slot].time_last_update = now;
}

/* The job scheduler is expected to call this at every transition busy <-> idle */
void GB02FUNC698(struct GB02STR39 *gb_dev, int slot)
{
	struct gb_devfreq_slot *devfreq_slot = &gb_dev->devfreq.slot[slot];

	mutex_lock(&slot_lock[slot]);
	GB02FUNC695(gb_dev, slot);
	devfreq_slot->busy = !devfreq_slot->busy;
	mutex_unlock(&slot_lock[slot]);
}

static void GB02FUNC703(struct GB02STR116 *ctx, enum freq_e freq)
{
	int locked = 1;
	if (ctx->cur_freq == freq)
		return;

	locked = spin_trylock(&ctx->gbdev->hw_irq_lock);
	if (!locked)
		return;

	ctx->cfg.type.gpu_pll_pllen = 0x1;
	ctx->cfg.type.gpu_pll_dsmen = 0x0;
	ctx->cfg.type.gpu_pll_refdiv = 0x1;
	ctx->cfg.type.gpu_pll_bypass = 0x0;
	ctx->cfg.type.gpu_pll_postdiv2 = 0x1;
	ctx->cfg.type.gpu_pll_postdiv1 = 0x1;
	switch (freq) {
	case GB02MAC1078:
		ctx->cfg.type.gpu_pll_fbdiv = 0x1C;
		GB02MAC1085("set freq 700\n");
		break;
	case GB02MAC1079:
		ctx->cfg.type.gpu_pll_fbdiv = 0x18;
		GB02MAC1085("set freq 600\n");
		break;
	case GB02MAC1076:
		ctx->cfg.type.gpu_pll_fbdiv = 0x24;
		GB02MAC1085("set freq 900\n");
		break;
	case GB02MAC1080:
		ctx->cfg.type.gpu_pll_fbdiv = 0x10;
		GB02MAC1085("set freq 400\n");
		break;
	case GB02MAC1077:
	default:
		ctx->cfg.type.gpu_pll_fbdiv = 0x20;
		GB02MAC1085("set freq 800\n");
		break;
	}
	//enable pll and config
	GB02FUNC528(ctx->cfg.val, ctx->sysctl_iofunc_base, GB02MAC1083);
	ctx->cur_freq = freq;
	spin_unlock(&ctx->gbdev->hw_irq_lock);
}

static ssize_t perf_test_store(struct device *dev, struct device_attribute *attr, const char *buf,
			size_t count)
{
	int ret;
	int value;
	struct GB02STR98 *freq_table = GB02FUNC424(dev->parent, GB02MAC1076);

	ret = sscanf(buf, "%d", &value);
	if (ret != 1)
		return -EINVAL;

	if (value == 1)
		GB02FUNC427(freq_table, true);
	else if (value == 0)
		GB02FUNC427(freq_table, false);

	return count;
}

static int GB02FUNC716(unsigned long open_bit)
{
	unsigned long val;
	int size, limit_size, i, bit;
	unsigned int on_bit = 0, off_bit = 0;
	unsigned int change_mask = 0xFFFFFFFF; //分bin方案：后续通过接口获取可操作核心bitmap mask

	limit_size = hweight32(change_mask);
	if (open_bit > limit_size)
		open_bit = limit_size;
	else if (open_bit < 1)
		open_bit = 1;

	val = gpu_read(g_ctx->gbdev, GB02MAC1451);
	size = hweight32(val);
	if (size > limit_size)
		return -EINVAL;

	if (size < open_bit) {
		on_bit = val;
		for (i = 0; i < open_bit - size; i++){
			bit = find_first_zero_bit(&val, 32);
			val |= BIT(bit);
			if (change_mask & BIT(bit))
				on_bit |= BIT(bit);
		}
		gpu_write(g_ctx->gbdev, SHADER_PWRON_LO, on_bit);
	} else if (size > open_bit){
		for (i = 0; i < size - open_bit; i++) {
			bit = find_first_bit(&val, 32);
			val &= ~BIT(bit);
			if (change_mask & BIT(bit))
				off_bit |= BIT(bit);
		}
		gpu_write(g_ctx->gbdev, SHADER_PWROFF_LO, off_bit);
	}

	return 0;
}

static ssize_t gb02_tpu_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 val;
	int size;

	val = gpu_read(g_ctx->gbdev, GB02MAC1451);
	size = hweight32(val);

	return sprintf(buf, "TPU_READY:0x%x, open_core:%d\n", val, size);
}

static ssize_t gb02_tpu_store(struct device *dev, struct device_attribute *attr, const char *buf,
			size_t count)
{
	unsigned long open_bit;
	int ret;

	if (kstrtoul(buf, 10, &open_bit))
		return -EINVAL;

	ret = GB02FUNC716(open_bit);
	if (ret)
		return ret;

	return count;
}

static DEVICE_ATTR_RW(gb02_tpu);
static DEVICE_ATTR_WO(perf_test);

static struct attribute *gb_gpufreq_attrs[] = {
	&dev_attr_gb02_tpu.attr,
	&dev_attr_perf_test.attr,
	NULL,
};

static const struct attribute_group gb_gpufreq_attr_group = {
	.attrs = gb_gpufreq_attrs,
};

static void GB02FUNC722(struct GB02STR39 *gb_dev)
{
	ktime_t now = ktime_get();
	int i;

	for (i = 0; i < GB02MAC288; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
		gb_dev->devfreq.slot[i].busy_time = 0;
		gb_dev->devfreq.slot[i].idle_time = 0;
#else
		gb_dev->devfreq.slot[i].busy_time.tv64 = 0;
		gb_dev->devfreq.slot[i].idle_time.tv64 = 0;
#endif
		gb_dev->devfreq.slot[i].time_last_update = now;
	}
}

static int GB02FUNC724(void)
{
	int i = 0;
	int cur_freq = g_ctx->cur_freq;

	for (i = 0; i < GB02MAC1084; i++) {
		if (GB02STR98[i].available && (cur_freq == GB02STR98[i].freq))
			return i;
	}
	return -1;
}

static int GB02FUNC726(struct device *dev, unsigned long *freq)
{
	struct GB02STR116 *ctx = g_ctx;
	struct gb_devfreq *df = ctx->devfreq;
	unsigned long target_rate;
	struct GB02STR98 *freq_table;
	struct GB02STR103 *user_data = NULL;

	freq_table = GB02FUNC424(dev, *freq);
	if (freq_table->available)
		target_rate = freq_table->freq;
	else
		target_rate = ctx->last_freq;

	*freq = target_rate;

	if (ctx->limit_freq && ctx->cur_freq < target_rate)
		return 0;

	if ((*freq == GB02MAC1077) || (*freq == GB02MAC1076))
		df->profile->polling_ms = 1000;
	else if (*freq == GB02MAC1079)
		df->profile->polling_ms = 50;

	spin_lock(&ctx->lock);
	GB02FUNC703(ctx, target_rate);
	spin_unlock(&ctx->lock);

	if (df->custom_mode.is_custom) {
		if (df->data)
			user_data = (struct GB02STR103 *)df->data;
		GB02FUNC716(user_data->tpu_cnt);
	}

	return 0;
}

static int GB02FUNC731(struct device *dev,
		struct gb_devfreq_dev_status *status)
{
	struct GB02STR116 *ctx = g_ctx;
	struct GB02STR39 *gb_dev = ctx->gbdev;
	int i;

	for (i = 0; i < GB02MAC288; i++) {
		GB02FUNC695(gb_dev, i);
	}

	status->current_frequency = ctx->cur_freq;
	status->total_time = ktime_to_ns(ktime_add(gb_dev->devfreq.slot[0].busy_time,
						   gb_dev->devfreq.slot[0].idle_time));

	status->busy_time = 0;
	for (i = 0; i < GB02MAC288; i++) {
		status->busy_time += ktime_to_ns(gb_dev->devfreq.slot[i].busy_time);
	}

	//make sure faster frequency rise
	if (status->busy_time / (status->total_time / 100) > GB02MAC1082) {
		spin_lock(&ctx->lock);
		GB02FUNC703(ctx, GB02MAC1077);
		spin_unlock(&ctx->lock);
	}

	/* We're scheduling only to one core atm, so don't divide for now */
	/* status->busy_time /= GB02MAC288; */
	GB02FUNC722(gb_dev);

	/* dev_dbg(gb_dev->dev, "busy %lu total %lu %lu %% freq %lu MHz\n", status->busy_time,
		status->total_time,
		status->busy_time / (status->total_time / 100),
		status->current_frequency); */

	return 0;
}

static int GB02FUNC736(struct device *dev, unsigned long *freq)
{
	struct GB02STR116 *ctx = g_ctx;

	if (!ctx)
		return -EINVAL;

	*freq = ctx->cur_freq;

	return 0;
}

static void GB02FUNC739(struct GB02STR116 *ctx)
{
	GB02FUNC528(GB02MAC740, ctx->sysctl_cfg_base, GB02MAC734);
}

static int GB02FUNC740(struct notifier_block *nb,
						unsigned long action, void *data)
{
	int level = GB02FUNC724();

	switch (action) {
	case GB02MAC1109:
		g_ctx->limit_freq = false;
		level += 1;
		GB02FUNC703(g_ctx, GB02STR98[level].freq);
		break;
	case GB02MAC1110:
		g_ctx->limit_freq = true;
		level -= 1;
		if (level < 0)
			level = 0;
		GB02FUNC703(g_ctx, GB02STR98[level].freq);
		break;
	default:
		g_ctx->limit_freq = false;
		break;
	}

	return NOTIFY_OK;
}

static struct gb_devfreq_dev_profile gb_devfreq_profile = {
	.polling_ms = GB02MAC1081,
	.target = GB02FUNC726,
	.get_dev_status = GB02FUNC731,
	.get_cur_freq = GB02FUNC736,
	.freq_table = GB02STR98,
	.max_state = GB02MAC1084,
};

static int GB02FUNC745(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	struct GB02STR116 *ctx;
	struct GB02STR70 *pcie_info;
	int mcu_peri_bar_id, peri_base_bar_id;
	int i = 0, ret = 0;

	pcie_info = gb_dev->gb_pcie;
	ctx = devm_kzalloc(gb_dev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mcu_peri_bar_id = GB02FUNC474(pcie_info->GB02STR153);
	peri_base_bar_id = GB02FUNC477(pcie_info->GB02STR153);
	ctx->gbdev = gb_dev;
	ctx->sysctl_iofunc_base = gb_dev->gb_pcie->pci_bars[mcu_peri_bar_id].mmio; // bar0
	ctx->sysctl_cfg_base = gb_dev->gb_pcie->pci_bars[peri_base_bar_id].mmio; //bar4
	ctx->cur_freq = GB02MAC1077;

	GB02FUNC739(ctx);
	spin_lock_init(&ctx->lock);
	for (i = 0; i < GB02MAC288; i++)
		mutex_init(&slot_lock[i]);

	GB02FUNC722(gb_dev);
	gb_devfreq_profile.initial_freq = GB02MAC1077;
	ctx->devfreq = GB02FUNC522(gb_dev->dev,
			&gb_devfreq_profile, GB_DEVFREQ_GOV_USERSPACE,
			NULL);
	if (IS_ERR(ctx->devfreq)) {
		DRM_DEV_ERROR(gb_dev->dev, "Couldn't initialize GPU devfreq\n");
		ret = PTR_ERR(ctx->devfreq);
		ctx->devfreq = NULL;
		return ret;
	}
	gb_dev->devfreq.devfreq = ctx->devfreq;

	ctx->nb.notifier_call = GB02FUNC740;
	blocking_notifier_chain_register(&limit_freq_notifier_chain, &ctx->nb);

	g_ctx = ctx;
	peri_info->priv = ctx;

	ret = sysfs_create_group(&ctx->devfreq->dev.kobj, &gb_gpufreq_attr_group);
	if (ret)
		dev_err(&ctx->devfreq->dev, "failed to register sysfs\n");

	return ret;
}

static void GB02FUNC751(void)
{
	blocking_notifier_chain_unregister(&limit_freq_notifier_chain, &g_ctx->nb);
}

static void GB02FUNC752(struct GB02STR72 *peri_info)
{
	GB02FUNC546();
	GB02FUNC597();
	GB02FUNC751();
	return;
}

int GB02FUNC754(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	int ret;
	ret = GB02FUNC595();
	ret = GB02FUNC533();
	ret = GB02FUNC745(gb_dev, peri_info);
	return ret;
}

void GB02FUNC755(struct GB02STR72 *peri_info)
{
	GB02FUNC752(peri_info);
}
