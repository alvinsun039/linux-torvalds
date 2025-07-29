// SPDX-License-Identifier: GPL-2.0
#include "gpu/gb_device.h"
#include "gb-peripherals-common.h"
#include "mcu_peripherals/i2c/tmp102.h"
#include "mcu_peripherals/i2c/ina2xx.h"
#include "mcu_peripherals/i2c/ads1015.h"
#include "mcu_peripherals/i2c/ps8625.h"
#include "mcu_peripherals/uart/gb02_usart.h"
#include "mcu_peripherals/gpio/pinctrl-gb02-mcu.h"
#include "mcu_peripherals/spi/spi-gb02-mcu.h"
#include "mcu_peripherals/i2c/i2c-gb02-mcu.h"
#include "mcu_peripherals/pwm/gb_pwm.h"
#include "mcu_peripherals/pvt/gb02-pvt.h"
#include "mcu_peripherals/gpu_freq/gpu_freq.h"
#include "mcu_peripherals/fan/gb02_fan.h"
#include "mcu_peripherals/sn_info/gb_sn_info.h"
#include "mcu_peripherals/pcie_msg/gb02_pcie_msg.h"

#define GB02MAC210 (sizeof(mcu_periph_info) / sizeof(struct GB02STR72))
#define GB02MAC212 (sizeof(mcu_i2c_device) / sizeof(struct GB02STR73))
#define GB02MAC213 (sizeof(mcu_pwm_device) / sizeof(struct GB02STR74))

static struct GB02STR72 mcu_periph_info[] = {
	/*UART*/
	{
		.mcu_peripherals_device_name = PLATFORM_UART_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.peri_offset_address = GB02MAC795,
		.gb02_peri_ip_drv_init = GB02FUNC1841,
		.gb02_peri_ip_drv_exit = GB02FUNC1842,
	},
	{
		.mcu_peripherals_device_name = PLATFORM_UART_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC780,
		.peri_offset_address = GB02MAC799,
		.gb02_peri_ip_drv_init = GB02FUNC1841,
		.gb02_peri_ip_drv_exit = GB02FUNC1842,
	},
	/*SPI*/
	{
		.mcu_peripherals_device_name = PLATFORM_SPI_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.peri_offset_address = GB02MAC822,
		.gb02_peri_ip_drv_init = GB02FUNC1723,
		.gb02_peri_ip_drv_exit = GB02FUNC1725,
	},
	/*I2C*/
	{
		.mcu_peripherals_device_name = PLATFORM_I2C_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.peri_offset_address = GB02MAC806,
		.gb02_peri_ip_drv_init = GB02FUNC910,
		.gb02_peri_ip_drv_exit = GB02FUNC913,
	},
	{
		.mcu_peripherals_device_name = PLATFORM_I2C_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC780,
		.peri_offset_address = GB02MAC810,
		.gb02_peri_ip_drv_init = GB02FUNC910,
		.gb02_peri_ip_drv_exit = GB02FUNC913,
	},
	{
		.mcu_peripherals_device_name = PLATFORM_I2C_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC783,
		.peri_offset_address = GB02MAC814,
		.gb02_peri_ip_drv_init = GB02FUNC910,
		.gb02_peri_ip_drv_exit = GB02FUNC913,
	},
	{
		.mcu_peripherals_device_name = PLATFORM_I2C_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC786,
		.peri_offset_address = GB02MAC817,
		.gb02_peri_ip_drv_init = GB02FUNC910,
		.gb02_peri_ip_drv_exit = GB02FUNC913,
	},
	{
		.mcu_peripherals_device_name = PLATFORM_LGPIO_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.peri_offset_address = GB02MAC834,
		.gb02_peri_ip_drv_init = GB02FUNC366,
		.gb02_peri_ip_drv_exit = GB02FUNC371,
	},
	/*GPIO_PWM*/
	{
		.mcu_peripherals_device_name = PLATFORM_GPIO_PWM_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.gpio_num = 40,
		.gb02_peri_ip_drv_init = GB02FUNC1412,
		.gb02_peri_ip_drv_exit = GB02FUNC1414,
		.gb02_peri_ip_drv_resume = GB02FUNC1415,
	},
	/*PWM*/
	{
		.mcu_peripherals_device_name = PLATFORM_PWM_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC780,
		.peri_offset_address = GB02MAC829,
		.gpio_num = -1,
		.gb02_peri_ip_drv_init = GB02FUNC1375,
		.gb02_peri_ip_drv_exit = GB02FUNC1378,
	},
	{
		.mcu_peripherals_device_name = PLATFORM_PWM_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.peri_offset_address = GB02MAC825,
		.gpio_num = -1,
		.gb02_peri_ip_drv_init = GB02FUNC1375,
		.gb02_peri_ip_drv_exit = GB02FUNC1378,
	},
	/*PVT*/
	{
		.mcu_peripherals_device_name = PLATFORM_PVT_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.gb02_peri_ip_drv_init = GB02FUNC1181,
		.gb02_peri_ip_drv_exit = GB02FUNC1182,
	},
	/*FREQ*/
	{
		.mcu_peripherals_device_name = PLATFORM_FREQ_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.gb02_peri_ip_drv_init = GB02FUNC754,
		.gb02_peri_ip_drv_exit = GB02FUNC755,
	},
	/*FAN*/
	{
		.mcu_peripherals_device_name = PLATFORM_FAN_DEVICE_NAME,
		.mcu_peripherals_device_id = GB02MAC777,
		.gb02_peri_ip_drv_init = GB02FUNC69,
		.gb02_peri_ip_drv_exit = GB02FUNC79,
		.gb02_peri_ip_drv_suspend = GB02FUNC82,
		.gb02_peri_ip_drv_resume = GB02FUNC86,
	},
};

