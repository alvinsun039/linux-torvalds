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
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include "mwv207d_drv.h"
#include "mwv207d_vi.h"

static int i2c_scl_gpio[6] = {34, 36, 38, 40, 42, 44};
static int i2c_sda_gpio[6] = {35, 37, 39, 41, 43, 45};

struct mwv207d_i2c {
	struct i2c_adapter adapter;
	struct mwv207d_device *mdev;
	struct i2c_algo_bit_data bit;
	struct mutex *mutex;
	u32 scl_gpio;
	u32 sda_gpio;
	int i2c_chan;
};

static inline void mwv207d_i2c_set_dir(struct mwv207d_i2c *i2c, u32 reg, bool is_input)
{

	mb();

	mdev_modify(i2c->mdev, reg, 1 << 16, is_input << 16);

	mb();
}

static void mwv207d_i2c_gpio_reset(struct mwv207d_i2c *i2c, int i2c_chan)
{
	struct mwv207d_device *mdev = i2c->mdev;

	switch (i2c_chan) {
	case MWV207D_HDMI0_I2C_CHAN:
		mdev_modify(mdev, 0x2B0928, 0x3 << 24, 0);
		mdev_modify(mdev, 0x2B0928, 0x3 << 28, 0);
		break;
	case MWV207D_HDMI1_I2C_CHAN:
		mdev_modify(mdev, 0x2B092C, 0x3 << 0, 0);
		mdev_modify(mdev, 0x2B092C, 0x3 << 4, 0);
		break;
	case MWV207D_HDMI2_I2C_CHAN:
		mdev_modify(mdev, 0x2B092C, 0x3 << 8, 0);
		mdev_modify(mdev, 0x2B092C, 0x3 << 12, 0);
		break;
	case MWV207D_HDMI3_I2C_CHAN:
		mdev_modify(mdev, 0x2B092C, 0x3 << 16, 0);
		mdev_modify(mdev, 0x2B092C, 0x3 << 20, 0);
		break;
	case MWV207D_VGA_I2C_CHAN:
		mdev_modify(mdev, 0x2B092C, 0x3 << 24, 0);
		mdev_modify(mdev, 0x2B092C, 0x3 << 28, 0);
		break;
	case MWV207D_DVO_I2C_CHAN:
		mdev_modify(mdev, 0x2B0930, 0x3 << 0, 0);
		mdev_modify(mdev, 0x2B0930, 0x3 << 4, 0);
		break;
	default:
		break;
	}
}

static int mwv207d_i2c_pre_xfer(struct i2c_adapter *i2c_adap)
{
	struct mwv207d_i2c *i2c = i2c_get_adapdata(i2c_adap);
	struct mwv207d_device *mdev = i2c->mdev;

	mutex_lock(i2c->mutex);
	mwv207d_i2c_gpio_reset(i2c, i2c->i2c_chan);

	mdev_modify(mdev, i2c->scl_gpio, 0x1 << 8, 0);
	mdev_modify(mdev, i2c->sda_gpio, 0x1 << 8, 0);

	mwv207d_i2c_set_dir(i2c, i2c->scl_gpio, 1);
	mwv207d_i2c_set_dir(i2c, i2c->sda_gpio, 1);

	return 0;
}

static void mwv207d_i2c_post_xfer(struct i2c_adapter *i2c_adap)
{
	struct mwv207d_i2c *i2c = i2c_get_adapdata(i2c_adap);

	mutex_unlock(i2c->mutex);
}

static int mwv207d_i2c_get_clock(void *i2c_priv)
{
	struct mwv207d_i2c *i2c = (struct mwv207d_i2c *)i2c_priv;
	struct mwv207d_device *mdev = i2c->mdev;

	mwv207d_i2c_set_dir(i2c, i2c->scl_gpio, 1);

	return mdev_read(mdev, i2c->scl_gpio) & 0x1;
}

static int mwv207d_i2c_get_data(void *i2c_priv)
{
	struct mwv207d_i2c *i2c = (struct mwv207d_i2c *)i2c_priv;
	struct mwv207d_device *mdev = i2c->mdev;

	mwv207d_i2c_set_dir(i2c, i2c->sda_gpio, 1);

	return mdev_read(mdev, i2c->sda_gpio) & 0x1;
}

static void mwv207d_i2c_set_clock(void *i2c_priv, int clock)
{
	struct mwv207d_i2c *i2c = (struct mwv207d_i2c *)i2c_priv;
	struct mwv207d_device *mdev = i2c->mdev;

	mwv207d_i2c_set_dir(i2c, i2c->scl_gpio, 0);
	mdev_modify(mdev, i2c->scl_gpio, 1 << 8, clock ? 1 << 8 : 0);
}

static void mwv207d_i2c_set_data(void *i2c_priv, int data)
{
	struct mwv207d_i2c *i2c = (struct mwv207d_i2c *)i2c_priv;

	if (data)
		mwv207d_i2c_set_dir(i2c, i2c->sda_gpio, 1);
	else
		mwv207d_i2c_set_dir(i2c, i2c->sda_gpio, 0);
}

void mwv207d_i2c_destroy(struct i2c_adapter *adapter)
{
	i2c_del_adapter(adapter);
}

struct i2c_adapter *mwv207d_i2c_create(struct mwv207d_device *mdev, int i2c_chan)
{
	struct mwv207d_i2c *i2c;
	int ret;

	if (i2c_chan < 0 || i2c_chan >= MWV207D_I2C_CHAN_COUNT)
		return NULL;

	i2c = devm_kzalloc(mdev->dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return NULL;

	i2c->i2c_chan = i2c_chan;
	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.class = I2C_CLASS_DDC;
	i2c->adapter.dev.parent = mdev->dev;
	i2c_set_adapdata(&i2c->adapter, i2c);
	i2c->mutex = &mdev->gpio_lock;
	i2c->mdev = mdev;
	i2c->scl_gpio = 0x268000 + 4 * i2c_scl_gpio[i2c_chan];
	i2c->sda_gpio = 0x268000 + 4 * i2c_sda_gpio[i2c_chan];

	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
			"MWV207D_I2C_%d", i2c_chan);
	i2c->adapter.algo_data = &i2c->bit;
	i2c->bit.pre_xfer = mwv207d_i2c_pre_xfer;
	i2c->bit.post_xfer = mwv207d_i2c_post_xfer;
	i2c->bit.setsda = mwv207d_i2c_set_data;
	i2c->bit.setscl = mwv207d_i2c_set_clock;
	i2c->bit.getsda = mwv207d_i2c_get_data;
	i2c->bit.getscl = mwv207d_i2c_get_clock;
	i2c->bit.udelay = 10;
	i2c->bit.timeout = usecs_to_jiffies(2200);
	i2c->bit.data = i2c;

	ret = i2c_bit_add_bus(&i2c->adapter);
	if (ret)
		return NULL;

	return &i2c->adapter;
}

bool mwv207d_i2c_probe(struct i2c_adapter *i2c_bus)
{
	u8 out = 0x0;
	u8 buf[8];
	int ret;
	struct i2c_msg msgs[] = {
		{
		 .addr = DDC_ADDR,
		 .flags = 0,
		 .len = 1,
		 .buf = &out,
		  },
		{
		 .addr = DDC_ADDR,
		 .flags = I2C_M_RD,
		 .len = 8,
		 .buf = buf,
		}
	};

	if (!i2c_bus)
		return false;

	ret = i2c_transfer(i2c_bus, msgs, 2);
	if (ret != 2) {

		return false;
	}

	if (drm_edid_header_is_valid(buf) < 6) {
		return false;
	}
	return true;
}
