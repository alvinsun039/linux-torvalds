/*
 * Copyright © 2012 Red Hat
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *      Dave Airlie <airlied@redhat.com>
 *      Rob Clark <rob.clark@linaro.org>
 *
 */

#include <linux/export.h>
#include <linux/dma-buf.h>
#include <linux/rbtree.h>

#include <drm/drm.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem.h>
#include <drm/drm_prime.h>
#include "gb_osl.h"
#include "common/gb_bo.h"
#include "common/gb_uk.h"
#include "vpu/vpu_mm/gb_vpu_ttm.h"
#include "vpu/vpu_mm/gb_vpu_gem.h"
/**
 * DOC: overview and lifetime rules
 *
 * Similar to GEM global names, PRIME file descriptors are also used to share
 * buffer objects across processes. They offer additional security: as file
 * descriptors must be explicitly sent over UNIX domain sockets to be shared
 * between applications, they can't be guessed like the globally unique GEM
 * names.
 *
 * Drivers that support the PRIME API implement the
 * &drm_driver.prime_handle_to_fd and &drm_driver.prime_fd_to_handle operations.
 * GEM based drivers must use GB02FUNC1022() and
 * GB02FUNC1084() to implement these. For GEM based drivers the
 * actual driver interfaces is provided through the &drm_gem_object_funcs.export
 * and &drm_driver.gem_prime_import hooks.
 *
 * &dma_buf_ops implementations for GEM drivers are all individually exported
 * for drivers which need to overwrite or reimplement some of them.
 *
 * Reference Counting for GEM Drivers
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * On the export the &dma_buf holds a reference to the exported buffer object,
 * usually a &drm_gem_object. It takes this reference in the PRIME_HANDLE_TO_FD
 * IOCTL, when it first calls &drm_gem_object_funcs.export
 * and stores the exporting GEM object in the &dma_buf.priv field. This
 * reference needs to be released when the final reference to the &dma_buf
 * itself is dropped and its &dma_buf_ops.release function is called.  For
 * GEM-based drivers, the &dma_buf should be exported using
 * gb_drm_gem_dmabuf_export() and then released by gb_drm_gem_dmabuf_release().
 *
 * Thus the chain of references always flows in one direction, avoiding loops:
 * importing GEM object -> dma-buf -> exported GEM bo. A further complication
 * are the lookup caches for import and export. These are required to guarantee
 * that any given object will always have only one uniqe userspace handle. This
 * is required to allow userspace to detect duplicated imports, since some GEM
 * drivers do fail command submissions if a given buffer object is listed more
 * than once. These import and export caches in &drm_prime_file_private only
 * retain a weak reference, which is cleaned up when the corresponding object is
 * released.
 *
 * Self-importing: If userspace is using PRIME as a replacement for flink then
 * it will get a fd->handle request for a GEM object that it created.  Drivers
 * should detect this situation and return back the underlying object from the
 * dma-buf private. For GEM based drivers this is handled in
 * GB02FUNC1071() already.
 */

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)

struct drm_prime_member {
	struct dma_buf *dma_buf;
	uint32_t handle;

	struct rb_node dmabuf_rb;
	struct rb_node handle_rb;
};

static int gb_drm_prime_add_buf_handle(struct drm_prime_file_private *prime_fpriv,
				    struct dma_buf *dma_buf, uint32_t handle)
{
	struct drm_prime_member *member;
	struct rb_node **p, *rb;

	member = kmalloc(sizeof(*member), GFP_KERNEL);
	if (!member)
		return -ENOMEM;

	get_dma_buf(dma_buf);
	member->dma_buf = dma_buf;
	member->handle = handle;

	rb = NULL;
	p = &prime_fpriv->dmabufs.rb_node;
	while (*p) {
		struct drm_prime_member *pos;

		rb = *p;
		pos = rb_entry(rb, struct drm_prime_member, dmabuf_rb);
		if (dma_buf > pos->dma_buf)
			p = &rb->rb_right;
		else
			p = &rb->rb_left;
	}
	rb_link_node(&member->dmabuf_rb, rb, p);
	rb_insert_color(&member->dmabuf_rb, &prime_fpriv->dmabufs);