static struct GB02STR73 mcu_i2c_device[] = {
	{
		.board_info = {I2C_BOARD_INFO(INA219_DEVICE_NAME, GB02MAC1252),},
		.bus_id = GB02MAC1250,
		.i2c_driver_init = GB02FUNC975,
		.i2c_driver_exit = GB02FUNC981,
		.enable = false,
	},
	{
		.board_info = {I2C_BOARD_INFO(ADS1015_DEVICE0_NAME, GB02MAC1119),},
		.bus_id = GB02MAC1120,
		.i2c_driver_init = GB02FUNC779,
		.i2c_driver_exit = GB02FUNC775,
		.enable = false,
	},
	{
		.board_info = {I2C_BOARD_INFO(ADS1015_DEVICE1_NAME, GB02MAC1121),},
		.bus_id = GB02MAC1122,
		.i2c_driver_init = GB02FUNC779,
		.i2c_driver_exit = GB02FUNC775,
		.enable = false,
	},
	{
		.board_info = {I2C_BOARD_INFO(ADS1015_DEVICE2_NAME, GB02MAC1123),},
		.bus_id = GB02MAC1124,
		.i2c_driver_init = GB02FUNC779,
		.i2c_driver_exit = GB02FUNC775,
		.enable = false,
	},
};

static struct GB02STR74 mcu_pwm_device[] = {
	{
		.device_id = GB02MAC2034,
		.name = GB02_PWM_FAN_DEV_NAME,
		.gpio_pwm = false, //for gpio pwm
		.pwm_driver_init = GB02FUNC1253,
		.pwm_driver_exit = GB02FUNC1262,
		.pwm_driver_resume = GB02FUNC1268,
		.pwm_driver_suspend = GB02FUNC1264,
		.enable = false,
	},
	{
		.device_id = GB02MAC777,
		.name = GB02_PWM_BL_DEV_NAME,
		.pwm_driver_init = GB02FUNC1215,
		.pwm_driver_exit = GB02FUNC1219,
		.num_connector = 0,
		.enable = true,
	},
};

static void GB02FUNC139(struct GB02STR39 *gbdev, int device_num)
{
	int ret;
	struct GB02STR72 *peri_info;

	peri_info = &gbdev->gb_peri[device_num];
	if (!peri_info || !peri_info->gb02_peri_ip_drv_init)
		return;

	ret = peri_info->gb02_peri_ip_drv_init(gbdev, peri_info);
	if (ret)
		gb_printf(KERN_ERR, "%s%d driver init failed\n",
			peri_info->mcu_peripherals_device_name, peri_info->mcu_peripherals_device_id);
}

