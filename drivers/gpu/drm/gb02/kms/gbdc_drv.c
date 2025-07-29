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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/compat.h>	/* is_compat_task */
#include <linux/mman.h>
#include <linux/version.h>

#ifdef CONFIG_PM_DEVFREQ
#include <linux/devfreq.h>
#endif /* CONFIG_PM_DEVFREQ */
#include <linux/clk.h>
#include <linux/delay.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
#include <linux/pm_opp.h>
#else
#include <linux/opp.h>
#endif
#if (defined CONFIG_CENTOS && !defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#include <drm/drm_probe_helper.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#else
#include <drm/drm_drv.h>
#include <drm/drm_vblank.h>
#endif
#include <drm/drm_modes.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>

#include <drm/drm_gem.h>
#include <drm/drm_fb_helper.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif
#ifndef	CONFIG_LOONGSON_OS
#include <drm/drm_of.h>
#endif
#include "common/gb_common.h"
#ifdef	GB02MAC485
#include "linux/kthread.h"
#endif
#include "gbdc_mode.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "gbdc_planes.h"
#include "gbdc_crtc.h"
#include "gbdc_encoder.h"
#include "gbdc_mode.h"
#include "gbdc_display.h"
#include "gbdc_drv.h"
#include "common/gb_procfs.h"
#include "common/gb_common.h"
#include "device/gb_ip.h"
#include "gbdc_hdmi_ddc.h"
#include "gpu/gb_device.h"
#include "gbdc_backlight.h"
#include "gpu/gb_ttm.h"
#include "common/gb_uk.h"
#include "vpu/vpu_comm/gb_vpu.h"
#include "vpu/vpu_comm/vpu_comm.h"
#include "vpu/vpu_mm/gb_vpu_gem.h"
#include "gpu/gb_gpu_irq.h"
#include "ip/gb_dp.h"
#include "common/gb02_debugfs.h"
//#include "gpu/btsgpu_pcie_info.h"
//#include "gpu/core/btsgpu_core_linux.h"

genbu_board_type_e gb_board_type = GENBU_PCIE_BOARD;

#ifdef GB02MAC485
static struct task_struct *hpd_thread = NULL;
static int GB02FUNC1175(void *arg)
{

//	uint32_t  phy_status = 0;
	int conn_id, num_conn;
	uint32_t h_status = 0, o_status = 0;
	unsigned int conn_stat;
//	struct drm_connector *connector;
//	struct gbdc_connector *gb_connector;
	struct drm_device *drm_dev = (struct drm_device *)arg;
	struct GB02STR155 *gb_dev =
			GB02FUNC85(drm_dev->dev_private);
	struct GB02STR152 *modeset_ops = gb_dev->dev_info->modeset_ops;

	num_conn = gb_dev->kms_info.num_connector;
	while(!kthread_should_stop()) {
		h_status = 0;
		schedule_timeout_interruptible(1 * HZ);

#if 0
		drm_for_each_connector(connector, drm_dev) {
			gb_connector = to_gbdc_connector(connector);
			conn_id = gb_connector->connector_id;
			phy_status =
			 kms_ops->connector_state(hdmiBase, conn_id);
			if(phy_status == connector_status_connected){
				h_status |= (GB02MAC1822<<gb_connector->connector_id);
			}
		}
#else
		for (conn_id = 0, conn_stat = 0; conn_id < num_conn;
					conn_id++) {
			if (modeset_ops->connector_state)
				conn_stat = modeset_ops->connector_state(
					&gb_dev->pcie_info.ip_config, conn_id);
			if (conn_stat)
				h_status |= (GB02MAC1822 << conn_id);
		}
#endif
		if (o_status != h_status){
			DRM_DEBUG("#%d connected: %d\n", conn_id, h_status);
			gb_printf(KERN_INFO, "gb %d connected %d\n", conn_id, h_status);
			o_status = h_status;
			drm_helper_hpd_irq_event(drm_dev);
		}
	}
	return 0;
}