	rb = NULL;
	p = &prime_fpriv->handles.rb_node;
	while (*p) {
		struct drm_prime_member *pos;

		rb = *p;
		pos = rb_entry(rb, struct drm_prime_member, handle_rb);
		if (handle > pos->handle)
			p = &rb->rb_right;
		else
			p = &rb->rb_left;
	}
	rb_link_node(&member->handle_rb, rb, p);
	rb_insert_color(&member->handle_rb, &prime_fpriv->handles);

	return 0;
}

static struct dma_buf *gb_drm_prime_lookup_buf_by_handle(struct drm_prime_file_private *prime_fpriv,
						      uint32_t handle)
{
	struct rb_node *rb;

	rb = prime_fpriv->handles.rb_node;
	while (rb) {
		struct drm_prime_member *member;

		member = rb_entry(rb, struct drm_prime_member, handle_rb);
		if (member->handle == handle)
			return member->dma_buf;
		else if (member->handle < handle)
			rb = rb->rb_right;
		else
			rb = rb->rb_left;
	}

	return NULL;
}

static int gb_drm_prime_lookup_buf_handle(struct drm_prime_file_private *prime_fpriv,
				       struct dma_buf *dma_buf,
				       uint32_t *handle)
{
	struct rb_node *rb;

	rb = prime_fpriv->dmabufs.rb_node;
	while (rb) {
		struct drm_prime_member *member;

		member = rb_entry(rb, struct drm_prime_member, dmabuf_rb);
		if (member->dma_buf == dma_buf) {
			*handle = member->handle;
			return 0;
		} else if (member->dma_buf < dma_buf) {
			rb = rb->rb_right;
		} else {
			rb = rb->rb_left;
		}
	}

	return -ENOENT;
}

/**
 * gb_drm_gem_dmabuf_export - &dma_buf export implementation for GEM
 * @dev: parent device for the exported dmabuf
 * @exp_info: the export information used by dma_buf_export()
 *
 * This wraps dma_buf_export() for use by generic GEM drivers that are using
 * gb_drm_gem_dmabuf_release(). In addition to calling dma_buf_export(), we take
 * a reference to the &drm_device and the exported &drm_gem_object (stored in
 * &dma_buf_export_info.priv) which is released by gb_drm_gem_dmabuf_release().
 *
 * Returns the new dmabuf.
 */
struct dma_buf *gb_drm_gem_dmabuf_export(struct drm_device *dev,
				      struct dma_buf_export_info *exp_info)
{
	struct dma_buf *dma_buf;

	dma_buf = dma_buf_export(exp_info);
	if (IS_ERR(dma_buf))
		return dma_buf;

	drm_dev_get(dev);
	drm_gem_object_get(exp_info->priv);

	return dma_buf;
}

/**
 * gb_drm_gem_dmabuf_release - &dma_buf release implementation for GEM
 * @dma_buf: buffer to be released
 *
 * Generic release function for dma_bufs exported as PRIME buffers. GEM drivers
 * must use this in their &dma_buf_ops structure as the release callback.
 * gb_drm_gem_dmabuf_release() should be used in conjunction with
 * gb_drm_gem_dmabuf_export().
 */
void gb_drm_gem_dmabuf_release(struct dma_buf *dma_buf)
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct drm_device *dev = obj->dev;

	/* drop the reference on the export fd holds */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(obj);
#else
	drm_gem_object_put_unlocked(obj);
#endif

	drm_dev_put(dev);
}


