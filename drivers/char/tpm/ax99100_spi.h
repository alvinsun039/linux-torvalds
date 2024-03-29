/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  linux/drivers/serial/99xx.h
 *
 *  Based on drivers/serial/8250.c by Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This code is modified to support ASIX 99100 series serial devices
 */

#ifndef SPIDRIVER_H_
#define SPIDRIVER_H_

#include <linux/types.h>
#include <linux/cdev.h>


/* Definition for IOCTL */
#define IOCTL_IO_SET_REGISTER		_IOW(0xD0, 11, int)
#define IOCTL_IO_READ_REGISTER		_IOW(0xD0, 12, int)
#define IOCTL_MEM_SET_REGISTER		_IOW(0xD0, 13, int)
#define IOCTL_MEM_READ_REGISTER		_IOW(0xD0, 14, int)
#define IOCTL_SET_TX_DMA_REG		_IOW(0xD0, 15, int)
#define IOCTL_SET_RX_DMA_REG		_IOW(0xD0, 16, int)
#define IOCTL_TX_DMA_WRITE		_IOW(0xD0, 17, int)
#define IOCTL_RX_DMA_READ		_IOW(0xD0, 18, int)
#define IOCTL_SET_SEMA_INTERRUPT	_IOW(0xD0, 19, int)

enum MMAP_BAR {
	BAR1,
	BAR5
};

/* Register */
struct SPI_REG {
	unsigned char	Offset;
	unsigned char	Value;
};


struct MMAP_SPI_REG {
	enum MMAP_BAR	Bar;
	unsigned long	Offset;
	unsigned long	Value;
};

/* DMA Buffer */
struct SPI_DMA {
	unsigned long	Length;
	unsigned char	Buffer[128*1024];
};


/*
 *
 *Register (I/O mapped)
 *
 */
#define REG_SPICMR		0x000
#define SPICMR_SSP	(1 << 0)
#define SPICMR_CPHA	(1 << 1)
#define SPICMR_CPOL	(1 << 2)
#define SPICMR_LSB	(1 << 3)
#define SPICMR_SPIMEN	(1 << 4)
#define SPICMR_ASS	(1 << 5)
#define SPICMR_SWE	(1 << 6)
#define SPICMR_SSOE	(1 << 7)
#define REG_SPICSS		0x001
#define REG_SPIBRR		0x004
#define REG_SPIDS		0x005
#define REG_SPIDT		0x006
#define REG_SDAOF		0x007
#define REG_STOF0		0x008
#define REG_STOF1		0x009
#define REG_STOF2		0x00A
#define REG_STOF3		0x00B
#define REG_STOF4		0x00C
#define REG_STOF5		0x00D
#define REG_STOF6		0x00E
#define REG_STOF7		0x00F
#define REG_SDFL0		0x010
#define REG_SDFL1		0x011
#define REG_SPISSOL		0x012
#define REG_SDCR		0x013
#define INTERRUPT_ENABLE_MASK	0xC0
#define REG_SPIMISR		0x014
#define INTERRUPT_MASK	0x003
#define SPIMISR_STC	(1 << 0)
#define SPIMISR_STERR	(1 << 1)

/*
 *
 *Register (MEM mapped)
 *
 */

/* SPI Common Reg. */
#define REG_SWRST		0x238
#define SW_RESET	(1 << 0)
/* TX DMA */
#define REG_TDMASAR0		0x080
#define REG_TDMASAR1		0x084
#define REG_TDMALR		0x088
#define REG_TDMASTAR		0x08C
	#define START_DMA	(1 << 0)
#define REG_TDMASTPR		0x090
#define REG_TDMASR		0x094
#define REG_TBNTS		0x098
/* RX DMA */
#define REG_RDMASAR0		0x100
#define REG_RDMASAR1		0x104
#define REG_RDMALR		0x108
#define REG_RDMASTAR		0x10C
#define REG_RDMASTPR		0x110
#define REG_RDMASR		0x114
#define REG_RBNTS		0x118

/*
 *
 *DMA setting
 *
 */

#define	DMA_ABORT		(1 << 0)
#define	DMA_START		(1 << 0)
#define DMA_BUFFER_SZ		65535



#define PCI_SUBVEN_ID_AX99100_SPI		0x6000



#define OFFSET_EEPORM		0x0C8

#define FL_BASE5		0x0005

#if defined(__i386__) && (defined(CONFIG_M386) || defined(CONFIG_M486))
#define _INLINE_ inline
#else
#define _INLINE_
#endif

#define DEFAULT99100_BAUD 115200

/* Device */
#define	DEV_NAME	"ax99100"
#define	CLASS_NAME	"tcmdev"
#define NODE_NAME	"tcm"

/* Netlink */
#define NETLINK_TEST 17
#define MAX_PAYLOAD_SIZE 1024


/*
 *
 * Register (I/O mapped)
 * BAR0
 */
