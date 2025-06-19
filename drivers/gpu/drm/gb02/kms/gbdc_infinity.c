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
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#include <linux/file.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_modes.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>

#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "gbdc_drv.h"
#include "device/gbdc_device.h"
#include "gbdc_connector.h"
#include "gbdc_planes.h"
#include "gbdc_crtc.h"
#include "gbdc_encoder.h"
#include "ip/gb_dp.h"

#include <sync_file.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
#include <drm/drm_atomic_uapi.h>
#endif
#include <drm/drm_writeback.h>
#include <drm/drm_lease.h>
#include <drm/drm_auth.h>
#include <drm/drm_drv.h>
#include <drm/drm_vblank.h>
#include "gbdc_infinity.h"

static bool gbdc_infinity_check_connect_status = true;

static const struct dma_fence_ops drm_crtc_fence_ops;

static struct drm_crtc *GB02FUNC1257(struct dma_fence *fence)
{
	BUG_ON(fence->ops != &drm_crtc_fence_ops);
	return container_of(fence->lock, struct drm_crtc, fence_lock);
}

static const char *GB02FUNC1259(struct dma_fence *fence)
{
	struct drm_crtc *crtc = GB02FUNC1257(fence);

	return crtc->dev->driver->name;
}

static const char *GB02FUNC1260(struct dma_fence *fence)
{
	struct drm_crtc *crtc = GB02FUNC1257(fence);

	return crtc->timeline_name;
}

static const struct dma_fence_ops drm_crtc_fence_ops = {
	.get_driver_name = GB02FUNC1259,
	.get_timeline_name = GB02FUNC1260,
};

static struct dma_fence *GB02FUNC1263(struct drm_crtc *crtc)
{
	struct dma_fence *fence;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return NULL;

	dma_fence_init(fence, &drm_crtc_fence_ops, &crtc->fence_lock,
		       crtc->fence_context, ++crtc->fence_seqno);

	return fence;
}

#define drm_for_each_lessee(lessee, lessor) \
	list_for_each_entry((lessee), &(lessor)->lessees, lessee_list)

/**
 * GB02FUNC1267 - check to see if an object is leased (or owned) by master (idr_mutex held)
 * @master: the master to check the lease status of
 * @id: the id to check
 *
 * Checks if the specified master holds a lease on the object. Return
 * value:
 *
 *	true		'master' holds a lease on (or owns) the object
 *	false		'master' does not hold a lease.
 */
static int GB02FUNC1267(struct drm_master *master, int id)
{
	lockdep_assert_held(&master->dev->mode_config.idr_mutex);
	if (master->lessor)
		return idr_find(&master->leases, id) != NULL;
	return true;
}

/**
 * GB02FUNC1269 - check to see if an object has been leased (idr_mutex held)
 * @master: the master to check the lease status of
 * @id: the id to check
 *
 * Checks if any lessee of 'master' holds a lease on 'id'. Return
 * value:
 *
 *	true		Some lessee holds a lease on the object.
 *	false		No lessee has a lease on the object.
 */
static bool __maybe_unused GB02FUNC1269(struct drm_master *master, int id)
{
	struct drm_master *lessee;

	lockdep_assert_held(&master->dev->mode_config.idr_mutex);
	drm_for_each_lessee(lessee, master)
		if (GB02FUNC1267(lessee, id))
			return true;
	return false;
}

/**
 * _drm_lease_held - check drm_mode_object lease status (idr_mutex held)
 * @file_priv: the master drm_file
 * @id: the object id
 *
 * Checks if the specified master holds a lease on the object. Return
 * value:
 *
 *	true		'master' holds a lease on (or owns) the object
 *	false		'master' does not hold a lease.
 */
bool GB02FUNC1271(struct drm_file *file_priv, int id)
{
	if (!file_priv || !file_priv->master)
		return true;

	return GB02FUNC1267(file_priv->master, id);
}

/**
 * drm_lease_required - check types which must be leased to be used
 * @type: type of object
 *
 * Returns whether the provided type of drm_mode_object must
 * be owned or leased to be used by a process.
 */
static bool GB02FUNC1273(uint32_t type)
{
	switch(type) {
	case DRM_MODE_OBJECT_CRTC:
	case DRM_MODE_OBJECT_CONNECTOR:
	case DRM_MODE_OBJECT_PLANE:
		return true;
	default:
		return false;
	}
}

static struct drm_mode_object *GB02FUNC1275(struct drm_device *dev,
					       struct drm_file *file_priv,
					       uint32_t id, uint32_t type)
{
	struct drm_mode_object *obj = NULL;

	mutex_lock(&dev->mode_config.idr_mutex);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	obj = idr_find(&dev->mode_config.object_idr, id);
#else
	obj = idr_find(&dev->mode_config.crtc_idr, id);
#endif
	if (obj && type != DRM_MODE_OBJECT_ANY && obj->type != type)
		obj = NULL;
	if (obj && obj->id != id)
		obj = NULL;

	if (obj && GB02FUNC1273(obj->type) &&
	    !GB02FUNC1271(file_priv, obj->id))
		obj = NULL;

	if (obj && obj->free_cb) {
		if (!kref_get_unless_zero(&obj->refcount))
			obj = NULL;
	}
	mutex_unlock(&dev->mode_config.idr_mutex);

	return obj;
}

/* Some properties could refer to dynamic refcnt'd objects, or things that
 * need special locking to handle lifetime issues (ie. to ensure the prop
 * value doesn't become invalid part way through the property update due to
 * race).  The value returned by reference via 'obj' should be passed back
 * to GB02FUNC1284() after the property is set (and the
 * object to which the property is attached has a chance to take its own
 * reference).
 */
static bool GB02FUNC1278(struct drm_property *property,
				   uint64_t value, struct drm_mode_object **ref)
{
	int i;

	if (property->flags & DRM_MODE_PROP_IMMUTABLE)
		return false;

	*ref = NULL;

	if (drm_property_type_is(property, DRM_MODE_PROP_RANGE)) {
		if (value < property->values[0] || value > property->values[1])
			return false;
		return true;
	} else if (drm_property_type_is(property, DRM_MODE_PROP_SIGNED_RANGE)) {
		int64_t svalue = U642I64(value);

		if (svalue < U642I64(property->values[0]) ||
				svalue > U642I64(property->values[1]))
			return false;
		return true;
	} else if (drm_property_type_is(property, DRM_MODE_PROP_BITMASK)) {
		uint64_t valid_mask = 0;

		for (i = 0; i < property->num_values; i++)
			valid_mask |= (1ULL << property->values[i]);
		return !(value & ~valid_mask);
	} else if (drm_property_type_is(property, DRM_MODE_PROP_BLOB)) {
		struct drm_property_blob *blob;

		if (value == 0)
			return true;

		blob = drm_property_lookup_blob(property->dev, value);
		if (blob) {
			*ref = &blob->base;
			return true;
		} else {
			return false;
		}
	} else if (drm_property_type_is(property, DRM_MODE_PROP_OBJECT)) {
		/* a zero value for an object property translates to null: */
		if (value == 0)
			return true;

		*ref = GB02FUNC1275(property->dev, NULL, value,
					      property->values[0]);
		return *ref != NULL;
	}

	for (i = 0; i < property->num_values; i++)
		if (property->values[i] == value)
			return true;
	return false;
}

static void GB02FUNC1284(struct drm_property *property,
		struct drm_mode_object *ref)
{
	if (!ref)
		return;

	if (drm_property_type_is(property, DRM_MODE_PROP_OBJECT)) {
		drm_mode_object_put(ref);
	} else if (drm_property_type_is(property, DRM_MODE_PROP_BLOB))
		drm_property_blob_put(obj_to_blob(ref));
}

static struct drm_pending_vblank_event *GB02FUNC1288(
		struct drm_crtc *crtc, uint64_t user_data)
{
	struct drm_pending_vblank_event *e = NULL;

	e = kzalloc(sizeof(*e), GFP_KERNEL);
	if (!e)
		return NULL;

	e->event.base.type = DRM_EVENT_FLIP_COMPLETE;
	e->event.base.length = sizeof(e->event);
	e->event.vbl.crtc_id = crtc->base.id;
	e->event.vbl.user_data = user_data;

	return e;
}

static void GB02FUNC1291(struct drm_atomic_state *state,
				   struct drm_crtc *crtc, s32 __user *fence_ptr)
{
	state->crtcs[drm_crtc_index(crtc)].out_fence_ptr = fence_ptr;
}

static s32 __user *GB02FUNC1292(struct drm_atomic_state *state,
					  struct drm_crtc *crtc)
{
	s32 __user *fence_ptr;

	fence_ptr = state->crtcs[drm_crtc_index(crtc)].out_fence_ptr;
	state->crtcs[drm_crtc_index(crtc)].out_fence_ptr = NULL;

	return fence_ptr;
}

static int GB02FUNC1295(struct drm_atomic_state *state,
					struct drm_connector *connector,
					s32 __user *fence_ptr)
{
	unsigned int index = drm_connector_index(connector);

	if (!fence_ptr)
		return 0;

	if (put_user(-1, fence_ptr))
		return -EFAULT;

	state->connectors[index].out_fence_ptr = fence_ptr;

	return 0;
}

static s32 __user *GB02FUNC1299(struct drm_atomic_state *state,
					       struct drm_connector *connector)
{
	unsigned int index = drm_connector_index(connector);
	s32 __user *fence_ptr;

	fence_ptr = state->connectors[index].out_fence_ptr;
	state->connectors[index].out_fence_ptr = NULL;

	return fence_ptr;
}

static int
drm_atomic_replace_property_blob_from_id(struct drm_device *dev,
					 struct drm_property_blob **blob,
					 uint64_t blob_id,
					 ssize_t expected_size,
					 ssize_t expected_elem_size,
					 bool *replaced)
{
	struct drm_property_blob *new_blob = NULL;

	if (blob_id != 0) {
		new_blob = drm_property_lookup_blob(dev, blob_id);
		if (new_blob == NULL)
			return -EINVAL;

		if (expected_size > 0 &&
		    new_blob->length != expected_size) {
			drm_property_blob_put(new_blob);
			return -EINVAL;
		}
		if (expected_elem_size > 0 &&
		    new_blob->length % expected_elem_size != 0) {
			drm_property_blob_put(new_blob);
			return -EINVAL;
		}
	}

	*replaced |= drm_property_replace_blob(blob, new_blob);
	drm_property_blob_put(new_blob);

	return 0;
}

static int GB02FUNC1303(struct drm_crtc *crtc,
		struct drm_crtc_state *state, struct drm_property *property,
		uint64_t val)
{
	struct drm_device *dev = crtc->dev;
	struct drm_mode_config *config = &dev->mode_config;
	bool replaced = false;
	int ret;

	if (property == config->prop_active)
		state->active = val;
	else if (property == config->prop_mode_id) {
		struct drm_property_blob *mode =
			drm_property_lookup_blob(dev, val);
		ret = drm_atomic_set_mode_prop_for_crtc(state, mode);
		drm_property_blob_put(mode);
		return ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	} else if (property == config->prop_vrr_enabled) {
		state->vrr_enabled = val;
#endif
	} else if (property == config->degamma_lut_property) {
		ret = drm_atomic_replace_property_blob_from_id(dev,
					&state->degamma_lut,
					val,
					-1, sizeof(struct drm_color_lut),
					&replaced);
		state->color_mgmt_changed |= replaced;
		return ret;
	} else if (property == config->ctm_property) {
		ret = drm_atomic_replace_property_blob_from_id(dev,
					&state->ctm,
					val,
					sizeof(struct drm_color_ctm), -1,
					&replaced);
		state->color_mgmt_changed |= replaced;
		return ret;
	} else if (property == config->gamma_lut_property) {
		ret = drm_atomic_replace_property_blob_from_id(dev,
					&state->gamma_lut,
					val,
					-1, sizeof(struct drm_color_lut),
					&replaced);
		state->color_mgmt_changed |= replaced;
		return ret;
	} else if (property == config->prop_out_fence_ptr) {
		s32 __user *fence_ptr = u64_to_user_ptr(val);

		if (!fence_ptr)
			return 0;

		if (put_user(-1, fence_ptr))
			return -EFAULT;

		GB02FUNC1291(state->state, crtc, fence_ptr);
	} else if (crtc->funcs->atomic_set_property) {
		return crtc->funcs->atomic_set_property(crtc, state, property, val);
	} else {
		DRM_DEBUG_ATOMIC("[CRTC:%d:%s] unknown property [PROP:%d:%s]]\n",
				 crtc->base.id, crtc->name,
				 property->base.id, property->name);
		return -EINVAL;
	}

	return 0;
}

static int __maybe_unused
drm_atomic_crtc_get_property(struct drm_crtc *crtc,
		const struct drm_crtc_state *state,
		struct drm_property *property, uint64_t *val)
{
	struct drm_device *dev = crtc->dev;
	struct drm_mode_config *config = &dev->mode_config;

	if (property == config->prop_active)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
		*val = drm_atomic_crtc_effectively_active(state);
#else
		*val = state->active;
#endif
	else if (property == config->prop_mode_id)
		*val = (state->mode_blob) ? state->mode_blob->base.id : 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	else if (property == config->prop_vrr_enabled)
		*val = state->vrr_enabled;
#endif
	else if (property == config->degamma_lut_property)
		*val = (state->degamma_lut) ? state->degamma_lut->base.id : 0;
	else if (property == config->ctm_property)
		*val = (state->ctm) ? state->ctm->base.id : 0;
	else if (property == config->gamma_lut_property)
		*val = (state->gamma_lut) ? state->gamma_lut->base.id : 0;
	else if (property == config->prop_out_fence_ptr)
		*val = 0;
	else if (crtc->funcs->atomic_get_property)
		return crtc->funcs->atomic_get_property(crtc, state, property, val);
	else
		return -EINVAL;

	return 0;
}

static int GB02FUNC1317(struct drm_plane *plane,
		struct drm_plane_state *state, struct drm_file *file_priv,
		struct drm_property *property, uint64_t val)
{
	struct drm_device *dev = plane->dev;
	struct drm_mode_config *config = &dev->mode_config;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	bool replaced = false;
	int ret;
#endif

	if (property == config->prop_fb_id) {
		struct drm_framebuffer *fb;
		fb = drm_framebuffer_lookup(dev, file_priv, val);
		drm_atomic_set_fb_for_plane(state, fb);
		if (fb)
			drm_framebuffer_put(fb);
	} else if (property == config->prop_in_fence_fd) {
		if (state->fence)
			return -EINVAL;

		if (U642I64(val) == -1)
			return 0;

		state->fence = sync_file_get_fence(val);
		if (!state->fence)
			return -EINVAL;

	} else if (property == config->prop_crtc_id) {
		struct drm_crtc *crtc = drm_crtc_find(dev, file_priv, val);
		if (val && !crtc)
			return -EACCES;
		return drm_atomic_set_crtc_for_plane(state, crtc);
	} else if (property == config->prop_crtc_x) {
		state->crtc_x = U642I64(val);
	} else if (property == config->prop_crtc_y) {
		state->crtc_y = U642I64(val);
	} else if (property == config->prop_crtc_w) {
		state->crtc_w = val;
	} else if (property == config->prop_crtc_h) {
		state->crtc_h = val;
	} else if (property == config->prop_src_x) {
		state->src_x = val;
	} else if (property == config->prop_src_y) {
		state->src_y = val;
	} else if (property == config->prop_src_w) {
		state->src_w = val;
	} else if (property == config->prop_src_h) {
		state->src_h = val;
	} else if (property == plane->alpha_property) {
		state->alpha = val;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	} else if (property == plane->blend_mode_property) {
		state->pixel_blend_mode = val;
#endif
	} else if (property == plane->rotation_property) {
		if (!is_power_of_2(val & DRM_MODE_ROTATE_MASK)) {
			DRM_DEBUG_ATOMIC("[PLANE:%d:%s] bad rotation bitmask: 0x%llx\n",
					 plane->base.id, plane->name, val);
			return -EINVAL;
		}
		state->rotation = val;
	} else if (property == plane->zpos_property) {
		state->zpos = val;
	} else if (property == plane->color_encoding_property) {
		state->color_encoding = val;
	} else if (property == plane->color_range_property) {
		state->color_range = val;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	} else if (property == config->prop_fb_damage_clips) {
		ret = drm_atomic_replace_property_blob_from_id(dev,
					&state->fb_damage_clips,
					val,
					-1,
					sizeof(struct drm_rect),
					&replaced);
		return ret;
#endif
	} else if (plane->funcs->atomic_set_property) {
		return plane->funcs->atomic_set_property(plane, state,
				property, val);
	} else {
		DRM_DEBUG_ATOMIC("[PLANE:%d:%s] unknown property [PROP:%d:%s]]\n",
				 plane->base.id, plane->name,
				 property->base.id, property->name);
		return -EINVAL;
	}

	return 0;
}

static int __maybe_unused
drm_atomic_plane_get_property(struct drm_plane *plane,
		const struct drm_plane_state *state,
		struct drm_property *property, uint64_t *val)
{
	struct drm_device *dev = plane->dev;
	struct drm_mode_config *config = &dev->mode_config;

	if (property == config->prop_fb_id) {
		*val = (state->fb) ? state->fb->base.id : 0;
	} else if (property == config->prop_in_fence_fd) {
		*val = -1;
	} else if (property == config->prop_crtc_id) {
		*val = (state->crtc) ? state->crtc->base.id : 0;
	} else if (property == config->prop_crtc_x) {
		*val = I642U64(state->crtc_x);
	} else if (property == config->prop_crtc_y) {
		*val = I642U64(state->crtc_y);
	} else if (property == config->prop_crtc_w) {
		*val = state->crtc_w;
	} else if (property == config->prop_crtc_h) {
		*val = state->crtc_h;
	} else if (property == config->prop_src_x) {
		*val = state->src_x;
	} else if (property == config->prop_src_y) {
		*val = state->src_y;
	} else if (property == config->prop_src_w) {
		*val = state->src_w;
	} else if (property == config->prop_src_h) {
		*val = state->src_h;
	} else if (property == plane->alpha_property) {
		*val = state->alpha;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	} else if (property == plane->blend_mode_property) {
		*val = state->pixel_blend_mode;
#endif
	} else if (property == plane->rotation_property) {
		*val = state->rotation;
	} else if (property == plane->zpos_property) {
		*val = state->zpos;
	} else if (property == plane->color_encoding_property) {
		*val = state->color_encoding;
	} else if (property == plane->color_range_property) {
		*val = state->color_range;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	} else if (property == config->prop_fb_damage_clips) {
		*val = (state->fb_damage_clips) ?
			state->fb_damage_clips->base.id : 0;
#endif
	} else if (plane->funcs->atomic_get_property) {
		return plane->funcs->atomic_get_property(plane, state, property, val);
	} else {
		return -EINVAL;
	}

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
static int gbdc_drm_writeback_set_fb(struct drm_connector_state *conn_state,
			 struct drm_framebuffer *fb)
{
	WARN_ON(conn_state->connector->connector_type != DRM_MODE_CONNECTOR_WRITEBACK);

	if (!conn_state->writeback_job) {
		conn_state->writeback_job =
			kzalloc(sizeof(*conn_state->writeback_job), GFP_KERNEL);
		if (!conn_state->writeback_job)
			return -ENOMEM;

		conn_state->writeback_job->connector =
			drm_connector_to_writeback(conn_state->connector);
	}

	drm_framebuffer_assign(&conn_state->writeback_job->fb, fb);
	return 0;
}

static int drm_atomic_set_writeback_fb_for_connector(
		struct drm_connector_state *conn_state,
		struct drm_framebuffer *fb)
{
	int ret;

	ret = gbdc_drm_writeback_set_fb(conn_state, fb);
	if (ret < 0)
		return ret;

	if (fb)
		DRM_DEBUG_ATOMIC("Set [FB:%d] for connector state %p\n",
				 fb->base.id, conn_state);
	else
		DRM_DEBUG_ATOMIC("Set [NOFB] for connector state %p\n",
				 conn_state);

	return 0;
}
#endif

static int GB02FUNC1325(struct drm_connector *connector,
		struct drm_connector_state *state, struct drm_file *file_priv,
		struct drm_property *property, uint64_t val)
{
	struct drm_device *dev = connector->dev;
	struct drm_mode_config *config = &dev->mode_config;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	bool replaced = false;
	int ret;
#endif

	if (property == config->prop_crtc_id) {
		struct drm_crtc *crtc = drm_crtc_find(dev, file_priv, val);
		if (val && !crtc)
			return -EACCES;
		return drm_atomic_set_crtc_for_connector(state, crtc);
	} else if (property == config->dpms_property) {
		/* setting DPMS property requires special handling, which
		 * is done in legacy setprop path for us.  Disallow (for
		 * now?) atomic writes to DPMS property:
		 */
		return -EINVAL;
	} else if (property == config->tv_select_subconnector_property) {
		state->tv.subconnector = val;
	} else if (property == config->tv_left_margin_property) {
		state->tv.margins.left = val;
	} else if (property == config->tv_right_margin_property) {
		state->tv.margins.right = val;
	} else if (property == config->tv_top_margin_property) {
		state->tv.margins.top = val;
	} else if (property == config->tv_bottom_margin_property) {
		state->tv.margins.bottom = val;
	} else if (property == config->tv_mode_property) {
		state->tv.mode = val;
	} else if (property == config->tv_brightness_property) {
		state->tv.brightness = val;
	} else if (property == config->tv_contrast_property) {
		state->tv.contrast = val;
	} else if (property == config->tv_flicker_reduction_property) {
		state->tv.flicker_reduction = val;
	} else if (property == config->tv_overscan_property) {
		state->tv.overscan = val;
	} else if (property == config->tv_saturation_property) {
		state->tv.saturation = val;
	} else if (property == config->tv_hue_property) {
		state->tv.hue = val;
	} else if (property == config->link_status_property) {
		/* Never downgrade from GOOD to BAD on userspace's request here,
		 * only hw issues can do that.
		 *
		 * For an atomic property the userspace doesn't need to be able
		 * to understand all the properties, but needs to be able to
		 * restore the state it wants on VT switch. So if the userspace
		 * tries to change the link_status from GOOD to BAD, driver
		 * silently rejects it and returns a 0. This prevents userspace
		 * from accidently breaking  the display when it restores the
		 * state.
		 */
		if (state->link_status != DRM_LINK_STATUS_GOOD)
			state->link_status = val;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	} else if (property == config->hdr_output_metadata_property) {
		ret = drm_atomic_replace_property_blob_from_id(dev,
				&state->hdr_output_metadata,
				val,
				sizeof(struct hdr_output_metadata), -1,
				&replaced);
		return ret;
#endif
	} else if (property == config->aspect_ratio_property) {
		state->picture_aspect_ratio = val;
	} else if (property == config->content_type_property) {
		state->content_type = val;
	} else if (property == connector->scaling_mode_property) {
		state->scaling_mode = val;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	} else if (property == config->content_protection_property) {
#else
	} else if (property == connector->content_protection_property) {
#endif
		if (val == DRM_MODE_CONTENT_PROTECTION_ENABLED) {
			DRM_DEBUG_KMS("only drivers can set CP Enabled\n");
			return -EINVAL;
		}
		state->content_protection = val;

		if (val == DRM_MODE_CONTENT_PROTECTION_ENABLED) {
			DRM_DEBUG_KMS("only drivers can set CP Enabled\n");
			return -EINVAL;
		}
		state->content_protection = val;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	} else if (property == config->hdcp_content_type_property) {
		state->hdcp_content_type = val;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
	} else if (property == connector->colorspace_property) {
		state->colorspace = val;
#endif
	} else if (property == config->writeback_fb_id_property) {
		struct drm_framebuffer *fb;
		int ret;
		fb = drm_framebuffer_lookup(dev, file_priv, val);
		ret = drm_atomic_set_writeback_fb_for_connector(state, fb);
		if (fb)
			drm_framebuffer_put(fb);
		return ret;
	} else if (property == config->writeback_out_fence_ptr_property) {
		s32 __user *fence_ptr = u64_to_user_ptr(val);

		return GB02FUNC1295(state->state, connector,
						   fence_ptr);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	} else if (property == connector->max_bpc_property) {
		state->max_requested_bpc = val;
#endif
	} else if (connector->funcs->atomic_set_property) {
		return connector->funcs->atomic_set_property(connector,
				state, property, val);
	} else {
		DRM_DEBUG_ATOMIC("[CONNECTOR:%d:%s] unknown property [PROP:%d:%s]]\n",
				 connector->base.id, connector->name,
				 property->base.id, property->name);
		return -EINVAL;
	}

	return 0;
}

static int __maybe_unused
drm_atomic_connector_get_property(struct drm_connector *connector,
		const struct drm_connector_state *state,
		struct drm_property *property, uint64_t *val)
{
	struct drm_device *dev = connector->dev;
	struct drm_mode_config *config = &dev->mode_config;

	if (property == config->prop_crtc_id) {
		*val = (state->crtc) ? state->crtc->base.id : 0;
	} else if (property == config->dpms_property) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
		if (state->crtc && state->crtc->state->self_refresh_active)
			*val = DRM_MODE_DPMS_ON;
		else
#endif
			*val = connector->dpms;
	} else if (property == config->tv_select_subconnector_property) {
		*val = state->tv.subconnector;
	} else if (property == config->tv_left_margin_property) {
		*val = state->tv.margins.left;
	} else if (property == config->tv_right_margin_property) {
		*val = state->tv.margins.right;
	} else if (property == config->tv_top_margin_property) {
		*val = state->tv.margins.top;
	} else if (property == config->tv_bottom_margin_property) {
		*val = state->tv.margins.bottom;
	} else if (property == config->tv_mode_property) {
		*val = state->tv.mode;
	} else if (property == config->tv_brightness_property) {
		*val = state->tv.brightness;
	} else if (property == config->tv_contrast_property) {
		*val = state->tv.contrast;
	} else if (property == config->tv_flicker_reduction_property) {
		*val = state->tv.flicker_reduction;
	} else if (property == config->tv_overscan_property) {
		*val = state->tv.overscan;
	} else if (property == config->tv_saturation_property) {
		*val = state->tv.saturation;
	} else if (property == config->tv_hue_property) {
		*val = state->tv.hue;
	} else if (property == config->link_status_property) {
		*val = state->link_status;
	} else if (property == config->aspect_ratio_property) {
		*val = state->picture_aspect_ratio;
	} else if (property == config->content_type_property) {
		*val = state->content_type;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
	} else if (property == connector->colorspace_property) {
		*val = state->colorspace;
#endif
	} else if (property == connector->scaling_mode_property) {
		*val = state->scaling_mode;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	} else if (property == config->hdr_output_metadata_property) {
		*val = state->hdr_output_metadata ?
			state->hdr_output_metadata->base.id : 0;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	} else if (property == config->content_protection_property) {
#else
	} else if (property == connector->content_protection_property) {
#endif
		*val = state->content_protection;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	} else if (property == config->hdcp_content_type_property) {
		*val = state->hdcp_content_type;
#endif
	} else if (property == config->writeback_fb_id_property) {
		/* Writeback framebuffer is one-shot, write and forget */
		*val = 0;
	} else if (property == config->writeback_out_fence_ptr_property) {
		*val = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	} else if (property == connector->max_bpc_property) {
		*val = state->max_requested_bpc;
#endif
	} else if (connector->funcs->atomic_get_property) {
		return connector->funcs->atomic_get_property(connector,
				state, property, val);
	} else {
		return -EINVAL;
	}