static struct dma_buf *GB02FUNC1019(struct drm_device *dev,
						  struct drm_gem_object *obj,
						  uint32_t flags)
{
	struct dma_buf *dmabuf;

	/* prevent races with concurrent gem_close. */
	if (obj->handle_count == 0) {
		dmabuf = ERR_PTR(-ENOENT);
		return dmabuf;
	}

	if (obj->funcs && obj->funcs->export)
		dmabuf = obj->funcs->export(obj, flags);
	else if (dev->driver->gem_prime_export)
		dmabuf = dev->driver->gem_prime_export(obj, flags);
	else
		dmabuf = GB02FUNC1060(obj, flags);
	if (IS_ERR(dmabuf)) {
		/* normally the created dma-buf takes ownership of the ref,
		 * but if that fails then drop the ref
		 */
		return dmabuf;
	}

	/*
	 * Note that callers do not need to clean up the export cache
	 * since the check for obj->handle_count guarantees that someone
	 * will clean it up.
	 */
	obj->dma_buf = dmabuf;
	get_dma_buf(obj->dma_buf);

	return dmabuf;
}

/**
 * GB02FUNC1022 - PRIME export function for GEM drivers
 * @dev: dev to export the buffer from
 * @file_priv: drm file-private structure
 * @handle: buffer handle to export
 * @flags: flags like DRM_CLOEXEC
 * @prime_fd: pointer to storage for the fd id of the create dma-buf
 *
 * This is the PRIME export function which must be used mandatorily by GEM
 * drivers to ensure correct lifetime management of the underlying GEM object.
 * The actual exporting from GEM object to a dma-buf is done through the
 * &drm_driver.gem_prime_export driver callback.
 */
int GB02FUNC1022(struct drm_device *dev,
			       struct drm_file *file_priv, uint32_t handle,
			       uint32_t flags,
			       int *prime_fd)
{
	struct drm_gem_object *obj;
	struct GB02STR59 *gb_bo;
	int ret = 0;
	struct dma_buf *dmabuf;

	mutex_lock(&file_priv->prime.lock);
	obj = drm_gem_object_lookup(file_priv, handle);
	if (!obj)  {
		ret = -ENOENT;
		goto out_unlock;
	}
	dmabuf = gb_drm_prime_lookup_buf_by_handle(&file_priv->prime, handle);
	if (dmabuf) {
		get_dma_buf(dmabuf);
		goto out_have_handle;
	}

	mutex_lock(&dev->object_name_lock);
	/* re-export the original imported object */
	if (obj->import_attach) {
		dmabuf = obj->import_attach->dmabuf;
		get_dma_buf(dmabuf);
		goto out_have_obj;
	}

	if (obj->dma_buf) {
		get_dma_buf(obj->dma_buf);
		dmabuf = obj->dma_buf;
		goto out_have_obj;
	}

	dmabuf = GB02FUNC1019(dev, obj, flags);
	if (IS_ERR(dmabuf)) {
		/* normally the created dma-buf takes ownership of the ref,
		 * but if that fails then drop the ref
		 */
		ret = PTR_ERR(dmabuf);
		mutex_unlock(&dev->object_name_lock);
		goto out;
	}

out_have_obj:
	/*
	 * If we've exported this buffer then cheat and add it to the import list
	 * so we get the correct handle back. We must do this under the
	 * protection of dev->object_name_lock to ensure that a racing gem close
	 * ioctl doesn't miss to remove this buffer handle from the cache.
	 */
	ret = gb_drm_prime_add_buf_handle(&file_priv->prime,
				       dmabuf, handle);
	mutex_unlock(&dev->object_name_lock);
	if (ret)
		goto fail_put_dmabuf;

out_have_handle:
	ret = dma_buf_fd(dmabuf, flags);
	/*
	 * We must _not_ remove the buffer from the handle cache since the newly
	 * created dma buf is already linked in the global obj->dma_buf pointer,
	 * and that is invariant as long as a userspace gem handle exists.
	 * Closing the handle will clean out the cache anyway, so we don't leak.
	 */
	if (ret < 0) {
		goto fail_put_dmabuf;
	} else {
		*prime_fd = ret;
		ret = 0;
	}
	goto out;

fail_put_dmabuf:
	dma_buf_put(dmabuf);
out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(obj);
#else
	drm_gem_object_put_unlocked(obj);
#endif
out_unlock:
	mutex_unlock(&file_priv->prime.lock);

	gb_bo = GB02FUNC212(obj);
	if (gb_bo->domain ==GB02MAC2678 && gb_bo->vbo_save_flag_old & GB02MAC932)
		GB02FUNC1721(gb_bo, file_priv, dev->dev_private, VBO_OUTPP_MID);

	return ret;
}

