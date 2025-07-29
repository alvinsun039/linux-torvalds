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
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/console.h>
#include <linux/of_device.h>
#ifndef CONFIG_MIPS
#include <linux/of_graph.h>
#endif
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <generated/uapi/linux/version.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <asm/atomic.h>
#include <linux/pm_runtime.h>

#include <linux/cdev.h>
#include <linux/pci.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_fb_cma_helper.h>
#endif
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#include <drm/ttm/ttm_page_alloc.h>
#endif

#ifndef CONFIG_MIPS
#include <drm/drm_of.h>
#endif
#include "common/gb_common.h"
#include "ip/gb_dp.h"
#include "common/xt.h"
#include "gb_pcie_map.h"
#include "gbdc_regs.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "gbdc_connector.h"
#include "gbdc_drv.h"
#include "gbdc_planes.h"
#include "gbdc_crtc.h"
#include "gpu/gb_device.h"


#define GBDC_ID(__group, __format) \
	((((__group) & 0x7) << 3) | ((__format) & 0x7))

#define DE_COEFTAB_DATA(a, b) ((((a) & 0xfff) << 16) | (((b) & 0xfff)))

#define	GB02MAC57	(GB02MAC498)
extern struct GB02STR199	gb_ip_ops;
extern struct GB02STR241 gb_pm_ops;
extern struct GB02STR244 gb_kms_ops;
extern struct GB02STR251  gb_cops;

struct GB02STR152 *GB02FUNC421(int gb_type);
struct GB02STR153 *GB02FUNC1188(int gb_type);

static const struct GB02STR9 {
	u16 start;
	u16 end;
} segments[GB02MAC1422] = {
		/* sector 0 */
		{
				0, 0}, {
				1, 1}, {
				2, 2}, {
				3, 3}, {
				4, 4}, {
				5, 5}, {
				6, 6}, {
				7, 7}, {
				8, 8}, {
				9, 9}, {
				10, 10}, {
				11, 11}, {
				12, 12}, {
				13, 13}, {
				14, 14}, {
				15, 15},
		/* sector 1 */
		{
				16, 19}, {
				20, 23}, {
				24, 27}, {
				28, 31},
		/* sector 2 */
		{
				32, 39}, {
				40, 47}, {
				48, 55}, {
				56, 63},
		/* sector 3 */
		{
				64, 79}, {
				80, 95}, {
				96, 111}, {
				112, 127},
		/* sector 4 */
		{
				128, 159}, {
				160, 191}, {
				192, 223}, {
				224, 255},
		/* sector 5 */
		{
				256, 319}, {
				320, 383}, {
				384, 447}, {
				448, 511},
		/* sector 6 */
		{
				512, 639}, {
				640, 767}, {
				768, 895}, {
				896, 1023}, {
				1024, 1151}, {
				1152, 1279}, {
				1280, 1407}, {
				1408, 1535}, {
				1536, 1663}, {
				1664, 1791}, {
				1792, 1919}, {
				1920, 2047}, {
				2048, 2175}, {
				2176, 2303}, {
				2304, 2431}, {
				2432, 2559}, {
				2560, 2687}, {
				2688, 2815}, {
				2816, 2943}, {
				2944, 3071}, {
				3072, 3199}, {
				3200, 3327}, {
				3328, 3455}, {
				3456, 3583}, {
				3584, 3711}, {
				3712, 3839}, {
				3840, 3967}, {
				3968, 4095},};

