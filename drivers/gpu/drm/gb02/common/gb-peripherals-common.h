#ifndef __GB02_PERIPHERALS_COMMON__
#define __GB02_PERIPHERALS_COMMON__
#define DEBUG
/*global*/
#include <linux/io.h>
#ifndef CONFIG_SW64
#include <asm-generic/io.h>
#endif
#include <linux/i2c.h>
#include "common/gb_common.h"
#define GB02MAC709 0x8510
#define GB02MAC712 0x0201

#define GB02MAC715 6
#define GB02MAC718 0
#define GB02MAC721 1
#define GB02MAC724 2
#define GB02MAC726 4
#define GB02MAC729 5

#define GB02MAC734 0xd0c
#define GB02MAC737 0xd10
#define GB02MAC740 0x60000000
#define GB02MAC743 0x80000000
#define GB02MAC745 0x403000
#define GB02MAC748 0x3000000
#define GB02MAC751 0xf0000000
#define GB02MAC754 0x52000
#define GB02MAC757		(3200 * 1000 / 12) /*in KHz*/
/*#define GB02MAC757 (5333)*/ /* in KHz  */

#define GB02MAC760 (2)
#define GB02MAC763 0x60000
#define GB02MAC766 0x1000

#define PLATFORM_UART_DEVICE_NAME "gb02-mcu-uart"
#define PLATFORM_SPI_DEVICE_NAME "gb02-mcu-spi"
#define PLATFORM_PWM_DEVICE_NAME "gb02-mcu-pwm"
#define PLATFORM_GPIO_PWM_DEVICE_NAME "gb02-mcu-gpio-pwm"
#define PLATFORM_I2C_DEVICE_NAME "gb02-mcu-i2c"
#define PLATFORM_LGPIO_DEVICE_NAME "gb02-mcu-gpio"
#define PLATFORM_PVT_DEVICE_NAME "gb02-pvt-thermal"
#define PLATFORM_FREQ_DEVICE_NAME "gpu_freq"
#define PLATFORM_FAN_DEVICE_NAME "gb02-fan"
#define GB02MAC777 0
#define GB02MAC780 1
#define GB02MAC783 2
#define GB02MAC786 3
#define GB02MAC788 4
#define GB02MAC791 5

/*uart resource*/
#define GB02MAC795 0x20000
#define GB02MAC796 (GB02MAC751 + GB02MAC795)
#define GB02MAC799 0x22000
#define GB02MAC801 (GB02MAC751 + GB02MAC799)
#define GB02MAC804 0x2000

/*i2c resource*/
#define GB02MAC806 0x30000
#define GB02MAC808 (GB02MAC751 + GB02MAC806)
#define GB02MAC810 0x32000
#define GB02MAC812 (GB02MAC751 + GB02MAC810)
#define GB02MAC814 0x34000
#define GB02MAC815 (GB02MAC751 + GB02MAC814)
#define GB02MAC817 0x36000
#define GB02MAC819 (GB02MAC751 + GB02MAC817)
#define GB02MAC821 0x2000

/*spi resource*/
#define GB02MAC822 0x52000
#define GB02MAC823 (GB02MAC751 + GB02MAC822)

/*pwm resource*/
#define GB02MAC825 0x60000
#define GB02MAC827 (GB02MAC751 + GB02MAC825)
#define GB02MAC829 0x61000
#define GB02MAC831 (GB02MAC751 + GB02MAC829)

/*pvt resource*/

/*lgpio resource*/
#define GB02MAC834 0x80000
#define GB02MAC835 0xf0080000
#define GB02MAC836 0xf0085fff
#define GB02MAC837 0x80000
#define GB02MAC838 (GB02MAC751 + GB02MAC837)
#define GB02MAC839 0x81000
#define GB02MAC840 (GB02MAC751 + GB02MAC839)
#define GB02MAC841 0x82000
#define GB02MAC842 (GB02MAC751 + GB02MAC841)
#define GB02MAC843 0x83000
#define GB02MAC844 (GB02MAC751 + GB02MAC843)
#define GB02MAC845 0x84000
#define GB02MAC846 (GB02MAC751 + GB02MAC845)
#define GB02MAC847 0x85000
#define GB02MAC848 (GB02MAC751 + GB02MAC847)


