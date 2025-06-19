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
#include <linux/fb.h>
#include <linux/fs.h>
#include <video/videomode.h>
#include <generated/uapi/linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_modes.h>
#include <drm/drm_crtc.h>
#include <drm/drm_plane.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "gbdc_planes.h"
#include "gbdc_crtc.h"
#include "gbdc_connector.h"
#include "gbdc_ip.h"
#include "gb_ip.h"
#include "reg_ops.h"
#include "gbdc_regs.h"
#include "gbdc_dev.h"
#include "gbdc_ops.h"
#include "ip/gb_dp.h"
#include "gbdc_drv.h"
#include "gpu_test/gb_mem_tool.h"
#include "gbdc_device.h"

#ifdef CONFIG_GBDC_ATOMIC_FEATURE
static int g_clk_on;
#endif

enum genbu_resolution {
	MODE_640x480,
	MODE_720x480,
	MODE_800x600,
	MODE_1024x768,
	MODE_1280x720,
	MODE_1280x800,
	MODE_1280x960,
	MODE_1280x1024,
	MODE_1360x768,
	MODE_1400x1050,
	MODE_1680x1050,
	MODE_1600x1200,
	MODE_1920x1080,
	MODE_1920x1200
};

enum genbu_crtc_id {
	CRTC0 = 0,
	CRTC1,
	CRTC_MAX,
};

struct GB02STR65 dc_binfo;

#if 0
static int GB02FUNC140( void __iomem * clk_addr,
							 int mode0, int mode1)
{
	int val = 0;
	int ret = 0;

	val = 0x007d250a;
	GB02FUNC733(clk_addr, 0x200, val);
	GB02FUNC733(clk_addr, 0x208, 0x05);
	GB02FUNC733(clk_addr, 0x214, 0x05);
	GB02FUNC733(clk_addr, 0x25c, 0x3);
	GB02FUNC733(clk_addr, 0x25c, 0x1);

	return ret;
}
 static int GB02FUNC142(void __iomem * clk_addr)
 {
	 int cnt = 0;

	 while (cnt <= 10000) {
		 if (GB02FUNC730(clk_addr, 4) == 1)
			 return 0;
		 cnt++;
	 }
	 gb_printf(KERN_INFO, "#############Invalid clk in addr: %pK\n", clk_addr);

	 return -1;
 }
#endif

#ifdef GB_ALLSCREEN
static struct GB02STR247 fb_info;
int GB02FUNC144(int *crtc_id_list, int crtc_num,
	struct GB02STR246 *crtc_info)
{
	if (crtc_num > 1) {
		if (crtc_info[crtc_id_list[0]].fb_addr !=
			crtc_info[crtc_id_list[1]].fb_addr)
			return 1;
	}
	return 0;
}

int GB02FUNC148(int *crtc_id_list)
{
	int crtc_id;
	int i = 0;
	for (crtc_id = 0; crtc_id < GB02MAC557; crtc_id++) {
		if (GB02FUNC1500(crtc_id)) {
			crtc_id_list[i] = crtc_id;
			i++;
		}
	}
	return i;
}

int GB02FUNC152(void)
{
	int crtc_id;
	for (crtc_id = 0; crtc_id < GB02MAC557; crtc_id++) {
		if (GB02FUNC1500(crtc_id)) {
			break;
		}
	}
	return crtc_id;
}
static void GB02FUNC156(void)
{
	u64 fb_addr = 0, tmp_addr = 0;
	int size = 0;
	int crtc_num, crtc_id, i;
	int crtc_list[GB02MAC557];
	crtc_num = GB02FUNC148(crtc_list);
	for (i = 0; i < crtc_num; i++) {
		crtc_id = crtc_list[i];
		tmp_addr = fb_info.crtc_plane[crtc_id].fb_addr;
		size += fb_info.crtc_plane[crtc_id].size;

		if (i == 0)
			fb_addr = tmp_addr;

		if (tmp_addr <= fb_addr) {
			fb_addr = tmp_addr;
		}
	}
#ifdef GB02MAC511
	//GB02FUNC308(fb_addr, size);
#endif
}
static void GB02FUNC159(struct GB02STR77 *plane_res,
	phys_addr_t fb_addr, u64 height, u64 width, int crtc_id)
{
	fb_info.plane_res = plane_res;
	fb_info.crtc_plane[crtc_id].fb_addr = fb_addr;
	fb_info.crtc_plane[crtc_id].height = height;
	fb_info.crtc_plane[crtc_id].width = width;
	fb_info.crtc_plane[crtc_id].size = height * width * 4;
	GB02FUNC156();
}

struct GB02STR247 *GB02FUNC161(void)
{
	return &fb_info;
}

int GB02FUNC164(struct GB02STR155 *gbdc_dev, struct GB02STR247 *fb_info,
	int fb_type, int expend_flag)
{
	u64 fb_addr;
	int i;
	void __iomem *de_base;
	void __iomem *dc_base;
	int crtc_id;
	int width_tmp = 0;
	int crtc_num;
	int crtc_list[GB02MAC557];
	crtc_num = GB02FUNC148(crtc_list);
	if (0 == atomic_read(&fb_info->info[fb_type].flag))
		fb_type = 0;

	for (i = 0; i < crtc_num; i++) {
		crtc_id = crtc_list[i];
		de_base = gbdc_dev->pcie_info.dc_config[crtc_id].de_base;
		dc_base = gbdc_dev->pcie_info.dc_config[crtc_id].dc_base;

		if (0 == fb_type)
			fb_addr = fb_info->crtc_plane[crtc_id].fb_addr;
		else if (fb_type != 0 && expend_flag) {
			fb_addr = fb_info->info[fb_type].fb_addr + width_tmp * 4;
			width_tmp  = fb_info->crtc_plane[crtc_id].width;
		} else if (fb_type != 0) {
			fb_addr = fb_info->info[fb_type].fb_addr;
		}

		GB02FUNC733(de_base, fb_info->plane_res->ptr,
		                          lower_32_bits(fb_addr));
		GB02FUNC733(de_base, fb_info->plane_res->ptr + 4,
		                           upper_32_bits(fb_addr));
		GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	}
	// after switch fb, need delay some time for stagger with vtov copy
	udelay(12000);
	return 0;
}

#endif

int GB02FUNC167(struct drm_display_mode *dmode)
{
	int mode = MODE_1920x1080;
	if (dmode->hdisplay == 1920 && dmode->vdisplay == 1200) {
		mode = MODE_1920x1200;
	} else if (dmode->hdisplay == 1920 && dmode->vdisplay == 1080) {
		mode = MODE_1920x1080;
	} else if (dmode->hdisplay == 1680 && dmode->vdisplay == 1050) {
		mode = MODE_1680x1050;
	} else if (dmode->hdisplay == 1600 && dmode->vdisplay == 1200) {
		mode = MODE_1600x1200;
	} else if (dmode->hdisplay == 1400 && dmode->vdisplay == 1050) {
		mode = MODE_1400x1050;
	} else if (dmode->hdisplay == 1360 && dmode->vdisplay == 768) {
		mode = MODE_1360x768;
	} else if (dmode->hdisplay == 1280 && dmode->vdisplay == 1024) {
		mode = MODE_1280x1024;
	} else if (dmode->hdisplay == 1280 && dmode->vdisplay == 960) {
		mode = MODE_1280x960;
	} else if (dmode->hdisplay == 1280 && dmode->vdisplay == 800) {
		mode = MODE_1280x800;
	} else if (dmode->hdisplay == 1280 && dmode->vdisplay == 720) {
		mode = MODE_1280x720;
	} else if (dmode->hdisplay == 1024 && dmode->vdisplay == 768) {
		mode = MODE_1024x768;
	} else if (dmode->hdisplay == 800 && dmode->vdisplay == 600) {
		mode = MODE_800x600;
	} else if (dmode->hdisplay == 720 && dmode->vdisplay == 480) {
		mode = MODE_720x480;
	} else if (dmode->hdisplay == 640 && dmode->vdisplay == 480) {
		mode = MODE_640x480;
	} else {
		gb_printf(KERN_INFO, "In Defaulte  Display mode 1920x1080\n");
	}

	return mode;
}