/**
 * DOC: PRIME Helpers
 *
 * Drivers can implement &drm_gem_object_funcs.export and
 * &drm_driver.gem_prime_import in terms of simpler APIs by using the helper
 * functions GB02FUNC1060() and GB02FUNC1071(). These functions
 * implement dma-buf support in terms of some lower-level helpers, which are
 * again exported for drivers to use individually:
 *
 * Exporting buffers
 * ~~~~~~~~~~~~~~~~~
 *
 * Optional pinning of buffers is handled at dma-buf attach and detach time in
 * GB02FUNC1042() and GB02FUNC1043(). Backing storage itself is
 * handled by GB02FUNC1044() and GB02FUNC1047(), which relies on
 * &drm_gem_object_funcs.get_sg_table.
 *
 * For kernel-internal access there's GB02FUNC1048() and
 * GB02FUNC1049(). Userspace mmap support is provided by
 * GB02FUNC1058().
 *
 * Note that these export helpers can only be used if the underlying backing
 * storage is fully coherent and either permanently pinned, or it is safe to pin
 * it indefinitely.
 *
 * FIXME: The underlying helper functions are named rather inconsistently.
 *
 * Exporting buffers
 * ~~~~~~~~~~~~~~~~~
 *
 * Importing dma-bufs using GB02FUNC1071() relies on
 * &drm_driver.gem_prime_import_sg_table.
 *
 * Note that similarly to the export helpers this permanently pins the
 * underlying backing storage. Which is ok for scanout, but is not the best
 * option for sharing lots of buffers for rendering.
 */

/**
 * GB02FUNC1042 - dma_buf attach implementation for GEM
 * @dma_buf: buffer to attach device to
 * @attach: buffer attachment data
 *
 * Calls &drm_gem_object_funcs.pin for device specific handling. This can be
 * used as the &dma_buf_ops.attach callback. Must be used together with
 * GB02FUNC1043().
 *
 * Returns 0 on success, negative error code on failure.
 */
int GB02FUNC1031(struct drm_gem_object *obj)
{
	if (obj->funcs && obj->funcs->pin)
		return obj->funcs->pin(obj);
	else if (obj->dev->driver->gem_prime_pin)
		return obj->dev->driver->gem_prime_pin(obj);
	else
		return 0;
}
void GB02FUNC1034(struct drm_gem_object *obj)
{
	if (obj->funcs && obj->funcs->unpin)
		obj->funcs->unpin(obj);
	else if (obj->dev->driver->gem_prime_unpin)
		obj->dev->driver->gem_prime_unpin(obj);
}
void *GB02FUNC1036(struct drm_gem_object *obj)
{
	void *vaddr;

	if (obj->funcs && obj->funcs->vmap)
		vaddr = obj->funcs->vmap(obj);
	else if (obj->dev->driver->gem_prime_vmap)
		vaddr = obj->dev->driver->gem_prime_vmap(obj);
	else
		vaddr = ERR_PTR(-EOPNOTSUPP);

	if (!vaddr)
		vaddr = ERR_PTR(-ENOMEM);

	return vaddr;
}


void GB02FUNC1040(struct drm_gem_object *obj, void *vaddr)
{
	if (!vaddr)
		return;

	if (obj->funcs && obj->funcs->vunmap)
		obj->funcs->vunmap(obj, vaddr);
	else if (obj->dev->driver->gem_prime_vunmap)
		obj->dev->driver->gem_prime_vunmap(obj, vaddr);
}

int GB02FUNC1042(struct dma_buf *dma_buf,
		       struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = dma_buf->priv;

	return GB02FUNC1031(obj);
}

/**
 * GB02FUNC1043 - dma_buf detach implementation for GEM
 * @dma_buf: buffer to detach from
 * @attach: attachment to be detached
 *
 * Calls &drm_gem_object_funcs.pin for device specific handling.  Cleans up
 * &dma_buf_attachment from GB02FUNC1042(). This can be used as the
 * &dma_buf_ops.detach callback.
 */
