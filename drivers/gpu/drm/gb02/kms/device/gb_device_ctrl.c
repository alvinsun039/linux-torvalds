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

#include "gb_device.h"
#include "gb_device_ctrl.h"
#include "mcu_peripherals/gpio/pinctrl-gb02-mcu.h"

#define GB02MAC824 25
#define GB02MAC826 25
#define GB02MAC828 27
#define GB02MAC830 24
#define GB02MAC832 26
#define GB02MAC833 39
static bool pcie_c0_200_gpio_gsv_reset_request = false;

static int GB02FUNC506(int gpio, int dir)
{
	int ret = -1;

#ifdef CONFIG_MCU_GPIO
	if ((GB02MAC824 == gpio) && pcie_c0_200_gpio_gsv_reset_request)
		return 0;

	ret = GB02FUNC295(gpio);
	if (ret) {
		gb_printf(KERN_ERR, "Failed to request  gpio %d\n", gpio);
		return ret;
	}
	ret = GB02FUNC301(gpio, dir);
	if (ret) {
		gb_printf(KERN_ERR, "Failed to set  gpio %d direction\n", gpio);
		return ret;
	}
	if (GB02MAC824 == gpio)
		pcie_c0_200_gpio_gsv_reset_request = true;
#endif

	return ret;
}

void GB02FUNC513(void *gpio_addr, u32 gpio_bit)
{
    u32 val;
    val = ioread32((void __iomem *)gpio_addr);
    val &= (~gpio_bit);
    iowrite32(val, (void __iomem *)gpio_addr);
//    msleep(200); // the time must >= 200ms
}
void GB02FUNC515(void *gpio_addr, u32 gpio_bit)
{
    u32 val;
    val = ioread32((void __iomem *)gpio_addr);
    val |= gpio_bit;
    iowrite32(val, (void __iomem *)gpio_addr);
//    msleep(200); // the time must >= 200ms
}

