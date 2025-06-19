/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __I2C_GB02_MCU_H
#define __I2C_GB02_MCU_H
#include "common/gb-peripherals-common.h"

#define GB02MAC1148 0x24
#define GB02MAC1149 0x28
#define GB02MAC1150 0x2C
#define GB02MAC1151 0x30
#define GB02MAC1152 0x34
#define GB02MAC1153 0x38
#define GB02MAC1154 0x40
#define GB02MAC1155 0x48

#define GB02MAC1156 (1 << 0)
#define GB02MAC1157 (1 << 1)
#define GB02MAC1158 (1 << 2)
#define GB02MAC1159 (1 << 3)
#define GB02MAC1160 (1 << 4)
#define GB02MAC1161 (1 << 5)

#define GB02MAC1162 (1 << 0)
#define GB02MAC1163 (1 << 1)
#define GB02MAC1164 (1 << 2)
#define GB02MAC1165 (1 << 3)
#define GB02MAC1166 (1 << 4)

#define GB02MAC1167 (1 << 0)
#define GB02MAC1168 (1 << 1)
#define GB02MAC1169 (1 << 2)
#define GB02MAC1170 (1 << 3)
#define GB02MAC1171 (1 << 4)
#define GB02MAC1172 (1 << 5)
#define GB02MAC1173 (1 << 6)
#define GB02MAC1174 (1 << 7)
#define GB02MAC1175 (1 << 8)
#define GB02MAC1176 (1 << 9)
#define GB02MAC1177 (1 << 11)
#define GB02MAC1178 (1 << 12)
#define GB02MAC1179 (1 << 13)
#define GB02MAC1180 (1 << 14)
#define GB02MAC1181 (1 << 15)

#define GB02MAC1182 (0x3 << 18)

#define GB02MAC1183 4
#define GB02MAC1184 10
// #define GB02MAC757		(3200 * 1000 / 12)	/* in KHz */
// #define GB02MAC757		(5333)				/* in KHz */

#define GB02MAC1185 'I'
#define I2C_MCU_CMD_I2C0 _IOW(GB02MAC1185, 1u, int)
#define I2C_MCU_CMD_I2C1 _IOW(GB02MAC1185, 2u, int)
#define I2C_MCU_CMD_I2C2 _IOW(GB02MAC1185, 3u, int)
#define I2C_MCU_CMD_I2C3 _IOW(GB02MAC1185, 4u, int)
#define I2C_MCU_CMD_I2C0_RD _IOW(GB02MAC1185, 5u, int)
#define I2C_MCU_CMD_I2C0_WR _IOW(GB02MAC1185, 6u, int)

#define GB02MAC1186 0x50
#define GB02MAC1187 256
#define GB02MAC1188 2
#define GB02_ADS1015_DEV_NAME "ads1015"
#define GB02_INA2XXX_DEV_NAME "ina2xx"
#define GB02_PS8625_DEV_NAME "ps8625"
enum I2C_DutyTypedef {
	I2C_30_DUTY                     = 0,/*!< I2C %30 duty */
	I2C_40_DUTY                     = 1,/*!< I2C %40 duty */
	I2C_50_DUTY                     = 2,/*!< I2C %50 duty */
};

struct GB02STR131 {
	unsigned int flags;
	unsigned int slave_addr;
	unsigned long frequency; /* in KHz */
	unsigned int sda_delay;
};

struct GB02STR132 {
	wait_queue_head_t wait;
	kernel_ulong_t quirks;
	struct i2c_msg *msg;
	unsigned int msg_num;
	unsigned int msg_idx;
	unsigned int msg_ptr;
	unsigned int tx_setup;
	unsigned int irq;
	void __iomem *regs;
	struct device *dev;
	struct i2c_adapter adap;
	void *driver_data;
	u32 bus_no;
	int stopped;
	unsigned int slave_addr;
};
struct GB02STR133 {
	void __iomem *regs;
	void __iomem *sysctl_iofunc_base; // bar0
	void __iomem *sysctl_cfg_base;    // bar4
	unsigned int i2c_bus_no;
	struct device *dev;

	struct GB02STR132 adapter; // todo

	const struct GB02STR131 *pdata;
	struct i2c_client *ads1015_i2c_client;
	struct i2c_client *ina219_i2c_client;
	struct i2c_client *tmp102_i2c_client;
	struct i2c_client *ps8625_i2c_client;
};

#ifdef CONFIG_MCU_I2C
int GB02FUNC910(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC913(struct GB02STR72 *peri_info);
struct i2c_client *
gb02_i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info);
#else
static inline int GB02FUNC910(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC913(struct GB02STR72 *peri_info)
{
	return;
}
static inline struct i2c_client *
gb02_i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info)
{
	return NULL;
}
#endif

#endif /* __I2C_GB02_MCU_H */
