// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/drivers/serial/99100.c
 *
 *  Based on drivers/serial/8250.c by Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This code is modified to support ASIX 99100 series serial devices
 */

#include <linux/version.h>

#if (defined(CONFIG_SERIAL_99xx_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ))
#define SUPPORT_SYSRQ
#endif
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/console.h>
#include <linux/sysrq.h>

#include <linux/sched.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/8250_pci.h>
#include <linux/interrupt.h>
#include <asm/byteorder.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/netlink.h>
#include <net/sock.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <crypto/sha1.h>
#include <linux/tpm.h>
#include <linux/printk.h>


#include "ax99100_spi.h"
#include "tpm.h"
#include "tcm_interface.h"

#define DRV_VERSION	"1.0.0"
#define PCI_SUBVEN_ID_AX99100_SP	0x1000
#define PCI_SUBDEV_ID_AX99100		0xa000



#define DEBUG(fmt...)


/* ================================================================ */
int spi_suspend_count;
#define NUM_DEVICE	16
static unsigned int spi_major = 241;
static unsigned int spi_min_count;

#define MAX_RESPONSE_SIZE 4096
#define HEADER_SIZE 6
unsigned char debugbuf[4096] = { 0 };
static int dev_nums;
static int ax99100_line = TCM_E_DEACTIVATED;

/* device Class */
static char *ax_devnode(const struct device *dev, umode_t *mode)
{
	return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}

struct class ax_spi_class = {
	.name		= CLASS_NAME,
	.devnode	= ax_devnode,
};
//extern struct net	init_net;
static struct sock		*nl_sk;

static int		 init_cdev;
static struct spi_99100 *axspi_device[NUM_DEVICE];

/* IOCTL*/
struct SPI_REG	*reg[NUM_DEVICE];
struct MMAP_SPI_REG	*reg_m[NUM_DEVICE];
struct SPI_DMA	*dma[NUM_DEVICE];
/* ================================================================ */

/* memmap read reg */
static _INLINE_ u32 ax99100_dread_mem_reg(int offset, int bar, int line)
{
	return readl(axspi_device[line]->membase[bar] + offset);
}

/* memmap write reg */
static _INLINE_ void ax99100_dwrite_mem_reg(int offset, int value,
		int bar, int line)
{
	writel(value, axspi_device[line]->membase[bar] + offset);
}

/* iomap read reg */
static _INLINE_ u8 ax99100_dread_io_reg(unsigned char offset, int line)
{
	return inb(axspi_device[line]->iobase0 + offset);
}

/* iomap write reg */
static _INLINE_ void ax99100_dwrite_io_reg(unsigned char offset,
		unsigned char value, int line)
{
	outb(value, axspi_device[line]->iobase0 + offset);
}

