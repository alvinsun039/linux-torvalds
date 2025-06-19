#ifndef __GB_VPU_GEM_H__
#define __GB_VPU_GEM_H__
#include "common/gb_bo.h"
#include "vpu/vpu_comm/gb_vpu.h"

/* BO is expected to be accessed by the CPU */
#define GB02MAC2635		(1 << 0)
/* CPU access is not expected to work for this BO */
#define GB02MAC2636	(1 << 1)

/** vpu submit params bo_handles = {cmdbuf_id, cmdbuf_size,};
 ** ENC_CMDBUF_SIZE 0x7d8;
 ** DEC_CMDBUF_SIZE 0x888;
 **/
enum vpu_submit_params {
	VPU_SUBMIT_CMD_ID = 0,
	VPU_SUBMIT_CMD_SIZE,
	VPU_SUBMIT_PARAMS_MAX,
};

int GB02FUNC1718(struct drm_file *filp, u64 bo_handles);
int GB02FUNC1719(struct drm_file *filp, u32 cmdbuf_id);
void GB02FUNC1721(struct GB02STR59 *gb_bo,
	struct drm_file *file_priv, struct GB02STR39 *gbdev, int handle_index);
int GB02FUNC1736(int cmdbuf_id, struct drm_file *filp);

int GB02FUNC1733(struct drm_device *dev, void *data,
	struct drm_file *filp);
int GB02FUNC1726(struct drm_device *dev,
	void *data, struct drm_file *filp);
int GB02FUNC1728(struct drm_device *dev,
	void *data, struct drm_file *filp);
int GB02FUNC1729(struct drm_device *dev,
	void *data, struct drm_file *filp);
void GB02FUNC1744(struct drm_device *dev,
	struct drm_file *file);

int GB02FUNC1740(struct drm_device *dev, void *data,
struct drm_file *filp);
int GB02FUNC1704(struct GB02STR39 *gbdev,
	unsigned long size, int alignment, int initial_domain,
	u32 flags, bool kernel, int ip_type, struct GB02STR3 *vcmd_buff);
/*
int GB02FUNC1753(struct drm_device *dev, struct drm_file *filp,
		     void **dec_priv, void **dec1_priv, void **enc_priv);
*/
int GB02FUNC1701(void);

#endif