	return 0;
}

/**
 * DOC: explicit fencing properties
 *
 * Explicit fencing allows userspace to control the buffer synchronization
 * between devices. A Fence or a group of fences are transfered to/from
 * userspace using Sync File fds and there are two DRM properties for that.
 * IN_FENCE_FD on each DRM Plane to send fences to the kernel and
 * OUT_FENCE_PTR on each DRM CRTC to receive fences from the kernel.
 *
 * As a contrast, with implicit fencing the kernel keeps track of any
 * ongoing rendering, and automatically ensures that the atomic update waits
 * for any pending rendering to complete. For shared buffers represented with
 * a &struct dma_buf this is tracked in &struct dma_resv.
 * Implicit syncing is how Linux traditionally worked (e.g. DRI2/3 on X.org),
 * whereas explicit fencing is what Android wants.
 *
 * "IN_FENCE_FD”:
 *	Use this property to pass a fence that DRM should wait on before
 *	proceeding with the Atomic Commit request and show the framebuffer for
 *	the plane on the screen. The fence can be either a normal fence or a
 *	merged one, the sync_file framework will handle both cases and use a
 *	fence_array if a merged fence is received. Passing -1 here means no
 *	fences to wait on.
 *
 *	If the Atomic Commit request has the DRM_MODE_ATOMIC_TEST_ONLY flag
 *	it will only check if the Sync File is a valid one.
 *
 *	On the driver side the fence is stored on the @fence parameter of
 *	&struct drm_plane_state. Drivers which also support implicit fencing
 *	should set the implicit fence using drm_atomic_set_fence_for_plane(),
 *	to make sure there's consistent behaviour between drivers in precedence
 *	of implicit vs. explicit fencing.
 *
 * "OUT_FENCE_PTR”:
 *	Use this property to pass a file descriptor pointer to DRM. Once the
 *	Atomic Commit request call returns OUT_FENCE_PTR will be filled with
 *	the file descriptor number of a Sync File. This Sync File contains the
 *	CRTC fence that will be signaled when all framebuffers present on the
 *	Atomic Commit * request for that given CRTC are scanned out on the
 *	screen.
 *
 *	The Atomic Commit request fails if a invalid pointer is passed. If the
 *	Atomic Commit request fails for any other reason the out fence fd
 *	returned will be -1. On a Atomic Commit with the
 *	DRM_MODE_ATOMIC_TEST_ONLY flag the out fence will also be set to -1.
 *
 *	Note that out-fences don't have a special interface to drivers and are
 *	internally represented by a &struct drm_pending_vblank_event in struct
 *	&drm_crtc_state, which is also used by the nonblocking atomic commit
 *	helpers and for the DRM event handling for existing userspace.
 */

struct GB02STR161 {
	s32 __user *out_fence_ptr;
	struct sync_file *sync_file;
	int fd;
};

static int GB02FUNC1341(struct GB02STR161 *fence_state,
			   struct dma_fence *fence)
{
	fence_state->fd = get_unused_fd_flags(O_CLOEXEC);
	if (fence_state->fd < 0)
		return fence_state->fd;

	if (put_user(fence_state->fd, fence_state->out_fence_ptr))
		return -EFAULT;

	fence_state->sync_file = sync_file_create(fence);
	if (!fence_state->sync_file)
		return -ENOMEM;

	return 0;
}

static int GB02FUNC1345(struct drm_device *dev,
				  struct drm_atomic_state *state,
				  struct drm_mode_atomic *arg,
				  struct drm_file *file_priv,
				  struct GB02STR161 **fence_state,
				  unsigned int *num_fences)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_connector *conn;
	struct drm_connector_state *conn_state;
	int i, c = 0, ret;

	if (arg->flags & DRM_MODE_ATOMIC_TEST_ONLY)
		return 0;

	for_each_new_crtc_in_state(state, crtc, crtc_state, i) {
		s32 __user *fence_ptr;

		fence_ptr = GB02FUNC1292(crtc_state->state, crtc);

		if (arg->flags & DRM_MODE_PAGE_FLIP_EVENT || fence_ptr) {
			struct drm_pending_vblank_event *e;

			e = GB02FUNC1288(crtc, arg->user_data);
			if (!e)
				return -ENOMEM;

			crtc_state->event = e;
		}

		if (arg->flags & DRM_MODE_PAGE_FLIP_EVENT) {
			struct drm_pending_vblank_event *e = crtc_state->event;

			if (!file_priv)
				continue;

			ret = drm_event_reserve_init(dev, file_priv, &e->base,
						     &e->event.base);
			if (ret) {
				kfree(e);
				crtc_state->event = NULL;
				return ret;
			}
		}

		if (fence_ptr) {
			struct dma_fence *fence;
			struct GB02STR161 *f;

			f = krealloc(*fence_state, sizeof(**fence_state) *
				     (*num_fences + 1), GFP_KERNEL);
			if (!f)
				return -ENOMEM;

			memset(&f[*num_fences], 0, sizeof(*f));

			f[*num_fences].out_fence_ptr = fence_ptr;
			*fence_state = f;

			fence = GB02FUNC1263(crtc);
			if (!fence)
				return -ENOMEM;

			ret = GB02FUNC1341(&f[(*num_fences)++], fence);
			if (ret) {
				dma_fence_put(fence);
				return ret;
			}

			crtc_state->event->base.fence = fence;
		}

		c++;
	}

	for_each_new_connector_in_state(state, conn, conn_state, i) {
		struct drm_writeback_connector *wb_conn;
		struct GB02STR161 *f;
		struct dma_fence *fence;
		s32 __user *fence_ptr;

		if (!conn_state->writeback_job)
			continue;

		fence_ptr = GB02FUNC1299(state, conn);
		if (!fence_ptr)
			continue;

		f = krealloc(*fence_state, sizeof(**fence_state) *
			     (*num_fences + 1), GFP_KERNEL);
		if (!f)
			return -ENOMEM;

		memset(&f[*num_fences], 0, sizeof(*f));

		f[*num_fences].out_fence_ptr = fence_ptr;
		*fence_state = f;

		wb_conn = drm_connector_to_writeback(conn);
		fence = drm_writeback_get_out_fence(wb_conn);
		if (!fence)
			return -ENOMEM;

		ret = GB02FUNC1341(&f[(*num_fences)++], fence);
		if (ret) {
			dma_fence_put(fence);
			return ret;
		}

		conn_state->writeback_job->out_fence = fence;
	}

	/*
	 * Having this flag means user mode pends on event which will never
	 * reach due to lack of at least one CRTC for signaling
	 */
	if (c == 0 && (arg->flags & DRM_MODE_PAGE_FLIP_EVENT))
		return -EINVAL;

	return 0;
}

static void GB02FUNC1364(struct drm_device *dev,
			       struct drm_atomic_state *state,
			       struct GB02STR161 *fence_state,
			       unsigned int num_fences,
			       bool install_fds)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	int i;

	if (install_fds) {
		for (i = 0; i < num_fences; i++)
			fd_install(fence_state[i].fd,
				   fence_state[i].sync_file->file);

		kfree(fence_state);
		return;
	}

	for_each_new_crtc_in_state(state, crtc, crtc_state, i) {
		struct drm_pending_vblank_event *event = crtc_state->event;
		/*
		 * Free the allocated event. drm_atomic_helper_setup_commit
		 * can allocate an event too, so only free it if it's ours
		 * to prevent a double free in drm_atomic_state_clear.
		 */
		if (event && (event->base.fence || event->base.file_priv)) {
			drm_event_cancel_free(dev, &event->base);
			crtc_state->event = NULL;
		}
	}

	if (!fence_state)
		return;

	for (i = 0; i < num_fences; i++) {
		if (fence_state[i].sync_file)
			fput(fence_state[i].sync_file->file);
		if (fence_state[i].fd >= 0)
			put_unused_fd(fence_state[i].fd);

		/* If this fails log error to the user */
		if (fence_state[i].out_fence_ptr &&
		    put_user(-1, fence_state[i].out_fence_ptr))
			DRM_DEBUG_ATOMIC("Couldn't clear out_fence_ptr\n");
	}

	kfree(fence_state);
}

static struct drm_property *GB02FUNC1370(struct drm_mode_object *obj,
					       uint32_t prop_id)
{
	int i;

	for (i = 0; i < obj->properties->count; i++)
		if (obj->properties->properties[i]->base.id == prop_id)
			return obj->properties->properties[i];

	return NULL;
}

static int GB02FUNC1371(struct drm_atomic_state *state,
			    struct drm_file *file_priv,
			    struct drm_mode_object *obj,
			    struct drm_property *prop,
			    uint64_t prop_value)
{
	struct drm_mode_object *ref;
	int ret;

	if (!GB02FUNC1278(prop, prop_value, &ref))
		return -EINVAL;

	switch (obj->type) {
	case DRM_MODE_OBJECT_CONNECTOR: {
		struct drm_connector *connector = obj_to_connector(obj);
		struct drm_connector_state *connector_state;

		connector_state = drm_atomic_get_connector_state(state, connector);
		if (IS_ERR(connector_state)) {
			ret = PTR_ERR(connector_state);
			break;
		}

		ret = GB02FUNC1325(connector,
				connector_state, file_priv,
				prop, prop_value);
		break;
	}
	case DRM_MODE_OBJECT_CRTC: {
		struct drm_crtc *crtc = obj_to_crtc(obj);
		struct drm_crtc_state *crtc_state;

		crtc_state = drm_atomic_get_crtc_state(state, crtc);
		if (IS_ERR(crtc_state)) {
			ret = PTR_ERR(crtc_state);
			break;
		}

		ret = GB02FUNC1303(crtc,
				crtc_state, prop, prop_value);
		break;
	}
	case DRM_MODE_OBJECT_PLANE: {
		struct drm_plane *plane = obj_to_plane(obj);
		struct drm_plane_state *plane_state;

		plane_state = drm_atomic_get_plane_state(state, plane);
		if (IS_ERR(plane_state)) {
			ret = PTR_ERR(plane_state);
			break;
		}

		ret = GB02FUNC1317(plane,
				plane_state, file_priv,
				prop, prop_value);
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}

	GB02FUNC1284(prop, ref);
	return ret;
}

#if 0
static int GB02FUNC1376(
				struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_plane *plane;
	struct drm_plane_state *plane_state;
	struct drm_device *dev = state->dev;
	struct GB02STR198 *infinity = NULL;
	struct drm_connector *conn = NULL;
	struct drm_connector_state *conn_state = NULL;
	bool found_virtual = false;
	struct drm_connector_list_iter conn_iter;
	int ret;

	gbdc_for_each_infinity(infinity, dev) {
		if (!infinity || !infinity->infinity_infos)
			continue;

		if (!infinity->infinity_infos->infinity_enable)
			continue;

		drm_connector_list_iter_begin(dev, &conn_iter);
		drm_for_each_connector_iter(conn, &conn_iter) {

			conn_state = drm_atomic_get_new_connector_state(state, conn) ? \
			drm_atomic_get_new_connector_state(state, conn) : conn->state;

			if (conn->base.id == infinity->infinity_connector_id \
				&& conn_state && conn == conn_state->connector) {

				found_virtual = true;
				break;
			}
		}
		drm_connector_list_iter_end(&conn_iter);
	}

	if (!found_virtual)
		return 0;

	if (!conn_state)
		return -EINVAL;

	crtc = conn_state->crtc;

	if (!crtc)
		return -EINVAL;

	crtc_state = drm_atomic_get_crtc_state(state, crtc);

	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	//if (!GB02FUNC1573(crtc)->is_enable)
	//	return 0;

	drm_for_each_plane(plane, dev) {

		plane_state = drm_atomic_get_new_plane_state(state, plane) ? \
			drm_atomic_get_new_plane_state(state, plane) : plane->state;

		if (plane_state && plane->state && plane_state->crtc == crtc) {
			struct drm_plane_state *new_plane_state =
				drm_atomic_get_plane_state(state, plane);
			if (IS_ERR(new_plane_state))
				return PTR_ERR(new_plane_state);

			ret = drm_atomic_set_crtc_for_plane(plane_state, crtc);
			if (ret != 0)
				return ret;
			drm_atomic_set_fb_for_plane(
				plane_state, plane_state->fb ? plane_state->fb : plane->state->fb);
		}
	}

	return 0;
}
#else
static int GB02FUNC1376(
				struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_plane *plane;
	struct drm_plane_state *plane_state;
	struct drm_device *dev = state->dev;
	struct drm_connector *conn = NULL;
	struct drm_connector_state *conn_state = NULL;
	struct drm_connector_list_iter conn_iter;
	int ret = 0;

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {

		conn_state = drm_atomic_get_new_connector_state(state, conn);

		if (!conn_state)
			continue;

		GB_PRINT_INFO("drm_atomic_get_connector_state %d\n",
				to_gbdc_connector(conn)->connector_id);

		crtc = conn_state->crtc;

		if (!crtc)
			continue;

		crtc_state = drm_atomic_get_crtc_state(state, crtc);

		if (IS_ERR(crtc_state))
			return PTR_ERR(crtc_state);

		GB_PRINT_INFO("drm_atomic_get_crtc_state %d\n", GB02FUNC1573(crtc)->crtc_id);

		drm_for_each_plane(plane, dev) {

			plane_state = drm_atomic_get_new_plane_state(state, plane) ? \
				drm_atomic_get_new_plane_state(state, plane) : plane->state;

			if (plane_state && plane->state && plane_state->crtc == crtc) {
				struct drm_plane_state *new_plane_state =
					drm_atomic_get_plane_state(state, plane);
				if (IS_ERR(new_plane_state))
					continue;

				GB_PRINT_INFO("drm_atomic_get_plane_state %d\n",
						to_gbdc_plane_info(plane)->crtc_id);

				ret = drm_atomic_set_crtc_for_plane(new_plane_state, crtc);
				if (ret != 0)
					continue;
				drm_atomic_set_fb_for_plane(
					new_plane_state, plane_state->fb ? plane_state->fb : plane->state->fb);

			}
		}

	}
	drm_connector_list_iter_end(&conn_iter);

	return ret;
}
#endif

static void GB02FUNC1387(struct drm_device *dev)
{
	struct drm_crtc_commit *commit, *stall_commit = NULL;
	struct drm_crtc_commit *commit_last = NULL;
	struct drm_crtc *crtc = NULL;
	long ret = 0;

	if (!dev)
		return;

	drm_for_each_crtc(crtc, dev) {
		if (!crtc)
			continue;

		stall_commit = NULL;
		commit_last = NULL;
		spin_lock(&crtc->commit_lock);
		list_for_each_entry(commit, &crtc->commit_list, commit_entry) {
			if (!commit)
				continue;

			commit_last = commit;
		}
		if (commit_last)
			stall_commit = drm_crtc_commit_get(commit_last);
		spin_unlock(&crtc->commit_lock);

		if (!stall_commit)
			continue;

		ret = wait_for_completion_interruptible_timeout(&stall_commit->flip_done,
						100*HZ);
		if (ret == 0)
			DRM_ERROR("[CRTC:%d:%s] flip_done timed out\n",
				crtc->base.id, crtc->name);

		ret = wait_for_completion_interruptible_timeout(&stall_commit->cleanup_done,
						100*HZ);
		if (ret == 0)
			DRM_ERROR("[CRTC:%d:%s] cleanup_done timed out\n",
				crtc->base.id, crtc->name);

		drm_crtc_commit_put(stall_commit);

		GB_PRINT_INFO("%s: %d [CRTC:%d:%s] flip_done cleanup_done\n", __func__, __LINE__,
			crtc->base.id, crtc->name);
	}

	return;
}

int GB02FUNC1390(struct drm_device *dev,
			  void *data, struct drm_file *file_priv)
{
	struct drm_mode_atomic *arg = data;
	uint32_t __user *objs_ptr = (uint32_t __user *)(unsigned long)(arg->objs_ptr);
	uint32_t __user *count_props_ptr = (uint32_t __user *)(unsigned long)(arg->count_props_ptr);
	uint32_t __user *props_ptr = (uint32_t __user *)(unsigned long)(arg->props_ptr);
	uint64_t __user *prop_values_ptr = (uint64_t __user *)(unsigned long)(arg->prop_values_ptr);
	unsigned int copied_objs, copied_props;
	struct drm_atomic_state *state;
	struct drm_modeset_acquire_ctx ctx;
	struct GB02STR161 *fence_state;
	int ret = 0;
	unsigned int i, j, num_fences;
	struct mutex *mutex = NULL;

	GB_PRINT_INFO("%s: %d\n", __func__, __LINE__);

	/* disallow for drivers not supporting atomic: */
	if (!drm_core_check_feature(dev, DRIVER_ATOMIC))
		return -EOPNOTSUPP;

	/* disallow for userspace that has not enabled atomic cap (even
	 * though this may be a bit overkill, since legacy userspace
	 * wouldn't know how to call this ioctl)
	 */
	if (!file_priv->atomic)
		return -EINVAL;

	if (arg->flags & ~DRM_MODE_ATOMIC_FLAGS)
		return -EINVAL;

	if (arg->reserved)
		return -EINVAL;

	if (arg->flags & DRM_MODE_PAGE_FLIP_ASYNC)
		return -EINVAL;

	/* can't test and expect an event at the same time. */
	if ((arg->flags & DRM_MODE_ATOMIC_TEST_ONLY) &&
			(arg->flags & DRM_MODE_PAGE_FLIP_EVENT))
		return -EINVAL;

	state = drm_atomic_state_alloc(dev);
	if (!state)
		return -ENOMEM;

	drm_modeset_acquire_init(&ctx, DRM_MODESET_ACQUIRE_INTERRUPTIBLE);
	state->acquire_ctx = &ctx;
	state->allow_modeset = !!(arg->flags & DRM_MODE_ATOMIC_ALLOW_MODESET);

retry:
	copied_objs = 0;
	copied_props = 0;
	fence_state = NULL;
	num_fences = 0;

	for (i = 0; i < arg->count_objs; i++) {
		uint32_t obj_id, count_props;
		struct drm_mode_object *obj;

		if (get_user(obj_id, objs_ptr + copied_objs)) {
			ret = -EFAULT;
			goto out;
		}

		obj = drm_mode_object_find(dev, file_priv, obj_id, DRM_MODE_OBJECT_ANY);
		if (!obj) {
			ret = -ENOENT;
			goto out;
		}

		if (!obj->properties) {
			drm_mode_object_put(obj);
			ret = -ENOENT;
			goto out;
		}

		if (get_user(count_props, count_props_ptr + copied_objs)) {
			drm_mode_object_put(obj);
			ret = -EFAULT;
			goto out;
		}

		copied_objs++;

		for (j = 0; j < count_props; j++) {
			uint32_t prop_id;
			uint64_t prop_value;
			struct drm_property *prop;

			if (get_user(prop_id, props_ptr + copied_props)) {
				drm_mode_object_put(obj);
				ret = -EFAULT;
				goto out;
			}

			prop = GB02FUNC1370(obj, prop_id);
			if (!prop) {
				drm_mode_object_put(obj);
				ret = -ENOENT;
				goto out;
			}

			if (copy_from_user(&prop_value,
					   prop_values_ptr + copied_props,
					   sizeof(prop_value))) {
				drm_mode_object_put(obj);
				ret = -EFAULT;
				goto out;
			}

			ret = GB02FUNC1371(state, file_priv,
						      obj, prop, prop_value);
			if (ret) {
				drm_mode_object_put(obj);
				goto out;
			}

			copied_props++;
		}

		drm_mode_object_put(obj);
	}

	ret = GB02FUNC1345(dev, state, arg, file_priv, &fence_state,
				&num_fences);
	if (ret)
		goto out;

	//if (arg->flags & DRM_MODE_ATOMIC_TEST_ONLY) {
	//	ret = drm_atomic_check_only(state);
	//} else if (arg->flags & DRM_MODE_ATOMIC_NONBLOCK) {
	//	ret = drm_atomic_nonblocking_commit(state);
	//} else {
	//	if (unlikely(drm_debug & DRM_UT_STATE))
	//		drm_atomic_print_state(state);

	//	ret = drm_atomic_commit(state);
	//}

	//GB02FUNC1430(dev);
	mutex = GB02FUNC1433(state);
	if (mutex)
		mutex_lock(mutex);

	if (GB02FUNC1376(state))
		GB_PRINT_ERR("GB02FUNC1376 fail\n");

	//TODO
	/*wait nonblock commit work finish*/
	GB02FUNC1387(dev);
	ret = drm_atomic_commit(state);
	GB02FUNC1674(state);
	if (mutex)
		mutex_unlock(mutex);
	//GB02FUNC1431(dev);

out:
	GB02FUNC1364(dev, state, fence_state, num_fences, !ret);

	if (ret == -EDEADLK) {
		drm_atomic_state_clear(state);
		ret = drm_modeset_backoff(&ctx);
		if (!ret)
			goto retry;
	}

	drm_atomic_state_put(state);

	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	return ret;
}

void GB02FUNC964(struct drm_connector *connector,
						struct drm_connector_state *state)
{
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev = GB02FUNC85(dev->dev_private);
	//struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	struct gbdc_connector_state *gbdc_state =
		to_gbdc_connector_state(state);

	drm_object_property_set_value(&connector->base,
					  gb_dev->kms_info.virtual_flag_property,
					  gbdc_state->is_virtual);
}

static int __maybe_unused GB02FUNC1403(struct drm_connector *connector,
						const struct GB02STR194 *infos)
{
	int ret = -EINVAL;
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev = GB02FUNC85(dev->dev_private);
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);

	unsigned int row = 0;
	unsigned int col = 0;
	unsigned int size = 0;
	unsigned int *data = (unsigned int*)infos;

	if (data) {
		row = data[2];
		col = data[3];

		size = sizeof(*infos) + (sizeof(struct GB02STR192) * row * col);
	}

	ret = drm_property_replace_global_blob(dev,
										&gbdc_connector->infinity_info_blob_ptr,
										size,
										infos,
										&connector->base,
										gb_dev->kms_info.infinity_info_property);

	return ret;
}

int GB02FUNC1406(struct drm_connector *connector)
{
	int ret = -EINVAL;
	struct drm_device *dev = connector->dev;
	struct GB02STR155 *gb_dev = GB02FUNC85(dev->dev_private);
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	struct GB02STR194 *infinity_infos;
	unsigned int size = GB02MAC2566;

	infinity_infos = kzalloc(size, GFP_KERNEL);
	if (infinity_infos) {
		ret = drm_property_replace_global_blob(dev,
										&gbdc_connector->infinity_info_blob_ptr,
										size,
										infinity_infos,
										&connector->base,
										gb_dev->kms_info.infinity_info_property);
		kfree(infinity_infos);
		infinity_infos = NULL;
	}

	return ret;
}

void GB02FUNC1408(struct drm_connector *connector,
									const struct GB02STR194 *infos,
									unsigned int size)
{
	struct gbdc_connector *gbdc_connector = to_gbdc_connector(connector);
	const struct GB02STR194 *infinity_infos = infos;

	if (infinity_infos && gbdc_connector->infinity_info_blob_ptr \
		&& gbdc_connector->infinity_info_blob_ptr->data \
		&& size <= gbdc_connector->infinity_info_blob_ptr->length)
		memcpy(gbdc_connector->infinity_info_blob_ptr->data, infinity_infos, size);
}

int GB02FUNC963(
						struct gbdc_connector *gbdc_connector,
						enum gbdc_infinity_info_type type,
						unsigned int pos,
						void *data)
{
	struct GB02STR194 *infinity_infos;
	struct GB02STR192 *infinity_info;

	if (!gbdc_connector || !data)
		return -EINVAL;

	if (!gbdc_connector->infinity_info_blob_ptr)
		return -EINVAL;

	infinity_infos =
		(struct GB02STR194 *)gbdc_connector->infinity_info_blob_ptr->data;

	if (!infinity_infos)
		return -EINVAL;

	if (type < INFINITY_CONN_ID) {
		switch (type) {
			case INFINITY_ID:
				infinity_infos->infinity_id = *(unsigned int*)data;
				break;
			case INFINITY_ENABLE:
				infinity_infos->infinity_enable = *(unsigned int*)data;
				break;
			case INFINITY_ROW:
				infinity_infos->infinity_row = *(unsigned int*)data;
				break;
			case INFINITY_COL:
				infinity_infos->infinity_col = *(unsigned int*)data;
				break;
			default:
				break;
		}
	} else {
		if (pos >= GB02MAC2558)
			return -EINVAL;

		infinity_info = &infinity_infos->infos[pos];
		GB_PRINT_INFO(\
			"%s: update pos %d type %d\n", __func__, pos, type);
		switch (type) {
			case INFINITY_CONN_ID:
				infinity_info->connector_id = *(unsigned int*)data;
				break;
			case INFINITY_CONN_STAT:
				infinity_info->connected = *(enum drm_connector_status*)data;
				break;
			case INFINITY_ORDER:
				infinity_info->order = *(struct GB02STR190*)data;
				break;
			case INFINITY_GEOMETRY:
				infinity_info->geometry = *(struct GB02STR191*)data;
				break;
			case INFINITY_PADDING:
				memcpy(infinity_info->padding, data, sizeof(infinity_info->padding));
				break;
			default:
				break;
		}
	}


	return 0;

}