static int spi99100_hardware_func(int line, unsigned int cmd, void *arg)
{
	unsigned long	length;
	struct SPI_REG	*preg = reg[line];
	struct MMAP_SPI_REG	*preg_m = reg_m[line];
	struct SPI_DMA	*pdma = dma[line];
	struct spi_99100 *axspi = NULL;

	if (arg == NULL) {
		pr_info("%s %d arg err!\n", __func__, __LINE__);
		return -EINVAL;
	}

	axspi = axspi_device[line];

	switch (cmd) {
	case IOCTL_IO_SET_REGISTER:
	{
		preg = (struct SPI_REG *)arg;

		ax99100_dwrite_io_reg(preg->Offset, preg->Value, line);

		DEBUG("IOCTL_IO_SET_REGISTER Offset:0x%x Value:0x%x\n",
				preg->Offset, preg->Value);
		break;
	}
	case IOCTL_IO_READ_REGISTER:
	{
		DEBUG("IOCTL_IO_READ_REGISTER\n");
		preg = (struct SPI_REG *)arg;

		preg->Value = ax99100_dread_io_reg(preg->Offset, line);

		arg = preg;

		break;
	}
	case IOCTL_MEM_SET_REGISTER:
	{
		DEBUG("IOCTL_MEM_SET_REGISTER\n");
		preg_m = (struct MMAP_SPI_REG *)arg;

		ax99100_dwrite_mem_reg(preg_m->Offset, preg_m->Value,
				preg_m->Bar, line);
		break;
	}
	case IOCTL_MEM_READ_REGISTER:
	{
		DEBUG("IOCTL_MEM_READ_REGISTER\n");
		preg_m = (struct MMAP_SPI_REG *)arg;

		preg_m->Value = ax99100_dread_mem_reg(preg_m->Offset,
				preg_m->Bar, line);

		arg = preg_m;

		break;
	}
	case IOCTL_SET_TX_DMA_REG:
	{
		DEBUG("IOCTL_SET_TX_DMA_REG\n");
		length = *(unsigned long *)arg;

		ax99100_dwrite_mem_reg(REG_TDMASAR0, axspi->tx_dma_p,
				BAR1, line);
		ax99100_dwrite_mem_reg(REG_TDMASAR1, 0x0, BAR1, line);
		ax99100_dwrite_mem_reg(REG_TDMALR, length, BAR1, line);
		ax99100_dwrite_mem_reg(REG_TDMASTAR, START_DMA, BAR1, line);

		break;
	}
	case IOCTL_SET_RX_DMA_REG:
	{
		DEBUG("IOCTL_SET_RX_DMA_REG\n");
		length = *(unsigned long *)arg;

		ax99100_dwrite_mem_reg(REG_RDMASAR0, axspi->rx_dma_p,
				BAR1, line);
		ax99100_dwrite_mem_reg(REG_RDMASAR1, 0x0, BAR1, line);
		ax99100_dwrite_mem_reg(REG_RDMALR, length, BAR1, line);
		ax99100_dwrite_mem_reg(REG_RDMASTAR, START_DMA, BAR1, line);

		break;
	}
	case IOCTL_TX_DMA_WRITE:
	{
		DEBUG("IOCTL_TX_DMA_WRITE\n");
		pdma = (struct SPI_DMA *)arg;

		memcpy_toio(axspi->tx_dma_v, pdma->Buffer, pdma->Length);

		break;
	}
	case IOCTL_RX_DMA_READ:
	{
		DEBUG("IOCTL_RX_DMA_READ\n");
		pdma = (struct SPI_DMA *)arg;

		memcpy_fromio(pdma->Buffer, axspi->rx_dma_v, pdma->Length);

		arg = pdma;

		break;
	}

	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

/* Register read/write */
/* GPIO mapping - READ*/
int GetGPIOReg(int handle, uint offset, ulong *RegValue)
{
	struct MMAP_SPI_REG reg;

	reg.Offset	= offset;
	reg.Value	= 0;
	reg.Bar		= BAR5;

	if (spi99100_hardware_func(handle, IOCTL_MEM_READ_REGISTER, &reg) < 0) {
		DEBUG("IOCTL_MEM_READ_REGISTER failed(GPIO)!!!\n");
		return -1;
	}

	*RegValue	= reg.Value;
	return 0;
}

/* GPIO mapping - WRITE*/
int SetGPIOReg(int handle, uint offset, ulong RegValue)
{
	struct MMAP_SPI_REG reg;

	reg.Offset	= offset;
	reg.Value	= RegValue;
	reg.Bar		= BAR5;

	if (spi99100_hardware_func(handle, IOCTL_MEM_SET_REGISTER, &reg) < 0) {
		DEBUG("IOCTL_MEM_SET_REGISTER failed(GPIO)!!!\n");
		return -1;
	}
	return 0;
}
//=============================================================================
//spi
/* Register read/write */
/* MEM mapping - READ*/
int ReadSPIMemCfgReg(int handle, uint offset, ulong *RegValue)
{
	struct MMAP_SPI_REG reg;

	reg.Offset	= offset;
	reg.Value	= 0;
	reg.Bar		= BAR1;

	if (spi99100_hardware_func(handle, IOCTL_MEM_READ_REGISTER, &reg) < 0) {
		DEBUG("IOCTL_MEM_READ_REGISTER failed!!!\n");
		return -1;
	}

	*RegValue	= reg.Value;
	return 0;
}

/* MEM mapping - WRITE*/
int WriteSPIMemCfgReg(int handle, uint offset, ulong RegValue)
{
	struct MMAP_SPI_REG reg;

	reg.Offset	= offset;
	reg.Value	= RegValue;
	reg.Bar		= BAR1;

	if (spi99100_hardware_func(handle, IOCTL_MEM_SET_REGISTER, &reg) < 0) {
		DEBUG("IOCTL_MEM_SET_REGISTER failed!!!\n");
		return -1;
	}
	return 0;
}

/* IO mapping - READ*/
int ReadSPIIOCfgReg(int handle, uint offset, unsigned char *RegValue)
{
	struct SPI_REG reg;

	reg.Offset	= offset;
	reg.Value	= 0;

	if (spi99100_hardware_func(handle, IOCTL_IO_READ_REGISTER, &reg) < 0) {
		DEBUG("IOCTL_IO_READ_REGISTER failed!!!\n");
		return -1;
	}

	*RegValue	= reg.Value;
	return 0;
}
/* IO mapping - WRITE*/
int WriteSPIIOCfgReg(int handle, uint offset, unsigned char RegValue)
{
	struct SPI_REG reg;

	reg.Offset	= offset;
	reg.Value	= RegValue;

	if (spi99100_hardware_func(handle, IOCTL_IO_SET_REGISTER, &reg) < 0) {
		DEBUG("IOCTL_IO_SET_REGISTER failed!!!\n");
		return -1;
	}
	return 0;
}

/*open spi function*/

void ResetSPI(int handle)
{
	WriteSPIMemCfgReg(handle, REG_SWRST, 1);
}

void Delayms(unsigned int dly)
{
	mdelay(dly);
}

unsigned long SetSPIDevice(int handle, enum emSPIMODE mode, char lsb,
		char autocs, char weakup, char enspi)
{
	unsigned char value = 0x00;

	value |= (unsigned char)mode;
	if (lsb == 1)
		value |= 0x08;
	if (weakup == 1)
		value |= 0x40;
	if (enspi == 1)
		value |= 0x91;
	if (autocs == 1) {
		value |= 0x20;
		value &= ~0x01;
	}

	return WriteSPIIOCfgReg(handle, REG_SPI_CMR, value);
}

unsigned long SetSPIClock(int handle, enum emSCS source,
		unsigned long freq, char diven)
{
	unsigned char temp;
	unsigned long val = 0;

	temp = ((unsigned char)source) & 0x3;
	if (diven == 1) {
		WriteSPIIOCfgReg(handle, REG_SPI_CSS, temp);
		temp |= 0x04;
		if (source == EM_SCS_125M) {
			if (freq > 125000000)
				return 125;
			val = 125000000 / freq;
			if (val > 255)
				return 256;
		} else {
			if (source == EM_SCS_100M) {
				if (freq > 100000000)
					return 100;
				val = 100000000 / freq;
				if (val > 255)
					return 256;
			} else {
				if (source == EM_SCS_EXT) {
					if (freq > EXT_CLOCK)
						return EXT_CLOCK / 1000000;
					val = EXT_CLOCK / freq;
					if (val > 255)
						return 256;
				}
			}
		}
		WriteSPIIOCfgReg(handle, REG_SPI_BRR,
				(unsigned char)val);
	}
	WriteSPIIOCfgReg(handle, REG_SPI_CSS, temp);
	return 0;
}

int SetSPIFIFODepth(int handle, unsigned char depth)
{
	unsigned char reg = 0;

	if (depth > 8)
		depth = 8;
	depth -= 1;
	ReadSPIIOCfgReg(handle, REG_SPI_SSOL, &reg);
	reg &= ~0x70;
	reg |= (depth & 0x7) << 4;
	WriteSPIIOCfgReg(handle, REG_SPI_SSOL, reg);
	return (depth + 1);
}

void SetSPIChipSelect(int handle, enum emSPICS cs, char ede, char valid)
{
	unsigned char reg = 0;
	unsigned char temp = 0;

	ReadSPIIOCfgReg(handle, REG_SPI_SSOL, &reg);
	reg &= ~0xF;
	temp = ((unsigned char)cs) & 0x7;
	if (ede == 1) {
		reg |= 0x08;
		reg |= temp;
	} else {
		switch (temp) {
		case 0:
		default:
			reg |= 0x06;
			break;
		case 1:
			reg |= 0x05;
			break;
		case 2:
			reg |= 0x03;
			break;
		}
	}
	WriteSPIIOCfgReg(handle, REG_SPI_SSOL, reg);

	ReadSPIIOCfgReg(handle, REG_SPI_CMR, &reg);
	reg |= 1;
	if (valid == 1)
		reg &= ~1;
	WriteSPIIOCfgReg(handle, REG_SPI_CMR, reg);
}

void open_spi_func(int line)
{
	unsigned long data;

	data = 0;
	GetGPIOReg(line, REG_GPIO_DIR, &data);
	data &= ~(1<<16);
	SetGPIOReg(line, REG_GPIO_DIR, data);

	GetGPIOReg(line, REG_GPIO_DATA, &data);
	data &= ~(1<<16);
	SetGPIOReg(line, REG_GPIO_DATA, data);

	ResetSPI(line);
	Delayms(10);
	SetSPIDevice(line, EM_SPI_MODE0, 0, 0, 0, 0);
	SetSPIClock(line, EM_SCS_100M, 42000000, 1);
	SetSPIFIFODepth(line, 8);
	SetSPIDevice(line, EM_SPI_MODE0, 0, 1, 0, 1);
	SetSPIChipSelect(line, EM_SPICS0, 0, 0);
}

unsigned char GetSPIStatus(int handle, char clear)
{
	unsigned char reg = 0;

	ReadSPIIOCfgReg(handle, REG_SPI_MISR, &reg);
	if (clear)
		WriteSPIIOCfgReg(handle, REG_SPI_MISR, 3);
	return reg;
}

unsigned long ReadWriteSpiData(int handle, unsigned char *txdata,
	unsigned long dlen, unsigned char *txrxdata, unsigned long rxlen)
{
	unsigned char fifo = REG_SPI_STOF0;
	unsigned char value;
	unsigned char dump = 0;
	unsigned long i, slen, rlen, len, fifolen;

	WriteSPIIOCfgReg(handle, REG_SPI_CMR, 0xB0);
	len = dlen;
	for (i = 0, len = dlen, slen = 0; len; ) {
		fifolen = len;
		if (len >= 8)
			fifolen = 8;

		fifo = REG_SPI_STOF0;
		for (i = 0; i < fifolen; i++)
			WriteSPIIOCfgReg(handle, fifo++, txdata[slen + i]);

		value = ((fifolen - 1) & 0x7) << 4 | 0x6;
		WriteSPIIOCfgReg(handle, REG_SPI_SSOL, value);
		value = 0x0C;
		WriteSPIIOCfgReg(handle, REG_SPI_SDCR, value);
		do {
		} while (GetSPIStatus(handle, 0) == 0);
		fifo = REG_SPI_STOF0;
		for (i = 0 ; i < fifolen; i++)
			ReadSPIIOCfgReg(handle, fifo++, &txrxdata[slen + i]);

		GetSPIStatus(handle, 1);

		len -= fifolen;
		slen += fifolen;
	}
	len = 0;
	rlen = dlen;
	if (rxlen > dlen)
		len = rxlen - dlen;
	else
		rlen = rxlen;

	for (i = 0; len; ) {
		fifolen = len;
		if (len >= 8)
			fifolen = 8;

		fifo = REG_SPI_STOF0;
		for (i = 0; i < fifolen; i++)
			WriteSPIIOCfgReg(handle, fifo++, dump);

		value = ((fifolen - 1) & 0x7) << 4 | 0x6;
		WriteSPIIOCfgReg(handle, REG_SPI_SSOL, value);
		value = 0x0C;
		WriteSPIIOCfgReg(handle, REG_SPI_SDCR, value);
		do {
		} while (GetSPIStatus(handle, 0) == 0);
		fifo = REG_SPI_STOF0;
		for (i = 0; i < fifolen; i++)
			ReadSPIIOCfgReg(handle, fifo++, &txrxdata[rlen + i]);


		GetSPIStatus(handle, 1);

		len -= fifolen;
		rlen += fifolen;
	}

	return rlen;
}

unsigned long WriteTPMRegister(int line, unsigned long addr,
	unsigned long rlen, unsigned char *indata)
{
	unsigned char *value;
	unsigned char rvalue[128] = { 0 };
	struct SpiFrameHeader header;
	int i;
	char readen = 0;

	if (rlen == 0 || indata == NULL) {
		pr_err("%s arg err!\n", __func__);
		return -EINVAL;
	}
	header.body[0] = (unsigned char)((readen ? 0x80 : 0)
			| 0x40 | (rlen - 1));
	for (i = 0; i < 3; i++)
		header.body[i + 1] = (addr >> (8 * (2 - i))) & 0xff;
	value = kzalloc((rlen+4)*sizeof(unsigned char),
			GFP_KERNEL);
	if (value == NULL || value == 0) {
		pr_err("%s kmalloc fail\n", __func__);
		return -EINVAL;
	}
	for (i = 0; i < 4; i++)
		value[i] = header.body[i];
	memcpy(value + 4, indata, rlen);
	ReadWriteSpiData(line, value, rlen + 4, rvalue, 0);

	kfree(value);
	return true;
}
unsigned long ReadTPMRegister(int line, unsigned long addr,
		unsigned long rlen, unsigned char *outdata)
{
	unsigned char *value;
	unsigned char *rvalue;
	struct SpiFrameHeader header;
	unsigned long i;
	char readen = true;

	rvalue = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!rvalue || rvalue == NULL) {
		pr_info("%s kzalloc fail!\n", __func__);
		return false;
	}

	header.body[0] = (unsigned char)((readen ? 0x80 : 0)
			| 0x40 | (rlen - 1));
	for (i = 0; i < 3; i++)
		header.body[i + 1] = (addr >> (8 * (2 - i))) & 0xff;

	value = kmalloc(rlen + 4, GFP_KERNEL);
	for (i = 0; i < 4; i++)
		value[i] = header.body[i];

	for (i = 4; i < (rlen + 4); i++)
		value[i] = 0x00;

	ReadWriteSpiData(line, value, rlen + 4,
			rvalue, rlen + 4);

	memcpy(outdata, &rvalue[4], rlen);

	kfree(rvalue);
	kfree(value);
	return true;
}

static int ReadTpmSts(int line, uint32_t *status)
{
	return ReadTPMRegister(line, TPM_STS_REG, sizeof(*status),
			(unsigned char *)status);
}

int init_spi_func(int line)
{
	uint32_t did_vid, status;
	uint8_t cmd;
	uint16_t vid;

	open_spi_func(line);

	ReadTPMRegister(line, TPM_DID_VID_REG, sizeof(did_vid),
			(unsigned char *)&did_vid);
	vid = did_vid & 0xffff;
	if ((vid != 0x1b4e) && (vid != 0x1050)) {
		pr_info("unknown did_vid: %#x\n", did_vid);
		return -1;
	}

	ReadTPMRegister(line, TPM_ACCESS_REG, sizeof(cmd), &cmd);
	if ((cmd & (activeLocality & tpmRegValidSts)) ==
		(activeLocality & tpmRegValidSts)) {
		/*
		 * Locality active - maybe reset line is not connected?
		 * Release the locality and try again
		 */
		cmd = activeLocality;
		WriteTPMRegister(line, TPM_ACCESS_REG, sizeof(cmd), &cmd);
		ReadTPMRegister(line, TPM_ACCESS_REG, sizeof(cmd), &cmd);
	}
	if ((cmd & ~(tpmEstablishment | activeLocality)) != tpmRegValidSts) {
		pr_info("invalid reset status: %#x\n", cmd);
		return -1;
	}
	cmd = requestUse;
	WriteTPMRegister(line, TPM_ACCESS_REG, sizeof(cmd), &cmd);
	ReadTPMRegister(line, TPM_ACCESS_REG, sizeof(cmd), &cmd);
	if ((cmd & ~tpmEstablishment) != (tpmRegValidSts | activeLocality)) {
		pr_info("failed to claim locality, status: %#x\n", cmd);
		return -1;
	}

	ReadTpmSts(line, &status);
	if (((status >> tpmFamilyShift) & tpmFamilyMask) != tpmFamilyTPM2) {
		pr_info("unexpected TPM family value, status: %#x\n", status);
		return -1;
	}
	ReadTPMRegister(line, TPM_RID_REG, sizeof(cmd), &cmd);
	pr_info("Connected to device vid:did:rid of %4.4x:%4.4x:%2.2x\n",
			did_vid & 0xffff, did_vid >> 16, cmd);

	return 0;
}

void close_spi_func(int line)
{
	unsigned long data = 0;

	GetGPIOReg(line, REG_GPIO_DIR, &data);
	data &= ~(1<<16);
	SetGPIOReg(line, REG_GPIO_DIR, data);

	GetGPIOReg(line, REG_GPIO_DATA, &data);
	data |= (1<<16);
	SetGPIOReg(line, REG_GPIO_DATA, data);
}

/* ax99100 function */
static int WriteTpmSts(int line, uint32_t status)
{
	return WriteTPMRegister(line, TPM_STS_REG,
			sizeof(status), (unsigned char *)&status);
}
/* This is in seconds. */
#define MAX_STATUS_TIMEOUT 120
static int WaitForStatus(int line, uint32_t statusMask, uint32_t statusExpected)
{
	uint32_t status;
	ktime_t target_time;
	static u64 max_timeout;
	u64 curr_sec;

	curr_sec = ktime_to_ms(ktime_get())/1000;
	target_time = curr_sec + MAX_STATUS_TIMEOUT;
	do {
		if (curr_sec >= target_time) {
			pr_info("failed to get expected status %x\n",
					statusExpected);
			return false;
		}
		ReadTpmSts(line, &status);
	} while ((status & statusMask) != statusExpected);

	/* Calculate time spent waiting */

	curr_sec = ktime_to_ms(ktime_get())/1000;
	target_time = MAX_STATUS_TIMEOUT - target_time + curr_sec;
	if (max_timeout < (u64)target_time) {
		max_timeout = target_time;
		pr_info("\nNew max timeout: %lld s\n", max_timeout);
	}

	return true;
}

/*
 *SwapByte
 */
unsigned long SwapByte(unsigned long src, char type)
{
	unsigned long temp = 0;
	char *psrc = (char *)&src;
	char *pthis = (char *)&temp;

	if (type == 2) {
		pthis[0] = psrc[1];
		pthis[1] = psrc[0];
	}
	if (type == 4) {
		pthis[0] = psrc[3];
		pthis[1] = psrc[2];
		pthis[2] = psrc[1];
		pthis[3] = psrc[0];
	}
	return temp;
}
static uint32_t GetBurstCount(int line)
{
	uint32_t status;

	ReadTpmSts(line, &status);
	return (status >> burstCountShift) & burstCountMask;
}

/******************************************************
 *
 * File Operation
 *
 * ****************************************************/

/*   tpm file operation  */
static int ax99100_reset_gpio48(int line)
{
	int ret = -1;
	unsigned long data = 0;

	data = 0;
	ret = GetGPIOReg(line, REG_GPIO_DIR, &data);
	data &= ~(1<<10);
	ret = SetGPIOReg(line, REG_GPIO_DIR, data);

	data = 0;
	ret = GetGPIOReg(line, REG_GPIO_DATA, &data);
	data &= ~(1<<10);
	ret = SetGPIOReg(line, REG_GPIO_DATA, data);

	msleep(20);

	data = 0;
	ret = GetGPIOReg(line, REG_GPIO_DATA, &data);
	data |= (1<<10);
	ret = SetGPIOReg(line, REG_GPIO_DATA, data);

	return ret;
}

static int spi99100_hardware_open(int line)
{
	reg[line] = kmalloc(sizeof(struct SPI_REG), GFP_KERNEL);
	if (reg[line] == NULL)
		goto err;

	memset(reg[line], 0xFF, sizeof(struct SPI_REG));

	reg_m[line] = kmalloc(sizeof(struct MMAP_SPI_REG), GFP_KERNEL);
	if (reg_m[line] == NULL)
		goto err;

	memset(reg_m[line], 0xFF, sizeof(struct MMAP_SPI_REG));

	dma[line] = kmalloc(sizeof(struct SPI_DMA), GFP_KERNEL);
	if (dma[line] == NULL)
		goto err;

	memset(dma[line], 0xFF, sizeof(struct SPI_DMA));

	return 0;
err:
	if (!reg[line])
		kfree(reg[line]);
	if (!reg_m[line])
		kfree(reg_m[line]);
	if (!dma[line])
		kfree(dma[line]);
	pr_info("%s %d fail!\n", __func__, __LINE__);
	return -1;
}

static int spi99100_hardware_release(int line)
{
	kfree(reg[line]);
	kfree(reg_m[line]);
	kfree(dma[line]);

	return 0;
}

int ax99100_recv(int line, u8 *buf, size_t len)
{
	uint32_t status;
	uint32_t expected_status_bits;
	size_t handled_so_far = 0;
	uint32_t payload_size;

	ReadTPMRegister(line, TPM_DATA_FIFO_REG, 2, buf);
	ReadTPMRegister(line, TPM_DATA_FIFO_REG, 4, &buf[2]);

	handled_so_far = HEADER_SIZE;
	memcpy(&payload_size, buf + 2, sizeof(payload_size));
	payload_size = SwapByte(payload_size, 4);

	if (payload_size > MAX_RESPONSE_SIZE) {
		pr_info("%s payload_size %d > 4028,err!\n",
				__func__, payload_size);
		return 0;
	}

	payload_size = payload_size - 1;
	do {
		uint32_t transaction_size;
		uint32_t burst_count = GetBurstCount(line);

		if (burst_count > 4)
			burst_count = 4;

		transaction_size = payload_size - handled_so_far;
		if (transaction_size > burst_count)
			transaction_size = burst_count;

		if (transaction_size) {
			ReadTPMRegister(line, TPM_DATA_FIFO_REG,
					transaction_size, buf + handled_so_far);
			handled_so_far += transaction_size;
		}
	} while (handled_so_far != payload_size);

	expected_status_bits = stsValid | dataAvail;
	ReadTpmSts(line, &status);
	if ((status & expected_status_bits) != expected_status_bits) {
		pr_info("%s invalid status and invalid data ,unexpected status %#x\n",
				__func__,  status);
		return 0;
	}

	ReadTPMRegister(line, TPM_DATA_FIFO_REG, 1, buf + handled_so_far);

	/* Verify that 'data available' is not asseretd any more.*/
	ReadTpmSts(line, &status);
	if ((status & expected_status_bits) != stsValid) {
		pr_info("%s %d unexpected status %#x\n",
				__func__, __LINE__, status);
		return 0;
	}

	/* Move the TPM back to idle state. */
	WriteTpmSts(line, commandReady);

	return handled_so_far + 1;
}

int tpm_ax99100_recv(struct tpm_chip *chip, u8 *buf, size_t len)
{
	struct spi_99100 *axspi = dev_get_drvdata(&chip->dev);
	int ret = 0;

	if (axspi == NULL) {
		pr_info("%s private arg err!\n", __func__);
		return -EINVAL;
	}

	ret = ax99100_recv(axspi->dev_resource_num, buf, len);
	return ret;
}

int ax99100_send(int line, u8 *buf, size_t len)
{
	uint32_t expected_status_bits;
	size_t handled_so_far = 0;
	uint32_t payload_size;
	char message[100];
	int offset = 0;
	size_t i;

	WriteTpmSts(line, commandReady);

	expected_status_bits = commandReady;
	if (!WaitForStatus(line, expected_status_bits, expected_status_bits)) {
		pr_info("Failed processing. %s:", message);
		for (i = 0; i < len; i++) {
			if (!(i % 16))
				pr_info("\n");
			pr_info(" %2.2x", buf[i]);
		}
		pr_info("\n");
		return -1;
	}

	memcpy(&payload_size, buf + 2, sizeof(payload_size));
	payload_size = SwapByte(payload_size, 4);

	offset += snprintf(message, sizeof(message),
			"Message size %d", payload_size);
	do {
		uint32_t transaction_size;
		uint32_t burst_count = GetBurstCount(line);

		if (burst_count > 4)
			burst_count = 4;

		transaction_size = len - handled_so_far;
		if (transaction_size > burst_count)
			transaction_size = burst_count;

		if (transaction_size) {
			memset(debugbuf, 0, sizeof(debugbuf));
			WriteTPMRegister(line, TPM_DATA_FIFO_REG,
				transaction_size,
				(unsigned char *)(buf + handled_so_far));
			handled_so_far += transaction_size;
		}
	} while (handled_so_far != len);

	WriteTpmSts(line, tpmGo);

	expected_status_bits = stsValid | dataAvail;
	if (!WaitForStatus(line, expected_status_bits, expected_status_bits)) {
		size_t i;

		pr_info("Failed processing. %s:", message);
		for (i = 0; i < len; i++) {
			if (!(i % 16))
				pr_info("\n");
			pr_info(" %2.2x", buf[i]);
		}
		pr_info("\n");
		return -1;
	}

	return 0;
}

int tpm_ax99100_send(struct tpm_chip *chip, u8 *buf, size_t len)
{
	struct spi_99100 *axspi = dev_get_drvdata(&chip->dev);
	int ret = 0;

	if (axspi == NULL) {
		pr_info("%s private arg err!\n", __func__);
		return -EINVAL;
	}

	ret = ax99100_send(axspi->dev_resource_num, buf, len);

	return ret;
}

void tpm_ax99100_cancel(struct tpm_chip *chip)
{
	struct spi_99100 *axspi = dev_get_drvdata(&chip->dev);

	if (axspi == NULL) {
		pr_info("%s private arg err!\n", __func__);
		return;
	}

}
u8 tpm_ax99100_status(struct tpm_chip *chip)
{
	uint32_t status = 0;
	struct spi_99100 *axspi = dev_get_drvdata(&chip->dev);

	if (axspi == NULL) {
		pr_info("%s private arg err!\n", __func__);
		return -EINVAL;
	}

	return (u8)status;
}
static struct tpm_class_ops tpm_ax99100 = {
	.recv = tpm_ax99100_recv,
	.send = tpm_ax99100_send,
	.cancel = tpm_ax99100_cancel,
	.status = tpm_ax99100_status,
};


/********************* tcm test *************************/
u32 Unpack32(u8 *src)
{
	return(((u32)src[0]) << 24
		| ((u32)src[1]) << 16
		| ((u32)src[2]) << 8
		| (u32)src[3]);
}

int Tddli_TransmitData_backup(int line, u8 pTransmitBuf[],
		u32 TransmitBufLen,
		u8 pReceiveBuf[],
		u32 *pReceiveBufLen)
{
	u32 err = 0;
	*pReceiveBufLen = 4096;

	err = ax99100_send(line, pTransmitBuf, TransmitBufLen);
	if (-1 == err) {
		pr_info("write data error,err=%d\n", err);
		return err;
	}

	err = ax99100_recv(line, pReceiveBuf, *pReceiveBufLen);
	if (err <= 0) {
		pr_info("read data error,err=%d\n", err);
		return err;
	}
	if (err != Unpack32(pReceiveBuf+2))
		pr_info("read data length error,err=%d\n", err);


	*pReceiveBufLen = err;

	if (err <= 0)
		return err;

	return Unpack32(pReceiveBuf+6);
}


int TCM_Startup(int line)
{
	u32 outBufferLen = 1024;
	u8 outBuffer[1024] = {0x00};
	int returnCode = 0;
	u8 cmd_inBufferStartup[] =  {0x00, 0xc1,
					0x00, 0x00, 0x00, 0x0c,
					0x00, 0x00, 0x80, 0x99,
					0x00, 0x01};

	returnCode = Tddli_TransmitData_backup(line, cmd_inBufferStartup,
			sizeof(cmd_inBufferStartup), outBuffer, &outBufferLen);
	if (outBufferLen != 10)
		return outBufferLen;
	if (!(Unpack32(outBuffer + 6) == 0) &&
			!(Unpack32(outBuffer + 6) == 0x26)) {
		return  Unpack32(outBuffer + 6);
	}
	return returnCode;
}

int TCM_SelfTest(int line)
{
	u32 outBufferLen = 1024;
	u8 outBuffer[1024] = {0x00};
	int returnCode = 0;

	u8 cmd_inBufferSelfTest[] = {0x00, 0xc1, 0x00, 0x00, 0x00,
		0x0a, 0x00, 0x00, 0x80, 0x50};
	u8 cmd_inBufferGetTestResult[] = {0x00, 0xc1, 0x00, 0x00, 0x00,
		0x0a, 0x00, 0x00, 0x80, 0x54};

	returnCode = Tddli_TransmitData_backup(line,
			cmd_inBufferSelfTest, sizeof(cmd_inBufferSelfTest),
			outBuffer, &outBufferLen);
	if (returnCode == 0) {
		if (outBufferLen != 10) {
			pr_info("Receive Data Length(%d)Error\n", outBufferLen);
			return outBufferLen;
		}

	} else {
		pr_info("Error Code:%d\n", Unpack32(outBuffer + 6));
	}

	returnCode = Tddli_TransmitData_backup(line,
			cmd_inBufferGetTestResult,
			sizeof(cmd_inBufferGetTestResult),
			outBuffer, &outBufferLen);
	if (returnCode == 0) {
		pr_info("TCM_GetTestResult:Success\n");
	} else {
		pr_info("Error Code:%d\n", Unpack32(outBuffer + 6));
		pr_info("TCM_GetTestResult:Failed\n");
		return returnCode;
	}

	return returnCode;

}


/***tpm function ***/
u32 TPM2_Startup(int line)
{
	u32	outLen = 32;
	u8	outBuf[1024];

	u32	ErrorCode = 0;

	u8	inBuf[] =  {0x80, 0x01,
						0x00, 0x00, 0x00, 0x0c,
						0x00, 0x00, 0x01, 0x44,
						0x00, 0x00};

	ErrorCode = Tddli_TransmitData_backup(line, inBuf,
		sizeof(inBuf), outBuf, &outLen);

	if (ErrorCode == 0) {
		ErrorCode = Unpack32(outBuf+6);
		if (0 == ErrorCode || 0x0100 == ErrorCode)
			return 0;
		return ErrorCode;
	} else {
		return ErrorCode;
	}
	return ErrorCode;
}

u32 TPM2_Shutdown(int line)
{
	u32	outLen = 32;
	u8	outBuf[1024];

	u32	ErrorCode = 0;

	u8	inBuf[] =  {0x80, 0x01,
						0x00, 0x00, 0x00, 0x0c,
						0x00, 0x00, 0x01, 0x45,
						0x00, 0x00};

	ErrorCode = Tddli_TransmitData_backup(line, inBuf,
		sizeof(inBuf), outBuf, &outLen);

	if (ErrorCode == 0) {
		ErrorCode = Unpack32(outBuf+6);
		if (ErrorCode == 0 || ErrorCode == 0x0100)
			return 0;
		return ErrorCode;
	} else {
		return ErrorCode;
	}
	return ErrorCode;
}


static char is_tcm_module(int line)
{
	int ret = 0;

	ret = TCM_Startup(line);
	if (0 == ret || 0x26 == ret) {
		pr_info("TCM_Startup Success.\n");
		ret = 1;
	} else {
		ret = TPM2_Startup(line);
		if (0 == ret  || 0x0100 == ret) {
			pr_info("TPM_Startup Success.\n");
			ret = 0;
		} else {
			pr_info("not tpm moudle and not tcm module\n");
			ret = -1;
		}
	}

	return ret;
}

/*********************** spi operation ********************************/

static void get_random(struct tpm_chip *chip)
{
	int ret = 0;
	char hash[32];

	ret = tpm_get_random(chip, hash, 32);
	if (ret != 32) {
		pr_info("tpm_get_random fail,ret is %d!\n", ret);
		return;
	}
	pr_info("tpm_get_random OK!\n");
}


static void tcm_getrandom(int line)
{
	u8 ranData[32];
	int ret = -1;

	ret = tcm_get_random(line, 32, ranData);
	if (ret != 32) {
		pr_info("tcm_get_random fail,ret is %d!\n", ret);
		return;
	}
	pr_info("tcm_get_random OK!\n");

}

static void tcm_hash(int line)
{
	uint8_t in[128] = {0};
	uint8_t out[128] = {0};
	int out_len = 0;
	int ret = -1;

	strscpy(in, "hello world\n", 12);

	ret = tcm_hash_sm3(line, in,
			strlen(in), out, &out_len);
	if (ret != 0 || out_len != 32) {
		pr_info("%s fail!out_len:%d\n", __func__, out_len);
		return;
	}
}

static void tcm_pcr(int line)
{
	int ret = -1;
	int i;
	uint8_t pcrvalue[32] = {0};
	uint8_t pcrsetvalue[32] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

	ret = tcm_pcr_read(line, 16, pcrvalue);
	if (ret != 0)
		pr_info("tcm_pcr_read fail\n");
	for (i = 0; i < 32; i++)
		pr_info("LDQ_DEBUG__PCR[%d]  = %2.2X\r\n", i, pcrvalue[i]);

	ret = tcm_pcr_extend(line, 16, pcrsetvalue);
	pr_info("tcm_pcr_extend ret=%d\r\n", ret);
	if (ret != 0)
		pr_info("tcm_pcr_extend fail\n");
	for (i = 0; i < 32; i++)
		pr_info("PCRout[%d]  = %2.2X\r\n", i, pcrsetvalue[i]);
}

static long tcm99100_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	int line = 0;
	struct spi_99100 *axspi = NULL;
	struct	tpm_chip *chip = NULL;

	axspi = filp->private_data;
	if (IS_ERR_OR_NULL(axspi)) {
		pr_err("%s struct spi_99100 arg get fail!\n", __func__);
		return -EINVAL;
	}

	line = axspi->dev_resource_num;
	chip = axspi->tpm_chip;
	if (IS_ERR_OR_NULL(chip)) {
		pr_err("%s struct tpm_chip arg get fail!\n", __func__);
		return -EINVAL;
	}

	if (axspi->dev_type == 1) {
		tcm_getrandom(axspi->dev_resource_num);
		tcm_hash(axspi->dev_resource_num);
		tcm_pcr(axspi->dev_resource_num);
	} else {
		get_random(chip);
	}

	return 0;
}

