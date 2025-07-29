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

#ifndef	__GBDC_INFINITY_H__
#define	__GBDC_INFINITY_H__

/*avoid memory overstep*/
#define GB02MAC2558 GB02MAC2693
#define GB02MAC2560 32

enum gbdc_infinity_info_type {
	INFINITY_ID,
	INFINITY_ENABLE,
	INFINITY_ROW,
	INFINITY_COL,
	INFINITY_CONN_ID,
	INFINITY_CONN_STAT,
	INFINITY_ORDER,
	INFINITY_GEOMETRY,
	INFINITY_PADDING,
};

struct GB02STR189 {
	unsigned int width;
	unsigned int height;
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
	unsigned int left;
};

struct GB02STR190 {
	unsigned int x;
	unsigned int y;
};

struct GB02STR191 {
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
};

struct GB02STR192 {
	unsigned int connector_id;
	enum drm_connector_status connected;
	struct GB02STR190 order;
	struct GB02STR191 geometry;
	struct GB02STR189 padding[GB02MAC2560];
};

struct GB02STR193 {
	unsigned int connector_id;
	struct GB02STR190 order;
	struct GB02STR189 padding[GB02MAC2560];
};

struct GB02STR194 {
	unsigned int infinity_id;
	unsigned int infinity_enable;
	unsigned int infinity_row;
	unsigned int infinity_col;
	struct GB02STR192 infos[0];
};

struct GB02STR195 {
	unsigned int infinity_row;
	unsigned int infinity_col;
	struct GB02STR193 adjusts[0];
};

struct GB02STR196 {
	unsigned int infinity_id;
	unsigned int infinity_row;
	unsigned int infinity_col;
	struct GB02STR190 order;
	struct GB02STR189 padding[GB02MAC2560];
};

struct GB02STR197{
	unsigned int id_changed : 1;
	unsigned int order_changed : 1;
	unsigned int padding_changed : 1;
};

union u_info_changed{
	unsigned int info_changed;
	struct GB02STR197 st;
};

struct GB02STR198 {
	struct list_head head;
	unsigned int infinity_connector_id;
	enum drm_connector_status connected;

	struct gbdc_connector *gbdc_connector;

	bool created;
	bool changed;
	bool mode_changed;
	bool geometry_changed;
	bool padding_changed;
	bool recreated;
	bool destroyed;
	bool rebuild;
	bool tiny_recreated;
	bool crtc_need_disable;

	union u_info_changed *uinfo_changed;

	struct GB02STR194 *infinity_infos;
};

#define GB02MAC2564 \
	(sizeof(struct GB02STR192) * GB02MAC2558)

#define GB02MAC2566 \
	(sizeof(struct GB02STR194) + GB02MAC2564)

#define GB02MAC2569 \
	(sizeof(struct GB02STR193) * GB02MAC2558)

#define GB02MAC2571 \
	(sizeof(struct GB02STR195) + GB02MAC2569)

#define gbdc_for_each_infinity(infinity, dev) \
	list_for_each_entry(infinity, \
		&GB02FUNC85((dev)->dev_private)->kms_info.infinity_list, head)

#define gbdc_for_each_infinity_safe(infinity, dev) \
	list_for_each_entry_safe(infinity, n_##infinity,\
		&GB02FUNC85((dev)->dev_private)->kms_info.infinity_list, head)

#define gbdc_for_each_gbdc_obj(name) \
	list_for_each_entry(gbdc_##name, \
		&gbdc_##name##_state->infinity_##name##_list->head, head)

#define gbdc_for_each_gbdc_obj_safe(name) \
	list_for_each_entry_safe(gbdc_##name, n_##name,\
		&gbdc_##name##_state->infinity_##name##_list->head, head)

#define GB_PRINT_INFO(fmt, ...) \
	gb_printf(KERN_INFO, fmt, ##__VA_ARGS__)

#define GB_PRINT_ERR(fmt, ...) \
	gb_printf(KERN_ERR, fmt, ##__VA_ARGS__)

#define GB_PRINT_DBG(fmt, ...) \
	gb_printf(KERN_DEBUG, fmt, ##__VA_ARGS__)

inline void GB02FUNC1430(struct drm_device *dev);
inline void GB02FUNC1431(struct drm_device *dev);
int GB02FUNC1563(struct drm_device *dev,
						struct drm_atomic_state *state);
int GB02FUNC1665(struct drm_device *dev,
						struct drm_atomic_state *state);
void GB02FUNC1674(struct drm_atomic_state *state);
//enum drm_connector_status gbdc_infinity_get_connect_status(struct drm_connector *connector);
//bool gbdc_infinity_status();
bool GB02FUNC1680(struct drm_connector *connector,
									struct GB02STR198 *infi, bool irq);
bool GB02FUNC1691(struct drm_device *dev,
											int connector,
											struct GB02STR198 *infi, bool irq);
void GB02FUNC1684(struct drm_device *dev,
							struct drm_connector *connector);
void GB02FUNC1689(struct drm_device *dev, int connector);
int GB02FUNC1419(
						struct gbdc_connector *gbdc_connector,
						enum gbdc_infinity_info_type type,
						unsigned int pos,
						void *data);
int GB02FUNC1692(struct drm_device *dev, int phy_id, bool irq);
void GB02FUNC1671(struct work_struct *work);
ssize_t gbdc_infinity_info_show(struct drm_device *dev, char *buf);
ssize_t gbdc_infinity_state_show(struct drm_device *dev, char *buf);
ssize_t gbdc_infinity_prop_show(struct drm_device *dev, char *buf);
void GB02FUNC1676(struct drm_device *dev);
int GB02FUNC1406(struct drm_connector *connector);
void GB02FUNC1408(struct drm_connector *connector,
									const struct GB02STR194 *infos,
									unsigned int size);
void GB02FUNC1677(struct drm_device *dev, int crtc_id);
int GB02FUNC1390(struct drm_device *dev,
			  void *data, struct drm_file *file_priv);
void GB02FUNC1529(struct drm_device *dev,
					       struct drm_atomic_state *old_state);
struct mutex *GB02FUNC1437(struct drm_atomic_state *state);
struct mutex *GB02FUNC1433(struct drm_atomic_state *state);
void GB02FUNC1696(struct drm_atomic_state *old_state);
#endif