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
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
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
#include <drm/drm_modes.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>

#include "common/gb_common.h"
#include "gbdc_encoder.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "gbdc_drv.h"

static void GB02FUNC1199(struct drm_encoder *encoder,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{

}

static void GB02FUNC1200(struct drm_encoder *encoder, int state)
{
}

static void GB02FUNC1201(struct drm_encoder *encoder)
{
}

static void GB02FUNC1204(struct drm_encoder *encoder)
{
}

static bool GB02FUNC1205(struct drm_encoder *encoder,
				    const struct drm_display_mode *mode,
				    struct drm_display_mode *adjusted_mode)
{

	return true;
}

static const struct drm_encoder_helper_funcs gbdc_encoder_helper_funcs = {
	.dpms = GB02FUNC1200,
	.mode_set = GB02FUNC1199,
	.prepare = GB02FUNC1201,
	.mode_fixup = GB02FUNC1205,
	.commit = GB02FUNC1204,
};

static const struct drm_encoder_funcs gbdc_encoder_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};
/**
 * GB02FUNC1206() - init encoder dev
 * @dev: drm device info
 * @enc_id: bind possible crtc id
 *
 * Returns the GB02STR156 dev info by zalloc.
 */
struct GB02STR156 *GB02FUNC1206(struct drm_device *dev, int enc_id, bool virt)
{
	int ret = 0;
	int enc_id_tmp;
	struct drm_encoder *encoder;
	struct GB02STR156 *gb_encoder;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);

	gb_encoder = kzalloc(sizeof(*gb_encoder), GFP_KERNEL);
	if (!gb_encoder)
		return ERR_PTR(-ENOMEM);

	encoder = &gb_encoder->base;
	if(!virt)
		enc_id_tmp = GB02FUNC1904(gb_type, enc_id);
	else
		enc_id_tmp = GB02FUNC1903(gb_type, enc_id);;

	pr_info("%s:virt %d,enc_id_tmp%d,enc_id%d\n", __func__, virt,
			enc_id_tmp, enc_id);

	encoder->possible_crtcs = 1 << enc_id_tmp;
	gb_encoder->encoder_id = enc_id;

	if (GB02FUNC1207(encoder))
		/* VGA */
		ret = drm_encoder_init(dev, encoder,
		 &gbdc_encoder_encoder_funcs, DRM_MODE_ENCODER_DAC, NULL);
	else
		ret = drm_encoder_init(dev, encoder,
		 &gbdc_encoder_encoder_funcs, DRM_MODE_ENCODER_TMDS, NULL);

	if (ret) {
		gb_printf(KERN_ERR, "error in %s: %d\n", __func__, __LINE__);
		kfree(gb_encoder);
		return (struct GB02STR156 *)NULL;
	}
	drm_encoder_helper_add(encoder, &gbdc_encoder_helper_funcs);

	return gb_encoder;
}

int GB02FUNC1207(struct drm_encoder *encoder)
{
	int ret = 0;
	struct GB02STR156 *gb_encoder = to_gbdc_encoder(encoder);
	genbu_board_type_e bd_type = GB02FUNC1184();

	if ((bd_type == GENBU_MXM_BOARD) &&
		(gb_encoder->encoder_id == GBDC_ENCODER1))
		ret = 1;

	return ret;
}
