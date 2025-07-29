#ifndef __GB_FIRMWARE_UPGRADE_H
#define __GB_FIRMWARE_UPGRADE_H

#define GB02MAC1068	6
//#define GB02MAC2844	GB02MAC1068
#define GB02MAC1069	 		0
#define GB02MAC1070		1
#define GB02MAC724  			2
#define GB02MAC1071		4
#define GB02MAC734		0xd0c
#define GB02MAC737	0xd10
#define GB02MAC740	0x60000000
#define GB02MAC743	0x80000000
#define GB02MAC745		0x403000

#define GB02MAC320					(0x1000) /* 4KB */

/* BAR0 register */
#define GB02MAC852	 0x14
#define GB02MAC853	BIT(4)
#define GB02MAC39     0x200
#define GB02MAC40     0x204
#define GB02MAC42     0x208
#define GB02MAC44     0x5a
#define GB02MAC46     0x20c
#define GB02MAC48  0x5a
#define GB02MAC49     0x210
#define GB02MAC50     0x55
#define GB02MAC855     0x214

/* BAR1 register */
#define GB02MAC858		0x0
#define GB02MAC859		0x4
#define GB02MAC860	0x8
#define GB02MAC861	0xc
#define GB02MAC862	0x18
#define GB02MAC863	0x1c
#define GB02MAC864	0x20
#define GB02MAC865	0x24

typedef enum {
	T_UPGRADE_MCU_RESULT_NO_UPGRADE = 0,	/* UPGRADE MCU failed */
	T_UPGRADE_MCU_RESULT_UPGRADING,			/* UPGRADE MCU is in progress */
	T_UPGRADE_MCU_RESULT_OK, 				/* UPGRADE MCU OK */
	T_UPGRADE_MCU_RESULT_MAX
} upgrade_mcu_result_e;

typedef enum {
	U_UPGRADE_TYPE_MCU_APP = 1,		/* upgrade MCU's APP image */
	U_UPGRADE_TYPE_UEFI_FW, 		/* upgrade UEFI firmware */
	U_UPGRADE_TYPE_MAX
} upgrade_mcu_sub_type_e;

struct GB02STR114 {
	uint8_t filename[0x100];	/* sub image name : upgrade_app.bin */
	uint64_t addr; /* the address of the sub image in VRAM */
	uint32_t size; /* the size of the sub image */
	uint16_t crc16;
	uint8_t checksum;
	uint8_t type; /* MCU app, UEFI or others */
};

struct GB02STR115 {
	struct device *dev;
	struct GB02STR39 *gbdev;
	struct GB02STR67 *dev_info;
	wait_queue_head_t wait;
	int upgrade_flag;
	struct proc_dir_entry *procdir;
	int upgrade_result;
};

int GB02FUNC670(struct GB02STR39 *gbdev, struct GB02STR67 *GB02STR153);
void GB02FUNC677(struct GB02STR39 *gbdev);
#endif
