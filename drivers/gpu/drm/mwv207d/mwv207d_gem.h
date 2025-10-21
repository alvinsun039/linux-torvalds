/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#ifndef MWV207D_GEM_H_TJTW9M4R
#define MWV207D_GEM_H_TJTW9M4R

#include <linux/version.h>
#include <linux/types.h>
#include <drm/drm_device.h>

#include "mwv207d_drv.h"

extern const struct drm_gem_object_funcs mwv207d_gem_object_funcs;

int mwv207d_gem_dumb_create(struct drm_file *file, struct drm_device *dev,
			    struct drm_mode_create_dumb *args);

void mwv207d_gem_prime_free(struct drm_gem_object *obj);

int mwv207d_gem_create_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *filp);

int mwv207d_gem_mmap_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *filp);

int mwv207d_gem_va_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *filp);

int mwv207d_gem_wait_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *filp);

struct drm_gem_object *
mwv207d_gem_prime_import_sg_table(struct drm_device *dev,
				  struct dma_buf_attachment *attac,
				  struct sg_table *sg);

int mwv207d_gem_metadata_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *filp);

#endif
