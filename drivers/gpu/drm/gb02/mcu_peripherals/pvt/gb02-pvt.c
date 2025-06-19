// SPDX-License-Identifier: GPL-2.0
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/reboot.h>
#include <linux/workqueue.h>
#include <linux/pci.h>
#include "gpu/gb_device.h"
#include "gb02-pvt.h"
#include "mcu_peripherals/gpu_freq/gpu_freq.h"

//#define PVT_TEST
#ifdef PVT_DEBUG
#define GB02MAC1331(arg...) gb_printf(KERN_INFO, "[" PLATFORM_PVT_DEVICE_NAME "] " arg)
#else
#define GB02MAC1331(arg...)
#endif

#define GB02MAC1334 0x150
#define GB02MAC1337 0x154
#define GB02MAC1340 0x158
#define GB02MAC1342 0x15C

#define GB02MAC1345	30000	/* 30C */
#define GB02MAC1348	50000	/* 50C */
#define GB02MAC1350	70000	/* 70C */
#define GB02MAC1352	90000	/* 90C */
#define GB02MAC1354	110000	/* 110C */
#define GB02MAC1356	0	/* 0C */

#define GB02MAC1360 (0x0005)
#define GB02MAC1363 (0x0008)
#define GB02MAC1366 (0x000B)
#define GB02MAC1369 (0x000C)

#define GB02MAC1372	4
#define GB02MAC1374 2000

static bool power_off_triggered;
static DEFINE_MUTEX(poweroff_lock);

enum gb02_pvt_thermal_trips {
	GB02_PVT_THERMAL_TEMP_TRIP_COOL,
	GB02_PVT_THERMAL_TEMP_TRIP_NORM,
	GB02_PVT_THERMAL_TEMP_TRIP_HIGH,
	GB02_PVT_THERMAL_TEMP_TRIP_HOT,
	GB02_PVT_THERMAL_TEMP_TRIP_CRIT,
};

struct GB02STR142 {
	int type;
	int temp;
	int hyst;
	int min_state;
	int max_state;
};
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static const struct GB02STR142 default_thermal_trips[] = {
	{
		.type		= THERMAL_TRIP_ACTIVE,
		.temp		= GB02MAC1345,
		.hyst		= GB02MAC1356,
		.min_state	= 0,
		.max_state	= 1,
	},
	{
		.type		= THERMAL_TRIP_ACTIVE,
		.temp		= GB02MAC1348,
		.hyst		= GB02MAC1356,
		.min_state	= 1,
		.max_state	= 2,
	},
	{
		.type		= THERMAL_TRIP_ACTIVE,
		.temp		= GB02MAC1350,
		.hyst		= GB02MAC1356,
		.min_state	= 2,
		.max_state	= 3,
	},
	{	/* Warning */
		.type		= THERMAL_TRIP_ACTIVE,
		.temp		= GB02MAC1352,
		.hyst		= GB02MAC1356,
		.min_state	= GB02MAC1372,
		.max_state	= GB02MAC1372,
	},
	{	/* Critical - soft poweroff */
		.type		= THERMAL_TRIP_HOT,
		.temp		= GB02MAC1354,
		.min_state	= GB02MAC1372,
		.max_state	= GB02MAC1372,
	}
};
#else
static struct thermal_trip default_thermal_trips[] = {
	{
		.type		= THERMAL_TRIP_ACTIVE,
		.temperature	= GB02MAC1345,
		.hysteresis	= GB02MAC1356,
	},
	{
		.type		= THERMAL_TRIP_ACTIVE,
		.temperature	= GB02MAC1348,
		.hysteresis	= GB02MAC1356,
	},
	{
		.type		= THERMAL_TRIP_ACTIVE,
		.temperature	= GB02MAC1350,
		.hysteresis	= GB02MAC1356,
	},
	{	/* Warning */
		.type		= THERMAL_TRIP_ACTIVE,
		.temperature	= GB02MAC1352,
		.hysteresis	= GB02MAC1356,
	},
	{	/* Critical - soft poweroff */
		.type		= THERMAL_TRIP_HOT,
		.temperature	= GB02MAC1354,
	}
};
#endif

#define GB02MAC1389	ARRAY_SIZE(default_thermal_trips)
#define GB02MAC1391	(BIT(GB02MAC1389) - 1)