static int tcm99100_open(struct inode *inop, struct file *filp)
{
	int line = 0;
	struct cdev *cdev = NULL;
	struct spi_99100 *axspi = NULL;

	cdev = inop->i_cdev;
	if (IS_ERR_OR_NULL(cdev)) {
		pr_err("%s cdev arg get fail!\n", __func__);
		return -EINVAL;
	}

	axspi = container_of(cdev, struct spi_99100, spi);
	if (IS_ERR_OR_NULL(axspi)) {
		pr_err("%s struct spi_99100 arg get fail!\n", __func__);
		return -EINVAL;
	}

	line = axspi->dev_resource_num;
	if (line < 0 || line >= NUM_DEVICE) {
		pr_err("%s dev_resource_num arg get fail!\n", __func__);
		return -EINVAL;
	}

	filp->private_data = axspi;

	pr_info("%s ax99100 tcm%d OK!\n", __func__, MINOR(inop->i_cdev->dev));
	return 0;
}

ssize_t tcm99100_read(struct file *file, char __user *user_buf,
		size_t size, loff_t *off)
{
	int line = 0;
	struct spi_99100 *axspi = NULL;
	u8 *data = NULL;
	size_t data_len = 0;
	int ret = 0;

	axspi = file->private_data;
	if (IS_ERR_OR_NULL(axspi)) {
		pr_err("%s struct spi_99100 arg get fail!\n", __func__);
		return -EINVAL;
	}

	line = axspi->dev_resource_num;
	if (line < 0 || line >= NUM_DEVICE) {
		pr_err("%s dev_resource_num arg get fail!\n", __func__);
		return -EINVAL;
	}

	data = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!data || data == NULL) {
		pr_info("%s kzalloc fail!\n", __func__);
		return -ENOMEM;
	}

	ret = ax99100_recv(line, data, size);
	data_len = ret > PAGE_SIZE ? PAGE_SIZE:ret;

	if (copy_to_user((void __user *)user_buf, data, data_len)) {
		pr_info("%s copy_to_user fail!\n", __func__);
		kfree(data);
		return -EFAULT;
	}

	kfree(data);
	return data_len;
}