#define REG_SPI_CMR		0x000
#define REG_SPI_CSS		0x001
#define REG_SPI_BRR		0x004
#define REG_SPI_DS		0x005
#define REG_SPI_DT		0x006
#define REG_SPI_SDAOF	0x007
#define REG_SPI_STOF0	0x008
#define REG_SPI_STOF1	0x009
#define REG_SPI_STOF2	0x00A
#define REG_SPI_STOF3	0x00B
#define REG_SPI_STOF4	0x00C
#define REG_SPI_STOF5	0x00D
#define REG_SPI_STOF6	0x00E
#define REG_SPI_STOF7	0x00F
#define REG_SPI_SDFL0	0x010
#define REG_SPI_SDFL1	0x011
#define REG_SPI_SSOL	0x012
#define REG_SPI_SDCR	0x013
#define REG_SPI_MISR	0x014

/* SPI Common Reg. BAR1*/
#define REG_SWRST		0x238

/* SPI DMA. BAR1. NO USER*/

//GPIO BAR5
#define REG_GPIO_DATA	0x3C0
#define REG_GPIO_DIR	0x3C4
#define REG_GPIO_EM		0x3C8
#define REG_GPIO_OD		0x3CC
#define REG_GPIO_PU		0x3D0
#define REG_GPIO_EDS	0x3D4
#define REG_GPIO_EDE	0x3D8
#define REG_GPIO_CTR	0x3DC

#define EXT_CLOCK				25000000

enum emSPIMODE {
	EM_SPI_MODE0 = 0x00,
	EM_SPI_MODE1 = 0x04,
	EM_SPI_MODE2 = 0x02,
	EM_SPI_MODE3 = 0x06,
};

enum emSCS {
	EM_SCS_125M,
	EM_SCS_100M,
	EM_SCS_EXT,
};

enum emSPICS {
	EM_SPICS0,
	EM_SPICS1,
	EM_SPICS2,
	EM_SPICS3,
	EM_SPICS4,
	EM_SPICS5,
	EM_SPICS6,
	EM_SPICS7,
};

struct SpiFrameHeader {
	unsigned char body[4];
};

//static  struct mpsse_context* mpsse_;
static unsigned long __maybe_unused locality_;   // Set at initialization.

// Assorted TPM2 registers for interface type FIFO.
#define TPM_REG_BASE			0xd40000

#define TPM_ACCESS_REG (TPM_REG_BASE + locality_ * 0x1000 + 0x0)
#define TPM_STS_REG (TPM_REG_BASE + locality_ * 0x1000 + 0x18)
#define TPM_DATA_FIFO_REG (TPM_REG_BASE + locality_ * 0x1000 + 0x80)
#define TPM_DID_VID_REG (TPM_REG_BASE + locality_ * 0x1000 + 0xf00)
#define TPM_RID_REG (TPM_REG_BASE + locality_ * 0x1000 + 0xf04)

// Locality management bits (in TPM_ACCESS_REG)
enum TpmAccessBits {
	tpmRegValidSts = (1 << 7),
	activeLocality = (1 << 5),
	requestUse = (1 << 1),
	tpmEstablishment = (1 << 0),
};

enum TpmStsBits {
	tpmFamilyShift = 26,
	tpmFamilyMask = ((1 << 2) - 1),  // 2 bits wide
	tpmFamilyTPM2 = 1,
	resetEstablishmentBit = (1 << 25),
	commandCancel = (1 << 24),
	burstCountShift = 8,
	burstCountMask = ((1 << 16) - 1),  // 16 bits wide
	stsValid = (1 << 7),
	commandReady = (1 << 6),
	tpmGo = (1 << 5),
	dataAvail = (1 << 4),
	Expect = (1 << 3),
	selfTestDone = (1 << 2),
	responseRetry = (1 << 1),
};


struct spi_99100 {
	struct cdev spi;

	unsigned int dev_major;
	unsigned int dev_minor;
	char dev_name[64];

	unsigned long iobase0;
	unsigned char __iomem *membase[2];
	resource_size_t mapbase[2];


	unsigned int irq;


	//Virtual Address of DMA Buffer for TX
	char *tx_dma_v;
	//Physical Address of DMA Buffer for TX
	dma_addr_t tx_dma_p;
	//Virtual Address of DMA Buffer for RX
	char *rx_dma_v;
	//Physical Address of DMA Buffer for RX
	dma_addr_t rx_dma_p;

	int tool_pid;
	int dev_resource_num;
	int dev_type;
	struct tpm_chip *tpm_chip;
};

void ResetSPI(int handle);
unsigned char GetSPIStatus(int handle, char clear);
void StartSPI(int handle, char interrupt);
unsigned long ReadWriteSpiData(int handle, unsigned char *txdata,
	unsigned long dlen, unsigned char *txrxdata, unsigned long rxlen);


int ReadSPIMemCfgReg(int handle, uint offset, ulong *RegValue);
int WriteSPIMemCfgReg(int handle, uint offset, ulong RegValue);
int ReadSPIIOCfgReg(int handle, uint offset, unsigned char *RegValue);
int WriteSPIIOCfgReg(int handle, uint offset, unsigned char RegValue);
int GetGPIOReg(int handle, uint offset, ulong *RegValue);
int SetGPIOReg(int handle, uint offset, ulong RegValue);

void Delayms(unsigned int dly);

int init_spi_func(int line);
void close_spi_func(int line);
int ax99100_recv(int line, u8 *buf, size_t len);
int ax99100_send(int line, u8 *buf, size_t len);


#endif
/* SPIDRIVER_H_ */