static struct thermal_zone_params gb02_pvt_zone_params = {
	.governor_name = "step_wise",
};
/**
 * struct GB02STR146 - gb02_pvt thermal sensor private structure
 * @pvt_tzd: struct thermal_zone_device where the sensor is registered
 * @lock: prevents read sensor in parallel
 * @regs: pointer to base address of the thermal sensor
 * @dev: struct device pointer
 */

struct GB02STR146 {
	struct thermal_zone_device *pvt_tzd;
	struct mutex lock;
	void __iomem *regs;
	void __iomem *sysctl_iofunc_base; // bar0
	void __iomem *gb_comp_irq_reg_bar_base;
	struct delayed_work work;
	int shutdown_temp;
	struct device *dev;
	struct GB02STR142 trips[GB02MAC1389];
};

static int GB02FUNC1157(struct GB02STR146 *priv, unsigned char mode);
static u16 GB02FUNC1147(struct GB02STR146 *priv);

static unsigned long long GB02FUNC1141(unsigned short dout_act_in)
{
	unsigned int Kcal = 0, Tcal = 0;
	unsigned long long ret_cal = 0;
	struct GB02STR146 priv;
	struct GB02STR70 *gb_pcie = GB02FUNC518();
	struct pci_dev *pdev = gb_pcie->pdev;

	if (gb_pcie == NULL)
		return 0;

	priv.regs = gb_pcie->pci_bars[GB02MAC1604].mmio + (GB02MAC1601 - GB02MAC1602);

	/* Reinitialize pvt when get register data 0, Fix bug GB2EN-1431 */
	if (dout_act_in == 0) {
		GB02FUNC1157(&priv, GB02MAC1637);
		dout_act_in = GB02FUNC1147(&priv);
	}

	if (D1 < dout_act_in && 4095 >= dout_act_in) {
		GB02MAC1331("D1-4096 (%lld-4096): %d\n", D1, dout_act_in);
		Kcal = 198;
		Tcal = -16;
	} else if ((DOUT75act < dout_act_in) && (dout_act_in <= D1)) {
		GB02MAC1331("DOUTact75-D1(%lld-%lld: %d)\n", DOUT75act, D1, dout_act_in);
		Kcal = 41;
		Tcal = -3;
	} else if ((dout_act_in > D2) && (dout_act_in <= DOUT75act)) {
		GB02MAC1331("D2-DOUTact75(%lld-%lld: %d)\n", D2, DOUT75act, dout_act_in);
		Kcal = -111;
		Tcal = 7;
	} else if ((DOUT25act < dout_act_in) && (dout_act_in <= D2)) {
		GB02MAC1331("DOUTact25-D2(%lld-%lld: %d)\n", DOUT25act, D2, dout_act_in);
		Kcal = 111;
		Tcal = -2;
	} else if ((dout_act_in >= D3) && (DOUT25act >= dout_act_in)) {
		GB02MAC1331("D3-DOUTact25(%lld-%lld: %d)\n", D3, DOUT25act, dout_act_in);
		Kcal = -233;
		Tcal = 5;
	} else if (D4 <= dout_act_in && D3 > dout_act_in) {
		GB02MAC1331("D4-D3(%lld-%lld: %d)\n", D4, D3, dout_act_in);
		Kcal = -250;
		Tcal = 5;
	} else if (0 <= dout_act_in && D4 > dout_act_in) {
		GB02MAC1331("0-D4(0-%lld: %d)\n", D4, dout_act_in);
		Kcal = -267;
		Tcal = 5;
	}
	ret_cal = (DOUT75std - DOUT25std) * (dout_act_in * 8192 - (8192 + Tcal) * (3 * DOUT25act - DOUT75act) / 2) / ((8192 + Kcal) * (DOUT75act - DOUT25act)) + (3 * DOUT25std - DOUT75std) / 2;
	if ((pdev->subsystem_device == GB02MAC1360) || (pdev->subsystem_device == GB02MAC1363)
			|| (pdev->subsystem_device == GB02MAC1369) || (pdev->subsystem_device == GB02MAC1366))
		/* 
		 * DOUT = (T / 44 + 27049) / 10
		 * thermal resistance compensation 8°C (8000 / 44 /10) = 18.1
		 */
		ret_cal += 18;
	GB02MAC1331("RET_CAL:%lld", ret_cal);
	return ret_cal;
}

