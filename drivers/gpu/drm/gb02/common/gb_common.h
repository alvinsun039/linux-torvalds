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
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#ifndef		__GB_COMMON_H__
#define		__GB_COMMON_H__
#include "common/gb_uk.h"

#define	KB	(1UL<<10)
#define	MB	(1UL<<20)
#define	GB	(1UL<<30)
struct pci_dev;
struct mutex;
#define GB02MAC481
#define GB02MAC482
//#define FPGA_MODE
#define	GB02MAC483
#define	GB02MAC484
struct GB02STR70;
//#define	GB_PRINT_TMP_DEBUG
//#define	GB02MAC297

extern u64 GB02FUNC1112(void);
extern atomic_t log_en;
#define gb_printf(level, format, arg...)				\
	do {								\
		if (atomic_read(&log_en) >= simple_strtol(&level[1], NULL, 10)) \
			printk(level format, ##arg);			\
	} while(0)							\

#define	GB02MAC485
#define GB_DRM_MODE_FMT    "\"%s\": %px %d %d %d %d %d %d %d %d %d 0x%x 0x%x %d"

#define GB_DRM_MODE_ARG(m) \
	(m)->name, m, (m)->clock, \
	(m)->hdisplay, (m)->hsync_start, (m)->hsync_end, (m)->htotal, \
	(m)->vdisplay, (m)->vsync_start, (m)->vsync_end, (m)->vtotal, \
	(m)->type, (m)->flags,(m)->status

#define dump_func_begin \
	pr_info("----%s begin----\n", __func__)
#define dump_func_param(fmt,...) \
	pr_info("%s: "fmt,__func__,##__VA_ARGS__)
#define dump_func_end \
	pr_info("----%s end----\n", __func__)
#define dump_current  \
	pr_info("%s, ********thread name %s,pid %d\n",\
		__func__,current->comm,current->pid)
#define dump_caller \
	pr_info("*****dump caller:%pS->%pS->%s \n",\
		__builtin_return_address(1),__builtin_return_address(0),__func__)
#define dump_mode(mode, con) \
	if(con) \
		pr_info("mode %px:%d x %d@%d Hz\n", mode, mode->hdisplay, mode->vdisplay, drm_mode_vrefresh(mode))
#define dump_mode_more(mode, con) \
	if (con) \
		pr_info(GB_DRM_MODE_FMT, GB_DRM_MODE_ARG(mode))
#define dump_vmode_pad(vpad,vidx) \
	pr_info("vidx %d,%d x %d:wpad(%d) hpad(%d)\n",vidx,vpad[vidx].vwidth, vpad[vidx].vheight, \
		vpad[vidx].max_wpad, vpad[vidx].max_hpad)
#define dump_mode_pad(pad,idx) \
	pr_info("%d:%d x %d:top(%d),right(%d),bottom(%d),left(%d)\n",idx,pad[idx].width, pad[idx].height, \
		pad[idx].top, pad[idx].right,pad[idx].bottom,pad[idx].left)
/* def vram max size 32GB*/
#define GB02MAC486 (32*GB)
/* def vpu len large 8GB to 32GB vram*/
#define GB02MAC487 (8*GB)

/* Vram address partition domain */
/*
	VRAM_DEF:|DC & VPU|GPU|MCU|AUDIO|HDMAC|PERF|MMU |LL| ->
	VRAM_TYPE:|  TTM  |TTM|PHY|PHY  |PHY  |PHY |BIT |PHY| ->
	VRAM_LEN:|   8G   |RES|2M |18M  |0MB  |1MB |64MB|24MB|
*/
/* Reserved for VPU */
#define GB02MAC488             (0*MB)
/* Reserved for MCU */
#define GB02MAC489             (2*MB)
/* Reserved for AUDIO */
#define GB02MAC490           (18*MB)
/* Reserved for HDMAC */
#define GB02MAC491      (0*MB)
/* Reserved for V2V */
#define GB02MAC492        (1*MB)
/* Reserved for MMU */
#define GB02MAC493             (64*MB)
/* Reserved for DMA LL TABLE */
#define GB02MAC494        (24*MB)
/* Reserved for ECC */
#define GB02MAC495        (GB02FUNC1112())
/* Reserved for ALL SIZE */
#define GB02MAC496         (GB02MAC488 + GB02MAC489 + GB02MAC490 + GB02MAC491 + GB02MAC492 + GB02MAC493 + GB02MAC494 + GB02MAC495)
/* Reserved for GPU */
#define GB02MAC497             (GB02FUNC464() - GB02MAC496)

/* start addr define */
#define GB02MAC498	(0 * GB)
#define GB02MAC499	(GB02MAC498)
#define GB02MAC500	(GB02MAC498 + GB02MAC488)
#define GB02MAC501	(GB02MAC500 + GB02MAC497)
#define GB02MAC502	(GB02MAC501 + GB02MAC489)
#define GB02MAC503	(GB02MAC502 + GB02MAC490)
#define GB02MAC504	(GB02MAC503 + GB02MAC491)
#define GB02MAC505		(GB02MAC504 + GB02MAC492)
#define GB02MAC506	(GB02MAC505 + GB02MAC493)

#define GB02MAC507 6
#define GB02MAC508 6
#define GB02MAC509 16

enum gb_board_type {
	PCIE_FULL_FUNC_DDR4 = 0,
	PCIE_LPDDR4,
	PCIE_FULL_LPDDR4,
	/*M6FL8G 4HDMI+2DP*/
	PCIE_M6FL8G_LPDDR4,
	/*HIE1LP4 4HDMI*/
	PCIE_HIE1LP4_LPDDR4,
	PCIE_C0_200,
	/*M4HL8G 4HDMI*/
	PCIE_M4HL8G_LPDDR4,
	GB02MAC1363,
	BOARD_TYPE_MAX,
};

enum genbu_asic_type {
	GENBU_FPGA = 0,
	GENBU_01,
	GENBU_02,
	GENBU_TYPE_MAX,
};

/* this enumerates display type. */
enum gb_drm_output_type {
	GB_DISPLAY_TYPE_NONE,
	/* RGB or CPU Interface. */
	GB_DISPLAY_TYPE_LCD,
	/* HDMI Interface. */
	GB_DISPLAY_TYPE_HDMI,
	/* DP Interface. */
	GB_DISPLAY_TYPE_DP,
	/* Virtual Display Interface. */
	GB_DISPLAY_TYPE_LVDS,
};

struct GB02STR61 {
	unsigned long *dma_ll_mem_bitmap;
	unsigned long dma_ll_mem_bitmap_maxno;
	struct mutex lock;
};

struct GB02STR62 {
	unsigned int wr_channel_usage;
	unsigned int rd_channel_usage;
	struct GB02STR61 ll_bitmap;
	struct mutex channel_rd_lock;
	struct mutex channel_wr_lock;
	struct mutex dma_lock;
	struct GB02STR8 *dma_ctrl;
};

extern int GB02FUNC1742(struct GB02STR70 *pcie_info);
extern void GB02FUNC1745(void);
extern int GB02FUNC1856(struct GB02STR70 *pcie_info);
extern void GB02FUNC1857(struct GB02STR70 *pcie_info);
extern void GB02FUNC305(const void __iomem *mcu_peri_bar_p, const void __iomem *sys_peri_bar_p,
		const void __iomem *ddr_bar_p, const void __iomem *peri_base_bar_p);
extern int GB02FUNC1703(struct GB02STR70 *pcie_info);
extern int GB02FUNC1005(int dp_index,
	struct GB02STR70 *pcie_info, bool is_edp);

extern int GB02FUNC1016(int dp_index,
	struct GB02STR70 *pcie_info, bool is_edp);
extern int GB02FUNC1015(int dp_index,
	struct GB02STR70 *pcie_info, bool is_edp);
extern enum gb_board_type GB02FUNC503(struct GB02STR70 *gpi);
extern bool GB02FUNC507(int connector_id);
extern int GB02FUNC439(u32 *gpu_ut);
extern void GB02FUNC1883(u32 *gpu_ut_2d, unsigned long *gpu_mem_used);
void HDMI_power_reset(struct GB02STR70 *gpi, int dp_index);
#endif
