#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "gb_uk.h"

#define u64 unsigned long long
#define u32 unsigned int

struct modeset_dev {
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t size;
	uint32_t handle;
	uint8_t *map;

	drmModeModeInfo mode;
	uint32_t fb;
	uint32_t conn;
	uint32_t crtc;
	drmModeCrtc *saved_crtc;
};


struct drm_vram_dev {
	uint32_t handle;
	uint32_t size;
	uint32_t map_offset;
	uint64_t gpu_va;
	uint64_t cpu_va;
};

/*
 * When the linux kernel detects a graphics-card on your machine, it loads the
 * correct device driver (located in kernel-tree at ./drivers/gpu/drm/<xy>) and
 * provides two character-devices to control it. Udev (or whatever hotplugging * application you use) will create them as:
 *     /dev/dri/card0
 *     /dev/dri/controlID64
 * We only need the first one. You can hard-code this path into your application
 * like we do here, but it is recommended to use libudev with real hotplugging
 * and multi-seat support. However, this is beyond the scope of this document.
 * Also note that if you have multiple graphics-cards, there may also be
 * /dev/dri/card1, /dev/dri/card2, ...
 *
 * We simply use /dev/dri/card0 here but the user can specify another path on
 * the command line.
 *
 * modeset_open(out, node): This small helper function opens the DRM device
 * which is given as @node. The new fd is stored in @out on success. On failure,
 * a negative error code is returned.
 * After opening the file, we also check for the DRM_CAP_DUMB_BUFFER capability.
 * If the driver supports this capability, we can create simple memory-mapped
 * buffers without any driver-dependent code. As we want to avoid any radeon,
 * nvidia, intel, etc. specific code, we depend on DUMB_BUFFERs here.
 */

static int modeset_open(int *out, const char *node)
{
	int fd, ret;
	uint64_t has_dumb;

	fd = open(node, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		ret = -errno;
		fprintf(stderr, "cannot open '%s': %m\n", node);
		return ret;
	}

	if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 ||
	    !has_dumb) {
		fprintf(stderr, "drm device '%s' does not support dumb buffers\n",
			node);
		close(fd);
		return -EOPNOTSUPP;
	}

	*out = fd;
	return 0;
}

/*
 * As a next step we need to find our available display devices. libdrm provides
 * a drmModeRes structure that contains all the needed information. We can
 * retrieve it via drmModeGetResources(fd) and free it via
 * drmModeFreeResources(res) again.
 *
 * A physical connector on your graphics card is called a "connector". You can
 * plug a monitor into it and control what is displayed. We are definitely
 * interested in what connectors are currently used, so we simply iterate
 * through the list of connectors and try to display a test-picture on each
 * available monitor.
 * However, this isn't as easy as it sounds. First, we need to check whether the
 * connector is actually used (a monitor is plugged in and turned on). Then we
 * need to find a CRTC that can control this connector. CRTCs are described
 * later on. After that we create a framebuffer object. If we have all this, we
 * can mmap() the framebuffer and draw a test-picture into it. Then we can tell
 * the DRM device to show the framebuffer on the given CRTC with the selected
 * connector.
 *
 * As we want to draw moving pictures on the framebuffer, we actually have to
 * remember all these settings. Therefore, we create one "struct modeset_dev"
 * object for each connector+crtc+framebuffer pair that we successfully
 * initialized and push it into the global device-list.
 *
 * Each field of this structure is described when it is first used. But as a
 * summary:
 * "struct modeset_dev" contains: {
 *  - @next: points to the next device in the single-linked list
 *
 *  - @width: width of our buffer object
 *  - @height: height of our buffer object
 *  - @stride: stride value of our buffer object
 *  - @size: size of the memory mapped buffer
 *  - @handle: a DRM handle to the buffer object that we can draw into
 *  - @map: pointer to the memory mapped buffer
 *
 *  - @mode: the display mode that we want to use
 *  - @fb: a framebuffer handle with our buffer object as scanout buffer
 *  - @conn: the connector ID that we want to use with this buffer
 *  - @crtc: the crtc ID that we want to use with this connector
 *  - @saved_crtc: the configuration of the crtc before we changed it. We use it
 *                 so we can restore the same mode when we exit.
 * }
 */

/*
 * modeset_create_fb(fd, dev): After we have found a crtc+connector+mode
 * combination, we need to actually create a suitable framebuffer that we can
 * use with it. There are actually two ways to do that:
 *   * We can create a so called "dumb buffer". This is a buffer that we can
 *     memory-map via mmap() and every driver supports this. We can use it for
 *     unaccelerated software rendering on the CPU.
 *   * We can use libgbm to create buffers available for hardware-acceleration.
 *     libgbm is an abstraction layer that creates these buffers for each
 *     available DRM driver. As there is no generic API for this, each driver
 *     provides its own way to create these buffers.
 *     We can then use such buffers to create OpenGL contexts with the mesa3D
 *     library.
 * We use the first solution here as it is much simpler and doesn't require any
 * external libraries. However, if you want to use hardware-acceleration via
 * OpenGL, it is actually pretty easy to create such buffers with libgbm and
 * libEGL. But this is beyond the scope of this document.
 *
 * So what we do is requesting a new dumb-buffer from the driver. We specify the
 * same size as the current mode that we selected for the connector.
 * Then we request the driver to prepare this buffer for memory mapping. After
 * that we perform the actual mmap() call. So we can now access the framebuffer
 * memory directly via the dev->map memory map.
 */