#define GBDC_COMMON_FORMATS \
	/*    fourcc,   layers supporting the format,      internal id   */ \
	{ DRM_FORMAT_ARGB2101010, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(0, 0) }, \
	{ DRM_FORMAT_ABGR2101010, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(0, 1) }, \
	{ DRM_FORMAT_RGBA1010102, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(0, 2) }, \
	{ DRM_FORMAT_BGRA1010102, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(0, 3) }, \
	{ DRM_FORMAT_ARGB8888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2 | DE_SMART, GBDC_ID(1, 0) }, \
	{ DRM_FORMAT_ABGR8888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2 | DE_SMART, GBDC_ID(1, 1) }, \
	{ DRM_FORMAT_RGBA8888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2 | DE_SMART, GBDC_ID(1, 2) }, \
	{ DRM_FORMAT_BGRA8888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2 | DE_SMART, GBDC_ID(1, 3) }, \
	{ DRM_FORMAT_XRGB8888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2 | DE_SMART, GBDC_ID(2, 0) }, \
	{ DRM_FORMAT_XBGR8888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2 | DE_SMART, GBDC_ID(2, 1) }, \
	{ DRM_FORMAT_RGBX8888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2 | DE_SMART, GBDC_ID(2, 2) }, \
	{ DRM_FORMAT_BGRX8888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2 | DE_SMART, GBDC_ID(2, 3) }, \
	{ DRM_FORMAT_RGB888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(3, 0) }, \
	{ DRM_FORMAT_BGR888, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(3, 1) }, \
	{ DRM_FORMAT_RGBA5551, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(4, 0) }, \
	{ DRM_FORMAT_ABGR1555, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(4, 1) }, \
	{ DRM_FORMAT_RGB565, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(4, 2) }, \
	{ DRM_FORMAT_BGR565, DE_VIDEO1 | DE_GRAPHICS1 | DE_VIDEO2, GBDC_ID(4, 3) }, \
	{ DRM_FORMAT_YUYV, DE_VIDEO1 | DE_VIDEO2, GBDC_ID(5, 2) },	\
	{ DRM_FORMAT_UYVY, DE_VIDEO1 | DE_VIDEO2, GBDC_ID(5, 3) },	\
	{ DRM_FORMAT_NV12, DE_VIDEO1 | DE_VIDEO2, GBDC_ID(5, 6) },	\
	{ DRM_FORMAT_YUV420, DE_VIDEO1 | DE_VIDEO2, GBDC_ID(5, 7) }


static struct GB02STR77 gbdcPlanes[] = {
	{DE_VIDEO1, GB02MAC657, GB02MAC659, GB02MAC591},
	{DE_GRAPHICS1, GB02MAC665, GB02MAC667, GB02MAC590},
	{DE_VIDEO2, GB02MAC661, GB02MAC663, GB02MAC591},
	{DE_SMART, GB02MAC669, GB02MAC671, GB02MAC592},
};

struct GB02STR76 gbIpRegOffset[] = {
	{GB02MAC57, GB02MAC685, GB02MAC693, GB02MAC687, GB02MAC688, GB02MAC689, GB02MAC690, GB02MAC691},
	{GB02MAC57, GB02MAC685, GB02MAC692, GB02MAC687, GB02MAC688, GB02MAC689, GB02MAC690, GB02MAC691},
	{GB02MAC57, GB02MAC1060, GB02MAC1061,
	 GB02MAC687, GB02MAC688, GB02MAC689,
	  GB02MAC690, GB02MAC691},
};
struct GB02STR75 crtc_offset = {
	0, GB02MAC674, GB02MAC678
};

struct GB02STR80 gbPlaneCap[] = {
	GBDC_COMMON_FORMATS,
};

struct GB02STR79 gbConnecterInfo[] = {
	{0, GB_DISPLAY_TYPE_HDMI},
	{1, GB_DISPLAY_TYPE_HDMI},
	{2, GB_DISPLAY_TYPE_HDMI},
	{3, GB_DISPLAY_TYPE_HDMI},
	{4, GB_DISPLAY_TYPE_HDMI},
	{5, GB_DISPLAY_TYPE_HDMI},
};

struct GB02STR82 gbKmsInfo[] = {
	{GB02MAC668, GB02MAC670, gbConnecterInfo,
		GB02MAC672, gbdcPlanes, gbPlaneCap, &gb_FPGA_max_res},
	{GB02MAC662, GB02MAC664, gbConnecterInfo,
		GB02MAC666, gbdcPlanes, gbPlaneCap, &gb_GB01_max_res},
	{GB02MAC656, GB02MAC658, gbConnecterInfo,
		GB02MAC660, gbdcPlanes, gbPlaneCap, &gb_GB02_max_res,true},
};

