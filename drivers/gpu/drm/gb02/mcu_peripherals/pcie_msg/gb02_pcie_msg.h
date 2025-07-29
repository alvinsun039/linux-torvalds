/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PCIE_MSG_H
#define __PCIE_MSG_H
#include "gb_device.h"

#define GB02MAC1316 (1024 * 1024 - 4 * 4) 

#define GB02MAC1025 'U'
#define UPGRADE_MCU_CMD_START _IOW(GB02MAC1025, 1u, int)
#define UPGRADE_MCU_CMD_GET_PROGRESS _IOW(GB02MAC1025, 2u, int)
#define UPGRADE_MCU_CMD_SET_IMAGE_INFO _IOW(GB02MAC1025, 3u, int)

#define GB02MAC1317  0x57140001
#define GB02MAC1318   0x5714FFF0
#define GB02MAC1319   0x1001
#define GB02MAC1320      0x1002

#define GB02MAC1322 (0x1000000 + 0x40000)

struct GB02STR140 {
	struct device *dev;
	struct GB02STR39 *gbdev;
};

typedef struct
{
	uint32_t magic;
	uint32_t cmd;
	uint32_t len;
	char * data;

}pcie_msg_t;

enum pcie_msg_cmd_e
{
	PCIE_DDR_TEST,
	PCIE_DDR_READ,
	PCIE_DDR_WRITE,
	PCIE_ECC_SET,
	PCIE_ECC_GET,
};

typedef struct
{
	char * cmd_str;
	pcie_msg_t pcie_msg;

}pcie_cmd_t;

int GB02FUNC1128(struct GB02STR39 *gbdev);
void GB02FUNC1131(struct GB02STR39 *gbdev);
u64 GB02FUNC1112(void);
#endif
