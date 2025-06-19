#ifndef __GB_OSL_H__
#define __GB_OSL_H__
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0)
int GB02FUNC1084(struct drm_device *dev, struct drm_file *file_priv, int prime_fd, uint32_t *handle);
struct dma_buf *GB02FUNC1060(struct drm_gem_object *obj, int flags);
struct drm_gem_object *GB02FUNC1071(struct drm_device *dev, struct dma_buf *dma_buf);
#endif
int GB02FUNC1022(struct drm_device *dev, struct drm_file *file_priv, uint32_t handle, uint32_t flags, int *prime_fd);
#endif
