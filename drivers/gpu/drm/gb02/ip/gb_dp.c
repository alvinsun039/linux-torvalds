#include <linux/delay.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_dp_helper.h>
#else
#include <drm/display/drm_dp.h>
#endif
#include <drm/drm_modes.h>
#include <drm/drm_edid.h>
#include <drm/drm_atomic_helper.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 21)
#include <drm/drm_probe_helper.h>
#endif
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include "common/gb_pcie_info.h"
#include "common/xt.h"
#include "common/gb_common.h"
#include "kms/device/gbdc_edid.h"
#include "audio/i2s_platform.h"
#include "gb_dp.h"
#include "gb_dp_reg.h"
#include "gb_dp_dptx.h"
#include "gb_dp_avgen.h"
#include "gb_dp_dbg.h"
#include "kms/gbdc_connector.h"
#include "mcu_peripherals/backlight/gb_backlight.h"

uint32_t GB02FUNC819(uint32_t addr, uint32_t data[4],
	uint32_t num_byte, uint32_t dp_index);
void __iomem *gb_dp_reg_bar_base;
//#define DPTX_DEBUG_REG
//#define DP_PSVG_DEBUG
#if 0
static const struct drm_display_mode gbdc_modes[] = {
#if 1
	/* 102 - 4096x2160@60Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 594000, 4096, 4184,
		   4272, 4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
#else
	/* 0x52 - 1920x1080@60Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) }
	/* 0x04 - 640x480@60Hz */
	{ DRM_MODE("640x480", DRM_MODE_TYPE_DRIVER, 25175, 640, 656,
		   752, 800, 0, 480, 490, 492, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x10 - 1024x768@60Hz */
	{ DRM_MODE("1024x768", DRM_MODE_TYPE_DRIVER, 65000, 1024, 1048,
		   1184, 1344, 0, 768, 771, 777, 806, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 97 - 3840x2160@60Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
		   4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x09 - 800x600@60Hz */
	{ DRM_MODE("800x600", DRM_MODE_TYPE_DRIVER, 40000, 800, 840,
		   968, 1056, 0, 600, 601, 605, 628, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x55 - 1280x720@60Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x1c - 1280x800@60Hz */
	{ DRM_MODE("1280x800", DRM_MODE_TYPE_DRIVER, 83500, 1280, 1352,
		   1480, 1680, 0, 800, 803, 809, 831, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x27 - 1360x768@60Hz */
	{ DRM_MODE("1360x768", DRM_MODE_TYPE_DRIVER, 85500, 1360, 1424,
		   1536, 1792, 0, 768, 771, 777, 795, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x51 - 1366x768@60Hz */
	{ DRM_MODE("1366x768", DRM_MODE_TYPE_DRIVER, 85500, 1366, 1436,
		   1579, 1792, 0, 768, 771, 774, 798, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x2f - 1440x900@60Hz */
	{ DRM_MODE("1440x900", DRM_MODE_TYPE_DRIVER, 106500, 1440, 1520,
		   1672, 1904, 0, 900, 903, 909, 934, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x53 - 1600x900@60Hz */
	{ DRM_MODE("1600x900", DRM_MODE_TYPE_DRIVER, 108000, 1600, 1624,
		   1704, 1800, 0, 900, 901, 904, 1000, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
#endif
};
#endif

void GB02FUNC825(void __iomem *reg, u32 value)
{
	u64 offset = (u64)(reg - gb_dp_reg_bar_base);
	u32 rdata;

	iowrite32(value, reg);

	rdata = ioread32(reg);
	if (rdata != value)
		gb_printf(KERN_INFO, "%s err!!! reg:0x%llx, value:0x%x, rdata:0x%x\n",
			__func__, offset, value, rdata);
	else
		gb_printf(KERN_INFO, "%s ok!!! reg:0x%llx, value:0x%x, rdata:0x%x\n",
			__func__, offset, value, rdata);
}

u32 GB02FUNC828(void __iomem *reg)
{
	u32 rdata;
	u64 offset = (u64)(reg - gb_dp_reg_bar_base);

	rdata = ioread32(reg);

	gb_printf(KERN_INFO, "%s reg:0x%llx, rdata:0x%x\n",
		__func__, offset, rdata);
	return rdata;
}

uint32_t GB02FUNC830(uint32_t write_type, uint32_t addr, uint32_t data[4],
	uint32_t num_byte, uint32_t dp_index)
{
	uint32_t rdata, loop = 0;
	void __iomem *base_addr;

	base_addr = gb_dp_reg_bar_base + GB02MAC1062(dp_index) +
		GB02MAC1564; // 4MB each

	rdata = ioread32(base_addr + 0x0d04);
	rdata &= 0xFFFFFFFD; // clean bit[1]
	GB02FUNC825(base_addr + 0x0d04, rdata);

	GB02FUNC825(base_addr + 0x0b08, data[0]);
	GB02FUNC825(base_addr + 0x0b0c, data[1]);
	GB02FUNC825(base_addr + 0x0b10, data[2]);
	GB02FUNC825(base_addr + 0x0b14, data[3]);

	rdata = write_type + ((addr & 0x000FFFFF) << 8)
	 + (num_byte & 0x0F);
	GB02FUNC825(base_addr + 0x0b00, rdata);

	do {
		rdata = ioread32(base_addr + 0x0b04);
		msleep(1);
		loop++;
		if (loop > 1000) {
			gb_printf(KERN_ERR, "===%s %d fail 0x%x!===\n",
				__func__, __LINE__, rdata);
			break;
		}
	} while ((rdata&0x10000) != 0);
#ifdef GB02_PAL_DP
	msleep(10000);
#endif
	if ((rdata & 0x00F) == 0) {
		gb_printf(KERN_INFO, "%s-%d: succ loop:%d!rdata:0x%x,base:0x%pK,\
			addr:0x%x:0x%x 0x%x 0x%x 0x%x===\n",
			__func__, __LINE__, loop, rdata, base_addr, addr, data[0],
			data[1], data[2], data[3]);
		return 0;   // success
	}
	gb_printf(KERN_ERR, "%s %d failed!rdata:0x%x\n",
		__func__, __LINE__, rdata);
	return 1;   // failed
}

uint32_t GB02FUNC834(uint32_t addr, uint32_t data[4],
	uint32_t num_byte, uint32_t dp_index)
{
	uint32_t ret;
	uint32_t buf_data[4] = {0};
	int i = 0, j = 0;

retry:
	j++;
	ret = GB02FUNC830(GB02MAC1586, addr, data,
	 num_byte, dp_index);
	GB02FUNC819(addr, buf_data, num_byte, dp_index);
	for (i = 0; i < (num_byte + 1); i++) {
		if (buf_data[i] != data[i]) {
			gb_printf(KERN_ERR, "=======AUX ERR!!!!=====, addr:0x%x, i:%d, wdata:0x%x, rdata:0x%x\n",
				addr, i, data[i], buf_data[i]);
			msleep(1000);
			if (j <= 10)
				goto retry;
		}
	}

	return ret;
}

uint32_t GB02FUNC838(uint32_t addr, uint32_t data[4],
	uint32_t num_byte, uint32_t dp_index)
{
	return GB02FUNC830(GB02MAC1577, addr, data,
	 num_byte, dp_index);
}

uint32_t GB02FUNC840(uint32_t read_type, uint32_t addr, uint32_t *data,
	uint32_t num_byte, uint32_t dp_index)
{
	uint32_t rdata, loop = 0;
	void __iomem *base_addr;

	base_addr = gb_dp_reg_bar_base + GB02MAC1062(dp_index)
		+ GB02MAC1564; // 4MB each

	rdata = ioread32((void *)(base_addr + 0x0d04));
	rdata &= 0xFFFFFFFD; // clean bit[1]
	GB02FUNC825(base_addr + 0x0d04, rdata);

	rdata = read_type + ((addr & 0x000FFFFF) << 8) + (num_byte & 0x0F);
	GB02FUNC825(base_addr + 0x0b00, rdata);

	do {
		rdata = ioread32((void *)(base_addr + 0x0b04));
		msleep(1);
		loop++;
		if (loop > 1000) {
			gb_printf(KERN_ERR, "===%s %d fail 0x%x!===\n",
				__func__, __LINE__, rdata);
			//break;
			return 1;
		}
	} while ((rdata & 0x10000) != 0);
#ifdef GB02_PAL_DP
	msleep(10000);
#endif
	if ((rdata & 0x00FF) == 0) {
		data[0] = ioread32((void *)(base_addr + 0x0b08));
		data[1] = ioread32((void *)(base_addr + 0x0b0C));
		data[2] = ioread32((void *)(base_addr + 0x0b10));
		data[3] = ioread32((void *)(base_addr + 0x0b14));
		gb_printf(KERN_INFO, "%s %d: base:0x%pK, rdata:0x%x, addr:0x%x:0x%x 0x%x 0x%x 0x%x\n",
		 __func__, __LINE__, base_addr, rdata, addr, data[0],
			data[1], data[2], data[3]);

		return 0; // Success
	}
	gb_printf(KERN_ERR, "%s %d failed!rdata:0x%x\n",
		__func__, __LINE__, rdata);
	return 1; // FAILED
}

uint32_t GB02FUNC819(uint32_t addr, uint32_t data[4],
	uint32_t num_byte, uint32_t dp_index)
{
    return GB02FUNC840(GB02MAC1587, addr,
		data, num_byte, dp_index);
}

uint32_t GB02FUNC847(uint32_t addr, uint32_t data[4],
	uint32_t num_byte, uint32_t dp_index)
{
	return GB02FUNC840(GB02MAC1578, addr, data, num_byte, dp_index);
}

static void GB02FUNC848(struct dptx *dptx_info,
	const struct drm_display_mode *mode)
{
	struct dtd *mdtd_info = &dptx_info->vparams.mdtd;
	struct GB02STR124 *vparams_info = &dptx_info->vparams;
	int vic = 0;

	vparams_info->video_format = VCEA;
	vparams_info->pix_enc = RGB;
	vparams_info->bpc = COLOR_DEPTH_8;
	mdtd_info->h_active = mode->hdisplay;
	mdtd_info->v_active = mode->vdisplay;
	mdtd_info->h_blanking = mode->htotal - mode->hdisplay;
	mdtd_info->v_blanking = mode->vtotal - mode->vdisplay;
	mdtd_info->h_sync_offset = mode->hsync_start - mode->hdisplay;
	mdtd_info->v_sync_offset = mode->vsync_start - mode->vdisplay;
	mdtd_info->h_sync_pulse_width = mode->hsync_end - mode->hsync_start;
	mdtd_info->v_sync_pulse_width = mode->vsync_end - mode->vsync_start;
	mdtd_info->h_sync_polarity = !!(mode->flags & DRM_MODE_FLAG_PHSYNC);
	mdtd_info->v_sync_polarity = !!(mode->flags & DRM_MODE_FLAG_PVSYNC);
	mdtd_info->interlaced = !!(mode->flags & DRM_MODE_FLAG_INTERLACE);
	mdtd_info->pixel_clock = mode->clock;

	/* Input video dynamic_range & colorimetry */
	vic = drm_match_cea_mode(mode);
	if ((vic == 6) || (vic == 7) || (vic == 21) || (vic == 22) ||
	    (vic == 2) || (vic == 3) || (vic == 17) || (vic == 18)) {
		vparams_info->dynamic_range = CEA;
		vparams_info->colorimetry = ITU601;
	} else if (vic) {
		vparams_info->dynamic_range = CEA;
		vparams_info->colorimetry = ITU709;
	} else {
		vparams_info->dynamic_range = VESA;
		vparams_info->colorimetry = ITU709;
	}
	vparams_info->dynamic_range = VESA;
}

int GB02FUNC852(struct dptx *dptx_info,
	struct GB02STR70 *pcie_info,
	const struct drm_display_mode *mode, bool is_edp)
{
	int i, ret = 0;
	int is_vga = 0;

	dptx_info->cr_fail = false;
	dptx_info->mst = false;
	/* review ssc 打开也验证一下*/
	dptx_info->ssc_en = false;
	/* review FEC 打开也验证一下*/
	dptx_info->fec = false;
	dptx_info->dsc = false;
	dptx_info->streams = 1;
	dptx_info->multipixel = GB02MAC2044;
	dptx_info->dummy_dtds_present = false;
	dptx_info->selected_est_timing = NONE;
#if 0
	/* PCIE_LPDDR4 only has DP2 & DP3 */
	if ((GB02FUNC503(pcie_info) == PCIE_LPDDR4)
			&& dptx_info->index == 3)
		is_vga = 1;
	/* PCIE_FULL_LPDDR4 DP1(lvds) have no use*/
	if ((GB02FUNC503(pcie_info) == PCIE_FULL_LPDDR4)
		&& dptx_info->index == 4)
		is_vga = 1;
#endif
	if (is_edp)
		dptx_info->max_rate = GB02MAC2192;
	else if (is_vga)
		dptx_info->max_rate = GB02MAC2185;
	else {
		dptx_info->ssc_en = true;
		dptx_info->max_rate = GB02MAC2187;
	}
	dptx_info->max_lanes = GB02MAC1379;
	/* Initialize link parameters */
	memset(dptx_info->link.preemp_level, 0, sizeof(u8) * 4);
	for (i = 0; i < 4; i++)
#ifdef GB02_PAL_DP
		dptx_info->link.vswing_level[i] = 3;
#else
		dptx_info->link.vswing_level[i] = 0;
#endif
	memset(dptx_info->link.status, 0, DP_LINK_STATUS_SIZE);
#ifdef GB02_PAL_DP
	dptx_info->link.rate = dptx_info->max_rate;
	dptx_info->link.lanes = dptx_info->max_lanes;
#else
	if (is_vga)
		dptx_info->link.rate = GB02MAC2185;
	else
		dptx_info->link.rate = GB02MAC2187;
	dptx_info->link.lanes = 4;
#endif
	dptx_info->edid_getted = false;
	dptx_info->edid = kzalloc(GB02MAC1428 * 3, GFP_KERNEL);
	dptx_info->edid_second = kzalloc(GB02MAC1428, GFP_KERNEL);
	mutex_init(&dptx_info->mutex);
#ifdef GB_DP_TEST
	if (mode) {
		GB02FUNC848(dptx_info, mode);
		ret = GB02FUNC682(dptx_info, dptx_info->max_lanes,
			dptx_info->link.rate, dptx_info->vparams.bpc,
			dptx_info->vparams.pix_enc,
			dptx_info->vparams.mdtd.pixel_clock);
		if (ret) {
			dptx_err(dptx_info, "%s %d GB02FUNC682 err! ret:%d\n",
				__func__, __LINE__, ret);
		}
	}
#endif
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0)
#define DP_TPS4_SUPPORTED	(1 << 7)
#define DP_TRAINING_PATTERN_4 7

static  bool
drm_dp_tps4_supported(const u8 dpcd[DP_RECEIVER_CAP_SIZE])
{
	return dpcd[DP_DPCD_REV] >= 0x14 &&
		dpcd[DP_MAX_DOWNSPREAD] & DP_TPS4_SUPPORTED;
}
#endif

#define GB02MAC1132 25000
#define GB02MAC1133 (1 << 24)
/* for PLLSM14FFLAFRACG datasheet, PLLP&PLLM use this, KHz */
#define GB02MAC1134 3200000
#define GB02MAC1135 800000
#define GB02MAC1136 7
#define GB02MAC1137 1
/* for PLLSM14FFLJFRACQ datasheet, PLLD use this, KHz */
#define GB02MAC1138 8000000
#define GB02MAC1139 2000000

#define GB02MAC1140	25
/* postdiv_total = postdiv1 * postdiv2, 1 <= postdiv2 <= postdiv1 <= 7 */
uint32_t postdiv_typical_val[GB02MAC1140] =
	{49, 42, 36, 35, 30, 28, 25, 24, 21, 20, 18, 16, 15, 14, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

int GB02FUNC871(int dp_index)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];
	u32 rdata;
	void __iomem *base_addr = dptx_info->base +
		GB02MAC1570 - GB02MAC1564;

	rdata = GB02FUNC828(base_addr + 0x0048);
	if (!(rdata & 0x1))
		return 0;
	rdata &= (~0x1);
	GB02FUNC825(base_addr + 0x0048, rdata);
	return 0;
}

int GB02FUNC873(int dp_index)
{
#if 1
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];
	u32 rdata;
	void __iomem *base_addr = dptx_info->base +
		GB02MAC1570 - GB02MAC1564;

	GB02FUNC788(dptx_info, 0);
	rdata = GB02FUNC828(base_addr + 0x0048);
	if (!(rdata & 0x1))
		return 0;

	rdata &= (~0x1);
	GB02FUNC825(base_addr + 0x0048, rdata);

	msleep(1);
#else
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];

	GB02FUNC788(dptx_info, 0);
#endif
	return 0;
}

int GB02FUNC879(int dp_index)
{
#if 1
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];
	u32 rdata, i;
	void __iomem *base_addr = dptx_info->base +
		GB02MAC1570 - GB02MAC1564;

	GB02FUNC789(dptx_info, 0);
	rdata = GB02FUNC828(base_addr + 0x0048);
	if (rdata & 0x1)
		return 0;

	rdata |= 0x1;
	GB02FUNC825(base_addr + 0x0048, rdata);

	/* waiting for PLL Lock */
	for (i = 0; i < 100; i++) {
		rdata = GB02FUNC828(base_addr + 0x0058);
		if (rdata & (1 << 1)) {
			gb_printf(KERN_INFO, "%s %d PLLM LOCKED!",
				__func__, __LINE__);
			msleep(1);
			return 0;
		}
		else
			msleep(1);
	}
	gb_printf(KERN_ERR, "%s %d PLLM LOCK FAIL!",
			__func__, __LINE__);

	return -1;
#else
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];

	GB02FUNC789(dptx_info, 0);
	return 0;
#endif
}

static int GB02FUNC885(void __iomem *base_addr)
{
	/*
	 *PLLP pixel clock
	 *PLLD link clock, 100MHz
	 *PLLM DC core clock，PLLM must >= PLLP when use DC scale
	 */
	tDCDP_PLLM_CFG0 pllm_cfg0 = {0};
	tDCDP_PLLM_CFG1 pllm_cfg1 = {0};
	tDCDP_PLLD_CFG0 plld_cfg0 = {0};
	tDCDP_PLLD_CFG1 plld_cfg1 = {0};
	uint32_t rdata, i;

	pllm_cfg0.val = 0;
	pllm_cfg1.val = 0;
	pllm_cfg0.data.en		= 0;
	pllm_cfg0.data.bypass	= 0;
	pllm_cfg0.data.dsmen	= 0;
	pllm_cfg0.data.refdiv	= 0;
	pllm_cfg0.data.postdiv2 = 1;
	pllm_cfg0.data.postdiv1 = 5;
	pllm_cfg0.data.fbdiv	= 120;
	pllm_cfg1.data.frac 	= 0;

	GB02FUNC825(base_addr + 0x0048, pllm_cfg0.val);
	GB02FUNC825(base_addr + 0x004C, pllm_cfg1.val);
	pllm_cfg0.data.en  = 1;
	GB02FUNC825(base_addr + 0x0048, pllm_cfg0.val);

	plld_cfg0.val = 0;
	plld_cfg1.val = 0;
	plld_cfg0.data.en	= 0;
	plld_cfg0.data.bypass	= 0;
	plld_cfg0.data.refdiv	= 0;
	plld_cfg0.data.postdiv2 = 4;
	plld_cfg0.data.postdiv1 = 5;
	plld_cfg0.data.fbdiv	= 120;
	plld_cfg0.data.pllbw	= 1;
	plld_cfg1.data.frac 	= 0;
	GB02FUNC825(base_addr + 0x0050, plld_cfg0.val);
	GB02FUNC825(base_addr + 0x0054, plld_cfg1.val);
	plld_cfg0.data.en  = 1;
	GB02FUNC825(base_addr + 0x0050, plld_cfg0.val);

	/* waiting for PLL Lock */
	for (i = 0; i < 100; i++) {
		rdata = GB02FUNC828(base_addr + 0x0058);
		if ((rdata & (1 << 1)) &&
			(rdata & (1 << 2)))
			return 0;
		else
			msleep(1);
	}

	gb_printf(KERN_ERR, "%s %d PLLM&D LOCK FAIL!",
		__func__, __LINE__);
	return -1;
}

static int GB02FUNC889(void __iomem *base_addr,
	uint32_t pixel_clock)
{
	/*
	 *PLLP pixel clock
	 *PLLD link clock, 100MHz
	 *PLLM DC core clock，PLLM must >= PLLP when use DC scale
	 */
	tDCDP_PLLP_CFG0 pllp_cfg0 = {0};
	tDCDP_PLLP_CFG1 pllp_cfg1 = {0};
	uint32_t rdata, postdiv_total, postdiv1 = 0,
		 postdiv2 = 0, fvco_real, fbdiv_int, fbdiv_frac;
	int i = 0;

	if ((pixel_clock != 0) && (pixel_clock <= GB02MAC1134))
		postdiv_total = GB02MAC1134 / pixel_clock;
	else {
		gb_printf(KERN_ERR, "%s %d clock freq err!%uKHz\n",
				__func__, __LINE__, pixel_clock);
		return -1;
	}

	/* postdiv_total = postdiv1 * postdiv2, 1 <= postdiv2 <= postdiv1 <= 7 */
	for (i = 0; i < GB02MAC1140; i++) {
		if (postdiv_total >= postdiv_typical_val[i]) {
			postdiv_total = postdiv_typical_val[i];
			break;
		}
	}

	fvco_real = pixel_clock * postdiv_total;
	fbdiv_int = fvco_real / GB02MAC1132;
	/* fbdiv_frac is frac * 2^24 */
	fbdiv_frac = (u32)(((u64)(fvco_real % GB02MAC1132) * GB02MAC1133) / GB02MAC1132);

	/* postdiv1 >= postdiv2 */
	for (i = GB02MAC1136; i >= GB02MAC1137; i--) {
		if ((postdiv_total % i) == 0) {
			postdiv1 = i;
			postdiv2 = postdiv_total / postdiv1;
			break;
		}
	}

	if (postdiv1 == 0) {
		gb_printf(KERN_ERR, "%s %d invalid postdiv! pixel:%u",
			__func__, __LINE__, pixel_clock);
		return -1;
	}

	/* extdiv和extdiven是对最终输出的时钟是否再进行一次分频 */
	// PLLP EN
	pllp_cfg0.val = 0;
	pllp_cfg1.val = 0;
	pllp_cfg0.data.en	= 0;
	pllp_cfg0.data.bypass	= 0;
	pllp_cfg0.data.dsmen	= 1;
	pllp_cfg0.data.refdiv	= 0;
	pllp_cfg0.data.postdiv2 = postdiv2;
	pllp_cfg0.data.postdiv1 = postdiv1;
	pllp_cfg0.data.fbdiv	= fbdiv_int;
	pllp_cfg0.data.extdiv	= 0;
	pllp_cfg0.data.extdiven = 1;
	pllp_cfg1.data.frac = fbdiv_frac;

	GB02FUNC825(base_addr + 0x0040, pllp_cfg0.val);
	GB02FUNC825(base_addr + 0x0044, pllp_cfg1.val);
	pllp_cfg0.data.en = 1;
	GB02FUNC825(base_addr + 0x0040, pllp_cfg0.val);

	/* waiting for PLL Lock */
	for (i = 0; i < 100; i++) {
		rdata = GB02FUNC828(base_addr + 0x0058);
		if (rdata & (1 << 0))
			return 0;
		else
			msleep(1);
	}

	gb_printf(KERN_ERR, "===%s PLLP LOCK FAIL!pix:%u, div2:%u, div1:%u, fbdivint:%u, frac:%u===\n",
		__func__, pixel_clock, postdiv2,
		postdiv1, fbdiv_int, fbdiv_frac);

	return -1;
}

static int GB02FUNC900(void __iomem *base_addr,
	uint32_t pixel_clock)
{
	int ret = 0;

	ret |= GB02FUNC885(base_addr);
	ret |= GB02FUNC889(base_addr, pixel_clock);

	return ret;
}

#define GB02MAC1141 16
int GB02FUNC903(int dp_index,
	struct GB02STR70 *pcie_info, bool is_edp)
{
	/*
	 * DCDP CONFIG
	 */
	void __iomem *base_addr = NULL;
	void __iomem *psvg_base_addr = NULL;
	uint32_t buf_data[GB02MAC1141 / sizeof(uint32_t)] = {0};
	uint32_t rdata = 0;
#ifdef DP_PSVG_DEBUG
	uint32_t psvg_data, h_front, v_front;
#endif
	u64 max_loop = 10;
	int loop_times = 0;
	int dp_bw = 0;
	int ret = 0;
	/* asic sleep time less */
#ifdef GB02_PAL_DP
	int sleep_time = 10000;
#else
	int sleep_time = 150;
#endif
	int retry_cr_time = 0;
	struct dptx *dptx_info = NULL;
	const struct drm_display_mode *mode = GB02FUNC119(dp_index, 0);
	void __iomem *reg_bar_addr =
		pcie_info->pci_bars[GB02FUNC465(pcie_info->GB02STR153)].mmio;

	if (!mode) {
		gb_printf(KERN_ERR, "%s %d GB02FUNC119 fail!\n",
			__func__, __LINE__);
		return -1;
	}

	dptx_info = kzalloc(sizeof(struct dptx), GFP_KERNEL);
	if (!dptx_info) {
		gb_printf(KERN_ERR, "%s %d alloc dptx_info fail!\n",
			__func__, __LINE__);
		return -1;
	}

	gb_dp_reg_bar_base = reg_bar_addr;
	ret = GB02FUNC852(dptx_info, pcie_info, mode, is_edp);
	if (ret) {
		gb_printf(KERN_ERR, "init dptx info err! ret:%d\n", ret);
		return ret;
	}
	dptx_info->dev = &pcie_info->pdev->dev;
	dptx_info->index = dp_index;

	/* ===dcdp cfg:GB02MAC1570 云盘文档cfg_reg.xml=== */
	base_addr = reg_bar_addr + GB02MAC1062(dp_index) +
		GB02MAC1570;
	/* review 只读的,luna确认一下 */
	/* zfl:ana:pwr_en，reserved */
	GB02FUNC825(base_addr + 0x08, 0x01);
	/* zfl:upcs 0:PCS pwr_en reserved  1:PCS pwr_stable reserved*/
	GB02FUNC825(base_addr + 0x0C, 0x03);
	/* review luna仿真为啥b */
	/*
	 * zfl:upcs bit0:raw PCS pwr_en reserved
	 * bit1:raw PCS pwr_stable reserved
	 * bit5:2:pipelane_powerdown 1号lane powerdown了？
	 * 仿真配的0xb,先改成0x3
	 */
	//write32(base_addr + 0x10, 0x0b);
	GB02FUNC825(base_addr + 0x10, 0x03);
	/* zfl:pma 0:pwr_en reserved  1:pwr_stable reserved*/
	GB02FUNC825(base_addr + 0x14, 0x03);
	/*
	 * zfl:bit0:配置为0用PLLD作为链路参考时钟，
	 * 配置为1用外部差分100Mhz时钟
	 */
	GB02FUNC825(base_addr + 0x18, 0x30);
	/* zfl:aux block enable */
	GB02FUNC825(base_addr + 0x20, 0x01);
	/* ===dcdp cfg end=== */

	/* asic needs PLL cfg */
	GB02FUNC900(base_addr, mode->clock);

	psvg_base_addr = reg_bar_addr + GB02MAC1062(dp_index)
		+ GB02MAC1575;
	rdata = GB02FUNC828(psvg_base_addr + 0x0);
	rdata &= (~0x1);
	GB02FUNC825(psvg_base_addr + (0 << 2), rdata);
	msleep(sleep_time);
	rdata = GB02FUNC828(psvg_base_addr + 0x0);
	rdata |= 1;
	GB02FUNC825(psvg_base_addr + (0 << 2), rdata);

	/* asic disable all dp intr before cfg */
	// STEP 0
	// CCTL
	base_addr = reg_bar_addr + GB02MAC1062(dp_index)
	 + GB02MAC1564;
	dptx_info->base = base_addr;

#ifndef GB02_PAL_DP
	/* disable all interrupt */
	GB02FUNC1057(dptx_info);
#endif
	rdata = dptx_readl(dptx_info, 0x0200);
	if (is_edp) {
		/* enable edp */
		rdata |= (1 << 27);
	}
	/* disable fec */
	rdata &= (~(1 << 26));
	/* reset SCALE DOWN */
	rdata &= (~(0x01FF << 16));
	/* disable fast train */
	rdata &= (~(0x0001 << 2));
	/* ENABLE_SCRAMBLE */
	rdata &= (~(0x0001 << 0));
	//CCTL
	dptx_writel(dptx_info, 0x0200, rdata);

	/* review:try */
	/* PHY POWER DOWN */
	dptx_writel(dptx_info, 0x0a00, 0x60000);
	msleep(sleep_time);
	rdata = dptx_readl(dptx_info, 0x0a00);
	if (rdata != 0x60000) {
		gb_printf(KERN_ERR, "ERROR! %s %d dp%d 0xa00!:0x%x\n",
		 __func__, __LINE__, dp_index, rdata);
//		return 0;
	}
	/* PHY POWER ON */
	rdata &= (~(0x0F << 17));
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);
	msleep(sleep_time);
	// Step 1: Assert PHY Reset .
	// a. Set the PHY soft reset by setting the SOFT_RESET_CTRL[1] register.
	rdata = dptx_readl(dptx_info, 0x0204);
	rdata |= (1 << 1);
	//SOFT_RESET_CTRL
	dptx_writel(dptx_info, 0x0204, rdata);

	// b. Refer the PHY document to know if anything else need to be
	// reset at this time.

	// c. Wait for 5 us to make sure that the soft reset takes effect and
	// then clear the PHY soft reset bit by writing a 0 to this bit.
	msleep(1000);
	rdata = dptx_readl(dptx_info, 0x0204);
	rdata &= (~(0x1<<1));
	//SOFT_RESET_CTRL
	dptx_writel(dptx_info, 0x0204, rdata);

	// d. Wait for the PHYIF_CTRL.PHY_BUSY register to be 0.
	//PHYIF_CTRL
	loop_times = 0;
	do {
		if (loop_times >= max_loop) {
			gb_printf(KERN_ERR, "%s %d PHY_BUSY err!rdata:0x%x\n",
				__func__, __LINE__, rdata);
			//break;
			return -1;
		}
		rdata = dptx_readl(dptx_info, 0x0a00);
		loop_times++;
		msleep(1);
	} while (((rdata >> 12) & 0xF) != 0);

	// e. write correct AUX_250US_CNT_LIMIT, AUX_2000US_CNT_LIMIT,
	// AUX_100000US_CNT_LIMIT value accroding aux16mhz_clk_i .
	//AUX_250US_CNT_LIMIT
	dptx_writel(dptx_info, 0x0b40, 0x000000fa);
	//AUX_2000US_CNT_LIMIT
	dptx_writel(dptx_info, 0x0b44, 0x000007d0);
	//AUX_100000US_CNT_LIMIT
	dptx_writel(dptx_info, 0x0b48, 0x000186a0);

	// f. Ensure that GENERAL_INTERRUPT_ENABLE.HPD_EVENT are cleared.
	rdata = dptx_readl(dptx_info, 0x0d04);
	rdata &= (~0x01);
	//GENERAL_INTERRUPT_ENABLE
	/* TODO:test */
	//dptx_writel(dptx_info, 0x0d04, 0xffffffff);

	// Step 2: Wait for HPD (Hot Plug Detect) interrupt.
	/* Enable all HPD interrupts */
	rdata = dptx_readl(dptx_info, GB02MAC2129);
	rdata |= (GB02MAC2248 |
			GB02MAC2249 |
			GB02MAC2250);

	dptx_writel(dptx_info, GB02MAC2129, rdata);
#ifndef GB02_PAL_DP
	/* Enable all top-level interrupts */
	GB02FUNC1055(dptx_info);
#endif

	loop_times = 0;
	do {
		if (loop_times >= max_loop) {
			gb_printf(KERN_ERR, "%s %d HPD_EVENT err!rdata:0x%x, hpdstatus:0x%x\n",
				__func__, __LINE__, rdata,
				dptx_readl(dptx_info, 0x0d08));
			//break;
			return -1;
		}
		msleep(1);
		rdata = dptx_readl(dptx_info, 0x0d00);
		loop_times++;
	} while ((rdata & 0x01) != 1);

	gb_printf(KERN_ERR, "%s %d HPD_EVENT ok!rdata:0x%x, hpdstatus:0x%x, loop:%d\n",
		__func__, __LINE__, rdata,
		dptx_readl(dptx_info, 0x0d08), loop_times);
	// Step 3: Read hardware parameter register DPTX_CONFIG_REG1,GB02MAC2367,DPTX_CONFIG_REG3 to get the DPTX configuration parameters.
	rdata = dptx_readl(dptx_info, 0x0100);
	rdata = dptx_readl(dptx_info, 0x0104);
	rdata = dptx_readl(dptx_info, 0x0108);

//// Step 4: Read EDID of the sink using I2C over AUX, and parse the EDID data
//		for(int i = 0; i < 0x7d; i = i + 16) begin
//		  i2c_over_aux_read(i, i2c_rdata);
//		end
//
//// Step 5: Read Sink DPCD registers
	//DP_MSTM_CAP
#if 0
	ret = GB02FUNC819(0x00021, buf_data, 0x1, dp_index);
	if (ret) {
		gb_printf(KERN_ERR, "===zfldebug auxread error！%s %d ret:%d===\n",
				__func__, __LINE__, ret);
		return -1;
	}
	if (buf_data[0] & DP_MST_CAP) {
	//	buf_data[0] = 0;
	//	GB02FUNC834(DP_MSTM_CTRL, buf_data, 1, dp_index);
	}
#endif
	//DP_DPCD_REV
	GB02FUNC819(0x00000, buf_data, 0xf, dp_index);
	memcpy((void *)dptx_info->rx_caps, (void *)buf_data, 16);
	if (buf_data[0] == 0 && buf_data[1] == 0 &&
		buf_data[2] == 0 && buf_data[3] == 0) {
		gb_printf(KERN_ERR, "get monitor info failed! return!\n");
		return -1;
	}

	//DP_DP13_DPCD_REV
	GB02FUNC819(0x02200, buf_data, 0xf, dp_index);
	//DP_EDP_CONFIGURATION_CAP
	GB02FUNC819(0x0000d, buf_data, 0x1, dp_index);
	//DP_DSC_SUPPORT
	GB02FUNC819(0x00060, buf_data, 0x0, dp_index);
	//DP_FEC_CAPABILITY
	GB02FUNC819(0x00090, buf_data, 0x0, dp_index);

#ifndef GB02_PAL_DP
	dptx_info->link.lanes = (min(dptx_info->link.lanes, drm_dp_max_lane_count(dptx_info->rx_caps)));
	dptx_info->link.rate = (min(dptx_info->link.rate, GB02FUNC1126(dptx_info->rx_caps[DP_MAX_LINK_RATE])));
#endif
	dptx_info(dptx_info, "dptx lanes:%u, rate:%u\n", dptx_info->link.lanes,
			dptx_info->link.rate);
	// Step 6: Program Sink DPCD Link Configuration registers.
	// This step is depend on the read infomation of Step 3 and Step 5
	if (is_edp) {
		/* ASSR Enable */
		buf_data[0] = DP_ALTERNATE_SCRAMBLER_RESET_ENABLE;
		GB02FUNC834(DP_EDP_CONFIGURATION_SET, buf_data, 0, dp_index);
	}

	// Step 7: Initiate link training
	// 7.a Disable FEC.
	rdata = dptx_readl(dptx_info, 0x0200);
	rdata &= (~(0x01<<26));
	//CCTL
	dptx_writel(dptx_info, 0x0200, rdata);
	// 7.b Disable the fast link training if enabled. Fast link training is enabled by default.
	/* already disabled */

	// 7.c Follow the steps specified in “Full Link Bringup Sequence,
	// ”for initiating link training.
	// 7.c.1 Based on the highest link rate,
	// highest number of lanes supported,
	// and Source policy, set PHYIF_CTRL.PHY_LANES,
	// and PHYIF_CTRL.PHY_RATE.
	// If Rocket IO PHY is used, set PHYIF_CTRL.SSC_DIS=1.
	rdata = dptx_readl(dptx_info, 0x0a00);
	if (is_edp) {
		//EDP PHY RATE
		rdata &= (~(0x7 << 26));
		rdata |= ((dptx_info->link.rate - GB02MAC2193) << 26);
	} else {
		rdata &= (~(0x3 << 4));
		rdata |= (dptx_info->link.rate << 4);
	}
	// 4 Lanes
	rdata &= (~(0x3 << 6));
	rdata |= ((dptx_info->link.lanes >> 1) << 6);
	//rdata |= (2 << 6);
	if (!dptx_info->ssc_en)
		rdata |= (0x01 << 16);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.2 Wait until PHYIF_CTRL.PHY_BUSY is cleared for
	// the desired lanes that have been enabled. For
	// example, if 4 lanes are enabled, then wait
	// until PHYIF_CTRL.PHY_BUSY[3:0]=4’h0. If only 1 lane
	// enabled, then wait until PHYIF_CTRL.PHY_BUSY[0]=0.
	//PHYIF_CTRL
	loop_times = 0;
	do {
		if (loop_times >= max_loop) {
			gb_printf(KERN_ERR, "%s %d PHY_BUSY err!rdata:0x%x\n",
				__func__, __LINE__, rdata);
			//break;
			return -1;
		}
		rdata = dptx_readl(dptx_info, 0x0a00);
		loop_times++;
		msleep(1);
	} while (((rdata >> 12) & 0xF) != 0);

	//GB02FUNC1525(dptx_info);
	// 7.c.3 Set PHYIF_CTRL.TPS_SEL=0 to force
	// the transmitted pattern to none.
	/* dptx/link.c +312 */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata &= (~0xF);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.4 Program the pre-emphasis and voltage swing
	// values into the PHY_TX_EQ registers corresponding to
	// voltage swing level 0 and pre-emphasis level 0
	// for all enabled lanes.
	/* dptx/link.c +310 */
	GB02FUNC1525(dptx_info);

	// 7.c.5 Set PHYIF_CTRL.TPS_SEL=1 to send TPS1.
	/* dptx/link.c none */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata = (rdata & (~0xF)) | 0x01;
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.6 Enable the transmitter by setting PHYIF_CTRL.XMIT_EN
	// for each enabled lane.
	/* dptx/link.c +318 */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata |= (0x0F << 8);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.7 Set the following DPCD registers on the Sink
	// – using AUX Channel Interface Registers.
	// 7.c.7.a LINK_BW_SET register (DPCD Address 00100h)
	// to the desired rate.
	// 06h = 1.62Gbps; 0Ah = 2.7Gbps; 14h = 5.4Gbps; 1Eh = 8.1Gbps;
	// other value for eDP .
	// 7.c.7.b LANE_COUNT_SET register(DPCD Address 00101h):
	// 0x84 = 4 lane, Enhanced_Frame_En.

	// 7.c.7.f TRAINING_LANEx_SET register
	// (DPCD Addresses 00103h through 00106h) for each lane to
	// voltage swing level 0 and pre-emphasis level 0 .
	// 7.c.7.g Note that TRAINING_PATTERN_SET and TRAINING_LANEx_SET
	// registers must be written in one AUX CH burst write transaction.
	/* zfl:第三个参数+1是真正写入/读取的字节数 */
	/* dptx/link.c +325 */
	if (!is_edp)
		dp_bw = GB02FUNC1125(dptx_info->link.rate);
	buf_data[0] = dp_bw;
	GB02FUNC834(0x00100, buf_data, 0, dp_index);
	buf_data[0] = dptx_info->link.lanes;
	if (drm_dp_enhanced_frame_cap(dptx_info->rx_caps))
		buf_data[0] |= DP_ENHANCED_FRAME_CAP;
	GB02FUNC834(0x00101, buf_data, 0, dp_index);

	/* 7.c.7.c DOWNSPREAD_CTRL register (DPCD Address 00107h),
	 * SPREAD_AMP must be set to 1 for a regular PHY and 0 for Rocket IO.
	 * 7.c.7.d MAIN_LINK_CHANNEL_CODING_SET register
	 * (DPCD Address 00108h) to 01h
	 * dptx/link.c +351
	 */
	if (dptx_info->ssc_en)
		buf_data[0] = 1 << 4;
	else
		buf_data[0] = 0;
	GB02FUNC834(0x00107, buf_data, 0, dp_index);
	msleep(sleep_time);
	buf_data[0] = 0x000001;
	GB02FUNC834(0x00108, buf_data, 0, dp_index);
	gb_printf(KERN_ERR, "108 cfg done!!!!!!!\n");
	msleep(100);
retry_cr:
	ret = GB02FUNC1550(dptx_info);
	if (ret) {
		/* TODO: CR FAIL RETRY, reduce lane num & lane rate */
		if (retry_cr_time < 5)
			goto retry_cr;
		else {
			gb_printf(KERN_ERR, "%s %d CR Fail!\n", __func__, __LINE__);
			ret = 0;
		}
	} else
			gb_printf(KERN_ERR, "%s %d CR Success!\n", __func__, __LINE__);

	/* asic needs retry for eq done */
	ret = GB02FUNC1553(dptx_info);
	if (ret) {
		/* TODO: EQ FAIL RETRY, reduce lane num & lane rate */
		gb_printf(KERN_ERR, "%s %d EQ fail!\n", __func__, __LINE__);
		ret = 0;
		goto retry_cr;
	} else
		gb_printf(KERN_ERR, "%s %d EQ Success!\n", __func__, __LINE__);

	// 7.c.12 If all the previous registers bits are 1,
	// then training is complete. Otherwidth, enter loop process .

	// 7.c.13 Set PHYIF_CTRL.TPS_SEL = 0.
	// Set DPCD Sink TRAINING_PATTERN_SET register to 00h.
	// dptx/link.c +626 GB02FUNC1118(dptx,
	// GB02MAC2173);
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata &=  (~0x0F);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	/* dptx/link.c +628 GB02FUNC1535(dptx,
	 * DP_TRAINING_PATTERN_DISABLE);
	 */
	buf_data[0] = 0x00000000;
	GB02FUNC834(0x00102, buf_data, 0, dp_index);

#if 1
	/* ===start set video para=== */
	GB02FUNC788(dptx_info, 0);
	GB02FUNC663(dptx_info, 0);
	// 8.h If Sink supports enhance_frame_cap, set CCTL.Enhance_framing_en
	/* dptx/link.c +329 */
	rdata = dptx_readl(dptx_info, 0x0200);
	//Enhance_framing_en
	if (drm_dp_enhanced_frame_cap(dptx_info->rx_caps))
		rdata |= GB02MAC2157;
	else
		rdata &= ~GB02MAC2157;
#ifdef GB02_PAL_DP
	/* paladium should disable sdp interleave */
	rdata |= (1 << 5);
#endif
	dptx_writel(dptx_info, 0x0200, rdata);

	// 8.i ~ 8.m for MST mode
	/*
	 * 8.n If Audio and encrypt is not support,
	 * then enable video after step 11
	 */
	rdata = dptx_readl(dptx_info, 0x0300);
	//Encryption enable
	if (is_edp)
		rdata |= (1 << 24);
	// VSAMPLE_CTRL[0].VIDEO_STREAM_ENABLE
	rdata |= 0x20;
	dptx_writel(dptx_info, 0x0300, rdata);

#endif
#ifdef DP_PSVG_DEBUG
	rdata = GB02FUNC828(psvg_base_addr + (0x12 << 2));
	rdata = 0xa;
	GB02FUNC825(psvg_base_addr + (0x12 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x1 << 2));
	rdata &= 0xfffffff8;
	rdata |= (1 | (dptx_info->vparams.mdtd.h_sync_polarity << 1) |
			(dptx_info->vparams.mdtd.v_sync_polarity << 1));
	GB02FUNC825(psvg_base_addr + (0x1 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x2 << 2));
	rdata = 0;
	GB02FUNC825(psvg_base_addr + (0x2 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x3 << 2));
	rdata &= 0xffffff00;
	rdata |= (dptx_info->vparams.mdtd.h_active & 0xff);
	GB02FUNC825(psvg_base_addr + (0x3 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x4 << 2));
	rdata &= 0xffffffe0;
	rdata |= (dptx_info->vparams.mdtd.h_active >> 8);
	GB02FUNC825(psvg_base_addr + (0x4 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x5 << 2));
	rdata &= 0xffffff00;
	rdata |= (dptx_info->vparams.mdtd.h_blanking & 0xff);
	GB02FUNC825(psvg_base_addr + (0x5 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x6 << 2));
	rdata &= 0xffffffe0;
	rdata |= (dptx_info->vparams.mdtd.h_blanking >> 8);
	GB02FUNC825(psvg_base_addr + (0x6 << 2), rdata);

	h_front = dptx_info->vparams.mdtd.h_sync_offset;
	rdata = GB02FUNC828(psvg_base_addr + (0x7 << 2));
	rdata &= 0xffffff00;
	rdata |= (h_front & 0xff);
	GB02FUNC825(psvg_base_addr + (0x7 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x8 << 2));
	rdata &= 0xffffffe0;
	rdata |= (h_front >> 8);
	GB02FUNC825(psvg_base_addr + (0x8 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x9 << 2));
	rdata &= 0xffffff00;
	rdata |= (dptx_info->vparams.mdtd.h_sync_pulse_width & 0xff);
	GB02FUNC825(psvg_base_addr + (0x9 << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0xa << 2));
	rdata &= 0xffffffe0;
	rdata |= (dptx_info->vparams.mdtd.h_sync_pulse_width >> 8);
	GB02FUNC825(psvg_base_addr + (0xa << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0xb << 2));
	rdata &= 0xffffff00;
	rdata |= (dptx_info->vparams.mdtd.v_active & 0xff);
	GB02FUNC825(psvg_base_addr + (0xb << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0xc << 2));
	rdata &= 0xffffffe0;
	rdata |= (dptx_info->vparams.mdtd.v_active >> 8);
	GB02FUNC825(psvg_base_addr + (0xc << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0xd << 2));
	rdata &= 0xffffff00;
	rdata |= (dptx_info->vparams.mdtd.v_blanking & 0xff);
	GB02FUNC825(psvg_base_addr + (0xd << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0xe << 2));
	rdata &= 0xffffff00;
	rdata |= (dptx_info->vparams.mdtd.v_sync_offset & 0xff);
	GB02FUNC825(psvg_base_addr + (0xe << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0xf << 2));
	rdata &= 0xffffff00;
	rdata |= (dptx_info->vparams.mdtd.v_sync_pulse_width & 0xff);
	GB02FUNC825(psvg_base_addr + (0xf << 2), rdata);

	rdata = GB02FUNC828(psvg_base_addr + (0x11 << 2));
	rdata = 4;
	GB02FUNC825(psvg_base_addr + (0x11 << 2), rdata);
#if 0

	rdata = GB02FUNC828(psvg_base_addr + (0x27 << 2));
	rdata &= 0xfffffffc;
	rdata |= 2;
	GB02FUNC825(psvg_base_addr + (0x27 << 2), rdata);
#endif
	GB02FUNC825(psvg_base_addr + (0x38 << 2), 1);
	GB02FUNC825(psvg_base_addr + (0x39 << 2), 1);
#endif
#if 1
	/* ===start set audio para=== */
	// Step 9: Configure the controller for Audio Mode.
	rdata = dptx_readl(dptx_info, 0x0400);
	/* zfl:AUD_CONFIG1,配置audio为I2S输入bit[0]
	 * 使能8个声道bit[4:1]、
	 * 对输入数据24bit采样bit[9:5]、
	 * ctl为192kHz x 8 x 16 bits in 8-channel bit[10]
	 * ---TODO:使能的channel帕拉丁上
	 * 需根据audio实际声道数进行配置
	 */
	rdata &= 0xfffff800;	//8 channels
	{	//two channels
	rdata &= 0xffff8fff;

	rdata |= 0x1000;
	}
	rdata |= 0x0 << 0;
	rdata |= 0xf << 1;

	//rdata |= 0x18 << 5;	//24bit
	rdata |= 0x10 << 5;		//16bit
	rdata |= 0x0 << 10;
	dptx_writel(dptx_info, 0x0400, rdata);

	rdata = dptx_readl(dptx_info, 0x0500);
	rdata &= 0xfffffffc;
	rdata |= 0x1 << 0;
	rdata |= 0x1 << 1;
	/* SDP_VERTICAL_CTRL */
	dptx_writel(dptx_info, 0x0500, rdata);

	rdata = dptx_readl(dptx_info, 0x0504);
	rdata &= 0xfffffffc;
	rdata |= 0x1 << 0;
	rdata |= 0x1 << 1;
	/* SDP_HORIZONTAL_CTRL */
	dptx_writel(dptx_info, 0x0504, rdata);
	/* ===end set audio para=== */
#endif

//	base_addr = reg_bar_addr + GB02MAC1062(dp_index) + GB02MAC1575;
//	GB02FUNC825(base_addr + 0x7038, 0x01);

	/* zfl:ana:pwr_en，reserved */
	kfree(dptx_info);
	gb_printf(KERN_ERR, "dp test OK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	return ret;
}

int GB02FUNC940(int dp_index,
	struct GB02STR70 *pcie_info, bool is_edp)
{
	/**
	* DCDP CONFIG
	*/
	void __iomem *base_addr = NULL;
	uint32_t buf_data[GB02MAC1141 / sizeof(uint32_t)] = {0};
	uint32_t rdata = 0;
	unsigned int pattern = 0;
	u8 dp_pattern = 0;
	u64 max_loop = 10;
	int loop_times = 0;
	int ret = 0;
	/* asic sleep time less */
#ifdef GB02_PAL_DP
	int sleep_time = 10000;
#else
	int sleep_time = 10;
#endif
	struct dptx *dptx_info = NULL;
	const struct drm_display_mode *mode = GB02FUNC119(dp_index, 0);
	void __iomem *reg_bar_addr =
		pcie_info->pci_bars[GB02FUNC465(pcie_info->GB02STR153)].mmio;

	if (!mode) {
		gb_printf(KERN_ERR, "%s %d GB02FUNC119 fail!\n",
			__func__, __LINE__);
		return -1;
	}

	dptx_info = kzalloc(sizeof(struct dptx), GFP_KERNEL);
	if (!dptx_info) {
		gb_printf(KERN_ERR, "%s %d alloc dptx_info fail!\n",
			__func__, __LINE__);
		return -1;
	}

	gb_dp_reg_bar_base = reg_bar_addr;
	ret = GB02FUNC852(dptx_info, pcie_info, mode, is_edp);
	if (ret) {
		gb_printf(KERN_ERR, "init dptx info err! ret:%d\n", ret);
		return ret;
	}
	dptx_info->dev = &pcie_info->pdev->dev;
	dptx_info->index = dp_index;

	/* ===dcdp cfg:GB02MAC1570 云盘文档cfg_reg.xml=== */
	base_addr = reg_bar_addr + GB02MAC1062(dp_index) + GB02MAC1570;
	/* zfl:ana:pwr_en，reserved */
	GB02FUNC825(base_addr + 0x08, 0x01);
	/* zfl:upcs 0:PCS pwr_en reserved  1:PCS pwr_stable reserved*/
	GB02FUNC825(base_addr + 0x0C, 0x03);
	/* zfl:upcs bit0:raw PCS pwr_en reserved
	bit1:raw PCS pwr_stable reserved
	bit5:2:pipelane_powerdown 1号lane powerdown了？仿真配的0xb,先改成0x3 */
	GB02FUNC825(base_addr + 0x10, 0x03);
	/* zfl:pma 0:pwr_en reserved  1:pwr_stable reserved*/
	GB02FUNC825(base_addr + 0x14, 0x03);
	/* zfl:bit0:配置为0用PLLD作为链路参考时钟，配置为1用外部差分100Mhz时钟 */
	GB02FUNC825(base_addr + 0x18, 0x30);
	/* zfl:aux block enable */
	GB02FUNC825(base_addr + 0x20, 0x01);
	/* ===dcdp cfg end=== */

	/* asic needs PLL cfg */
	GB02FUNC900(base_addr, mode->clock);

#ifdef DP_PHY_CFG
	// Begin to initial DPTX_CONFIG_REG1
	// Fast Mode
	/* zfl:DCDP_CR_OFFSET是 smicPHY手册，此处仅配置仿真加速 帕拉丁不管 */
	base_addr = reg_bar_addr + GB02MAC1062(dp_index) + GB02MAC1568;
	GB02FUNC825(base_addr + 0xB05C, 0x0000FFFF);
#endif

	/* disable all interrupt */
	GB02FUNC1057(dptx_info);

	// STEP 0
	// CCTL
	base_addr = reg_bar_addr + GB02MAC1062(dp_index) + GB02MAC1564;
	dptx_info->base = base_addr;
	rdata = dptx_readl(dptx_info, 0x0200);
	if (is_edp) {
		/* enable edp */
		rdata |= (1 << 27);
	}
	/* disable fec */
	rdata &= (~(1 << 26));
	/* reset SCALE DOWN */
	rdata &= (~(0x01FF << 16));
	/* reduce HPD input from 100ms to 10us */
	//rdata |= (0x01 << 3);
	/* enable fast train */
	rdata |= (0x1 << 2);

	/* ENABLE_SCRAMBLE */
	rdata &= (~(0x0001 << 0));
	//CCTL
	dptx_writel(dptx_info, 0x0200, rdata);

	/* PHY POWER DOWN */
	dptx_writel(dptx_info, 0x0a00, 0x60000);
	msleep(sleep_time);
	rdata = dptx_readl(dptx_info, 0x0a00);
	if (rdata != 0x60000) {
		gb_printf(KERN_ERR, "ERROR! %s %d dp%d 0xa00!:0x%x\n", __func__, __LINE__, dp_index, rdata);
		return 0;
	}
	/* PHY POWER ON */
	rdata &= (~(0x0F << 17));
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);
	msleep(sleep_time);

	// Step 1: Assert PHY Reset .
	// a. Set the PHY soft reset by setting the SOFT_RESET_CTRL[1] register.
	rdata = dptx_readl(dptx_info, 0x0204);
	rdata |= (1 << 1);
	//SOFT_RESET_CTRL
	dptx_writel(dptx_info, 0x0204, rdata);

	// b. Refer the PHY document to know if anything else need to be reset at this time.

	// c. Wait for 5 us to make sure that the soft reset takes effect and then clear the PHY soft reset bit by writing a 0 to this bit.
	msleep(sleep_time);
	rdata = dptx_readl(dptx_info, 0x0204);
	rdata &= (~(0x1<<1));
	//SOFT_RESET_CTRL
	dptx_writel(dptx_info, 0x0204, rdata);

	// d. Wait for the PHYIF_CTRL.PHY_BUSY register to be 0.
	//PHYIF_CTRL
	loop_times = 0;
	do {
		if (loop_times >= max_loop) {
			gb_printf(KERN_ERR, "%s %d PHY_BUSY err!rdata:0x%x\n",
				__func__, __LINE__, rdata);
			//break;
			return -1;
		}
		rdata = dptx_readl(dptx_info, 0x0a00);
		loop_times++;
		msleep(1);
	} while(((rdata >> 12) & 0xF) != 0);

	/* 仿真没配，不需要吗?? */
	// e. write correct AUX_250US_CNT_LIMIT, AUX_2000US_CNT_LIMIT, AUX_100000US_CNT_LIMIT value accroding aux16mhz_clk_i .
	//AUX_250US_CNT_LIMIT ??
	dptx_writel(dptx_info, 0x0b40, 0x000000fa);
	//AUX_2000US_CNT_LIMIT
	dptx_writel(dptx_info, 0x0b44, 0x000007d0);
	//AUX_100000US_CNT_LIMIT
	dptx_writel(dptx_info, 0x0b48, 0x000186a0);

	// f. Ensure that GENERAL_INTERRUPT_ENABLE.HPD_EVENT are cleared.
	rdata = dptx_readl(dptx_info, 0x0d04);
	rdata &= (~0x01);
	//GENERAL_INTERRUPT_ENABLE===========================================
	//dptx_writel(dptx_info, 0x0d04, rdata);

	// Step 2: Wait for HPD (Hot Plug Detect) interrupt.
	/* Enable all HPD interrupts */
	rdata = dptx_readl(dptx_info, GB02MAC2129);
	rdata |= (GB02MAC2248 |
			GB02MAC2249 |
			GB02MAC2250);

	dptx_writel(dptx_info, GB02MAC2129, rdata);
	/* Enable all top-level interrupts */
	GB02FUNC1055(dptx_info);

	loop_times = 0;
	do {
		if (loop_times >= max_loop) {
			gb_printf(KERN_ERR, "%s %d HPD_EVENT err!rdata:0x%x, hpdstatus:0x%x\n",
				__func__, __LINE__, rdata, dptx_readl(dptx_info, 0x0d08));
			//break;
			return -1;
		}
		msleep(1);
		rdata = dptx_readl(dptx_info, 0x0d00);
		loop_times++;
	} while((rdata & 0x01) != 1);

	gb_printf(KERN_ERR, "%s %d HPD_EVENT ok!rdata:0x%x, hpdstatus:0x%x, loop:%d\n",
		__func__, __LINE__, rdata, dptx_readl(dptx_info, 0x0d08), loop_times);
	// Step 3: Read hardware parameter register DPTX_CONFIG_REG1,GB02MAC2367,DPTX_CONFIG_REG3 to get the DPTX configuration parameters.
	rdata = dptx_readl(dptx_info, 0x0100);
	rdata = dptx_readl(dptx_info, 0x0104);
	rdata = dptx_readl(dptx_info, 0x0108);

//// Step 4: Read EDID of the sink using I2C over AUX, and parse the EDID data
//		for(int i = 0; i < 0x7d; i = i + 16) begin
//		  i2c_over_aux_read(i, i2c_rdata);
//		end
//
//// Step 5: Read Sink DPCD registers
	//DP_MSTM_CAP
#if 0
	ret = GB02FUNC819(0x00021, buf_data, 0x1, dp_index);
	if (ret) {
		gb_printf(KERN_ERR, "===zfldebug auxread error！%s %d ret:%d===\n",
				__func__, __LINE__, ret);
		return -1;
	}
	if (buf_data[0] & DP_MST_CAP) {
	//	buf_data[0] = 0;
	//	GB02FUNC834(DP_MSTM_CTRL, buf_data, 1, dp_index);
	}
#endif
	//DP_DPCD_REV
	GB02FUNC819(0x00000, buf_data, 0xf, dp_index);
	memcpy((void *)dptx_info->rx_caps, (void *)buf_data, 16);

	//DP_DP13_DPCD_REV
	GB02FUNC819(0x02200, buf_data, 0xf, dp_index);
	//DP_EDP_CONFIGURATION_CAP
	GB02FUNC819(0x0000d, buf_data, 0x1, dp_index);
	//DP_DSC_SUPPORT
	GB02FUNC819(0x00060, buf_data, 0x0, dp_index);
	//DP_FEC_CAPABILITY
	GB02FUNC819(0x00090, buf_data, 0x0, dp_index);

	/* 仿真没配？ */
#ifdef FULL_TRAIN
	// Step 6: Program Sink DPCD Link Configuration registers. This step is depend on the read infomation of Step 3 and Step 5 .
	if (is_edp) {
		/* ASSR Enable */
		buf_data[0] = DP_ALTERNATE_SCRAMBLER_RESET_ENABLE;
		GB02FUNC834(DP_EDP_CONFIGURATION_SET, buf_data, 0, dp_index);
	}
#endif

	// Step 7: Initiate link training
	// 7.a Disable FEC.
	/* already disabled */

	// 7.c Follow the steps specified in “Full Link Bringup Sequence,”for initiating link training.
	// 7.c.1 Based on the highest link rate, highest number of lanes supported,
	//		 and Source policy, set PHYIF_CTRL.PHY_LANES, and PHYIF_CTRL.PHY_RATE.
	//		 If Rocket IO PHY is used, set PHYIF_CTRL.SSC_DIS=1.
	rdata = dptx_readl(dptx_info, 0x0a00);
	if (is_edp) {
		//EDP PHY RATE
		rdata &= (~(0x7 << 26));
		rdata |= ((dptx_info->link.rate - GB02MAC2193) << 26);
	} else {
		rdata &= (~(0x3 << 4));
		rdata |= (dptx_info->link.rate << 4);
	}
	// 4 Lanes
	rdata |= (0x1 << 7);
	rdata &= (~(0x1 << 6));

	if (!dptx_info->ssc_en)
		rdata |= (0x01 << 16);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.2 Wait until PHYIF_CTRL.PHY_BUSY is cleared for the desired lanes that have been enabled. For
	//		 example, if 4 lanes are enabled, then wait until PHYIF_CTRL.PHY_BUSY[3:0]=4’h0. If only 1 lane
	//		 enabled, then wait until PHYIF_CTRL.PHY_BUSY[0]=0.
	//PHYIF_CTRL
	loop_times = 0;
	do {
		if (loop_times >= max_loop) {
			gb_printf(KERN_ERR, "%s %d PHY_BUSY err!rdata:0x%x\n",
				__func__, __LINE__, rdata);
			//break;
			return -1;
		}
		rdata = dptx_readl(dptx_info, 0x0a00);
		loop_times++;
		msleep(1);
	} while(((rdata >> 12) & 0xF) != 0);

	//GB02FUNC1525(dptx_info);
	// 7.c.3 Set PHYIF_CTRL.TPS_SEL=0 to force the transmitted pattern to none.
	/* dptx/link.c +312 */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata &= (~0xF);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.4 Program the pre-emphasis and voltage swing values into the PHY_TX_EQ registers corresponding to
	//		 voltage swing level 0 and pre-emphasis level 0 for all enabled lanes.
	/* dptx/link.c +310 */
	GB02FUNC1525(dptx_info);

	// 7.c.5 Set PHYIF_CTRL.TPS_SEL=1 to send TPS1.
	/* dptx/link.c none */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata = (rdata & (~0xF)) | 0x01;
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.6 Enable the transmitter by setting PHYIF_CTRL.XMIT_EN for each enabled lane.
	/* dptx/link.c +318 */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata |= (0x0F << 8);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	/* fast link train no need to cfg for cr&eq done */
#ifdef FULL_TRAIN
	// 7.c.7 Set the following DPCD registers on the Sink – using AUX Channel Interface Registers.
	// 7.c.7.a LINK_BW_SET register (DPCD Address 00100h) to the desired rate.
	//		   06h = 1.62Gbps; 0Ah = 2.7Gbps; 14h = 5.4Gbps; 1Eh = 8.1Gbps; other value for eDP .
	// 7.c.7.b LANE_COUNT_SET register(DPCD Address 00101h): 0x84 = 4 lane, Enhanced_Frame_En.
	/* dptx/link.c +325 */
	if (!is_edp)
		dp_bw = GB02FUNC1125(dptx_info->link.rate);
	buf_data[0] = 0x00008400 | dp_bw;
	GB02FUNC834(0x00100, buf_data, 1, dp_index);

	/* 7.c.7.c DOWNSPREAD_CTRL register (DPCD Address 00107h),
	SPREAD_AMP must be set to 1 for a regular PHY and 0 for Rocket IO.
	7.c.7.d MAIN_LINK_CHANNEL_CODING_SET register (DPCD Address 00108h) to 01h .*/
	/* dptx/link.c +351 */
	if (!dptx_info->ssc_en)
		buf_data[0] = 0x00000100;
	else
		buf_data[0] = 0x00000100 | (1 << 4);
	msleep(sleep_time);
	GB02FUNC834(0x00107, buf_data, 1, dp_index);

	/* asic needs retry for cr done */
	ret = GB02FUNC1550(dptx_info);
	if (ret) {
		/* TODO: CR FAIL RETRY, reduce lane num & lane rate */
	}

	/* asic needs retry for eq done */
	ret = GB02FUNC1553(dptx_info);
	if (ret) {
		/* TODO: EQ FAIL RETRY, reduce lane num & lane rate */
	}
#endif

#ifndef FULL_TRAIN
	// 7.c.9 Set PHYIF_CTRL.TPS_SEL to desired rate. 2:HBR, 3:HBR2, 4:HBR3
	/* dptx/link.c +595 GB02FUNC1553
							GB02FUNC1118*/
	rdata = dptx_readl(dptx_info, 0x0a00);
	switch (dptx_info->link.rate) {
	case GB02MAC2188:
		if (drm_dp_tps4_supported(dptx_info->rx_caps)) {
			pattern = GB02MAC2177;
			dp_pattern = DP_TRAINING_PATTERN_4;
			break;
		}
		fallthrough;
		/* Fall through */
	case GB02MAC2187:
	case GB02MAC2192:
	/* R324用哪个??? */
	case GB02MAC2191:
		if (drm_dp_tps3_supported(dptx_info->rx_caps)) {
			pattern = GB02MAC2176;
			dp_pattern = DP_TRAINING_PATTERN_3;
			break;
		}
		fallthrough;
		/* Fall through */
	case GB02MAC2185:
	case GB02MAC2186:
	case GB02MAC2190:
	case GB02MAC2189:
		pattern = GB02MAC2175;
		dp_pattern = DP_TRAINING_PATTERN_2;
		break;
	default:
		dptx_err(dptx, "Invalid rate %d\n", dptx_info->link.rate);
		return -EINVAL;
	}

	rdata = (rdata & 0xFFFFFFF0) | pattern;
#endif
	// 7.c.12 If all the previous registers bits are 1, then training is complete. Otherwidth, enter loop process .

	// 7.c.13 Set PHYIF_CTRL.TPS_SEL = 0. Set DPCD Sink TRAINING_PATTERN_SET register to 00h.
	/* dptx/link.c +626 GB02FUNC1118(dptx, GB02MAC2173); */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata &=  (~0x0F);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	/* dptx/link.c +628 GB02FUNC1535(dptx,
							DP_TRAINING_PATTERN_DISABLE); */
	buf_data[0] = 0x00000000;
	GB02FUNC834(0x00102, buf_data, 0, dp_index);

#if 1
	/* ===start set video para=== */
	GB02FUNC663(dptx_info, 0);
	// 8.h If Sink supports enhance_frame_cap, set CCTL.Enhance_framing_en
	/* dptx/link.c +329 */
	rdata = dptx_readl(dptx_info, 0x0200);
	//Enhance_framing_en
	rdata |= 0x2;
	rdata |= 0x2a;
	dptx_writel(dptx_info, 0x0200, rdata);

	// 8.i ~ 8.m for MST mode
	// 8.n If Audio and encrypt is not support, then enable video after step 11 .
	rdata = dptx_readl(dptx_info, 0x0300);
	/* 仿真没配？ */
#ifdef FULL_TRAIN
	//Encryption enable
	if (is_edp)
		rdata |= (1 << 24);
#endif
	// VSAMPLE_CTRL[0].VIDEO_STREAM_ENABLE
	rdata |= 0x20;
	dptx_writel(dptx_info, 0x0300, rdata);
	/* ===end set video para=== */
#endif

#if 1
	/* ===start set audio para=== */
	// Step 9: Configure the controller for Audio Mode.
	rdata = dptx_readl(dptx_info, 0x0400);
	/* zfl:AUD_CONFIG1,配置audio为I2S输入bit[0]、使能8个声道bit[4:1]、
	对输入数据24bit采样bit[9:5]、ctl为192kHz x 8 x 16 bits in 8-channel bit[10]
	---TODO:使能的channel帕拉丁上需根据audio实际声道数进行配置 */
	rdata &= 0xfffff800;	//8 channels
	{	//two channels
	rdata &= 0xffff8fff;

	rdata |= 0x1000;
	}
	rdata |= 0x0 << 0;
	rdata |= 0xf << 1;

	//rdata |= 0x18 << 5;	//24bit
	rdata |= 0x10 << 5; 	//16bit
	rdata |= 0x0 << 10;
	dptx_writel(dptx_info, 0x0400, rdata);

	rdata = dptx_readl(dptx_info, 0x0500);
	rdata &= 0xfffffffc;
	rdata |= 0x1 << 0;
	rdata |= 0x1 << 1;
	/* SDP_VERTICAL_CTRL */
	dptx_writel(dptx_info, 0x0500, rdata);

	rdata = dptx_readl(dptx_info, 0x0504);
	rdata &= 0xfffffffc;
	rdata |= 0x1 << 0;
	rdata |= 0x1 << 1;
	/* SDP_HORIZONTAL_CTRL */
	dptx_writel(dptx_info, 0x0504, rdata);
	/* ===end set audio para=== */
#endif

//	base_addr = reg_bar_addr + GB02MAC1062(dp_index) + GB02MAC1575;
//	GB02FUNC825(base_addr + 0x7038, 0x01);

	/* zfl:ana:pwr_en，reserved */
	kfree(dptx_info);
	return ret;
}

void GB02FUNC977(struct dptx *dptx)
{
	u32 reg = 0;
	struct GB02STR122 *aparams;

	aparams = &dptx->aparams;

	/* AG_CONFIG1 */
	reg = dptx_readl(dptx, GB02MAC2097);
	reg &= ~(GB02MAC2255 | (0x01 << 11));
	reg |= aparams->use_lut << GB02MAC2254;
	dptx_writel(dptx, GB02MAC2097, reg);
	GB02FUNC475(dptx);

	/* AG_CONFIG3 */
	reg = dptx_readl(dptx, GB02MAC2099);
	reg &= ~DPTX_AG_CONFIG3_CH_NUMCL0_MASK;
	reg |= aparams->iec_channel_numcl0 << GB02MAC2256;
	reg &= ~DPTX_AG_CONFIG3_CH_NUMCR0_MASK;
	reg |= aparams->iec_channel_numcr0  << GB02MAC2257;
	dptx_writel(dptx, GB02MAC2099, reg);

	/* AG_CONFIG4 */
	GB02FUNC483(dptx);
	reg = dptx_readl(dptx, GB02MAC2100);
	reg &= ~DPTX_AG_CONFIG4_WORD_LENGTH_MASK;
	reg |= aparams->iec_word_length  << GB02MAC2259;
	dptx_writel(dptx, GB02MAC2100, reg);

	/* AG_CONFIG5 */
	reg = dptx_readl(dptx, GB02MAC2101);
	reg &= ~DPTX_AG_CONFIG3_CH_NUMCL0_MASK;
	/*GB02MAC2256;*/
	reg |= aparams->iec_channel_numcl0 << 0;
	reg &= ~DPTX_AG_CONFIG3_CH_NUMCR0_MASK;
	/*GB02MAC2257;*/
	reg |= aparams->iec_channel_numcr0 << 4;
	dptx_writel(dptx, GB02MAC2101, reg);
}

void GB02FUNC979(struct GB02STR122 *params)
{
	gb_printf(KERN_INFO, "%s-%d: dp audio params start!\n", __func__, __LINE__);
	gb_printf(KERN_INFO, "params->iec_channel_numcl0 = %d\n",
		params->iec_channel_numcl0);
	gb_printf(KERN_INFO, "params->iec_channel_numcr0 = %d\n",
		params->iec_channel_numcr0);
	gb_printf(KERN_INFO, "params->use_lut = %d\n", params->use_lut);
	gb_printf(KERN_INFO, "params->iec_samp_freq = %d\n", params->iec_samp_freq);
	gb_printf(KERN_INFO, "params->iec_word_length = %d\n", params->iec_word_length);
	gb_printf(KERN_INFO, "params->iec_orig_samp_freq = %d\n",
		params->iec_orig_samp_freq);
	gb_printf(KERN_INFO, "params->data_width = %d\n", params->data_width);
	gb_printf(KERN_INFO, "params->num_channels = %d\n", params->num_channels);
	gb_printf(KERN_INFO, "params->inf_type = %d\n", params->inf_type);
	gb_printf(KERN_INFO, "params->ats_ver = %d\n", params->ats_ver);
	gb_printf(KERN_INFO, "params->mute = %d\n", params->mute);
	gb_printf(KERN_INFO, "%s-%d: dp audio params finish!\n", __func__, __LINE__);
}
int GB02FUNC982(const struct drm_display_mode *mode,
	int dp_index)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];
	int ret;
	uint32_t rdata = 0;
	u8 edp_ver = 0;

	dptx_link_wait(dptx_info, dptx_info->link.trained, 5000);
	ret = GB02FUNC889(dptx_info->base + (GB02MAC1570 - GB02MAC1564),
		mode->clock);
	if (ret) {
		dptx_err(dptx_info, "%s %d GB02FUNC889 err! ret:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}

	GB02FUNC848(dptx_info, mode);
#if 1
	ret = GB02FUNC682(dptx_info, dptx_info->link.lanes,
		dptx_info->link.rate, dptx_info->vparams.bpc,
		dptx_info->vparams.pix_enc,
		dptx_info->vparams.mdtd.pixel_clock);
	if (ret) {
		dptx_err(dptx_info, "%s %d GB02FUNC682 err! ret:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}
#endif
	/* ===start set video para=== */
//	GB02FUNC454(dptx_info, 1, 0);
//	GB02FUNC454(dptx_info, 0, 0);
	GB02FUNC788(dptx_info, 0);
	GB02FUNC663(dptx_info, 0);
	GB02FUNC699(dptx_info, 0);
	GB02FUNC789(dptx_info, 0);
	dptx_writel(dptx_info, GB02MAC2089(0), 0);
	// 8.h If Sink supports enhance_frame_cap, set CCTL.Enhance_framing_en
	/* dptx/link.c +329 */
	rdata = dptx_readl(dptx_info, 0x0200);
	//Enhance_framing_en
	if (drm_dp_enhanced_frame_cap(dptx_info->rx_caps))
		rdata |= GB02MAC2157;
	else
		rdata &= ~GB02MAC2157;
#ifdef GB02_PAL_DP
	/* paladium should disable sdp interleave */
	rdata |= (1 << 5);
#endif
	dptx_writel(dptx_info, 0x0200, rdata);

	// 8.i ~ 8.m for MST mode
	/*
	 * 8.n If Audio and encrypt is not support,
	 * then enable video after step 11
	 */
	rdata = dptx_readl(dptx_info, 0x0300);
	GB02FUNC1151(dptx_info, 0x0700, &edp_ver);
	dptx_dbg(dptx_info,"eDP version is %d\n",edp_ver);

	//Encryption enable
	//when eDP version is 1.2 edp_ver is 1 eDP version is 1.4 edp_ver is 4 else is 0
	if (dptx_info->is_edp && edp_ver >= 4)
		rdata |= (1 << 24);
	// VSAMPLE_CTRL[0].VIDEO_STREAM_ENABLE
	rdata |= 0x20;
	dptx_writel(dptx_info, 0x0300, rdata);
#if 0
	/* ===start set audio para=== */
	// Step 9: Configure the controller for Audio Mode.
	rdata = dptx_readl(dptx_info, 0x0400);
	/* zfl:AUD_CONFIG1,配置audio为I2S输入bit[0]
	 * 使能8个声道bit[4:1]、
	 * 对输入数据24bit采样bit[9:5]、
	 * ctl为192kHz x 8 x 16 bits in 8-channel bit[10]
	 * ---TODO:使能的channel帕拉丁上
	 * 需根据audio实际声道数进行配置
	 */
	rdata &= 0xfffff800;    //8 channels
	{// two channels
	rdata &= 0xffff8fff;

	rdata |= 0x1000; // 2 channels
	}
	rdata |= 0x0 << 0;
	rdata |= 0xf << 1;

	//rdata |= 0x18 << 5;   //24bit
	rdata |= 0x10 << 5;     //16bit
	rdata |= 0x0 << 10;
	dptx_writel(dptx_info, 0x0400, rdata);

	rdata = dptx_readl(dptx_info, 0x0500);
	rdata &= 0xfffffffc;
	rdata |= 0x1 << 0;
	rdata |= 0x1 << 1;
	/* SDP_VERTICAL_CTRL */
	dptx_writel(dptx_info, 0x0500, rdata);

	rdata = dptx_readl(dptx_info, 0x0504);
	rdata &= 0xfffffffc;
	rdata |= 0x1 << 0;
	rdata |= 0x1 << 1;
	/* SDP_HORIZONTAL_CTRL */
	dptx_writel(dptx_info, 0x0504, rdata);
	/* ===end set audio para=== */
	GB02FUNC979(&dptx_info->aparams);
	//GB02FUNC977(dptx_info);
	GB02FUNC399(dptx_info);
	GB02FUNC402(dptx_info);
	GB02FUNC404(dptx_info);
#endif
	gb_printf(KERN_INFO, "%s-%d: finish!\n", __func__, __LINE__);
	return ret;
}

int GB02FUNC988(struct dptx *dptx_info)
{
	uint32_t buf_data[GB02MAC1141 / sizeof(uint32_t)] = {0};
	uint32_t rdata = 0;
	u64 max_loop = 10;
	int loop_times = 0;
	int dp_bw = 0;
	int ret = 0;
	/* asic sleep time less */
#ifdef GB02_PAL_DP
	int sleep_time = 10000;
#else
	int sleep_time = 150;
#endif
	int retry_eq_time = 0, retry_cr_time = 0, retrain_time = 0;
	uint32_t dp_index = dptx_info->index;
	bool is_edp = dptx_info->is_edp;

retrain:
	GB02FUNC819(0x00000, buf_data, 0xf, dp_index);
	//GB02FUNC1151(dptx, 0x00000, buf_data);
	memcpy((void *)dptx_info->rx_caps, (void *)buf_data, 16);
	if (buf_data[0] == 0 && buf_data[1] == 0 &&
		buf_data[2] == 0 && buf_data[3] == 0) {
		gb_printf(KERN_ERR, "get monitor info failed! return!\n");
		return -1;
	}

	//DP_DP13_DPCD_REV
	GB02FUNC819(0x02200, buf_data, 0xf, dp_index);
	//DP_EDP_CONFIGURATION_CAP
	GB02FUNC819(0x0000d, buf_data, 0x1, dp_index);
	//DP_DSC_SUPPORT
	GB02FUNC819(0x00060, buf_data, 0x0, dp_index);
	//DP_FEC_CAPABILITY
	GB02FUNC819(0x00090, buf_data, 0x0, dp_index);

#ifndef GB02_PAL_DP
	dptx_info->link.lanes = (min(dptx_info->max_lanes,
		drm_dp_max_lane_count(dptx_info->rx_caps)));
	dptx_info->link.rate = (min(dptx_info->max_rate,
		GB02FUNC1126(dptx_info->rx_caps[DP_MAX_LINK_RATE])));
#endif
	dptx_info(dptx_info, "dptx lanes modify :%u, rate:%u\n",
			dptx_info->link.lanes,
			dptx_info->link.rate);
	// Step 6: Program Sink DPCD Link Configuration registers.
	// This step is depend on the read infomation of Step 3 and Step 5
	if (is_edp) {
		/* ASSR Enable */
		buf_data[0] = DP_ALTERNATE_SCRAMBLER_RESET_ENABLE;
		GB02FUNC834(DP_EDP_CONFIGURATION_SET, buf_data, 0, dp_index);
	}

	// Step 7: Initiate link training
	// 7.a Disable FEC.
	rdata = dptx_readl(dptx_info, 0x0200);
	rdata &= (~(0x01<<26));
	//CCTL
	dptx_writel(dptx_info, 0x0200, rdata);
	// 7.b Disable the fast link training if enabled. Fast link training is enabled by default.
	/* already disabled */

	// 7.c Follow the steps specified in “Full Link Bringup Sequence,
	// ”for initiating link training.
	// 7.c.1 Based on the highest link rate,
	// highest number of lanes supported,
	// and Source policy, set PHYIF_CTRL.PHY_LANES,
	// and PHYIF_CTRL.PHY_RATE.
	// If Rocket IO PHY is used, set PHYIF_CTRL.SSC_DIS=1.
	rdata = dptx_readl(dptx_info, 0x0a00);
	if (is_edp) {
		//EDP PHY RATE
		rdata &= (~(0x7 << 26));
		rdata |= ((dptx_info->link.rate - GB02MAC2193) << 26);
	} else {
		rdata &= (~(0x3 << 4));
		rdata |= (dptx_info->link.rate << 4);
	}
	// 4 Lanes
	rdata &= (~(0x3 << 6));
	rdata |= ((dptx_info->link.lanes >> 1) << 6);
	//rdata |= (2 << 6);
	if (!dptx_info->ssc_en)
		rdata |= (0x01 << 16);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.2 Wait until PHYIF_CTRL.PHY_BUSY is cleared for
	// the desired lanes that have been enabled. For
	// example, if 4 lanes are enabled, then wait
	// until PHYIF_CTRL.PHY_BUSY[3:0]=4’h0. If only 1 lane
	// enabled, then wait until PHYIF_CTRL.PHY_BUSY[0]=0.
	//PHYIF_CTRL
	loop_times = 0;
	do {
		if (loop_times >= max_loop) {
			gb_printf(KERN_ERR, "%s %d PHY_BUSY err!rdata:0x%x\n",
				__func__, __LINE__, rdata);
			//break;
			return -1;
		}
		rdata = dptx_readl(dptx_info, 0x0a00);
		loop_times++;
		msleep(1);
	} while (((rdata >> 12) & 0xF) != 0);

	// 7.c.3 Set PHYIF_CTRL.TPS_SEL=0 to force
	// the transmitted pattern to none.
	/* dptx/link.c +312 */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata &= (~0xF);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.4 Program the pre-emphasis and voltage swing
	// values into the PHY_TX_EQ registers corresponding to
	// voltage swing level 0 and pre-emphasis level 0
	// for all enabled lanes.
	/* dptx/link.c +310 */
	GB02FUNC1525(dptx_info);

	// 7.c.5 Set PHYIF_CTRL.TPS_SEL=1 to send TPS1.
	/* dptx/link.c none */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata = (rdata & (~0xF)) | 0x01;
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.6 Enable the transmitter by setting PHYIF_CTRL.XMIT_EN
	// for each enabled lane.
	/* dptx/link.c +318 */
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata &= (~(0xf << 8));
	if (dptx_info->link.lanes == 4)
		rdata |= (0x0F << 8);
	else if (dptx_info->link.lanes == 2)
		rdata |= (0x03 << 8);
	else if (dptx_info->link.lanes == 1)
		rdata |= (0x01 << 8);
	else {
		gb_printf(KERN_ERR, "dp lanes err!lane cnt:%u\n",
			dptx_info->link.lanes);
		return -1;
	}
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	// 7.c.7 Set the following DPCD registers on the Sink
	// – using AUX Channel Interface Registers.
	// 7.c.7.a LINK_BW_SET register (DPCD Address 00100h)
	// to the desired rate.
	// 06h = 1.62Gbps; 0Ah = 2.7Gbps; 14h = 5.4Gbps; 1Eh = 8.1Gbps;
	// other value for eDP .
	// 7.c.7.b LANE_COUNT_SET register(DPCD Address 00101h):
	// 0x84 = 4 lane, Enhanced_Frame_En.

	// 7.c.7.f TRAINING_LANEx_SET register
	// (DPCD Addresses 00103h through 00106h) for each lane to
	// voltage swing level 0 and pre-emphasis level 0 .
	// 7.c.7.g Note that TRAINING_PATTERN_SET and TRAINING_LANEx_SET
	// registers must be written in one AUX CH burst write transaction.
	/* zfl:第三个参数+1是真正写入/读取的字节数 */
	/* dptx/link.c +325 */
	if (!is_edp)
		dp_bw = GB02FUNC1125(dptx_info->link.rate);
	buf_data[0] = dp_bw;
	GB02FUNC834(0x00100, buf_data, 0, dp_index);
	buf_data[0] = dptx_info->link.lanes;
	if (drm_dp_enhanced_frame_cap(dptx_info->rx_caps))
		buf_data[0] |= DP_ENHANCED_FRAME_CAP;
	GB02FUNC834(0x00101, buf_data, 0, dp_index);

	/* 7.c.7.c DOWNSPREAD_CTRL register (DPCD Address 00107h),
	 * SPREAD_AMP must be set to 1 for a regular PHY and 0 for Rocket IO.
	 * 7.c.7.d MAIN_LINK_CHANNEL_CODING_SET register
	 * (DPCD Address 00108h) to 01h
	 * dptx/link.c +351
	 */
	if (dptx_info->ssc_en)
		buf_data[0] = 1 << 4;
	else
		buf_data[0] = 0;
	GB02FUNC834(0x00107, buf_data, 0, dp_index);
	msleep(sleep_time);
	buf_data[0] = 0x000001;
	GB02FUNC834(0x00108, buf_data, 0, dp_index);
	msleep(10);
retry_cr:
	ret = GB02FUNC1550(dptx_info);
	if (ret) {
		/* TODO: CR FAIL RETRY, reduce lane num & lane rate */
		if (retry_cr_time < 50) {
			retry_cr_time++;
			goto retry_cr;
		} else {
			gb_printf(KERN_ERR, "%s %d CR Fail!\n", __func__, __LINE__);
			retrain_time++;
			if (retrain_time < 5)
				goto retrain;
			return -1;
		}
	} else
		dptx_info(dptx_info, "%s %d CR Success!\n",
			__func__, __LINE__);

	/* asic needs retry for eq done */
	ret = GB02FUNC1553(dptx_info);
	if (ret) {
		/* TODO: EQ FAIL RETRY, reduce lane num & lane rate */
		if (retry_eq_time < 50) {
			retry_eq_time++;
			retry_cr_time = 0;
			goto retry_cr;
		} else {
			gb_printf(KERN_ERR, "%s %d EQ Fail!\n", __func__, __LINE__);
			retrain_time++;
			if (retrain_time < 5)
				goto retrain;
			return -1;
		}
	} else {
		dptx_info(dptx_info, "%s %d EQ Success!\n",
			__func__, __LINE__);
#ifdef	GB02MAC292
		GB02FUNC556(dptx_info->index, AUDIO_NEED_RESTART);
#endif
	}

	// 7.c.12 If all the previous registers bits are 1,
	// then training is complete. Otherwidth, enter loop process .

	// 7.c.13 Set PHYIF_CTRL.TPS_SEL = 0.
	// Set DPCD Sink TRAINING_PATTERN_SET register to 00h.
	// dptx/link.c +626 GB02FUNC1118(dptx,
	// GB02MAC2173);
	rdata = dptx_readl(dptx_info, 0x0a00);
	rdata &=  (~0x0F);
	//PHYIF_CTRL
	dptx_writel(dptx_info, 0x0a00, rdata);

	/* dptx/link.c +628 GB02FUNC1535(dptx,
	 * DP_TRAINING_PATTERN_DISABLE);
	 */
	buf_data[0] = 0x00000000;
	GB02FUNC834(0x00102, buf_data, 0, dp_index);

	return ret;
}

int GB02FUNC1001(int dp_index,
	struct drm_device *drm)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info = pcie_info->dptx[dp_index];

	dptx_info->drm_dev = drm;
	return 0;
//	return GB02FUNC1079(dptx_info, dptx_info->is_edp);
}
int GB02FUNC1005(int dp_index,
	struct GB02STR70 *pcie_info, bool is_edp)
{
	/**
	* DCDP CONFIG
	*/
	void __iomem *base_addr = NULL;
	int ret = 0;
	u8 edp_ver = 0;
	struct dptx *dptx_info = NULL;
	void __iomem *reg_bar_addr =
		pcie_info->pci_bars[GB02FUNC465(pcie_info->GB02STR153)].mmio;

	dptx_info = devm_kzalloc(&pcie_info->pdev->dev,
		sizeof(struct dptx), GFP_KERNEL);
	if (!dptx_info) {
		gb_printf(KERN_ERR, "%s %d alloc dptx_info fail!\n",
			__func__, __LINE__);
		return -1;
	}

	/* ===sw para init=== */
	gb_dp_reg_bar_base = reg_bar_addr;
	dptx_info->dev = &pcie_info->pdev->dev;
	dptx_info->index = dp_index;
	dptx_info->base = reg_bar_addr + GB02MAC1062(dp_index) + GB02MAC1564;
	GB02FUNC1151(dptx_info, 0x0700, &edp_ver);
	//if(edp_ver > 0)
	//	is_edp = true;
	(void)GB02FUNC852(dptx_info, pcie_info, NULL, is_edp);
	/* ===sw para init end=== */
	/* ===DCDP_CFG init:GB02MAC1570 云盘文档cfg_reg.xml=== */
	base_addr = reg_bar_addr + GB02MAC1062(dp_index) + GB02MAC1570;
#if 0
	/* zfl:ana:pwr_en，reserved */
	GB02FUNC825(base_addr + 0x08, 0x01);
	/* zfl:upcs 0:PCS pwr_en reserved  1:PCS pwr_stable reserved*/
	GB02FUNC825(base_addr + 0x0C, 0x03);
	/* zfl:upcs bit0:raw PCS pwr_en reserved
	bit1:raw PCS pwr_stable reserved */
	GB02FUNC825(base_addr + 0x10, 0x03);
	/* zfl:pma 0:pwr_en reserved  1:pwr_stable reserved*/
	GB02FUNC825(base_addr + 0x14, 0x03);
	/* zfl:bit0:配置为0用PLLD作为链路参考时钟，配置为1用外部差分100Mhz时钟 */
	GB02FUNC825(base_addr + 0x18, 0x30);
	/* zfl:aux block enable */
	GB02FUNC825(base_addr + 0x20, 0x01);
	/* ===DCDP_CFG init end=== */
	/* audio params init */
	GB02FUNC792(&dptx_info->aparams);

#ifdef CONFIG_X86_64
	/* default cfg 1080P pixclock */
	ret = GB02FUNC900(base_addr, 148500);
	if (ret) {
		dptx_err(dptx_info, "%s %d GB02FUNC900 err! ret:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}
#endif
	/* disable all interrupt */
	GB02FUNC1057(dptx_info);
#endif
	dptx_info->is_edp = is_edp;
	pcie_info->dptx[dp_index] = dptx_info;
	/* PCIE_LPDDR4 only has DP2 & DP3 */
	if ((GB02FUNC503(pcie_info) == PCIE_LPDDR4 ||
		(GB02FUNC503(pcie_info) == PCIE_C0_200))
		&& (dp_index != 2 && dp_index != 3))
		goto out;
	/* PCIE_FULL_LPDDR4 DP1(lvds) have no use*/
	if ((GB02FUNC503(pcie_info) == PCIE_FULL_LPDDR4)
		&& (dp_index == 1))
		goto out;
	/* PCIE_HIE1LP4_LPDDR4 DP-1-4) have no use*/
	if ((GB02FUNC503(pcie_info) == PCIE_HIE1LP4_LPDDR4)
		&& ((dp_index == 1) || (dp_index == 4))) {
		GB02FUNC871(dp_index);
		goto out;
	}
	/* PCIE_M4HL8G_LPDDR4 DP-1-4) have no use*/
	if ((GB02FUNC503(pcie_info) == PCIE_M4HL8G_LPDDR4)
		&& ((dp_index == 1) || (dp_index == 4))) {
		GB02FUNC871(dp_index);
		goto out;
	}
	INIT_DELAYED_WORK(&dptx_info->hotplug_work, GB02FUNC1505);
	init_waitqueue_head(&dptx_info->waitq);
#if 1
	ret = GB02FUNC1079(dptx_info, dptx_info->is_edp);
#else
	GB02FUNC1055(dptx_info);
#endif
out:
	if (ret)
		dptx_err(dptx_info, "===%s GB02FUNC1079 err!id:%d, is_edp:%d===\n",
			__func__, dp_index, is_edp);
	GB02FUNC461(dptx_info);
	GB02FUNC496(dptx_info, 0);
	return ret;
}

void GB02FUNC1013(struct dptx *dptx_info)
{
	uint32_t rdata = 0;

	/* ===start set audio para=== */
	// Step 9: Configure the controller for Audio Mode.
	rdata = dptx_readl(dptx_info, 0x0400);
	/* zfl:AUD_CONFIG1,配置audio为I2S输入bit[0]
	 * 使能8个声道bit[4:1]、
	 * 对输入数据24bit采样bit[9:5]、
	 * ctl为192kHz x 8 x 16 bits in 8-channel bit[10]
	 * ---TODO:使能的channel帕拉丁上
	 * 需根据audio实际声道数进行配置
	 */
	rdata &= 0xfffff800;    //8 channels
	{// two channels
	rdata &= 0xffff8fff;

	rdata |= 0x1000; // 2 channels
	}
	rdata |= 0x0 << 0;
	rdata |= 0xf << 1;

	//rdata |= 0x18 << 5;   //24bit
	rdata |= 0x10 << 5;     //16bit
	rdata |= 0x0 << 10;
	dptx_writel(dptx_info, 0x0400, rdata);

	rdata = dptx_readl(dptx_info, 0x0500);
	rdata &= 0xfffffffc;
	rdata |= 0x1 << 0;
	rdata |= 0x1 << 1;
	/* SDP_VERTICAL_CTRL */
	dptx_writel(dptx_info, 0x0500, rdata);

	rdata = dptx_readl(dptx_info, 0x0504);
	rdata &= 0xfffffffc;
	rdata |= 0x1 << 0;
	rdata |= 0x1 << 1;
	/* SDP_HORIZONTAL_CTRL */
	dptx_writel(dptx_info, 0x0504, rdata);
	/* ===end set audio para=== */
}
int GB02FUNC1015(int dp_index,
	struct GB02STR70 *pcie_info, bool is_edp)
{
	void __iomem *base_addr = NULL;
	struct dptx *dptx_info = NULL;
	//tDCDP_PLLD_CFG0 plld_cfg0 = {0};
	void __iomem *reg_bar_addr = pcie_info->pci_bars
		[GB02FUNC465(pcie_info->GB02STR153)].mmio;
	dptx_info = pcie_info->dptx[dp_index];
	/* ===sw para init=== */
	gb_dp_reg_bar_base = reg_bar_addr;
	gb_printf(KERN_INFO, "reg_bar_addr = 0x%p\n", reg_bar_addr);
	/* ===DCDP_CFG init:GB02MAC1570 云盘文档cfg_reg.xml=== */
	base_addr = reg_bar_addr + GB02MAC1062(dp_index)
			+ GB02MAC1570;
	/*pll disable*/
	GB02FUNC871(dp_index);
	/*
	plld_cfg0.data.en = 0;
	GB02FUNC825(base_addr + 0x0050, plld_cfg0.val);
	*/
	/* disable all interrupt */
	GB02FUNC1057(dptx_info);
	return 0;
}


int GB02FUNC1016(int dp_index,
	struct GB02STR70 *pcie_info, bool is_edp)
{
	/**
	* DCDP CONFIG
	*/
	void __iomem *base_addr = NULL;
	int ret = 0;
	struct dptx *dptx_info = NULL;
	void __iomem *reg_bar_addr =
		pcie_info->pci_bars[GB02FUNC465(pcie_info->GB02STR153)].mmio;

	dptx_info = pcie_info->dptx[dp_index];
	/* ===sw para init=== */
	gb_dp_reg_bar_base = reg_bar_addr;
	gb_printf(KERN_INFO, "reg_bar_addr = 0x%p\n", reg_bar_addr);
	/* ===DCDP_CFG init:GB02MAC1570 云盘文档cfg_reg.xml=== */
	base_addr = reg_bar_addr + GB02MAC1062(dp_index) + GB02MAC1570;
#if 1
	/* zfl:ana:pwr_en，reserved */
	GB02FUNC825(base_addr + 0x08, 0x01);
	/* zfl:upcs 0:PCS pwr_en reserved  1:PCS pwr_stable reserved*/
	GB02FUNC825(base_addr + 0x0C, 0x03);
	/* zfl:upcs bit0:raw PCS pwr_en reserved
	bit1:raw PCS pwr_stable reserved */
	GB02FUNC825(base_addr + 0x10, 0x03);
	/* zfl:pma 0:pwr_en reserved  1:pwr_stable reserved*/
	GB02FUNC825(base_addr + 0x14, 0x03);
	/* zfl:bit0:配置为0用PLLD作为链路参考时钟，配置为1用外部差分100Mhz时钟 */
	GB02FUNC825(base_addr + 0x18, 0x30);
	/* zfl:aux block enable */
	GB02FUNC825(base_addr + 0x20, 0x01);
	/* ===DCDP_CFG init end=== */
	/* default cfg 1080P pixclock */
	ret = GB02FUNC900(base_addr, 148500);
	if (ret) {
		dptx_err(dptx_info, "%s %d GB02FUNC900 err! ret:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}
	/* disable all interrupt */
	GB02FUNC1057(dptx_info);
	if (GB02FUNC503(pcie_info) == PCIE_HIE1LP4_LPDDR4
		&& ((dp_index == 1) || (dp_index == 4))) {
		/*pll disable*/
		GB02FUNC871(dp_index);
		return ret;
	}
	if (GB02FUNC503(pcie_info) == PCIE_M4HL8G_LPDDR4
		&& ((dp_index == 1) || (dp_index == 4))) {
		/*pll disable*/
		GB02FUNC871(dp_index);
		return ret;
	}

	ret = GB02FUNC1079(dptx_info, dptx_info->is_edp);
	if (ret)
		dptx_err(dptx_info, "===%s GB02FUNC1079 err!id:%d, is_edp:%d===\n",
			__func__, dp_index, is_edp);
	else
		dptx_info(dptx_info, "===%s OK!id:%d, is_edp:%d===\n",
			__func__, dp_index, is_edp);
	/* Enable all top-level interrupts */
//	GB02FUNC1055(dptx_info);
#endif
	GB02FUNC461(dptx_info);
	GB02FUNC496(dptx_info, 0);
	GB02FUNC1013(dptx_info);

	gb_printf(KERN_INFO, "this ret = %d\n", ret);
	return ret;
}

int GB02FUNC1024(struct dptx *dptx, struct drm_dp_aux_msg *aux_msg,
			int read_cnt)
{
	unsigned int addr;
	u8 req;
	char *buf;
	int len;
	u32 hpdsts;
	int i;
	u8 bytes[1];
	bool rw = false;
	bool i2c = false;
	bool mot = false;
	bool addr_only = false;
	int result = 0;
	struct GB02STR12 *GB02STR154;

	hpdsts = dptx_readl(dptx, GB02MAC2128);

	if (!(hpdsts & GB02MAC2245)) {
		dptx_dbg(dptx, "%s: Not connected\n", __func__);
		return -ENODEV;
	}

	addr = aux_msg->address;
	req = aux_msg->request;
	buf = aux_msg->buffer;
	len = aux_msg->size;

	/* sync edp brightness to pwm backlight_device */
	if (dptx->is_edp && (len > 6) && (buf[0] == 0x51)) {
		GB02STR154 = GB02FUNC2(dptx->index);
		if (GB02STR154 && GB02STR154->bl) {
			GB02STR154->by_dptx = true;
			result = backlight_device_set_brightness(GB02STR154->bl, buf[5]);
			return result;
		}
	}

	if (req & GB02MAC1592)
		rw = true;
	if (req & DP_AUX_I2C_MOT)
		mot = true;
	if (req <= 7)
		i2c = true;
	if (len == 0)
		addr_only = true;

	if (addr_only) {
		result = GB02FUNC340(dptx, rw, i2c, mot,
				addr_only, addr, &bytes[0], 1);
	} else {
		if (rw) {
			for (i = 0; i < read_cnt; i++) {
				result =  GB02FUNC340(dptx, rw, i2c, mot,
						addr_only, addr, buf, len);
			}
		} else {
			result =  GB02FUNC340(dptx, rw, i2c, mot,
					addr_only, addr, buf, len);
		}
	}

	return result;
}

ssize_t GB02FUNC1030(struct drm_dp_aux *aux,
                                  struct drm_dp_aux_msg *msg)
{
	ssize_t result = 0;
	int dp_index;
	struct gbdc_connector *gbdc_conn;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct dptx *dptx_info;
	enum gb_board_type gb_type;
	int read_cnt = 1;
	struct gbdc_connector_state *gbdc_connector_state;
	struct gbdc_connector *gbdc_connector;

	if (msg->size > DP_AUX_MAX_PAYLOAD_BYTES) {
		dptx_err(dptx, "msg->size > DP_AUX_MAX_PAYLOAD_BYTES\n");
		return -1;
	}

	gbdc_conn = container_of(aux, struct gbdc_connector, aux);

	if (gbdc_conn->virt) {
		gbdc_connector_state = to_gbdc_connector_state(gbdc_conn->base.state);
		if (!gbdc_connector_state) {
			gb_printf(KERN_ERR, "%s:gbdc_connector_state null\n", __func__);
			return -1;
		}
		gbdc_for_each_gbdc_obj(connector) {
			if (!gbdc_connector)
				continue;
			dp_index = gbdc_connector->connector_id;
			dptx_info = pcie_info->dptx[dp_index];
			gb_type = GB02FUNC503(pcie_info);
			if (GB02FUNC1024(dptx_info, msg, read_cnt) == 0)
				result = msg->size;
		}

		return result;
	}

	dp_index = gbdc_conn->connector_id;
	dptx_info = pcie_info->dptx[dp_index];
	gb_type = GB02FUNC503(pcie_info);
#if 0
	/*HMDI reading 16 byte data through aux needs to be repeated twice
	VGA repeat 3 times*/
	if (gb_type == PCIE_LPDDR4) {
		if (dp_index == 2)
			read_cnt = 2;
		else if  (dp_index == 3)
			read_cnt = 3;
	} else if (gb_type == PCIE_FULL_LPDDR4) {
		if (dp_index == 5)
			read_cnt = 2;
		else if  (dp_index  == 4)
			read_cnt = 3;
	} else if (gb_type == PCIE_M6FL8G_LPDDR4) {
		if ((dp_index != 0) && (dp_index != 2))
			read_cnt = 2;
	} else if (gb_type == PCIE_HIE1LP4_LPDDR4) {
		if ((dp_index != 1) && (dp_index != 4))
			read_cnt = 2;
	}
#endif
	if (0 == GB02FUNC1024(dptx_info, msg, read_cnt))
		result = msg->size;

	return result;
}

int GB02FUNC1041(struct GB02STR70 *gb_pcie)
{
	enum gb_board_type type;
	int hdmi_index = -1;
	int vga_index = -1;
	uint32_t buf_data[GB02MAC1141 / sizeof(uint32_t)] = {0};

        if (gb_pcie == NULL) {
		gb_printf(KERN_ERR, "%s %d gb_pcie = NULL!\n",__func__, __LINE__);
		return -1;
	}

	type = GB02FUNC503(gb_pcie);
	switch (type) {
	case PCIE_C0_200:
	case GB02MAC1363:
		hdmi_index = 2;
		vga_index = 3;
		break;
        case PCIE_FULL_LPDDR4:
		hdmi_index = 5;
		vga_index = 4;
		break;
	case PCIE_M6FL8G_LPDDR4:
		if (GB02FUNC1500(1)) {
			hdmi_index = 1;
		} else if (GB02FUNC1500(3)) {
			hdmi_index = 3;
		} else if (GB02FUNC1500(4)) {
			hdmi_index = 4;
		} else {
			hdmi_index = 5;
		}
		break;
	case PCIE_HIE1LP4_LPDDR4:
	case PCIE_M4HL8G_LPDDR4:
		if (GB02FUNC1500(0)) {
			hdmi_index = 0;
		} else if (GB02FUNC1500(2)) {
			hdmi_index = 2;
		} else if (GB02FUNC1500(3)) {
			hdmi_index = 3;
		} else {
			hdmi_index = 5;
                }
		break;
	default:
		break;
	}

	if (hdmi_index == -1 &&  vga_index == -1) {
		return 0;
	}

	if (hdmi_index >= 0) {
		if (!GB02FUNC1500(hdmi_index))
			hdmi_index = -1;
	}

	if (vga_index >= 0) {
		if (!GB02FUNC1500(vga_index))
			vga_index = -1;
	}

	if (!(gb_pcie->f_info.version_info & GB02MAC1595)) {
		if (hdmi_index >= 0) {
			GB02FUNC819(GB02MAC1593, buf_data, 0xf, hdmi_index);
			gb_pcie->f_info.hdmi_firmware_maj = (buf_data[0] & 0xff) >> 1;
			GB02FUNC819(GB02MAC1594, buf_data, 0xf, hdmi_index);
			gb_pcie->f_info.hdmi_firmware_min = (buf_data[0] & 0xff) >> 1;
			gb_pcie->f_info.version_info |= GB02MAC1595;
		}
        }

	if (!(gb_pcie->f_info.version_info & GB02MAC1596)) {
		if (vga_index >= 0) {
			GB02FUNC819(GB02MAC1593, buf_data, 0x2, vga_index);
			gb_pcie->f_info.vga_firmware_maj = buf_data[0] & 0xff;
			GB02FUNC819(GB02MAC1594, buf_data, 0x2, vga_index);
			gb_pcie->f_info.vga_firmware_min = buf_data[0] & 0xff;
			gb_pcie->f_info.version_info |= GB02MAC1596;
		}
	}

        return 0;
}