static int GB02FUNC1177(struct drm_device *drm_dev)
{
	hpd_thread = kthread_run(GB02FUNC1175, (void *)drm_dev, "hpd_thread");
	if (IS_ERR(hpd_thread)) {
		return -EFAULT;
	}
	return 0;
}

#endif
static int GB02FUNC1178(void __iomem *ip_base)
{
	GB02FUNC661(ip_base);
	return 0;
}

static int GB02FUNC1179(struct GB02STR254 *pcie_info,
	struct GB02STR39 *gbdev)
{
	int i;
	int num_bar = gbdev->gb_pcie->GB02STR153->pci_bar_cnt;
	int fb_offset = pcie_info->vram_config.fb_start_offset;
	int ddr_bar_id = gbdev->gb_pcie->GB02STR153->ddr_bar_id;
	struct GB02STR253 *vram_config = &pcie_info->vram_config;

	DRM_INFO("running %s: %d\n", __func__, __LINE__);

	if (pcie_info == NULL) {
		DRM_ERROR("hwdev or gb_pcie is NULL %s\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < num_bar; i++) {
		DRM_INFO("i: %d GB02MAC2844: %d\n", i, GB02MAC2844);

		pcie_info->start_bar[i] = gbdev->gb_pcie->pci_bars[i].base;
		pcie_info->len_bar[i] = gbdev->gb_pcie->pci_bars[i].len;

		DRM_INFO("PCI BAR[%d] start: 0x%lx, size: 0x%llx\n", i,
			pcie_info->start_bar[i], pcie_info->len_bar[i]);
		if (pcie_info->len_bar[i] == 0) {
			continue;
		}

		pcie_info->pci_mmio_bar[i] = gbdev->gb_pcie->pci_bars[i].mmio;
		if (i == ddr_bar_id) {
			vram_config->fb_base = pcie_info->start_bar[ddr_bar_id] + fb_offset;
			vram_config->fb_map  = pcie_info->pci_mmio_bar[ddr_bar_id] + fb_offset;
			DRM_INFO("hw_init*** fb_base: 0x%lx fb_map: 0x%pK\n",
				vram_config->fb_base, vram_config->fb_map);
		}
		DRM_INFO("pci bar%d phy_addr: 0x%lx, virt_addr: 0x%pK\n",
			   i, pcie_info->start_bar[i], pcie_info->pci_mmio_bar[i]);
	}

	DRM_INFO("exiting... %s: %d\n", __func__, __LINE__);

	return 0;
}

void GB02FUNC1183(struct GB02STR155 *gb_dev)
{
	u32 val = 0;
	genbu_board_type_e bd_type = GENBU_PCIE_BOARD;

	switch (val) {
	case 1:
		bd_type = GENBU_MXM_BOARD;
		break;
	default:
		break;
	}

	gb_dev->board_type = bd_type;
	gb_board_type = bd_type;
	gb_printf(KERN_INFO, "---gb- BOARD TYPE = %d\n", gb_dev->board_type);
}

genbu_board_type_e GB02FUNC1184(void)
{
	return gb_board_type;
}

int GB02FUNC1185(struct drm_device *drm, unsigned long flags)
{
	int ret;
#ifdef GB02MAC481
	int i;
#endif
	int card_type;
	int num_crtc, num_connector;
	struct GB02STR155 *gbdc_dev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct pci_dev *pdev = drm->pdev;
	struct device *dev = &pdev->dev;
#else
	struct pci_dev *pdev = to_pci_dev(drm->dev);
	struct device *dev = drm->dev;
#endif
	struct GB02STR39 *gbdev;
	struct GB02STR253 *vram_config;
	struct GB02STR160 *fb_info;
	struct GB02STR3 cmd_pool;
	struct GB02STR3 cmd_reg;

	gbdev = (struct GB02STR39 *)flags;
	if (!gbdev) {
		DRM_ERROR("%s %d:gbdev is NULL!\n", __func__, __LINE__);
		return -ENODEV;
	}

	gbdev->ddev = drm;
	drm->dev_private = gbdev;
	if (!drm_core_check_feature(drm, DRIVER_MODESET)){
		gb_printf(KERN_ERR, "---gb drm has no kms--\n");
		pci_set_drvdata(pdev, drm);
		gbdev->gbdc_dev = NULL;
#ifdef GB02MAC481
		return 0;
#endif
	}

	gbdc_dev = devm_kzalloc(dev, sizeof(struct GB02STR155), GFP_KERNEL);
	if (gbdc_dev == NULL)
		return -ENOMEM;
	gbdev->gbdc_dev = gbdc_dev;

	gb_printf(KERN_INFO, "---gb drm has kms- CARD TYPE = %d\n",
			gbdev->gb_pcie->GB02STR153->gb_type);

	card_type = gbdev->gb_pcie->GB02STR153->gb_type;

	GB02FUNC67(gbdc_dev, drm, pdev, card_type);
	GB02FUNC1179(&gbdc_dev->pcie_info, gbdev);
	GB02FUNC76(gbdc_dev);
	GB02FUNC1183(gbdc_dev);

	num_crtc = gbdc_dev->kms_info.num_crtc;
	num_connector = gbdc_dev->kms_info.num_connector;
	vram_config = &gbdc_dev->pcie_info.vram_config;
	fb_info = &gbdc_dev->kms_info.fb_info;
	ret = GB02FUNC1480(gbdc_dev->drm_dev, vram_config, &gbdev->gb_mm.ttm);
	if (ret) {
		DRM_ERROR( "gbdc_mm_init error: %d\n", ret);
		return -1;
	}
	gb_printf(KERN_INFO, "zfldebug %s %d \n", __func__, __LINE__);

#ifdef GB02MAC481
	ret = GB02FUNC1899(&gbdc_dev->kms_info, gbdc_dev->drm_dev, vram_config);
	if (ret) {
		DRM_ERROR("%s %d:GB02FUNC1899 fail! %d\n",
			__func__, __LINE__, ret);
		goto query_hw_fail;
	}

#ifdef GB02MAC481
	for (i = 0; i < num_connector; i++) {
		if ((GB02FUNC503(GB02FUNC518()) == PCIE_LPDDR4 ||
			GB02FUNC503(GB02FUNC518()) == PCIE_C0_200)
			&& ((i != 2) && (i != 3)))
			continue;
		else if (GB02FUNC503(GB02FUNC518())
			== PCIE_FULL_LPDDR4 && i == 1)
			continue;
		else if ((GB02FUNC503(GB02FUNC518())
			== PCIE_HIE1LP4_LPDDR4) && ((i == 1) || (i == 4)))
			continue;
		else if ((GB02FUNC503(GB02FUNC518())
			== PCIE_M4HL8G_LPDDR4) && ((i == 1) || (i == 4)))
			continue;

		/* enable dp irq & etc */
		ret = GB02FUNC1001(i, drm);
		if (ret) {
			DRM_ERROR("GB02FUNC1001 error:dp-%d\n", i);
			return ret;
		}
	}
#endif
#endif

	if (gbdc_dev->board_type == GENBU_MXM_BOARD) {
		/* backlight */
		ret = GB02FUNC749(gbdc_dev);
	}
	ret = drm_vblank_init(gbdc_dev->drm_dev, drm->mode_config.num_crtc);
	if (ret) {
		DRM_ERROR("%s %d:drm_vblank_init fail! %d\n",
			__func__, __LINE__, ret);
		goto query_hw_fail;
	}
#if (KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE)
	gbdc_dev->drm_dev->irq_enabled = true;
#endif

#ifdef GB02MAC485
	if (gbdc_dev->dev_info->gb_kthread)
		ret = gbdc_dev->dev_info->gb_kthread(gbdc_dev->drm_dev);
	if (ret) {
		DRM_ERROR("%s %d:GB02FUNC1177 fail! %d\n",
			__func__, __LINE__, ret);
		goto query_hw_fail;
	}
#endif
	init_rwsem(&gbdev->mclk_lock);

#ifdef GB02MAC483
	INIT_LIST_HEAD(&gbdev->vpu_bo_list_head);
	GB02FUNC1704(gbdev, GB02MAC158, PAGE_SIZE,
		GB02MAC2678, ttm_bo_type_kernel, false, GB_IP_DEC, &cmd_pool);
	GB02FUNC1704(gbdev, GB02MAC160, PAGE_SIZE,
		GB02MAC2678, ttm_bo_type_kernel, false, GB_IP_DEC, &cmd_reg);
	gb_printf(KERN_INFO,"%s-%d: dec ttm handle finish!\n", __func__, __LINE__);
	GB02FUNC22(gbdev->gbdec_data, (void *)gbdev->gb_pcie->pci_bars,
		gbdev->gb_pcie->GB02STR153, &cmd_pool, &cmd_reg,
		gbdev->ipoffset_base.vpu_offset);

	GB02FUNC1704(gbdev, GB02MAC158, PAGE_SIZE,
		GB02MAC2678, ttm_bo_type_kernel, false, GB_IP_DEC1, &cmd_pool);
	GB02FUNC1704(gbdev, GB02MAC160, PAGE_SIZE,
		GB02MAC2678, ttm_bo_type_kernel, false, GB_IP_DEC1, &cmd_reg);
	gb_printf(KERN_INFO,"%s-%d: dec1 ttm handle finish!\n", __func__, __LINE__);
	GB02FUNC26(gbdev->gbdec_data, (void *)gbdev->gb_pcie->pci_bars,
		gbdev->gb_pcie->GB02STR153, &cmd_pool, &cmd_reg,
		gbdev->ipoffset_base.vpu_offset);
#endif

#ifdef GB02MAC484
	GB02FUNC1704(gbdev, GB02MAC158, PAGE_SIZE,
		GB02MAC2678, ttm_bo_type_kernel, false, GB_IP_ENC, &cmd_pool);
	GB02FUNC1704(gbdev, GB02MAC160, PAGE_SIZE,
		GB02MAC2678, ttm_bo_type_kernel, false, GB_IP_ENC, &cmd_reg);
	gb_printf(KERN_INFO,"%s-%d: enc ttm handle finish!\n", __func__, __LINE__);
	GB02FUNC28(gbdev->gbenc_data, (void *)gbdev->gb_pcie->pci_bars,
		gbdev->gb_pcie->GB02STR153, &cmd_pool, &cmd_reg,
		gbdev->ipoffset_base.vpu_offset);
#endif

#ifdef GB02MAC481
	ret = GB02FUNC1223(gbdc_dev->drm_dev, fb_info,
					num_crtc, num_connector);
	if (ret) {
		DRM_ERROR("%s %d:GB02FUNC1223 fail! %d\n",
			__func__, __LINE__, ret);
		goto query_hw_fail;
	}
	drm_kms_helper_poll_init(drm);
#endif

	if (card_type == GENBU_FPGA)
		GB02FUNC895(gbdev);

	ret = GB02FUNC552(&gbdc_dev->pcie_info);
	if (ret) {
		DRM_ERROR("%s %d:GB02FUNC552 fail! %d\n",
			__func__, __LINE__, ret);
		goto query_hw_fail;
	}
#ifdef GB02MAC240
	ret = GB02FUNC115(gbdc_dev);
	if (ret) {
		DRM_ERROR("%s %d:gb_dbg_sysfs_init fail! %d\n",
			__func__, __LINE__, ret);
		goto query_hw_fail;
	}
#endif

	return 0;
query_hw_fail:
	drm->dev_private = NULL;
#if ((defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	!defined SYS_CENTOS7_COMPILE_ENV) \
	|| LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	drm_dev_put(drm);
#else
	drm_dev_unref(drm);
#endif
	kfree(gbdc_dev);

	return ret;
}
EXPORT_SYMBOL(GB02FUNC1185);

#if !(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009 || \
	defined SYS_CENTOS7_COMPILE_ENV) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