#define GB02MAC849 16
/* BAR0 register */
#define GB02MAC850 0x4
#define GB02MAC851 0x10
#define GB02MAC852 0x14
#define GB02MAC853 BIT(4)
#define GB02MAC854 0x18
#define GB02MAC39 0x200
#define GB02MAC40 0x204
#define GB02MAC42 0x208
#define GB02MAC44 0x5a
#define GB02MAC46 0x20c
#define GB02MAC48 0x5a
#define GB02MAC49 0x210
#define GB02MAC50 0x55
#define GB02MAC855 0x214
#define GB02MAC856 0x218
#define GB02MAC857 0x21c

/* BAR1 register */
#define GB02MAC858 0x0
#define GB02MAC859 0x4
#define GB02MAC860 0x8
#define GB02MAC861 0xc
#define GB02MAC862 0x18
#define GB02MAC863 0x1c
#define GB02MAC864 0x20
#define GB02MAC865 0x24

#define GB02MAC866 64
#define GB02MAC867 32
typedef struct GB02STR39 GB02STR39;

struct GB02STR71 {
	struct pci_dev *pdev;
};

struct GB02STR72 {
	unsigned char mcu_peripherals_device_name[GB02MAC866];
	unsigned int mcu_peripherals_device_id;
	unsigned int peri_offset_address;
	int gpio_num; //for gpio pwm
	int (*gb02_peri_ip_drv_init)(struct GB02STR39 *, struct GB02STR72 *);
	void (*gb02_peri_ip_drv_exit)(struct GB02STR72 *);
	int (*gb02_peri_ip_drv_suspend)(struct GB02STR72 *);
	int (*gb02_peri_ip_drv_resume)(struct GB02STR72 *);
	void *priv;
};

struct GB02STR73 {
	struct i2c_board_info board_info;
	u8 bus_id;
	int (*i2c_driver_init)(struct i2c_client *);
	void (*i2c_driver_exit)(struct i2c_client *);
	struct i2c_client * client;
	bool enable;
};

struct GB02STR74 {
	int device_id;
	bool gpio_pwm;
	unsigned char *name;
	bool enable;
	int num_connector;
	int (*pwm_driver_init)(struct GB02STR39 *, struct GB02STR72 *, struct GB02STR74 *pwm_dev);
	void (*pwm_driver_exit)(struct GB02STR72 *);
	int (*pwm_driver_suspend)(struct GB02STR72 *);
	int (*pwm_driver_resume)(struct GB02STR39 *gbdev, struct GB02STR72 *);
};

static inline void GB02FUNC528(u32 val, void __iomem *regbase, u32 offset)
{
	iowrite32(val, regbase + offset);
	gb_printf(KERN_DEBUG, "REG: write 0x%x value: 0x%x\n", offset, val);
}

static inline u32 GB02FUNC530(void __iomem *regbase, u32 offset)
{
	u32 val;

	val = ioread32(regbase + offset);
	gb_printf(KERN_DEBUG, "REG: read 0x%x value: 0x%x\n", offset, val);

	return val;
}
#ifdef CONFIG_MCU_PERI
void GB02FUNC175(struct GB02STR39 *gbdev);
void GB02FUNC177(struct GB02STR39 *gbdev);
int GB02FUNC178(struct GB02STR39 *gbdev);
int GB02FUNC185(struct GB02STR39 *gbdev);
#else
static inline void GB02FUNC175(struct GB02STR39 *gbdev)
{
	return;
}

static inline void GB02FUNC177(struct GB02STR39 *gbdev)
{
	return;
}

static inline int GB02FUNC178(struct GB02STR39 *gbdev)
{
	return 0;
}

static inline int GB02FUNC185(struct GB02STR39 *gbdev)
{
	return 0;
}
#endif
#endif
