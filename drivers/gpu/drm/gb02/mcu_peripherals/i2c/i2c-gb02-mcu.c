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
#include <asm/irq.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/cdev.h>
#include <linux/regmap.h>
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
#include <linux/seq_file.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include "common/gb-peripherals-common.h"
#include "gpu/gb_device.h"
#include "i2c-gb02-mcu.h"
//#define I2C_DEBUG
#ifdef I2C_DEBUG
#define GB02MAC1126(arg...) gb_printf(KERN_INFO, "[" PLATFORM_I2C_DEVICE_NAME "] " arg)
#else
#define GB02MAC1126(arg...)
#endif
//#define I2C_TEST
static struct GB02STR133 gb02_i2c_infos[4];

static const struct GB02STR131 gb02_i2c_extra[] = {
	{.frequency = 100},
	{.frequency = 100},
	{.frequency = 100},
	{.frequency = 100}
};

static inline void GB02FUNC803(unsigned int val,
		struct GB02STR132 *i2c, unsigned int reg)
{
	writel(val, i2c->regs + reg);
	//GB02MAC1126("addr %x reg %x\n",i2c->regs + reg,val);
}

static inline unsigned long GB02FUNC805(struct GB02STR132 *i2c,
		unsigned int reg)
{
	//unsigned int val;
	return readl(i2c->regs + reg);
	//val=readl(i2c->regs + reg);
	//GB02MAC1126("addr %x reg %x\n",i2c->regs + reg,val);
	//return val;
}

static inline void GB02FUNC806(struct GB02STR132 *i2c)
{
	GB02FUNC803(0, i2c, GB02MAC1153);
}

static inline void GB02FUNC807(struct GB02STR132 *i2c)
{
	unsigned long tmp;

	tmp = readl(i2c->regs + GB02MAC1150);
	writel(tmp | GB02MAC1173, i2c->regs + GB02MAC1150);
}

static inline void GB02FUNC808(struct GB02STR132 *i2c)
{
	unsigned long tmp;

	tmp = readl(i2c->regs + GB02MAC1150);
	writel(tmp & ~GB02MAC1173, i2c->regs + GB02MAC1150);
}

static inline void GB02FUNC810(struct GB02STR132 *i2c)
{
	unsigned long tmp;

	tmp = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(tmp | GB02MAC1169, i2c, GB02MAC1150);
}

struct i2c_client *
gb02_i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info)
{
	struct i2c_client	*client;
	int	status = -1;

	client = kzalloc(sizeof *client, GFP_KERNEL);
	if (!client)
		return NULL;

	client->adapter = adap;

	client->dev.platform_data = info->platform_data;

	client->flags = info->flags;
	client->addr = info->addr;

	strlcpy(client->name, info->type, sizeof(client->name));

	if (client->addr == 0x00 || client->addr > 0x7f)
		goto out_err;

	client->dev.parent = &client->adapter->dev;
	client->dev.bus = &i2c_bus_type;
	//client->dev.type = &i2c_client_type;
	client->dev.of_node = info->of_node;
	client->dev.fwnode = info->fwnode;

	dev_set_name(&client->dev, "%d-%04x", i2c_adapter_id(adap), client->addr);

	status = device_register(&client->dev);
	if (status)
		goto out_err;

	dev_dbg(&adap->dev, "client [%s] registered with bus id %s\n",
		client->name, dev_name(&client->dev));

	return client;

out_err:
	dev_err(&adap->dev,
		"Failed to register i2c client %s at 0x%02x (%d)\n",
		client->name, client->addr, status);
	kfree(client);
	return NULL;
}

static bool GB02FUNC815(struct GB02STR132 *i2c)
{
	int tries;

	for (tries = 5; tries; --tries) {
		if (!(readl(i2c->regs + GB02MAC1148) &
			  GB02MAC1158))
			return true;
		usleep_range(50, 100);
	}
	dev_err(i2c->dev, "ack was not received\n");
	return false;
}

