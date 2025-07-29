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

#include <linux/module.h>
#ifndef __GBDC_REGS_H__
#define __GBDC_REGS_H__

/*
 * abbreviations used:
 *    - DC - display core (general settings)
 *    - DE - display engine
 *    - SE - scaling engine
 */

/* interrupt bit masks */
#define GB02MAC564			(1 << 0)

#define GB02MAC565		(1 << 8)
#define GB02MAC566			(1 << 12)
#define GB02MAC567		(1 << 13)
#define GB02MAC568		(1 << 16)
#define GB02MAC569			(1 << 0)
#define GB02MAC570		(1 << 16)
#define GB02MAC571		(1 << 0)
#define GB02MAC572		(1 << 4)
#define GB02MAC573		(1 << 16)
#define GB02MAC574			(1 << 20)
#define GB02MAC575			(1 << 24)

/* bit masks that are common between products */
#define   GB02MAC576		(1 << 0)
#define   GB02MAC577	(1 << 0)
#define   GB02MAC578		(1 << 4)
#define   GB02MAC579	(1 << 8)

/* register offsets for IRQ management */
#define GB02MAC580		0x00000
#define GB02MAC581		0x00004
#define GB02MAC582		0x00008
#define GB02MAC583		0x0000c

/* register offsets */
#define GB02MAC584		0x00018
#define GB02MAC585		0x00020

/* these offsets are relative to GBDC5x0_TIMINGS_BASE */
#define GB02MAC586		0x0
#define GB02MAC587		0x4
#define GB02MAC588		0x8
#define GB02MAC589		0xc

/* Stride register offsets relative to Lx_BASE */
#define GB02MAC590		0x18
#define GB02MAC591		0x18
#define GB02MAC592	0x28

/* macros to set values into registers */
#define GB02MAC593(x)	(((x) & 0xfff) << 0)
#define GB02MAC594(x)	(((x) & 0x3ff) << 16)
#define GB02MAC595(x)	(((x) & 0xfff) << 0)
#define GB02MAC596(x)	(((x) & 0xff) << 16)
#define GB02MAC597(x)	(((x) & 0x3ff) << 0)
#define GB02MAC598(x)	(((x) & 0xff) << 16)
#define GB02MAC599(x)		(((x) & 0x1fff) << 0)
#define GB02MAC600(x)		(((x) & 0x1fff) << 16)

#define GB02MAC601(__core_id) ((u32)(__core_id) >> 16)

/* register offsets relative to GBDC5x0_COEFFS_BASE */
#define GB02MAC602		0x00000
#define GB02MAC603		0x00030
#define GB02MAC604		0x00034

/* Scaling engine registers and masks. */
#define   GB02MAC605			(1 << 0)
#define   GB02MAC606			(1 << 1)
#define   GB02MAC607			3
#define   GB02MAC608(x)			(((x) & GB02MAC607) << 2)
#define   GB02MAC609			(1 << 4)
#define   GB02MAC610		7
#define   GB02MAC611(x) \
		(((x) & GB02MAC610) << 20)
#define   GB02MAC612(x) \
		(((x) & GB02MAC610) << 16)

/* Blocks with offsets from SE_CONTROL register. */
#define GB02MAC613			0x14
#define   GB02MAC614			0x00
#define   GB02MAC615			0x04
#define   GB02MAC616(x)		(((x) & 0x1fff) << 16)
#define   GB02MAC617(x)		(((x) & 0x1fff) << 0)
#define GB02MAC618		0x24
#define   GB02MAC619			0x00
#define   GB02MAC620			0x04
#define   GB02MAC622			0x08
#define   GB02MAC623			0x0c
#define   GB02MAC624		0x10
#define     GB02MAC625	0x7f
#define     GB02MAC626		(1 << 8)
#define     GB02MAC627		(1 << 9)
#define     GB02MAC628(x) \
		(GB02MAC626 | ((x) & GB02MAC625))
#define     GB02MAC629(x) \
		(GB02MAC627 | ((x) & GB02MAC625))
#define   GB02MAC630		0x14
#define     GB02MAC631	0x3fff
#define     GB02MAC632(x) \
		((x) & GB02MAC631)
/* Enhance coeffents reigster offset */
#define GB02MAC633			0x3C
/* ENH_LIMITS offset 0x0 */
#define     GB02MAC634		24
#define     GB02MAC635		63
#define     GB02MAC636		0xfff
#define     GB02MAC637(x) \
		((x) & GB02MAC636)
