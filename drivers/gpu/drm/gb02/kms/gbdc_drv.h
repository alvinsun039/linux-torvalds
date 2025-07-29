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
#ifndef		__GBDC_DRV_H__
#define		__GBDC_DRV_H__


#include <linux/mutex.h>
#include <linux/wait.h>
#include <generated/uapi/linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <linux/backlight.h>
#if (defined CONFIG_CENTOS && !defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_state_helper.h>
#endif
#include "gbdc_dev.h"
#include "common/gb02_debugfs.h"

/* KMS ATOMIC Feature */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0))
#define CONFIG_GBDC_ATOMIC_FEATURE
#endif

//#define CONFIG_GBDC_DEBUG
#define		GB02MAC1822		1


struct gb_gpu_info{

};

typedef enum {
	GENBU_PCIE_BOARD = 0,
	GENBU_MXM_BOARD,
	GENBU_BOARD_MAX,
} genbu_board_type_e;

genbu_board_type_e GB02FUNC1184(void);

struct GB02STR155 {
	struct device			*dev;
	struct pci_dev 			*pdev;
	struct drm_device		*drm_dev;

	struct GB02STR254	pcie_info;
	struct GB02STR245		kms_info;
	struct gb_gpu_info		gpu_info;
	struct GB02STR153		*dev_info;
	//struct GB02STR176		gb_mm;
	struct GB02STR242		pm_info;
	struct GB02STR199	*ip_mode_ops;

	int						card_type;
	int						board_type;
	struct backlight_device *backlight;
	struct mutex            bk_mutex;
	struct GB02STR22 gb_dbg_info;
};

int GB02FUNC1185(struct drm_device *drm, unsigned long flags);
#if !(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009 || \
	defined SYS_CENTOS7_COMPILE_ENV) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
int gbdc_unload(struct drm_device *dev);
#else
void gbdc_unload(struct drm_device *dev);
#endif


int  GB02FUNC1190(struct GB02STR70 *gpi);
#endif