static int GB02FUNC817(struct GB02STR132 *i2c)
{
	unsigned long iicstat;
	int timeout = 400;

	while (timeout-- > 0) {
		iicstat = GB02FUNC805(i2c, GB02MAC1148);

		if (!(iicstat & GB02MAC1160))
			return 0;

		usleep_range(50, 100);
	}

	return -ETIMEDOUT;
}

#ifdef I2C_TEST
/*
 * get the i2c bus for a master transaction
 */
static int GB02FUNC820(struct GB02STR132 *i2c)
{
	unsigned long iicstat;
	int timeout = 400;

	while (timeout-- > 0) {
		iicstat = GB02FUNC805(i2c, GB02MAC1148);

		if (!(iicstat & GB02MAC1156))
			return 0;

		usleep_range(50, 100);
	}

	return -ETIMEDOUT;
}
#endif

static int GB02FUNC822(struct GB02STR132 *i2c, int for_busy)
{
	unsigned long orig_jiffies = jiffies;
	unsigned int temp;

	while (1) {
		temp = GB02FUNC805(i2c, GB02MAC1148);

		/* check for arbitration lost */
		if (temp & GB02MAC1157) {
			dev_err(&i2c->adap.dev,
					"<%s> arbitration lost\n", __func__);
			return -EAGAIN;
		}

		if (for_busy && !(temp & GB02MAC1156))
			break;

		if (time_after(jiffies, orig_jiffies + msecs_to_jiffies(500))) {
			dev_dbg(&i2c->adap.dev,
					"<%s> I2C bus is busy\n", __func__);
			return -ETIMEDOUT;
		}
		schedule();
	}

	return 0;
}

static int GB02FUNC826(struct GB02STR132 *i2c)
{
	int ret = 0;

	GB02FUNC807(i2c);
	ret = GB02FUNC822(i2c, 1);
	if (ret)
		return ret;

	GB02FUNC810(i2c);

	return ret;
}

static int GB02FUNC829(struct GB02STR132 *i2c)
{
	unsigned int val = 0;
	int ret = 0;

	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val | GB02MAC1170, i2c, GB02MAC1150);

#ifdef I2C_TEST
	int i = 0;
	for (i = 0; i < GB02MAC1184; i++) {
		ret = GB02FUNC820(i2c);
		if (ret != 0)
			ret = -1;
		GB02FUNC807(i2c);
		GB02FUNC810(i2c);

		/* write slave address */
		GB02FUNC803(i2c->slave_addr, i2c, GB02MAC1151);

		val = GB02FUNC805(i2c, GB02MAC1150);
		GB02FUNC803(val | GB02MAC1171, i2c, GB02MAC1150);
		ret = GB02FUNC817(i2c);
		if (ret < 0)
			/* gb_printf(KERN_ERR, "I2C%d:transfer is on going\n", i2c->busno); */
			ret = -2;
		ret = GB02FUNC815(i2c);
		if (!ret)
			/* gb_printf(KERN_ERR, "I2C%d:no ACK occurs\n", i2c->busno); */
			ret = -3;
		else {
			ret = 0;
			break;
		}

		/* STOP */
		val = GB02FUNC805(i2c, GB02MAC1150);
		GB02FUNC803(val | GB02MAC1170, i2c, GB02MAC1150);

		mdelay(1);
	}

	if (ret < 0)
		gb_printf(KERN_ERR, "%s:%d:stop error(%d)\n", __func__, __LINE__, ret);

	/* STOP */
	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val | GB02MAC1170, i2c, GB02MAC1150);
#endif

	return ret;
}

static u8 GB02FUNC835(struct i2c_msg *msg)
{
	return (msg->addr << 1) | (msg->flags & I2C_M_RD ? 1 : 0);
}