#ifndef GB02MAC2593
	 static int GB02FUNC172(void __iomem * clk_addr, int mode)
	 {
		 int val0 = 0;
		 int val1 = 0;

		 gb_printf(KERN_INFO, "clk_addr %pk, mode: %d\n", clk_addr, mode);
		 if (mode == MODE_1920x1200) {
			 val0 = 0x1f42e0b;
			 val1 = 0x17704;
		 } else if (mode == MODE_1920x1080) {
			 val0 = 0x007d2508;
			 val1 = 0xfa06;
		 } else if (mode == MODE_1680x1050) {
			 val0 = 0x36b2b08;
			 val1 = 0x1f407;
		 } else if (mode == MODE_1600x1200) {
			 val0 = 0x7d0a02;
			 val1 = 0xfa06;
		 } else if (mode == MODE_1400x1050) {
			 val0 = 0x36b3c0a;
			 val1 = 0xa;
		 } else if (mode == MODE_1360x768) {
			 val0 = 0x1771504;
			 val1 = 0x1f40c;
		 } else if (mode == MODE_1280x1024 || mode == MODE_1280x960) {
			 val0 = 0xa02;
			 val1 = 0x17709;
		 } else if (mode == MODE_1280x800) {
			 val0 = 0x36b1404;
			 val1 = 0x1f40c;
		 } else if (mode == MODE_1280x720) {
			 val0 = 0x7d2508;
			 val1 = 0x1f40c;
		 } else if (mode == MODE_1024x768) {
			 val0 = 0x177320a;
			 val1 = 0x1f40f;
		 } else if (mode == MODE_800x600) {
			 val0 = 0x0501;
			 val1 = 0x19;
		 } else if (mode == MODE_720x480) {
			 val0 = 0x02bf0214;
			 val1 = 0x1;
		 } else if (mode == MODE_640x480) {
			 val0 = 0x02080214;
			 val1 = 0x1;
		 } else {
			 gb_printf(KERN_ERR, "error mode: %d\n", mode);
			 return -EINVAL;
		 }
		 GB02FUNC733(clk_addr, 0x200, val0);
		 GB02FUNC733(clk_addr, 0x208, val1);
		 GB02FUNC733(clk_addr, 0x25c, 0x3);
		 GB02FUNC733(clk_addr, 0x25c, 0x1);

		 return 0;
	 }
#endif

/*
static int GB02FUNC140(void __iomem *clk_addr, int mode0, int mode1)
{
	int val = 0;
	int ret = 0;

	val = 0x007d250a;
	GB02FUNC733(clk_addr, 0x200, val);
	GB02FUNC733(clk_addr, 0x208, 0x05);
	GB02FUNC733(clk_addr, 0x214, 0x05);
	GB02FUNC733(clk_addr, 0x25c, 0x3);
	GB02FUNC733(clk_addr, 0x25c, 0x1);

	return ret;
}
*/
static int GB02FUNC181(void __iomem *dc_base,
		struct GB02STR252 *ip_config, int mode0, int crtc_id)
{
	int cnt = 0;
	int ret = 0;
	int status;

#ifdef CONFIG_GBDC_ATOMIC_FEATURE
		if (g_clk_on) {
			gb_printf(KERN_ERR, "genbu clk already on!return\n");
			return 0;
		}
#endif
	gb_printf(KERN_INFO, "gb prepare config\n");
	GB02FUNC735(dc_base, GB02MAC694, GB02MAC680);
	GB02FUNC734(dc_base, GB02MAC694, GB02MAC680);

	while (cnt < 5000) {
		status = GB02FUNC730(dc_base, GB02MAC694);
		if ((status & GB02MAC680) == GB02MAC680)
			break;
		usleep_range(1000, 10000);
		cnt++;
	}
	if ((status & GB02MAC680) != GB02MAC680) {
		gb_printf(KERN_ERR, "*******status != DC_CONFIG_REQ\n");
		return -1;
	}
/*
#ifdef GB02MAC2593
		// 10000 is just a invalid value, no more sense.
		ret = GB02FUNC140(ip_config->fpga_pll_base, mode0, 10000);
		ret = GB02FUNC142(ip_config->fpga_pll_base);
		if (ret) {
			gb_printf(KERN_ERR, "set gb clk error!!!\n");
			return -1;
		}
#else
		// config CRTC0 clk
		ret = GB02FUNC172(ip_config->fpga_pll_base, mode0);
		ret = GB02FUNC142(ip_config->fpga_pll_base);
		if (ret) {
			gb_printf(KERN_ERR, "set gb clk0 error!!!\n");
			return -1;
		}
		// config CRTC1 clk
		ret = GB02FUNC172(ip_config->fpga_pll_base +
			ip_config->fpga_pll_step, mode0);
		ret = GB02FUNC142(ip_config->fpga_pll_base +
			ip_config->fpga_pll_step);
		if (ret) {
			gb_printf(KERN_ERR, "set gb clk1 error!!!\n");
			return -1;
		}
		//config CRTC2 clk
#endif
*/
#ifdef CONFIG_GBDC_ATOMIC_FEATURE
	g_clk_on = true;
#endif
	return ret;
}

static int GB02FUNC191(void __iomem *dc_base,
		struct GB02STR252 *ip_config, int mode0, int crtc_id)
{
	int cnt = 0;
	int status;

#ifdef CONFIG_GBDC_ATOMIC_FEATURE
		if (g_clk_on) {
			gb_printf(KERN_INFO, "genbu clk already on!return\n");
			return 0;
		}
#endif
	gb_printf(KERN_INFO, "gb prepare config\n");
	GB02FUNC735(dc_base, GB02MAC694, GB02MAC680);
	GB02FUNC734(dc_base, GB02MAC694, GB02MAC680);

	while (cnt < 5000) {
		status = GB02FUNC730(dc_base, GB02MAC694);
		if ((status & GB02MAC680) == GB02MAC680)
			break;
		usleep_range(1000, 10000);
		cnt++;
	}
	if ((status & GB02MAC680) != GB02MAC680) {
		gb_printf(KERN_ERR, "*******status != DC_CONFIG_REQ\n");
		return -1;
	}
	return 0;
}
static int GB02FUNC194(void __iomem *dc_base,
	struct GB02STR252 *ip_config, int mode0, int crtc_id)
{
	int cnt = 0;
	int status;

#ifdef CONFIG_GBDC_ATOMIC_FEATURE
		if (g_clk_on) {
			gb_printf(KERN_INFO, "genbu clk already on!return\n");
			return 0;
		}
#endif
	gb_printf(KERN_INFO, "gb prepare config\n");
	GB02FUNC735(dc_base, GB02MAC694, GB02MAC680);
	GB02FUNC734(dc_base, GB02MAC694, GB02MAC680);

	while (cnt < 5000) {
		status = GB02FUNC730(dc_base, GB02MAC694);
		if ((status & GB02MAC680) == GB02MAC680)
			break;
		usleep_range(1000, 10000);
		cnt++;
	}
	if ((status & GB02MAC680) != GB02MAC680) {
		gb_printf(KERN_ERR, "*******status != DC_CONFIG_REQ\n");
		return -1;
	}

#ifdef CONFIG_GBDC_ATOMIC_FEATURE
	g_clk_on = true;
#endif
	return 0;
}

static int GB02FUNC196(void __iomem *dc_base, void __iomem *de_base)
{
	int count = 100000;
	u32 status;

	gb_printf(KERN_INFO, "gb enter config\n");
	GB02FUNC735(dc_base, GB02MAC694, GB02MAC680);
	GB02FUNC734(dc_base, GB02MAC694, GB02MAC680);
	GB02FUNC735(dc_base, GB02MAC694, GB02MAC680);
	GB02FUNC734(dc_base, GB02MAC694, GB02MAC680);

	while (count) {
		status =
				GB02FUNC730(dc_base, GB02MAC580);
		if ((status & GB02MAC680) == GB02MAC680)
			break;
		/*
		 * entering config mode can take as long as the rendering
		 * of a full frame, hence the long sleep here
		 */
		msleep(1);
		count--;
	}

	if (count == 0) {
		gb_printf(KERN_ERR, "===%s %d ERROR into config mode!!!!===\n",
		 __func__, __LINE__);
		return -1;
	}
	WARN(count == 0, "timeout while entering config mode, status:0x%x",
	 status);
	GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	return 0;
}