ssize_t tcm99100_write(struct file *file, const char __user *user_buf,
		size_t size, loff_t *off)
{
	int line = 0;
	struct spi_99100 *axspi = NULL;
	u8 *data = NULL;
	size_t data_len = 0;
	int ret = 0;

	axspi = file->private_data;
	if (IS_ERR_OR_NULL(axspi)) {
		pr_err("%s struct spi_99100 arg get fail!\n", __func__);
		return -EINVAL;
	}

	line = axspi->dev_resource_num;
	if (line < 0 || line >= NUM_DEVICE) {
		pr_err("%s dev_resource_num arg get fail!\n", __func__);
		return -EINVAL;
	}

	data = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!data || data == NULL) {
		pr_info("%s kzalloc fail!\n", __func__);
		return -ENOMEM;
	}

	data_len = size > PAGE_SIZE ? PAGE_SIZE:size;
	if (copy_from_user(data, (void __user *)user_buf, data_len)) {
		pr_info("%s copy_from_user fail!\n", __func__);
		kfree(data);
		return -EFAULT;
	}

	ret = ax99100_send(line, data, data_len);
	kfree(data);

	return ret;
}

static int tcm99100_release(struct inode *inop, struct file *filp)
{
	int line = 0;
	struct spi_99100 *axspi = NULL;

	axspi = filp->private_data;
	if (IS_ERR_OR_NULL(axspi)) {
		pr_err("%s struct spi_99100 arg get fail!\n", __func__);
		return -EINVAL;
	}

	line = axspi->dev_resource_num;
	if (line < 0 || line >= NUM_DEVICE) {
		pr_err("%s dev_resource_num arg get fail!\n", __func__);
		return -EINVAL;
	}

	pr_info("%s ax99100 tcm%d OK!\n", __func__, MINOR(inop->i_cdev->dev));

	return 0;
}