int GB02FUNC1419(
						struct gbdc_connector *gbdc_connector,
						enum gbdc_infinity_info_type type,
						unsigned int pos,
						void *data)
{
	int row, col;
	unsigned int conn_id;
	struct GB02STR198 *infinity = NULL;
	unsigned long int flags;
	struct drm_device *dev = gbdc_connector->base.dev;
	int ret = 0;

	spin_lock_irqsave(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	gbdc_for_each_infinity(infinity, dev) {
		struct GB02STR194 *info = infinity->infinity_infos;

		if (!info)
			continue;

		conn_id = infinity->infinity_connector_id;

		if (conn_id != gbdc_connector->base.base.id)
			continue;

		row = infinity->infinity_infos->infinity_row;
		col = infinity->infinity_infos->infinity_col;

		if (pos < row * col) {
			switch (type) {
				case INFINITY_PADDING:
					memcpy(info->infos[pos].padding, data, sizeof(info->infos[pos].padding));
				break;
				default:
				break;
			}
		}
	}
	spin_unlock_irqrestore(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);

	return ret;

}

static bool GB02FUNC1423(struct GB02STR196 *infinity_set)
{
	return !!((infinity_set->infinity_id > 0)
			&& (infinity_set->infinity_row * infinity_set->infinity_col <= GB02MAC2693)
			&& (infinity_set->order.x < infinity_set->infinity_row)
			&& (infinity_set->order.y < infinity_set->infinity_col)
			);
}

static void __maybe_unused GB02FUNC1425(
		struct gbdc_connector_state *gbdc_old_con_state, struct gbdc_connector_state *gbdc_new_con_state)
{
	pr_info("gbdc_old_con_state->is_virtual=%d\n", \
		gbdc_old_con_state->is_virtual);
	pr_info("gbdc_old_con_state->gbdc_infinity_enable=%d\n", \
		gbdc_old_con_state->gbdc_infinity_enable);
	pr_info("gbdc_old_con_state->infinity_adj.infinity_row=%d\n", \
		gbdc_old_con_state->infinity_adj.infinity_row);
	pr_info("gbdc_old_con_state->infinity_adj.infinity_col=%d\n", \
		gbdc_old_con_state->infinity_adj.infinity_col);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[0].connector_id=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[0].connector_id);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[0].order.x=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[0].order.x);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[0].order.y=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[0].order.y);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[0].padding[0].top=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[0].padding[0].top);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[0].padding[0].right=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[0].padding[0].right);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[0].padding[0].bottom=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[0].padding[0].bottom);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[0].padding[0].left=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[0].padding[0].left);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[1].connector_id=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[1].connector_id);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[1].order.x=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[1].order.x);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[1].order.y=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[1].order.y);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[1].padding[1].top=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[1].padding[1].top);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[1].padding[1].right=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[1].padding[1].right);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[1].padding[1].bottom=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[1].padding[1].bottom);
	pr_info("gbdc_old_con_state->infinity_adj.adjusts[1].padding[1].left=%d\n", \
		gbdc_old_con_state->infinity_adj.adjusts[1].padding[1].left);

	pr_info("gbdc_new_con_state->is_virtual=%d\n", \
		gbdc_new_con_state->is_virtual);
	pr_info("gbdc_new_con_state->gbdc_infinity_enable=%d\n", \
		gbdc_new_con_state->gbdc_infinity_enable);
	pr_info("gbdc_new_con_state->infinity_adj.infinity_row=%d\n", \
		gbdc_new_con_state->infinity_adj.infinity_row);
	pr_info("gbdc_new_con_state->infinity_adj.infinity_col=%d\n", \
		gbdc_new_con_state->infinity_adj.infinity_col);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[0].connector_id=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[0].connector_id);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[0].order.x=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[0].order.x);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[0].order.y=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[0].order.y);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[0].padding[0].top=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[0].padding[0].top);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[0].padding[0].right=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[0].padding[0].right);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[0].padding[0].bottom=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[0].padding[0].bottom);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[0].padding[0].left=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[0].padding[0].left);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[1].connector_id=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[1].connector_id);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[1].order.x=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[1].order.x);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[1].order.y=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[1].order.y);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[1].padding[1].top=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[1].padding[1].top);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[1].padding[1].right=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[1].padding[1].right);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[1].padding[1].bottom=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[1].padding[1].bottom);
	pr_info("gbdc_new_con_state->infinity_adj.adjusts[1].padding[1].left=%d\n", \
		gbdc_new_con_state->infinity_adj.adjusts[1].padding[1].left);
}

inline void GB02FUNC1430(struct drm_device *dev)
{
	mutex_lock(
		&GB02FUNC85((dev)->dev_private)->kms_info.gbdc_infinity_atomic_mutex);
}

inline void GB02FUNC1431(struct drm_device *dev)
{
	mutex_unlock(
		&GB02FUNC85((dev)->dev_private)->kms_info.gbdc_infinity_atomic_mutex);
}

struct mutex *GB02FUNC1433(struct drm_atomic_state *state)
{
	struct drm_device *dev = state->dev;
	struct GB02STR198 *infinity = NULL;
	struct drm_connector *conn = NULL;
	struct drm_connector_state *conn_state = NULL;
	bool found_virtual = false;
	struct drm_connector_list_iter conn_iter;
	struct gbdc_connector *gbdc_connector;

	gbdc_for_each_infinity(infinity, dev) {
		if (!infinity || !infinity->infinity_infos)
			continue;

		drm_connector_list_iter_begin(dev, &conn_iter);
		drm_for_each_connector_iter(conn, &conn_iter) {

			conn_state = drm_atomic_get_new_connector_state(state, conn) ? \
			drm_atomic_get_new_connector_state(state, conn) : conn->state;

			if (conn->base.id == infinity->infinity_connector_id \
				&& conn_state && conn == conn_state->connector) {

				found_virtual = true;
				break;
			}
		}
		if (conn)
			drm_connector_get(conn);
		drm_connector_list_iter_end(&conn_iter);

		if (found_virtual) {
			gbdc_connector = to_gbdc_connector(conn);
			drm_connector_put(conn);
			return &gbdc_connector->gbdc_infinity_atomic_mutex;
		}
		if (conn)
			drm_connector_put(conn);
	}

	return NULL;
}

struct mutex *GB02FUNC1437(struct drm_atomic_state *state)
{
	struct drm_device *dev = state->dev;
	struct GB02STR198 *infinity = NULL;
	struct drm_connector *conn = NULL;
	struct drm_connector_state *conn_state = NULL;
	bool found_virtual = false;
	struct drm_connector_list_iter conn_iter;
	struct gbdc_connector *gbdc_connector;

	gbdc_for_each_infinity(infinity, dev) {
		if (!infinity || !infinity->infinity_infos)
			continue;

		drm_connector_list_iter_begin(dev, &conn_iter);
		drm_for_each_connector_iter(conn, &conn_iter) {

			conn_state = drm_atomic_get_new_connector_state(state, conn) ? \
			drm_atomic_get_new_connector_state(state, conn) : conn->state;

			if (conn->base.id == infinity->infinity_connector_id \
				&& conn_state && conn == conn_state->connector) {

				found_virtual = true;
				break;
			}
		}
		if (conn)
			drm_connector_get(conn);
		drm_connector_list_iter_end(&conn_iter);

		if (found_virtual) {
			gbdc_connector = to_gbdc_connector(conn);
			drm_connector_put(conn);
			return &gbdc_connector->gbdc_infinity_mutex;
		}
		if (conn)
			drm_connector_put(conn);
	}

	return NULL;
}

static bool GB02FUNC1442(struct gbdc_connector_state *state)
{
	int i, j, row, col;

	row = state->infinity_adj.infinity_row;
	col = state->infinity_adj.infinity_col;

	if (row * col <= 0)
		return false;

	if (row * col > GB02MAC2558) {
		GB_PRINT_ERR("%s: infinity adjust check error layout(%dx%d)\n", __func__, \
			row, \
			col);
		return false;
	}

	for (j = 0; j < col; j++) {
		for (i = 0; i < row; i++) {
			if (state->infinity_adj.adjusts[j * row + i].order.x >= row \
				|| state->infinity_adj.adjusts[j * row + i].order.y >= col) {
				GB_PRINT_ERR("%s: infinity adjust check error order(%d,%d)(%dx%d)\n", \
				__func__, state->infinity_adj.adjusts[j * row + i].order.x, \
				state->infinity_adj.adjusts[j * row + i].order.y, row, col);
				return false;
			}
		}
	}

	return true;
}

static bool GB02FUNC1443(struct gbdc_connector_state *state)
{
	int row, col;

	row = state->infinity_set.infinity_row;
	col = state->infinity_set.infinity_col;

	if (row * col > GB02MAC2558) {
		GB_PRINT_ERR("%s: infinity set check error layout(%dx%d)\n", __func__, \
			row, \
			col);
		return false;
	}

	if (state->infinity_set.order.x >= row \
		|| state->infinity_set.order.y >= col) {
		GB_PRINT_ERR("%s: infinity set check error order(%d,%d)(%dx%d)\n", \
		__func__, state->infinity_set.order.x, \
		state->infinity_set.order.y, row, col);
		return false;
	}

	return true;
}

static bool GB02FUNC1446(
		struct gbdc_connector_state *old_con_state, struct gbdc_connector_state *new_con_state)
{
	if (false) GB02FUNC1425(old_con_state, new_con_state);
	if (!GB02FUNC1442(new_con_state))
		return false;

	if ((old_con_state->infinity_adj.infinity_row != \
			new_con_state->infinity_adj.infinity_row) \
		|| (old_con_state->infinity_adj.infinity_col != \
			new_con_state->infinity_adj.infinity_col)) {
		GB_PRINT_INFO("%s: infinity adjust check (%dx%d-%dx%d)\n", __func__, \
			old_con_state->infinity_adj.infinity_row, old_con_state->infinity_adj.infinity_col, \
			new_con_state->infinity_adj.infinity_row, new_con_state->infinity_adj.infinity_col);
		return true;
	}

	return !!memcmp(old_con_state->infinity_adj.adjusts, \
		new_con_state->infinity_adj.adjusts, \
		old_con_state->infinity_adj.infinity_row * \
		old_con_state->infinity_adj.infinity_col * \
		sizeof(struct GB02STR193));
}

/*avoid to lead endless loop*/
static bool GB02FUNC1447(
					struct gbdc_connector *gbdc_connector_head,
					struct gbdc_connector *gbdc_connector)
{
	struct gbdc_connector *gbdc_conn;
	bool exist = false;

	list_for_each_entry(gbdc_conn, &gbdc_connector_head->head, head) {
		if (gbdc_conn == gbdc_connector)
			exist = true;
	}

	return exist;
}

static bool GB02FUNC1450(
					struct gbdc_crtc *gbdc_crtc_head,
					struct gbdc_crtc *gbdc_crtc)
{
	struct gbdc_crtc *gbdc_crtc_pos;
	bool exist = false;

	list_for_each_entry(gbdc_crtc_pos, &gbdc_crtc_head->head, head) {
		if (gbdc_crtc_pos == gbdc_crtc)
			exist = true;
	}

	return exist;
}

static bool GB02FUNC1454(
					struct gbdc_plane *gbdc_plane_head,
					struct gbdc_plane *gbdc_plane)
{
	struct gbdc_plane *gbdc_plane_pos;
	bool exist = false;

	list_for_each_entry(gbdc_plane_pos, &gbdc_plane_head->head, head) {
		if (gbdc_plane_pos == gbdc_plane)
			exist = true;
	}

	return exist;
}

static bool GB02FUNC1456(
					struct list_head *gbdc_ins_head,
					struct GB02STR198 *gbdc_ins)
{
	struct GB02STR198 *gbdc_instance_pos;
	bool exist = false;

	list_for_each_entry(gbdc_instance_pos, gbdc_ins_head, head) {
		if (gbdc_instance_pos == gbdc_ins)
			exist = true;
	}

	return exist;
}

static void GB02FUNC1458(
					struct gbdc_connector *gbdc_connector_head,
					struct gbdc_connector *gbdc_connector)
{
	if (gbdc_connector_head && gbdc_connector) {
		GB_PRINT_INFO("%s: add connector%d to connector%d\n", __func__, \
			gbdc_connector->connector_id, gbdc_connector_head->connector_id);
		if (!GB02FUNC1447(gbdc_connector_head, gbdc_connector))
			list_add_tail(&gbdc_connector->head, &gbdc_connector_head->head);
		else
			GB_PRINT_ERR("%s: error connector%d exist in connector%d\n", __func__, \
			gbdc_connector->connector_id, gbdc_connector_head->connector_id);
	}
}

static struct GB02STR198 *GB02FUNC1460(
							struct drm_device *dev,
							struct drm_atomic_state *state,
							struct gbdc_connector_state *new_con_state)
{
	struct GB02STR198 *instance = NULL;
	struct GB02STR194 *infinity_infos;
	struct GB02STR192 *infinity_info;
	union u_info_changed *change;
	int size;

	int row = new_con_state->infinity_set.infinity_row;
	int col = new_con_state->infinity_set.infinity_col;
	int orderx = new_con_state->infinity_set.order.x;
	int ordery = new_con_state->infinity_set.order.y;
	unsigned int infinity_id = new_con_state->infinity_set.infinity_id;

	struct drm_connector *conn = NULL;
	struct drm_connector_list_iter conn_iter;
	bool found_virtual = false;
	struct gbdc_connector *gbdc_connector;
	struct drm_connector_state *connector_state;

	if (infinity_id <= 0)
		return NULL;

	GB_PRINT_INFO("%s: create infinity %d:%dx%d\n", __func__, infinity_id, row, col);

	//mutex_lock(&dev->mode_config.mutex);
	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		gbdc_connector = to_gbdc_connector(conn);
		connector_state = conn->state ? conn->state : \
				(drm_atomic_get_new_connector_state(state, conn) ? \
				drm_atomic_get_new_connector_state(state, conn) : \
				drm_atomic_get_old_connector_state(state, conn));

		if (!connector_state)
			GB_PRINT_INFO("%s: drm_atomic_get_connector_state NULL\n", __func__);

		if (gbdc_connector->virt && !gbdc_connector->infinity_inuse && \
			!(connector_state ? \
			to_gbdc_connector_state(connector_state)->gbdc_infinity_enable : false)) {
			found_virtual = true;
			gbdc_connector->infinity_inuse = true;
			break;
		}
	}
	if (conn)
		drm_connector_get(conn);
	drm_connector_list_iter_end(&conn_iter);
	//mutex_unlock(&dev->mode_config.mutex);

	if (!found_virtual) {
		GB_PRINT_ERR(\
			"%s: there is no suitable virtual connector for creating infinity %d:%dx%d\n", \
			__func__, infinity_id, row, col);
		if (conn)
			drm_connector_put(conn);
		return NULL;
	}

	if (row * col > GB02MAC2558) {
		GB_PRINT_ERR(\
			"%s: creating infinity %d:%dx%d error\n", \
			__func__, infinity_id, row, col);
		if (conn)
			drm_connector_put(conn);
		return NULL;
	}

	size = sizeof(*infinity_infos) + \
		(sizeof(struct GB02STR192) * GB02MAC2558);
	infinity_infos = kzalloc(size, GFP_KERNEL);
	change = kzalloc(sizeof(union u_info_changed) * GB02MAC2558, GFP_KERNEL);
	instance = kzalloc(sizeof(*instance), GFP_KERNEL);
	if (instance && infinity_infos && change) {

		infinity_infos->infinity_id = infinity_id;
		infinity_infos->infinity_enable = 1;
		infinity_infos->infinity_row = row;
		infinity_infos->infinity_col = col;

		infinity_info = &infinity_infos->infos[0/*row * ordery + orderx*/];
		infinity_info->connector_id = 0;//new_con_state->base.connector->base.id;
		infinity_info->connected = connector_status_disconnected;
		infinity_info->order.x = 0;//orderx;
		infinity_info->order.y = 0;//ordery;
		infinity_info->geometry.x = 0;
		infinity_info->geometry.y = 0;
		infinity_info->geometry.w = 0;
		infinity_info->geometry.h = 0;

		change[row * ordery + orderx].info_changed = 0;
		instance->infinity_infos = infinity_infos;
		instance->uinfo_changed = change;
		instance->changed = true;
		instance->created = true;
		instance->infinity_connector_id = conn->base.id;
		INIT_LIST_HEAD(&instance->head);
		gbdc_connector->infinity_row = row;
		gbdc_connector->infinity_col = col;
		instance->gbdc_connector = gbdc_connector;
	}

	if (conn)
		drm_connector_put(conn);

	return instance;
}

static enum drm_mode_status
drm_mode_validate_flag(const struct drm_display_mode *mode,
		       int flags)
{
	if ((mode->flags & DRM_MODE_FLAG_INTERLACE) &&
	    !(flags & DRM_MODE_FLAG_INTERLACE))
		return MODE_NO_INTERLACE;

	if ((mode->flags & DRM_MODE_FLAG_DBLSCAN) &&
	    !(flags & DRM_MODE_FLAG_DBLSCAN))
		return MODE_NO_DBLESCAN;

	if ((mode->flags & DRM_MODE_FLAG_3D_MASK) &&
	    !(flags & DRM_MODE_FLAG_3D_MASK))
		return MODE_NO_STEREO;

	return MODE_OK;
}

static int GB02FUNC1468(struct drm_connector *connector)
{
	struct drm_cmdline_mode *cmdline_mode;
	struct drm_display_mode *mode;

	cmdline_mode = &connector->cmdline_mode;
	if (!cmdline_mode->specified)
		return 0;

	/* Only add a GTF mode if we find no matching probed modes */
	list_for_each_entry(mode, &connector->probed_modes, head) {
		if (mode->hdisplay != cmdline_mode->xres ||
		mode->vdisplay != cmdline_mode->yres)
			continue;

		if (cmdline_mode->refresh_specified) {
			/* The probed mode's vrefresh is set until later */
			if (drm_mode_vrefresh(mode) != cmdline_mode->refresh)
				continue;
		}

		return 0;
	}

	mode = drm_mode_create_from_cmdline_mode(connector->dev, cmdline_mode);
	if (mode == NULL)
		return 0;

	drm_mode_probed_add(connector, mode);
	return 1;
}

int
drm_connector_mode_valid(struct drm_connector *connector,
			 struct drm_display_mode *mode,
			 enum drm_mode_status *status)
{
	const struct drm_connector_helper_funcs *connector_funcs =
		connector->helper_private;
	int ret = 0;

	if (!connector_funcs)
		*status = MODE_OK;
	/* else if (connector_funcs->mode_valid_ctx)
		ret = connector_funcs->mode_valid_ctx(connector, mode, ctx,
						      status);*/
	else if (connector_funcs->mode_valid)
		*status = connector_funcs->mode_valid(connector, mode);
	else
		*status = MODE_OK;

	return ret;
}

enum drm_mode_status GB02FUNC1471(struct drm_encoder *encoder,
					    const struct drm_display_mode *mode)
{
	const struct drm_encoder_helper_funcs *encoder_funcs =
		encoder->helper_private;

	if (!encoder_funcs || !encoder_funcs->mode_valid)
		return MODE_OK;

	return encoder_funcs->mode_valid(encoder, mode);
}

enum drm_mode_status GB02FUNC1472(struct drm_crtc *crtc,
					 const struct drm_display_mode *mode)
{
	const struct drm_crtc_helper_funcs *crtc_funcs = crtc->helper_private;

	if (!crtc_funcs || !crtc_funcs->mode_valid)
		return MODE_OK;

	return crtc_funcs->mode_valid(crtc, mode);
}

static int
drm_mode_validate_pipeline(struct drm_display_mode *mode,
			   struct drm_connector *connector,
			   enum drm_mode_status *status)
{
	struct drm_device *dev = connector->dev;
	struct drm_encoder *encoder;
	int ret;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0))
	int i;
#endif

	/* Step 1: Validate against connector */
	ret = drm_connector_mode_valid(connector, mode, status);
	if (ret || *status != MODE_OK)
		return ret;

	/* Step 2: Validate against encoders and crtcs */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0))
	drm_connector_for_each_possible_encoder(connector, encoder, i) {
#else
	drm_connector_for_each_possible_encoder(connector, encoder) {
#endif
#if !(LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
		struct drm_bridge *bridge;
#endif
		struct drm_crtc *crtc;

		*status = GB02FUNC1471(encoder, mode);
		if (*status != MODE_OK) {
			/* No point in continuing for crtc check as this encoder
			 * will not accept the mode anyway. If all encoders
			 * reject the mode then, at exit, ret will not be
			 * MODE_OK. */
			continue;
		}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
		*status = drm_bridge_mode_valid(encoder->bridge, mode);
		if (*status != MODE_OK) {
			/* There is also no point in continuing for crtc check
			 * here. */
			continue;
		}
#else
		bridge = drm_bridge_chain_get_first_bridge(encoder);
		*status = drm_bridge_chain_mode_valid(bridge,
						      &connector->display_info,
						      mode);
		if (*status != MODE_OK) {
			/* There is also no point in continuing for crtc check
			 * here. */
			continue;
		}
#endif

		drm_for_each_crtc(crtc, dev) {
			if (!drm_encoder_crtc_ok(encoder, crtc))
				continue;

			*status = GB02FUNC1472(crtc, mode);
			if (*status == MODE_OK) {
				/* If we get to this point there is at least
				 * one combination of encoder+crtc that works
				 * for this mode. Lets return now. */
				return 0;
			}
		}
	}

	return 0;
}

static int GB02FUNC1474(struct drm_connector *connector,
					uint32_t maxX, uint32_t maxY)
{
	struct drm_device *dev = connector->dev;
	struct drm_display_mode *mode;
	const struct drm_connector_helper_funcs *connector_funcs =
		connector->helper_private;
	int count = 0, ret;
	int mode_flags = 0;
	bool verbose_prune = true;

	/* set all old modes to the stale state */
	list_for_each_entry(mode, &connector->modes, head)
		mode->status = MODE_STALE;

	count = (*connector_funcs->get_modes)(connector);

	/*
	 * Fallback for when DDC probe failed in drm_get_edid() and thus skipped
	 * override/firmware EDID.
	 */
	if (count == 0 && connector->status == connector_status_connected)
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
		count = drm_add_override_edid_modes(connector);
#else
		count = drm_edid_override_connector_update(connector);
#endif

	if (count == 0 && connector->status == connector_status_connected)
                count = drm_add_modes_noedid(connector, 1024, 768);
	count += GB02FUNC1468(connector);
	if (count == 0)
		goto prune;

	drm_connector_list_update(connector);

	if (connector->interlace_allowed)
		mode_flags |= DRM_MODE_FLAG_INTERLACE;
	if (connector->doublescan_allowed)
                mode_flags |= DRM_MODE_FLAG_DBLSCAN;
	if (connector->stereo_allowed)
                mode_flags |= DRM_MODE_FLAG_3D_MASK;

	list_for_each_entry(mode, &connector->modes, head) {
		if (mode->status != MODE_OK)
			continue;

		mode->status = drm_mode_validate_driver(dev, mode);
		if (mode->status != MODE_OK)
			continue;

		mode->status = drm_mode_validate_size(mode, maxX, maxY);
		if (mode->status != MODE_OK)
			continue;

		mode->status = drm_mode_validate_flag(mode, mode_flags);
		if (mode->status != MODE_OK)
			continue;

		ret = drm_mode_validate_pipeline(mode, connector,
						 &mode->status);
		if (ret) {
			#if 0
			drm_dbg_kms(dev,
				    "drm_mode_validate_pipeline failed: %d\n",
				    ret);

			if (drm_WARN_ON_ONCE(dev, ret != -EDEADLK)) {
				mode->status = MODE_ERROR;
			} else {
				
			}
			#endif
		}

		if (mode->status != MODE_OK)
			continue;
		mode->status = drm_mode_validate_ycbcr420(mode, connector);
	}

prune:
	drm_mode_prune_invalid(dev, &connector->modes, verbose_prune);

	if (list_empty(&connector->modes))
		return 0;

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 9, 0))
	list_for_each_entry(mode, &connector->modes, head)
		mode->vrefresh = drm_mode_vrefresh(mode);
#endif

	drm_mode_sort(&connector->modes);

	DRM_DEBUG_KMS("[CONNECTOR:%d:%s] probe modes :\n", connector->base.id, connector->name);
	list_for_each_entry(mode, &connector->modes, head) {
		drm_mode_set_crtcinfo(mode, CRTC_INTERLACE_HALVE_V);
		drm_mode_debug_printmodeline(mode);
	}

	return count;
}

static int GB02FUNC1478(struct drm_device *dev,
								struct drm_atomic_state *state,
								struct GB02STR198 *infinity,
								struct drm_connector *connector,
								struct gbdc_connector_state *con_state)
{
	struct GB02STR194 *infinity_infos = infinity->infinity_infos;
	union u_info_changed *info_changed = infinity->uinfo_changed;
	struct GB02STR192 *infinity_info;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
	int ret = 0;
	int i = 0;
	unsigned long int flags;
	bool mutex_locked = false;

	int row = con_state->infinity_set.infinity_row;
	int col = con_state->infinity_set.infinity_col;
	int orderx = con_state->infinity_set.order.x;
	int ordery = con_state->infinity_set.order.y;