static int GB02FUNC1143(struct GB02STR146 *priv, unsigned char power)
{
	void __iomem *regs;
	u32 val;
	int ret;

	regs = priv->regs;
	val = readl_relaxed(regs + GB02MAC1617);
	if (power == GB02MAC1629) {
		val &= ~GB02MAC1625; // power down
		ret = GB02MAC1629;
	} else {
		val |= GB02MAC1625; // power on
		ret = GB02MAC1627;
	}
	writel_relaxed(val, regs + GB02MAC1617);
	val = readl_relaxed(regs + GB02MAC1617);
	GB02MAC1331("REG_00:%x\n", val);
	return ret;
}
#ifdef PVT_TEST
static int GB02FUNC1144(struct GB02STR146 *priv, unsigned char mode)
{
	void __iomem *regs;
	u32 val = 0;
	int ret;

	regs = priv->regs;
	if (mode == GB02MAC1639) {
		val &= ~GB02MAC1635;
		ret = GB02MAC1639;
	} else if (mode == GB02MAC1637) {
		val |= GB02MAC1635;
		ret = GB02MAC1637;
	}
	writel_relaxed(val, regs + GB02MAC1617);
	return ret;
}
#endif
static void GB02FUNC1145(struct GB02STR146 *priv)
{

	void __iomem *regs;
	u32 val;

	regs = priv->regs;
	val = readl_relaxed(regs + GB02MAC1617);
	val &= ~PVT_RESETn_MASK;
	writel_relaxed(val, regs + GB02MAC1617);
}

static void GB02FUNC1146(struct GB02STR146 *priv)
{

	void __iomem *regs;
	u32 val;

	regs = priv->regs;
	val = readl_relaxed(regs + GB02MAC1617);
	val |= PVT_RESETn_MASK;
	writel_relaxed(val, regs + GB02MAC1617);
}
static u16 GB02FUNC1147(struct GB02STR146 *priv)
{
	void __iomem *regs;
	static u32 old_dout;
	u32 val;
	u16 ret;

	regs = priv->regs;
	val = readl_relaxed(regs + GB02MAC1698);
	ret = DOUT(val);
	if (ret == DOUT(0xffffffff)) {
		gb_printf(KERN_ERR, "%s GB02MAC1698:%x\n", __func__, ret);
		return old_dout;
	}
	old_dout = ret;
	return ret;
}
#ifdef VOLTAGE
static u8 GB02FUNC1148(struct GB02STR146 *priv)
{
#ifdef PVT_TEST
	void __iomem *regs;
	u32 val;
	u8 trimval;
	u16 dout;

	regs = priv->regs;
	val = readl_relaxed(regs + GB02MAC1617);
	// by following steps ,we can get a bigger DOUTacl , in case overvoltage
	while (GB02FUNC1147(priv) > DOUTstd) {
		val++;
		writel_relaxed(val, regs + GB02MAC1617);
		trimval = BAND_GAP_TRIM(val);
		msleep(20);
	}
	while (GB02FUNC1147(priv) < DOUTstd) {
		val--;
		writel_relaxed(val, regs + GB02MAC1617);
		trimval = BAND_GAP_TRIM(val);
		msleep(20);
	}
out:
#endif
	return trimval;
}
#endif

#ifdef PVT_TEST
static int GB02FUNC1150(struct GB02STR146 *priv, unsigned int low, unsigned int high)
{
	void __iomem *regs;
	u32 val;

	regs = priv->regs;
	val = readl_relaxed(regs + GB02MAC1617);
	val |= (GB02MAC1619(high) | GB02MAC1623(low));
	writel_relaxed(val, regs + GB02MAC1617);

	return 0;
}
#endif

#ifdef PVT_TEST
static void GB02FUNC1152(struct GB02STR146 *priv, unsigned int low, unsigned int high)
{
	void __iomem *regs;
	u32 val;

	regs = priv->regs;
	val = readl_relaxed(regs + GB02MAC1643);
	val |= (GB02MAC1647(high) | GB02MAC1651(low));
	writel_relaxed(val, regs + GB02MAC1643);
}
#endif

