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
#include <linux/pci.h>
#include <linux/pagemap.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#include <linux/atomic.h>
#include <drm/drm_drv.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_syncobj.h>
#include <drm/drm_utils.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
#include <drm/drm_pci.h>
#endif
#include <drm/drm_auth.h>
#include <drm/drm_file.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_resource.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
#include <drm/ttm/ttm_range_manager.h>
#endif
#include "common/gb_uk.h"
#include "gpu/gb_device.h"
#include "gpu/gb_ttm.h"
#include "common/gb_bo.h"
#include "gb_vpu_object.h"
#include "vpu/vpu_dec/vpu_vcmd.h"
#include "vpu/vpu_dec1/vpu_vcmd.h"
#include "vpu/vpu_enc/vpu_vc8000E_vcmd.h"
#include "gb_vpu_gem.h"
#include "common/gb_gpuinfo.h"

int GB02FUNC1701(void)
{
	u64 total_used = 0;
	struct GB02STR39 *gl_gbdev;
	struct GB02STR59 *gb_bo;
	struct GB02STR50 *gb_tbo;
	struct GB02STR56 *gbgpubo;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct ttm_bo_device *tdev;
	struct ttm_mem_type_manager *man;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	struct ttm_bo_device *tdev;
	struct ttm_bo_global *glob = &ttm_bo_glob;
	struct ttm_resource_manager *man;
#else
	struct ttm_device *tdev;
	struct ttm_resource_manager *man;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	struct ttm_resource *res;
	struct GB02STR177 *rman;
	const struct drm_mm_node *entry;
	struct ttm_range_mgr_node *node;
#else
	struct ttm_buffer_object *tbo;
#endif
	struct gpu_info *gpu_info = GB02FUNC314();
	struct GB02STR70 *pcie_dev = GB02FUNC518();
	

	gl_gbdev = pcie_dev->gbdev;
	if (!gl_gbdev) {
		gb_printf(KERN_ERR, "%s:gl_gbdev is NULL\n", __func__);
		return -EINVAL;
	}

	tdev = &gl_gbdev->gb_mm.ttm.bdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	man = &tdev->man[TTM_PL_VRAM];
#else
	man = tdev->man_drv[TTM_PL_VRAM];
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	spin_lock(&tdev->glob->lru_lock);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	spin_lock(&glob->lru_lock);
#else
	spin_lock(&tdev->lru_lock);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	list_for_each_entry(tbo, &man->lru[0], lru) {
		gb_tbo = ttm_ob_to_ttm_bo(tbo);
		gb_bo = GB02FUNC205(gb_tbo);
#else
	rman = to_range_manager(man);
	drm_mm_for_each_node(entry, &rman->mm) {
		node = container_of(entry, struct ttm_range_mgr_node, mm_nodes[0]);
		res = &node->base;
		gb_tbo = ttm_ob_to_ttm_bo(res->bo);
                gb_bo = GB02FUNC205(gb_tbo);

		if (!gb_bo) {
			gb_printf(KERN_INFO, "%s gb bo is null!\n", __func__);
			continue;
		}
		if (gb_bo->is_heap_growable)
			continue;
		else {
#endif
		gbgpubo = &gb_bo->gb_base;
		if (gb_bo->domain == GB02MAC2678 ||
			gbgpubo->initial_domain == GB02MAC2678)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
			total_used += tbo->num_pages;
#else
			total_used += (gb_tbo->bo.base.size / GB02MAC311);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
		}
#endif
	}


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	spin_unlock(&tdev->glob->lru_lock);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	spin_unlock(&glob->lru_lock);
#else
	spin_unlock(&tdev->lru_lock);
#endif
	gpu_info->vpu_used =
		(total_used * GB02MAC311) / (1024 * 1024);

	return 0;
}

//#define GB_TIME_DEBUG
int GB02FUNC1704(struct GB02STR39 *gbdev, unsigned long size,
	int alignment, int initial_domain, u32 flags, bool kernel, int ip_type,
	struct GB02STR3 *vcmd_buff)
{
	int r = -1;
	void *cpu_addr;
	struct drm_gem_object *gobj;
	struct GB02STR59 *bo;
	struct GB02STR56 *pgb_base;
	struct ttm_buffer_object *tbo;
	/*physical addr*/
	uint64_t gpu_addr;

	bo = GB02FUNC824(gbdev->ddev, size, initial_domain,
		flags, kernel);
	if (IS_ERR(bo))
		return r;

	pgb_base = &bo->gb_base;
	tbo = &pgb_base->ttm_bo.bo;

	r = GB02FUNC1762(tbo, false);
	if (r) {
		GB02FUNC1757(tbo);
		dev_err(gbdev->dev, "(%d) failed to reserve UVD bo\n", r);
		return r;
	}

	gpu_addr = GB02FUNC1484(&pgb_base->ttm_bo);

	r = GB02FUNC1758(pgb_base, &cpu_addr);
	if (r) {
		dev_err(gbdev->dev, "(%d) UVD map failed\n", r);
		return r;
	}
#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	gobj = &tbo->base;
#else
	gobj = &pgb_base->ttm_bo.base;
#endif
#ifdef GB02MAC426
	vcmd_buff->flp_offset = GB02FUNC1394(&bo->gb_base.ttm_bo);
#else
	vcmd_buff->flp_offset = drm_vma_node_offset_addr(&bo->gb_base.ttm_bo.bo.base.vma_node);
#endif
	vcmd_buff->obj = gobj;

	GB02FUNC1763(tbo);
	vcmd_buff->offset = gpu_addr;
	vcmd_buff->vir_buff = cpu_addr;
	gb_printf(KERN_INFO, "%s-%d:ttm finish ip_type = %d, offset addr = 0x%llx,\
		var addr = 0x%llx, flp offset = 0x%llx\n",
		__func__, __LINE__, ip_type, vcmd_buff->offset,
		(long long)vcmd_buff->vir_buff, vcmd_buff->flp_offset);
	return r;

}

void GB02FUNC1708(struct GB02STR47 *priv, bool flag)
{
	int i;
	struct GB02STR63 *vpu_time;
	struct gpu_info *gpu_info = GB02FUNC314();

	if (!gpu_info)
		return;

	vpu_time = gpu_info->vpu_time;

	for (i = 0; i < GB02MAC519; i++) {
		if (flag && vpu_time[i].filp == priv)
			return;
	}

	for (i = 0; i < GB02MAC519; i++) {
		if (flag && !vpu_time[i].filp) {
			vpu_time[i].filp = priv;
			return;
		}
		if (!flag && vpu_time[i].filp == priv) {
			vpu_time[i].filp = NULL;
			return;
		}
	}
}

#define GB02MAC2556 10
void GB02FUNC1711(struct GB02STR47 *priv,
	bool flag)
{
	int i;
	long curr_time;
	struct GB02STR63 *vpu_time;
	struct gpu_info *gpu_info = GB02FUNC314();
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1;
#else
	struct timespec64 t1;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif

	curr_time = t1.tv_sec * 1000000 + t1.tv_nsec / 1000;

	if (!gpu_info)
		return;

	vpu_time = gpu_info->vpu_time;
	for (i = 0; i < GB02MAC519; i++) {
		if (vpu_time[i].filp == priv)
			break;
	}

	if (flag && i < GB02MAC519) {
		vpu_time[i].frame_num++;
		if (vpu_time[i].frame_num <= 1)
			vpu_time[i].resv_time = 0;
		if (vpu_time[i].frame_num > GB02MAC2556) {
			vpu_time[i].resv_time = 0;
			vpu_time[i].frame_num = 1;
		}
		vpu_time[i].resv_start_time = curr_time;
		vpu_time[i].stop_flag = false;
	} else if (!flag && i < GB02MAC519) {
		if (vpu_time[i].frame_num <= 1) {
			vpu_time[i].start_time = curr_time;
		} else {
			vpu_time[i].resv_time += (curr_time -
				vpu_time[i].resv_start_time);
			vpu_time[i].resv_free_ave_time =
				vpu_time[i].resv_time /
				(vpu_time[i].frame_num - 1);
			vpu_time[i].vpu_ave_time =
				(curr_time - vpu_time[i].start_time) /
				(vpu_time[i].frame_num - 1);
		}
	}
}

int GB02FUNC1714(u32 *cmdbuf_id, u16 *cmdbuf_size, u64 bo_handles)
{
	int ret = 0;
	u32 *handles;

	handles = kvmalloc_array(VPU_SUBMIT_PARAMS_MAX, sizeof(u32), GFP_KERNEL);
	if (!handles) {
		ret = -ENOMEM;
		return ret;
	}

	if (copy_from_user(handles, (void __user *)(uintptr_t)bo_handles,
	 	VPU_SUBMIT_PARAMS_MAX * sizeof(u32))) {
		ret = -EFAULT;
		kvfree(handles);
		return ret;
	}

	*cmdbuf_id = (u32)handles[VPU_SUBMIT_CMD_ID];
	*cmdbuf_size = (u16)handles[VPU_SUBMIT_CMD_SIZE];

	if (handles)
		kvfree(handles);
	return ret;
}

int GB02FUNC1718(struct drm_file *filp, u64 bo_handles)
{
	int ret_val = 0;
	int core_id = 0;
	u32 cmdbuf_id;
	u16 cmdbuf_size;
	struct GB02STR47 *priv = filp->driver_priv;

	ret_val = GB02FUNC1714(&cmdbuf_id, &cmdbuf_size, bo_handles);
	if (ret_val)
		gb_printf(KERN_ERR,"%s:%d GB02FUNC1714 failed %d\n",
			__func__, __LINE__, ret_val);

	gb_printf(KERN_INFO, "%s:%d:--module type=%d--cmdbufid=%d--cmdbuf_size=%d\n",
			__func__, __LINE__, priv->cmdbuf_type, cmdbuf_id, cmdbuf_size);
	if (priv->cmdbuf_type == GB02MAC964)
		ret_val = GB02FUNC845(filp,
			cmdbuf_id, cmdbuf_size, &core_id);
	else if (priv->cmdbuf_type == GB02MAC968)
		ret_val = GB02FUNC1566(filp,
			cmdbuf_id, cmdbuf_size, &core_id);
	else if (priv->cmdbuf_type == GB02MAC966)
		ret_val = GB02FUNC1308(filp,
			cmdbuf_id, cmdbuf_size, &core_id);

	if (ret_val)
		gb_printf(KERN_ERR,"%s:%d GB02FUNC1718 failed %d\n",
			__func__, __LINE__, ret_val);
	return ret_val;
}

int GB02FUNC1719(struct drm_file *filp, u32 cmdbuf_id)
{
	int tmp = 0;
	u32 irq_status_ret = 0;
	struct GB02STR47 *priv = filp->driver_priv;

	if (priv->cmdbuf_type == GB02MAC964)
		tmp = GB02FUNC868(filp, cmdbuf_id, &irq_status_ret);
	else if (priv->cmdbuf_type == GB02MAC968)
		tmp = GB02FUNC1582(filp, cmdbuf_id, &irq_status_ret);
	else if (priv->cmdbuf_type == GB02MAC966)
		tmp = GB02FUNC1323(filp, cmdbuf_id, &irq_status_ret);

	gb_printf(KERN_INFO, "%s:%d. cmdbuf id = %d, cmdbuf_type = %d, ret = %d\n",
		  __func__, __LINE__, cmdbuf_id, priv->cmdbuf_type, tmp);
	if (tmp)
		gb_printf(KERN_ERR,"%s:%d GB02FUNC1719 failed %d\n",
			__func__, __LINE__, tmp);
	return tmp;
}

struct GB02STR2 * vpu_bo_private_handle(struct drm_file *file_priv,
	struct GB02STR39 *gbdev)
{
	struct GB02STR2 *vbo_priv_tmp = NULL;

	vbo_priv_tmp = (struct GB02STR2 *)vmalloc(sizeof(struct GB02STR2));
	if (vbo_priv_tmp == NULL) {
		gb_printf(KERN_ERR, "%s-%d: vbo_priv_tmp alloc fail\n", __func__, __LINE__);
		return NULL;
	}

	memset(vbo_priv_tmp, 0, sizeof(struct GB02STR2));
	vbo_priv_tmp->file_priv = file_priv;
	vbo_priv_tmp->vbo_priv = vbo_priv_tmp;
	list_add(&vbo_priv_tmp->vbo_list, &gbdev->vpu_bo_list_head);

	return vbo_priv_tmp;
}

void GB02FUNC1721(struct GB02STR59 *gb_bo,
	struct drm_file *file_priv, struct GB02STR39 *gbdev, int handle_index)
{
	int index;
	bool file_priv_flag = false;
	struct GB02STR2 *tmp = NULL;
	struct GB02STR2 *vbo_priv_tmp = NULL;

	if (list_empty(&gbdev->vpu_bo_list_head)) {
		vbo_priv_tmp = vpu_bo_private_handle(file_priv, gbdev);
		if (vbo_priv_tmp == NULL)
			return;
	}

	list_for_each_entry(tmp, &gbdev->vpu_bo_list_head, vbo_list) {
		if (tmp->file_priv == file_priv) {
			vbo_priv_tmp = tmp->vbo_priv;
			file_priv_flag = true;
			continue;
		}
	}

	if (!file_priv_flag) {
		vbo_priv_tmp = vpu_bo_private_handle(file_priv, gbdev);
		if (vbo_priv_tmp == NULL)
			return;
	}

	index = vbo_priv_tmp->vpu_part_bo_num[handle_index];
	vbo_priv_tmp->vpu_bo_priv_addr[handle_index][index] = (u64)gb_bo;
	vbo_priv_tmp->vpu_part_bo_num[handle_index]++;

	if (handle_index == VBO_OUTPP_MID && vbo_priv_tmp->vpu_part_bo_num[handle_index] == 5)
		vbo_priv_tmp->vpu_part_bo_num[handle_index] = 0;
	else if (handle_index == VBO_OUT && vbo_priv_tmp->vpu_part_bo_num[handle_index] == 10)
		vbo_priv_tmp->vpu_part_bo_num[handle_index] = 0;
	else if (handle_index == VBO_OUTPP && vbo_priv_tmp->vpu_part_bo_num[handle_index] == GB02MAC17)
		vbo_priv_tmp->vpu_part_bo_num[handle_index] = 0;
}

int GB02FUNC1726(struct drm_device *dev,
		void *data, struct drm_file *filp)
{
	int tmp = 0;
	struct GB02STR97 *args = (struct GB02STR97 *)data;
	struct GB02STR39 *gbdev = dev->dev_private;
	struct GB02STR47 *gb_priv = filp->driver_priv;

	if (args->ip_type == GB_IP_ENC) {
		tmp = GB02FUNC1618(filp, &gb_priv->enc_priv, args);
	} else if (args->ip_type == GB_IP_DEC) {
		if (!gb_priv->dec_id) {
			if (atomic_read(&gbdev->used_dec0_cnt) <=
				atomic_read(&gbdev->used_dec1_cnt)) {
				gb_priv->dec_id = GB02MAC964;
				atomic_inc(&gbdev->used_dec0_cnt);
			} else {
				gb_priv->dec_id = GB02MAC966;
				atomic_inc(&gbdev->used_dec1_cnt);
			}
		}

		if (gb_priv->dec_id == GB02MAC964) {
			tmp = GB02FUNC898(filp, &gb_priv->dec_priv, args);
		} else if (gb_priv->dec_id == GB02MAC966) {
			tmp = GB02FUNC1347(filp, &gb_priv->dec1_priv, args);
		} else {
			gb_printf(KERN_INFO,
				"%s:%d, gb file gb_priv dec_id is unknown = %d",
				__func__, __LINE__, gb_priv->dec_id);
			tmp = -EINVAL;
		}
	}
	GB02FUNC1708(gb_priv, true);

	return tmp;
}

int GB02FUNC1728(struct drm_device *dev,
		void *data, struct drm_file *filp)
{
	struct GB02STR47 *gb_priv = filp->driver_priv;
	unsigned int tmp = 0;

	GB02FUNC1708(gb_priv, false);
	if (tmp)
		goto open_fail;

open_fail:
	return tmp;
}

int GB02FUNC1729(struct drm_device *dev,
		void *data, struct drm_file *filp)
{
	unsigned int tmp = 0;
	struct GB02STR93 *arg = (struct GB02STR93 *)data;
	struct GB02STR47 *priv = filp->driver_priv;
	struct GB02STR39 *gbdev = priv->gbdev;

	if (arg->ip_type == GB_IP_ENC) {
		tmp = GB02FUNC1634(filp, arg->cmd, arg->priv);
	} else if (arg->ip_type == GB_IP_DEC) {
		if (priv->dec_id == 0) {
			if (atomic_read(&gbdev->used_dec0_cnt) <=
				atomic_read(&gbdev->used_dec1_cnt)) {
				priv->dec_id = GB02MAC964;
				atomic_inc(&gbdev->used_dec0_cnt);
			} else {
				priv->dec_id = GB02MAC966;
				atomic_inc(&gbdev->used_dec1_cnt);
			}
		}

		if (priv->dec_id == GB02MAC964)
			tmp = GB02FUNC549(filp, arg->cmd, arg->priv);
		else if (priv->dec_id == GB02MAC966)
			tmp = GB02FUNC1109(filp, arg->cmd, arg->priv);
		else
			gb_printf(KERN_INFO,
				  "%s:%d, gb file priv dec_id is unknown = %d",
				  __func__, __LINE__, priv->dec_id);
	}

	return tmp;
}

int GB02FUNC1731(struct drm_file *filp, void __user *bo_handles,
			   int count, struct drm_gem_object ***objs_out)
{
	int ret = 0;
#if (KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE)
	int i;
	u32 *handles;
	struct drm_gem_object **objs, *obj;

	if (!count)
		return 0;

	objs = kvmalloc_array(count, sizeof(struct drm_gem_object *),
			     GFP_KERNEL | __GFP_ZERO);
	if (!objs)
		return -ENOMEM;

	handles = kvmalloc_array(count, sizeof(u32), GFP_KERNEL);
	if (!handles) {
		ret = -ENOMEM;
		goto out;
	}

	if (copy_from_user(handles, bo_handles, count * sizeof(u32))) {
		ret = -EFAULT;
		DRM_DEBUG("Failed to copy in GEM handles\n");
		goto out;
	}

	spin_lock(&filp->table_lock);

	for (i = 0; i < count; i++) {
		/* Check if we currently have a reference on the object */
		obj = idr_find(&filp->object_idr, handles[i]);
		if (!obj) {
			ret = -ENOENT;
			break;
		}
		drm_gem_object_get(obj);
		objs[i] = obj;
	}
	spin_unlock(&filp->table_lock);
	*objs_out = objs;

out:
	if (handles)
		kvfree(handles);
#else
	ret = drm_gem_objects_lookup(filp,
		bo_handles, count, objs_out);
#endif

	return ret;
}

int GB02FUNC1733(struct drm_device *dev, void *data,
	struct drm_file *filp)
{
	int ret = 0;
	struct GB02STR47 *priv = filp->driver_priv;
	struct drm_gb_cmdbuf_resv *input_para =
		(struct drm_gb_cmdbuf_resv *)data;

	GB02FUNC1711(priv, true);

	gb_printf(KERN_INFO, "-%s:%d, module type = %d--\n",
		__func__, __LINE__, input_para->module_type);
	if (input_para->module_type == 0) {
		priv->cmdbuf_type = GB02MAC968;
	} else if (input_para->module_type == 2) {
		if (priv->dec_id == GB02MAC964)
			priv->cmdbuf_type = GB02MAC964;
		else if (priv->dec_id == GB02MAC966)
			priv->cmdbuf_type = GB02MAC966;
		else
			gb_printf(KERN_INFO, "%s:%d, gb_priv dec_id is unknown = %d",
				__func__, __LINE__, priv->dec_id);
	}

	if (priv->cmdbuf_type == GB02MAC964) {
		ret = GB02FUNC985(filp, input_para);
	} else if (priv->cmdbuf_type == GB02MAC968) {
		ret = GB02FUNC1632(filp, input_para);
	} else if (priv->cmdbuf_type == GB02MAC966) {
		ret = GB02FUNC1421(filp, input_para);
	}
	gb_printf(KERN_INFO, "%s:%d, cmdbuf resv--type= %d --cmdbuf_id= %d -----\n",
		__func__, __LINE__, priv->cmdbuf_type, input_para->cmdbuf_id);

	return ret;
}

int GB02FUNC1736(int cmdbuf_id, struct drm_file *filp)
{
	unsigned int tmp = 0;
	struct GB02STR47 *priv = filp->driver_priv;

	if (priv->cmdbuf_type == GB02MAC964)
		tmp = GB02FUNC980(filp, cmdbuf_id);
	else if (priv->cmdbuf_type == GB02MAC968)
		tmp = GB02FUNC1630(filp, cmdbuf_id);
	else if (priv->cmdbuf_type == GB02MAC966)
		tmp = GB02FUNC1417(filp, cmdbuf_id);

	GB02FUNC1711(priv, false);

	gb_printf(KERN_INFO, "%s:%d----cmdbuf type = %d --cmdbuf id = %d ---ret=%d\n",
		__func__, __LINE__, priv->cmdbuf_type, cmdbuf_id, tmp);

	return tmp;
}

int GB02FUNC1740(struct drm_device *dev, void *data,
	struct drm_file *filp)
{
	int index;
	int ret = 0;
	struct drm_gb_cmdbuf_resv *cmdbuf = (struct drm_gb_cmdbuf_resv *)data;
	struct GB02STR59 *bo;
	struct drm_gem_object **gem_obj;
	struct drm_gem_object **gem_obj_tmp;
	struct GB02STR39 *gbdev = dev->dev_private;

	gb_printf(KERN_INFO, "%s:%d--cmdbuf type = %d --cmdbuf_id = %d--\n",
		  __func__, __LINE__, cmdbuf->module_type, cmdbuf->cmdbuf_id);

	GB02FUNC1736(cmdbuf->cmdbuf_id, filp);
	if (!cmdbuf->handle_count)
		return ret;

	ret = GB02FUNC1731(filp,
		(void __user *)(uintptr_t)cmdbuf->handles,
		cmdbuf->handle_count, &gem_obj);
	if (ret) {
		gb_printf(KERN_ERR, "%s:%d-----lookup gem obj handle fail---\n",
				__func__, __LINE__);
		if (gem_obj)
			kvfree(gem_obj);
		return -ENOENT;
	}

	for (index = 0, gem_obj_tmp = gem_obj; index < cmdbuf->handle_count; index++, *gem_obj_tmp++) {
		bo = GB02FUNC212(*gem_obj_tmp);
		if (bo->vbo_save_flag_old & GB02MAC932) {
			GB02FUNC1721(bo, filp, gbdev, VBO_OUTPP);
		} else if (bo->vbo_save_flag_old & GB02MAC934) {
			GB02FUNC1721(bo, filp, gbdev, VBO_OUT);
		} else {
			gb_printf(KERN_ERR, "%s:%d-----GB02FUNC1721 lookup gem bo fail---\n",
				__func__, __LINE__);
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
		drm_gem_object_put_unlocked(*gem_obj_tmp);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(*gem_obj_tmp);
#else
		drm_gem_object_unreference_unlocked(*gem_obj_tmp);
#endif
	}
	if (gem_obj)
		kvfree(gem_obj);

	return ret;
}
void GB02FUNC1744(struct drm_device *dev, struct drm_file *file)
{
	struct GB02STR47 *gb_priv = file->driver_priv;
	struct GB02STR39 *gbdev = gb_priv->gbdev;
	struct GB02STR2 *tmp = NULL;
	struct GB02STR2 *vbo_priv_tmp = NULL;

	if (gb_priv->dec_priv)
		GB02FUNC934(file, gb_priv->dec_priv);

	if (gb_priv->dec1_priv)
		GB02FUNC1384(file, gb_priv->dec1_priv);

	if (gb_priv->dec_id == GB02MAC964)
		atomic_dec(&gbdev->used_dec0_cnt);
	else if (gb_priv->dec_id == GB02MAC966)
		atomic_dec(&gbdev->used_dec1_cnt);
	else
		gb_printf(KERN_INFO, "%s:%d, gb_priv dec_id is unknown = %d",
			  __func__, __LINE__, gb_priv->dec_id);

	if (gb_priv->enc_priv)
		GB02FUNC1623(file, gb_priv->enc_priv);

	if (gb_priv->dec_priv || gb_priv->dec1_priv || gb_priv->enc_priv) {
		GB02FUNC1708(gb_priv, false);
		list_for_each_entry_safe(vbo_priv_tmp, tmp, &gbdev->vpu_bo_list_head, vbo_list) {
			if (vbo_priv_tmp->file_priv == file) {
				list_del(&vbo_priv_tmp->vbo_list);
				vfree(vbo_priv_tmp);
			}
		}
	}
}