	if (row != infinity_infos->infinity_row || col != infinity_infos->infinity_col) {
		struct GB02STR198 *instance;

		GB_PRINT_ERR("BUG: %s: %d: impossible ????????????\n", \
			__func__, con_state->infinity_set.infinity_id);
		GB_PRINT_ERR("%s: change %d:%dx%d to %d:%dx%d\n", __func__, \
			con_state->infinity_set.infinity_id, infinity_infos->infinity_row, \
			infinity_infos->infinity_col, \
			con_state->infinity_set.infinity_id, row, col);

		instance = GB02FUNC1460(dev,
								state, con_state);
		if (instance) {
			infinity_infos = instance->infinity_infos;
			GB_PRINT_ERR("%s:replace from infinity %d of connector %d" \
				"to infinity %d of connector %d\n", __func__, \
				infinity->infinity_infos->infinity_id, infinity->infinity_connector_id, \
				instance->infinity_infos->infinity_id, instance->infinity_connector_id);
			info_changed = instance->uinfo_changed;
			spin_lock_irqsave(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
			list_replace_init(&infinity->head, &instance->head);
			spin_unlock_irqrestore(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
			kfree(infinity->uinfo_changed);
			infinity->uinfo_changed = NULL;
			kfree(infinity->infinity_infos);
			infinity->infinity_infos = NULL;
			kfree(infinity);
			infinity = NULL;
		}
	}

	GB_PRINT_INFO("%s: update infinity %d:%dx%d pos:%dx%d\n", __func__, \
		con_state->infinity_set.infinity_id, row, col, orderx, ordery);

	ret = 0;

//	for (i = 0; i < GB02MAC2558; i++)
//		if (infinity_infos->infos[i].connector_id == 0)
//			break;

	i = ordery * row + orderx;

	if (i >= GB02MAC2558) {
		GB_PRINT_ERR("%s: update infinity %d:%dx%d pos:%dx%d error\n", __func__, \
		con_state->infinity_set.infinity_id, row, col, orderx, ordery);
		return -EINVAL;
	}

	infinity_info = &infinity_infos->infos[i];
	if (infinity_info->connector_id != connector->base.id) {
		GB_PRINT_INFO("update connector_id %d to %d\n", \
			infinity_info->connector_id, connector->base.id);
		infinity_info->connector_id = connector->base.id;

		//info_changed[row * ordery + orderx].st.id_changed = 1;
	}

	if (orderx != infinity_info->order.x || ordery != infinity_info->order.y) {
		GB_PRINT_INFO("update order(%d,%d) to (%d,%d)\n", \
			infinity_info->order.x, infinity_info->order.y, orderx, ordery);
		infinity_info->order.x = orderx;
		infinity_info->order.y = ordery;

		//info_changed[row * ordery + orderx].st.order_changed = 1;
	}

	if (memcmp(infinity_info->padding, con_state->infinity_set.padding,
							sizeof(con_state->infinity_set.padding))) {
		for (i = 0; i < GB02MAC2560; i++)
			if (infinity_info->padding[i].width || infinity_info->padding[i].height \
				|| con_state->infinity_set.padding[i].width || \
				con_state->infinity_set.padding[i].height)
				GB_PRINT_INFO("update padding(%d,%d,%d,%d,%d,%d) "\
					"to (%d,%d,%d,%d,%d,%d)\n", \
					infinity_info->padding[i].width, infinity_info->padding[i].height, \
					infinity_info->padding[i].top, infinity_info->padding[i].right, \
					infinity_info->padding[i].bottom, infinity_info->padding[i].left, \
					con_state->infinity_set.padding[i].width, \
					con_state->infinity_set.padding[i].height, \
					con_state->infinity_set.padding[i].top, \
					con_state->infinity_set.padding[i].right, \
					con_state->infinity_set.padding[i].bottom, \
					con_state->infinity_set.padding[i].left);
		memcpy(infinity_info->padding, con_state->infinity_set.padding,
							sizeof(con_state->infinity_set.padding));
		//info_changed[row * ordery + orderx].st.padding_changed = 1;
	}

	//infinity->changed |= !!info_changed[row * ordery + orderx].info_changed;

	/* set infinity_set to 0, can also be retained */
	memset(&con_state->infinity_set, 0, sizeof(con_state->infinity_set));
	/*disable crtcs when create a infinity*/
	#if 0
	if (con_state->base.crtc) {
		struct drm_crtc_state *new_crtc_state =
			drm_atomic_get_new_crtc_state(state, con_state->base.crtc);
		if (new_crtc_state)
			to_gbdc_crtc_state(new_crtc_state)->need_disable = true;
	} else
		con_state->crtc_need_disable = true;
	#endif

	gbdc_conn->infinity_row = row;
	gbdc_conn->infinity_col = col;
	gbdc_conn->order.x = orderx;
	gbdc_conn->order.y = ordery;
	memcpy(gbdc_conn->padding, infinity_info->padding, sizeof(infinity_info->padding));


	if (unlikely(GB02FUNC874(connector) != connector_status_connected))
		msleep(1000);

	if (likely(GB02FUNC874(connector) == connector_status_connected)) {

		if (!GB02FUNC891(connector))
			GB_PRINT_ERR("%s: connector%d wait edid error\n", __func__, \
                		gbdc_conn->connector_id);

		mutex_locked = mutex_is_locked(&dev->mode_config.mutex);
		if (!mutex_locked)
			mutex_lock(&dev->mode_config.mutex);

		if (!GB02FUNC1474(connector,
				dev->mode_config.max_width, dev->mode_config.max_height))
			GB_PRINT_ERR("%s: connector%d refill modes error\n", __func__, \
                                gbdc_conn->connector_id);

		GB02FUNC894(connector);

		if (!mutex_locked)
			mutex_unlock(&dev->mode_config.mutex);
	}

	GB02FUNC1458(infinity->gbdc_connector, gbdc_conn);

	return ret;
}

static int GB02FUNC1486(struct drm_atomic_state *state,
									struct drm_crtc *crtc,
									bool flag)
{
	struct gbdc_crtc *gbdc_crtc = NULL;
	struct gbdc_crtc *n_crtc;
	struct gbdc_crtc_state *gbdc_crtc_state = NULL;
	struct drm_crtc_state *old_crtc_state = NULL;
	struct drm_crtc_state *new_crtc_state = NULL;

	GB_PRINT_INFO("******%s \n", __func__);

	if (!crtc)
		return -1;

	old_crtc_state = drm_atomic_get_old_crtc_state(state, crtc) ? \
		drm_atomic_get_old_crtc_state(state, crtc) : crtc->state;
	gbdc_crtc_state = to_gbdc_crtc_state(old_crtc_state);
	if (!gbdc_crtc_state)
		return 0;

	if (flag)
		GB02FUNC1573(crtc)->need_disable =
			gbdc_crtc_state->need_disable_changed =
				gbdc_crtc_state->need_disable = false;

	if (gbdc_crtc_state) {
		gbdc_for_each_gbdc_obj_safe(crtc) {
			if (!gbdc_crtc)
				continue;

			memset(&gbdc_crtc->adjusted_mode, 0, sizeof(gbdc_crtc->adjusted_mode));
			memset(&gbdc_crtc->order, 0, sizeof(gbdc_crtc->order));
			memset(&gbdc_crtc->geometry, 0, sizeof(gbdc_crtc->geometry));
			if (flag)
				gbdc_crtc->need_disable = false;
			GB_PRINT_INFO("%s:del crtc(%d):%d from old crtc(%d):%d\n", \
								__func__, gbdc_crtc->crtc_id, gbdc_crtc->base.base.id, \
								GB02FUNC1573(crtc)->crtc_id, \
								GB02FUNC1573(crtc)->base.base.id);
			GB02FUNC1026(&gbdc_crtc->base);
			list_del_init(&gbdc_crtc->head);
		}
		//TODO
		INIT_LIST_HEAD(&gbdc_crtc_state->infinity_crtc_list->head);
	}
	new_crtc_state = drm_atomic_get_new_crtc_state(state, crtc);
	gbdc_crtc_state = to_gbdc_crtc_state(new_crtc_state);
	if (!gbdc_crtc_state)
		return 0;

	if (flag)
		GB02FUNC1573(crtc)->need_disable =
			gbdc_crtc_state->need_disable_changed =
				gbdc_crtc_state->need_disable = false;

	if (gbdc_crtc_state) {
		gbdc_for_each_gbdc_obj_safe(crtc) {
			if (!gbdc_crtc)
				continue;

			memset(&gbdc_crtc->adjusted_mode, 0, sizeof(gbdc_crtc->adjusted_mode));
			memset(&gbdc_crtc->order, 0, sizeof(gbdc_crtc->order));
			memset(&gbdc_crtc->geometry, 0, sizeof(gbdc_crtc->geometry));
			if (flag)
				gbdc_crtc->need_disable = false;
			GB_PRINT_INFO("%s:del crtc(%d):%d to crtc(%d):%d\n", \
								__func__, gbdc_crtc->crtc_id, gbdc_crtc->base.base.id, \
								GB02FUNC1573(crtc)->crtc_id, \
								GB02FUNC1573(crtc)->base.base.id);
			list_del_init(&gbdc_crtc->head);
		}
		INIT_LIST_HEAD(&gbdc_crtc_state->infinity_crtc_list->head);
	}

	return 0;
}

static int GB02FUNC1489(struct drm_atomic_state *state,
									struct drm_plane *plane)
{
	struct gbdc_plane *gbdc_plane = NULL;
	struct gbdc_plane *n_plane;
	struct gbdc_plane_state *gbdc_plane_state = NULL;
	struct drm_plane_state *old_plane_state = NULL;
	struct drm_plane_state *new_plane_state = NULL;

	GB_PRINT_INFO("******%s \n", __func__);

	if (!plane)
		return -1;

	old_plane_state = drm_atomic_get_old_plane_state(state, plane) ? \
		drm_atomic_get_old_plane_state(state, plane) : plane->state;
	gbdc_plane_state = to_gbdc_plane_state(old_plane_state);
	if (!gbdc_plane_state)
		return 0;

	gbdc_for_each_gbdc_obj_safe(plane) {
		if (!gbdc_plane)
			continue;

		memset(&gbdc_plane->geometry, 0, sizeof(gbdc_plane->geometry));
		memset(&gbdc_plane->order, 0, sizeof(gbdc_plane->order));
		memset(&gbdc_plane->padding, 0, sizeof(gbdc_plane->padding));
		GB_PRINT_INFO("%s:del plane-%d(%d):%d from plane-%d(%d):%d\n", \
								__func__, plane->type, gbdc_plane->plane_id, \
								gbdc_plane->base.base.id, \
								plane->type, \
								to_gbdc_plane_info(plane)->plane_id, \
								to_gbdc_plane_info(plane)->base.base.id);
		list_del_init(&gbdc_plane->head);
	}
	INIT_LIST_HEAD(&gbdc_plane_state->infinity_plane_list->head);

	new_plane_state = drm_atomic_get_new_plane_state(state, plane);
	gbdc_plane_state = to_gbdc_plane_state(new_plane_state);
	if (!gbdc_plane_state)
		return 0;

	gbdc_for_each_gbdc_obj_safe(plane) {
		if (!gbdc_plane)
			continue;

		memset(&gbdc_plane->geometry, 0, sizeof(gbdc_plane->geometry));
		memset(&gbdc_plane->order, 0, sizeof(gbdc_plane->order));
		memset(&gbdc_plane->padding, 0, sizeof(gbdc_plane->padding));
		GB_PRINT_INFO("%s:del plane-%d(%d):%d from plane-%d(%d):%d\n", \
								__func__, plane->type, gbdc_plane->plane_id, \
								gbdc_plane->base.base.id, \
								plane->type, \
								to_gbdc_plane_info(plane)->plane_id, \
								to_gbdc_plane_info(plane)->base.base.id);
		list_del_init(&gbdc_plane->head);
	}
	INIT_LIST_HEAD(&gbdc_plane_state->infinity_plane_list->head);

	return 0;
}

static int GB02FUNC1494(struct drm_atomic_state *state,
									struct drm_connector *connector,
									bool flag)
{
	struct gbdc_connector *gbdc_connector = NULL;
	struct gbdc_connector *n_connector;
	struct gbdc_connector_state *gbdc_connector_state = NULL;
	struct drm_connector_state *old_connector_state = NULL;
	struct drm_connector_state *new_connector_state = NULL;

	GB_PRINT_INFO("******%s \n", __func__);

	if (!connector)
		return -1;

	old_connector_state = drm_atomic_get_old_connector_state(state, connector) ? \
		drm_atomic_get_old_connector_state(state, connector) : connector->state;
	gbdc_connector_state = to_gbdc_connector_state(old_connector_state);

	if (!gbdc_connector_state)
		return 0;

	if (flag)
		gbdc_connector_state->crtc_need_disable = false;

	gbdc_for_each_gbdc_obj_safe(connector) {
		if (!gbdc_connector)
			continue;

		gbdc_connector->order.x = 0;
		gbdc_connector->order.y = 0;
		if (gbdc_connector->base.state)
			memset(&to_gbdc_connector_state(gbdc_connector->base.state)->infinity_set, 0, \
				sizeof(to_gbdc_connector_state(gbdc_connector->base.state)->infinity_set));
		GB_PRINT_INFO("%s:del connector(%d):%d from connector(%d):%d\n", \
								__func__, gbdc_connector->connector_id, \
								gbdc_connector->base.base.id, \
								to_gbdc_connector(connector)->connector_id, \
								to_gbdc_connector(connector)->base.base.id);
		list_del_init(&gbdc_connector->head);
	}
	memset(&gbdc_connector_state->infinity_adj, 0, \
			GB02MAC2571);
	INIT_LIST_HEAD(&gbdc_connector_state->infinity_connector_list->head);
	gbdc_connector_state->gbdc_infinity_enable = false;
	GB_PRINT_INFO("******%s set gbdc_infinity_enable false\n", __func__);

	new_connector_state = drm_atomic_get_new_connector_state(state, connector);
	gbdc_connector_state = to_gbdc_connector_state(new_connector_state);
	if (!gbdc_connector_state)
		return 0;

	if (flag)
		gbdc_connector_state->crtc_need_disable = false;

	gbdc_for_each_gbdc_obj_safe(connector) {
		if (!gbdc_connector)
			continue;

		memset(&gbdc_connector->order, 0, sizeof(gbdc_connector->order));
		memset(&gbdc_connector->padding, 0, sizeof(gbdc_connector->padding));
		if (gbdc_connector->base.state)
			memset(&to_gbdc_connector_state(gbdc_connector->base.state)->infinity_set, 0, \
				sizeof(to_gbdc_connector_state(gbdc_connector->base.state)->infinity_set));
		GB_PRINT_INFO("%s:del connector(%d):%d from connector(%d):%d\n", \
								__func__, gbdc_connector->connector_id, \
								gbdc_connector->base.base.id, \
								to_gbdc_connector(connector)->connector_id, \
								to_gbdc_connector(connector)->base.base.id);
		list_del_init(&gbdc_connector->head);
	}
	memset(&gbdc_connector_state->infinity_adj, 0, \
			GB02MAC2571);

	INIT_LIST_HEAD(&gbdc_connector_state->infinity_connector_list->head);
	gbdc_connector_state->gbdc_infinity_enable = false;
	GB_PRINT_INFO("******%s set gbdc_infinity_enable false\n", __func__);

	return 0;
}

static int GB02FUNC1499(struct drm_atomic_state *state,
						struct GB02STR198 *infinity)
{
	int n, ret = 0;
	struct gbdc_connector_state *old_con_state;
	struct drm_connector *conn;
	struct drm_crtc_state *crtc_state;
	struct drm_plane_state *old_plane_state, *new_plane_state;
	struct drm_plane *plane;
	struct drm_connector_state *conn_state;
	struct drm_device *dev = state->dev;
	unsigned long int flags;
	bool success = false;
	struct drm_connector_list_iter conn_iter;
	//struct gbdc_crtc_state *gbdc_crtc_state;
	//struct gbdc_plane_state *gbdc_plane_state = NULL;

	if (!infinity)
		return ret;

	if (infinity->infinity_infos) {
		GB_PRINT_INFO("%s: destroy infinity %d:%dx%d\n", \
			__func__, infinity->infinity_infos->infinity_id, \
			infinity->infinity_infos->infinity_row, infinity->infinity_infos->infinity_col);
		/*TODO: infinity->infinity_connector_id */
		for_each_old_connector_in_state(state, conn, conn_state, n) {
			if (infinity->infinity_connector_id == conn->base.id) {
				struct gbdc_connector *gbdc_connector = to_gbdc_connector(conn);
				GB_PRINT_INFO("%s: find connector %d\n", __func__, conn->base.id);
				old_con_state = to_gbdc_connector_state(conn_state);
				crtc_state = conn_state->crtc ? conn_state->crtc->state : NULL;

				//for_each_oldnew_plane_in_state(state, plane,
				//			old_plane_state, new_plane_state, n) {
				drm_for_each_plane(plane, state->dev) {
					old_plane_state = drm_atomic_get_old_plane_state(state, plane);
					new_plane_state = drm_atomic_get_new_plane_state(state, plane);

					if ((new_plane_state && new_plane_state->crtc == conn_state->crtc) \
						|| (old_plane_state && old_plane_state->crtc == conn_state->crtc) \
						|| (plane->state && plane->state->crtc == conn_state->crtc))
						GB02FUNC1489(state, plane);
				}

				GB02FUNC1486(state, conn_state->crtc, true);
				GB02FUNC1494(state, conn, true);

				/*ret = GB02FUNC1403(conn,
							NULL);*/
				/*userspace uses one blob id, so do not use replaced blob, instead of a fixed one*/
				infinity->infinity_infos->infinity_enable = 0;
				GB02FUNC963(
					gbdc_connector, INFINITY_ENABLE, 0,
					&infinity->infinity_infos->infinity_enable);
				gbdc_connector->infinity_inuse = false;
				success = true;
				//if (mutex_is_locked(&gbdc_connector->gbdc_infinity_atomic_mutex))
				//	mutex_unlock(&gbdc_connector->gbdc_infinity_atomic_mutex);
				break;
			}
		}

		if (!success) {

			struct drm_crtc *crtc;
			struct drm_crtc_state *new_crtc_state;
			struct drm_crtc_state *old_crtc_state;
			int i;

			//TODO:not all crtc, but crtcs in infinity
			for_each_oldnew_crtc_in_state(state, crtc, old_crtc_state, new_crtc_state, i) {
				struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

				if (!new_crtc_state)
					continue;

				if (!GB02FUNC1691(dev,
											gbdc_crtc->crtc_id,
											infinity, false))
					continue;

				GB02FUNC1573(crtc)->need_disable =
					to_gbdc_crtc_state(new_crtc_state)->need_disable_changed =
						to_gbdc_crtc_state(new_crtc_state)->need_disable = false;

			}

			drm_connector_list_iter_begin(dev, &conn_iter);
			drm_for_each_connector_iter(conn, &conn_iter) {
				if (infinity->infinity_connector_id == conn->base.id) {
					struct gbdc_connector *gbdc_connector = to_gbdc_connector(conn);
					GB_PRINT_INFO("%s: 1find connector %d\n", __func__, conn->base.id);
					GB02FUNC1494(state, conn, true);

					/*ret = GB02FUNC1403(conn,
								NULL);*/
					/*userspace uses one blob id, so do not use replaced blob, instead of a fixed one*/
					infinity->infinity_infos->infinity_enable = 0;
					GB02FUNC963(
						gbdc_connector, INFINITY_ENABLE, 0,
						&infinity->infinity_infos->infinity_enable);
					to_gbdc_connector(conn)->infinity_inuse = false;
					success = true;
					//if (mutex_is_locked(&gbdc_connector->gbdc_infinity_atomic_mutex))
					//	mutex_unlock(&gbdc_connector->gbdc_infinity_atomic_mutex);
				}
			}
			drm_connector_list_iter_end(&conn_iter);
		}
	}

	if (!success)
		return ret;

	spin_lock_irqsave(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	list_del_init(&infinity->head);
	spin_unlock_irqrestore(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	kfree(infinity->uinfo_changed);
	infinity->uinfo_changed = NULL;
	kfree(infinity->infinity_infos);
	infinity->infinity_infos = NULL;
	kfree(infinity);
	infinity = NULL;

	return ret;
}

static int GB02FUNC1509(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct drm_crtc *crtc,
			struct drm_crtc_state *old_crtc_state,
			struct drm_crtc_state *new_crtc_state)
{
	struct GB02STR198 *infinity;
	struct drm_connector *conn;
	struct drm_connector_list_iter conn_iter;
	int ret = -1;

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		struct gbdc_connector *gbdc_connector = to_gbdc_connector(conn);
		struct drm_connector_state *connector_state =
				drm_atomic_get_new_connector_state(state, conn) ?
				drm_atomic_get_new_connector_state(state, conn) : conn->state;

		if (!gbdc_connector->virt)
			continue;

		if (!connector_state)
			continue;

		if(crtc != connector_state->crtc && \
			gbdc_connector->connector_id != GB02FUNC1573(crtc)->crtc_id)
			continue;

		gbdc_for_each_infinity(infinity, dev) {
			if (infinity && infinity->infinity_connector_id == conn->base.id) {
				infinity->mode_changed = true;
				infinity->geometry_changed = true;
				//infinity->changed |= true;
				ret = 0;
			}

		}
	}
	drm_connector_list_iter_end(&conn_iter);

	return ret;
}

static void GB02FUNC1514(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct drm_crtc *crtc,
			struct drm_crtc_state *new_crtc_state)
{
	struct drm_connector *conn;
	struct drm_connector_list_iter conn_iter;
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		//struct gbdc_connector *gbdc_connector = to_gbdc_connector(conn);
		struct drm_connector_state *connector_state =
				drm_atomic_get_new_connector_state(state, conn);

		//if (!gbdc_connector->virt)
		//	continue;

		if (!connector_state)
			continue;

		if (crtc != connector_state->crtc)
			continue;

		if (to_gbdc_connector_state(connector_state)->crtc_need_disable) {
			if (!to_gbdc_crtc_state(new_crtc_state)->need_disable) {
				GB_PRINT_INFO("==================crtc need disable\n");
				to_gbdc_crtc_state(new_crtc_state)->need_disable = true;
				gbdc_crtc->need_disable = true;
			}
			to_gbdc_connector_state(connector_state)->crtc_need_disable = false;
		}
	}
	drm_connector_list_iter_end(&conn_iter);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 18)
static bool
crtc_needs_disable(struct drm_crtc_state *old_state,
		   struct drm_crtc_state *new_state)
{
	/*
	 * No new_state means the crtc is off, so the only criteria is whether
	 * it's currently active or in self refresh mode.
	 */
	if (!new_state)
		return drm_atomic_crtc_effectively_active(old_state);

	/*
	 * We need to run through the crtc_funcs->disable() function if the crtc
	 * is currently on, if it's transitioning to self refresh mode, or if
	 * it's in self refresh mode and needs to be fully disabled.
	 */
	return old_state->active ||
	       (old_state->self_refresh_active && !new_state->enable) ||
	       new_state->self_refresh_active ||
		   to_gbdc_crtc_state(new_state)->need_disable;
}
#endif
static inline bool
gbdc_atomic_crtc_needs_modeset(const struct drm_crtc_state *state)
{
	return state->mode_changed || state->active_changed ||
	       state->connectors_changed ||
		   to_gbdc_crtc_state(state)->need_disable_changed;
}

static void
disable_outputs(struct drm_device *dev, struct drm_atomic_state *old_state)
{
	struct drm_connector *connector;
	struct drm_connector_state *old_conn_state, *new_conn_state;
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state, *new_crtc_state;
	int i;

	for_each_oldnew_connector_in_state(old_state, connector, old_conn_state, new_conn_state, i) {
		const struct drm_encoder_helper_funcs *funcs;
		struct drm_encoder *encoder;
		#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 6, 0)
		struct drm_bridge *bridge;
		#endif

		/* Shut down everything that's in the changeset and currently
		 * still on. So need to check the old, saved state. */
		if (!old_conn_state->crtc)
			continue;

		old_crtc_state = drm_atomic_get_old_crtc_state(old_state, old_conn_state->crtc);

		if (new_conn_state->crtc)
			new_crtc_state = drm_atomic_get_new_crtc_state(
						old_state,
						new_conn_state->crtc);
		else
			new_crtc_state = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 18)
		if (!crtc_needs_disable(old_crtc_state, new_crtc_state) ||
#else
		if (!old_crtc_state->active ||
#endif
		    !gbdc_atomic_crtc_needs_modeset(old_conn_state->crtc->state))
			continue;

		encoder = old_conn_state->best_encoder;

		/* We shouldn't get this far if we didn't previously have
		 * an encoder.. but WARN_ON() rather than explode.
		 */
		if (WARN_ON(!encoder))
			continue;

		funcs = encoder->helper_private;

		DRM_DEBUG_ATOMIC("disabling [ENCODER:%d:%s]\n",
				 encoder->base.id, encoder->name);

		/*
		 * Each encoder has at most one connector (since we always steal
		 * it away), so we won't call disable hooks twice.
		 */
		#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6,0)
		#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 18)
		drm_bridge_disable(encoder->bridge);
		#else
		drm_atomic_bridge_disable(encoder->bridge, old_state);
		#endif
		#else
		bridge = drm_bridge_chain_get_first_bridge(encoder);
		drm_atomic_bridge_chain_disable(bridge,old_state);
		#endif
		/* Right function depends upon target state. */
		if (funcs) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 18)
			if (funcs->atomic_disable)
				funcs->atomic_disable(encoder, old_state);
			else if (new_conn_state->crtc && funcs->prepare)
		#else
			if (new_conn_state->crtc && funcs->prepare)
		#endif
				funcs->prepare(encoder);
			else if (funcs->disable)
				funcs->disable(encoder);
			else if (funcs->dpms)
				funcs->dpms(encoder, DRM_MODE_DPMS_OFF);
		}

		#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 18)
		drm_bridge_post_disable(encoder->bridge);
		#else
		#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
		drm_atomic_bridge_post_disable(encoder->bridge, old_state);
		#else
		drm_atomic_bridge_chain_post_disable(bridge, old_state);
		#endif
		#endif
	}

	for_each_oldnew_crtc_in_state(old_state, crtc, old_crtc_state, new_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;
		int ret;

		/* Shut down everything that needs a full modeset. */
		if (!gbdc_atomic_crtc_needs_modeset(new_crtc_state))
			continue;

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 18)
		if (!crtc_needs_disable(old_crtc_state, new_crtc_state))
			continue;
		#else
		if (!old_crtc_state->active)
			continue;
		#endif

		funcs = crtc->helper_private;

		DRM_DEBUG_ATOMIC("disabling [CRTC:%d:%s]\n",
				 crtc->base.id, crtc->name);


		/* Right function depends upon target state. */
		if (new_crtc_state->enable && funcs->prepare)
			funcs->prepare(crtc);
		else if (funcs->atomic_disable)
			#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
			funcs->atomic_disable(crtc, old_crtc_state);
			#else
			funcs->atomic_disable(crtc,old_state);
			#endif
		else if (funcs->disable)
			funcs->disable(crtc);
		else if (funcs->dpms)
			funcs->dpms(crtc, DRM_MODE_DPMS_OFF);
		#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
		if (!(dev->irq_enabled && dev->num_crtcs))
		#else
		if (!drm_dev_has_vblank(dev))
		#endif
			continue;

		ret = drm_crtc_vblank_get(crtc);
		WARN_ONCE(ret != -EINVAL, "driver forgot to call drm_crtc_vblank_off()\n");
		if (ret == 0)
			drm_crtc_vblank_put(crtc);
	}
}