#ifdef HAS_PVT_IRQ
static void GB02FUNC1155(struct GB02STR146 *priv)
{
	void __iomem *regs;
	u32 val;

	regs = priv->regs;
	val = readl_relaxed(regs + GB02MAC1682);
	val |= (GB02MAC1685 | GB02MAC1688);
	writel_relaxed(val, regs + GB02MAC1682);
}
#endif
static int GB02FUNC1157(struct GB02STR146 *priv, unsigned char mode)
{
	void __iomem *regs;
	int ret = 0;

	regs = priv->regs;
	GB02FUNC1143(priv, GB02MAC1627);
//	GB02FUNC1144(priv, mode);
//	GB02FUNC1150(priv, GB02MAC1614, GB02MAC1612);
//	GB02FUNC1152(priv, GB02MAC1614, GB02MAC1612);
#ifdef VOLTAGE
	GB02FUNC1148(priv);
#endif

	GB02FUNC1146(priv);
	usleep_range(10, 20);
	GB02FUNC1145(priv);
	usleep_range(10, 20);
	GB02FUNC1146(priv);
	return ret;
}

int GB02FUNC1158(void)
{
	long long tempval;
	u16 outdata;
	struct GB02STR146 priv;
	struct GB02STR70 *gb_pcie = GB02FUNC518();

	if (gb_pcie == NULL)
		return 0;

	priv.regs = gb_pcie->pci_bars[GB02MAC1604].mmio + (GB02MAC1601 - GB02MAC1602);
	GB02FUNC1157(&priv, GB02MAC1637);
	outdata = GB02FUNC1147(&priv);
	tempval = GB02FUNC1141(outdata);
	return ((tempval * 10 - 27049) * 44) / 1000;
}

static int GB02FUNC1161(struct GB02STR146 *thermal,
								   struct thermal_cooling_device *cdev)
{
	int ret;

	ret = strcmp(cdev->type, "pwm-fan");
	if (ret)
		ret = strcmp(cdev->type, "gb02-freq");

	return ret;
}

static int GB02FUNC1163(struct thermal_zone_device *tzdev,
								 struct thermal_cooling_device *cdev)
{
	struct GB02STR146 *thermal = tzdev->devdata;
	struct device *dev = thermal->dev;
	int i, err;

	/* If the cooling device is one of ours bind it */
	if (GB02FUNC1161(thermal, cdev))
		return 0;

	for (i = 0; i < GB02MAC1389; i++) {
		const struct GB02STR142 *trip = &thermal->trips[i];

		err = thermal_zone_bind_cooling_device(tzdev, i, cdev,
											   trip->max_state,
											   trip->min_state,
											   THERMAL_WEIGHT_DEFAULT);
		if (err < 0) {
			dev_err(dev, "Failed to bind cooling device to trip %d\n", i);
			return err;
		}
	}
	return 0;
}

static int GB02FUNC1166(struct thermal_zone_device *tzdev,
								   struct thermal_cooling_device *cdev)
{
	struct GB02STR146 *thermal = tzdev->devdata;
	struct device *dev = thermal->dev;
	int i;
	int err;

	/* If the cooling device is our one unbind it */
	if (GB02FUNC1161(thermal, cdev))
		return 0;

	for (i = 0; i < GB02MAC1389; i++) {
		err = thermal_zone_unbind_cooling_device(tzdev, i, cdev);
		if (err < 0) {
			dev_err(dev, "Failed to unbind cooling device\n");
			return err;
		}
	}
	return 0;
}

/**
 * thermal_emergency_poweroff_func - emergency poweroff work after a known delay
 * @work: work_struct associated with the emergency poweroff function
 *
 */
static void GB02FUNC1167(void)
{
	/*
	 * We have reached here after the emergency thermal shutdown
	 * Waiting period has expired. This means orderly_poweroff has
	 * not been able to shut off the system for some reason.
	 * Try to shut down the system immediately using kernel_power_off
	 * if populated
	 */
	WARN(1, "Attempting kernel_power_off: Temperature too high\n");
	kernel_power_off();

	/*
	 * Worst of the worst case trigger emergency restart
	 */
	WARN(1, "Attempting emergency_restart: Temperature too high\n");
	emergency_restart();
}