void GB02FUNC1043(struct dma_buf *dma_buf,
			struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = dma_buf->priv;

	GB02FUNC1034(obj);
}

/**
 * GB02FUNC1044 - map_dma_buf implementation for GEM
 * @attach: attachment whose scatterlist is to be returned
 * @dir: direction of DMA transfer
 *
 * Calls &drm_gem_object_funcs.get_sg_table and then maps the scatterlist. This
 * can be used as the &dma_buf_ops.map_dma_buf callback. Should be used together
 * with GB02FUNC1047().
 *
 * Returns:sg_table containing the scatterlist to be returned; returns ERR_PTR
 * on error. May return -EINTR if it is interrupted by a signal.
 */
struct sg_table *GB02FUNC1044(struct dma_buf_attachment *attach,
				     enum dma_data_direction dir)
{
	struct drm_gem_object *obj = attach->dmabuf->priv;
	struct sg_table *sgt;

	if (WARN_ON(dir == DMA_NONE))
		return ERR_PTR(-EINVAL);

	if (obj->funcs)
		sgt = obj->funcs->get_sg_table(obj);
	else
		sgt = obj->dev->driver->gem_prime_get_sg_table(obj);

	if (!dma_map_sg_attrs(attach->dev, sgt->sgl, sgt->nents, dir,
			      DMA_ATTR_SKIP_CPU_SYNC)) {
		sg_free_table(sgt);
		kfree(sgt);
		sgt = ERR_PTR(-ENOMEM);
	}

	return sgt;
}

/**
 * GB02FUNC1047 - unmap_dma_buf implementation for GEM
 * @attach: attachment to unmap buffer from
 * @sgt: scatterlist info of the buffer to unmap
 * @dir: direction of DMA transfer
 *
 * This can be used as the &dma_buf_ops.unmap_dma_buf callback.
 */
void GB02FUNC1047(struct dma_buf_attachment *attach,
			   struct sg_table *sgt,
			   enum dma_data_direction dir)
{
	if (!sgt)
		return;

	dma_unmap_sg_attrs(attach->dev, sgt->sgl, sgt->nents, dir,
			   DMA_ATTR_SKIP_CPU_SYNC);
	sg_free_table(sgt);
	kfree(sgt);
}

/**
 * GB02FUNC1048 - dma_buf vmap implementation for GEM
 * @dma_buf: buffer to be mapped
 *
 * Sets up a kernel virtual mapping. This can be used as the &dma_buf_ops.vmap
 * callback. Calls into &drm_gem_object_funcs.vmap for device specific handling.
 *
 * Returns the kernel virtual address or NULL on failure.
 */
void *GB02FUNC1048(struct dma_buf *dma_buf)
{
	struct drm_gem_object *obj = dma_buf->priv;
	void *vaddr;

	vaddr = GB02FUNC1036(obj);
	if (IS_ERR(vaddr))
		vaddr = NULL;

	return vaddr;
}

/**
 * GB02FUNC1049 - dma_buf vunmap implementation for GEM
 * @dma_buf: buffer to be unmapped
 * @vaddr: the virtual address of the buffer
 *
 * Releases a kernel virtual mapping. This can be used as the
 * &dma_buf_ops.vunmap callback. Calls into &drm_gem_object_funcs.vunmap for device specific handling.
 */
void GB02FUNC1049(struct dma_buf *dma_buf, void *vaddr)
{
	struct drm_gem_object *obj = dma_buf->priv;

	GB02FUNC1040(obj, vaddr);
}

/**
 * GB02FUNC1052 - PRIME mmap function for GEM drivers
 * @obj: GEM object
 * @vma: Virtual address range
 *
 * This function sets up a userspace mapping for PRIME exported buffers using
 * the same codepath that is used for regular GEM buffer mapping on the DRM fd.
 * The fake GEM offset is added to vma->vm_pgoff and &drm_driver->fops->mmap is
 * called to set up the mapping.
 *
 * Drivers can use this as their &drm_driver.gem_prime_mmap callback.
 */