static int GB02FUNC200(struct GB02STR77 *plane,
	void __iomem *dc_base, void __iomem *de_base, uint64_t csr_addr,
	int width, int height)
{
	u16 offset_ptr;

	offset_ptr = plane->stride_offset;
	GB02FUNC733(de_base, plane->ptr, lower_32_bits(csr_addr));
	GB02FUNC733(de_base, plane->ptr + 4, upper_32_bits(csr_addr));

#if 0
	GB02FUNC733(de_base, plane->base + offset_ptr, width * 4);
	GB02FUNC733(de_base, plane->base + GB02MAC727,
	 GB02MAC730(width) | GB02MAC732(height));
	GB02FUNC733(de_base, plane->base + GB02MAC735,
	 GB02MAC730(width) | GB02MAC732(height));
	GB02FUNC733(de_base, plane->base + GB02MAC741,
	 GB02MAC730(width) | GB02MAC732(height));

	val = GB02FUNC730(de_base, plane->base + GB02MAC696);
	val |= GB02MAC698;
	GB02FUNC733(de_base, plane->base + GB02MAC696, val);
	gb_printf(KERN_INFO, "---gb--%s:%d----layer control base=%lx, value =%x---\n",
	 __func__, __LINE__, plane->base, val);

	GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
#endif
	return 0;
}
static int GB02FUNC204(struct GB02STR77 *plane, void __iomem *dc_base,
	void __iomem *de_base)
{
	int val = 0;

	val = GB02FUNC730(de_base, plane->base + GB02MAC696);

	val &= ~GB02MAC698;
	GB02FUNC733(de_base, plane->base + GB02MAC696, val);
	//gb_printf(KERN_INFO, "---gb--%s:%d----layer control base=%x, value =%x---\n",
	// __func__, __LINE__, plane->base, val);

	//GB02FUNC733(de_base, plane->base + GB02MAC696, 0);
	GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	return 0;
}
static int GB02FUNC207(struct GB02STR77 *plane,
	void __iomem *dc_base, void __iomem *de_base, int x,
	int y, int width, int height, int stride)
{
	int val;
	u16 offset_ptr = plane->stride_offset;

	GB02FUNC733(de_base, plane->base + offset_ptr, stride);
	GB02FUNC733(de_base, plane->base + GB02MAC727,
	 GB02MAC730(width) | GB02MAC732(height));
	GB02FUNC733(de_base, plane->base + GB02MAC735,
	 GB02MAC730(width) | GB02MAC732(height));
	GB02FUNC733(de_base, plane->base + GB02MAC741,
	 GB02MAC730(width) | GB02MAC732(height));

	GB02FUNC733(de_base, plane->base + GB02MAC738,
	 GB02MAC730(x) | GB02MAC732(y));

	GB02FUNC733(de_base, plane->base + 0x18, 0xff);
	GB02FUNC733(de_base, plane->base + 0x24, 0x0);
	GB02FUNC733(de_base, plane->base + 0x1c, 0x1);

	/* first clear the rotation bits */
	val = GB02FUNC730(de_base, plane->base + GB02MAC696);
	val &= ~GB02MAC716;

	/*
	 * always enable pixel alpha blending until we have a way to change
	 * blend modes
	 * Test Kylin10 found we should close LAYER_COMP, or Desktop display distortions
	 */
	val &= ~GB02MAC719;
	val |= GB02MAC711;
	val &= ~GB02MAC722;
	val |= GB02MAC722;
	val &= ~GB02MAC702(GB02MAC700);
	val |= GB02MAC698;
	GB02FUNC733(de_base, plane->base + GB02MAC696, val);

	GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	return 0;
}
static int GB02FUNC216(void __iomem *de_base, struct videomode *vmode)
{
	u32	val = 0;

	gb_printf(KERN_INFO, "gb config crtc mode\n");

	val = (((GB02MAC746 >> 4) & 0xff) << 16) |
		  (((GB02MAC749 >> 4) & 0xff) << 8) |
		  ((GB02MAC752 >> 4) & 0xff);
	GB02FUNC733(de_base, GB02MAC650, val);
	val = 0x3f << 16 | 0xf << 8 | 0xf;
	GB02FUNC733(de_base, GB02MAC644, val);

	GB02FUNC733(de_base, GB02MAC651, 0x000c0c0c);

	val = GB02MAC593(vmode->hfront_porch) |
		  GB02MAC594(vmode->hback_porch);
	gb_printf(KERN_INFO, "vmode.h/vfront_porch: %d val: %x\n", vmode->hfront_porch, val);
	GB02FUNC733(de_base, GB02MAC646 + GB02MAC586, val);

	val = GB02MAC595(vmode->vfront_porch) |
		  GB02MAC596(vmode->vback_porch);
	gb_printf(KERN_INFO, "vmode.h/vback_porch: %d val: %x\n",
		   vmode->hfront_porch, val);

	GB02FUNC733(de_base, GB02MAC646 + GB02MAC587, val);

	val = GB02MAC597(vmode->hsync_len)
			| GB02MAC598(vmode->vsync_len);
	gb_printf(KERN_INFO, "DE_SYNC_WIDTH val: %x\n", val);

	if (vmode->flags & DISPLAY_FLAGS_HSYNC_HIGH)
		val |= GB02MAC647;
	if (vmode->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		val |= GB02MAC648;

	gb_printf(KERN_INFO, "DE_SYNC_WIDTH val2: %x\n", val);
	GB02FUNC733(de_base, GB02MAC646 + GB02MAC588, val);

	val = GB02MAC599(vmode->hactive) | GB02MAC600(vmode->vactive);
	GB02FUNC733(de_base, GB02MAC646 + GB02MAC589, val);
	gb_printf(KERN_INFO, "HV_ACTIVE: %x\n", val);

	if (vmode->flags & DISPLAY_FLAGS_INTERLACED)
		GB02FUNC734(de_base, GB02MAC585, GB02MAC579);
	else {
		GB02FUNC735(de_base, GB02MAC585,
		 GB02MAC579);
		val = GB02FUNC730(de_base, GB02MAC585);
		GB02FUNC733(de_base, GB02MAC585,
		 (val & 0xffffff) | 0xe4000000);


		GB02FUNC734(de_base, GB02MAC585,
				GB02MAC577);
	}
	gb_printf(KERN_INFO, "H_TIMING: %x--> value: %x\n",
		   GB02MAC646 + GB02MAC586,
		   GB02FUNC730(de_base, GB02MAC646+GB02MAC586));

	gb_printf(KERN_INFO, "V_TIMING: %x--> value: %x\n",
		   GB02MAC646 + GB02MAC587,
		   GB02FUNC730(de_base, GB02MAC646+GB02MAC587));

	gb_printf(KERN_INFO, "SYNC_WIDTh: %x--> value: %x\n",
		   GB02MAC646 + GB02MAC588,
		   GB02FUNC730(de_base, GB02MAC646+GB02MAC588));
	gb_printf(KERN_INFO, "SYNC_WIDTh: %x--> value: %x\n",
		   GB02MAC646 + GB02MAC589,
		   GB02FUNC730(de_base, GB02MAC646+GB02MAC589));
	return 0;
}
__attribute__((unused)) int gb_get_forcc_plane_num(u32 format)
{
	int fplane_num = 0;
	switch(format){
		case DRM_FORMAT_ARGB2101010:
		case DRM_FORMAT_ABGR2101010:
		case DRM_FORMAT_RGBA1010102:
		case DRM_FORMAT_BGRA1010102:
		case DRM_FORMAT_ARGB8888:
		case DRM_FORMAT_ABGR8888:
		case DRM_FORMAT_RGBA8888:
		case DRM_FORMAT_XRGB8888:
		case DRM_FORMAT_XBGR8888:
		case DRM_FORMAT_RGBX8888:
		case DRM_FORMAT_BGRX8888:

		case DRM_FORMAT_RGB888:
		case DRM_FORMAT_BGR888:
		case DRM_FORMAT_RGBA5551:
		case DRM_FORMAT_ABGR1555:
		case DRM_FORMAT_RGB565:
		case DRM_FORMAT_BGR565:
		case DRM_FORMAT_YUYV:
		case DRM_FORMAT_UYVY:
				fplane_num = 1;
				break;
		case DRM_FORMAT_NV12:
				fplane_num = 2;
				break;
		case DRM_FORMAT_YUV420:
				fplane_num = 3;
				break;
		default:
			fplane_num = 0;
	}
	return fplane_num;
}

int GB02FUNC238(void __iomem *dc_base, void __iomem *de_base,
	struct GB02STR66 *olinfo)
{
	int layer_offset = 0;
	int layer_base = 0;
	int val = 0;
	u32 stride = 0;
	u32 active = 0;
	u32	pic_format = 0;

	if (olinfo->ol_id == DE_SMART) {
		layer_offset = GB02MAC659;
		layer_base = GB02MAC657;
	} else if (olinfo->ol_id == DE_VIDEO2) {
		layer_offset = GB02MAC663;
		layer_base = GB02MAC661;
	}

	if (olinfo->attr == VOERLAY_ADDR) {
		gb_printf(KERN_INFO, "--------config overlay addr-------\n");
		GB02FUNC733(de_base, layer_offset,
		 lower_32_bits(olinfo->yaddr));
		GB02FUNC733(de_base, layer_offset + 4,
		 upper_32_bits(olinfo->yaddr));

		GB02FUNC733(de_base, layer_offset + 0x10,
		 lower_32_bits(olinfo->cbaddr));
		GB02FUNC733(de_base, layer_offset + 0x14,
		 upper_32_bits(olinfo->cbaddr));

		GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);

	} else if (olinfo->attr == OVERLAY_RESOLUTION) {
		gb_printf(KERN_INFO, "--------config overlay resolution-------\n");
		active = olinfo->h << 16 | olinfo->w;
#if 0
		for (i = 0;
		 i < sizeof(gbPlaneCap) / sizeof(struct GB02STR80);
		 i++) {
			if (olinfo.format == gbPlaneCap[i].format)
				pic_format = gbPlaneCap[i].id;
		}
		if (olinfo.format == DRM_FORMAT_NV12)
			stride = olinfo.w;
		else if (olinfo.format == DRM_FORMAT_NV12)
			stride = olinfo.w * 2;
#else
		pic_format = olinfo->format;
		if (olinfo->format == 0x2e)
			stride = olinfo->w;
		else if (olinfo->format == 0x37)
			stride = olinfo->w * 2;

#endif
		GB02FUNC733(de_base, layer_base, pic_format);
		GB02FUNC733(de_base, layer_base + 0x18, stride);
		GB02FUNC733(de_base, layer_base + 0x1c, stride);

		GB02FUNC733(de_base, layer_base + 0xc, active);
		GB02FUNC733(de_base, layer_base + 0x10, active);
		GB02FUNC733(de_base, layer_base + 0x14, 0);

		GB02FUNC733(de_base, layer_base + 0x84, 0x400);
		GB02FUNC733(de_base, layer_base + 0x88, 0x0);
		GB02FUNC733(de_base, layer_base + 0x8c, 0x59c);
		GB02FUNC733(de_base, layer_base + 0x90, 0x400);
		GB02FUNC733(de_base, layer_base + 0x94, 0x1aa1);
		GB02FUNC733(de_base, layer_base + 0x98, 0x1d30);
		GB02FUNC733(de_base, layer_base + 0x9c, 0x400);
		GB02FUNC733(de_base, layer_base + 0xa0, 0x6f8);
		GB02FUNC733(de_base, layer_base + 0xa4, 0x0);
		GB02FUNC733(de_base, layer_base + 0xa8, 0x0);
		GB02FUNC733(de_base, layer_base + 0xac, 0x0200);
		GB02FUNC733(de_base, layer_base + 0xb0, 0x0200);

				/* first clear the rotation bits */
		val = GB02FUNC730(de_base, layer_base + GB02MAC696);
		val &= ~GB02MAC716;

		val &= ~GB02MAC719;
	//	val |= GB02MAC711;
		val &= ~GB02MAC702(GB02MAC700);
		val |= GB02MAC698;
		gb_printf(KERN_INFO, "---------layer control =%x---\n", val);
		GB02FUNC733(de_base, layer_base + GB02MAC696, val);
	}

	return 0;
}