static void GB02FUNC1168(struct work_struct *work)
{
	struct GB02STR146 *priv =
			container_of(work, struct GB02STR146, work.work);

	if (priv->shutdown_temp > GB02MAC1354)
	{
		//shutdown
		dev_emerg(&priv->pvt_tzd->device,
			"critical temperature reached (%d C), shutting down\n",
			priv->shutdown_temp / 1000);
		mutex_lock(&poweroff_lock);
		if (!power_off_triggered) {
			/*
			 * Queue a backup emergency shutdown in the event of
			 * orderly_poweroff failure
			 */
			GB02FUNC1167();
			orderly_poweroff(true);
			power_off_triggered = true;
		}
		mutex_unlock(&poweroff_lock);
	}
}

static int GB02FUNC1170(struct thermal_zone_device *thermal, int *temp)
{
	struct GB02STR146 *priv = thermal->devdata;
	long long tempval;
	int ret = 0;
	u16 outdata;
	bool temp_exceed_threshold;
	static bool trip_hot = false;

	if (thermal->trips_disabled)
		thermal->trips_disabled = 0;

	if (!priv->pvt_tzd)
		return -EAGAIN;
	msleep(20);
	mutex_lock(&priv->lock);
//	GB02FUNC1144(priv, GB02MAC1637);
	outdata = GB02FUNC1147(priv);

	GB02MAC1331("DOUTact: %d\n", outdata);

	tempval = GB02FUNC1141(outdata);

	GB02MAC1331("DOUTcal: %lld\n", tempval);
	/*
	 * Temperature calulation:
	 * Dout = 2.2632 * T + 2704.9
	 * To avoid float-poing arithmetic,increase
	 * the value of Temperature by a factor of
	 * 1000:
	 * T = (10*DOUT - 27049)*44
	 */
	if (thermal->emul_temperature)
		*temp = thermal->emul_temperature;
	else
		*temp = (tempval * 10 - 27049) * 44; // 441 -> 1000/2.2632

	GB02MAC1331("caculate temp: %d\n", *temp);
	priv->shutdown_temp = *temp;
	mutex_unlock(&priv->lock);

	temp_exceed_threshold = (*temp > GB02MAC1352);
	if (temp_exceed_threshold && !trip_hot) {
		trip_hot = true;
		blocking_notifier_call_chain(&limit_freq_notifier_chain, GB02MAC1110, NULL);
	} else if (!temp_exceed_threshold && trip_hot) {
		trip_hot = false;
		blocking_notifier_call_chain(&limit_freq_notifier_chain, GB02MAC1109, NULL);
	}

	if (*temp > GB02MAC1354)
		schedule_delayed_work(&priv->work, msecs_to_jiffies(GB02MAC1374));

	return ret;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static int gb02_pvt_thermal_get_trip_type(struct thermal_zone_device *tzdev,
		int trip,
		enum thermal_trip_type *p_type)
{
	struct GB02STR146 *thermal = tzdev->devdata;

	if (trip < 0 || trip >= GB02MAC1389)
		return -EINVAL;

	*p_type = thermal->trips[trip].type;
	return 0;
}

static int gb02_pvt_thermal_get_trip_temp(struct thermal_zone_device *tzdev,
		int trip, int *p_temp)
{
	struct GB02STR146 *thermal = tzdev->devdata;

	if (trip < 0 || trip >= GB02MAC1389)
		return -EINVAL;

	*p_temp = thermal->trips[trip].temp;
	return 0;
}
#endif
static int GB02FUNC1173(struct thermal_zone_device *tzdev,
		int trip, int temp)
{
	struct GB02STR146 *thermal = tzdev->devdata;

	if (trip < 0 || trip >= GB02MAC1389)
		return -EINVAL;

	thermal->trips[trip].temp = temp;
	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static int gb02_pvt_thermal_get_trip_hyst(struct thermal_zone_device *tzdev,
		int trip, int *p_hyst)
{
	struct GB02STR146 *thermal = tzdev->devdata;

	*p_hyst = thermal->trips[trip].hyst;
	return 0;
}
#endif

static int GB02FUNC1174(struct thermal_zone_device *tzdev,
		int trip, int hyst)
{
	struct GB02STR146 *thermal = tzdev->devdata;

	thermal->trips[trip].hyst = hyst;
	return 0;
}

#if (!IS_ENABLED(CONFIG_THERMAL_EMULATION))
static ssize_t
emul_temp_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct thermal_zone_device *tz =
				container_of(dev, struct thermal_zone_device, device);
	int temperature;

	if (kstrtoint(buf, 10, &temperature))
		return -EINVAL;

	mutex_lock(&tz->lock);
	tz->emul_temperature = temperature;
	mutex_unlock(&tz->lock);

	return count;
}
static DEVICE_ATTR_WO(emul_temp);
#endif

static struct thermal_zone_device_ops gb02_pvt_thermal_ops = {
	.bind = GB02FUNC1163,
	.unbind = GB02FUNC1166,
	.get_temp = GB02FUNC1170,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)	
	.get_trip_type	= gb02_pvt_thermal_get_trip_type,
	.get_trip_temp	= gb02_pvt_thermal_get_trip_temp,
#endif	
	.set_trip_temp	= GB02FUNC1173,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)	
	.get_trip_hyst	= gb02_pvt_thermal_get_trip_hyst,