static void GB02FUNC141(struct GB02STR39 *gbdev, int device_num)
{
	struct GB02STR72 *peri_info;

	peri_info = &gbdev->gb_peri[device_num];
	if (!peri_info || !peri_info->gb02_peri_ip_drv_exit)
		return;

	peri_info->gb02_peri_ip_drv_exit(peri_info);
}

static void GB02FUNC143(struct GB02STR39 *gbdev, int device_num)
{
	struct GB02STR72 *peri_info;

	peri_info = &gbdev->gb_peri[device_num];
	if (!peri_info || !peri_info->gb02_peri_ip_drv_suspend)
		return;

	peri_info->gb02_peri_ip_drv_suspend(peri_info);
}

static void GB02FUNC145(struct GB02STR39 *gbdev, int device_num)
{
	struct GB02STR72 *peri_info;

	peri_info = &gbdev->gb_peri[device_num];
	if (!peri_info || !peri_info->gb02_peri_ip_drv_resume)
		return;

	peri_info->gb02_peri_ip_drv_resume(peri_info);
}

static struct GB02STR72 *GB02FUNC149(struct GB02STR39 *gbdev, int device_id, char *name)
{
	int i, ret;

	for (i = 0; i < GB02MAC210; i++) {
		ret = strcmp(gbdev->gb_peri[i].mcu_peripherals_device_name, name);
		if (!ret) {
			if (gbdev->gb_peri[i].mcu_peripherals_device_id == device_id) {
				if (!gbdev->gb_peri[i].priv)
					return NULL;
				else
					return &gbdev->gb_peri[i];
			}
		}
	}

	return NULL;
}

static void GB02FUNC153(struct GB02STR39 *gbdev, struct GB02STR73 *i2c_device)
{
	struct i2c_adapter *adapter;
	struct GB02STR133 *mcu_i2c;
	struct GB02STR72 *peri_info;
	int ret;

	if (!i2c_device->enable)
		return;
	peri_info = GB02FUNC149(gbdev, i2c_device->bus_id, PLATFORM_I2C_DEVICE_NAME);
	if (!peri_info)
		return;

	mcu_i2c = (struct GB02STR133 *)peri_info->priv;
	if (!mcu_i2c)
		return;
	adapter = &mcu_i2c->adapter.adap;
	if (!adapter)
		return;

	i2c_device->client = gb02_i2c_new_device(adapter, &i2c_device->board_info);
	if (!i2c_device->i2c_driver_init || !i2c_device->client)
		return;
	ret = i2c_device->i2c_driver_init(i2c_device->client);
	if (ret)
		gb_printf(KERN_ERR, "i2c_device:%s init failed\n", i2c_device->board_info.type);
}

static void GB02FUNC160(struct GB02STR73 *i2c_device)
{
	if (!i2c_device->enable)
		return;

	if (!i2c_device->i2c_driver_exit || !i2c_device->client)
		return;

	i2c_device->i2c_driver_exit(i2c_device->client);
}

static void GB02FUNC163(struct GB02STR39 *gbdev, struct GB02STR74 *pwm_dev)
{
	int ret;
	struct GB02STR72 *peri_info;

	if (!pwm_dev->enable)
		return;

	if (pwm_dev->gpio_pwm)
		peri_info = GB02FUNC149(gbdev, pwm_dev->device_id, PLATFORM_GPIO_PWM_DEVICE_NAME);
	else
		peri_info = GB02FUNC149(gbdev, pwm_dev->device_id, PLATFORM_PWM_DEVICE_NAME);
	if (!peri_info || !pwm_dev->pwm_driver_init)
		return;

	ret = pwm_dev->pwm_driver_init(gbdev, peri_info, pwm_dev);
	if (ret)
		gb_printf(KERN_ERR, "pwm_dev:%s init failed\n", pwm_dev->name);
}

static void GB02FUNC166(struct GB02STR39 *gbdev, struct GB02STR74 *pwm_dev)
{
	struct GB02STR72 *peri_info;

	if (!pwm_dev->enable)
		return;

	if (!pwm_dev->gpio_pwm)
		peri_info = GB02FUNC149(gbdev, pwm_dev->device_id, PLATFORM_GPIO_PWM_DEVICE_NAME);
	else
		peri_info = GB02FUNC149(gbdev, pwm_dev->device_id, PLATFORM_PWM_DEVICE_NAME);
	if (!peri_info || !pwm_dev->pwm_driver_exit)
		return;
	pwm_dev->pwm_driver_exit(peri_info);
}