int ax99100_tcm_pcr_read(int line, UINT32 PcrIndex, BYTE *PcrValue)
{
	return tcm_pcr_read(line, PcrIndex, PcrValue);
}
EXPORT_SYMBOL(ax99100_tcm_pcr_read);

int ax99100_tcm_pcr_extend(int line, UINT32 PcrIndex, BYTE *PcrValue)
{
	return tcm_pcr_extend(line, PcrIndex, PcrValue);
}
EXPORT_SYMBOL(ax99100_tcm_pcr_extend);

int ax99100_tcm_pcr_reset(int line, UINT32 PcrIndex)
{
	return tcm_pcr_reset(line, PcrIndex);
}
EXPORT_SYMBOL(ax99100_tcm_pcr_reset);

int ax99100_tcm_default_chip(void)
{
	return ax99100_line;
}
EXPORT_SYMBOL(ax99100_tcm_default_chip);


static const struct file_operations bridge_fops = {
	.owner		=	THIS_MODULE,
	.unlocked_ioctl	=	tcm99100_ioctl,
	.open		=	tcm99100_open,
	.write      =   tcm99100_write,
	.read       =   tcm99100_read,
	.release	=	tcm99100_release,
};

/********************************************************************
 *
 * NETLINK
 *
 ********************************************************************/
