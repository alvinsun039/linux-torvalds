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
#include <linux/pci.h>
#include <linux/mm.h>
#include <common/gb_common.h>

#include "gb_sn_info.h"

serial_number_info_t sn;

unsigned int str2dec(unsigned char *pstr)
{
    unsigned int sum = 0;
    int len = strlen((char *)(pstr));
    unsigned int x;
	size_t i;

	for (i = 0; i < len; i++) {
        sum *= 10;
        x = pstr[i] - '0';
        sum += x;
    }

    return sum;
}

bool GB02FUNC1568(void)
{
	return sn.snvaild;
}
int GB02FUNC1569(void)
{
	return sn.vendor;
}
int GB02FUNC1570(void)
{
	return sn.factory;
}
int GB02FUNC1571(void)
{
	return sn.series;
}
int GB02FUNC1572(void)
{
	return sn.interface;
}
int GB02FUNC1574(void)
{
	return sn.gpu_type;
}
int GB02FUNC1575(void)
{
	return sn.gpu_batch;
}
int GB02FUNC1576(void)
{
	return sn.bin_level;
}
date_info_t * gb_get_date_form_sn(void)
{
	return &sn.date_time;
}
int GB02FUNC1577(void)
{
	return sn.lot;
}
serial_number_info_t * gb_get_sn_info_t(void)
{
	return &sn;
}

char* GB02FUNC1580(serial_number_info_t *p_info)
{
	sprintf(p_info->sn_code, "%s%01d%02d%02d%02d%02d%01d%02d%02d%02d%04d",
			(p_info->vendor==1)?"XT":"UN",
					p_info->factory,
					p_info->series,
					p_info->interface,
					p_info->gpu_type,
					p_info->gpu_batch,
					p_info->bin_level,
					p_info->date_time.date.year,
					p_info->date_time.date.mon,
					p_info->date_time.date.day,
					p_info->lot );

	gb_printf(KERN_INFO, "SN:%s", p_info->sn_code);
	return p_info->sn_code;
}

void GB02FUNC1584(serial_number_info_t *p_info)
{
	gb_printf(KERN_INFO, "Vendor:%s\n", GetVendorString(p_info->vendor));
	gb_printf(KERN_INFO, "Factory:%s\n", GetFactoryString(p_info->factory));
	gb_printf(KERN_INFO, "Series:%s\n", GetSeriesString(p_info->series));
	gb_printf(KERN_INFO, "Interface:%s\n", GetInterfaceString(p_info->interface));
	gb_printf(KERN_INFO, "GPU Type:%s\n", GetGpuTypeString(p_info->gpu_type));
	gb_printf(KERN_INFO, "GPU Batch:KP5%02d\n", p_info->gpu_batch);
	gb_printf(KERN_INFO, "BIN Level:%s\n", GetBinLevelString(p_info->bin_level));
	gb_printf(KERN_INFO, "MFG Date:20%02d-%02d-%02d\n", p_info->date_time.date.year,
			p_info->date_time.date.mon,p_info->date_time.date.day);

	gb_printf(KERN_INFO, "LOT:%04d\n", p_info->lot);
}