static struct GB02STR67 genbu_glb_asic_pcie_info[GENBU_TYPE_MAX] = {
	{GENBU_FPGA_DEV_INFO},
	{GENBU_01_DEV_INFO},
	{GENBU_02_DEV_INFO},
};

int GB02FUNC43(int card_type, struct GB02STR254 *pcie_info,
	struct GB02STR67 *pcie_res,
	struct GB02STR76 *gbip_reg_offset,
	struct GB02STR75 *ctc_offset)
{
	int i = 0;
	struct GB02STR249 *gbdcConfig;
	struct GB02STR252 *ipConfig = &pcie_info->ip_config;
	//struct GB02STR253 *vram_config = &pcie_info->vram_config;

	gb_printf(KERN_INFO, "gb base init\n");
	pcie_info->dc_config = kzalloc(pcie_info->dc_num*sizeof(struct GB02STR249), GFP_KERNEL);
	if (!pcie_info->dc_config)
		return -ENOMEM;

#if 1
	for (i = 0; i < pcie_info->dc_num; i++) {
		gbdcConfig = &pcie_info->dc_config[i];
		gbdcConfig->de_base = pcie_info->pci_mmio_bar[pcie_res->dc_reg_bar_id]
							+ gbip_reg_offset->crtc_reg_offset + i*gbip_reg_offset->crtc_offset_step + ctc_offset->crtc_de_offset;
		gbdcConfig->se_base = pcie_info->pci_mmio_bar[pcie_res->dc_reg_bar_id]
							+ gbip_reg_offset->crtc_reg_offset + i*gbip_reg_offset->crtc_offset_step + ctc_offset->crtc_se_offset;
		gbdcConfig->dc_base = pcie_info->pci_mmio_bar[pcie_res->dc_reg_bar_id]
							+ gbip_reg_offset->crtc_reg_offset + i*gbip_reg_offset->crtc_offset_step + ctc_offset->crtc_dc_offset;
		if (card_type == GENBU_02) {
			gbdcConfig->de_base += GB02MAC1572;
			gbdcConfig->se_base += GB02MAC1572;
			gbdcConfig->dc_base += GB02MAC1572;
		}
		gb_printf(KERN_INFO, "%s-%d: crtc%d, base:0x%p, de=0x%p, dc=0x%p, se=0x%p\n",
		 __func__, __LINE__, i,
		  pcie_info->pci_mmio_bar[pcie_res->dc_reg_bar_id],
		   gbdcConfig->de_base, gbdcConfig->dc_base,
		    gbdcConfig->se_base);
	}
	if (card_type == GENBU_FPGA) {
		ipConfig->fpga_pll_step = GB02MAC2846;
		ipConfig->fpga_pll_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id];
	} else if (card_type == GENBU_01) {
		ipConfig->pcie_config_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id];
		ipConfig->dpll_config_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id]
		  + gbip_reg_offset->ddr_conf_offset;
		ipConfig->pll_config_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id]
		  + gbip_reg_offset->pll_conf_offset;
		ipConfig->hdmi_config_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id]
		  + gbip_reg_offset->hdmi_reg_offset;
		ipConfig->hdmi_debug_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id]
		  + gbip_reg_offset->hdmi_dreg_offset;
#ifndef CONFIG_X86_64
	} else if (card_type == GENBU_02) {
		ipConfig->pcie_config_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id];
		ipConfig->dpll_config_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id]
		  + gbip_reg_offset->ddr_conf_offset;
		ipConfig->pll_config_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id]
		  + gbip_reg_offset->pll_conf_offset;
		ipConfig->hdmi_config_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id]
		  + gbip_reg_offset->hdmi_reg_offset;
		ipConfig->hdmi_debug_base =
		 pcie_info->pci_mmio_bar[pcie_res->ipc_bar_id]
		  + gbip_reg_offset->hdmi_dreg_offset;
#endif
	}
#endif
	pcie_info->gb_cops = &gb_cops;
	gb_printf(KERN_INFO, "gb base end\n");
	return 0;
}