static int GB02FUNC836(struct GB02STR132 *i2c, struct i2c_msg *msgs)
{
	int i = 0, ret = 0;
	unsigned long val;

	dev_dbg(&i2c->adap.dev, "<%s> write slave address: addr=0x%x\n",
			__func__, GB02FUNC835(msgs));

	/* write slave address */
	GB02FUNC803(GB02FUNC835(msgs), i2c, GB02MAC1151);

	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val | GB02MAC1171, i2c, GB02MAC1150);
	ret = GB02FUNC817(i2c);
	if (ret < 0) {
		gb_printf(KERN_ERR, "I2C%d:transfer is on going\n", i2c->bus_no);
		return ret;
	}
	ret = GB02FUNC815(i2c);
	if (!ret) {
		gb_printf(KERN_ERR, "I2C%d:no ACK occurs\n", i2c->bus_no);
		ret = -1;
		return ret;
	}

	dev_dbg(&i2c->adap.dev, "<%s> write data\n", __func__);

	/* write data */
	for (i = 0; i < msgs->len; i++) {
		//		GB02MAC1126("<%s> write byte: B%d=0x%X\n",__func__, i, msgs->buf[i]);
		GB02FUNC803(msgs->buf[i], i2c, GB02MAC1151);

		val = GB02FUNC805(i2c, GB02MAC1150);
		GB02FUNC803(val | GB02MAC1171, i2c, GB02MAC1150);

		ret = GB02FUNC817(i2c);
		if (ret < 0) {
			gb_printf(KERN_ERR, "I2C%d:transfer is on going\n", i2c->bus_no);
			return ret;
		}
		ret = GB02FUNC815(i2c);
		if (!ret) {
			gb_printf(KERN_ERR, "I2C%d:no ACK occurs\n", i2c->bus_no);
			ret = -1;
			return ret;
		}

	}

	return 0;
}

static int GB02FUNC843(struct GB02STR132 *i2c, struct i2c_msg *msgs, bool is_lastmsg)
{
	int i = 0, ret = 0;
	unsigned long val;

	//GB02MAC1126("<%s> write slave address: addr=0x%x\n",__func__, GB02FUNC835(msgs));

	/* write slave address */
	GB02FUNC803(GB02FUNC835(msgs), i2c, GB02MAC1151);

	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val | GB02MAC1171, i2c, GB02MAC1150);
	ret = GB02FUNC817(i2c);
	if (ret < 0) {
		gb_printf(KERN_ERR, "I2C%d:transfer is on going\n", i2c->bus_no);
		return ret;
	}
	ret = GB02FUNC815(i2c);
	if (!ret) {
		gb_printf(KERN_ERR, "I2C%d:no ACK occurs\n", i2c->bus_no);
		ret = -1;
		return ret;
	}

	dev_dbg(&i2c->adap.dev, "<%s> setup bus\n", __func__);

	/* setup bus to read data */
	GB02FUNC808(i2c);

	dev_dbg(&i2c->adap.dev, "<%s> read data\n", __func__);
	/* read data */
	for (i = 0; i < msgs->len; i++) {
		if (i == (msgs->len - 1)) {
			if (is_lastmsg) {
				dev_dbg(&i2c->adap.dev,
						"<%s> set NOACK\n", __func__);

				GB02FUNC807(i2c);
			}
		}

		val = GB02FUNC805(i2c, GB02MAC1150);
		GB02FUNC803(val | GB02MAC1172, i2c, GB02MAC1150);

		ret = GB02FUNC817(i2c);
		if (ret < 0) {
			gb_printf(KERN_ERR, "I2C%d:transfer is on going\n", i2c->bus_no);
			return ret;
		}

		msgs->buf[i] = GB02FUNC805(i2c, GB02MAC1152);
		dev_dbg(&i2c->adap.dev,
				"<%s> read byte: B%d=0x%X\n",
				__func__, i, msgs->buf[i]);
		//GB02MAC1126("<%s> read byte: B%d=0x%X\n",
		//__func__, i, msgs->buf[i]);
	}
	return 0;
}

/*
 * this starts an i2c transfer
 */
static int GB02FUNC851(struct GB02STR132 *i2c,
						   struct i2c_msg *msgs, int num)
{
	int ret = 0;
	unsigned int i = 0;
	bool is_lastmsg = false;

	/* Start I2C transfer */
	ret = GB02FUNC826(i2c);
	if (ret) {
		if (i2c->adap.bus_recovery_info) {
			i2c_recover_bus(&i2c->adap);
			ret = GB02FUNC826(i2c);
		}
	}