int GB02FUNC258(struct drm_crtc *dcrtc, struct drm_plane *drm_planes,
		struct GB02STR77 *plane, struct drm_framebuffer *fb,
		void __iomem *de_base, int plane_id)
{
	int crtc_id = 0, i;
	u8 format_id = 0;
	u16 ptr;
	u16 offset_ptr;
	dma_addr_t fb_addr;
	dma_addr_t fb_addr_p2;
	dma_addr_t fb_addr_p3;
	u32 p1_len;
	u32 p2_len;
	u32 fb_stride;
	int disx = 0, disy = 0;
	u32 src_w, src_h, dest_w, dest_h, val;
	struct gbdc_crtc *gb_crtc = NULL;
	struct gbdc_plane *gb_plane =  NULL;
	struct drm_framebuffer *cur_cppl_fb = NULL;
	struct drm_plane_state *state = drm_planes->state;
	struct drm_rect *src = &state->src;
	struct drm_rect *dest = &state->dst;

	gb_crtc = GB02FUNC1573(dcrtc);
	gb_plane = &gb_crtc->cplanes[plane_id];
	if (!gb_crtc || !gb_plane || !fb)
		return -1;
	cur_cppl_fb = fb;
	crtc_id = gb_crtc->crtc_id;

	ptr = plane->ptr;
	offset_ptr = plane->stride_offset;
	fb_addr = gb_plane->cur_fb_offset;
	for (i = 0; i < gb_crtc->num_formats; i++) {
		u32 format = gb_crtc->pixel_formats[i].format;
#if !(defined SYS_CENTOS7_COMPILE_ENV || defined SYS_CENTOS7_9_2009) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
		if (format == cur_cppl_fb->pixel_format) {
#else
		if (format == cur_cppl_fb->format->format) {
#endif
			format_id = gb_crtc->pixel_formats[i].id;
			break;
		}
	}
	src_w = drm_rect_width(src) >> 16;
	src_h = drm_rect_height(src) >> 16;
	dest_w = drm_rect_width(dest);
	dest_h = drm_rect_height(dest);
	disx = dest->x1;
	disy = dest->y1;
	GB02FUNC733(de_base, plane->base, format_id);

	GB02FUNC733(de_base, ptr, lower_32_bits(fb_addr));
	GB02FUNC733(de_base, ptr + 0x4, upper_32_bits(fb_addr));
	if ((format_id == 0x2e) || (format_id == 0x29) || (format_id == 0x7)) {
		fb_stride = src_w;
		fb_addr_p2 = fb_addr + (fb_stride * src_h);
		GB02FUNC733(de_base, ptr + 0x10, lower_32_bits(fb_addr_p2));
		GB02FUNC733(de_base, ptr + 0x14, upper_32_bits(fb_addr_p2));
	} else if (format_id == 0x2f) {
		fb_stride = src_w;
		p1_len = fb_stride * src_h;
		fb_addr_p2 = fb_addr + p1_len;
		p2_len = (fb_stride / 2) * src_h;
		fb_addr_p3 = fb_addr + p1_len + p2_len;
		GB02FUNC733(de_base, ptr + 0x10, lower_32_bits(fb_addr_p2));
		GB02FUNC733(de_base, ptr + 0x14, upper_32_bits(fb_addr_p2));
		GB02FUNC733(de_base, ptr + 0x20, lower_32_bits(fb_addr_p3));
		GB02FUNC733(de_base, ptr + 0x24, upper_32_bits(fb_addr_p3));
	}

	GB02FUNC733(de_base, plane->base + 0xb4, 0x0);
	GB02FUNC733(de_base, plane->base + 0x8, 0x3fff3fff);
	GB02FUNC733(de_base, plane->base + offset_ptr, fb->pitches[0]);

	if ((format_id == 0x2e) || (format_id == 0x29) || (format_id == 0x37))
		GB02FUNC733(de_base, plane->base + offset_ptr + 0x4,
			fb->pitches[0]);
	else if (format_id == 0x2f)
		GB02FUNC733(de_base, plane->base + offset_ptr + 0x4,
			fb->pitches[0] / 2);

	GB02FUNC733(de_base, plane->base + GB02MAC727,
		GB02MAC730(src_w) | GB02MAC732(src_h));
	GB02FUNC733(de_base, plane->base + GB02MAC735,
		GB02MAC730(dest_w) | GB02MAC732(dest_h));
	GB02FUNC733(de_base, plane->base + GB02MAC738,
		GB02MAC730(disx) | GB02MAC732(disy));
	if ((format_id == 0x2e) || (format_id == 0x28) || (format_id == 0x29) ||
	(format_id == 0x30) || (format_id == 0x2f) || (format_id == 0x2b) ||
	(format_id == 0x2a) || (format_id == 0x37)) {
		GB02FUNC733(de_base, plane->base + 0x84, 0x400);
		GB02FUNC733(de_base, plane->base + 0x88, 0x0);
		GB02FUNC733(de_base, plane->base + 0x8c, 0x59c);
		GB02FUNC733(de_base, plane->base + 0x90, 0x400);
		GB02FUNC733(de_base, plane->base + 0x94, 0x1aa1);
		GB02FUNC733(de_base, plane->base + 0x98, 0x1d30);
		GB02FUNC733(de_base, plane->base + 0x9c, 0x400);
		GB02FUNC733(de_base, plane->base + 0xa0, 0x6f8);
		GB02FUNC733(de_base, plane->base + 0xa4, 0x0);
		GB02FUNC733(de_base, plane->base + 0xa8, 0x0);
		GB02FUNC733(de_base, plane->base + 0xac, 0x0200);
		GB02FUNC733(de_base, plane->base + 0xb0, 0x0200);
	}
	val = GB02FUNC730(de_base, plane->base + GB02MAC696);
	val &= ~GB02MAC716;
	val &= ~GB02MAC719;
	val |= GB02MAC711;
	val &= ~GB02MAC702(GB02MAC700);
	val |= GB02MAC698;
	GB02FUNC733(de_base, plane->base + GB02MAC696, val);

	return 0;
}

int GB02FUNC278(u16 base, void __iomem *dc_base, void __iomem *de_base)
{
	int val = 0;

	val = GB02FUNC730(de_base, base + GB02MAC696);

	val &= ~GB02MAC698;
	GB02FUNC733(de_base, base + GB02MAC696, val);
	gb_printf(KERN_INFO, "---gb--%s:%d----layer control base=%x, value =%x---\n",
	 __func__, __LINE__, base, val);

	//GB02FUNC733(de_base, base + GB02MAC696, 0);
	GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);
	return 0;
}

