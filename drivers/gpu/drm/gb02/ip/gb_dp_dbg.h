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

#include "common/gb_common.h"

#ifndef __DPTX_DBG_H__
#define __DPTX_DBG_H__

//#define DPTX_DEBUG_REG
//#define DPTX_DEBUG_AUX
#define GB02MAC1321
#define GB02MAC1323

#define dptx_dbg(_dp, _fmt...) gb_printf(KERN_DEBUG, _fmt)
#define dptx_info(_dp, _fmt...) gb_printf(KERN_INFO, _fmt)
#define dptx_warn(_dp, _fmt...) gb_printf(KERN_WARNING, _fmt)
#define dptx_err(_dp, _fmt...) gb_printf(KERN_ERR, _fmt)

#ifdef DPTX_DEBUG_AUX
#define dptx_dbg_aux(_dp, _fmt...) dev_err(_dp->dev, _fmt)
#else
#define dptx_dbg_aux(_dp, _fmt...)
#endif

#ifdef GB02MAC1321
#define dptx_dbg_irq(_dp, _fmt...) dev_err(_dp->dev, _fmt)
#else
#define dptx_dbg_irq(_dp, _fmt...)
#endif

#endif