	if (ret)
		goto fail0;

	i2c->slave_addr = (msgs[0].addr & 0x7f) << 1;

	/* read/write data */
	for (i = 0; i < num; i++) {
		if (i == num - 1)
			is_lastmsg = true;

		if (i) {
			dev_dbg(&i2c->adap.dev,
					"<%s> repeated start\n", __func__);
			GB02FUNC810(i2c);
		}

		dev_dbg(&i2c->adap.dev,
				"<%s> transfer message: %d\n", __func__, i);
		/* write/read data */
		if (msgs[i].flags & I2C_M_RD)
			ret = GB02FUNC843(i2c, &msgs[i], is_lastmsg);
		else
			ret = GB02FUNC836(i2c, &msgs[i]);
		if (ret)
			goto fail0;
	}
	//return 0;
fail0:
	/* Stop I2C transfer */
	ret = GB02FUNC829(i2c);
	return (ret < 0) ? ret : num;
}

/*
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
 */
static int GB02FUNC857(struct i2c_adapter *adap,
						 struct i2c_msg *msgs, int num)
{
	struct GB02STR132 *i2c = (struct GB02STR132 *)adap->algo_data;
	int retry;
	int ret;

	for (retry = 0; retry < adap->retries; retry++) {
		ret = GB02FUNC851(i2c, msgs, num);
		if (ret != -EAGAIN)
			return ret;
		dev_dbg(i2c->dev, "Retrying transmission (%d)\n", retry);
		udelay(100);
	}

	return -EREMOTEIO;
}

/* declare our i2c functionality */
static u32 GB02FUNC862(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}

/* i2c bus registration info */
static const struct i2c_algorithm gb02_i2c_algorithm = {
	.master_xfer = GB02FUNC857,
	.functionality = GB02FUNC862,
};

void GB02FUNC864(struct GB02STR132 *i2c, enum I2C_DutyTypedef duty)
{
	unsigned long val;

	val = GB02FUNC805(i2c, GB02MAC1150);
	val &= ~(GB02MAC1182);
	GB02FUNC803(val, i2c, GB02MAC1150);
	val |= duty << 18;
	GB02FUNC803(val, i2c, GB02MAC1150);
}

/*
 * work out a divisor for the user requested frequency setting,
 * either by the requested frequency, or scanning the acceptable
 * range of frequencies until something is found
 */
static int GB02FUNC866(struct GB02STR132 *i2c, unsigned int freq, enum I2C_DutyTypedef duty)
{
	unsigned int div, periph_freq_div, periph_freq;
	struct GB02STR133 *gb02_i2c = i2c->driver_data;

	dev_dbg(i2c->dev, "i2c%d desired frequency %d KHz\n", i2c->bus_no, freq);
	periph_freq_div = GB02FUNC530(gb02_i2c->sysctl_iofunc_base, GB02MAC850) & 0x7;
	periph_freq = GB02MAC757 / (periph_freq_div + 1);

	//GB02FUNC864(i2c, duty);
	//switch (duty) {
	//case I2C_30_DUTY:
	//case I2C_50_DUTY:
	//	div = ((periph_freq / freq) / 6) - 1;
	//	break;
	//case I2C_40_DUTY:
	//	div = ((periph_freq / freq) / 5) - 1;
	//	break;
	//default:
	//	break;
	//}

	div = ((periph_freq / freq) / 5) - 1;
	/* set I2C frequency */
	GB02FUNC803(div, i2c, GB02MAC1149);

	return 0;
}