#endif	
	.set_trip_hyst	= GB02FUNC1174,
};

static int GB02FUNC1176(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info, unsigned int device_id)
{
	struct GB02STR146 *priv;
	struct GB02STR70 *pcie_info;
	int mcu_peri_bar_id;
	int id, ret = 0;

	GB02MAC1331("Start to probe GB02 PVT\n");

	pcie_info = gb_dev->gb_pcie;
	id = device_id;
	priv = devm_kzalloc(gb_dev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	mcu_peri_bar_id = GB02FUNC474(pcie_info->GB02STR153);
	priv->regs = gb_dev->gb_pcie->pci_bars[GB02MAC1604].mmio + (GB02MAC1601 - GB02MAC1602);
	priv->sysctl_iofunc_base = gb_dev->gb_pcie->pci_bars[mcu_peri_bar_id].mmio; // bar0
	priv->gb_comp_irq_reg_bar_base = gb_dev->gb_pcie->pci_bars[GB02MAC721].mmio; //bar1

	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);

	mutex_init(&priv->lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)	
	priv->pvt_tzd = thermal_zone_device_register("gb02_pvt-thermal.0", GB02MAC1389, GB02MAC1391, priv, &gb02_pvt_thermal_ops, &gb02_pvt_zone_params, 0, 1000);
#else
	priv->pvt_tzd = thermal_zone_device_register_with_trips("gb02_pvt-thermal.0", default_thermal_trips, GB02MAC1389, GB02MAC1391, priv, &gb02_pvt_thermal_ops, &gb02_pvt_zone_params, 0, 1000);
#endif
	if (IS_ERR(priv->pvt_tzd)) {
		ret = PTR_ERR(priv->pvt_tzd);
		dev_err(gb_dev->dev, "failed to register sensor: %d\n", ret);
		return ret;
	}

	GB02MAC1331("regist thermal: %s\n", priv->pvt_tzd->type);
	memcpy(priv->trips, default_thermal_trips, sizeof(priv->trips));
	priv->dev = gb_dev->dev;
	peri_info->priv = priv;
	GB02FUNC1157(priv, GB02MAC1637);
	INIT_DELAYED_WORK(&priv->work, GB02FUNC1168);
#if (!IS_ENABLED(CONFIG_THERMAL_EMULATION))
	ret = sysfs_create_file(&priv->pvt_tzd->device.kobj, &dev_attr_emul_temp.attr);
	if (ret)
		dev_err(gb_dev->dev, "Failed to register emul_temp node\n");
#endif
	return ret;
}

static int GB02FUNC1180(struct GB02STR72 *peri_info)
{
	struct GB02STR146 *priv = (struct GB02STR146 *)peri_info->priv;

	GB02FUNC1145(priv);
	GB02FUNC1143(priv, GB02MAC1629);
	thermal_zone_device_unregister(priv->pvt_tzd);
	return 0;
}

int GB02FUNC1181(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	int ret;
	int device_id = 0;

	GB02MAC1331("pvt sensor register\n");
	device_id = peri_info->mcu_peripherals_device_id;
	ret = GB02FUNC1176(gb_dev, peri_info, device_id);

	return ret;
}

void GB02FUNC1182(struct GB02STR72 *peri_info)
{
	GB02FUNC1180(peri_info);
}