int gbdc_unload(struct drm_device *dev)
#else
void gbdc_unload(struct drm_device *dev)
#endif
{
	struct GB02STR39 *gb_dev = (struct GB02STR39 *)dev->dev_private;
	struct GB02STR155 *gbdc_dev = gb_dev->gbdc_dev;

	struct GB02STR160 *fb_info = &gbdc_dev->kms_info.fb_info;
	if (!drm_core_check_feature(dev, DRIVER_MODESET)) {
		dev->dev_private = NULL;
		goto out;
	}

	gbdc_dev = GB02FUNC85(dev->dev_private);
	fb_info = &gbdc_dev->kms_info.fb_info;
#ifdef GB02MAC485
	if(hpd_thread)
		kthread_stop(hpd_thread);
#endif
	GB02FUNC557();
#ifdef GB02MAC240
	GB02FUNC129(gbdc_dev);
#endif
#ifdef GB02MAC481
	GB02FUNC1227(fb_info);
	GB02FUNC1900(&gbdc_dev->kms_info, dev);
#endif

	GB02FUNC1482(&gb_dev->gb_mm);
	GB02FUNC73(gbdc_dev);
	dev->dev_private = NULL;
	drm_kms_helper_poll_fini(dev);
out:
	gb_printf(KERN_INFO, "%s finish", __func__);
#if !(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009 || \
	defined SYS_CENTOS7_COMPILE_ENV) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
	return 0;