static int GB02FUNC870(struct GB02STR132 *i2c)
{
	struct GB02STR133 *gb02_i2c = i2c->driver_data;
	unsigned int freq = gb02_i2c->pdata[i2c->bus_no].frequency;
	unsigned long val = 0;

	GB02FUNC806(i2c);
	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val & ~GB02MAC1168, i2c, GB02MAC1150);

	/* soft reset*/
	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val | GB02MAC1174, i2c, GB02MAC1150);
	udelay(100);
	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val & ~GB02MAC1174, i2c, GB02MAC1150);

	/* I2C/I3C mode */
	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val & ~GB02MAC1178, i2c, GB02MAC1150);

	/* clock divider */
	if (GB02FUNC866(i2c, freq, I2C_40_DUTY) != 0) {
		dev_err(i2c->dev, "cannot meet bus frequency required\n");
		return -EINVAL;
	}
	GB02MAC1126("bus frequency set to %d KHz\n", freq);

	/* I2C master mode */
	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val & ~GB02MAC1177, i2c, GB02MAC1150);

	/* write slave address */

	/* PULLUP enable */
	val = GB02FUNC805(i2c, GB02MAC1150);
	val |= (GB02MAC1180 | GB02MAC1181);
	GB02FUNC803(val, i2c, GB02MAC1150);

	/* I2C enable */
	val = GB02FUNC805(i2c, GB02MAC1150);
	GB02FUNC803(val | GB02MAC1168, i2c, GB02MAC1150);

	return 0;
}

static void GB02FUNC875(struct GB02STR133 *gb02_i2c)
{
	GB02FUNC528(GB02MAC740, gb02_i2c->sysctl_cfg_base, GB02MAC734);
	gb02_i2c->pdata = gb02_i2c_extra;
}

static int GB02FUNC878(struct GB02STR133 *gb02_i2c)
{
	int ret = 0;
	struct GB02STR132 *i2c = &gb02_i2c->adapter;
	char label[10];

	i2c->driver_data = gb02_i2c;
	snprintf(label, 11, "gb02-i2c%d", gb02_i2c->i2c_bus_no);
	strscpy(i2c->adap.name, label, sizeof(i2c->adap.name));
	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &gb02_i2c_algorithm;
	i2c->adap.retries = 1;
	i2c->adap.class = I2C_CLASS_DEPRECATED;
	i2c->tx_setup = 50;
	i2c->bus_no = gb02_i2c->i2c_bus_no; // -1 for non static bus_num

	init_waitqueue_head(&i2c->wait);
	i2c->dev = gb02_i2c->dev;

	i2c->regs = gb02_i2c->regs;
	dev_dbg(gb02_i2c->dev, "registers %p (%d)\n",
			i2c->regs, i2c->bus_no);

	/* setup info block for the i2c core */
	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = gb02_i2c->dev;

	ret = GB02FUNC870(i2c);
	if (ret != 0) {
		dev_err(gb02_i2c->dev, "I2C controller init failed\n");
		return ret;
	}

	ret = i2c_add_adapter(&i2c->adap);
	if (ret < 0) {
		dev_warn(gb02_i2c->dev, "cannot add %s I2C adapter\n", i2c->adap.name);
		return ret;
	}

	GB02MAC1126("%s: GB02 I2C adapter(%d)\n", dev_name(&i2c->adap.dev), gb02_i2c->i2c_bus_no);
	return 0;
}

static int GB02FUNC884(struct GB02STR132 *i2c, unsigned int addr,
								   unsigned int len, u8 *data)
{
	int ret = 0, i = 0;
	u8 out_buf[GB02MAC1188];
	u8 in_buf[GB02MAC1187];
	struct i2c_msg msgs[] = {
		{
			.addr = GB02MAC1186,
			.flags = 0,
			.len = GB02MAC1188,
			.buf = out_buf,
		},
		{
			.addr = GB02MAC1186,
			.flags = I2C_M_RD,
			.len = len,
			.buf = in_buf,
		}
	};

	out_buf[0] = addr & 0xff;
	out_buf[1] = addr >> 8;

	ret = i2c_transfer(&i2c->adap, msgs, 2);
	if (ret != 2) {
		gb_printf(KERN_ERR, "%s:%d:I2C transfer error(%d)\n", __func__, __LINE__, ret);
	} else {
		memcpy(data, in_buf, len);
		for (i = 0; i < len; i++)
			GB02MAC1126("read data:0x%x ", in_buf[i]);
		ret = 0;
		GB02MAC1126("%s:%d:I2C read finished\n", __func__, __LINE__);
	}

	return ret;
}