int GB02FUNC1052(struct drm_gem_object *obj, struct vm_area_struct *vma)
{
	struct drm_file *priv;
	struct file *fil;
	int ret;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	fil = kzalloc(sizeof(*fil), GFP_KERNEL);
	if (!priv || !fil) {
		ret = -ENOMEM;
		goto out;
	}

	/* Used by drm_gem_mmap() to lookup the GEM object */
	priv->minor = obj->dev->primary;
	fil->private_data = priv;

	ret = drm_vma_node_allow(&obj->vma_node, priv);
	if (ret)
		goto out;

	vma->vm_pgoff += drm_vma_node_start(&obj->vma_node);

	ret = obj->dev->driver->fops->mmap(fil, vma);

	drm_vma_node_revoke(&obj->vma_node, priv);
out:
	kfree(priv);
	kfree(fil);

	return ret;
}

/**
 * GB02FUNC1058 - dma_buf mmap implementation for GEM
 * @dma_buf: buffer to be mapped
 * @vma: virtual address range
 *
 * Provides memory mapping for the buffer. This can be used as the
 * &dma_buf_ops.mmap callback. It just forwards to &drm_driver.gem_prime_mmap,
 * which should be set to GB02FUNC1052().
 *
 * FIXME: There's really no point to this wrapper, drivers which need anything
 * else but GB02FUNC1052 can roll their own &dma_buf_ops.mmap callback.
 *
 * Returns 0 on success or a negative error code on failure.
 */
int GB02FUNC1058(struct dma_buf *dma_buf, struct vm_area_struct *vma)
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct drm_device *dev = obj->dev;

	if (!dev->driver->gem_prime_mmap)
		return -ENOSYS;

	return dev->driver->gem_prime_mmap(obj, vma);
}

static const struct dma_buf_ops drm_gem_prime_dmabuf_ops =  {
	.cache_sgt_mapping = true,
	.attach = GB02FUNC1042,
	.detach = GB02FUNC1043,
	.map_dma_buf = GB02FUNC1044,
	.unmap_dma_buf = GB02FUNC1047,
	.release = gb_drm_gem_dmabuf_release,
	.mmap = GB02FUNC1058,
	.vmap = GB02FUNC1048,
	.vunmap = GB02FUNC1049,
};

/**
 * GB02FUNC1060 - helper library implementation of the export callback
 * @obj: GEM object to export
 * @flags: flags like DRM_CLOEXEC and DRM_RDWR
 *
 * This is the implementation of the &drm_gem_object_funcs.export functions for GEM drivers
 * using the PRIME helpers. It is used as the default in
 * GB02FUNC1022().
 */
struct dma_buf *GB02FUNC1060(struct drm_gem_object *obj,
				     int flags)
{
	struct drm_device *dev = obj->dev;
	struct dma_buf_export_info exp_info = {
		.exp_name = KBUILD_MODNAME, /* white lie for debug */
		.owner = dev->driver->fops->owner,
		.ops = &drm_gem_prime_dmabuf_ops,
		.size = obj->size,
		.flags = flags,
		.priv = obj,
		.resv = obj->resv,
	};

	return gb_drm_gem_dmabuf_export(dev, &exp_info);
}

/**
 * GB02FUNC1066 - core implementation of the import callback
 * @dev: drm_device to import into
 * @dma_buf: dma-buf object to import
 * @attach_dev: struct device to dma_buf attach
 *
 * This is the core of GB02FUNC1071(). It's designed to be called by
 * drivers who want to use a different device structure than &drm_device.dev for
 * attaching via dma_buf. This function calls
 * &drm_driver.gem_prime_import_sg_table internally.
 *
 * Drivers must arrange to call drm_prime_gem_destroy() from their
 * &drm_gem_object_funcs.free hook when using this function.
 */
struct drm_gem_object *GB02FUNC1066(struct drm_device *dev,
					    struct dma_buf *dma_buf,
					    struct device *attach_dev)
{
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct drm_gem_object *obj;
	int ret;

