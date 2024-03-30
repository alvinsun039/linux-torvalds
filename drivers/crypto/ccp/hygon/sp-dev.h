/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON Secure Processor interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef __CCP_HYGON_SP_DEV_H__
#define __CCP_HYGON_SP_DEV_H__

#include <linux/processor.h>
#include <linux/ccp.h>

#include <linux/machine_t.h>

#include "../ccp-dev.h"
#include "../sp-dev.h"

extern const struct sp_dev_vdata hygon_dev_vdata[];

#endif	/* __CCP_HYGON_SP_DEV_H__ */
