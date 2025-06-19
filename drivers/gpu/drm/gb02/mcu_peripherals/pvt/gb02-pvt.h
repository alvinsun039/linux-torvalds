/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __GB02_PVT__
#define __GB02_PVT__
#include "gb-peripherals-common.h"

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define GB02MAC1601 0x82400000
#define GB02MAC1602 0x80000000

#define GB02MAC1604 GB02MAC721
#define GB02MAC1606 GB02MAC1059 // 0x82400000

/* 75 25 typical conditions steady DOUT values */
#define DOUT75std 2874LL
#define DOUT25std 2761LL

/* assumes 75 and 25 degrees actual conditions DOUT values, for now*/
#define DOUT75act 2902LL
#define DOUT25act 2784LL

#define DOUTstd 2067
#define Vref 1170 // mV
#define GB02MAC1612 3000
#define GB02MAC1614 2000
#define D1 ((3 * DOUT75act - DOUT25act) / 2)
#define D2 ((DOUT75act + DOUT25act) / 2)
#define D3 ((3 * DOUT25act - DOUT75act) / 2)
#define D4 (DOUT25act - (DOUT75act - DOUT25act) * 9 / 10)

#define GB02MAC1617 (0x00)

#define GB02MAC1618 20
#define GB02MAC1619(x) (((x)&0x7ff) << GB02MAC1618) // default 3000
#define GB02MAC1621 8
#define GB02MAC1623(x) (((x)&0x7ff) << GB02MAC1621) // default 2500
#define GB02MAC1625 BIT(7)															 // 1: power up  0:power down --default 1
#define GB02MAC1627 1
#define GB02MAC1629 0
#define PVT_RESETn_MASK BIT(6) // low active ,default 0
#define GB02MAC1631 0
#define GB02MAC1633 1

#define GB02MAC1635 BIT(5) // 1:temperature 0 :voltage default 1
#define GB02MAC1637 1
#define GB02MAC1639 0
#define TEST_EN_MASK BIT(4)		   // IP test enable signal ,defaul 0
#define BAND_GAP_TRIM(x) ((x)&0x7) // bandgap trim signal ,default 4'b0111

#define GB02MAC1643 (0x04)

#define GB02MAC1645 12
#define GB02MAC1647(x) (((x)&0x7ff) << GB02MAC1645) // default 3000
#define GB02MAC1649 0
#define GB02MAC1651(x) (((x)&0x7ff) << GB02MAC1649) // default 2500

#define GB02MAC1653 (0x08)
#define GB02MAC1654 12
#define GB02MAC1655(x) (((x)&0x7ff) << GB02MAC1654) // default 3000
#define GB02MAC1657 0
#define GB02MAC1659(x) (((x)&0x7ff) << GB02MAC1657) // default 2500

#define GB02MAC1663 (0x0C)

#define GB02MAC1665 24
#define GB02MAC1667(x) (((x)&0xff) << GB02MAC1665) // default 8'd31

#define GB02MAC1670 12
#define GB02MAC1673(x) (((x)&0x7ff) << GB02MAC1670) // default 3000
#define GB02MAC1676 0
#define GB02MAC1679(x) (((x)&0x7ff) << GB02MAC1676) // default 2500

#define GB02MAC1682 (0x10)
#define GB02MAC1685 BIT(3) // write 1 clear default 0
#define GB02MAC1688 BIT(2)	 // write 1 clear default 0
#define GB02MAC1691 BIT(1) // write 1 clear default 0
#define GB02MAC1694 BIT(0)	 // write 1 clear default 0

#define GB02MAC1698 (0x14)

#define GB02MAC1702 BIT(19)
#define GB02MAC1704 BIT(18)
#define GB02MAC1707 BIT(17)
#define GB02MAC1710 BIT(16)
#define GB02MAC1713 BIT(15)
#define GB02MAC1716 BIT(14)
#define GB02MAC1719 BIT(13)
#define GB02MAC1722 BIT(12)
#define DOUT(x) ((x)&0xfff)

#ifdef CONFIG_MCU_PVT
int GB02FUNC1181(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC1182(struct GB02STR72 *peri_info);
int GB02FUNC1158(void);
#else
static inline int GB02FUNC1181(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC1182(struct GB02STR72 *peri_info)
{
	return;
}
static inline int GB02FUNC1158(void)
{
	return 0;
}
#endif
#endif
