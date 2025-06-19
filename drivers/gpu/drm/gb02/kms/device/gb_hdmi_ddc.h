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

#ifndef __HDMI_DDC_H__
#define __HDMI_DDC_H__

/* HDMI EDDC */
#define GB02MAC929                      0x0105
#define GB02MAC931                   0x0180
#define GB02MAC933                   0x0181
#define GB02MAC935                   0x0182
#define GB02MAC937                   0x0183
#define GB02MAC939                  0x0184
#define GB02MAC941                 0x0185
#define GB02MAC943                  0x0186
#define GB02MAC945                   0x0187
#define GB02MAC947              0x0188
#define GB02MAC950            0x0189
#define GB02MAC953                            0x01FF

/* HDMI Master PHY Registers */
#define GB02MAC958                0x3020
#define GB02MAC959              0x3021
#define GB02MAC962              0x3022
#define GB02MAC965              0x3023
#define GB02MAC967              0x3024
#define GB02MAC969              0x3025
#define GB02MAC970            0x3026
#define GB02MAC971                  0x3027
#define GB02MAC972               0x3028
#define GB02MAC973                  0x3029
#define GB02MAC974             0x302a
#define GB02MAC976        0x302b
#define GB02MAC978        0x302c
#define GB02MAC980        0x302d
#define GB02MAC982        0x302e
#define GB02MAC984        0x302f
#define GB02MAC985        0x3030
#define GB02MAC986        0x3031
#define GB02MAC987        0x3032

/* I2C Master Registers (E-DDC) */
#define GB02MAC989                         0x7E00
#define GB02MAC991                       0x7E01
#define GB02MAC993                         0x7E02
#define GB02MAC994                         0x7E03
#define GB02MAC995                     0x7E04
#define GB02MAC996                           0x7E05
#define GB02MAC998                        0x7E06
#define GB02MAC1001                           0x7E07
#define GB02MAC1003                       0x7E08
#define GB02MAC1005                      0x7E09
#define GB02MAC1008                        0x7E0A
#define GB02MAC1009            0x7E0B
#define GB02MAC1011            0x7E0C
#define GB02MAC1013            0x7E0D
#define GB02MAC1014            0x7E0E
#define GB02MAC1015            0x7E0F
#define GB02MAC1016            0x7E10
#define GB02MAC1017            0x7E11
#define GB02MAC1018            0x7E12

enum {
	/* IH_I2CM_STAT0 and IH_MUTE_I2CM_STAT0 field values */
	HDMI_IH_I2CM_STAT0_DONE = 0x2,
	HDMI_IH_I2CM_STAT0_ERROR = 0x1,

	/* IH_MUTE_I2CMPHY_STAT0 field values */
	HDMI_IH_MUTE_I2CMPHY_STAT0_I2CMPHYDONE = 0x2,
	HDMI_IH_MUTE_I2CMPHY_STAT0_I2CMPHYERROR = 0x1,

	/* I2CM_OPERATION field values */
	HDMI_I2CM_OPERATION_WRITE = 0x10,
	HDMI_I2CM_OPERATION_READ_EXT = 0x2,
	HDMI_I2CM_OPERATION_READ = 0x1,

	/* I2CM_INT field values */
	HDMI_I2CM_INT_DONE_POL = 0x8,
	HDMI_I2CM_INT_DONE_MASK = 0x4,

	/* I2CM_CTLINT field values */
	HDMI_I2CM_CTLINT_NAC_POL = 0x80,
	HDMI_I2CM_CTLINT_NAC_MASK = 0x40,
	HDMI_I2CM_CTLINT_ARB_POL = 0x8,
	HDMI_I2CM_CTLINT_ARB_MASK = 0x4,

	/* PHY_I2CM_OPERATION_ADDR field values */
	HDMI_PHY_I2CM_OPERATION_ADDR_WRITE = 0x10,
	HDMI_PHY_I2CM_OPERATION_ADDR_READ = 0x1,

	/* GB02MAC971 */
	HDMI_PHY_I2CM_INT_ADDR_DONE_POL = 0x08,
	HDMI_PHY_I2CM_INT_ADDR_DONE_MASK = 0x04,

	/* GB02MAC972 */
	HDMI_PHY_I2CM_CTLINT_ADDR_NAC_POL = 0x80,
	HDMI_PHY_I2CM_CTLINT_ADDR_NAC_MASK = 0x40,
	HDMI_PHY_I2CM_CTLINT_ADDR_ARBITRATION_POL = 0x08,
	HDMI_PHY_I2CM_CTLINT_ADDR_ARBITRATION_MASK = 0x04,
};

extern int GB02FUNC574(struct i2c_adapter *adap,
			    struct i2c_msg *msgs, int num);
extern void GB02FUNC581(struct GB02STR155 *gb_dev);
extern void GB02FUNC583(struct GB02STR155 *gb_dev);

#endif