static void
crtc_set_mode(struct drm_device *dev, struct drm_atomic_state *old_state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *new_crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *new_conn_state;
	int i;

	for_each_new_crtc_in_state(old_state, crtc, new_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		if (!new_crtc_state->mode_changed)
			continue;

		funcs = crtc->helper_private;

		if (new_crtc_state->enable && funcs->mode_set_nofb) {
			DRM_DEBUG_ATOMIC("modeset on [CRTC:%d:%s]\n",
					 crtc->base.id, crtc->name);

			funcs->mode_set_nofb(crtc);
		}
	}

	for_each_new_connector_in_state(old_state, connector, new_conn_state, i) {
		const struct drm_encoder_helper_funcs *funcs;
		struct drm_encoder *encoder;
		struct drm_display_mode *mode, *adjusted_mode;
		#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 6, 0)
		struct drm_bridge *bridge;
		#endif
		if (!new_conn_state->best_encoder)
			continue;

		encoder = new_conn_state->best_encoder;
		funcs = encoder->helper_private;
		new_crtc_state = new_conn_state->crtc->state;
		mode = &new_crtc_state->mode;
		adjusted_mode = &new_crtc_state->adjusted_mode;

		if (!new_crtc_state->mode_changed)
			continue;

		DRM_DEBUG_ATOMIC("modeset on [ENCODER:%d:%s]\n",
				 encoder->base.id, encoder->name);

		/*
		 * Each encoder has at most one connector (since we always steal
		 * it away), so we won't call mode_set hooks twice.
		 */
		if (funcs && funcs->atomic_mode_set) {
			funcs->atomic_mode_set(encoder, new_crtc_state,
					       new_conn_state);
		} else if (funcs && funcs->mode_set) {
			funcs->mode_set(encoder, mode, adjusted_mode);
		}
		#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6 ,0)
		drm_bridge_mode_set(encoder->bridge, mode, adjusted_mode);
		#else
		bridge = drm_bridge_chain_get_first_bridge(encoder);
		drm_bridge_chain_mode_set(bridge, mode, adjusted_mode);
		#endif
	}
}

/**
 * drm_atomic_helper_commit_modeset_disables - modeset commit to disable outputs
 * @dev: DRM device
 * @old_state: atomic state object with old state structures
 *
 * This function shuts down all the outputs that need to be shut down and
 * prepares them (if required) with the new mode.
 *
 * For compatibility with legacy crtc helpers this should be called before
 * drm_atomic_helper_commit_planes(), which is what the default commit function
 * does. But drivers with different needs can group the modeset commits together
 * and do the plane commits at the end. This is useful for drivers doing runtime
 * PM since planes updates then only happen when the CRTC is actually enabled.
 */
void GB02FUNC1529(struct drm_device *dev,
					       struct drm_atomic_state *old_state)
{
	disable_outputs(dev, old_state);

	drm_atomic_helper_update_legacy_modeset_state(dev, old_state);
	#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 10 ,0)
	drm_atomic_helper_calc_timestamping_constants(old_state);
	#endif
	crtc_set_mode(dev, old_state);
}

static __maybe_unused int GB02FUNC1531(struct drm_atomic_state *state,
				     struct drm_connector *connector,
				     int mode)
{
	struct drm_connector *tmp_connector;
	struct drm_connector_state *new_conn_state;
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	int i, ret, old_mode = connector->dpms;
	bool active = false;

	ret = drm_modeset_lock(&state->dev->mode_config.connection_mutex,
			       state->acquire_ctx);
	if (ret)
		return ret;

	if (mode != DRM_MODE_DPMS_ON)
		mode = DRM_MODE_DPMS_OFF;
	connector->dpms = mode;

	crtc = connector->state->crtc;
	if (!crtc)
		goto out;
	ret = drm_atomic_add_affected_connectors(state, crtc);
	if (ret)
		goto out;

	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state)) {
		ret = PTR_ERR(crtc_state);
		goto out;
	}

	for_each_new_connector_in_state(state, tmp_connector, new_conn_state, i) {
		if (new_conn_state->crtc != crtc)
			continue;
		if (tmp_connector->dpms == DRM_MODE_DPMS_ON) {
			active = true;
			break;
		}
	}

	crtc_state->active = active;
	//ret = drm_atomic_commit(state);
out:
	if (ret != 0)
		connector->dpms = old_mode;
	return ret;
}

static int GB02FUNC1533(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct drm_connector *connector,
			struct gbdc_connector_state *old_con_state,
			struct gbdc_connector_state *new_con_state)
{
	int ret = 0;
	unsigned int row = new_con_state->infinity_adj.infinity_row;
	unsigned int col = new_con_state->infinity_adj.infinity_col;
	struct gbdc_connector *gbdc_conn = to_gbdc_connector(connector);
	int i, j, n;
	struct GB02STR198 *infinity;
	struct GB02STR194 *infinity_infos;
	struct drm_crtc_state *new_crtc_state;
	bool found = false;
	bool need_recreate = false;

	gbdc_for_each_infinity(infinity, dev) {
		if (infinity && infinity->infinity_connector_id == connector->base.id) {
			found = true;
			break;
		}
	}

	if (!found) {
		GB_PRINT_ERR("%s:cannot find infinity connector %d\n", __func__, \
			connector->base.id);
		return -EINVAL;
	}

	infinity_infos = infinity->infinity_infos;

	if (!infinity_infos) {
		GB_PRINT_ERR("%s:infinity_infos is NULL\n", __func__);
		return -EINVAL;
	}

	if (!new_con_state->gbdc_infinity_enable && old_con_state->gbdc_infinity_enable) {
		/* destroy infinity*/
		GB_PRINT_INFO("%s:check destroy infinity %d\n", __func__, infinity_infos->infinity_id);
		infinity->destroyed = true;
		infinity->changed = true;
		/*disable crtcs when delete a infinity*/
		#if 0
		if (new_con_state->base.crtc) {
			new_crtc_state = drm_atomic_get_new_crtc_state(state, new_con_state->base.crtc);
			if (new_crtc_state)
				to_gbdc_crtc_state(new_crtc_state)->need_disable = true;
		} else
			new_con_state->crtc_need_disable = true;
		#endif
		//ret = GB02FUNC1499(state, infinity);
		return ret;
	}

	if (new_con_state->infinity_adj.infinity_row != old_con_state->infinity_adj.infinity_row \
	|| new_con_state->infinity_adj.infinity_col != old_con_state->infinity_adj.infinity_col) {
		/* change infinity*/
		GB_PRINT_INFO("infinity changed from %dx%d to %dx%d\n", \
			old_con_state->infinity_adj.infinity_row, \
			old_con_state->infinity_adj.infinity_col, \
			new_con_state->infinity_adj.infinity_row, \
			new_con_state->infinity_adj.infinity_col);

		if (row * col > GB02MAC2558) {
			GB_PRINT_ERR("infinity changed error\n");
			return -EINVAL;
		}

		infinity_infos->infinity_row = new_con_state->infinity_adj.infinity_row;

		ret |= GB02FUNC963(
						gbdc_conn,
						INFINITY_ROW,
						0,
						&infinity_infos->infinity_row);

		infinity_infos->infinity_col = new_con_state->infinity_adj.infinity_col;

		ret |= GB02FUNC963(
						gbdc_conn,
						INFINITY_COL,
						0,
						&infinity_infos->infinity_col);

		infinity->recreated = true;
		infinity->changed = true;
		infinity->crtc_need_disable = true;

		/*disable crtcs when recreate a infinity*/
		if (new_con_state->base.crtc) {
			new_crtc_state = drm_atomic_get_new_crtc_state(state, new_con_state->base.crtc);
			if (new_crtc_state)
				to_gbdc_crtc_state(new_crtc_state)->need_disable = true;
		} else
			new_con_state->crtc_need_disable = true;
		//if (GB02FUNC1531(state,
		//		     connector,
		//		     DRM_MODE_DPMS_OFF))
		//	GB_PRINT_ERR("recreate change GB02FUNC1531 error\n");
	}

	for (j = 0; j < col; j++) {
		for (i = 0; i < row; i++) {
			unsigned int old_orderx =
				old_con_state->infinity_adj.adjusts[j * row + i].order.x;
			unsigned int old_ordery =
				old_con_state->infinity_adj.adjusts[j * row + i].order.y;
			unsigned int new_orderx =
				new_con_state->infinity_adj.adjusts[j * row + i].order.x;
			unsigned int new_ordery =
				new_con_state->infinity_adj.adjusts[j * row + i].order.y;

			if (old_orderx != new_orderx || old_ordery != new_ordery) {
				GB_PRINT_INFO("adjusts order changed (%d,%d)-(%d,%d)\n", \
					infinity_infos->infos[j * row + i].order.x, \
					infinity_infos->infos[j * row + i].order.y, \
					new_con_state->infinity_adj.adjusts[j * row + i].order.x, \
					new_con_state->infinity_adj.adjusts[j * row + i].order.y);
				infinity_infos->infos[j * row + i].order = \
					new_con_state->infinity_adj.adjusts[j * row + i].order;

				ret |= GB02FUNC963(
						gbdc_conn,
						INFINITY_ORDER,
						j * row + i,
						&new_con_state->infinity_adj.adjusts[j * row + i].order);

				/*check if has padding*/
				need_recreate = false;
				for (n = 0; n < GB02MAC2560; n++) {
					if (infinity_infos->infos[j * row + i].padding[n].width \
						&& infinity_infos->infos[j * row + i].padding[n].height) {
						if (infinity_infos->infos[j * row + i].padding[n].top \
							|| infinity_infos->infos[j * row + i].padding[n].right \
							|| infinity_infos->infos[j * row + i].padding[n].bottom \
							|| infinity_infos->infos[j * row + i].padding[n].left) {
							need_recreate = true;
							break;
						}
					}
				}

				if (need_recreate) {
					/*if recreate or order change, we set padding to 0*/
					#if 0
					#if 0
					memset(infinity_infos->infos[j * row + i].padding, 0, \
						sizeof(infinity_infos->infos[j * row + i].padding));
					memset(new_con_state->infinity_adj.adjusts[j * row + i].padding, 0, \
						sizeof(new_con_state->infinity_adj.adjusts[j * row + i].padding));
					#else
					for (n = 0; n < GB02MAC2560; n++) {
						if (infinity_infos->infos[j * row + i].padding[n].width \
							|| infinity_infos->infos[j * row + i].padding[n].height) {
							infinity_infos->infos[j * row + i].padding[n].top = 0;
							infinity_infos->infos[j * row + i].padding[n].right = 0;
							infinity_infos->infos[j * row + i].padding[n].bottom = 0;
							infinity_infos->infos[j * row + i].padding[n].left = 0;
						}
					}
					memcpy(new_con_state->infinity_adj.adjusts[j * row + i].padding, \
						infinity_infos->infos[j * row + i].padding, \
						sizeof(new_con_state->infinity_adj.adjusts[j * row + i].padding));
					#endif

					ret |= GB02FUNC963(
							gbdc_conn,
							INFINITY_PADDING,
							j * row + i,
							new_con_state->infinity_adj.adjusts[j * row + i].padding);
					#endif
					//infinity->recreated = true;
					infinity->tiny_recreated = true;
					infinity->changed = true;
					GB_PRINT_INFO("recreate change\n");
				}

				if (!infinity->recreated) {
					infinity->uinfo_changed[j * row + i].st.order_changed = 1;
					infinity->changed = true;
					GB_PRINT_INFO("set order_changed\n");
				}

			}
			if (old_con_state->infinity_adj.adjusts[j * row + i].connector_id \
				!= new_con_state->infinity_adj.adjusts[j * row + i].connector_id) {
				GB_PRINT_INFO("adjusts id changed (%d)-(%d)\n", \
					infinity_infos->infos[j * row + i].connector_id, \
					new_con_state->infinity_adj.adjusts[j * row + i].connector_id);
				infinity_infos->infos[j * row + i].connector_id = \
					new_con_state->infinity_adj.adjusts[j * row + i].connector_id;

				ret |= GB02FUNC963(
						gbdc_conn,
						INFINITY_CONN_ID,
						j * row + i,
						&new_con_state->infinity_adj.adjusts[j * row + i].connector_id);

				need_recreate = false;
				/*check if has padding*/
				for (n = 0; n < GB02MAC2560; n++) {
					if (infinity_infos->infos[j * row + i].padding[n].width \
						&& infinity_infos->infos[j * row + i].padding[n].height) {
						if (infinity_infos->infos[j * row + i].padding[n].top \
							|| infinity_infos->infos[j * row + i].padding[n].right \
							|| infinity_infos->infos[j * row + i].padding[n].bottom \
							|| infinity_infos->infos[j * row + i].padding[n].left) {
							need_recreate = true;
							break;
						}
					}
				}

				if (need_recreate) {
					/*if recreate or id change, we set padding to 0*/
					#if 0
					#if 0
					memset(infinity_infos->infos[j * row + i].padding, 0, \
						sizeof(infinity_infos->infos[j * row + i].padding));
					memset(new_con_state->infinity_adj.adjusts[j * row + i].padding, 0, \
						sizeof(new_con_state->infinity_adj.adjusts[j * row + i].padding));
					#else
					for (n = 0; n < GB02MAC2560; n++) {
						if (infinity_infos->infos[j * row + i].padding[n].width \
							|| infinity_infos->infos[j * row + i].padding[n].height) {
							infinity_infos->infos[j * row + i].padding[n].top = 0;
							infinity_infos->infos[j * row + i].padding[n].right = 0;
							infinity_infos->infos[j * row + i].padding[n].bottom = 0;
							infinity_infos->infos[j * row + i].padding[n].left = 0;
						}
					}
					memcpy(new_con_state->infinity_adj.adjusts[j * row + i].padding, \
						infinity_infos->infos[j * row + i].padding, \
						sizeof(new_con_state->infinity_adj.adjusts[j * row + i].padding));
					#endif

					ret |= GB02FUNC963(
							gbdc_conn,
							INFINITY_PADDING,
							j * row + i,
							new_con_state->infinity_adj.adjusts[j * row + i].padding);
					#endif
					//infinity->recreated = true;
					infinity->tiny_recreated = true;
					infinity->changed = true;
					GB_PRINT_INFO("recreate change\n");
				}

				if (!infinity->recreated) {
					infinity->uinfo_changed[j * row + i].st.id_changed = 1;
					infinity->changed = true;
					GB_PRINT_INFO("set id_changed\n");
				}

			}

			if (memcmp(old_con_state->infinity_adj.adjusts[j * row + i].padding, \
				new_con_state->infinity_adj.adjusts[j * row + i].padding, \
				sizeof(new_con_state->infinity_adj.adjusts[j * row + i].padding))) {
				int n;
				GB_PRINT_INFO("adjusts padding changed\n");
				for (n = 0; n < GB02MAC2560; n++)
					if (infinity_infos->infos[j * row + i].padding[n].width \
						|| infinity_infos->infos[j * row + i].padding[n].height \
						|| new_con_state->infinity_adj.adjusts[j * row + i].padding[n].width \
						|| new_con_state->infinity_adj.adjusts[j * row + i].padding[n].height)
						GB_PRINT_INFO("(%d %d %d %d %d %d)-(%d %d %d %d %d %d))\n", \
						infinity_infos->infos[j * row + i].padding[n].width, \
						infinity_infos->infos[j * row + i].padding[n].height, \
						infinity_infos->infos[j * row + i].padding[n].top, \
						infinity_infos->infos[j * row + i].padding[n].right, \
						infinity_infos->infos[j * row + i].padding[n].bottom, \
						infinity_infos->infos[j * row + i].padding[n].left, \
						new_con_state->infinity_adj.adjusts[j * row + i].padding[n].width, \
						new_con_state->infinity_adj.adjusts[j * row + i].padding[n].height, \
						new_con_state->infinity_adj.adjusts[j * row + i].padding[n].top, \
						new_con_state->infinity_adj.adjusts[j * row + i].padding[n].right, \
						new_con_state->infinity_adj.adjusts[j * row + i].padding[n].bottom, \
						new_con_state->infinity_adj.adjusts[j * row + i].padding[n].left);
				memcpy(infinity_infos->infos[j * row + i].padding, \
					new_con_state->infinity_adj.adjusts[j * row + i].padding, \
					sizeof(new_con_state->infinity_adj.adjusts[j * row + i].padding));

				ret |= GB02FUNC963(
						gbdc_conn,
						INFINITY_PADDING,
						j * row + i,
						new_con_state->infinity_adj.adjusts[j * row + i].padding);
				if (!infinity->recreated) {
					infinity->uinfo_changed[j * row + i].st.padding_changed = true;
					infinity->changed = true;
					GB_PRINT_INFO("set padding_changed\n");
				}
				//infinity->padding_changed = true;
			}
		}
	}

	if (infinity->recreated) {
		GB_PRINT_INFO("set recreate\n");
		//infinity->crtc_need_disable = true;
		//new_con_state->crtc_need_disable = true;
		for (j = 0; j < col; j++) {
			for (i = 0; i < row; i++) {
				infinity->uinfo_changed[j * row + i].info_changed = 0;
			}
		}
	}

	return ret;
}

static int GB02FUNC1557(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct drm_connector *connector,
			struct gbdc_connector_state *old_con_state,
			struct gbdc_connector_state *new_con_state)
{
	int ret = 0;
	struct GB02STR198 *infinity = NULL;
	unsigned int infinity_id;
	unsigned long int flags;

	infinity_id = new_con_state->infinity_set.infinity_id;