#define     GB02MAC638(x) \
		(((x) & GB02MAC636) << 16)
#define   GB02MAC639			0x04

#define GB02MAC640			0x00200

#define GB02MAC641	0x10000
#define GB02MAC642		0x00010
#define GB02MAC643	0x00014
#define GB02MAC644	0x00018
#define GB02MAC645		0x0001c
#define GB02MAC646		0x00030
#define GB02MAC647		(1 << 12)
#define GB02MAC648		(1 << 28)

#define GB02MAC649	0x00040
#define GB02MAC650		0x00044
#define GB02MAC651	0x0004c
#define GB02MAC653		0x00050
#define GB02MAC654		0x00080
#define GB02MAC655		0x00084
#define GB02MAC657		0x00100
#define GB02MAC659	0x00124
#define GB02MAC661		0x00200
#define GB02MAC663	0x00224
#define GB02MAC665		0x00300
#define GB02MAC667	0x0031c
#define GB02MAC669		0x00400
#define GB02MAC671	0x0042c
#define GB02MAC673		0x00500
#define GB02MAC674		0x08000
#define GB02MAC676		0x08010
#define GB02MAC678		0x0c000
#define GB02MAC679		0x0c010
#define GB02MAC680	(1 << 16)
#define GB02MAC681	(1 << 17)
#define GB02MAC682	(1 << 18)
#define GB02MAC683		0x0c014
#define GB02MAC684		0x0ffd4



#define	GB02MAC685					(0x00100000UL)
#define	GB02MAC686					(0x00020000UL)
#define GB02MAC687              (0x00000000UL)
#define GB02MAC688               (0x00010000UL)
#define GB02MAC689         (0x00020000UL)
#define GB02MAC690              (0x00040000UL)
#define GB02MAC691          (0x00080000UL)

#define	GB02MAC692					(0x00020000UL)
#define	GB02MAC693					(0x00020000UL)

#define	GB02MAC694	0x00010
#define	GB02MAC695	0X00014



/* Layer specific register offsets */
#define GB02MAC696		0x004
#define   GB02MAC698			(1 << 0)
#define   GB02MAC700		7
#define   GB02MAC702(x)		(((x) & GB02MAC700) << 1)
#define     GB02MAC704	3
#define   GB02MAC706		8
#define   GB02MAC708			(1 << 10)
#define	  GB02MAC711		(1 << 12)
#define   GB02MAC714			(1 << 11)
#define   GB02MAC716		(0xf << 8)
#define   GB02MAC719		(0x3 << 12)
#define   GB02MAC722		(0xff << 16)
#define GB02MAC725		0x008
#define GB02MAC727		0x00c
#define   GB02MAC730(x)		(((x) & 0x1fff) << 0)
#define   GB02MAC732(x)		(((x) & 0x1fff) << 16)
#define GB02MAC735		0x010
#define GB02MAC738		0x014
#define GB02MAC741		0x020

#define GB02MAC746		0x000
#define GB02MAC749		0x000
#define GB02MAC752		0x000

/* REG */
#define GB02MAC755 0x3004
#define GB02MAC758 0x202
#define GB02MAC761 0x02
#define GB02MAC764 0x01
#define	GB02MAC767	'h'
#define	GB02MAC769			'r'
#define	GB02MAC771			0x203
#define	GB02MAC773			0x204
#define	GB02MAC775			0x205
#define	GB02MAC778			0x206
#define	GB02MAC781			0x218
#define	GB02MAC784			0x21c
#define	GB02MAC787		0x200
#define	GB02MAC790			0x204

/*
 * bit7 : 1 = GB01 access; 0 = MCU access;
 * bit6 : 1 = GB01 read; 0 = GB01 write;
 */
#define GB02MAC794		0x1201
#define GB02MAC797		7
#define GB02MAC798		6
#define GB02MAC800		0x1202
#define GB02MAC803	0x1203
#define GB02MAC805	0x1204

/*
 * bit7 : 1 = GB01 access; 0 = MCU access;
 * bit6 : 1 = GB01 read; 0 = GB01 write;
 */
#define GB02MAC807			0x1205
#define GB02MAC809		7
#define GB02MAC811			6
#define GB02MAC813		0x1206

/*
 * bit2 : 1 = VGA connector; 0 = others;
 * bit[1:0] : 1 = MXM BOARD; 0 = PCIE BOARD;
 */
#define GB02MAC816			0x1207
#define GB02MAC818			0x3
#define GB02MAC820		2

#endif				/* __GBDC_REGS_H__ */