static int GB02FUNC887(struct GB02STR132 *i2c, unsigned int addr,
									unsigned int len, u8 *data)
{
	int ret = 0, i = 0;
	u8 in_buf[GB02MAC1188 + GB02MAC1187];
	struct i2c_msg msgs[] = {
		{
			.addr = GB02MAC1186,
			.flags = 0,
			.len = GB02MAC1188 + len,
			.buf = in_buf,
		}
	};

	in_buf[0] = addr & 0xff;
	in_buf[1] = addr >> 8;
	memcpy(in_buf + GB02MAC1188, data, len);

	for (i = 0; i < len + GB02MAC1188; i++)
		GB02MAC1126("write data 0x%x ", in_buf[i]);
	ret = i2c_transfer(&i2c->adap, msgs, 1);
	if (ret != 1)
		gb_printf(KERN_ERR, "%s:%d:I2C transfer error(%d)\n", __func__, __LINE__, ret);
	else
		GB02MAC1126("%s:%d:I2C write finished\n", __func__, __LINE__);

	return ret;
}

static int GB02FUNC892(struct GB02STR132 *i2c)
{
	int ret = 0;
	unsigned int start_addr = 0, len = GB02MAC1187;
	u8 rdata[GB02MAC1187] = {0},
										 wdata[GB02MAC1187] = {0};
	int i = 0;

	/* read */
	ret = GB02FUNC884(i2c, start_addr, len, rdata);
	if (ret) {
		gb_printf(KERN_ERR, "%s:%d:I2C read failed(%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	/* write */
	for (i = 0; i < len; i++)
		wdata[i] = 0x5a + i;
	ret = GB02FUNC887(i2c, start_addr, len, wdata);
	if (ret) {
		gb_printf(KERN_ERR, "%s:%d:I2C write failed(%d)\n", __func__, __LINE__, ret);
		return ret;
	}
	/* read again */
	ret = GB02FUNC884(i2c, start_addr, len, rdata);
	if (ret) {
		gb_printf(KERN_ERR, "%s:%d:I2C read failed(%d)\n", __func__, __LINE__, ret);
		return ret;
	}
	return 0;
}

static int GB02FUNC897(struct GB02STR132 *i2c)
{
	int ret = 0;
	//int i = 0;
	unsigned int start_addr = 0, len = GB02MAC1187;
	u8 rdata[GB02MAC1187] = {0};

	/* read */
	ret = GB02FUNC884(i2c, start_addr, len, rdata);

	if (ret) {
		gb_printf(KERN_ERR, "%s:%d:I2C read failed(%d)\n", __func__, __LINE__, ret);
		//for (i = 0; i < len; i++)
		//	GB02MAC1126("read data 0x%x\n", rdata[i]);
		return ret;
	}

	return 0;
}

static int GB02FUNC899(struct GB02STR132 *i2c)
{
	int ret = 0;
	unsigned int start_addr = 0, len = GB02MAC1187;
	u8 wdata[GB02MAC1187] = {0};
	int i = 0;

	/* write */
	for (i = 0; i < len; i++)
		wdata[i] = 0x5a + i;

	ret = GB02FUNC887(i2c, start_addr, len, wdata);
	if (ret) {
		gb_printf(KERN_ERR, "%s:%d:I2C write failed(%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	return 0;
}

static int GB02FUNC251(struct inode *inode, struct file *filp)
{
	filp->private_data = gb02_i2c_infos;
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
	struct GB02STR133 *mcu_i2c = filp->private_data;
	struct GB02STR132 *i2c;

	GB02MAC1126("enter ioctrl!\n");

	switch (cmd) {
	case I2C_MCU_CMD_I2C0:
		GB02MAC1126("=====MCU's I2C0 start!======\n");
		i2c = &mcu_i2c[0].adapter;
		ret = GB02FUNC892(i2c);
		break;
	case I2C_MCU_CMD_I2C1:
		GB02MAC1126("=====MCU's I2C1 start!======\n");
		i2c = &mcu_i2c[1].adapter;
		ret = GB02FUNC892(i2c);
		break;
	case I2C_MCU_CMD_I2C2:
		GB02MAC1126("=====MCU's I2C2 start!======\n");
		i2c = &mcu_i2c[2].adapter;
		ret = GB02FUNC892(i2c);
		break;
	case I2C_MCU_CMD_I2C3:
		GB02MAC1126("=====MCU's I2C3 start!======\n");
		i2c = &mcu_i2c[3].adapter;
		ret = GB02FUNC892(i2c);
		break;
	case I2C_MCU_CMD_I2C0_RD:
		GB02MAC1126("=====MCU's I2C0 read start!======\n");
		i2c = &mcu_i2c[0].adapter;
		ret = GB02FUNC897(i2c);
		break;
	case I2C_MCU_CMD_I2C0_WR:
		GB02MAC1126("=====MCU's I2C0 write start!======\n");
		i2c = &mcu_i2c[0].adapter;
		ret = GB02FUNC899(i2c);
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
	.name = "gbfpga_i2c",
	.minor = MISC_DYNAMIC_MINOR,
};

static int GB02FUNC907(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info, unsigned int device_id)
{
	struct GB02STR133 *gb02_i2c;
	struct GB02STR70 *pcie_info;
	int mcu_peri_bar_id, peri_base_bar_id;
	int id, ret = 0;

	GB02MAC1126("Start to probe GB02 MCU's I2C:%d\n", device_id);
	pcie_info = gb_dev->gb_pcie;
	id = device_id;
	gb02_i2c = devm_kzalloc(gb_dev->dev, sizeof(struct GB02STR133), GFP_KERNEL);
	if (gb02_i2c == NULL)
		return -ENOMEM;
	gb02_i2c->dev = gb_dev->dev;
	gb02_i2c->i2c_bus_no = id;

	mcu_peri_bar_id = GB02FUNC474(pcie_info->GB02STR153);
	peri_base_bar_id = GB02FUNC477(pcie_info->GB02STR153);
	gb02_i2c->sysctl_iofunc_base = gb_dev->gb_pcie->pci_bars[mcu_peri_bar_id].mmio; // bar0
	gb02_i2c->sysctl_cfg_base = gb_dev->gb_pcie->pci_bars[peri_base_bar_id].mmio; //bar4
	gb02_i2c->regs = gb02_i2c->sysctl_iofunc_base + GB02MAC748 + GB02MAC806 + GB02MAC821 * id;
	GB02FUNC875(gb02_i2c);
	ret = GB02FUNC878(gb02_i2c);
	if (ret)
		return ret;
	memcpy(&gb02_i2c_infos[id], gb02_i2c, sizeof(struct GB02STR133));
	peri_info->priv = gb02_i2c;
	GB02MAC1126("GB02 MCU's I2C initialized\n");

	return ret;
}

static void GB02FUNC909(struct GB02STR72 *peri_info)
{
	struct GB02STR133 *gb02_i2c = (struct GB02STR133 *)peri_info->priv;

	i2c_del_adapter(&(gb02_i2c->adapter.adap));
	memset(gb02_i2c_infos, 0, sizeof(gb02_i2c_infos));
}

int GB02FUNC910(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	int ret;
	int device_id = 0;
	static bool i2c_misc_inited;

	GB02MAC1126("mcu_i2c_drv_init\n");
	if (!i2c_misc_inited) {
		ret = misc_register(&gbfpga_misc);
		if (ret)
			dev_err(gb_dev->dev, "unable to register i2c misc device(%d)\n", ret);
		else
			i2c_misc_inited = true;
	}
	device_id = peri_info->mcu_peripherals_device_id;
	ret = GB02FUNC907(gb_dev, peri_info, device_id);
	return ret;
}

void GB02FUNC913(struct GB02STR72 *peri_info)
{
	GB02FUNC909(peri_info);
	misc_deregister(&gbfpga_misc);
	GB02MAC1126("Bye!\n");
}