	if (GB02FUNC1423(&new_con_state->infinity_set)) {

		bool infinity_exist = false;

		gbdc_for_each_infinity(infinity, dev) {
			if (infinity->infinity_infos) {
				if (infinity->infinity_infos->infinity_id == infinity_id) {
					GB_PRINT_INFO("%s:infinity %d exist\n", __func__, infinity_id);
					infinity_exist = true;
					ret = GB02FUNC1478(dev, state, infinity, connector, new_con_state);
				}
			}
		}

		if (!infinity_exist) {
			bool is_distroy = true;
			struct GB02STR198 *n_infinity = NULL;

			/*destroy old infinity*/
			gbdc_for_each_infinity_safe(infinity, dev) {
				if (infinity && infinity->infinity_infos) {
					if ((old_con_state->infinity_set.infinity_id > 0) \
						&& (infinity->infinity_infos->infinity_id == \
						old_con_state->infinity_set.infinity_id)) {

						GB_PRINT_INFO("%s:destroy infinity %d\n", __func__, \
							old_con_state->infinity_set.infinity_id);
						is_distroy =
							!GB02FUNC1499(state, infinity);
					}
				}
			}

			if (is_distroy) {
				struct GB02STR198 *instance = GB02FUNC1460(dev,
																state, new_con_state);
				if (instance) {
					GB_PRINT_INFO("%s:add infinity %d of connector %d\n", __func__,
						instance->infinity_infos->infinity_id, instance->infinity_connector_id);
					spin_lock_irqsave(
						&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
					if (!GB02FUNC1456(
						&GB02FUNC85((dev)->dev_private)->kms_info.infinity_list, instance))
						list_add_tail(&instance->head, \
							&GB02FUNC85((dev)->dev_private)->kms_info.infinity_list);
					else
						GB_PRINT_ERR("%s: error instance%d exist in lsit\n", __func__, \
								instance->infinity_infos->infinity_id);
					spin_unlock_irqrestore(
						&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
					/* ret |= GB02FUNC1403(connector,
						instance->infinity_infos); */

					ret = GB02FUNC1478(dev, state, instance, connector, new_con_state);
				}
			}
		}

	}

	return ret;
}

int GB02FUNC1563(struct drm_device *dev,
			struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	struct drm_crtc_state *new_crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *old_con_state, *new_con_state;
	struct gbdc_connector_state *gbdc_old_con_state, *gbdc_new_con_state;
	int i;
	//unsigned int infinity_id;
	//struct GB02STR155 *gb_dev = GB02FUNC85(dev->dev_private);
	int ret = 0;

	GB_PRINT_DBG("%s %s\n", __func__, current->comm);

	for_each_oldnew_connector_in_state(state, connector, old_con_state, new_con_state, i) {

		if (!old_con_state || !new_con_state)
			continue;

		//if (old_con_state->crtc != new_con_state->crtc)
		//	continue;

		gbdc_old_con_state = to_gbdc_connector_state(old_con_state);
		gbdc_new_con_state = to_gbdc_connector_state(new_con_state);

		if (!gbdc_old_con_state || !gbdc_new_con_state)
			continue;

		if (gbdc_new_con_state->is_virtual) {
			GB_PRINT_DBG("%s check virtual\n", __func__);

			if (!gbdc_new_con_state->gbdc_infinity_enable && \
				!gbdc_old_con_state->gbdc_infinity_enable)
				continue;

			if (gbdc_new_con_state->gbdc_infinity_enable && \
				!gbdc_old_con_state->gbdc_infinity_enable)
				continue;

			//if (!gbdc_new_con_state->gbdc_infinity_enable) {
			//	GB_PRINT_ERR("this connector(%d) is invalid\n", connector->base.id);
			//	return -EINVAL;
			//}

			if ((gbdc_new_con_state->gbdc_infinity_enable && \
				GB02FUNC1446(gbdc_old_con_state, gbdc_new_con_state))
				|| !gbdc_new_con_state->gbdc_infinity_enable) {
				GB_PRINT_INFO("%s check virtual change\n", __func__);

				/*check virtual screen*/
				ret = GB02FUNC1533(dev, state, connector,
										gbdc_old_con_state,
										gbdc_new_con_state);
				if (ret)
					GB_PRINT_INFO("GB02FUNC1533 fail\n");

				//if (infinity->crtc_need_disable) {
				//	if (gbdc_crtc_state)
				//		gbdc_crtc_state->need_disable = true;
				//	infinity->crtc_need_disable = false;
				//}
			}

		} else {
			GB_PRINT_DBG("%s check physical\n", __func__);

			/*if (GB02FUNC1680(connector, NULL, false)) {
				GB_PRINT_ERR("this connector(%d) is invalid\n", connector->base.id);
				ret = -EINVAL;
				continue;
			}*/

			if (!memcmp(&gbdc_new_con_state->infinity_set,
					&gbdc_old_con_state->infinity_set, sizeof(gbdc_old_con_state->infinity_set)))
				continue;

			GB_PRINT_INFO("%s check physical change\n", __func__);
			if (!GB02FUNC1443(gbdc_new_con_state)) {
				/*destroy previous created instance*/
				struct GB02STR198 *infinity = NULL;
				struct GB02STR198 *n_infinity = NULL;

				gbdc_for_each_infinity_safe(infinity, dev) {
					if (!infinity || !infinity->infinity_infos)
						continue;

					if (infinity->infinity_infos->infinity_id != \
						gbdc_new_con_state->infinity_set.infinity_id)
						continue;

					GB_PRINT_INFO("%s destroy infinity %d which created before\n", \
						__func__, gbdc_new_con_state->infinity_set.infinity_id);
					GB02FUNC1499(state, infinity);
				}
				continue;
			}

			/*check and create virtual screen*/
			ret = GB02FUNC1557(dev, state, connector,
										gbdc_old_con_state,
										gbdc_new_con_state);
			if (ret)
				GB_PRINT_INFO("GB02FUNC1557 fail\n");

		}
	}

	for_each_oldnew_crtc_in_state(state, crtc, old_crtc_state, new_crtc_state, i) {
		struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

		if (!old_crtc_state || !new_crtc_state)
			continue;

		if (!gbdc_crtc/* || !gbdc_crtc->virt*/)
			continue;

		if (GB02FUNC1691(
				dev, GB02FUNC1573(crtc)->crtc_id, NULL, false)) {
			if (to_gbdc_crtc_state(new_crtc_state)->need_disable)
				gbdc_crtc->need_disable = true;
			else
				GB02FUNC1514(dev, state, crtc, new_crtc_state);

			if (to_gbdc_crtc_state(old_crtc_state)->need_disable \
				!= to_gbdc_crtc_state(new_crtc_state)->need_disable \
				&& to_gbdc_crtc_state(new_crtc_state)->need_disable) {
				to_gbdc_crtc_state(new_crtc_state)->need_disable_changed = true;
				GB_PRINT_INFO("==================need_disable_changed\n");
			}
		}

		if (gbdc_crtc->virt && new_crtc_state->mode_changed) {
			GB_PRINT_INFO("+++++++++++++++++%s crtc %d mode_changed\n", \
					current->comm, gbdc_crtc->crtc_id);
			if (GB02FUNC1509(dev, state, crtc,
										old_crtc_state,
										new_crtc_state)) {
				GB_PRINT_INFO("virtual is deleted, disable crtc, new_crtc_state->active %d\n",
					new_crtc_state->active);
			}

		}

	}

	GB_PRINT_DBG("%s end\n", __func__);

	return ret;
}

static int GB02FUNC1579(struct gbdc_crtc *gbdc_crtc,
						struct GB02STR191 *geometry)
{
	//geometry->w = gbdc_crtc->adjusted_mode.hdisplay;
	//geometry->h = gbdc_crtc->adjusted_mode.vdisplay;
	//geometry->x = gbdc_crtc->order.y * geometry->w;
	//geometry->y = gbdc_crtc->order.x * geometry->h;.
	*geometry = gbdc_crtc->geometry;

	return 0;
}

static int GB02FUNC1581(struct gbdc_crtc *gbdc_crtc,
						struct GB02STR189 *padding)
{
	*padding = gbdc_crtc->padding;
	return 0;
}

static int GB02FUNC1583(struct drm_atomic_state *state,
					struct gbdc_crtc_state *gbdc_crtc_state,
					struct gbdc_plane_state *gbdc_plane_state,
					struct drm_plane *vplane)
{
	struct drm_device *dev = state->dev;
	struct gbdc_plane *gbdc_plane;
	struct drm_plane *plane;
	struct gbdc_plane *gbdc_vplane = to_gbdc_plane_info(vplane);
	struct gbdc_crtc *gbdc_crtc;
	//int crtc_id = 0;
	int ret = 0;

	GB_PRINT_INFO("******%s \n", __func__);

	gbdc_plane_state->infinity_plane_list->infinity_row \
		= gbdc_crtc_state->infinity_crtc_list->infinity_row;
	gbdc_plane_state->infinity_plane_list->infinity_col = \
		gbdc_crtc_state->infinity_crtc_list->infinity_col;

	gbdc_for_each_gbdc_obj(crtc) {
		if (!gbdc_crtc)
			continue;

		drm_for_each_plane(plane, dev) {
			gbdc_plane = to_gbdc_plane_info(plane);

			if (plane == vplane)
				continue;

			if (gbdc_plane->virt)
				continue;

			if (gbdc_crtc->crtc_id != gbdc_plane->crtc_id)
				continue;

			if (plane->type == vplane->type \
				&& gbdc_plane->plane_id == gbdc_vplane->plane_id) {

				gbdc_plane->order.x = gbdc_crtc->order.x;
				gbdc_plane->order.y = gbdc_crtc->order.y;
				GB_PRINT_INFO("******%s:plane(%d) %d,%d\n", __func__, \
												gbdc_plane->plane_id, \
												gbdc_plane->order.x, \
												gbdc_plane->order.y);
				gbdc_plane->infinity_row = gbdc_crtc->infinity_row;
				gbdc_plane->infinity_col = gbdc_crtc->infinity_col;
				GB02FUNC1579(gbdc_crtc, &gbdc_plane->geometry);
				GB02FUNC1581(gbdc_crtc, &gbdc_plane->padding);
				GB_PRINT_INFO("%s:add plane-%d(%d):%d(%d,%d) to plane-%d(%d):%d\n", \
								__func__, plane->type, gbdc_plane->plane_id, \
								gbdc_plane->base.base.id, \
								gbdc_plane->order.x, \
								gbdc_plane->order.y, \
								vplane->type, \
								gbdc_vplane->plane_id, \
								gbdc_vplane->base.base.id);
				GB_PRINT_INFO("commit plane padding:%d %d %d %d %d %d\n", \
						gbdc_plane->padding.width, \
						gbdc_plane->padding.height, \
						gbdc_plane->padding.top, \
						gbdc_plane->padding.right, \
						gbdc_plane->padding.bottom, \
						gbdc_plane->padding.left);
				if (!GB02FUNC1454(
					gbdc_plane_state->infinity_plane_list, gbdc_plane))
					list_add_tail(&gbdc_plane->head, \
						&gbdc_plane_state->infinity_plane_list->head);
				else
					GB_PRINT_ERR("%s: error plane%d exist in plane%d\n", __func__, \
						gbdc_plane->plane_id, gbdc_vplane->plane_id);
			}
		}
	}

	return ret;
}

static int GB02FUNC1589(struct gbdc_crtc_state *gbdc_crtc_state,
					struct gbdc_crtc *gbdc_crtc,
					struct gbdc_connector *gbdc_connector,
					struct gbdc_connector *gbdc_conn)
{
	struct drm_display_mode *mode;
	int ret = 0;

	GB_PRINT_INFO("%s:commit mode:%dx%d@%d\n", __func__, \
		gbdc_crtc_state->base.adjusted_mode.hdisplay, \
		gbdc_crtc_state->base.adjusted_mode.vdisplay, \
		gbdc_crtc_state->base.adjusted_mode.clock);

	mode = GB02FUNC906(&gbdc_crtc_state->base.adjusted_mode, \
									gbdc_connector);

	if (!mode) {
		GB_PRINT_ERR("%s:find crtc(%d):%d mode fail\n", __func__, gbdc_crtc->crtc_id, \
			gbdc_crtc->base.base.id);
		mode = &gbdc_crtc->old_adjusted_mode;
	}
	if (mode) {
		GB_PRINT_INFO("%s:crtc(%d):%d %dx%d@%d\n", __func__, gbdc_crtc->crtc_id, \
			gbdc_crtc->base.base.id, mode->hdisplay, mode->vdisplay, mode->clock);
		gbdc_crtc->adjusted_mode = *mode;
		gbdc_crtc->old_adjusted_mode = gbdc_crtc->adjusted_mode;
		gbdc_crtc->geometry.w = gbdc_crtc->adjusted_mode.hdisplay;
		gbdc_crtc->geometry.h = gbdc_crtc->adjusted_mode.vdisplay;
		gbdc_crtc->geometry.x = gbdc_crtc->order.y * gbdc_crtc->geometry.w;
		gbdc_crtc->geometry.y = gbdc_crtc->order.x * gbdc_crtc->geometry.h;
	}

	return ret;
}

static bool GB02FUNC1590(
					struct gbdc_crtc_state *gbdc_crtc_state,
					struct gbdc_connector *gbdc_conn)
{
	return true;//GB02FUNC812(gbdc_conn, &gbdc_crtc_state->base.adjusted_mode);
}

static void GB02FUNC1592(
					struct gbdc_connector_state *gbdc_connector_state,
					struct GB02STR194 *infinity_infos)
{
	int i, j, row, col;

	row = infinity_infos->infinity_row;
	col = infinity_infos->infinity_col;

	for (j = 0; j < col; j++) {
		for (i = 0; i < row; i++) {
			memset(infinity_infos->infos[j * row + i].padding, 0, \
				sizeof(infinity_infos->infos[j * row + i].padding));
			memcpy(gbdc_connector_state->infinity_adj.adjusts[j * row + i].padding, \
				infinity_infos->infos[j * row + i].padding, \
				sizeof(infinity_infos->infos[j * row + i].padding));

		}
	}

}

static int GB02FUNC1594(struct gbdc_crtc_state *gbdc_crtc_state,
					struct gbdc_crtc *gbdc_crtc,
					int crtc_id,
					struct gbdc_connector *gbdc_connector)
{
	if (GB02FUNC874(&gbdc_connector->base) == connector_status_connected) {
		gbdc_crtc_state->infinity_crtc_list->vblank_id = crtc_id;
		gbdc_crtc_state->vblank_id = crtc_id;
	}

	return 0;
}

static int GB02FUNC1596(
					struct gbdc_crtc *gbdc_crtc,
					struct GB02STR194 *infinity_infos)
{
	int orderx, ordery;
	int i, n;
	int row, col;
	int width;
	int height;

	if (!gbdc_crtc || !infinity_infos)
		return -EINVAL;

	row = infinity_infos->infinity_row;
	col = infinity_infos->infinity_col;

	if (row * col > GB02MAC2558)
		return -EINVAL;

	orderx = gbdc_crtc->order.x;
	ordery = gbdc_crtc->order.y;

	if (orderx >= row || ordery >= col)
		return -EINVAL;

	width = gbdc_crtc->geometry.w;
	height = gbdc_crtc->geometry.h;

	for (n = 0; n < row * col; n++) {
		if (infinity_infos->infos[n].order.y == ordery \
			&& infinity_infos->infos[n].order.x < orderx) {
			for (i = 0; i < GB02MAC2560; i++) {
				if (width == infinity_infos->infos[n].padding[i].width \
					&& height == infinity_infos->infos[n].padding[i].height) {
					gbdc_crtc->geometry.y += \
						infinity_infos->infos[n].padding[i].top \
						+ infinity_infos->infos[n].padding[i].bottom;
					GB_PRINT_INFO("crtc(%d) geometry.y + %d + %d\n", \
						gbdc_crtc->crtc_id, \
						infinity_infos->infos[n].padding[i].top, \
						infinity_infos->infos[n].padding[i].bottom);
					break;
				}
			}
		}

		if (infinity_infos->infos[n].order.x == orderx \
			&& infinity_infos->infos[n].order.y < ordery) {
			for (i = 0; i < GB02MAC2560; i++) {
				if (width == infinity_infos->infos[n].padding[i].width \
					&& height == infinity_infos->infos[n].padding[i].height) {
					gbdc_crtc->geometry.x += \
						infinity_infos->infos[n].padding[i].left \
						+ infinity_infos->infos[n].padding[i].right;
					GB_PRINT_INFO("crtc(%d) geometry.x + %d + %d\n", \
						gbdc_crtc->crtc_id, \
						infinity_infos->infos[n].padding[i].left, \
						infinity_infos->infos[n].padding[i].right);
					break;
				}
			}
		}
	}

	return 0;
}

static bool GB02FUNC1599(struct drm_connector *conn,
														int id)
{
	struct drm_crtc *crtc = NULL;
	struct gbdc_crtc *gbdc_crtc;

	if (conn->state)
		crtc = conn->state->crtc;

	if (!crtc)
		return true;

	gbdc_crtc = GB02FUNC1573(crtc);
	if (gbdc_crtc->crtc_id == id)
		return true;

	return false;
}

static bool __maybe_unused GB02FUNC1601(struct gbdc_crtc_state *gbdc_crtc_state,
					struct gbdc_connector *gbdc_conn)
{
	struct drm_display_mode *mode;
	bool found = false;

	GB_PRINT_INFO("%s:check mode:%dx%d@%d\n", __func__, \
		gbdc_crtc_state->base.adjusted_mode.hdisplay, \
		gbdc_crtc_state->base.adjusted_mode.vdisplay, \
		gbdc_crtc_state->base.adjusted_mode.clock);

	list_for_each_entry(mode, &gbdc_conn->base.modes, head) {
		if (drm_mode_equal(mode, &gbdc_crtc_state->base.adjusted_mode)) {
			found = true;
			break;
		}
	}


	return found;
}

static int GB02FUNC1602(struct drm_atomic_state *state,
					struct drm_crtc *crtc,
					struct gbdc_crtc_state *gbdc_crtc_state,
					struct gbdc_connector_state *gbdc_connector_state,
					struct gbdc_connector *gbdc_conn,
					struct GB02STR198 *infinity)
{
	struct drm_device *dev = state->dev;
	struct gbdc_connector *gbdc_connector;
	int crtc_id = 0;
	struct gbdc_crtc *gbdc_crtc;
	unsigned int pos = 0;
	bool custom_mode = false;
	struct GB02STR194 *infinity_infos = infinity->infinity_infos;
	int i;
	bool mode_commit;// = infinity->rebuild | infinity->tiny_recreated;

	GB_PRINT_INFO("******%s %dx%d pid=%s\n", __func__, \
		gbdc_connector_state->infinity_connector_list->infinity_row, \
		gbdc_connector_state->infinity_connector_list->infinity_col, \
		current->comm);

	//if (!GB02FUNC1601(gbdc_crtc_state, gbdc_conn)) {
	//	GB_PRINT_INFO(
	//		"%s: mode need changed, do not commit crtc and plane\n", __func__);
		//return -1;
	//} else
	//	GB_PRINT_INFO(
	//		"%s: \n", __func__);
	/*here we can do not commit mode info, because layout changed but mode not changed yet*/
	mode_commit = true;

	gbdc_crtc_state->infinity_crtc_list->infinity_row \
		= gbdc_connector_state->infinity_connector_list->infinity_row;
	gbdc_crtc_state->infinity_crtc_list->infinity_col = \
		gbdc_connector_state->infinity_connector_list->infinity_col;

	/*if the mode is not go with padding, set current padding value to 0*/
	custom_mode = GB02FUNC1590(gbdc_crtc_state,
														gbdc_conn);
	if (!custom_mode) {
		GB_PRINT_INFO(\
			"%s: set infinity padding to 0\n", __func__);

		/*clear adjust and infinity_info padding value*/
		GB02FUNC1592(gbdc_connector_state, infinity_infos);
		infinity->padding_changed = true;
	}

	gbdc_for_each_gbdc_obj(connector) {
		if (!gbdc_connector)
			continue;

		/*here we use connector id to find ref crtc*/
		crtc_id = gbdc_connector->connector_id;
		if (crtc_id >= GB02MAC2693)
			continue;

		if (!GB02FUNC1599(&gbdc_connector->base, crtc_id))
			GB_PRINT_ERR("%s:fail???\n", __func__);

		gbdc_crtc = GB02FUNC85((dev)->dev_private)->kms_info.crtcs[crtc_id];
		if (gbdc_crtc && !gbdc_crtc->virt) {
			gbdc_crtc->order.x = gbdc_connector->order.x;
			gbdc_crtc->order.y = gbdc_connector->order.y;
			GB_PRINT_INFO("******%s:crtc(%d) %d,%d\n", __func__, \
												crtc_id, \
												gbdc_crtc->order.x, \
												gbdc_crtc->order.y);
			gbdc_crtc->infinity_row = gbdc_connector->infinity_row;
			gbdc_crtc->infinity_col = gbdc_connector->infinity_col;
			if (!custom_mode) {
				memset(gbdc_connector->padding, 0, sizeof(gbdc_connector->padding));
				for (pos = 0; pos < GB02MAC2558; pos++) {
					if (infinity_infos->infos[pos].connector_id == gbdc_connector->base.base.id) {
						memcpy(infinity_infos->infos[pos].padding, gbdc_connector->padding, \
						sizeof(gbdc_connector->padding));
						GB02FUNC963(
													gbdc_conn,
													INFINITY_PADDING,
													pos,
													gbdc_connector->padding);
						break;
					}
				}
			}

			gbdc_crtc_state->infinity_crtc_list->vblank_id = crtc_id;
			gbdc_crtc_state->vblank_id = crtc_id;

			if (mode_commit) {
				GB02FUNC1589(gbdc_crtc_state, gbdc_crtc, gbdc_connector, gbdc_conn);

				for (i = 0; i < GB02MAC2560; i++) {
					if (gbdc_crtc->adjusted_mode.hdisplay == gbdc_connector->padding[i].width \
						&& gbdc_crtc->adjusted_mode.vdisplay == gbdc_connector->padding[i].height) {
						gbdc_crtc->padding = gbdc_connector->padding[i];
						GB_PRINT_INFO("commit crtc padding:%d %d %d %d %d %d\n", \
							gbdc_crtc->padding.width, \
							gbdc_crtc->padding.height, \
							gbdc_crtc->padding.top, \
							gbdc_crtc->padding.right, \
							gbdc_crtc->padding.bottom, \
							gbdc_crtc->padding.left);
						//gbdc_crtc->geometry.x += gbdc_crtc->padding.left;
						//gbdc_crtc->geometry.y += gbdc_crtc->padding.top;
						break;
					}
				}

				if (!GB02FUNC812(gbdc_conn, &gbdc_crtc_state->base.adjusted_mode)) {
					GB_PRINT_INFO("origin mode set padding to 0\n");
					memset(&gbdc_crtc->padding, 0, sizeof(gbdc_crtc->padding));
					//gbdc_crtc->geometry.x += gbdc_crtc->padding.left;
					//gbdc_crtc->geometry.y += gbdc_crtc->padding.top;
				} else {
					gbdc_crtc->geometry.x += gbdc_crtc->padding.left;
					gbdc_crtc->geometry.y += gbdc_crtc->padding.top;

					GB_PRINT_INFO("crtc(%d) geometry.x + %d\n", \
						gbdc_crtc->crtc_id, gbdc_crtc->padding.left);
					GB_PRINT_INFO("crtc(%d) geometry.y + %d\n", \
						gbdc_crtc->crtc_id, gbdc_crtc->padding.top);

					GB02FUNC1596(gbdc_crtc, infinity_infos);
				}

				for (pos = 0; pos < GB02MAC2558; pos++) {
					if (infinity_infos->infos[pos].connector_id == gbdc_connector->base.base.id) {
						infinity_infos->infos[pos].geometry = gbdc_crtc->geometry;
						GB_PRINT_INFO(\
							"geometry.x=%d\n", infinity_infos->infos[pos].geometry.x);
						GB_PRINT_INFO(\
							"geometry.y=%d\n", infinity_infos->infos[pos].geometry.y);
						GB_PRINT_INFO(\
							"geometry.w=%d\n", infinity_infos->infos[pos].geometry.w);
						GB_PRINT_INFO(\
							"geometry.h=%d\n", infinity_infos->infos[pos].geometry.h);
						GB02FUNC963(
														gbdc_conn,
														INFINITY_GEOMETRY,
														pos,
														&gbdc_crtc->geometry);
						break;
					}
				}

				GB02FUNC1594(gbdc_crtc_state, gbdc_crtc, crtc_id, gbdc_connector);

			}
			GB_PRINT_INFO("%s:add crtc(%d):%d(%d,%d) to crtc(%d):%d\n", \
								__func__, gbdc_crtc->crtc_id, gbdc_crtc->base.base.id, \
								gbdc_crtc->order.x, gbdc_crtc->order.y, \
								GB02FUNC1573(crtc)->crtc_id, \
								GB02FUNC1573(crtc)->base.base.id);
			if (!GB02FUNC1450(gbdc_crtc_state->infinity_crtc_list, gbdc_crtc))
				list_add_tail(&gbdc_crtc->head, &gbdc_crtc_state->infinity_crtc_list->head);
			else
				GB_PRINT_ERR("%s: error crtc%d exist in crtc%d\n", __func__, \
					gbdc_crtc->crtc_id, GB02FUNC1573(crtc)->crtc_id);
		}
	}

	//infinity->tiny_recreated = infinity->rebuild = false;

	return 0;
}

static int GB02FUNC1609(struct drm_connector *connector,
					struct drm_atomic_state *state,
					struct gbdc_connector_state *gbdc_connector_state,
					struct GB02STR194 *infinity_infos)
{
	int ret = 0;
	struct drm_device *dev = state->dev;
	int i, j, row, col, n;
	int conn_id;

	GB_PRINT_INFO("******%s \n", __func__);

	if (!gbdc_connector_state || !infinity_infos)
		return -EINVAL;

	if (!gbdc_connector_state->is_virtual)
		return -EINVAL;


	INIT_LIST_HEAD(&gbdc_connector_state->infinity_connector_list->head);
	gbdc_connector_state->infinity_connector_list->infinity_row = infinity_infos->infinity_row;
	gbdc_connector_state->infinity_connector_list->infinity_col = infinity_infos->infinity_col;

	row = infinity_infos->infinity_row;
	col = infinity_infos->infinity_col;

	for (j = 0; j < col; j++) {
		for (i = 0; i < row; i++) {
			struct drm_connector *conn;
			struct drm_connector_list_iter conn_iter;

			conn_id = infinity_infos->infos[j * row + i].connector_id;
			if (!gbdc_connector_state->gbdc_infinity_enable) {
				gbdc_connector_state->infinity_adj.adjusts[j * row + i].connector_id = conn_id;
				gbdc_connector_state->infinity_adj.adjusts[j * row + i].order.x = \
					infinity_infos->infos[j * row + i].order.x;
				gbdc_connector_state->infinity_adj.adjusts[j * row + i].order.y = \
					infinity_infos->infos[j * row + i].order.y;
				memcpy(gbdc_connector_state->infinity_adj.adjusts[j * row + i].padding, \
					infinity_infos->infos[j * row + i].padding, \
					sizeof(infinity_infos->infos[j * row + i].padding));

			}

			drm_connector_list_iter_begin(dev, &conn_iter);
			drm_for_each_connector_iter(conn, &conn_iter) {
				if (conn && conn->base.id == conn_id) {
					struct gbdc_connector *gbdc_connector = to_gbdc_connector(conn);

					if (gbdc_connector && !gbdc_connector->virt) {
						/*check physical link status*/
						infinity_infos->infos[j * row + i].connected \
							= GB02FUNC874(conn);

						if (gbdc_infinity_check_connect_status) {
							if (infinity_infos->infos[j * row + i].connected \
								!= connector_status_connected) {
								GB_PRINT_ERR("gbdc connector%d: %s\n", \
										conn_id, "disconnected");
								//ret = -ENODEV;
								//break;
							}
						}
						gbdc_connector->order.x = infinity_infos->infos[j * row + i].order.x;
						gbdc_connector->order.y = infinity_infos->infos[j * row + i].order.y;
						gbdc_connector->infinity_row = row;
						gbdc_connector->infinity_col = col;
						memcpy(gbdc_connector->padding,
							infinity_infos->infos[j * row + i].padding, \
							sizeof(infinity_infos->infos[j * row + i].padding));
						for (n = 0; n < GB02MAC2560; n++)
							if (gbdc_connector->padding[n].width || \
								gbdc_connector->padding[n].height)
								GB_PRINT_INFO("commit connector(%d) "\
									"pad (%d %d %d %d %d %d))\n", \
									conn_id, \
									gbdc_connector->padding[n].width, \
									gbdc_connector->padding[n].height, \
									gbdc_connector->padding[n].top, \
									gbdc_connector->padding[n].right, \
									gbdc_connector->padding[n].bottom, \
									gbdc_connector->padding[n].left);

						GB_PRINT_INFO("%s:add connector(%d):%d(%d,%d) to connector(%d):%d\n", \
							__func__, gbdc_connector->connector_id, \
							gbdc_connector->base.base.id, \
							gbdc_connector->order.x, \
							gbdc_connector->order.y, \
							to_gbdc_connector(connector)->connector_id, \
							to_gbdc_connector(connector)->base.base.id);
						if (!GB02FUNC1447(
							gbdc_connector_state->infinity_connector_list, gbdc_connector))
							list_add_tail(&gbdc_connector->head, \
								&gbdc_connector_state->infinity_connector_list->head);
						else
							GB_PRINT_ERR("%s: error connector%d exist in connector%d\n", \
								__func__, \
								gbdc_connector->connector_id, \
								gbdc_connector_state->infinity_connector_list->connector_id);
					}
					break;
				}
			}
			drm_connector_list_iter_end(&conn_iter);
		}
	}

	if (!gbdc_connector_state->gbdc_infinity_enable) {
		unsigned size = 0;
		gbdc_connector_state->gbdc_infinity_enable = true;
		GB_PRINT_INFO("******%s set gbdc_infinity_enable true\n", __func__);

		gbdc_connector_state->infinity_adj.infinity_row = infinity_infos->infinity_row;
		gbdc_connector_state->infinity_adj.infinity_col = infinity_infos->infinity_col;

		memset(&gbdc_connector_state->infinity_set, 0, sizeof(gbdc_connector_state->infinity_set));
		/*ret = GB02FUNC1403(connector,
					infinity_infos);*/
		/*userspace uses one blob id, so do not use replaced blob, instead of a fixed one*/
		size = sizeof(*infinity_infos) + \
			(sizeof(struct GB02STR192) * \
			infinity_infos->infinity_row * infinity_infos->infinity_col);
		GB02FUNC1408(
										connector,
										infinity_infos,
										size);
	}


	return ret;
}

static int __maybe_unused GB02FUNC1620(struct drm_device *dev,
					struct drm_atomic_state *state,
					struct gbdc_connector_state *gbdc_connector_state,
					int i, int j,
					struct GB02STR192 *infos)
{

	return 0;
}

static int GB02FUNC1622(struct drm_atomic_state *state,
								struct drm_connector *conn,
								struct drm_crtc *crtc,
								struct GB02STR192 *info)
{
	struct drm_connector_state *conn_state;
	struct drm_crtc_state *crtc_state;
	struct drm_plane_state *plane_state;
	struct drm_plane *plane;
	struct gbdc_connector_state *gbdc_connector_state;
	struct gbdc_crtc_state *gbdc_crtc_state;
	struct gbdc_plane_state *gbdc_plane_state;
	struct gbdc_connector *gbdc_connector = NULL;
	struct gbdc_crtc *gbdc_crtc;
	struct gbdc_plane *gbdc_plane;
	//struct gbdc_connector *gbdc_conn = to_gbdc_connector(conn);
	int ret = 0;
	int n, i;
	unsigned int orderx = 0, ordery = 0;

	GB_PRINT_INFO("******%s \n", __func__);

	conn_state = drm_atomic_get_new_connector_state(state, conn);
	if (!conn_state)
		return -EINVAL;

	gbdc_connector_state = to_gbdc_connector_state(conn_state);
	gbdc_for_each_gbdc_obj(connector) {
		if (!gbdc_connector)
			continue;

		if (gbdc_connector->base.base.id == info->connector_id) {
			memcpy(gbdc_connector->padding, info->padding, sizeof(info->padding));
			for (i = 0; i < GB02MAC2560; i++) {
				if (info->padding[i].width != 0 \
					&& info->padding[i].height != 0) {
					GB_PRINT_INFO("commit connector padding:%d %d %d %d %d %d\n", \
						gbdc_connector->padding[i].width, \
						gbdc_connector->padding[i].height, \
						gbdc_connector->padding[i].top, \
						gbdc_connector->padding[i].right, \
						gbdc_connector->padding[i].bottom, \
						gbdc_connector->padding[i].left);
				}
			}
			orderx = gbdc_connector->order.x;
			ordery = gbdc_connector->order.y;
			break;
		}
	}

	if (!gbdc_connector) {
		GB_PRINT_ERR("find physical(%d) conn error\n", \
						info->connector_id);
		return -EINVAL;
	}

	return 0;

	crtc_state = drm_atomic_get_new_crtc_state(state, crtc);
	if (!crtc_state)
		return -EINVAL;

	gbdc_crtc_state = to_gbdc_crtc_state(crtc_state);
	gbdc_for_each_gbdc_obj(crtc) {
		if (!gbdc_crtc)
			continue;

		if (gbdc_crtc->order.x == orderx && gbdc_crtc->order.y == ordery) {

			for (i = 0; i < GB02MAC2560; i++) {
				if (gbdc_crtc->adjusted_mode.hdisplay == info->padding[i].width\
					&& gbdc_crtc->adjusted_mode.vdisplay == info->padding[i].height) {
					gbdc_crtc->padding = info->padding[i];
					GB_PRINT_INFO("commit crtc padding:%d %d %d %d %d %d\n", \
						gbdc_crtc->padding.width, \
						gbdc_crtc->padding.height, \
						gbdc_crtc->padding.top, \
						gbdc_crtc->padding.right, \
						gbdc_crtc->padding.bottom, \
						gbdc_crtc->padding.left);
					break;
				}
			}
			break;
		}
	}

	for_each_new_plane_in_state(state, plane, plane_state, n) {
		if (plane_state && plane_state->crtc == crtc) {
			gbdc_plane_state = to_gbdc_plane_state(plane_state);
			gbdc_for_each_gbdc_obj(plane) {
				if (!gbdc_plane)
					continue;

				if (gbdc_plane->order.x == orderx && gbdc_plane->order.y == ordery) {

					for (i = 0; i < GB02MAC2560; i++) {
						if (gbdc_plane->padding.width == info->padding[i].width\
							&& gbdc_plane->padding.height == info->padding[i].height) {
							gbdc_plane->padding = info->padding[i];
							GB_PRINT_INFO("commit plane padding:%d %d %d %d %d %d\n", \
								gbdc_plane->padding.width, \
								gbdc_plane->padding.height, \
								gbdc_plane->padding.top, \
								gbdc_plane->padding.right, \
								gbdc_plane->padding.bottom, \
								gbdc_plane->padding.left);
							break;
						}
					}
					//break;
				}
			}
		}
	}

	return ret;
}

static int __maybe_unused GB02FUNC1626(enum gbdc_infinity_info_type type,
								struct GB02STR192 *info,
								struct drm_atomic_state *state)
{
	int ret = 0;

	return ret;
}

static int GB02FUNC1628(struct drm_device *dev,
				struct drm_atomic_state *state,
				struct GB02STR198 *infinity)
{
	struct gbdc_crtc_state *gbdc_crtc_state = NULL;
	struct gbdc_connector_state *gbdc_connector_state = NULL;
	struct drm_connector_list_iter conn_iter;
	struct gbdc_plane_state *gbdc_plane_state = NULL;
	struct drm_plane *plane;
	struct drm_plane_state *plane_state;
	struct gbdc_plane *gbdc_plane;
	struct drm_crtc *crtc;
	struct drm_connector *conn = NULL;
	struct drm_crtc_state *crtc_state = NULL;
	struct drm_connector_state *conn_state = NULL;
	struct gbdc_connector *gbdc_connector = NULL;
	int ret = 0;
	bool valid = false;

	GB_PRINT_DBG("%s: check virtual connector\n", __func__);
	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		gbdc_connector = to_gbdc_connector(conn);

		if (!gbdc_connector->virt)
			continue;

		if (conn->base.id == infinity->infinity_connector_id) {

			valid = true;
			break;
		}
	}
	if (conn)
		drm_connector_get(conn);
	drm_connector_list_iter_end(&conn_iter);

	if (valid) {
		GB_PRINT_DBG("%s: check physical connector\n", __func__);
		conn_state = drm_atomic_get_new_connector_state(state, conn) ? \
			drm_atomic_get_new_connector_state(state, conn) : conn->state;
		gbdc_connector_state = to_gbdc_connector_state(conn_state);

		if (list_empty(&gbdc_connector_state->infinity_connector_list->head)) {
			INIT_LIST_HEAD(&gbdc_connector_state->infinity_connector_list->head);
			ret |= GB02FUNC1609(conn, state, gbdc_connector_state,
											infinity->infinity_infos);
		}
	}

	if (valid && conn_state && conn_state->crtc) {
		GB_PRINT_DBG("%s: check physical crtc\n", __func__);
		crtc = conn_state->crtc;
		crtc_state = drm_atomic_get_new_crtc_state(state, crtc) ? \
			drm_atomic_get_new_crtc_state(state, crtc) : conn_state->crtc->state;
		gbdc_crtc_state = to_gbdc_crtc_state(crtc_state);

		if (list_empty(&gbdc_crtc_state->infinity_crtc_list->head)) {
			INIT_LIST_HEAD(&gbdc_crtc_state->infinity_crtc_list->head);
			ret |= GB02FUNC1602(state, crtc, gbdc_crtc_state,
											gbdc_connector_state, gbdc_connector,
											infinity);
		}
	}

	if (conn)
		drm_connector_put(conn);

	if (!gbdc_crtc_state)
		return -1;

	GB_PRINT_DBG("%s: check physical plane\n", __func__);
	drm_for_each_plane(plane, dev) {
		if (!plane)
			continue;

		plane_state = drm_atomic_get_new_plane_state(state, plane) ? \
			drm_atomic_get_new_plane_state(state, plane) : plane->state;
		if (!plane_state)
			continue;
		gbdc_plane = to_gbdc_plane_info(plane);
		gbdc_plane_state = to_gbdc_plane_state(plane_state);

		if (!gbdc_plane->virt)
			continue;

		if (!plane_state->crtc)
			continue;

		if (!crtc_state->crtc)
			continue;

		if (plane_state && plane_state->crtc && plane_state->crtc == crtc_state->crtc) {

			if (list_empty(&gbdc_plane_state->infinity_plane_list->head)) {
				INIT_LIST_HEAD(&gbdc_plane_state->infinity_plane_list->head);

				ret |= GB02FUNC1583(state, gbdc_crtc_state,
						gbdc_plane_state, plane);

			}
		}
	}

	return ret;
}

static bool GB02FUNC1633(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct GB02STR198 *infinity)
{
	unsigned int row, col, i, j, n;
	int conn_id;
	struct GB02STR190 order;

	struct GB02STR194 *infinity_infos;
	bool ret = true;

	if (!infinity || !infinity->infinity_infos)
		return false;

	infinity_infos = infinity->infinity_infos;
	row = infinity_infos->infinity_row;
	col = infinity_infos->infinity_col;

	for (j = 0; j < col; j++) {
		for (i = 0; i < row; i++) {
			struct drm_connector *conn;
			struct drm_connector_list_iter conn_iter;
			bool found = false;

			conn_id = infinity_infos->infos[j * row + i].connector_id;
			order.x = infinity_infos->infos[j * row + i].order.x;
			order.y = infinity_infos->infos[j * row + i].order.y;

			/*avoid duplicated connectorid and order,
			  it will lead endless loop or other exception*/
			for (n = 0; n < j * row + i; n++) {
				if (infinity_infos->infos[n].connector_id == conn_id) {
					GB_PRINT_ERR("can not be same connector %d\n", conn_id);
					return false;
				}
				if (infinity_infos->infos[n].order.x == order.x && \
					infinity_infos->infos[n].order.y == order.y) {
					GB_PRINT_ERR("can not be same order %d,%d\n", order.x, order.y);
					return false;
				}
			}

			drm_connector_list_iter_begin(dev, &conn_iter);
			drm_for_each_connector_iter(conn, &conn_iter) {
				if (conn && conn->base.id == conn_id) {
					struct gbdc_connector *gbdc_connector = to_gbdc_connector(conn);

					if (gbdc_connector && !gbdc_connector->virt) {
						found = true;

						if (gbdc_infinity_check_connect_status) {
							/*check physical link status*/
							if (GB02FUNC874(conn) \
								!= connector_status_connected) {
								GB_PRINT_ERR("gbdc connector%d: %s\n", \
										conn_id, "disconnected");
								ret = false;
							}

							if (!GB02FUNC890(conn)) {
								GB_PRINT_ERR("gbdc connector%d: %s\n", \
												conn_id, "no edid");
								ret = false;
							}
						}
					}
					break;
				}
			}
			drm_connector_list_iter_end(&conn_iter);

			if (!found) {
				GB_PRINT_ERR("can not find connector %d\n", conn_id);
				ret = false;
				return ret;
			}
		}
	}
	return ret;
}

static int GB02FUNC1637(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct GB02STR198 *infinity)
{
	struct drm_plane *plane;
	struct drm_crtc *crtc;
	struct drm_connector *conn = NULL;
	struct drm_crtc_state *crtc_state;
	struct drm_plane_state *plane_state;
	struct drm_connector_state *conn_state = NULL;
	//int n;
	int ret = 0;
	//unsigned int row, col, orderx, ordery;
	//int i, j;
	struct gbdc_crtc_state *gbdc_crtc_state;
	struct gbdc_connector_state *gbdc_connector_state = NULL;
	struct gbdc_plane_state *gbdc_plane_state = NULL;
	//struct gbdc_plane *gbdc_plane;
	struct gbdc_connector *gbdc_connector;
	//struct gbdc_connector *gbdc_conn;
	struct drm_connector_list_iter conn_iter;
	bool found_virtual = false;

	if (!infinity || !infinity->infinity_infos)
		return -EINVAL;

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		gbdc_connector = to_gbdc_connector(conn);

		if (conn->base.id == infinity->infinity_connector_id) {
			//if (!drm_atomic_get_new_connector_state(state, conn))
			//	break;

			found_virtual = true;
			gbdc_connector->infinity_row = infinity->infinity_infos->infinity_row;
			gbdc_connector->infinity_row = infinity->infinity_infos->infinity_col;
			#if 0
			/*TODO: call connector get_modes*/
			if (conn->funcs && conn->funcs->detect)
				if (conn->funcs->detect(conn, true) == connector_status_connected)
					if (conn->helper_private && conn->helper_private->get_modes)
						conn->helper_private->get_modes(conn);
			#endif
			break;
		}
	}
	if (conn)
		drm_connector_get(conn);
	drm_connector_list_iter_end(&conn_iter);