int GB02FUNC1586(char *sn, serial_number_info_t *p_info)
{
	char* p = sn;

	uint8_t vendor[3] = {0,};
	uint8_t factory[2] = {0,};
	uint8_t series[3] = {0,};
	uint8_t interface[3] = {0,};
	uint8_t gpu_type[3] = {0,};
	uint8_t gpu_batch[3] = {0,};
	uint8_t bin_level[2] = {0,};
	uint8_t date_year[3] = {0,};
	uint8_t date_mon[3] = {0,};
	uint8_t date_day[3] = {0,};
	uint8_t lot[5] = {0,};

	printk(KERN_INFO "%s, sn is %s\n", __func__, sn);
	if(sn == NULL || p_info == NULL)
		return -EIO;

//	if(strlen(sn)!= GB02MAC2096)
//		return -1;

	memcpy(vendor, p, 2);
	vendor[2] = '\0';
	p_info->vendor = !strcmp(vendor,"XT")?VENDOR_SIETIUM:0;

	// parse factory section
	p+=2;
	memcpy(factory, p, 1);
	factory[1] = '\0';
	if(1!=CheckFactory(str2dec(factory)))
	{
		p_info->factory = FACTORY_SIETIUM_SHENZHEN;
	}
	p_info->factory = str2dec(factory);

	// parse series section
	p+=1;
	memcpy(series, p, 2);
	series[2] = '\0';
	if(1!=CheckSeries(str2dec(series)))
	{
		p_info->series = SERIES_CHENQI1;
	}
	p_info->series = str2dec(series);

	// parse interface type section
	p+=2;
	memcpy(interface, p, 2);
	interface[2] = '\0';
	if(1!=CheckInterface(str2dec(interface)))
	{
		p_info->interface = INTERFACE_PCIE_HALF_L_FULL_H;
	}
	p_info->interface = str2dec(interface);

	// parse GPU type section
	p+=2;
	memcpy(gpu_type, p, 2);
	gpu_type[2] = '\0';
	if(1!=CheckGpuType(str2dec(gpu_type)))
	{
		p_info->gpu_type = GPU_TYPE_GENBU_02;
	}
	p_info->gpu_type = str2dec(gpu_type);

	// parse GPU batch section
	p+=2;
	memcpy(gpu_batch, p, 2);
	gpu_batch[2] = '\0';
	p_info->gpu_batch = str2dec(gpu_batch);

	// parse bin level section
	p+=2;
	memcpy(bin_level, p, 1);
	bin_level[1] = '\0';
	if(1!=CheckBinLevel(str2dec(bin_level)))
	{
		p_info->bin_level = BIN_LEVEL_FULL_OK;
	}
	p_info->bin_level = str2dec(bin_level);
	// parse product date section
	p+=1;
	memcpy(date_year, p, 2);
	date_year[2] = '\0';
	p_info->date_time.date.year = str2dec(date_year);
	p+=2;
	memcpy(date_mon, p, 2);
	date_mon[2] = '\0';
	p_info->date_time.date.mon = str2dec(date_mon);
	p+=2;
	memcpy(date_day, p, 2);
	date_day[2] = '\0';
	p_info->date_time.date.day = str2dec(date_day);

	// parse lot section
	p+=2;
	memcpy(lot, p, 4);
	p_info->lot = str2dec(lot);

	return 0;
}

int GB02FUNC1593(struct pci_dev *pdev)
{
	void __iomem *rom;
	size_t size;
	char sn_buf[GB02MAC2096+2+1];

	if (!pdev->rom_attr_enabled)
		return -EINVAL;

	rom = pci_map_rom(pdev, &size);	/* size starts out as PCI window size */
	if (!rom || !size)
		return -EIO;

	gb_printf(KERN_INFO, "%s, rom size is %ld\n", __func__, size);

	memcpy_fromio(sn_buf, rom + GB02MAC2090, sizeof(sn_buf));

	sn_buf[GB02MAC2096+2] = '\0';

	if(sn_buf[0] != 0x10 || sn_buf[1] != 0x85)
	{
		printk(KERN_INFO "%s, sn title err\n", __func__);
		sn.snvaild = false;
		goto err;
	}
	else
	{
		sn.snvaild = true;
		printk(KERN_INFO "%s, sn title ok \n", __func__);
	}

	GB02FUNC1586(sn_buf+2, &sn);
	GB02FUNC1584(&sn);
//	GB02FUNC1580(&sn);

err:
	pci_unmap_rom(pdev, rom);
	return 0;
}

void GB02FUNC1598(struct pci_dev *pdev)
{
	pdev->rom_attr_enabled = 1;
	GB02FUNC1593(pdev);
}