int GB02FUNC281(void __iomem *dc_base, void __iomem *de_base)
{

	dc_binfo.dc_base = dc_base;
	dc_binfo.de_base = de_base;
	gb_printf(KERN_INFO, "---set base --dc addr=%p, de addr = %p--%p, %p-\n",
	 dc_base, de_base, dc_binfo.dc_base, dc_binfo.de_base);
	return 0;
}

int GB02FUNC284(void __iomem **dc_base, void __iomem **de_base)
{

	*dc_base = dc_binfo.dc_base;
	*de_base = dc_binfo.de_base;
	gb_printf(KERN_INFO, "---get base --dc addr=%p, de addr = %p--%p, %p-\n",
	 *dc_base, *de_base, dc_binfo.dc_base, dc_binfo.de_base);
//	udelay(100000);
	return 0;
}

int GB02FUNC289(struct drm_crtc *dcrtc, struct GB02STR77 *plane, struct drm_plane_state *state, void __iomem *de_base)
{
	int crtc_id = 0, i;
	u8 format_id = 0;
	u16 ptr;
	u16 offset_ptr;

	dma_addr_t fb_addr;
	int disx = 0, disy = 0;
	u32 src_w, src_h, dest_w, dest_h, val;
	struct gbdc_crtc *gb_crtc = NULL;
	struct gbdc_plane *gb_plane = NULL;
	struct drm_framebuffer *cur_cppl_fb = NULL;
	struct drm_framebuffer *fb = state->fb;
	struct drm_rect *src = &state->src;
	struct drm_rect *dest = &state->dst;

	gb_crtc = GB02FUNC1573(dcrtc);
	if (plane->id != DE_GRAPHICS1)
		return -1;

	gb_plane = &gb_crtc->cplanes[DC_PLANE_GRAPHIC];

	if (!gb_crtc || !gb_plane || !fb)
		return -1;

	cur_cppl_fb = fb;
	crtc_id = gb_crtc->crtc_id;

	ptr = plane->ptr;
	offset_ptr = plane->stride_offset;
	fb_addr = gb_plane->cur_fb_offset;
	//reserved,used for expending other mode
	for (i = 0; i < gb_crtc->num_formats; i++) {
		u32 format = gb_crtc->pixel_formats[i].format;
#if !(defined SYS_CENTOS7_COMPILE_ENV || defined SYS_CENTOS7_9_2009) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
		if (format == cur_cppl_fb->pixel_format) {
#else
		if (format == cur_cppl_fb->format->format) {
#endif
			format_id = gb_crtc->pixel_formats[i].id;
			//gb_plane->fplane_num = gb_get_forcc_plane_num(format);
			break;
		}
	}
	/* because dc only support ARGB, drm send XRGB mode,
	 * so need to config fixed ARGB mode
	 */
	format_id = gb_crtc->pixel_formats[4].id;

	src_w = drm_rect_width(src) >> 16;
	src_h = drm_rect_height(src) >> 16;
	dest_w = drm_rect_width(dest);
	dest_h = drm_rect_height(dest);
	disx = dest->x1;
	disy = dest->y1;
		//printk("crtc->mode.htotal=%d\n", gb_crtc->base.mode.htotal);
		//printk("crtc->mode.hsync_start=%d\n", gb_crtc->base.mode.hsync_start);
		//printk("crtc->mode.vtotal=%d\n", gb_crtc->base.mode.vtotal);
		//printk("crtc->mode.vsync_start=%d\n", gb_crtc->base.mode.vsync_start);
		//printk("src->x1=%d\n", src->x1);
		//printk("src->y1=%d\n", src->y1);
		//printk("dest->x1=%d\n", dest->x1);
		//printk("dest->y1=%d\n", dest->y1);
		//printk("src_w=%d\n", src_w);
		//printk("src_h=%d\n", src_h);
		//printk("dest_w=%d\n", dest_w);
		//printk("dest_h=%d\n", dest_h);

	GB02FUNC733(de_base, plane->base, format_id);

	gb_printf(KERN_DEBUG, "######################set fb_addr = 0x%llx\n", fb_addr);
	GB02FUNC733(de_base, ptr, lower_32_bits(fb_addr));
	GB02FUNC733(de_base, ptr + 4, upper_32_bits(fb_addr));

	GB02FUNC733(de_base, plane->base + offset_ptr, fb->pitches[0]);

	GB02FUNC733(de_base, plane->base + GB02MAC727,
	 GB02MAC730(src_w) | GB02MAC732(src_h));
	GB02FUNC733(de_base, plane->base + GB02MAC735,
	 GB02MAC730(dest_w) | GB02MAC732(dest_h));
	GB02FUNC733(de_base, plane->base + GB02MAC738,
	 GB02MAC730(disx) | GB02MAC732(disy));

	/* first clear the rotation bits */
	val = GB02FUNC730(de_base, plane->base + GB02MAC696);
	val &= ~GB02MAC716;

	/*
	 * always enable pixel alpha blending until we have a way to change
	 * blend modes
	 * Test Kylin10 found we should close LAYER_COMP, or Desktop display distortion
	 */
	val &= ~GB02MAC719;
	val |= GB02MAC711;
	val &= ~GB02MAC722;
	val |= GB02MAC722;
	val &= ~GB02MAC702(GB02MAC700);
	val |= GB02MAC698;
	GB02FUNC733(de_base, plane->base + GB02MAC696, val);
	return 0;
}

int GB02FUNC298(struct drm_crtc *dcrtc,
	struct GB02STR77 *plane, struct drm_plane_state *state)
{
	int crtc_id = 0, i;
	u8 format_id = 0;
	u16 ptr;
	u16 offset_ptr;

	dma_addr_t fb_addr;
	dma_addr_t fb_addr_virt;
	void __iomem *de_base;
	int disx = 0, disy = 0;
	u32 src_w, src_h, dest_w, dest_h, val;
	u32 x, y, w, h;
	u32 w_total, h_total;
	struct drm_crtc *crtc = NULL;
	struct gbdc_crtc *gb_crtc = NULL;
	struct gbdc_plane *gb_plane = NULL;
	struct gbdc_plane *gbdc_plane = NULL;
	struct drm_framebuffer *cur_cppl_fb = NULL;
	struct GB02STR155 *gb_dev;
	struct GB02STR249 *dc_config;
	struct drm_framebuffer *fb = state->fb;
	struct drm_rect *src = &state->src;
	struct drm_rect *dest = &state->dst;
	struct gbdc_plane_state *gbdc_plane_state = to_gbdc_plane_state(state);
	int row, col;

	gb_crtc = GB02FUNC1573(dcrtc);
	if (plane->id != DE_GRAPHICS1)
		return -1;

	gb_plane = &gb_crtc->cplanes[DC_PLANE_GRAPHIC];

	if (!gb_crtc || !gb_plane || !fb)
		return -1;

	cur_cppl_fb = fb;

	ptr = plane->ptr;
	offset_ptr = plane->stride_offset;
	fb_addr = gb_plane->cur_fb_offset;

	for (i = 0; i < gb_crtc->num_formats; i++) {
		u32 format = gb_crtc->pixel_formats[i].format;
#if !(defined SYS_CENTOS7_COMPILE_ENV || defined SYS_CENTOS7_9_2009) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
		if (format == cur_cppl_fb->pixel_format) {
#else
		if (format == cur_cppl_fb->format->format) {
#endif
			format_id = gb_crtc->pixel_formats[i].id;
			break;
		}
	}

	format_id = gb_crtc->pixel_formats[4].id;

	row = gb_crtc->infinity_row;
	col = gb_crtc->infinity_col;
	if (row == 0 || col == 0) {
		gb_printf(KERN_ERR, "%s erro value row = %d cow = %d\n", __func__, row, col);
		return -1;
	}

	if (GB02FUNC1849(row, col, state) != 0) {
		gb_printf(KERN_ERR, "%s save_screnn_border erro", __func__);
		return -1;
	}

	gbdc_for_each_gbdc_obj(plane) {
		if (!gbdc_plane)
			continue;

		crtc = dcrtc;
		crtc_id = gbdc_plane->crtc_id;
		gb_dev = GB02FUNC85(crtc->dev->dev_private);
		dc_config = &gb_dev->pcie_info.dc_config[crtc_id];
		de_base = dc_config->de_base;

		x = gbdc_plane->order.x;
		y = gbdc_plane->order.y;
		w = gbdc_plane->geometry.w;
		h = gbdc_plane->geometry.h;
		w_total = gbdc_plane_state->gb_border[x * col + y].w_total;
		h_total = gbdc_plane_state->gb_border[x * col + y].h_total;
		fb_addr_virt = fb_addr + fb->pitches[0] * h_total + w_total * 4;

		gb_printf(KERN_DEBUG, "w_total =  %d, h_total = %d\n", w_total, h_total);
		gb_printf(KERN_DEBUG, "x = %d, y = %d w = %d h= %d\n", x, y, w, h);
		gb_printf(KERN_DEBUG, "fb_addr = 0x%llx\n", fb_addr);
		gb_printf(KERN_DEBUG, "crtc_id = %d\n", crtc_id);

		src = &gbdc_plane->base.state->src;
		dest = &gbdc_plane->base.state->dst;
#if 0
		src_w = drm_rect_width(src) >> 16;
		src_h = drm_rect_height(src) >> 16;
		dest_w = drm_rect_width(dest);
		dest_h = drm_rect_height(dest);
		disx = dest->x1;
		disy = dest->y1;
#endif
		src_w = w;
		src_h = h;
		dest_w = w;
		dest_h = h;
		disx = 0;
		disy = 0;
		gb_printf(KERN_DEBUG, "src_w = %d, src_h = %d\n", src_w, src_h);
		gb_printf(KERN_DEBUG, "dest_w = %d, dest_h = %d\n", dest_w, dest_h);
		gb_printf(KERN_DEBUG, "disx = %d, disy = %d\n", disx, disy);


		GB02FUNC733(de_base, plane->base, format_id);

		gb_printf(KERN_DEBUG, "######################set fb_addr_virt = 0x%llx\n", fb_addr_virt);
		GB02FUNC733(de_base, ptr, lower_32_bits(fb_addr_virt));
		GB02FUNC733(de_base, ptr + 4, upper_32_bits(fb_addr_virt));

		GB02FUNC733(de_base, plane->base + offset_ptr, fb->pitches[0]);

		GB02FUNC733(de_base, plane->base + GB02MAC727,
		 GB02MAC730(src_w) | GB02MAC732(src_h));
		GB02FUNC733(de_base, plane->base + GB02MAC735,
		 GB02MAC730(dest_w) | GB02MAC732(dest_h));
		GB02FUNC733(de_base, plane->base + GB02MAC738,
		 GB02MAC730(disx) | GB02MAC732(disy));

		/* first clear the rotation bits */
		val = GB02FUNC730(de_base, plane->base + GB02MAC696);
		val &= ~GB02MAC716;
		val &= ~GB02MAC719;
		val |= GB02MAC711;
		val &= ~GB02MAC722;
		val |= GB02MAC722;
		val &= ~GB02MAC702(GB02MAC700);
		val |= GB02MAC698;
		GB02FUNC733(de_base, plane->base + GB02MAC696, val);

	}

	return 0;
}



int GB02FUNC327(struct drm_crtc *dcrtc, struct GB02STR77 *plane,
	void __iomem *de_base)
{
	int crtc_id = 0, i;
	u8 format_id = 0;
	u16 ptr;
	u16 offset_ptr;

	dma_addr_t fb_addr;
	int disx = 0, disy = 0;
	u32 src_w, src_h, dest_w, dest_h, val;
	struct gbdc_crtc *gb_crtc = NULL;
	struct gbdc_plane *gb_plane = NULL;
	struct drm_framebuffer *cur_cppl_fb = NULL;

	gb_crtc = GB02FUNC1573(dcrtc);
	if (plane->id == DE_GRAPHICS1)
		gb_plane = &gb_crtc->cplanes[DC_PLANE_GRAPHIC];
	else if (plane->id == DE_SMART)
		gb_plane = &gb_crtc->cplanes[DC_PLANE_VIDEO_1];

	//cur_cppl_fb = dcrtc->primary->fb;
	cur_cppl_fb = dcrtc->primary->state->fb;
	if(!cur_cppl_fb)
		{WARN_ON(1); return -1;}

	crtc_id = gb_crtc->crtc_id;

	ptr = plane->ptr;
	offset_ptr = plane->stride_offset;
	fb_addr = gb_plane->cur_fb_offset;
//reserved,used for expending other mode
	for (i = 0; i < gb_crtc->num_formats; i++) {
		u32 format = gb_crtc->pixel_formats[i].format;
#if !(defined SYS_CENTOS7_COMPILE_ENV || defined SYS_CENTOS7_9_2009) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
		if (format == cur_cppl_fb->pixel_format) {
#else
		if (format == cur_cppl_fb->format->format) {
#endif
			format_id = gb_crtc->pixel_formats[i].id;
			//gb_plane->fplane_num = gb_get_forcc_plane_num(format);
			break;
		}
	}
	/* because dc only support ARGB, drm send XRGB mode,
	 * so need to config fixed ARGB mode
	 */
	format_id = gb_crtc->pixel_formats[4].id;
	src_w = dest_w = gb_crtc->base.mode.hdisplay;
	src_h = dest_h = gb_crtc->base.mode.vdisplay;

	GB02FUNC733(de_base, plane->base, format_id);

	if (plane->id == DE_GRAPHICS1) {
		GB02FUNC733(de_base, ptr, lower_32_bits(fb_addr));
		GB02FUNC733(de_base, ptr + 4, upper_32_bits(fb_addr));
#ifdef GB_ALLSCREEN
		GB02FUNC159(plane, fb_addr, src_h ,src_w, crtc_id);
#endif
	} else if (plane->id == DE_SMART) {
		GB02FUNC733(de_base, ptr,
		 lower_32_bits(gb_crtc->cursor_addr));
		GB02FUNC733(de_base, ptr + 4,
		 upper_32_bits(gb_crtc->cursor_addr));
	}
	//gb_printf(KERN_INFO, "read:%x value:%x\n", ptr, GB02FUNC730(de_base, ptr));
	//gb_printf(KERN_INFO, "read:%x value:%x\n", ptr + 4, GB02FUNC730(de_base, ptr + 4));
	if (plane->id == DE_GRAPHICS1)
		GB02FUNC733(de_base, plane->base + offset_ptr,
		 dcrtc->primary->fb->pitches[0]);
	else if (plane->id == DE_SMART) {
		//gb_printf(KERN_INFO, "--gb config base cursor width = %x--\n",
		// gb_crtc->cursor_width);
		GB02FUNC733(de_base, plane->base + offset_ptr,
		 gb_crtc->cursor_width * 4);
	}

	if (plane->id == DE_GRAPHICS1) {
		GB02FUNC733(de_base, plane->base + GB02MAC727,
		 GB02MAC730(src_w) | GB02MAC732(src_h));
		GB02FUNC733(de_base, plane->base + GB02MAC735,
		 GB02MAC730(dest_w) | GB02MAC732(dest_h));
		GB02FUNC733(de_base, plane->base + GB02MAC738,
		 GB02MAC730(disx) | GB02MAC732(disy));
	} else if (plane->id == DE_SMART) {
		//gb_printf(KERN_INFO, "--gb config base cursor size--\n");
		GB02FUNC733(de_base, plane->base + GB02MAC727,
		 GB02MAC730(gb_crtc->cursor_width) |
		  GB02MAC732(gb_crtc->cursor_height));
		GB02FUNC733(de_base, plane->base + GB02MAC735,
		 GB02MAC730(gb_crtc->cursor_width) |
		  GB02MAC732(gb_crtc->cursor_height));
		GB02FUNC733(de_base, plane->base + GB02MAC738,
		 GB02MAC730(gb_crtc->cursor_x) |
		  GB02MAC732(gb_crtc->cursor_y));

	}
#if 1
	if (plane->id == DE_SMART) {
		GB02FUNC733(de_base, plane->base + GB02MAC741,
		 GB02MAC730(gb_crtc->cursor_width) |
		  GB02MAC732(gb_crtc->cursor_height));
		GB02FUNC733(de_base, plane->base + 0x18, 0xff);
		GB02FUNC733(de_base, plane->base + 0x24, 0x0);
		GB02FUNC733(de_base, plane->base + 0x1c, 0x1);
	}
#endif
	/* first clear the rotation bits */
	val = GB02FUNC730(de_base, plane->base + GB02MAC696);
	val &= ~GB02MAC716;

	/*
	 * always enable pixel alpha blending until we have a way to change
	 * blend modes
	 * Test Kylin10 found we should close LAYER_COMP, or Desktop display distortion
	 */
	val &= ~GB02MAC719;
	val |= GB02MAC711;
	val &= ~GB02MAC702(GB02MAC700);
	val |= GB02MAC698;
	GB02FUNC733(de_base, plane->base + GB02MAC696, val);
	//gb_printf(KERN_INFO, "%s-%d: gb----layer control base=%x, value =%x---\n",
	 //__func__, __LINE__, plane->base, val);
	return 0;
}

static const struct GB02STR64 {
	u16 start;
	u16 end;
} segments[GB02MAC548] = {
	{    0,    0 }, {    1,    1 }, {    2,    2 }, {    3,    3 },
	{    4,    4 }, {    5,    5 }, {    6,    6 }, {    7,    7 },
	{    8,    8 }, {    9,    9 }, {   10,   10 }, {   11,   11 },
	{   12,   12 }, {   13,   13 }, {   14,   14 }, {   15,   15 },
	{   16,   19 }, {   20,   23 }, {   24,   27 }, {   28,   31 },
	{   32,   39 }, {   40,   47 }, {   48,   55 }, {   56,   63 },
	{   64,   79 }, {   80,   95 }, {   96,  111 }, {  112,  127 },
	{  128,  159 }, {  160,  191 }, {  192,  223 }, {  224,  255 },
	{  256,  319 }, {  320,  383 }, {  384,  447 }, {  448,  511 },
	{  512,  639 }, {  640,  767 }, {  768,  895 }, {  896, 1023 },
	{ 1024, 1151 }, { 1152, 1279 }, { 1280, 1407 }, { 1408, 1535 },
	{ 1536, 1663 }, { 1664, 1791 }, { 1792, 1919 }, { 1920, 2047 },
	{ 2048, 2175 }, { 2176, 2303 }, { 2304, 2431 }, { 2432, 2559 },
	{ 2560, 2687 }, { 2688, 2815 }, { 2816, 2943 }, { 2944, 3071 },
	{ 3072, 3199 }, { 3200, 3327 }, { 3328, 3455 }, { 3456, 3583 },
	{ 3584, 3711 }, { 3712, 3839 }, { 3840, 3967 }, { 3968, 4095 },
};
#define DE_COEFTAB_DATA(a, b) ((((a) & 0xfff) << 16) | (((b) & 0xfff)))

int GB02FUNC349(void __iomem *dc_base,
		 void __iomem *de_base, struct drm_color_lut *lut)
{
	int i = 0;
	int data = 0;
	int value = 0x110000;
	
	for (i = 0; i < GB02MAC548; ++i) {
		u32 a, b, delta_in, out_start, out_end;

		GB02FUNC733(de_base, GB02MAC654, value | i);
		delta_in = segments[i].end - segments[i].start;

		out_start =
		 drm_color_lut_extract(lut[segments[i].start >> 4].blue, 12);
		out_end = drm_color_lut_extract(lut[segments[i].end >> 4].blue, 12);
		a =
		 (delta_in == 0) ? 0 : ((out_end - out_start) * 256) / delta_in;
		b = out_start;
		data = DE_COEFTAB_DATA(a, b);
		GB02FUNC733(de_base, GB02MAC655, data);
	}
	GB02FUNC734(de_base, GB02MAC585,
				GB02MAC577);
	GB02FUNC734(dc_base, GB02MAC695, GB02MAC576);

	return 0;
}

static int GB02FUNC355(void __iomem *hdmi_base, int conn_id)
{
	return 0;
}
static int GB02FUNC356(void __iomem *hdmi_base, int conn_id,
				int conn_cmd)
{
	int status = 0;
	return status;
}
static int GB02FUNC358(void __iomem *hdmi_base, int conn_id,
				int conn_cmd)
{
	int ret = 0;
	if (conn_cmd == GB02MAC1091)
		ret = GB02FUNC873(conn_id);
	else if (conn_cmd == GB02MAC1090)
		ret = GB02FUNC879(conn_id);

	return ret;
}
static int GB02FUNC361(void __iomem *hdmi_base, int conn_id,
				int conn_cmd)
{
	int status = 0;
	if (conn_id == CONNECTOR0) {
		if (conn_cmd == GB02MAC1091)
			GB02FUNC631(hdmi_base);
		else
			GB02FUNC635(hdmi_base);
	} else
		status = GB02FUNC669(hdmi_base, conn_cmd);

	return status;
}

static int GB02FUNC365(struct GB02STR252 *ip_config,
	const struct drm_display_mode *ptr, int crtc_id)
{
	return 0;
}
static int GB02FUNC367(struct GB02STR252 *ip_config,
	const struct drm_display_mode *ptr, int crtc_id)
{
	return GB02FUNC982(ptr, crtc_id);
}

static int GB02FUNC369(struct GB02STR252 *ip_config,
	const struct drm_display_mode *ptr, int crtc_id)
{
	int ret = 0;

	ret = GB02FUNC689(ip_config->pll_config_base, ptr, crtc_id);
	if (ret != 0) {
		gb_printf(KERN_ERR, "GB02FUNC689 error!\n");
		return ret;
	}
	if (crtc_id == CRTC0)
		ret = GB02FUNC658(ip_config->hdmi_config_base, ptr);

	return ret;
}

static int GB02FUNC374(void __iomem *dc_base, void __iomem *de_base)
{
	int count = 100;
	int status;

	gb_printf(KERN_INFO, "%s \n", __func__);
	gb_printf(KERN_INFO, "gb leave config\n");
	GB02FUNC735(dc_base, GB02MAC695, GB02MAC576);
	GB02FUNC735(dc_base, GB02MAC694, GB02MAC680);

	while (count) {
		status = GB02FUNC730(dc_base, GB02MAC580);
		if ((status & GB02MAC680) == 0)
			break;
		usleep_range(100, 1000);
		count--;
	}
	WARN(count == 0, "timeout while leaving config mode");
	return 0;
}

int GB02FUNC377(void __iomem *ddc_base)
{
#if 0
	void __iomem *dc_base	= NULL;
	struct GB02STR155 *gb_dev
					= (struct GB02STR155 *)drm_dev->dev_private;
	struct GB02STR249 *dc_config = gb_dev->pcie_info.dc_config[crtc_id];

	dc_base= dc_config->dc_base;

	u32 conf = GB02FUNC730(dc_base, GB02MAC684);
	u8 ln_size = (conf >> 4) & 0x3, rsize;
	ctx->min_line_size = 2;

	switch (ln_size) {
	case 0:
		ctx->max_line_size = SZ_2K;
		/* two banks of 64KB for rotation memory */
		rsize = 64;
		break;
	case 1:
		ctx->max_line_size = SZ_4K;
		/* two banks of 128KB for rotation memory */
		rsize = 128;
		break;
	case 2:
		ctx->max_line_size = 1280;
		/* two banks of 40KB for rotation memory */
		rsize = 40;
		break;
	case 3:
		/* reserved value */
		ctx->max_line_size = 0;
		return -EINVAL;
	}

	ctx->rotation_memory[0] = ctx->rotation_memory[1] = rsize * SZ_1K;
#endif
	return 0;
}
static char *GB02FUNC384(struct file *fp, char *buf, int buf_len)
{
	int ret;
	int i = 0;
	int j = 0;
	char *str = buf;

#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
	ret = kernel_read(fp, buf, buf_len, &(fp->f_pos));
#else
	ret = kernel_read(fp, fp->f_pos, buf, buf_len);
#endif
	if (ret <= 0)
		return NULL;

	for (i = 0; buf[i] != '\0'; i++) {
		if (buf[i] != ' ')
			str[j++] = buf[i];
	}
	str[j] = '\0';
	buf = str;

	i = 0;

	while (buf[i++] != '\n' && i < ret);

	if (i < ret)
		fp->f_pos += i - ret;

	if (i < buf_len)
		buf[i] = 0;
	return buf;
}
static int strkv(char *src, char *key, char *val, size_t len)
{
	char *p, *q, *s;

	s = strstr(src, key);
	if (s == NULL)
		return 0;

	p = strchr(s, '=');
	q = strchr(s, '\n');
	if (p != NULL && q != NULL) {
		*q = '\0';
		//strncpy(key, src, p - src);
		strncpy(val, p + 1, len);
		return 1;
	}
	return 0;
}
int GB02FUNC394(char *key, char *val, size_t len)
{
	int ret = -1;
	struct file *fp;
	char buf[1000] = {0};

	fp = filp_open("/etc/sietium.conf", O_RDONLY, 0644);
	if (IS_ERR(fp))
		return -ENOENT;

	while (GB02FUNC384(fp, buf, sizeof(buf))) {
		if (buf[0] == '#' || buf[0] == '\n')
			continue;
		//gb_printf(KERN_INFO, "read_conf:%s\n", buf);
		if (strkv(buf, key, val, len)) {
			ret = 0;
			break;
		}
	}

	filp_close(fp, NULL);

	return ret;
}
static int GB02FUNC396(int id)
{
	char cmd[64] = {0};
	char val[64] = {0};
	int conn_stat = -1;

	sprintf(cmd, "connector%d", id);
	if (GB02FUNC394(cmd, val, sizeof(val)) == 0) {
		if (strncmp(val, "1", strlen(val)) == 0)
			conn_stat = true;
		else
			conn_stat = false;
	}

	return conn_stat;
}
static bool GB02FUNC401(struct GB02STR252 *ip_config,
					int conn_id)
{
	int phy_status = 0;
	int conn_stat;


	if (CONNECTOR0 == conn_id) {
		phy_status = GB02FUNC730(ip_config->hdmi_config_base,
					GB02MAC1089(GB02MAC755));
		conn_stat = (phy_status & GB02MAC761) ? true : false;
	} else if (CONNECTOR1 == conn_id) {
		phy_status = GB02FUNC730(ip_config->hdmi_config_base,
					GB02MAC1089(GB02MAC758));
		conn_stat = (phy_status & GB02MAC767) ? true : false;
	} else
		conn_stat = false;
	phy_status = GB02FUNC396(conn_id);
	if (phy_status > -1)
		conn_stat = (phy_status) ? true : false;

	return conn_stat;
}
static bool GB02FUNC405(struct GB02STR252 *ip_config,
					int conn_id)
{
	if (CONNECTOR0 == conn_id)
		return true;
	else
		return false;
}

static bool GB02FUNC407(struct GB02STR252 *ip_config,
					int conn_id)
{
	return GB02FUNC1500(conn_id);
}

int GB02FUNC408(void __iomem *ddr_base)
{
#if 0
	void __iomem *ddrBase = NULL;
	if (NULL == ctx) {
		gb_printf(KERN_ERR, "ddr init fail\n");
		return -1;
	}
	ddrBase = ctx->ipConfig.ddr_config_base;

	ddr_init(ddrBase);
#endif
	return 0;
}
int GB02FUNC413(void __iomem *pcie_base, void *gb_ver)
{
	struct GB02STR117 *gb_version = (struct GB02STR117 *)gb_ver;
	GB02FUNC607(pcie_base, gb_version);
	return 0;
}

struct GB02STR244 gb_kms_ops = {
	.enter_config		= GB02FUNC196,
	.config_mode		= GB02FUNC216,
	.gamma_set		= GB02FUNC349,
	.config_base		= GB02FUNC327,
	.config_base_atomic = GB02FUNC289,
	.get_resolution		= GB02FUNC167,
	.save_csr_image		= GB02FUNC200,
	.config_csr_pos		= GB02FUNC207,
	.hide_csr		= GB02FUNC204,
	.query_hw		= GB02FUNC377,
	.leave_config		= GB02FUNC374,
	.config_overly_plane    = GB02FUNC258,
	.colse_overly_plane     = GB02FUNC278,
	.config_virt_base_atomic = GB02FUNC298,
};
struct GB02STR152 gb_mops[] = {
	{
		.card_type = GENBU_FPGA,
		.modeset_config		= GB02FUNC365,
		.prepare_config		= GB02FUNC181,
		.connector_state	= GB02FUNC405,
		.switch_conn		= GB02FUNC356,
	},
	{
		.card_type = GENBU_01,
		.modeset_config		= GB02FUNC369,
		.prepare_config		= GB02FUNC194,
		.connector_state	= GB02FUNC401,
		.switch_conn		= GB02FUNC361,
	},
	{
		.card_type = GENBU_02,
		.modeset_config		= GB02FUNC367,
		.prepare_config		= GB02FUNC191,
		.connector_state	= GB02FUNC407,
		.switch_conn		= GB02FUNC358,
	},
};
struct GB02STR152 *GB02FUNC421(int gb_type)
{
	if (gb_type > sizeof(gb_mops) / sizeof(gb_mops[0]))
		return NULL;

	return &(gb_mops[gb_type]);
}

struct GB02STR199 gb_ip_ops = {
	.init_ddr	= GB02FUNC408,
};

struct GB02STR241 gb_pm_ops = {
	.dpms_off	= GB02FUNC355,
};


struct GB02STR251  gb_cops= {
	.get_version	= GB02FUNC413,
};