void netlink_get(struct sk_buff *__skb)
{
	struct nlmsghdr *nlh = NULL;
	char str[100];
	int line;

	if (__skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(__skb);
		memcpy(str, NLMSG_DATA(nlh), sizeof(str));
		pr_info("%s: received netlink message payload:%s\n",
			__func__, (char *)NLMSG_DATA(nlh));
		line = str[0] - '0';
		axspi_device[line]->tool_pid = nlh->nlmsg_pid;
	}
}

void netlink_sendmsg(struct spi_99100 *axspi)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	int		ret;
	int		pid = axspi->tool_pid;

	char msg[30] = "Interrupt Complete!";

	if (!nl_sk)
		return;

	skb = nlmsg_new(MAX_PAYLOAD_SIZE, GFP_KERNEL);

	if (!skb)
		pr_err("nlmsg_new error");

	nlh = nlmsg_put(skb, 0, 0, 0, MAX_PAYLOAD_SIZE, 0);

	memcpy(NLMSG_DATA(nlh), msg, sizeof(msg));

	ret = netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
	if (ret < 0)
		pr_info("Netlink sends failed.\n");

}

struct netlink_kernel_cfg netlink_kerncfg = {
		.input = netlink_get,
};
/********************************************************************
 *
 * PCIE FUNCTION
 *
 ********************************************************************/

