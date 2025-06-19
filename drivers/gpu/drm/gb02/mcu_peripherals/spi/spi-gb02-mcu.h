/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SPI_GB02_MCU_H
#define __SPI_GB02_MCU_H
#include "gb-peripherals-common.h"
#define GB02_SPI_DRIVER_NAME "GB02STR187"

#define GB02MAC2561 32
#define GB02MAC2562 4
#define GB02MAC2563 8

/* register offsets */
#define GB02MAC2565 0x00    /* Serial clock divisor */
#define GB02MAC2567 0x04   /* Serial clock mode */
#define GB02MAC2568 0x08 /* SPI data sampling divisor */
#define GB02MAC2570 0x0C     /* SPI oe ctrl when not use other pad */
#define GB02MAC2572 0x10      /* Chip select ID */
#define GB02MAC2573 0x14     /* Chip select default */
#define GB02MAC2574 0x18    /* Chip select mode */
#define GB02MAC2575 0x1C   /* SPI version */
#define GB02MAC2576 0x28    /* Delay control 0 */
#define GB02MAC2577 0x2c    /* Delay control 1 */
#define GB02MAC2578 0x40       /* Frame format */
#define GB02MAC2579 0x48    /* Tx FIFO data */
#define GB02MAC2580 0x4c    /* Rx FIFO data */
#define GB02MAC2581 0x50    /* Tx FIFO watermark */
#define GB02MAC2582 0x54    /* Rx FIFO watermark */
#define GB02MAC2583 0x60     /* SPI flash interface control */
#define GB02MAC2584 0x64      /* SPI flash instruction format */
#define GB02MAC2585 0x70        /* Interrupt Enable Register */
#define GB02MAC2586 0x74        /* Interrupt Pendings Register */
#define GB02MAC2587 0x78     /* SPI flash instruction format 1 */
#define GB02MAC2588 0x7C    /* SPI busy status */
#define GB02MAC2589 0x80    /* SPI RX sample edge ctrl */
#define GB02MAC2590 0x84        /* SPI control register */

/* spi version tags */
#define GB02MAC2591 0x00010100

/* sckdiv bits */
#define GB02MAC2592 0xfffU

/* sckmode bits */
#define GB02MAC2594 BIT(0)
#define GB02MAC2595 BIT(1)
#define GB02MAC2596 (GB02MAC2594 | GB02MAC2595)

/* csmode bits */
#define GB02MAC2597 0U
#define GB02MAC2598 2U
#define GB02MAC2599 3U

/* delay0 bits */
#define GB02MAC2600(x) ((u32)(x))
#define GB02MAC2601 0xffU
#define GB02MAC2602(x) ((u32)(x) << 16)
#define GB02MAC2604 (0xffU << 16)

/* delay1 bits */
#define GB02MAC2605(x) ((u32)(x))
#define GB02MAC2606 0xffU
#define GB02MAC2607(x) ((u32)(x) << 16)
#define GB02MAC2608 (0xffU << 16)

/* fmt bits */
#define GB02MAC2609 0U
#define GB02MAC2610 1U
#define GB02MAC2611 2U
#define GB02MAC2612 3U
#define GB02MAC2613 BIT(2)
#define GB02MAC2614 BIT(3)
#define GB02MAC2615(x) ((u32)(x) << 16)
#define GB02MAC2616 (0xfU << 16)

/* txdata bits */
#define GB02MAC2617 0xffU
#define GB02MAC2618 BIT(31)

/* rxdata bits */
#define GB02MAC2619 0xffU
#define GB02MAC2620 BIT(31)

/* ie and ip bits */
#define GB02MAC2621 BIT(0)
#define GB02MAC2622 BIT(1)

/* status bits */
#define GB02MAC2623 BIT(0)
#define GB02MAC2624 BIT(2)
#define GB02MAC2625 BIT(3)
#define GB02MAC2626 BIT(4)
#define GB02MAC2627 BIT(5)

#define GB02MAC2628 BIT(0)

#ifdef CONFIG_MCU_SPI
int GB02FUNC1723(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC1725(struct GB02STR72 *peri_info);
void  GB02FUNC1652(void);
int GB02FUNC1650(void);
#else
static inline int GB02FUNC1723(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC1725(struct GB02STR72 *peri_info)
{
	return;
}
static inline void  GB02FUNC1652(void)
{
	return;
}
static inline int GB02FUNC1650(void)
{
	return 0;
}
#endif
#endif /* __SPI_GB02_MCU_H */
