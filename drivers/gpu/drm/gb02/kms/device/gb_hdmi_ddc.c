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
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_fb_helper.h>
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "gb_dev_res.h"
#include "gb_kms.h"
#include "gbdc_drv.h"
#include "reg_ops.h"
#include "gb_ip.h"
#include "gb_hdmi_ddc.h"
#include "gbdc_device.h"

#define GB02MAC880     0x37
#define GB02MAC881    0x30

void GB02FUNC563(struct GB02STR155 *gb_dev, u32 val, int offset)
{
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;
	void __iomem *hdmi_base = ip_config->hdmi_config_base;

	GB02FUNC733(hdmi_base, GB02MAC1089(offset), val);
}

uint32_t GB02FUNC564(struct GB02STR155 *gb_dev, int offset)
{
	uint32_t val = 0;
	struct GB02STR252 *ip_config = &gb_dev->pcie_info.ip_config;
	void __iomem *hdmi_base = ip_config->hdmi_config_base;

	val = GB02FUNC730(hdmi_base, GB02MAC1089(offset));
	return val;
}

int GB02FUNC565(struct gbdc_connector *gbdc_conn,
			    unsigned char *buf, unsigned int length)
{
	struct GB02STR134 *i2c = gbdc_conn->ddc_bus;
	struct drm_connector *connector = &gbdc_conn->base;
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev = GB02FUNC85(dev->dev_private);
	uint32_t stat = 0, i = 0;

	if (!i2c->is_regaddr) {
		dev_dbg(dev->dev, "set read register address to 0\n");
		i2c->slave_reg = 0x00;
		i2c->is_regaddr = true;
	}

	while (length--) {
		GB02FUNC563(gb_dev, i2c->slave_reg++, GB02MAC991);
		if (i2c->is_segment)
			GB02FUNC563(gb_dev, HDMI_I2CM_OPERATION_READ_EXT,
				    GB02MAC995);
		else
			GB02FUNC563(gb_dev, HDMI_I2CM_OPERATION_READ,
				    GB02MAC995);

		udelay(1000);
		for (i = 0; i < 100; i++) {
			stat = GB02FUNC564(gb_dev, GB02MAC929);
			i2c->stat = stat;

			if (i2c->stat & HDMI_IH_I2CM_STAT0_DONE) {
				GB02FUNC563(gb_dev, GB02MAC929,
				    i2c->stat);
				break;
			}
			/* Check for error condition on the bus */
			if (i2c->stat & HDMI_IH_I2CM_STAT0_ERROR)
				return -EIO;

			udelay(100);
		}
		if (i >= 100)
			return -EIO;

		*buf++ = GB02FUNC564(gb_dev, GB02MAC994);
	}
	i2c->is_segment = false;

	return 0;
}

int GB02FUNC572(struct gbdc_connector *gbdc_conn,
			     unsigned char *buf, unsigned int length)
{
	struct GB02STR134 *i2c = gbdc_conn->ddc_bus;
	struct drm_connector *connector = &gbdc_conn->base;
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev = GB02FUNC85(dev->dev_private);
	uint32_t stat = 0;

	if (!i2c->is_regaddr) {
		/* Use the first write byte as register address */
		i2c->slave_reg = buf[0];
		length--;
		buf++;
		i2c->is_regaddr = true;
	}

	while (length--) {
		GB02FUNC563(gb_dev, *buf++, GB02MAC993);
		GB02FUNC563(gb_dev, i2c->slave_reg++, GB02MAC991);
		GB02FUNC563(gb_dev, HDMI_I2CM_OPERATION_WRITE,
			GB02MAC995);

		udelay(500);
		stat = GB02FUNC564(gb_dev, GB02MAC929);
		i2c->stat = stat;
		/* Check for error condition on the bus */
		if (i2c->stat & HDMI_IH_I2CM_STAT0_ERROR)
			return -EIO;
	}

	return 0;
}

int GB02FUNC574(struct i2c_adapter *adap,
			    struct i2c_msg *msgs, int num)
{
	struct gbdc_connector *gbdc_conn = i2c_get_adapdata(adap);
	struct drm_connector *connector = &gbdc_conn->base;
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev = GB02FUNC85(dev->dev_private);
	struct GB02STR134 *i2c = gbdc_conn->ddc_bus;
	u8 addr = msgs[0].addr;
	int i = 0, ret = 0;