	if (!found_virtual) {
		GB_PRINT_ERR("[CONNECTOR] infinity commit failed:connector is invalid\n");
		if (conn)
			drm_connector_put(conn);
		return -EINVAL;
	}

	conn_state = drm_atomic_get_new_connector_state(state, conn) ? \
		drm_atomic_get_new_connector_state(state, conn) : conn->state;

	if (conn_state) {

		gbdc_connector_state = to_gbdc_connector_state(conn_state);

		ret = GB02FUNC1494(state, conn, false) | \
			GB02FUNC1609(conn, state, gbdc_connector_state,
												infinity->infinity_infos);
	}

	//if (conn->funcs && conn->funcs->detect)
	//	if (conn->funcs->detect(conn, true) == connector_status_connected)
	//		if (conn->helper_private && conn->helper_private->get_modes)
	//			conn->helper_private->get_modes(conn);

	if (!conn_state || !conn_state->crtc) {
		GB_PRINT_INFO("[CONNECTOR] infinity commit failed:crtc is NULL\n");
		if (conn)
			drm_connector_put(conn);
		return 0;
	}

	crtc = conn_state->crtc;
	crtc_state = drm_atomic_get_new_crtc_state(state, crtc) ? \
		drm_atomic_get_new_crtc_state(state, crtc) : conn_state->crtc->state;
	gbdc_crtc_state = to_gbdc_crtc_state(crtc_state);

	ret = GB02FUNC1486(state, crtc, false);
	if (ret) {
		GB_PRINT_ERR("[CRTC:%d:%s] infinity destroy failed\n",
				crtc->base.id, crtc->name);
		if (conn)
			drm_connector_put(conn);
		return ret;
	}

	drm_for_each_plane(plane, dev) {
		struct gbdc_plane *gbdc_vplane = to_gbdc_plane_info(plane);

		plane_state = drm_atomic_get_new_plane_state(state, plane) ? \
			drm_atomic_get_new_plane_state(state, plane) : plane->state;

		if (!gbdc_vplane->virt)
			continue;

		if (plane_state && plane_state->crtc == crtc_state->crtc) {

			gbdc_plane_state = to_gbdc_plane_state(plane_state);


			ret = GB02FUNC1489(state, plane);
			if (ret) {
				GB_PRINT_ERR("[PLANE:%d:%s] infinity destroy failed\n",
						plane->base.id, plane->name);
				if (conn)
					drm_connector_put(conn);
				return ret;
			}

		}
	}
	/*
	 *  consider if we need this: if (infinity->recreated) return;
	 *  when infinity need recreate, if we do not add this, we use the previous configuration,
	 *  but now we use the new configuration recreated, it is faster but is it safe?
	 *  if we do not add this, we use the previous configuration till the next commit coming,
	 *  we update the new configuration and use it, it is much safe. Before this, we had to
	 *  destroy all crtc and plane lists.
	*/
	/*need to disable crtc, when modify the layout(2x4->3x2...), so ignore this. if do not do this,
	  when open a window in kyrin, the window will blink, but if not open a window, it is not*/
#if 0
	if (infinity->recreated) {
		GB_PRINT_INFO("[CONNECTOR] infinity commit failed:recreated\n");
		if (conn)
			drm_connector_put(conn);
		return -EINVAL;
	}
#endif

	crtc = conn_state->crtc;
	crtc_state = drm_atomic_get_new_crtc_state(state, crtc) ? \
		drm_atomic_get_new_crtc_state(state, crtc) : conn_state->crtc->state;
	gbdc_crtc_state = to_gbdc_crtc_state(crtc_state);

	ret = GB02FUNC1602(state, crtc, gbdc_crtc_state, \
			gbdc_connector_state, gbdc_connector, infinity);
	if (ret) {
		GB_PRINT_ERR("[CRTC:%d:%s] infinity commit failed\n",
				crtc->base.id, crtc->name);
		if (conn)
			drm_connector_put(conn);
		return ret;
	}
	GB_PRINT_INFO("[CRTC:%d:%s] infinity commit success\n",
				crtc->base.id, crtc->name);

	if (conn)
		drm_connector_put(conn);

	drm_for_each_plane(plane, dev) {
		struct gbdc_plane *gbdc_vplane = to_gbdc_plane_info(plane);

		plane_state = drm_atomic_get_new_plane_state(state, plane) ? \
			drm_atomic_get_new_plane_state(state, plane) : plane->state;

		if (!gbdc_vplane->virt)
			continue;

		if (plane_state && plane_state->crtc == crtc_state->crtc) {

			gbdc_plane_state = to_gbdc_plane_state(plane_state);


			ret = GB02FUNC1583(state, gbdc_crtc_state,
						gbdc_plane_state, plane);
			if (ret) {
				GB_PRINT_ERR("[PLANE:%d:%s] infinity commit failed\n",
						plane->base.id, plane->name);
				return ret;
			}

		}
		GB_PRINT_INFO("[PLANE:%d:%s] infinity commit success\n",
						plane->base.id, plane->name);
	}

	//infinity->infinity_connector_id = conn->base.id;

	return ret;
}

bool __maybe_unused GB02FUNC1648(struct drm_device *dev,
			struct drm_atomic_state *state, int conn_id)
{
	struct drm_connector *conn;
	struct drm_connector_list_iter conn_iter;
	bool ret = false;

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {

		if (conn->base.id == conn_id) {
			if (!drm_atomic_get_new_connector_state(state, conn) && !conn->state)
				ret = false;
			else
				ret = true;
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);

	return ret;
}

static int GB02FUNC1651(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct drm_crtc *crtc,
			struct drm_connector *conn,
			struct GB02STR198 *infinity,
			bool *rebuild)
{
	int i, j;
	unsigned int row, col/*, orderx, ordery*/;

	row = infinity->infinity_infos->infinity_row;
	col = infinity->infinity_infos->infinity_col;

	/*ergodic crtc && plane state infinity list*/
	for (j = 0; j < col; j++) {
		for (i = 0; i < row; i++) {
			if (!infinity->uinfo_changed[j * row + i].info_changed)
				continue;

			/*update virtual screen  refer operations*/
			if (infinity->uinfo_changed[j * row + i].st.id_changed) {
				GB_PRINT_INFO("commit id changed\n");
				//ret = GB02FUNC1626(INFINITY_CONN_ID,
				//		&infinity->infinity_infos->infos[j * row + i],
				//		state);
				*rebuild = true;
				infinity->padding_changed = true;
			}

			if (infinity->uinfo_changed[j * row + i].st.order_changed) {
				GB_PRINT_INFO("commit order changed\n");
				//ret = GB02FUNC1626(INFINITY_ORDER,
				//		&infinity->infinity_infos->infos[j * row + i],
				//		state);
				*rebuild = true;
				infinity->padding_changed = true;
			}

			if (infinity->uinfo_changed[j * row + i].st.padding_changed) {
				GB_PRINT_INFO("commit padding changed\n");

				GB02FUNC1622(state,
													conn,
													crtc,
				&infinity->infinity_infos->infos[j * row + i]);
				infinity->padding_changed = true;
			}

			infinity->uinfo_changed[j * row + i].info_changed = 0;
		}
	}

	return 0;
}

static int GB02FUNC1653(struct gbdc_crtc_state *gbdc_crtc_state)
{
	struct gbdc_crtc *gbdc_crtc;
	int crtc_id = 0;

	GB_PRINT_INFO("GB02FUNC1653 %p\n", gbdc_crtc_state);

	if (!gbdc_crtc_state)
		return -EINVAL;

	gbdc_for_each_gbdc_obj(crtc) {

		if (!gbdc_crtc)
			continue;

		crtc_id = gbdc_crtc->crtc_id;
		GB_PRINT_INFO("crtc_id %d\n", crtc_id);
	}

	return 0;
}

static int GB02FUNC1656(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct drm_crtc *crtc,
			struct drm_connector_state *conn_state,
			struct gbdc_connector *gbdc_conn,
			struct GB02STR198 *infinity)
{
	struct gbdc_crtc_state *gbdc_crtc_state;
	struct gbdc_connector_state *gbdc_connector_state = NULL;
	struct gbdc_connector *gbdc_connector;
	struct drm_plane *plane;
	unsigned int pos = 0;
	bool custom_mode = false;
	int i;

	GB_PRINT_INFO("******%s \n", __func__);

	gbdc_crtc_state =
		to_gbdc_crtc_state(drm_atomic_get_new_crtc_state(state, crtc));
	gbdc_connector_state = to_gbdc_connector_state(conn_state);

	GB02FUNC1653(gbdc_crtc_state);

	/*if the mode is not with padding, set current padding value to 0*/
	custom_mode = GB02FUNC1590(gbdc_crtc_state, gbdc_conn);
	if (!custom_mode) {
		GB_PRINT_INFO(\
			"%s: set infinity padding to 0\n", __func__);

		/*clear adjust and infinity_info padding value*/
		GB02FUNC1592(gbdc_connector_state, infinity->infinity_infos);
		infinity->padding_changed = true;
	}

	if (gbdc_connector_state && gbdc_crtc_state) {

		int crtc_id = 0;
		struct gbdc_crtc *gbdc_crtc;

		gbdc_for_each_gbdc_obj(connector) {
			if (!gbdc_connector)
				continue;

			crtc_id = gbdc_connector->connector_id;
			if (crtc_id >= GB02MAC2693)
				continue;

			gbdc_crtc =
				GB02FUNC85((dev)->dev_private)->kms_info.crtcs[crtc_id];

			if (gbdc_crtc && !gbdc_crtc->virt) {
				if (!custom_mode) {
					memset(gbdc_connector->padding, 0, sizeof(gbdc_connector->padding));
					for (pos = 0; pos < GB02MAC2558; pos++) {
						if (infinity->infinity_infos->infos[pos].connector_id \
								== gbdc_connector->base.base.id) {
							memcpy(infinity->infinity_infos->infos[pos].padding, \
								gbdc_connector->padding, sizeof(gbdc_connector->padding));
							GB02FUNC963(
														gbdc_conn,
														INFINITY_PADDING,
														pos,
														gbdc_connector->padding);
							break;
						}
					}
				}

				GB02FUNC1589(gbdc_crtc_state, gbdc_crtc, gbdc_connector,
											gbdc_conn);

				for (i = 0; i < GB02MAC2560; i++) {
					if (gbdc_crtc->adjusted_mode.hdisplay == gbdc_connector->padding[i].width \
						&& gbdc_crtc->adjusted_mode.vdisplay == gbdc_connector->padding[i].height) {
						gbdc_crtc->padding = gbdc_connector->padding[i];
						GB_PRINT_INFO("commit crtc padding:%d %d %d %d %d %d\n", \
						gbdc_crtc->padding.width, \
						gbdc_crtc->padding.height, \
						gbdc_crtc->padding.top, \
						gbdc_crtc->padding.right, \
						gbdc_crtc->padding.bottom, \
						gbdc_crtc->padding.left);
						//gbdc_crtc->geometry.x += gbdc_crtc->padding.left;
						//gbdc_crtc->geometry.y += gbdc_crtc->padding.top;
						break;
					}
				}

				if (!GB02FUNC812(gbdc_conn, &gbdc_crtc_state->base.adjusted_mode)) {
					GB_PRINT_INFO("origin mode set padding to 0\n");
					memset(&gbdc_crtc->padding, 0, sizeof(gbdc_crtc->padding));
					//gbdc_crtc->geometry.x += gbdc_crtc->padding.left;
					//gbdc_crtc->geometry.y += gbdc_crtc->padding.top;
				} else {
					gbdc_crtc->geometry.x += gbdc_crtc->padding.left;
					gbdc_crtc->geometry.y += gbdc_crtc->padding.top;

					GB_PRINT_INFO("crtc(%d) geometry.x + %d\n", \
						gbdc_crtc->crtc_id, gbdc_crtc->padding.left);
					GB_PRINT_INFO("crtc(%d) geometry.y + %d\n", \
						gbdc_crtc->crtc_id, gbdc_crtc->padding.top);

					GB02FUNC1596(gbdc_crtc, infinity->infinity_infos);
				}

				for (pos = 0; pos < GB02MAC2558; pos++) {
					if (infinity->infinity_infos->infos[pos].connector_id == \
							gbdc_connector->base.base.id) {
						infinity->infinity_infos->infos[pos].geometry = gbdc_crtc->geometry;
						GB02FUNC963(
													gbdc_conn,
													INFINITY_GEOMETRY,
													pos,
													&gbdc_crtc->geometry);
						break;
					}
				}

				drm_for_each_plane(plane, dev) {
					struct gbdc_plane *gbdc_plane = to_gbdc_plane_info(plane);

					if (gbdc_plane->virt)
						continue;

					if (crtc_id != gbdc_plane->crtc_id)
						continue;

					GB02FUNC1579(gbdc_crtc, &gbdc_plane->geometry);
					GB02FUNC1581(gbdc_crtc, &gbdc_plane->padding);
					GB_PRINT_INFO("commit plane padding:%d %d %d %d %d %d\n", \
						gbdc_plane->padding.width, \
						gbdc_plane->padding.height, \
						gbdc_plane->padding.top, \
						gbdc_plane->padding.right, \
						gbdc_plane->padding.bottom, \
						gbdc_plane->padding.left);
				}

			}
		}

		infinity->mode_changed = false;
	}

	return 0;
}

static int GB02FUNC1663(struct drm_device *dev,
			struct drm_atomic_state *state,
			struct GB02STR198 *infinity)
{
	//struct drm_mode_config *config = &dev->mode_config;

	struct drm_crtc *crtc;
	struct drm_connector *conn;

	struct drm_connector_state *conn_state = NULL;
	int n, ret = 0;

	struct gbdc_connector *gbdc_conn;

	for_each_new_connector_in_state(state, conn, conn_state, n) {
		if (infinity->infinity_connector_id == conn->base.id) {
			crtc = conn_state->crtc;
			gbdc_conn = to_gbdc_connector(conn);

			if (infinity->uinfo_changed && infinity->infinity_infos) {
				bool rebuild = false;

				ret = GB02FUNC1651(dev,
													state,
													crtc,
													conn,
													infinity,
													&rebuild);

				if (rebuild) {
					GB_PRINT_INFO("rebuild GB02FUNC1637\n");
					infinity->rebuild = rebuild;
					if (GB02FUNC1633(dev, state, infinity))
						ret = GB02FUNC1637(dev,
														state,
														infinity);
					else
						ret = -EINVAL;
					if (ret)
						GB_PRINT_ERR("rebuild GB02FUNC1637 failed\n");
					rebuild = false;
				}
			}

			if (infinity->mode_changed) {
				if (crtc) {

					ret = GB02FUNC1656(dev,
														state,
														crtc,
														conn_state,
														gbdc_conn,
														infinity);
				}
			}

			if (infinity->destroyed) {
				//infinity->destroyed = false;
				//ret = GB02FUNC1499(state, infinity);
				//return ret;
			}

			if (infinity->recreated) {

			}

			break;
		}
	}

	return ret;
}

int GB02FUNC1665(struct drm_device *dev,
			struct drm_atomic_state *state)
{
	int ret = 0;
	struct GB02STR198 *infinity = NULL;
	struct GB02STR198 *n_infinity = NULL;

	GB_PRINT_DBG("%s\n", __func__);