void HDMI_power_down(struct GB02STR70 *gpi)
{
	void __iomem *base = gpi->pci_bars[0].mmio;
	int type = GB02FUNC503(gpi);

	switch (type) {
	case PCIE_LPDDR4:
		GB02FUNC513(base + GB02MAC873, 0x3);
		msleep(200); // the time must >= 200ms
		GB02FUNC515(base + GB02MAC873, 0x1);
		GB02FUNC515(base + GB02MAC873, 0x2);
		break;
	case PCIE_FULL_LPDDR4:
		GB02FUNC513(base + GB02MAC873, 0x3);
		msleep(200); // the time must >= 200ms
		GB02FUNC515(base + GB02MAC873, 0x1);
		GB02FUNC515(base + GB02MAC873, 0x2);
		break;
	case PCIE_M6FL8G_LPDDR4:
		/*Change the GPIO mode of hdmi-1 */
		GB02FUNC515(base + 0x10, 0x40);
		msleep(200); // the time must >= 200ms
		/*Power down PDB&5v*/
		/*hdmi 1*/
		GB02FUNC513(base + GB02MAC873, 0xc0);
		/*hdmi 3*/
		GB02FUNC513(base + GB02MAC872, 0x40);
		GB02FUNC513(base + GB02MAC873, 0x8);
		/*hdmi 4*/
		GB02FUNC513(base + GB02MAC872, 0x80);
		GB02FUNC513(base + GB02MAC873, 0x1);
		/*hdmi 5*/
		GB02FUNC513(base + GB02MAC873, 0x6);
		msleep(200); // the time must >= 200ms
		/*Power on 5v firstly*/
		/*hdmi 1*/
		GB02FUNC515(base + GB02MAC873, 0x40);
		/*hdmi 3*/
		GB02FUNC515(base + GB02MAC872, 0x40);
		/*hdmi 4*/
		GB02FUNC515(base + GB02MAC872, 0x80);
		/*hdmi 5*/
		GB02FUNC515(base + GB02MAC873, 0x2);
		msleep(200); // the time must >= 200ms
		/*Power on PDB secondly*/
		/*hdmi 1*/
		GB02FUNC515(base + GB02MAC873, 0x80);
		/*hdmi 3*/
		GB02FUNC515(base + GB02MAC873, 0x8);
		/*hdmi 4*/
		GB02FUNC515(base + GB02MAC873, 0x1);
		/*hdmi 5*/
		GB02FUNC515(base + GB02MAC873, 0x4);
		gb_printf(KERN_INFO, "%s-%d: board_type = %d HDMI_power down\n",
		__func__, __LINE__, GB02FUNC503(gpi));
		break;
	case PCIE_HIE1LP4_LPDDR4:
		/*hdmi 5v ctrl & PDB PDB3 clear*/
		GB02FUNC513(base + GB02MAC873, 0xb);
		/*hdmi PDB2 PDB4 clear*/
		GB02FUNC513(base + GB02MAC872, 0xc0);
		msleep(200); // the time must >= 200ms
		/*hdmi 5v ctrl*/
		GB02FUNC515(base + GB02MAC873, 0x1);
		/*hdmi1 PDB*/
		GB02FUNC515(base + GB02MAC873, 0x2);
		/*hdmi3 PDB3*/
		GB02FUNC515(base + GB02MAC873, 0x8);
		/*hdmi2 PDB2*/
		GB02FUNC515(base + GB02MAC872, 0x40);
		/*hdmi4 PDB4*/
		GB02FUNC515(base + GB02MAC872, 0x80);
		gb_printf(KERN_INFO, "%s-%d: board_type = %d HDMI_power down\n",
		__func__, __LINE__, GB02FUNC503(gpi));
		break;
	case PCIE_C0_200:
		if (GB02FUNC506(GB02MAC824,
					GB02MAC542) == 0)
			GB02FUNC312(GB02MAC824, 0);
		break;
	case PCIE_M4HL8G_LPDDR4:
		GB02FUNC312(GB02MAC828, 0);
		GB02FUNC312(GB02MAC830, 0);
		GB02FUNC312(GB02MAC832, 0);
		GB02FUNC312(GB02MAC833, 0);
		gb_printf(KERN_INFO, "%s-%d: board_type = %d HDMI_power down\n",
		__func__, __LINE__, GB02FUNC503(gpi));
		break;
	case GB02MAC1363:
		GB02FUNC312(GB02MAC826, 0);
		break;
	default:
		break;

	}
}
void HDMI_power_reset(struct GB02STR70 *gpi, int dp_index)
{
	void __iomem *base = gpi->pci_bars[0].mmio;
	int type = GB02FUNC503(gpi);

	switch (type) {
	case PCIE_LPDDR4:
		if (dp_index == 2) {
			GB02FUNC513(base + GB02MAC873, 0x2);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC873, 0x2);
		}
		gb_printf(KERN_INFO,
		"%s-%d: board_type = %d, dp_index = %dHDMI_power finish\n",
		__func__, __LINE__, GB02FUNC503(gpi), dp_index);
		break;
	case PCIE_FULL_LPDDR4:
		if (dp_index == 5) {
			GB02FUNC513(base + GB02MAC873, 0x2);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC873, 0x2);
		}
		gb_printf(KERN_INFO,
		"%s-%d: board_type = %d, dp_index = %d HDMI_power finish\n",
		__func__, __LINE__, GB02FUNC503(gpi), dp_index);
		break;
	case PCIE_M6FL8G_LPDDR4:
		if (dp_index == 1) {
			/*hdmi 1*/
			/*Change the GPIO mode of hdmi-1 */
			GB02FUNC515(base + 0x10, 0x40);
			msleep(200); // the time must >= 200ms
			/*Power down PDB*/
			GB02FUNC513(base + GB02MAC873, 0x80);
			msleep(200); // the time must >= 200ms
			/*Power on PDB */
			GB02FUNC515(base + GB02MAC873, 0x80);
		} else if (dp_index == 3) {
			/*hdmi 3*/
			GB02FUNC513(base + GB02MAC873, 0x8);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC873, 0x8);
		} else if (dp_index == 4) {
			 /*hdmi 4*/
			GB02FUNC513(base + GB02MAC873, 0x1);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC873, 0x1);
		} else if (dp_index == 5) {
			/*hdmi 5*/
			GB02FUNC513(base + GB02MAC873, 0x4);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC873, 0x4);
		}
		gb_printf(KERN_INFO,
		"%s-%d: board_type = %d, dp_index = %d HDMI_power finish\n",
		__func__, __LINE__, GB02FUNC503(gpi), dp_index);
		break;
	case PCIE_HIE1LP4_LPDDR4:
		if (dp_index == 0) {
			/*hdmi 1*/
			GB02FUNC513(base + GB02MAC873, 0x2);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC873, 0x2);
		} else if (dp_index == 2) {
			/*hdmi 2*/
			GB02FUNC513(base + GB02MAC872, 0x40);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC872, 0x40);
		} else if (dp_index == 3) {
			/*hdmi 3*/
			GB02FUNC513(base + GB02MAC873, 0x8);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC873, 0x8);
		} else if (dp_index == 5) {
			/*hdmi 4*/
			GB02FUNC513(base + GB02MAC872, 0x80);
			msleep(200); // the time must >= 200ms
			GB02FUNC515(base + GB02MAC872, 0x80);
		}
		gb_printf(KERN_INFO,
		"%s-%d: board_type = %d, dp_index = %d HDMI_power finish\n",
		__func__, __LINE__, GB02FUNC503(gpi), dp_index);

		break;
	case PCIE_C0_200:
#if 0
		if (dp_index == 2) {
			if (GB02FUNC506(GB02MAC824,
					GB02MAC542) == 0) {
				GB02FUNC312(GB02MAC824, 0);
				msleep(100);
				GB02FUNC312(GB02MAC824, 1);
			}
		}
#endif
		break;
	default:
		break;
	}
}

void HDMI_power_reset_all(struct GB02STR70 *gpi)
{
	void __iomem *base = gpi->pci_bars[0].mmio;
	int type = GB02FUNC503(gpi);

	switch (type) {
	case PCIE_LPDDR4:
		GB02FUNC513(base + GB02MAC873, 0x3);
		msleep(200); // the time must >= 200ms
		GB02FUNC515(base + GB02MAC873, 0x1);
		GB02FUNC515(base + GB02MAC873, 0x2);
		break;
	case PCIE_FULL_LPDDR4:
		GB02FUNC513(base + GB02MAC873, 0x3);
		msleep(200); // the time must >= 200ms
		GB02FUNC515(base + GB02MAC873, 0x1);
		GB02FUNC515(base + GB02MAC873, 0x2);
		break;
	case PCIE_M6FL8G_LPDDR4:
		/*Change the GPIO mode of hdmi-1 */
		GB02FUNC515(base + 0x10, 0x40);
		msleep(200); // the time must >= 200ms
		/*Power down PDB*/
		/*hdmi 1*/
		GB02FUNC513(base + GB02MAC873, 0x80);
		/*hdmi 3*/
		GB02FUNC513(base + GB02MAC873, 0x8);
		/*hdmi 4*/
		GB02FUNC513(base + GB02MAC873, 0x1);
		/*hdmi 5*/
		GB02FUNC513(base + GB02MAC873, 0x4);
		msleep(200); // the time must >= 200ms
		/*Power on PDB */
		/*hdmi 1*/
		GB02FUNC515(base + GB02MAC873, 0x80);
		/*hdmi 3*/
		GB02FUNC515(base + GB02MAC873, 0x8);
		/*hdmi 4*/
		GB02FUNC515(base + GB02MAC873, 0x1);
		/*hdmi 5*/
		GB02FUNC515(base + GB02MAC873, 0x4);
		gb_printf(KERN_INFO, "%s-%d: board_type = %d HDMI_power down\n",
		__func__, __LINE__, GB02FUNC503(gpi));
		break;
	case PCIE_HIE1LP4_LPDDR4:
		/*hdmi PDB PDB3 clear*/
		GB02FUNC513(base + GB02MAC873, 0x2);
		GB02FUNC513(base + GB02MAC873, 0x8);
		/*hdmi PDB2 PDB4 clear*/
		GB02FUNC513(base + GB02MAC872, 0xc0);
		msleep(200); // the time must >= 200ms
		/*hdmi1 PDB*/
		GB02FUNC515(base + GB02MAC873, 0x2);
		/*hdmi3 PDB3*/
		GB02FUNC515(base + GB02MAC873, 0x8);
		/*hdmi2 PDB2*/
		GB02FUNC515(base + GB02MAC872, 0x40);
		/*hdmi4 PDB4*/
		GB02FUNC515(base + GB02MAC872, 0x80);
		gb_printf(KERN_INFO, "%s-%d: board_type = %d HDMI_power down\n",
		__func__, __LINE__, GB02FUNC503(gpi));
		break;
	case PCIE_C0_200:
		// if (GB02FUNC506(GB02MAC824,
		// 			GB02MAC542) == 0) {
		// 	GB02FUNC312(GB02MAC824, 0);
		// 	msleep(1);
		// 	GB02FUNC312(GB02MAC824, 1);
		// }
		break;
	case PCIE_M4HL8G_LPDDR4:
		/*hdmi 0*/
		GB02FUNC312(GB02MAC828, 0);
		msleep(1);
		GB02FUNC312(GB02MAC828, 1);
		/*hdmi 2*/
		GB02FUNC312(GB02MAC830, 0);
		msleep(1);
		GB02FUNC312(GB02MAC830, 1);
		/*hdmi 3*/
		GB02FUNC312(GB02MAC832, 0);
		msleep(1);
		GB02FUNC312(GB02MAC832, 1);
		/*hdmi 5*/
		GB02FUNC312(GB02MAC833, 0);
		msleep(1);
		GB02FUNC312(GB02MAC833, 1);
		break;
	case GB02MAC1363:
		/*hdmi*/
		GB02FUNC312(GB02MAC826, 0);
		msleep(1);
		GB02FUNC312(GB02MAC826, 1);
		break;
	default:
		break;
	}
}