static int GB02FUNC168(struct GB02STR39 *gbdev, struct GB02STR74 *pwm_dev)
{
	int ret = -1;
	struct GB02STR72 *peri_info;

	if (!pwm_dev->enable)
		return 0;

	if (!pwm_dev->gpio_pwm)
		peri_info = GB02FUNC149(gbdev, pwm_dev->device_id, PLATFORM_GPIO_PWM_DEVICE_NAME);
	else
		peri_info = GB02FUNC149(gbdev, pwm_dev->device_id, PLATFORM_PWM_DEVICE_NAME);
	if (!peri_info || !pwm_dev->pwm_driver_suspend)
		return ret;

	ret = pwm_dev->pwm_driver_suspend(peri_info);
	if (ret)
		gb_printf(KERN_ERR, "pwm_dev:%s suspend failed\n", pwm_dev->name);

	return ret;
}

static int GB02FUNC171(struct GB02STR39 *gbdev, struct GB02STR74 *pwm_dev)
{
	int ret = -1;
	struct GB02STR72 *peri_info;

	if (!pwm_dev->enable)
		return 0;

	if (!pwm_dev->gpio_pwm)
		peri_info = GB02FUNC149(gbdev, pwm_dev->device_id, PLATFORM_GPIO_PWM_DEVICE_NAME);
	else
		peri_info = GB02FUNC149(gbdev, pwm_dev->device_id, PLATFORM_PWM_DEVICE_NAME);
	if (!peri_info || !pwm_dev->pwm_driver_resume)
		return ret;

	ret = pwm_dev->pwm_driver_resume(gbdev, peri_info);
	if (ret)
		gb_printf(KERN_ERR, "pwm_dev:%s resume failed\n", pwm_dev->name);

	return ret;
}

void GB02FUNC175(struct GB02STR39 *gbdev)
{
	int i;

	memcpy(gbdev->gb_peri, mcu_periph_info, sizeof(mcu_periph_info));

	//for mcu ip driver init
	for (i = 0; i < GB02MAC210; i++)
		GB02FUNC139(gbdev, i);

	//for i2c device init
	for (i = 0; i < GB02MAC212; i++)
		GB02FUNC153(gbdev, &mcu_i2c_device[i]);

	//for pwm device init
	for (i = 0; i < GB02MAC213; i++)
		GB02FUNC163(gbdev, &mcu_pwm_device[i]);

	GB02FUNC1598(gbdev->gb_pcie->pdev);
	GB02FUNC1128(gbdev);
}

void GB02FUNC177(struct GB02STR39 *gbdev)
{
	int i;

	//for pwm device exit
	for (i = 0; i < GB02MAC213; i++)
		GB02FUNC166(gbdev, &mcu_pwm_device[i]);

	//for i2c device exit
	for (i = 0; i < GB02MAC212; i++)
		GB02FUNC160(&mcu_i2c_device[i]);

	//for mcu ip driver exit
	for (i = 0; i < GB02MAC210; i++)
		GB02FUNC141(gbdev, i);

	GB02FUNC1131(gbdev);
}

int GB02FUNC178(struct GB02STR39 *gbdev)
{
	int i;

	//for pwm device suspend
	for (i = 0; i < GB02MAC213; i++)
		GB02FUNC168(gbdev, &mcu_pwm_device[i]);
	//for mcu ip driver suspend
	for (i = 0; i < GB02MAC210; i++)
		GB02FUNC143(gbdev, i);
	return 0;
}

int GB02FUNC185(struct GB02STR39 *gbdev)
{
	int i;

	//for mcu ip driver resume
	for (i = 0; i < GB02MAC210; i++)
		GB02FUNC145(gbdev, i);
	//for pwm device resume
	for (i = 0; i < GB02MAC213; i++)
		GB02FUNC171(gbdev, &mcu_pwm_device[i]);

	return 0;
}