static int modeset_create_fb(int fd, struct modeset_dev *dev)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_destroy_dumb dreq;
	struct drm_mode_map_dumb mreq;
	int ret;

	/* create dumb buffer */
	memset(&creq, 0, sizeof(creq));
	creq.width = dev->width;
	creq.height = dev->height;
	creq.bpp = 32;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
	if (ret < 0) {
		fprintf(stderr, "cannot create dumb buffer (%d): %m\n",
			errno);
		return -errno;
	}
	dev->stride = creq.pitch;
	dev->size = creq.size;
	dev->handle = creq.handle;

	/* create framebuffer object for the dumb-buffer */
	ret = drmModeAddFB(fd, dev->width, dev->height, 24, 32, dev->stride,
			   dev->handle, &dev->fb);
	if (ret) {
		fprintf(stderr, "cannot create framebuffer (%d): %m\n",
			errno);
		ret = -errno;
		goto err_destroy;
	}

	/* prepare buffer for memory mapping */
	memset(&mreq, 0, sizeof(mreq));
	mreq.handle = dev->handle;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
	if (ret) {
		fprintf(stderr, "cannot map dumb buffer (%d): %m\n",
			errno);
		ret = -errno;
		goto err_fb;
	}

	/* perform actual memory mapping */
	dev->map = mmap(0, dev->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		        fd, mreq.offset);
	if (dev->map == MAP_FAILED) {
		fprintf(stderr, "cannot mmap dumb buffer (%d): %m\n",
			errno);
		ret = -errno;
		goto err_fb;
	}

	/* clear the framebuffer to 0 */
	memset(dev->map, 0, dev->size);

	return 0;

err_fb:
	drmModeRmFB(fd, dev->fb);
err_destroy:
	memset(&dreq, 0, sizeof(dreq));
	dreq.handle = dev->handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	return ret;
}

int drm_create_vram(int fd, struct drm_vram_dev *drm_vram)
{
	int ret = 0;

	/*1. create bo to get handle */
	struct drm_gb_create_bo bo_handle = {
		.size = drm_vram->size,
		.flags = 0,
		.domain = GPU_DOMAIN_FLAG,
	};

	ret = drmIoctl(fd, DRM_IOCTL_GB_CREATE_BO, &bo_handle);
	if (ret) {
		fprintf(stderr, "cannot create vram bo faild\n");
		return ret;
	}
	drm_vram->handle = bo_handle.handle;
	drm_vram->gpu_va = bo_handle.offset;

	/*2. get bo phy offset for handle */
	struct drm_gb_get_bo_offset bo_offset = {
		.handle = bo_handle.handle,
		.pad = 0,
		.offset = 0,
	};

	ret = drmIoctl(fd, DRM_IOCTL_GB_GET_BO_OFFSET, &bo_offset);
	if (ret) {
		fprintf(stderr, "cannot get bo offset faild \n");
		return ret;
	}
	drm_vram->handle = bo_handle.offset;

	/*3. map bo handle to get cpu map offset */
	struct drm_gb_mmap_bo bo_map = {
		.handle = bo_handle.handle,
		.flags = 0,
		.offset = 0,
	};

	ret = drmIoctl(fd, DRM_IOCTL_GB_MMAP_BO, &bo_map);
	if (ret) {
		fprintf(stderr, "cannot map bo offset faild \n");
		return ret;
	}
	drm_vram->map_offset = bo_map.offset;

	/*4. map bo to get process addr to read and write buff */
	drm_vram->cpu_va = (uint64_t)mmap(0, bo_handle.size, PROT_READ | PROT_WRITE, MAP_SHARED,
		        fd, bo_map.offset);
	if (!drm_vram->cpu_va) {
		fprintf(stderr, "cannot map bo faild \n");
		return ret;
	}

	return 0;

}

int main(int argc, char **argv)
{
	int ret, fd;
	const char *card;
	struct modeset_dev dev;
	struct drm_mode_destroy_dumb dreq;
	
	struct drm_vram_dev drm_vram;
	struct drm_gem_close gem_close;

	/* check which DRM device to open */
	if (argc > 1)
		card = argv[1];
	else
		card = "/dev/dri/card1";

	fprintf(stderr, "using card '%s'\n", card);

	/* open the DRM device */
	ret = modeset_open(&fd, card);
	if (ret)
		goto out_return;

	dev.width = 1920;
	dev.height = 1080;

	/* create fb */
	ret = modeset_create_fb(fd, &dev);
	if (ret) {
		fprintf(stderr, "cannot create framebuffer\n");
		return ret;
	}

	drm_vram.size = 1024;
	/* create drm vram */
	ret = drm_create_vram(fd, &drm_vram);
	if (ret) {
		fprintf(stderr, "cannot create drm vram \n");
		return ret;
	}
	
	memcpy((void *)drm_vram.cpu_va, "create drm vram success\n", drm_vram.size);
	fprintf(stderr, "buffer:%s", (char *)drm_vram.cpu_va);
	
	/* destory fb */
	munmap(dev.map, dev.size);
	dreq.handle = dev.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	memset(&dreq, 0, sizeof(dreq));

	/* destory drm_vram */
	munmap((void*)drm_vram.cpu_va, drm_vram.size);
	gem_close.handle = drm_vram.handle;
	drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
	memset(&drm_vram, 0, sizeof(drm_vram));

out_return:
	if (ret) {
		errno = -ret;
		fprintf(stderr, "modeset failed with error %d: %m\n", errno);
	} else {
		fprintf(stderr, "exiting\n");
	}
	return ret;
}