static void spi99100_remove_one(struct pci_dev *dev)
{
	dev_t device;
	dev_t device_tmp;
	struct spi_99100 *axspi = NULL;
	struct  tpm_chip *chip = NULL;

	DEBUG("In %s ---------------------------------------START\n", __func__);

	axspi = (struct spi_99100 *)pci_get_drvdata(dev);
	chip = axspi->tpm_chip;

	device = MKDEV(axspi->dev_major, axspi->dev_minor);

	/* close spi function */
	close_spi_func(axspi->dev_resource_num);

	/* unregister tpm_chip */
	if (chip != NULL)
		tpm_chip_unregister(chip);

	/* DMA free */
	dma_free_coherent(&dev->dev, DMA_BUFFER_SZ,
			axspi->tx_dma_v, axspi->tx_dma_p);
	dma_free_coherent(&dev->dev, DMA_BUFFER_SZ,
			axspi->rx_dma_v, axspi->rx_dma_p);

	if (dev->subsystem_device != PCI_SUBVEN_ID_AX99100_SPI) {
		dev_err(&dev->dev, "Not AX99100 SPI device when remove!\n");
		return;
	}

	/* Remove Char Device & Class */
	device_destroy(&ax_spi_class, device);
	cdev_del(&axspi->spi);
	if (init_cdev == 1) {
		device_tmp = MKDEV(axspi->dev_major, 0);
		unregister_chrdev_region(device_tmp, NUM_DEVICE);
		init_cdev = 0;
	}

	/* Remove netlink setting */
	if (nl_sk != NULL) {
		sock_release(nl_sk->sk_socket);
		nl_sk = NULL;
	}

	free_irq(axspi->irq, axspi);

	pci_disable_device(dev);

	spi99100_hardware_release(axspi->dev_resource_num);
	dev_nums--;

	kfree(axspi);

	DEBUG("In %s---------------------------------------END\n", __func__);
}

void init_local_data(struct pci_dev *dev)
{
	struct spi_99100 *axspi = NULL;
	unsigned long base, len;

	DEBUG("In %s---------------------------------------START\n", __func__);

	axspi = (struct spi_99100 *)pci_get_drvdata(dev);

	/* memory map  */
	/* bar1 */
	len =  pci_resource_len(dev, FL_BASE1);
	base = pci_resource_start(dev, FL_BASE1);
	axspi->mapbase[0] = base;
	axspi->membase[0] = ioremap(base, len);
	/* bar5 */
	len =  pci_resource_len(dev, FL_BASE5);
	base = pci_resource_start(dev, FL_BASE5);
	axspi->mapbase[1] = base;
	axspi->membase[1] = ioremap(base, len);


	DEBUG("bar1 membase=0x%x mapbase=0x%x\n",
		(unsigned int)axspi->membase[0],
		(unsigned int)axspi->mapbase[0]);
	DEBUG("bar5 membase=0x%x mapbase=0x%x\n",
		(unsigned int)axspi->membase[1],
		(unsigned int)axspi->mapbase[1]);

	/* io map */
	base = pci_resource_start(dev, FL_BASE0);
	axspi->iobase0 = base;

	DEBUG("bar0 iobase=0x%x\n", (unsigned int)axspi->iobase0);


	/* DMA for TX */
	axspi->tx_dma_v =
		dma_alloc_coherent(&dev->dev,
				DMA_BUFFER_SZ, &axspi->tx_dma_p, GFP_ATOMIC);
	memset(axspi->tx_dma_v, 0, DMA_BUFFER_SZ);

	DEBUG("tx_dma_v=0x%x tx_dma_p=0x%x\n", (unsigned int)axspi->tx_dma_v,
		(unsigned int)axspi->tx_dma_p);

	/* DMA for RX */
	axspi->rx_dma_v =
		dma_alloc_coherent(&dev->dev, DMA_BUFFER_SZ,
				&axspi->rx_dma_p, GFP_ATOMIC);
	memset(axspi->rx_dma_v, 0, DMA_BUFFER_SZ);

	DEBUG("rx_dma_v=0x%x rx_dma_p=0x%x\n", (unsigned int)axspi->rx_dma_v,
		(unsigned int)axspi->rx_dma_p);

	DEBUG("In %s---------------------------------------END\n", __func__);
}

static irqreturn_t spi99100_interrupt(int irq, void *dev_id)
{
	int line = 0;
	int handled = 0;
	unsigned long isr_status = 0;
	unsigned long sdcr = 0;
	struct spi_99100 *axspi = (struct spi_99100 *)dev_id;

	DEBUG("In %s---------------------------------------START\n", __func__);

	line = axspi->dev_resource_num;

	DEBUG("In %s---------line: %d\n", __func__, line);


	/* Read SDCR */
	sdcr = ax99100_dread_io_reg(REG_SDCR, line);
	if (!(sdcr & INTERRUPT_ENABLE_MASK))
		return IRQ_RETVAL(0);

	/* Read ISR */
	isr_status = ax99100_dread_io_reg(REG_SPIMISR, line);
	if (!(isr_status & INTERRUPT_MASK))
		return IRQ_RETVAL(0);

	/* Clear ISR */
	ax99100_dwrite_io_reg(REG_SPIMISR, isr_status, line);

	DEBUG("In %s---------ISR: 0x%x\n", __func__,  (int)isr_status);

	if (isr_status & SPIMISR_STC) {
		DEBUG("SPI Transceiver Complete\n");
		netlink_sendmsg(axspi_device[line]);
		handled = 1;
	}
	if (isr_status & SPIMISR_STERR) {
		DEBUG("SPI Transceiver Error Indication\n");
		handled = 1;
	}

	DEBUG("In %s---------handled: %d\n", __func__,  handled);

	DEBUG("In %s--------------------------------------END\n", __func__);

	return IRQ_RETVAL(handled);
}

/* helper function to reset device connect to spi */
void spi_reset(int line)
{
	ax99100_dwrite_mem_reg(REG_SWRST, SW_RESET, BAR1, line);
}

static void axspi_line_name(int index, char *p)
{
	sprintf(p, "%s%d", NODE_NAME, (index & 0xFF));
}

static int register_char_device(struct spi_99100 *spi_device)
{
	dev_t dev = MKDEV(spi_major, spi_min_count);
	struct cdev *spi = &spi_device->spi;
	int alloc_ret = 0, cdev_ret = 0;
	struct device *device = NULL;

	memset(spi, 0, sizeof(struct cdev));

	if (init_cdev == 0) {
		alloc_ret = alloc_chrdev_region(&dev, 0, NUM_DEVICE, DEV_NAME);
		if (alloc_ret) {
			DEBUG("alloc_chrdev_region Failed.\n");
			goto disable;
		}
		spi_major = MAJOR(dev);
		init_cdev++;
	}

	spi_device->dev_major = MAJOR(dev);
	spi_device->dev_minor = MINOR(dev);
	DEBUG("maj: %d,min: %d\n", spi_device->dev_major,
			spi_device->dev_minor);

	axspi_line_name(spi_device->dev_minor, spi_device->dev_name);

	DEBUG("device name: %s\n", spi_device->dev_name);

	device = device_create(&ax_spi_class, NULL, dev,
			NULL, spi_device->dev_name);

	if (IS_ERR(device)) {
		DEBUG("device_create Failed %ld.\n", PTR_ERR(device));
		goto disable;
	}

	cdev_init(spi, &bridge_fops);
	spi->owner = THIS_MODULE;
	spi->ops = &bridge_fops;
	cdev_ret = cdev_add(spi, dev, 1);
	if (cdev_ret) {
		DEBUG("cdev_add Failed.\n");
		goto disable;
	}

	spi_min_count++;
	return 1;

disable:
	if (cdev_ret != 0)
		cdev_del(spi);
	if (device != NULL)
		device_destroy(&ax_spi_class, dev);
	if (alloc_ret != 0)
		unregister_chrdev_region(dev, 1);
	return -1;
};