	if (addr == GB02MAC880)
		/*
		 * The internal I2C controller does not support the multi-byte
		 * read and write operations needed for DDC/CI.
		 * TOFIX: Blacklist the DDC/CI address until we filter out
		 * unsupported I2C operations.
		 */
		return -EOPNOTSUPP;

	dev_dbg(dev->dev, "xfer: num: %d, addr: %#x\n", num, addr);

	for (i = 0; i < num; i++) {
		if (msgs[i].len == 0) {
			dev_dbg(dev->dev,
				"unsupported transfer %d/%d, no data\n",
				i + 1, num);
			return -EOPNOTSUPP;
		}
	}

	mutex_lock(&i2c->lock);

	/* Unmute DONE and ERROR interrupts */
	GB02FUNC563(gb_dev, 0x00, GB02MAC941);

	/* Set slave device address taken from the first I2C message */
	GB02FUNC563(gb_dev, addr, GB02MAC989);

	/* Set slave device register address on transfer */
	i2c->is_regaddr = false;

	/* Set segment pointer for I2C extended read mode operation */
	i2c->is_segment = false;

	for (i = 0; i < num; i++) {
		dev_dbg(dev->dev, "xfer: num: %d/%d, len: %d, flags: %#x\n",
			i + 1, num, msgs[i].len, msgs[i].flags);
		if (msgs[i].addr == GB02MAC881 && msgs[i].len == 1) {
			i2c->is_segment = true;
			GB02FUNC563(gb_dev, GB02MAC881,
				GB02MAC1003);
			GB02FUNC563(gb_dev, *msgs[i].buf, GB02MAC1008);
		} else {
			if (msgs[i].flags & I2C_M_RD)
				ret = GB02FUNC565(gbdc_conn, msgs[i].buf,
						       msgs[i].len);
			else
				ret = GB02FUNC572(gbdc_conn, msgs[i].buf,
							msgs[i].len);
		}
		if (ret < 0) {
			dev_err(dev->dev, "gb hdmi ddc access failed(%d)\n",
						ret);
			break;
		}
	}

	if (!ret)
		ret = num;

	/* Mute DONE and ERROR interrupts */
	GB02FUNC563(gb_dev, HDMI_IH_I2CM_STAT0_ERROR | HDMI_IH_I2CM_STAT0_DONE,
		GB02MAC941);

	mutex_unlock(&i2c->lock);

	return ret;
}

void GB02FUNC581(struct GB02STR155 *gb_dev)
{
	/* Software reset */
	GB02FUNC563(gb_dev, 0x00, GB02MAC1005);

	/* Set Standard Mode speed (determined to be 100KHz on iMX6) */
	GB02FUNC563(gb_dev, 0x00, GB02MAC1001);

	/* Set done, not acknowledged and arbitration interrupt polarities */
	GB02FUNC563(gb_dev, HDMI_I2CM_INT_DONE_POL, GB02MAC996);
	GB02FUNC563(gb_dev, HDMI_I2CM_CTLINT_NAC_POL | HDMI_I2CM_CTLINT_ARB_POL,
		GB02MAC998);

	/* Clear DONE and ERROR interrupts */
	GB02FUNC563(gb_dev, HDMI_IH_I2CM_STAT0_ERROR | HDMI_IH_I2CM_STAT0_DONE,
		GB02MAC929);

	/* Mute DONE and ERROR interrupts */
	GB02FUNC563(gb_dev, HDMI_IH_I2CM_STAT0_ERROR | HDMI_IH_I2CM_STAT0_DONE,
		GB02MAC941);
}

void GB02FUNC583(struct GB02STR155 *gb_dev)
{
	GB02FUNC563(gb_dev, HDMI_PHY_I2CM_INT_ADDR_DONE_POL,
		GB02MAC971);

	GB02FUNC563(gb_dev, HDMI_PHY_I2CM_CTLINT_ADDR_NAC_POL |
		HDMI_PHY_I2CM_CTLINT_ADDR_ARBITRATION_POL,
		GB02MAC972);
}