void GB02FUNC59(struct GB02STR245 *kms_info, struct GB02STR82 *kms_res)
{
	kms_info->num_crtc = kms_res->crtc_num;
	kms_info->num_connector = kms_res->connector_num;
	kms_info->num_plane = kms_res->plane_num;
	kms_info->kms_ops = &gb_kms_ops;
	kms_info->connector_res = kms_res->connector_res;
	kms_info->plane_res = kms_res->plane_res;
	kms_info->plane_cap = kms_res->plane_cap;
	kms_info->num_plane_cap = sizeof(gbPlaneCap)/sizeof(gbPlaneCap[0]);
	kms_info->max_res = kms_res->max_res;
	kms_info->infinity = kms_res->infinity;

	INIT_LIST_HEAD(&kms_info->virt_crtc_head);
	INIT_LIST_HEAD(&kms_info->virt_conn_head);
	gb_printf(KERN_INFO, " --gb- num cap = %d--\n", kms_info->num_plane_cap);
}

void GB02FUNC64(struct GB02STR242 *gb_pm)
{
	gb_pm->pm_ops = &gb_pm_ops;
}

void GB02FUNC67(struct GB02STR155 *gb_dev,
		    struct drm_device *drm_dev,
		    struct pci_dev *pdev, int card_type)
{
	struct GB02STR76 *gbip_reg_offset;
	struct GB02STR67 *pcie_res;
	struct GB02STR253 *vram_config;

	gb_dev->dev = &(pdev->dev);
	gb_dev->drm_dev = drm_dev;
	gb_dev->pdev = pdev;
	gb_dev->card_type = card_type;

	vram_config = &gb_dev->pcie_info.vram_config;
	pcie_res = &genbu_glb_asic_pcie_info[card_type];
	gbip_reg_offset = &gbIpRegOffset[card_type];

	vram_config->fb_start_offset = gbip_reg_offset->vram_start_offset;
	vram_config->vram_size = GB02FUNC464();
}
void GB02FUNC73(struct GB02STR155 *gb_dev)
{
	struct GB02STR254 *pcie_info;

	if(NULL != gb_dev){
		pcie_info = &gb_dev->pcie_info;
		if(NULL != pcie_info->dc_config){
			kfree(pcie_info->dc_config);
			pcie_info->dc_config = NULL;
		}
	}
}

int GB02FUNC76(struct GB02STR155 *gb_dev)
{
	int ret = 0;
	struct GB02STR245 *kms_info;
	struct GB02STR82 *kms_res;
	struct GB02STR67 *pcie_res;
	int card_type = gb_dev->card_type;

	gb_printf(KERN_INFO, "gbdev info init\n");
	if (GB02FUNC421(card_type) == NULL) {
		gb_printf(KERN_ERR, "gb_type more than the modeset_ops struct defined!\n");
		return -ENOMEM;
	}
	kms_info = &gb_dev->kms_info;
	gb_dev->dev_info = GB02FUNC1188(card_type);
#ifdef GB02MAC481
	gb_dev->dev_info->modeset_ops = GB02FUNC421(card_type);
	gb_dev->ip_mode_ops = &gb_ip_ops;
#endif
	kms_res = &gbKmsInfo[card_type];
	pcie_res = &genbu_glb_asic_pcie_info[card_type];
	gb_dev->pcie_info.dc_num = kms_res->crtc_num;
	ret = GB02FUNC43(card_type, &gb_dev->pcie_info, pcie_res, &gbIpRegOffset[card_type], &crtc_offset);
	if(ret){
		gb_printf(KERN_ERR, "malloc dcConfig space fail\n");
		return ret;
	}
#ifdef GB02MAC240
	gb_dev->gb_dbg_info.modeinfo =
		kzalloc(kms_res->crtc_num*sizeof(struct GB02STR19), GFP_KERNEL);
	if (!gb_dev->gb_dbg_info.modeinfo)
		return -ENOMEM;
#endif
#ifdef GB02MAC481
	GB02FUNC59(kms_info, kms_res);
#endif
	GB02FUNC64(&gb_dev->pm_info);
	gb_printf(KERN_INFO, "gbdev info end\n");
	return ret;
}

struct GB02STR155 *GB02FUNC85(void *dev_private)
{
	struct GB02STR39 *gb_dev = (struct GB02STR39 *)dev_private;

	return gb_dev->gbdc_dev;
}
