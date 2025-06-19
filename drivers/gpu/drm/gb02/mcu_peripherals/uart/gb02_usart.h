/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __UART_GB02_MCU_H
#define __UART_GB02_MCU_H
#include "gb-peripherals-common.h"

#ifdef CONFIG_MCU_UART
int GB02FUNC1841(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC1842(struct GB02STR72 *peri_info);
#else
static inline int GB02FUNC1841(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC1842(struct GB02STR72 *peri_info)
{
	return;
}
#endif
#endif /* __UART_GB02_MCU_H */
