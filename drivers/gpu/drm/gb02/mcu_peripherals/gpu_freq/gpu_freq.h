/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FREQ_GB02_MCU_H
#define __FREQ_GB02_MCU_H
#include <linux/notifier.h>
#include "gb-peripherals-common.h"

#define GB02MAC1109 0
#define GB02MAC1110 1
#define GB02MAC1111 2

extern struct blocking_notifier_head limit_freq_notifier_chain;

#ifdef CONFIG_MCU_FREQ
u32 GB02FUNC693(void);
int GB02FUNC754(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC755(struct GB02STR72 *peri_info);
void GB02FUNC698(struct GB02STR39 *gb_dev, int slot);
#else
static inline u32 GB02FUNC693(void)
{
	return 0;
}
static inline int GB02FUNC754(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC755(struct GB02STR72 *peri_info)
{
	return;
}
static inline void GB02FUNC698(struct GB02STR39 *gb_dev, int slot)
{
	return;
}
#endif
#endif /* __FREQ_GB02_MCU_H */