	gbdc_for_each_infinity_safe(infinity, dev) {

		if (!infinity || !infinity->infinity_infos)
			continue;

		//if (!infinity->changed)
		//	continue;

		if (infinity->destroyed)
			continue;

		if (infinity->created) {

			if (GB02FUNC1633(dev, state, infinity)) {
				GB_PRINT_INFO("GB02FUNC1637 begin\n");
				ret = GB02FUNC1637(dev, state, infinity);

				if (ret) {
					GB_PRINT_ERR("GB02FUNC1637 fail\n");
					continue;
				}

				//infinity->infinity_infos->infinity_enable = 1;
				//infinity->created = false;
				GB_PRINT_INFO("GB02FUNC1637 success\n");
			} else {

				GB_PRINT_ERR("GB02FUNC1637 check failed\n");
				/*delete*/
				ret = GB02FUNC1499(state, infinity);
				continue;
			}

		}

		if (infinity->recreated) {

			if (GB02FUNC1633(dev, state, infinity)) {

				GB_PRINT_INFO("gbdc_infinity_recreate_commit begin\n");
				ret = GB02FUNC1637(dev, state, infinity);

				if (ret) {
					GB_PRINT_ERR("gbdc_infinity_recreate_commit fail\n");
					continue;
				}

				GB_PRINT_INFO("gbdc_infinity_recreate_commit success\n");

			} else {

				GB_PRINT_ERR("gbdc_infinity_recreate_commit check failed\n");
				/*delete*/
				ret = GB02FUNC1499(state, infinity);
				continue;
			}

		}

		if (GB02FUNC1648(dev, state,
				infinity->infinity_connector_id)) {

			if (!infinity->infinity_infos->infinity_enable)
				continue;

			GB02FUNC1628(dev, state, infinity);

			if (!infinity->changed && !infinity->mode_changed)
				continue;

			GB_PRINT_DBG("gbdc commit changes %d %d 0x%x\n",
					infinity->changed, infinity->mode_changed,
					infinity->uinfo_changed ? infinity->uinfo_changed->info_changed : 0);

			ret = GB02FUNC1663(dev, state, infinity);
		}

		//infinity->changed = false;
	}
	GB_PRINT_DBG("%s end\n", __func__);
	return ret;
}

void GB02FUNC1671(struct work_struct *work)
{
	struct GB02STR70 *pcie_info = GB02FUNC518();
	struct drm_device *dev = NULL;
	struct GB02STR245 *kms_info = container_of(work, struct GB02STR245,
						  infinity_hotplug_work.work);

	GB_PRINT_INFO("%d GB02FUNC1671...................................\n",
		kms_info->connect_status_change);
	if (pcie_info && pcie_info->gbdev && pcie_info->gbdev->ddev)
		dev = pcie_info->gbdev->ddev;

	if (!dev)
		return;

	if (kms_info->connect_status_change) {
		drm_helper_hpd_irq_event(dev);
		kms_info->connect_status_change = false;
	} else {
		if (dev->mode_config.poll_enabled)
			drm_kms_helper_hotplug_event(dev);
	}
}

void GB02FUNC1674(struct drm_atomic_state *state)
{
	struct drm_device *dev = state->dev;
	bool hpd_report = false;
	struct GB02STR198 *infinity = NULL;
	struct GB02STR198 *n_infinity = NULL;
	struct GB02STR194 *infinity_infos;
	struct GB02STR39 *gb_dev = GB02FUNC518()->gbdev;
	struct GB02STR245 *kms_info = &gb_dev->gbdc_dev->kms_info;
	struct drm_connector *conn = NULL;
	bool found_virtual = false;
	struct drm_connector_list_iter conn_iter;

	gbdc_for_each_infinity_safe(infinity, dev) {
		if (!infinity || !infinity->infinity_infos)
			continue;

		if (!infinity->changed)
			continue;

		infinity_infos = infinity->infinity_infos;

		if (infinity->recreated) {

			drm_connector_list_iter_begin(dev, &conn_iter);
			drm_for_each_connector_iter(conn, &conn_iter) {

				if (conn->base.id == infinity->infinity_connector_id) {

					found_virtual = true;
					break;
				}
			}
			if (conn)
				drm_connector_get(conn);
			drm_connector_list_iter_end(&conn_iter);

			if (found_virtual) {
				//if (GB02FUNC1531(state,
				//		conn,
				//		DRM_MODE_DPMS_ON))
				//	GB_PRINT_ERR("recreate change GB02FUNC1531 error\n");
			}
			if (conn)
				drm_connector_put(conn);
			GB_PRINT_INFO("%s: infinity %d create change\n", \
				__func__, infinity->infinity_connector_id);
			infinity->recreated = false;
			hpd_report = true;
		}

		if (infinity->padding_changed) {

			infinity->padding_changed = false;

			GB_PRINT_INFO("%s: infinity %d padding change\n", \
				__func__, infinity->infinity_connector_id);
			hpd_report = true;
		}

		if (infinity->tiny_recreated) {

			infinity->tiny_recreated = false;

			GB_PRINT_INFO("%s: infinity %d tiny_recreated change\n", \
				__func__, infinity->infinity_connector_id);
			hpd_report = true;
		}

		if (infinity->destroyed) {

			GB_PRINT_INFO("%s: destroy %d\n", __func__, infinity->infinity_connector_id);
			infinity->changed = false;
			GB02FUNC1499(state, infinity);
			/*set this crtc not active or no enable?*/
			//if (conn_state && conn_state->crtc && conn_state->crtc->state)
			//	conn_state->crtc->state->active = false;
			hpd_report = true;
			kms_info->connect_status_change = true;
			continue;
		}

		if (infinity->created || !infinity_infos->infinity_enable) {

			GB_PRINT_INFO("%s: create %d\n", __func__, infinity->infinity_connector_id);
			infinity->created = false;
			infinity_infos->infinity_enable = 1;
			hpd_report = true;
			kms_info->connect_status_change = true;
		}

		infinity->changed = false;
	}

	if (hpd_report)
		schedule_delayed_work(
			&GB02FUNC85((dev)->dev_private)->kms_info.infinity_hotplug_work, 0);

}

void GB02FUNC1676(struct drm_device *dev)
{
	struct GB02STR39 *gb_dev = GB02FUNC518()->gbdev;
	struct GB02STR245 *kms_info = &gb_dev->gbdc_dev->kms_info;
	kms_info->connect_status_change = true;
	schedule_delayed_work(
			&GB02FUNC85((dev)->dev_private)->kms_info.infinity_hotplug_work, 0);

}

void GB02FUNC1677(struct drm_device *dev, int crtc_id)
{
	int row, col;
	struct drm_crtc *crtc;

	unsigned int conn_id;
	struct GB02STR198 *infinity = NULL;
	unsigned long int flags;

	return;

	spin_lock_irqsave(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	gbdc_for_each_infinity(infinity, dev) {
		struct GB02STR194 *info = infinity->infinity_infos;
		struct drm_connector *connector;
		struct drm_connector_list_iter conn_iter;

		if (!info)
			continue;

		row = infinity->infinity_infos->infinity_row;
		col = infinity->infinity_infos->infinity_col;
		conn_id = infinity->infinity_connector_id;

		drm_connector_list_iter_begin(dev, &conn_iter);
		drm_for_each_connector_iter(connector, &conn_iter) {
			if (connector->base.id == conn_id) {
				struct gbdc_crtc *gbdc_crtc;
				struct gbdc_crtc *gbdc_vcrtc;
				struct gbdc_crtc_state *gbdc_crtc_state;

				if (!connector->state || !connector->state->crtc)
					continue;

				crtc = connector->state->crtc;

				gbdc_vcrtc = GB02FUNC1573(crtc);

				if (gbdc_vcrtc->vblank_id == crtc_id) {
					gbdc_crtc_state = to_gbdc_crtc_state(crtc->state);
					gbdc_for_each_gbdc_obj(crtc) {
						if (!gbdc_crtc)
							continue;

						if (GB02FUNC1500(gbdc_crtc->crtc_id)) {
							GB_PRINT_INFO("%s: change vblank id from %d to %d\n", \
								__func__, gbdc_vcrtc->vblank_id, gbdc_crtc->crtc_id);
							gbdc_vcrtc->vblank_id = gbdc_crtc->crtc_id;
							gbdc_crtc_state->vblank_id = gbdc_crtc->crtc_id;
						}
					}
				}
				break;
			}
		}
		drm_connector_list_iter_end(&conn_iter);

	}
	spin_unlock_irqrestore(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
}

bool GB02FUNC1680(
							struct drm_connector *connector,
							struct GB02STR198 *infi,
							bool irq)
{
	struct drm_device *dev = connector->dev;
	struct GB02STR198 *infinity = NULL;
	int j, i, row, col;
	struct GB02STR194 *infinity_infos;
	unsigned long int flags = 0;

	if (irq)
		spin_lock(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock);
	else
		spin_lock_irqsave(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	gbdc_for_each_infinity(infinity, dev) {
		if (!infinity || !infinity->infinity_infos)
			continue;

		//if (infinity->create || infinity->recreate || !infinity->infinity_infos->infinity_enable)
		//	continue;

		/* if (infinity->destroyed)
			continue;

		if (infinity->padding_changed)
			continue; */

		if (infi && infi != infinity)
			continue;

		infinity_infos = infinity->infinity_infos;
		if (infinity_infos->infinity_enable) {
			row = infinity_infos->infinity_row;
			col = infinity_infos->infinity_col;

			if (infinity->infinity_connector_id == connector->base.id) {
				if (irq)
					spin_unlock(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock);
				else
					spin_unlock_irqrestore(
						&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
				return true;
			}

			for (j = 0; j < col; j++) {
				for (i = 0; i < row; i++) {
					int conn_id = infinity_infos->infos[j * row + i].connector_id;

					if (connector->base.id == conn_id) {
						if (irq)
							spin_unlock(
								&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock);
						else
							spin_unlock_irqrestore(
								&GB02FUNC85(
									(dev)->dev_private)->kms_info.infinity_lock, flags);
						return true;
					}
				}
			}
		}
	}
	if (irq)
		spin_unlock(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock);
	else
		spin_unlock_irqrestore(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);

	return false;
}

void GB02FUNC1684(struct drm_device *dev,
							struct drm_connector *connector)
{
	struct GB02STR198 *infinity = NULL;
	int j, i, row, col;
	struct GB02STR194 *infinity_infos;
	int vconn_id = 0;
	struct gbdc_connector *gbdc_conn;
	unsigned long int flags;
	struct drm_connector_list_iter conn_iter;
	struct drm_connector *conn = NULL;
	enum drm_connector_status status;
	int conn_id;
	int pos;

	spin_lock_irqsave(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	gbdc_for_each_infinity(infinity, dev) {
		if (!infinity || !infinity->infinity_infos)
			continue;

		//if (infinity->create || infinity->recreate || !infinity->infinity_infos->infinity_enable)
		//	continue;

		/* if (infinity->destroyed)
			continue;

		if (infinity->padding_changed)
			continue; */

		infinity_infos = infinity->infinity_infos;
		if (infinity_infos->infinity_enable) {
			row = infinity_infos->infinity_row;
			col = infinity_infos->infinity_col;

			if (infinity->infinity_connector_id == connector->base.id)
				GB_PRINT_INFO("can not be virtual connector\n");

			for (j = 0; j < col; j++) {
				for (i = 0; i < row; i++) {

					conn_id = infinity_infos->infos[j * row + i].connector_id;

					if (connector->base.id == conn_id) {
						vconn_id = infinity->infinity_connector_id;
						status = infinity_infos->infos[j * row + i].connected = \
							GB02FUNC874(connector);
						pos = j * row + i;
					}
				}
			}
		}
	}

	spin_unlock_irqrestore(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);

	if (!vconn_id)
		return;

	GB_PRINT_INFO("set connector%d: %s\n", conn_id, status == \
			connector_status_connected ? "connected" : "disconnected");

	mutex_lock(&dev->mode_config.mutex);
	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		if (vconn_id == conn->base.id) {
			gbdc_conn = to_gbdc_connector(conn);
			GB02FUNC963(
				gbdc_conn,
				INFINITY_CONN_STAT,
				pos,
				&status);
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);
	mutex_unlock(&dev->mode_config.mutex);

	return;
}

void GB02FUNC1689(struct drm_device *dev, int id)
{
	struct drm_connector_list_iter conn_iter;
	struct drm_connector *conn;
	struct drm_connector *connector = NULL;

	mutex_lock(&dev->mode_config.mutex);
	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		struct gbdc_connector *gbdc_connector = to_gbdc_connector(conn);

		if (gbdc_connector->connector_id == id) {
			connector = conn;
			drm_connector_get(conn);
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);
	mutex_unlock(&dev->mode_config.mutex);

	if (connector) {
		GB02FUNC1684(dev, connector);
		drm_connector_put(conn);
	}
}

bool GB02FUNC1691(struct drm_device *dev, int connector,
							struct GB02STR198 *infi, bool irq)
{
	struct drm_connector_list_iter conn_iter;
	struct drm_connector *conn;
	bool contained = false;
	//struct GB02STR198 *infinity = NULL;
	//int j, i, row, col;
	//struct GB02STR194 *infinity_infos;

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		struct gbdc_connector *gbdc_connector = to_gbdc_connector(conn);

		if (gbdc_connector->connector_id == connector) {
			contained = GB02FUNC1680(conn, infi, irq);
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);

	return contained;
}

int GB02FUNC1692(struct drm_device *dev, int phy_id, bool irq)
{
	struct drm_connector_list_iter conn_iter;
	struct drm_connector *conn;
	//int j, i, row, col;
	struct GB02STR198 *infinity;
	struct GB02STR194 *infinity_infos;
	int id = -1;
	unsigned long int flags = 0;

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(conn, &conn_iter) {
		struct gbdc_connector *gbdc_vconnector = to_gbdc_connector(conn);

		if (irq)
			spin_lock(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock);
		else
			spin_lock_irqsave(&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
		gbdc_for_each_infinity(infinity, dev) {
			if (!infinity || !infinity->infinity_infos)
				continue;

			if (infinity->infinity_connector_id != conn->base.id)
				continue;

			infinity_infos = infinity->infinity_infos;
			if (infinity_infos->infinity_enable) {
				struct gbdc_connector_state *gbdc_connector_state;
				struct drm_connector_state *state = conn->state;
				struct gbdc_connector *gbdc_connector;
				gbdc_connector_state = to_gbdc_connector_state(state);

				if (gbdc_connector_state) {
					gbdc_for_each_gbdc_obj(connector) {
						if (gbdc_connector->connector_id == phy_id) {
							id = gbdc_vconnector->connector_id;
							if (irq)
								spin_unlock(
									&GB02FUNC85(
										(dev)->dev_private)->kms_info.infinity_lock);
							else
								spin_unlock_irqrestore(
									&GB02FUNC85(
										(dev)->dev_private)->kms_info.infinity_lock, flags);
							goto end;
						}
					}
				}
			}
		}
		if (irq)
			spin_unlock(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock);
		else
			spin_unlock_irqrestore(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	}

end:
	drm_connector_list_iter_end(&conn_iter);

	return id;
}

void GB02FUNC1696(struct drm_atomic_state *old_state)
{
	struct drm_crtc_state *new_crtc_state;
	struct drm_crtc *crtc;
	int i;

	for_each_new_crtc_in_state(old_state, crtc, new_crtc_state, i) {
		unsigned long flags;

		if (!GB02FUNC1573(crtc)->virt)
			continue;

		if (!GB02FUNC1573(crtc)->need_disable)
			continue;

		if (GB02FUNC1573(crtc)->is_enable)
			continue;

		GB_PRINT_INFO("CRTC-%d need fake vblank\n", crtc->base.id);
		spin_lock_irqsave(&old_state->dev->event_lock, flags);
		if (new_crtc_state->event) {
			drm_crtc_send_vblank_event(crtc,
						   new_crtc_state->event);
			new_crtc_state->event = NULL;
		}
		spin_unlock_irqrestore(&old_state->dev->event_lock, flags);
	}
}

ssize_t __gbdc_infinity_info_show(struct drm_device *dev, char *buf)
{
	int len = 0;
	int j, i, row, col, n;
	struct GB02STR198 *infinity = NULL;
	unsigned long int flags;

	spin_lock_irqsave(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	gbdc_for_each_infinity(infinity, dev) {
		struct GB02STR194 *info = infinity->infinity_infos;
		if (!info)
			continue;

		row = infinity->infinity_infos->infinity_row;
		col = infinity->infinity_infos->infinity_col;

		len += snprintf(buf + len, PAGE_SIZE, "infinity(%d) info:\n", info->infinity_id);
		len += snprintf(buf + len, PAGE_SIZE, "{\n");
		len += snprintf(buf + len, PAGE_SIZE, "\tinfinity_id:%d\n", info->infinity_id);
		len += snprintf(buf + len, PAGE_SIZE, "\tinfinity_enable:%d\n", info->infinity_enable);
		len += snprintf(buf + len, PAGE_SIZE, "\tinfinity_row:%d\n", info->infinity_row);
		len += snprintf(buf + len, PAGE_SIZE, "\tinfinity_col:%d\n", info->infinity_col);

		len += snprintf(buf + len, PAGE_SIZE, "\tinfinity connector info:\n");
		for (j = 0; j < col; j++) {
			len += snprintf(buf + len, PAGE_SIZE, "\t{\n");
			for (i = 0; i < row; i++) {
				struct GB02STR192 *infos =
					&infinity->infinity_infos->infos[j * row + i];
				len += snprintf(buf + len, PAGE_SIZE, "\t\tconnector_id:%d\n", infos->connector_id);
				len += snprintf(buf + len, PAGE_SIZE, "\t\tconnected:%d\n", infos->connected);
				len += snprintf(buf + len, PAGE_SIZE, "\t\torder\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\t{\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\t\tx:%d\n", infos->order.x);
				len += snprintf(buf + len, PAGE_SIZE, "\t\t\ty:%d\n", infos->order.y);
				len += snprintf(buf + len, PAGE_SIZE, "\t\t}\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\tgeometry\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\t{\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\t\tx:%d\n", infos->geometry.x);
				len += snprintf(buf + len, PAGE_SIZE, "\t\t\ty:%d\n", infos->geometry.y);
				len += snprintf(buf + len, PAGE_SIZE, "\t\t\tw:%d\n", infos->geometry.w);
				len += snprintf(buf + len, PAGE_SIZE, "\t\t\th:%d\n", infos->geometry.h);
				len += snprintf(buf + len, PAGE_SIZE, "\t\t}\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\tpadding\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\t{\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\t\twidth\theight\ttop\tright\tbottom\tleft\n");
				for (n = 0; n < GB02MAC2560; n++)
					if (infos->padding[n].width && infos->padding[n].height)
						len += snprintf(buf + len, PAGE_SIZE, "\t\t\t%d\t%d\t%d\t%d\t%d\t%d\n", \
															infos->padding[n].width, \
															infos->padding[n].height, \
															infos->padding[n].top, \
															infos->padding[n].right, \
															infos->padding[n].bottom, \
															infos->padding[n].left);
				len += snprintf(buf + len, PAGE_SIZE, "\t\t}\n");
			}
			len += snprintf(buf + len, PAGE_SIZE, "\t}\n");
		}
		len += snprintf(buf + len, PAGE_SIZE, "}\n");
	}
	spin_unlock_irqrestore(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	return len < PAGE_SIZE ? len : PAGE_SIZE - 1;
}

ssize_t __gbdc_infinity_state_show(struct drm_device *dev, char *buf)
{
	int len = 0;
	int /*j, i, */row, col, n;
	struct drm_plane *plane;
	struct drm_crtc *crtc;

	unsigned int conn_id;
	struct GB02STR198 *infinity = NULL;
	unsigned long int flags;

	spin_lock_irqsave(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	gbdc_for_each_infinity(infinity, dev) {
		struct GB02STR194 *info = infinity->infinity_infos;
		struct drm_connector *connector;
		//struct drm_crtc_state *crtc_state;
		//struct drm_plane_state *plane_state;
		//struct drm_connector_state *conn_state;
		struct drm_connector_list_iter conn_iter;

		if (!info)
			continue;

		row = infinity->infinity_infos->infinity_row;
		col = infinity->infinity_infos->infinity_col;
		//conn_id = infinity_infos->infos[j * row + i].connector_id;
		conn_id = infinity->infinity_connector_id;

		len += snprintf(buf + len, PAGE_SIZE, "infinity(%d) state:\n", info->infinity_id);
		len += snprintf(buf + len, PAGE_SIZE, "{\n");

		drm_connector_list_iter_begin(dev, &conn_iter);
		drm_for_each_connector_iter(connector, &conn_iter) {
			if (connector->base.id == conn_id) {
				struct gbdc_connector *gbdc_connector;
				struct gbdc_crtc *gbdc_crtc;
				struct gbdc_plane *gbdc_plane;
				struct gbdc_crtc_state *gbdc_crtc_state;
				struct gbdc_plane_state *gbdc_plane_state;
				struct gbdc_connector_state *gbdc_connector_state;

				if (!connector->state || !connector->state->crtc)
					continue;

				crtc = connector->state->crtc;

				gbdc_connector = to_gbdc_connector(connector);
				gbdc_connector_state = to_gbdc_connector_state(connector->state);
				len += snprintf(buf + len, PAGE_SIZE, "\tconnector:%d\n", conn_id);
				len += snprintf(buf + len, PAGE_SIZE, "\t{\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\tenable: %d\n", \
						gbdc_connector_state->gbdc_infinity_enable);
				gbdc_for_each_gbdc_obj(connector) {
					if (!gbdc_connector)
						continue;

					len += snprintf(buf + len, PAGE_SIZE, "\t\tconnector%d: order(%d, %d)\n", \
						gbdc_connector->base.base.id, gbdc_connector->order.x, gbdc_connector->order.y);
					for (n = 0; n < GB02MAC2560; n++)
						if (gbdc_connector->padding[n].width && gbdc_connector->padding[n].height)
							len += snprintf(buf + len, PAGE_SIZE, \
											"\t\t\t pad(%d %d %d %d %d %d)\n", \
															gbdc_connector->padding[n].width, \
															gbdc_connector->padding[n].height, \
															gbdc_connector->padding[n].top, \
															gbdc_connector->padding[n].right, \
															gbdc_connector->padding[n].bottom, \
															gbdc_connector->padding[n].left);
				}
				len += snprintf(buf + len, PAGE_SIZE, "\t}\n");

				gbdc_crtc = GB02FUNC1573(crtc);
				gbdc_crtc_state = to_gbdc_crtc_state(crtc->state);
				len += snprintf(buf + len, PAGE_SIZE, "\tcrtc:%d\n", crtc->base.id);
				len += snprintf(buf + len, PAGE_SIZE, "\t{\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\tvblank: %d\n", gbdc_crtc->vblank_id);
				len += snprintf(buf + len, PAGE_SIZE, "\t\tneed_disable: %d\n", gbdc_crtc->need_disable);

				gbdc_for_each_gbdc_obj(crtc) {
					if (!gbdc_crtc)
						continue;

					len += snprintf(buf + len, PAGE_SIZE, "\t\tcrtc%d: order(%d, %d)\n", \
						gbdc_crtc->base.base.id, gbdc_crtc->order.x, gbdc_crtc->order.y);
					len += snprintf(buf + len, PAGE_SIZE, "\t\tmode:\n");
					len += snprintf(buf + len, PAGE_SIZE, "\t\t{\n");
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\tclock:%d\n", \
											gbdc_crtc->adjusted_mode.clock);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\thdisplay:%d\n", \
											gbdc_crtc->adjusted_mode.hdisplay);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\thsync_start:%d\n", \
											gbdc_crtc->adjusted_mode.hsync_start);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\thsync_end:%d\n", \
											gbdc_crtc->adjusted_mode.hsync_end);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\thtotal:%d\n", \
											gbdc_crtc->adjusted_mode.htotal);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\thskew:%d\n", \
											gbdc_crtc->adjusted_mode.hskew);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\tvdisplay:%d\n", \
											gbdc_crtc->adjusted_mode.vdisplay);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\tvsync_start:%d\n", \
											gbdc_crtc->adjusted_mode.vsync_start);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\tvsync_end:%d\n", \
											gbdc_crtc->adjusted_mode.vsync_end);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\tvtotal:%d\n", \
											gbdc_crtc->adjusted_mode.vtotal);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t\tvscan:%d\n", \
											gbdc_crtc->adjusted_mode.vscan);
					len += snprintf(buf + len, PAGE_SIZE, "\t\t}\n");
					len += snprintf(buf + len, PAGE_SIZE, "\t\tgeometry(%d %d %d %d)\n", \
						gbdc_crtc->geometry.x, gbdc_crtc->geometry.y, \
						gbdc_crtc->geometry.w, gbdc_crtc->geometry.h);
					len += snprintf(buf + len, PAGE_SIZE, "\t\tpad(%d %d %d %d %d %d)\n", \
						gbdc_crtc->padding.width, gbdc_crtc->padding.height, \
						gbdc_crtc->padding.top, gbdc_crtc->padding.right, \
						gbdc_crtc->padding.bottom, gbdc_crtc->padding.left);
				}
				len += snprintf(buf + len, PAGE_SIZE, "\t}\n");

				drm_for_each_plane(plane, dev) {
					if (plane->state && plane->state->crtc == crtc) {
						len += snprintf(buf + len, PAGE_SIZE, "\tplane-%d(%d):\n", \
														plane->type, plane->base.id);
						gbdc_plane_state = to_gbdc_plane_state(plane->state);
						len += snprintf(buf + len, PAGE_SIZE, "\t{\n");
						gbdc_for_each_gbdc_obj(plane) {
							if (!gbdc_plane)
								continue;

							len += snprintf(buf + len, PAGE_SIZE, \
								"\t\tplane-%d(%d): order(%d, %d)\n", plane->type, \
								gbdc_plane->base.base.id, \
								gbdc_plane->order.x, gbdc_plane->order.y);
							len += snprintf(buf + len, PAGE_SIZE, \
								"\t\t		 geometry(%d %d %d %d)\n", \
												gbdc_plane->geometry.x, \
												gbdc_plane->geometry.y, \
												gbdc_plane->geometry.w, \
												gbdc_plane->geometry.h);
							len += snprintf(buf + len, PAGE_SIZE, \
								"\t\t		 padding(%d %d %d %d %d %d)\n", \
												gbdc_plane->padding.width, \
												gbdc_plane->padding.height, \
												gbdc_plane->padding.top, \
												gbdc_plane->padding.right, \
												gbdc_plane->padding.bottom, \
												gbdc_plane->padding.left);
						}
						len += snprintf(buf + len, PAGE_SIZE, "\t}\n");
					}
				}
			}
		}
		drm_connector_list_iter_end(&conn_iter);

		len += snprintf(buf + len, PAGE_SIZE, "}\n");
	}
	spin_unlock_irqrestore(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	return len < PAGE_SIZE ? len : PAGE_SIZE - 1;
}

ssize_t __gbdc_infinity_prop_show(struct drm_device *dev, char *buf)
{
	int len = 0;

	unsigned int conn_id;
	struct GB02STR198 *infinity = NULL;
	int i;
	unsigned char *data;
	unsigned long int flags;

	spin_lock_irqsave(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	gbdc_for_each_infinity(infinity, dev) {
		struct GB02STR194 *info = infinity->infinity_infos;
		struct drm_connector *connector;

		struct drm_connector_list_iter conn_iter;

		if (!info)
			continue;

		if (len >= PAGE_SIZE - 96)
			break;

		conn_id = infinity->infinity_connector_id;

		len += snprintf(buf + len, PAGE_SIZE, "infinity(%d) info prop:\n", info->infinity_id);
		len += snprintf(buf + len, PAGE_SIZE, "{\n");

		drm_connector_list_iter_begin(dev, &conn_iter);
		drm_for_each_connector_iter(connector, &conn_iter) {
			if (connector->base.id == conn_id) {
				struct gbdc_connector *gbdc_connector;

				gbdc_connector = to_gbdc_connector(connector);

				if (!gbdc_connector->infinity_info_blob_ptr)
					continue;

				len += snprintf(buf + len, PAGE_SIZE, "\tconnector:%d\n", conn_id);
				len += snprintf(buf + len, PAGE_SIZE, "\t{\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t\tprop length: %ld\n", \
								gbdc_connector->infinity_info_blob_ptr->length);
				len += snprintf(buf + len, PAGE_SIZE, "\t\tprop value:\n");
				data = (unsigned char *)gbdc_connector->infinity_info_blob_ptr->data;
				if (!data)
					continue;

				for (i = 0; i < gbdc_connector->infinity_info_blob_ptr->length; i++) {
					if (len >= PAGE_SIZE - 96)
						break;
					if ((i % 64) == 0)
						len += snprintf(buf + len, PAGE_SIZE, "\n\t\t\t");
					len += snprintf(buf + len, PAGE_SIZE, "%02x", data[i]);
				}
				len += snprintf(buf + len, PAGE_SIZE, "\n");
				len += snprintf(buf + len, PAGE_SIZE, "\t}\n");
			}
		}
		drm_connector_list_iter_end(&conn_iter);

		len += snprintf(buf + len, PAGE_SIZE, "}\n");
	}
	spin_unlock_irqrestore(
				&GB02FUNC85((dev)->dev_private)->kms_info.infinity_lock, flags);
	return len < PAGE_SIZE ? len : PAGE_SIZE - 1;
}

ssize_t gbdc_infinity_state_show(struct drm_device *dev, char *buf)
{
	return __gbdc_infinity_state_show(dev, buf);
}

ssize_t gbdc_infinity_info_show(struct drm_device *dev, char *buf)
{
	return __gbdc_infinity_info_show(dev, buf);
}

ssize_t gbdc_infinity_prop_show(struct drm_device *dev, char *buf)
{
	return __gbdc_infinity_prop_show(dev, buf);
}
