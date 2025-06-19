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
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/io.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_gem.h>
#include <drm/drm_fb_helper.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "gbdc_drv.h"
#include "device/gb_hdmi_ddc.h"
#include "gbdc_hdmi_ddc.h"
#include "device/gbdc_device.h"
#include "device/gb_ip.h"

#define GB02MAC881    0x30
#define GB02MAC1941    DDC_ADDR

static u32 GB02FUNC1234(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm gb_hdmi_algorithm = {
	.master_xfer	= GB02FUNC574,
	.functionality	= GB02FUNC1234,
};

static struct i2c_adapter *GB02FUNC1236(struct gbdc_connector *gbdc_conn)
{
	struct i2c_adapter *adap;
	struct GB02STR134 *i2c;
	int ret = 0;
	struct device *dev = gbdc_conn->base.dev->dev;

	i2c = devm_kzalloc(dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return ERR_PTR(-ENOMEM);

	mutex_init(&i2c->lock);

	adap = &i2c->adapter;
	adap->class = I2C_CLASS_DDC;
	adap->owner = THIS_MODULE;
	adap->dev.parent = dev;
	adap->algo = &gb_hdmi_algorithm;
	strlcpy(adap->name, "Genbu HDMI", sizeof(adap->name));
	i2c_set_adapdata(adap, gbdc_conn);

	ret = i2c_add_adapter(adap);
	if (ret) {
		dev_warn(dev, "cannot add %s I2C adapter\n", adap->name);
		devm_kfree(dev, i2c);
		return ERR_PTR(ret);
	}

	gbdc_conn->ddc_bus = i2c;

	dev_info(dev, "registered %s I2C bus driver\n", adap->name);

	return adap;
}

int GB02FUNC1242(struct gbdc_connector *gbdc_conn)
{
	struct drm_connector *connector = &gbdc_conn->base;
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev =
			GB02FUNC85(dev->dev_private);

	gbdc_conn->ddc = GB02FUNC1236(gbdc_conn);
	if (IS_ERR(gbdc_conn->ddc)) {
		gbdc_conn->ddc = NULL;
		dev_err(dev->dev, "HDMI DDC I2C init failed.\n");
		return -1;
	}

	GB02FUNC583(gb_dev);

	if (gbdc_conn->ddc_bus)
		GB02FUNC581(gb_dev);

	dev_info(dev->dev, "HDMI DDC I2C init OK.\n");
	return 0;
}
EXPORT_SYMBOL_GPL(GB02FUNC1242);

int GB02FUNC1246(void *data, u8 *buf, unsigned int block,
						size_t len)
{
	struct drm_connector *connector = data;
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev =
			 (struct GB02STR155 *)connector->dev->dev_private;
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
	struct i2c_adapter *adapter = &(gbdc_conn->ddc_bus->adapter);
	unsigned char start = block * EDID_LENGTH;
	unsigned char segment = block >> 1;
	unsigned char xfers = segment ? 3 : 2;
	int ret, retries = 5;

	ret = GB02FUNC692(ip_config->hdmi_config_base);
	if (ret < 0)
		dev_dbg(dev->dev, "LT86102 preparing for EDID failed(%d).\n",
				 ret);

	/* 2nd : get EDID */
	/*
	 * The core I2C driver will automatically retry the transfer if the
	 * adapter reports EAGAIN. However, we find that bit-banging transfers
	 * are susceptible to errors under a heavily loaded machine and
	 * generate spurious NAKs and timeouts. Retrying the transfer
	 * of the individual block a few times seems to overcome this.
	 */
	do {
		struct i2c_msg msgs[] = {
			{
				.addr   = GB02MAC881,
				.flags  = 0,
				.len    = 1,
				.buf    = &segment,
			}, {
				.addr   = GB02MAC1941,
				.flags  = 0,
				.len    = 1,
				.buf    = &start,
			}, {
				.addr   = GB02MAC1941,
				.flags  = I2C_M_RD,
				.len    = len,
				.buf    = buf,
			}
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(adapter, &msgs[3 - xfers], xfers);
		if (ret == -ENXIO) {
			DRM_DEBUG_KMS("drm: skipping non-existent adapter %s\n",
						adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	/* 3rd : deselect DDC channel of LT86102's TX downstream */

	return ret == xfers ? 0 : -1;
}
EXPORT_SYMBOL_GPL(GB02FUNC1246);
