/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FAN_GB02_MCU_H
#define __FAN_GB02_MCU_H
#include "gb-peripherals-common.h"

#define GB02_FAN_DEV_NAME "gb02-fan"

#ifdef CONFIG_MCU_FAN
int GB02FUNC69(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC79(struct GB02STR72 *peri_info);
int GB02FUNC82(struct GB02STR72 *peri_info);
int GB02FUNC86(struct GB02STR72 *peri_info);
#else
static inline int GB02FUNC69(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC79(struct GB02STR72 *peri_info)
{
	return;
}
static inline int GB02FUNC82(struct GB02STR72 *peri_info)
{
	return 0;
}
static inline int GB02FUNC86(struct GB02STR72 *peri_info)
{
	return 0;
}
#endif
#endif /* __FAN_GB02_MCU_H */
