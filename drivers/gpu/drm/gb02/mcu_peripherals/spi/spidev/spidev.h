/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 * include/linux/spi/spidev.h
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GB02MAC2382
#define GB02MAC2382

#include <linux/types.h>
#include <linux/ioctl.h>

/* User space versions of kernel symbols for SPI clocking modes,
 * matching <linux/spi/spi.h>
 */

#define GB02MAC2386 0x01
#define GB02MAC2388 0x02

#define GB02MAC2390 (0 | 0)
#define GB02MAC2392 (0 | GB02MAC2386)
#define GB02MAC2394 (GB02MAC2388 | 0)
#define GB02MAC2396 (GB02MAC2388 | GB02MAC2386)

#define GB02MAC2398 0x04
#define GB02MAC2400 0x08
#define GB02MAC2402 0x10
#define GB02MAC2404 0x20
#define GB02MAC2406 0x40
#define GB02MAC2408 0x80
#define GB02MAC2410 0x100
#define GB02MAC2412 0x200
#define GB02MAC2414 0x400
#define GB02MAC2416 0x800
#define GB02MAC2418 0x1000
#define GB02MAC2420 0x2000
#define GB02MAC2422 0x4000
#define GB02MAC2423 0x8000

/*---------------------------------------------------------------------------*/

/* IOCTL commands */

#define GB02MAC2427 'k'

/**
 * struct GB02STR183 - describes a single SPI transfer
 * @tx_buf: Holds pointer to userspace buffer with transmit data, or null.
 *	If no data is provided, zeroes are shifted out.
 * @rx_buf: Holds pointer to userspace buffer for receive data, or null.
 * @len: Length of tx and rx buffers, in bytes.
 * @speed_hz: Temporary override of the device's bitrate.
 * @bits_per_word: Temporary override of the device's wordsize.
 * @delay_usecs: If nonzero, how long to delay after the last bit transfer
 *	before optionally deselecting the device before the next transfer.
 * @cs_change: True to deselect device before starting the next transfer.
 * @word_delay_usecs: If nonzero, how long to wait between words within one
 *	transfer. This property needs explicit support in the SPI controller,
 *	otherwise it is silently ignored.
 *
 * This structure is mapped directly to the kernel spi_transfer structure;
 * the fields have the same meanings, except of course that the pointers
 * are in a different address space (and may be of different sizes in some
 * cases, such as 32-bit i386 userspace over a 64-bit x86_64 kernel).
 * Zero-initialize the structure, including currently unused fields, to
 * accommodate potential future updates.
 *
 * SPI_IOC_MESSAGE gives userspace the equivalent of kernel spi_sync().
 * Pass it an array of related transfers, they'll execute together.
 * Each transfer may be half duplex (either direction) or full duplex.
 *
 *	struct GB02STR183 mesg[4];
 *	...
 *	status = ioctl(fd, SPI_IOC_MESSAGE(4), mesg);
 *
 * So for example one transfer might send a nine bit command (right aligned
 * in a 16-bit word), the next could read a block of 8-bit data before
 * terminating that command by temporarily deselecting the chip; the next
 * could send a different nine bit command (re-selecting the chip), and the
 * last transfer might write some register values.
 */
struct GB02STR183 {
	__u64 tx_buf;
	__u64 rx_buf;

	__u32 len;
	__u32 speed_hz;

	__u16 delay_usecs;
	__u8 bits_per_word;
	__u8 cs_change;
	__u8 tx_nbits;
	__u8 rx_nbits;
	__u8 word_delay_usecs;
	__u8 pad;

	/* If the contents of 'struct GB02STR183' ever change
	 * incompatibly, then the ioctl number (currently 0) must change;
	 * ioctls with constant size fields get a bit more in the way of
	 * error checking than ones (like this) where that field varies.
	 *
	 * NOTE: struct layout is the same in 64bit and 32bit userspace.
	 */
};

/* not all platforms use <asm-generic/ioctl.h> or _IOC_TYPECHECK() ... */
#define GB02MAC2434(N)                                                  \
	((((N) * (sizeof(struct GB02STR183))) < (1 << _IOC_SIZEBITS)) \
		 ? ((N) * (sizeof(struct GB02STR183)))                    \
		 : 0)
#define SPI_IOC_MESSAGE(N) _IOW(GB02MAC2427, 0, char[GB02MAC2434(N)])

/* Read / Write of SPI mode (GB02MAC2390..GB02MAC2396) (limited to 8 bits) */
#define SPI_IOC_RD_MODE _IOR(GB02MAC2427, 1, __u8)
#define SPI_IOC_WR_MODE _IOW(GB02MAC2427, 1, __u8)

/* Read / Write SPI bit justification */
#define SPI_IOC_RD_LSB_FIRST _IOR(GB02MAC2427, 2, __u8)
#define SPI_IOC_WR_LSB_FIRST _IOW(GB02MAC2427, 2, __u8)

/* Read / Write SPI device word length (1..N) */
#define SPI_IOC_RD_BITS_PER_WORD _IOR(GB02MAC2427, 3, __u8)
#define SPI_IOC_WR_BITS_PER_WORD _IOW(GB02MAC2427, 3, __u8)

/* Read / Write SPI device default max speed hz */
#define SPI_IOC_RD_MAX_SPEED_HZ _IOR(GB02MAC2427, 4, __u32)
#define SPI_IOC_WR_MAX_SPEED_HZ _IOW(GB02MAC2427, 4, __u32)

/* Read / Write of the SPI mode field */
#define SPI_IOC_RD_MODE32 _IOR(GB02MAC2427, 5, __u32)
#define SPI_IOC_WR_MODE32 _IOW(GB02MAC2427, 5, __u32)

#endif /* GB02MAC2382 */
