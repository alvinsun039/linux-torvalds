/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __GB_BACKLIGHT_H
#define __GB_BACKLIGHT_H

#include <linux/backlight.h>
#include "gpu/gb_device.h"

struct GB02STR12 {
	void __iomem *sysctl_iofunc_base;
	u32 last_brightness;
	struct dptx *dptx_info;
	struct backlight_device *bl;
	bool by_dptx;
};

#ifdef CONFIG_GB_BACKLIGHT
struct GB02STR12 *GB02FUNC2(int index);
int GB02FUNC10(struct GB02STR70 *pcie_info, struct dptx *dptx_info);
void GB02FUNC20(struct dptx *dptx_info);
#else
static inline struct GB02STR12 *GB02FUNC2(int index){return NULL;}
static inline int GB02FUNC10(struct GB02STR70 *pcie_info, struct dptx *dptx_info){return -1;}
static inline void GB02FUNC20(struct dptx *dptx_info){return;}
#endif
#endif