static int spi99100_probe(struct pci_dev *dev, const struct pci_device_id *ent)
{
	struct spi_99100 *axspi = NULL;
	int retval, ret;
	struct tpm_chip *chip = NULL;


	DEBUG("In %s---------------------------------------START\n", __func__);

	axspi = kmalloc(sizeof(struct spi_99100), GFP_KERNEL);
	if (axspi == NULL || axspi == 0) {
		dev_err(&dev->dev, "Allocate AX99100 spi device FAILED\n");
		return -1;
	}

	pci_set_drvdata(dev, axspi);

	memset(axspi, 0, sizeof(struct spi_99100));

	axspi->dev_resource_num = dev_nums;

	retval = pci_enable_device(dev);

	if (retval) {
		dev_err(&dev->dev, "Device enable FAILED\n");
		return retval;
	}

	/* To verify whether it is a local bus communication hardware */
	if ((dev->class >> 16) != PCI_CLASS_OTHERS) {
		DEBUG("Not a spi communication hardware\n");
		retval = -ENODEV;
		goto disable;
	}

	/* Initial Netlink sock */
	if (nl_sk == NULL)
		nl_sk = netlink_kernel_create(&init_net,
				NETLINK_TEST, &netlink_kerncfg);

	DEBUG("In %s nl_sk: 0x%x\n", __func__, nl_sk);

	axspi_device[axspi->dev_resource_num] = axspi;

	pci_set_master(dev);

	init_local_data(dev);

	spi_reset(axspi->dev_resource_num);

	retval = request_irq(dev->irq, spi99100_interrupt,
				IRQF_SHARED, "ax99100_spi", axspi);
	if (retval)
		goto disable;

	axspi->irq = dev->irq;

	pr_info("%s at I/O 0x%x (irq = %d)is a AX99100 SPI\n",
			axspi->dev_name,
			(unsigned int)axspi->iobase0,
			axspi->irq);

	//init ax99100_spi function
	spi99100_hardware_open(axspi->dev_resource_num);
	init_spi_func(axspi->dev_resource_num);

	//check tcm module
	ret = is_tcm_module(dev_nums);
	dev_nums++;
	if (ret == 1) {
		axspi->dev_type = 1;
		ret = register_char_device(axspi);
		ax99100_line = ret;
		if (ret < 0) {
			DEBUG("In %s char_device_register FAILED\n", __func__);
			goto relesae_spi;
		}
		ax99100_line = 0;
	} else if (ret == 0) {
		axspi->dev_type = 0;
		chip = tpm_chip_alloc(&dev->dev, &tpm_ax99100);
		if (IS_ERR(chip)) {
			retval = PTR_ERR(chip);
			goto relesae_spi;
		}

		chip->flags = TPM_CHIP_FLAG_TPM2;
		axspi->tpm_chip = chip;
		dev_set_drvdata(&chip->dev, axspi);
		tpm_chip_register(chip);
	} else {
		goto disable;
	}

	DEBUG("In %s---0-----------------------------------END\n", __func__);
	return 0;
relesae_spi:
	spi99100_hardware_release(axspi->dev_resource_num);
disable:
	dev_nums--;
	free_irq(axspi->irq, axspi);
	pci_disable_device(dev);
	DEBUG("In %s---1-----------------------------------END\n", __func__);
	return retval;
}


static int spi99100_suspend(struct pci_dev *dev, pm_message_t state)
{
	u16 data;

	spi_suspend_count++;

	/* Enable PME and D3 */
	if (dev->pm_cap) {
		pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &data);
		pci_write_config_word(dev, dev->pm_cap + PCI_PM_CTRL, data |
				PCI_PM_CTRL_PME_ENABLE | PCI_D3hot);
		pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &data);
	}

	pci_disable_device(dev);
	pci_save_state(dev);
	pci_enable_wake(dev, PCI_D3hot, 1);
	pci_set_power_state(dev, PCI_D3hot);

	return 0;
};

static int spi99100_resume(struct pci_dev *dev)
{
	u16 data;

	pci_set_power_state(dev, PCI_D0);
	pci_restore_state(dev);
	pci_enable_wake(dev, PCI_D0, 0);

	if (pci_enable_device(dev) < 0) {
		pr_err("pci_enable_device failed disabling device\n");
		return -EIO;
	}
	pci_set_master(dev);

	spi_suspend_count--;

	/* Disable PME */
	if (dev->pm_cap) {
		pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &data);
		pci_write_config_word(dev, dev->pm_cap + PCI_PM_CTRL,
				data & (~PCI_PM_CTRL_PME_ENABLE));
		pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &data);
	}

	return 0;
};

void ax99100_shutdown(struct pci_dev *dev)
{
	int ret = -1;
	struct spi_99100 *axspi = pci_get_drvdata(dev);

	if (ax99100_reset_gpio48(axspi->dev_resource_num) != 0)
		pr_err("ax99100 reset gpio48 fail!\n");

	mdelay(1000);
	init_spi_func(axspi->dev_resource_num);
	ret = TPM2_Startup(axspi->dev_resource_num);
	if (0 != ret  && 0x0100 != ret)
		pr_info("%s TPM_Startup fail.\n", __func__);

	pr_info("%s ok!\n", __func__);
}

static struct pci_device_id spi99100_pci_tbl[] = {
	{0x125B, 0x9100, PCI_SUBDEV_ID_AX99100,
		PCI_SUBVEN_ID_AX99100_SPI, 0, 0, 0},

	{0, },
};
static struct pci_driver starex_spi_driver = {
	.name = "AX99100_SPI",
	.probe = spi99100_probe,
	.remove = spi99100_remove_one,
	.id_table = spi99100_pci_tbl,
	.suspend = spi99100_suspend,
	.resume = spi99100_resume,
	.shutdown = ax99100_shutdown,
};
/* Drivers entry function. register with the pci core */
static int __init spi99100_init(void)
{
	int ret;



	DEBUG("In %s---------------------------------------START\n", __func__);

	ret = class_register(&ax_spi_class);
	if (ret) {
		DEBUG("unable to register ax spi class\n");
		return ret;
	}

	ret = pci_register_driver(&starex_spi_driver);
	if (ret < 0) {
		DEBUG("In %s pci_register_driver FAILED\n", __func__);
		goto err;
	}


	DEBUG("In %s ---------------------------------------END\n", __func__);

	return ret;
err:
	class_unregister(&ax_spi_class);
	return ret;
}

/* Drivers exit function. Unregister with the PCI core as well as serial core */
static void __exit spi99100_exit(void)
{
	DEBUG("In %s ---------------------------------------START\n", __func__);

	pci_unregister_driver(&starex_spi_driver);
	class_unregister(&ax_spi_class);

	DEBUG("In %s ---------------------------------------END\n", __func__);
}

module_init(spi99100_init);
module_exit(spi99100_exit);

MODULE_DEVICE_TABLE(pci, spi99100_pci_tbl);
MODULE_DESCRIPTION("ASIX AX99100 Serial Driver Module");
MODULE_LICENSE("GPL");
