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
#ifndef	__GBDC_DEV_H__
#define	__GBDC_DEV_H__
struct gbdc_connector;
/*
 * This struct is a callback hook created based on the mode setting,
 * all members must have corresponding implementation functions.
 */
struct GB02STR152 {
	enum genbu_asic_type card_type;
	int (*modeset_config)(struct GB02STR252 *ip_config,
			const struct drm_display_mode *ptr, int crtc_id);
	int (*prepare_config)(void __iomem *dc_base,
			struct GB02STR252 *ip_config,
			int mode0, int crtc_id);
	bool (*connector_state)(struct GB02STR252 *ip_config,
			int conn_id);
	int (*switch_conn)(void __iomem *hdmi_base, int conn_id, int vga_cmd);
};
/*
 * This is a callback hook used to distinguish different chips and
 * add different control functions.
 * If the device-related callbacks can be added later, not all of them
 * are required. If the corresponding member is empty, set to NULL.
 */
struct GB02STR153 {
	int		card_type;
	struct GB02STR152	*modeset_ops;
	struct task_struct *hpd_thread;
	int (*gb_kthread)(struct drm_device *drm_dev);
	int (*gb_shundown)(void __iomem *ip_base);
	void (*gb_port_disable)(void __iomem *ip_base);
	int (*gb_hdmi_ddc)(struct gbdc_connector *gbdc_conn);
};

#endif