	if (dma_buf->ops == &drm_gem_prime_dmabuf_ops) {
		obj = dma_buf->priv;
		if (obj->dev == dev) {
			/*
			 * Importing dmabuf exported from out own gem increases
			 * refcount on gem itself instead of f_count of dmabuf.
			 */
			drm_gem_object_get(obj);
			return obj;
		}
	}

	if (!dev->driver->gem_prime_import_sg_table)
		return ERR_PTR(-EINVAL);

	attach = dma_buf_attach(dma_buf, attach_dev);
	if (IS_ERR(attach))
		return ERR_CAST(attach);

	get_dma_buf(dma_buf);

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		goto fail_detach;
	}

	obj = dev->driver->gem_prime_import_sg_table(dev, attach, sgt);
	if (IS_ERR(obj)) {
		ret = PTR_ERR(obj);
		goto fail_unmap;
	}

	obj->import_attach = attach;
	obj->resv = dma_buf->resv;

	return obj;

fail_unmap:
	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
fail_detach:
	dma_buf_detach(dma_buf, attach);
	dma_buf_put(dma_buf);

	return ERR_PTR(ret);
}

/**
 * GB02FUNC1071 - helper library implementation of the import callback
 * @dev: drm_device to import into
 * @dma_buf: dma-buf object to import
 *
 * This is the implementation of the gem_prime_import functions for GEM drivers
 * using the PRIME helpers. Drivers can use this as their
 * &drm_driver.gem_prime_import implementation. It is used as the default
 * implementation in GB02FUNC1084().
 *
 * Drivers must arrange to call drm_prime_gem_destroy() from their
 * &drm_gem_object_funcs.free hook when using this function.
 */
struct drm_gem_object *GB02FUNC1071(struct drm_device *dev,
					    struct dma_buf *dma_buf)
{
	return GB02FUNC1066(dev, dma_buf, dev->dev);
}

/**
 * GB02FUNC1084 - PRIME import function for GEM drivers
 * @dev: dev to export the buffer from
 * @file_priv: drm file-private structure
 * @prime_fd: fd id of the dma-buf which should be imported
 * @handle: pointer to storage for the handle of the imported buffer object
 *
 * This is the PRIME import function which must be used mandatorily by GEM
 * drivers to ensure correct lifetime management of the underlying GEM object.
 * The actual importing of GEM object from the dma-buf is done through the
 * &drm_driver.gem_prime_import driver callback.
 *
 * Returns 0 on success or a negative error code on failure.
 */
static void GB02FUNC1073(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;

	/* Remove any name for this object */
	if (obj->name) {
		idr_remove(&dev->object_name_idr, obj->name);
		obj->name = 0;
	}
}

static void GB02FUNC1077(struct drm_gem_object *obj)
{
	/* Unbreak the reference cycle if we have an exported dma_buf. */
	if (obj->dma_buf) {
		dma_buf_put(obj->dma_buf);
		obj->dma_buf = NULL;
	}
}
static void
drm_gem_object_handle_put_unlocked(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;
	bool final = false;

	if (WARN_ON(obj->handle_count == 0))
		return;

	/*
	* Must bump handle count first as this may be the last
	* ref, in which case the object would disappear before we
	* checked for a name
	*/

	mutex_lock(&dev->object_name_lock);
	if (--obj->handle_count == 0) {
		GB02FUNC1073(obj);
		GB02FUNC1077(obj);
		final = true;
	}
	mutex_unlock(&dev->object_name_lock);

	if (final)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(obj);
#else
	drm_gem_object_put_unlocked(obj);
#endif
}