#endif

}
EXPORT_SYMBOL(gbdc_unload);

struct GB02STR153 gb_dev_infos[] = {
	{
		.card_type = GENBU_FPGA,
#ifdef GB02MAC485
		.gb_kthread = NULL,
#endif
		.gb_shundown = NULL,
		.gb_port_disable = NULL,
		.gb_hdmi_ddc = NULL,
	},
	{
		.card_type = GENBU_01,
#ifdef GB02MAC485
		.gb_kthread = GB02FUNC1177,
#endif
		.gb_shundown = GB02FUNC1178,
		.gb_port_disable = GB02FUNC672,
		.gb_hdmi_ddc = GB02FUNC1242,
	},
	{
		.card_type = GENBU_02,
#ifdef GB02MAC485
		.gb_kthread = NULL,//GB02FUNC1177,
#endif
		.gb_shundown = NULL,//GB02FUNC1178,
		.gb_port_disable = NULL, //GB02FUNC672,
		.gb_hdmi_ddc = NULL, //GB02FUNC1242,
	},
};

struct GB02STR153 *GB02FUNC1188(int gb_type)
{
	if (gb_type > sizeof(gb_dev_infos) / sizeof(gb_dev_infos[0]))
		return NULL;

	return &gb_dev_infos[gb_type];
}

int  GB02FUNC1190(struct GB02STR70 *gpi)
{
	int board = GB02FUNC503(gpi);
	int link_num = 0;

	switch (board) {
	case PCIE_LPDDR4:
	case PCIE_C0_200:
		link_num = 2;
		break;
	case PCIE_FULL_LPDDR4:
		link_num = 5;
		break;
	case PCIE_M6FL8G_LPDDR4:
		link_num = 6;
		break;
	case PCIE_HIE1LP4_LPDDR4:
	case PCIE_M4HL8G_LPDDR4:
		link_num = 4;
		break;
	default:
		link_num = 6;
		break;
	}
	return link_num;
}