int
gb_drm_gem_handle_create_tail(struct drm_file *file_priv,
			   struct drm_gem_object *obj,
			   u32 *handlep)
{
	struct drm_device *dev = obj->dev;
	u32 handle;
	int ret;

	WARN_ON(!mutex_is_locked(&dev->object_name_lock));
	if (obj->handle_count++ == 0)
		drm_gem_object_get(obj);

	/*
	 * Get the user-visible handle using idr.  Preload and perform
	 * allocation under our spinlock.
	 */
	idr_preload(GFP_KERNEL);
	spin_lock(&file_priv->table_lock);

	ret = idr_alloc(&file_priv->object_idr, obj, 1, 0, GFP_NOWAIT);

	spin_unlock(&file_priv->table_lock);
	idr_preload_end();

	mutex_unlock(&dev->object_name_lock);
	if (ret < 0)
		goto err_unref;

	handle = ret;

	ret = drm_vma_node_allow(&obj->vma_node, file_priv);
	if (ret)
		goto err_remove;

	if (obj->funcs && obj->funcs->open) {
		ret = obj->funcs->open(obj, file_priv);
		if (ret)
			goto err_revoke;
	} else if (dev->driver->gem_open_object) {
		ret = dev->driver->gem_open_object(obj, file_priv);
		if (ret)
			goto err_revoke;
	}

	*handlep = handle;
	return 0;

err_revoke:
	drm_vma_node_revoke(&obj->vma_node, file_priv);
err_remove:
	spin_lock(&file_priv->table_lock);
	idr_remove(&file_priv->object_idr, handle);
	spin_unlock(&file_priv->table_lock);
err_unref:
	drm_gem_object_handle_put_unlocked(obj);
	return ret;
}

int GB02FUNC1084(struct drm_device *dev,
			       struct drm_file *file_priv, int prime_fd,
			       uint32_t *handle)
{
	struct dma_buf *dma_buf;
	struct drm_gem_object *obj;
	int ret;

	dma_buf = dma_buf_get(prime_fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);
	mutex_lock(&file_priv->prime.lock);

	ret = gb_drm_prime_lookup_buf_handle(&file_priv->prime,
			dma_buf, handle);
	if (ret == 0) {
		goto out_put;
	}

	/* never seen this one, need to import */
	mutex_lock(&dev->object_name_lock);
	if (dev->driver->gem_prime_import)
		obj = dev->driver->gem_prime_import(dev, dma_buf);
	else
		obj = GB02FUNC1071(dev, dma_buf);
	if (IS_ERR(obj)) {
		ret = PTR_ERR(obj);
		goto out_unlock;
	}

	if (obj->dma_buf) {
		WARN_ON(obj->dma_buf != dma_buf);
	} else {
		obj->dma_buf = dma_buf;
		get_dma_buf(dma_buf);
	}

	/* _handle_create_tail unconditionally unlocks dev->object_name_lock. */
	ret = gb_drm_gem_handle_create_tail(file_priv, obj, handle);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put(obj);
#else
	drm_gem_object_put_unlocked(obj);
#endif
	if (ret)
		goto out_put;

	ret = gb_drm_prime_add_buf_handle(&file_priv->prime,
			dma_buf, *handle);
	mutex_unlock(&file_priv->prime.lock);
	if (ret)
		goto fail;

	dma_buf_put(dma_buf);

	return 0;

fail:
	/* hmm, if driver attached, we are relying on the free-object path
	 * to detach.. which seems ok..
	 */
	drm_gem_handle_delete(file_priv, *handle);
	dma_buf_put(dma_buf);
	return ret;

out_unlock:
	mutex_unlock(&dev->object_name_lock);
out_put:
	mutex_unlock(&file_priv->prime.lock);
	dma_buf_put(dma_buf);
	return ret;
}
#else
int GB02FUNC1022(struct drm_device *dev,
			       struct drm_file *file_priv, uint32_t handle,
			       uint32_t flags,
			       int *prime_fd)
{
	struct drm_gem_object *obj;
	struct GB02STR59 *gb_bo;

	mutex_lock(&file_priv->prime.lock);
	obj = drm_gem_object_lookup(file_priv, handle);
	gb_bo = GB02FUNC212(obj);

	if (gb_bo->domain ==GB02MAC2678 && gb_bo->vbo_save_flag_old & GB02MAC932)
		GB02FUNC1721(gb_bo, file_priv, dev->dev_private, VBO_OUTPP_MID);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(obj);
#else
		drm_gem_object_put_unlocked(obj);
#endif
	mutex_unlock(&file_priv->prime.lock);
	drm_gem_prime_handle_to_fd(dev, file_priv, handle, flags, prime_fd);

	return 0;
}
#endif
