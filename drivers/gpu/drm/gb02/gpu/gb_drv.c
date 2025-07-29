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
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0))
#include <linux/timekeeping32.h>
#else
#include <linux/timekeeping.h>
#endif
#include <drm/drm_drv.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_syncobj.h>
#include <drm/drm_utils.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
#include <drm/drm_pci.h>
#endif
#include <drm/drm_auth.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/dma-resv.h>
#else
#include <linux/reservation.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
#include <drm/drm_aperture.h>
#endif
#include <drm/drm_file.h>
#if (defined CONFIG_CENTOS && !defined SYS_CENTOS7_COMPILE_ENV) \
        || LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#include <drm/drm_probe_helper.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif

#include "vpu/vpu_mm/gb_vpu_gem_comm.h"
#include "vpu/vpu_mm/gb_vpu_gem.h"
#include "gb_device.h"
#include "gb_regs.h"
#include "gb_gem.h"
#include "gb_mmu.h"
#include "gb_stage.h"
#include "gb_gpu.h"
#include "gb_pcie_info.h"
#include "gb_pages_alloc.h"
#include "kms/gbdc_mm.h"
#include "kms/gb_kms.h"
#include "kms/gbdc_crtc.h"
#include "gb_gpu_irq.h"
#include "gb_osl.h"
#ifdef	GB02MAC292
#include "audio/local.h"
#include "audio/gb02_fpga_v2vdma.h"
#include "audio/i2s_platform.h"
#include "audio/gb_snd_codec.h"
#include "audio/gb_audio_procfs.h"
#endif
#include "ip/gb_hdmac.h"
#include "ip/gb_dp.h"
#include "ip/gb_v2vdma.h"
#include "common/gb_common.h"
#include "common/gb_gpuinfo.h"
#include "common/xt.h"
#include "common/gb_irq.h"
#include "gb_ttm.h"
#include "gpu_test/gb_easy_shell.h"
#include "gpu_test/gb_mem_tool.h"
#include "kms/device/gbdc_ops.h"
#include "kms/device/gb_device_ctrl.h"
#include "vpu/vpu_dec/vpu_vcmd.h"
#include "vpu/vpu_dec1/vpu_vcmd.h"
#include "vpu/vpu_enc/vpu_vc8000E_vcmd.h"
#include "gb_firmware_upgrade.h"
#include "gb_debugfs.h"
#include "mcu_peripherals/gpu_freq/gpu_freq.h"
#include "common/gb02_pcie_resizebar.h"
#include "common/gb_kernel_ver.h"
#include "gbdc_infinity.h"

#define DRIVER_NAME "genbu"
#define DRIVER_DESC "GB DRM Driver"
#define DRIVER_DATE "2021-0901"
#define GB02MAC447 0
#define GB02MAC450 0
#define GB02MAC453 0

extern bool gb_rb_tree;
extern bool gb_time_debug;
extern int job_timeout_ms;
extern bool perf_cnt;
extern bool performance2_set;
extern int performance_ms;

unsigned long gBaseHdwr; /* PCI base register address (Hardware address) */
unsigned long gBaseDDRHw;        /* PCI base register address (memalloc) */
unsigned long gBaseDDRLen;
unsigned long gBaseSwitch;
unsigned long gBasePll;
u32  gBaseLen;                   /* Base register address Length */

u8 *hd_va;
u8 *ddr_va;
struct GB02STR39 *gl_gbdev;
extern int GB02FUNC1185(struct drm_device *drm, unsigned long flags);
#if !(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009 || \
	defined SYS_CENTOS7_COMPILE_ENV) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
extern int gbdc_unload(struct drm_device *dev);
#else
extern void gbdc_unload(struct drm_device *dev);
#endif

static int GB02FUNC198(struct drm_device *ddev, void *data, struct drm_file *file)
{
	struct GB02STR100 *param = data;
	struct GB02STR39 *gbdev = ddev->dev_private;

	if (param->pad != 0)
		return -EINVAL;

#define GB_FEATURE(name, member)			\
	case DRM_GB_PARAM_ ## name:		\
		param->value = gbdev->features.member;	\
		break
#define GB_FEATURE_ARRAY(name, member, max)			\
	case DRM_GB_PARAM_ ## name ## 0 ...			\
		DRM_GB_PARAM_ ## name ## max:			\
		param->value = gbdev->features.member[param->param -	\
			DRM_GB_PARAM_ ## name ## 0];		\
		break
	switch (param->param) {
		GB_FEATURE(GPU_PROD_ID, id);
		GB_FEATURE(GPU_REVISION, revision);

		GB_FEATURE(SHADER_PRESENT, shader_present);
		GB_FEATURE(TILER_PRESENT, tiler_present);
		GB_FEATURE(L2_PRESENT, l2_present);
		GB_FEATURE(STACK_PRESENT, stack_present);
		GB_FEATURE(AS_PRESENT, as_present);
		GB_FEATURE(SS_PRESENT, ss_present);
		GB_FEATURE(L2_FEATURES, l2_features);
		GB_FEATURE(TILER_FEATURES, tiler_features);
		GB_FEATURE(MEM_FEATURES, mem_features);
		GB_FEATURE(MMU_FEATURES, mmu_features);
		GB_FEATURE(THREAD_FEATURES, thread_features);
		GB_FEATURE(MAX_THREADS, max_threads);
		GB_FEATURE(THREAD_MAX_WORKGROUP_SZ,
				thread_max_workgroup_sz);
		GB_FEATURE(THREAD_MAX_BARRIER_SZ,
				thread_max_barrier_sz);
		GB_FEATURE(COHERENCY_FEATURES, coherency_features);
		GB_FEATURE_ARRAY(TEXTURE_FEATURES, texture_features, 3);
		GB_FEATURE_ARRAY(SS_FEATURES, ss_features, 15);
		GB_FEATURE(NR_CORE_GROUPS, nr_core_groups);
		GB_FEATURE(GROW_HEAP_MEMORY, heap_memory_growable);
		GB_FEATURE(VPU_BO_SAVE_FLAG, vpu_bo_save_flag);
		default:
			return -EINVAL;

	}

	return 0;
}

struct list_head gb_list_head = {0};

void GB02FUNC203(struct GB02STR47 *gb_priv)
{
	struct GB02STR39 *gbdev;
	struct drm_file *drm_file;
	struct GB02STR46 *task_list_node_tmp, *tmp;

	if (!gb_priv) {
		gb_printf(KERN_ERR, "%s input gb_priv is null \n", __func__);
		return;
	}

	gbdev = gb_priv->gbdev;
	if (!gbdev) {
		gb_printf(KERN_ERR, "%s input gbdev is null \n", __func__);
		return;
	}

	drm_file = gb_priv->drm_file;
	list_for_each_entry_safe(task_list_node_tmp, tmp, &gb_list_head, list) {
		if (task_list_node_tmp->drm_file == drm_file) {
			list_del_init(&task_list_node_tmp->list);
			break;
		}
	}
}

/*
 * mode: 0 - gpu_va  1 - cpu_va
 */
struct rb_root *GB02FUNC209(struct GB02STR47 *gb_priv, int mode)
{
	struct drm_file *drm_file;
	struct GB02STR46 *gb_list_node_tmp, *tmp;
	struct GB02STR39 *gbdev;

	if (!gb_priv) {
		gb_printf(KERN_INFO, "%s input gb_priv  == NULL\n", __func__);
		return NULL;
	}

	gbdev = gb_priv->gbdev;
	if (!gbdev) {
		gb_printf(KERN_INFO, "%s input gbdev  == NULL\n", __func__);
		return NULL;
	}

	drm_file = gb_priv->drm_file;
	list_for_each_entry_safe(gb_list_node_tmp, tmp, &gb_list_head, list) {
		if (gb_list_node_tmp->drm_file == drm_file)
			break;
	}

	if (mode)
		return &gb_list_node_tmp->cpu_root_node;
	else
		return &gb_list_node_tmp->gpu_root_node;

	gb_printf(KERN_ERR, "%s get mode:%d rb_root faild,drm_file:0x%llx\n",
			__func__, mode, (unsigned long long)drm_file);
	return NULL;
}



/*
*	GB02FUNC220 -- compare the double gpu_va addr
*	@gpu_va: compare gpu_va in rb_tree
*	@iovaddr: rb_node gpu_va init data
*	@size: rb_node gpu_va init size data
*	return GB02MAC1127 intput gpu_va is in the rb_node left,
*		   GB02MAC1128 intput gpu_va is in the rb_node right,
*		   GB02MAC1129 intput gpu_va is in the rb_node,
*	       0 is success
*/
int GB02FUNC220(__u64 gpu_va, __u64 iovaddr, __u32 size)
{
	if (iovaddr || gpu_va || size) {
		//gb_printf(KERN_INFO, "%s gpu_va:0x%llx iovaddr:0x%llx size:0x%x\n",
		//		__func__, (long long)gpu_va, (long long)iovaddr, size);
		if (gpu_va < iovaddr)
			/*domain left*/
			return GB02MAC1127;
		else if (gpu_va >= (iovaddr + size))
			/*domain right*/
			return GB02MAC1128;
		else if (gpu_va >= iovaddr && gpu_va < (iovaddr + size))
			/*in domain*/
			return GB02MAC1129;
	}
	/*error domain*/
	return -EINVAL;
}

/*
*	GB02FUNC222 -- compare the double cpu_va addr
*	@cpu_va: compare cpu_va in rb_tree
*	@va_start: mmap cpu addr start
*	@va_end: mmap cpu addr end
*	return GB02MAC1127 intput cpu_va is in the rb_node left,
*		   GB02MAC1128 intput gpu_va is in the rb_node right,
*		   GB02MAC1129 intput gpu_va is in the rb_node,
*	       0 is success
*/
int GB02FUNC222(__u64 cpu_va, __u64 va_start, __u64 va_end)
{
	if (cpu_va || va_start || va_end) {
		//gb_printf(KERN_INFO, "%s cpu_va:0x%llx va_start:0x%llx va_end:0x%llx\n",
		//	__func__, cpu_va, va_start, va_end);
		if (cpu_va < va_start)
			/*domain left*/
			return GB02MAC1127;
		else if (cpu_va >= va_end)
			/*domain right*/
			return GB02MAC1128;
		else if (cpu_va >= va_start && cpu_va < va_end)
			/*in domain*/
			return GB02MAC1129;
	}
	/*error domain*/
	return -EINVAL;
}

struct GB02STR60 *GB02FUNC226(void)
{
	struct GB02STR60 *base_rb_node;
	base_rb_node = kmalloc(sizeof(struct GB02STR60), GFP_KERNEL);
	if (!base_rb_node) {
		gb_printf(KERN_ERR, "%s kmalloc base_rb_node error \n", __func__);
		return NULL;
	}
	return base_rb_node;
}

/*
 * Clean cpu_va rb_tree before node insert
 *
 * Dirver would not know user application release vma space, rb
 * node can never be deleted on time. This may cause spatial
 * overlap among nodes in rb_tree.
 *
 * We need to reclaim all node that may overlap with the new node
 *
 * All we have to ensure than the same tree can only hit on one
 * rb_node per address.
 */
int GB02FUNC228(struct rb_root *rb_root, struct GB02STR60 *new)
{
	struct rb_node	*node;
	unsigned long vm_start, vm_end;

	if (!rb_root || !new) {
		gb_printf(KERN_ERR, "%s rb_root(%#llx) is null or node(%#llx) is null\n",
			__func__, (long long)rb_root, (long long)new);
		return -EINVAL;
	}

	node = rb_root->rb_node;
	vm_start = new->gb_gem_bo->vm_info.vm_start;
	vm_end = new->gb_gem_bo->vm_info.vm_end;

	while (node) {
		struct GB02STR60 *tmp = container_of(node,
				struct GB02STR60, gb_node);
		int result;
		result = GB02FUNC222(vm_start,
					tmp->gb_gem_bo->vm_info.vm_start,
					tmp->gb_gem_bo->vm_info.vm_end);

		if (result == GB02MAC1127)
			node = node->rb_left;
		else if (result == GB02MAC1128)
			node = node->rb_right;
		else if (result == GB02MAC1129) {
			/*found it, tmp need to release*/
			rb_erase(&tmp->gb_node, rb_root);
			kvfree(tmp);
			node = rb_root->rb_node;
		}
	}
	return 0;
}

int GB02FUNC233(struct rb_root *rb_root, struct GB02STR60 *data, int mode)
{
	struct rb_node **new = &(rb_root->rb_node), *parent = NULL;
	if (!rb_root || !data)
		return -EINVAL;

	while (*new) {
		struct GB02STR60 *this = container_of(*new, struct GB02STR60, gb_node);
		int result;
		if (mode)
			result = GB02FUNC222(data->gb_gem_bo->vm_info.vm_start,
					this->gb_gem_bo->vm_info.vm_start,
					this->gb_gem_bo->vm_info.vm_end);
		else
			result = GB02FUNC220(data->gb_gem_bo->vm_info.iovaddr,
					this->gb_gem_bo->vm_info.iovaddr,
					this->gb_gem_bo->vm_info.size);
		parent = *new;
		if (result == GB02MAC1127) {
			new = &((*new)->rb_left);
		} else if (result == GB02MAC1128) {
			new = &((*new)->rb_right);
		} else if (result == GB02MAC1129) {
			gb_printf(KERN_INFO, "%s 0x%lx is exist bypass to insert \n",
				__func__, data->gb_gem_bo->vm_info.iovaddr);
			return 0;
		} else {
			gb_printf(KERN_ERR, "%s 0x%lx is compare error,ret:%d\n",
				__func__, data->gb_gem_bo->vm_info.iovaddr, result);
			return result;
		}
	}
	rb_link_node(&data->gb_node, parent, new);
	rb_insert_color(&data->gb_node, rb_root);
	gb_printf(KERN_INFO, "%s insert 0x%lx\n", __func__,
		data->gb_gem_bo->vm_info.iovaddr);

	return 0;
}

struct GB02STR60 *GB02FUNC241(struct rb_root *rb_root, __u64 vaddr, int mode)
{
	struct rb_node	*node;

	if (!rb_root || !vaddr) {
		gb_printf(KERN_INFO, "%s rb_root is null rb_root:0x%llx vaddr:0x%llx mode:0x%d\n",
			__func__, (long long)rb_root, (long long)vaddr, mode);
		return NULL;
	}

	node = rb_root->rb_node;

	while (node) {
		struct GB02STR60 *data = container_of(node, struct GB02STR60, gb_node);
		int result;
		if (!data->gb_gem_bo)
			return NULL;

		if (mode)
			result = GB02FUNC222(vaddr,
				data->gb_gem_bo->vm_info.vm_start,
				data->gb_gem_bo->vm_info.vm_end);
		else
			result = GB02FUNC220(vaddr,
				data->gb_gem_bo->vm_info.iovaddr,
				data->gb_gem_bo->vm_info.size);

		if (result == GB02MAC1127)
			node = node->rb_left;
		else if (result == GB02MAC1128)
			node = node->rb_right;
		else if (result == GB02MAC1129)
			return data;
	}

	return NULL;
}

void GB02FUNC250(struct GB02STR59 *bo, struct GB02STR89 *args)
{
	int ret = 0;
	struct GB02STR60 *gb_rb_data;

	if (!gb_rb_tree)
		return;
	mutex_lock(&bo->gbdev->rb_mutex);
	/* init vm_info */
	gb_rb_data = GB02FUNC226();
	if (gb_rb_data) {
		bo->vm_info.iovaddr = bo->base.iovaddr;
		bo->vm_info.size = args->size;
		bo->vm_info.vm_start = 0;
		bo->vm_info.vm_end = 0;

		/* init gb_rb_data and insert it */
		gb_rb_data->gb_gem_bo = bo;
		ret = GB02FUNC233(GB02FUNC209(bo->private_data,
			GB02MAC1130), gb_rb_data, GB02MAC1130);
		if (ret)
			gb_printf(KERN_ERR, "%s insert erro %d\n", __func__, ret);
	} else {
		gb_printf(KERN_ERR, "%s init erro %pk\n", __func__, gb_rb_data);
	}
	gb_printf(KERN_INFO, "%s bo:0x%llx phy offset: 0x%llx",
		__func__, (unsigned long long)bo, args->offset_pa);
	if (ret)
		pr_err("%s bo rb create faild %d\n", __func__, ret);
	mutex_unlock(&bo->gbdev->rb_mutex);
}

int GB02FUNC259(struct GB02STR59 *bo, struct vm_area_struct *vma)
{
	int ret = 0;
	struct rb_root *gpu_rb_root;
	struct rb_root *cpu_rb_root;
	struct GB02STR60 *gb_bo_rb_node;
	struct GB02STR60 *gb_rb_cpu_node;
	struct GB02STR60 *data;

	if (bo->vm_info.iovaddr) {
		/* update vm_info */
		gpu_rb_root = GB02FUNC209(bo->private_data, GB02MAC1130);
		if (!gpu_rb_root) {
			gb_printf(KERN_ERR, "%s get gpu rb root error\n",
				__func__);
			return -EINVAL;
		}

		gb_bo_rb_node = GB02FUNC241(gpu_rb_root,
							bo->vm_info.iovaddr, GB02MAC1130);
		if (gb_bo_rb_node) {
			/*
			 * Since same bo only has one rb node in cpu addr tree
			 * It is necessary to check if rb node exists
			 * Next, check if rb node needs to be updated
			 */

			cpu_rb_root = GB02FUNC209(bo->private_data,
							GB02MAC1131);
			if (!cpu_rb_root) {
				gb_printf(KERN_ERR, "%s get cpu rb root error\n", __func__);
				return -EINVAL;
			}

			/*
			 * Yes I know this is ugly, but what can I do.
			 * User task direct call drmUnmap to unmap bo, gpu driver would
			 * never know.
			 * File_operation doesn't have munmap, I have to consider the
			 * situation rb_node is going to insert has same vm_start with
			 * rb_node from cpu_va rb_tree.
			 *
			 * It is necessary to check if rb_node vm_start conflict before
			 * node actually insert. If does check whether rb node vm_info
			 * needs to be updated. If does delete the node then insert a
			 * new one.
			 */
			data = GB02FUNC241(cpu_rb_root,
				gb_bo_rb_node->gb_gem_bo->vm_info.vm_start,
				GB02MAC1131);

			if (data) {
				if (gb_bo_rb_node->gb_gem_bo->vm_info.vm_start == vma->vm_start
					&& gb_bo_rb_node->gb_gem_bo->vm_info.vm_end == vma->vm_end
					&& gb_bo_rb_node->gb_gem_bo == bo) {
					/* node exist and works fine, DO NOTHING */
					return ret;
				} else {
					/* rb node need to update, delete it */
					gb_printf(KERN_INFO, "%s: vminfo need to update, rb_node:%#llx, bo:%#llx\n",
							__func__, (long long)data->gb_gem_bo, (long long)data->gb_gem_bo);
					rb_erase(&data->gb_node, cpu_rb_root);
					kvfree(data);
				}
			}

			gb_bo_rb_node->gb_gem_bo->vm_info.vm_start = vma->vm_start;
			gb_bo_rb_node->gb_gem_bo->vm_info.vm_end = vma->vm_end;

			gb_rb_cpu_node = GB02FUNC226();
			gb_rb_cpu_node->gb_gem_bo =  gb_bo_rb_node->gb_gem_bo;

			/* pre processing before insertion */
			ret = GB02FUNC228(cpu_rb_root, gb_rb_cpu_node);
			if (ret) {
				gb_printf(KERN_ERR, "in %s clean the multi tree faild %d\n", __func__, ret);
				return ret;
			}

			ret = GB02FUNC233(cpu_rb_root, gb_rb_cpu_node, GB02MAC1131);
			if (ret) {
				gb_printf(KERN_ERR, "in %s something goes really wrong, rb_node insert failed\n", __func__);
				kvfree(gb_rb_cpu_node);
				/* cpu_va do not has this bo rb node */
				return -EINVAL;
			}
		}
	}

	return ret;
}

void GB02FUNC271(struct rb_root *rb_root, __u64 vaddr, int mode)
{
	struct GB02STR60 *data = NULL;
	if (!rb_root || !vaddr)
		return;
	data = GB02FUNC241(rb_root, vaddr, mode);
	if (data) {
		gb_printf(KERN_INFO, "%s delete 0x%llx\n", __func__, (long long)vaddr);
		rb_erase(&data->gb_node, rb_root);
		kvfree(data);
	}
}

void GB02FUNC274(struct GB02STR59 *bo, struct GB02STR47 *gb_priv)
{
	struct rb_root *rb_root = NULL;
	if (!gb_rb_tree)
		return;
		/* delete bo_rb_node */
	if (!bo || !gb_priv)
		return;
	mutex_lock(&bo->gbdev->rb_mutex);
	rb_root = GB02FUNC209(gb_priv, GB02MAC1130);
	if (rb_root) {
		GB02FUNC271(rb_root, bo->base.iovaddr, GB02MAC1130);
	}

	rb_root = GB02FUNC209(gb_priv, GB02MAC1131);
	if (rb_root) {
		GB02FUNC271(rb_root, bo->vm_info.vm_start, GB02MAC1131);
	}
	mutex_unlock(&bo->gbdev->rb_mutex);

}

#ifdef GB_BO_RB_PRINTK
void GB02FUNC283(struct rb_root *rb_root)
{
	struct rb_node *node;
	struct GB02STR60 *gb_tmp;
	for (node = rb_first(rb_root); node; node = rb_next(node)) {
		gb_tmp = rb_entry(node, struct GB02STR60, gb_node);
		gb_printf(KERN_DEBUG, "iovaddr:0x%lx,",
			gb_tmp->gb_gem_bo->vm_info.iovaddr);
		gb_printf(KERN_DEBUG, "vm_start:0x%lx,",
			gb_tmp->gb_gem_bo->vm_info.vm_start);
		gb_printf(KERN_DEBUG, "vm_end:0x%lx\n",
			gb_tmp->gb_gem_bo->vm_info.vm_end);
	}
}
#endif

int GB02FUNC288(struct drm_device *dev, void *data,
		struct drm_file *file)
{
	struct GB02STR59 *bo;
	int domain = 0;
	struct GB02STR89 *args = data;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2;
#else
	struct timespec64 t1, t2;
#endif
	long time_use1;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif

	if (!args->size ||
	    (args->flags & ~GB02MAC960))
		return -EINVAL;

	if ((args->flags & GB02MAC925) &&
	    !(args->flags & GB02MAC924))
		return -EINVAL;

	if (args->domain & GB02MAC921)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
		domain = TTM_PL_FLAG_VRAM;
#else
		domain = TTM_PL_VRAM;
#endif
	else if (args->domain & GB02MAC923)
		domain = GB02MAC2678;

	if (!!(args->flags & GB02MAC926))
		domain = GB02MAC2678;

	bo = GB02FUNC800(file, dev, args->size, domain,
			args->flags, false, &args->handle);

	if (IS_ERR(bo))
		return PTR_ERR(bo);

	args->offset_va = bo->base.iovaddr;
	args->offset_pa = bo->base.iopaddr;

	if (gb_rb_tree)
		GB02FUNC250(bo, args);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	if (gb_time_debug) {
		pr_info("[%s,pid:%d] %s:cost time: %ld\n",
			current->comm,current->pid,
			 __func__, time_use1);
	}

	return 0;
}
#ifdef GB_ALLSCREEN
int GB02FUNC297(struct drm_device *dev, void *data,
		struct drm_file *file)
{
	int ret = 0;
	int crtc_id;
	struct GB02STR90 *args = data;
	struct GB02STR89 *bo = &args->bo;
	struct GB02STR247 *fb_base_info = NULL;
	struct GB02STR59 *gb_bo = NULL;
	struct drm_gem_object *obj = NULL;
	int fb_type = args->fb_type;

	fb_base_info = GB02FUNC161();
	crtc_id = GB02FUNC152();
	gb_printf(KERN_INFO, "[%s]crtc_id is %d, size %x", __func__, crtc_id, bo->size);
	ret = GB02FUNC288(dev, (void *)bo, file);
	if (ret) {
		gb_printf(KERN_ERR, "[%s] create new fb bo failed", __func__);
		return ret;
	}

	obj = idr_find(&file->object_idr, bo->handle);
    if (!obj) {
        ret = -ENOENT;
        return ret;
    }

	gb_bo = GB02FUNC212(obj);

	if (gb_bo->is_fb_bo) {
		atomic_set(&fb_base_info->info[fb_type].flag, 1);
		fb_base_info->info[fb_type].fb_addr = gb_bo->base.pages[0];
		fb_base_info->info[fb_type].size = bo->size;
		fb_base_info->info[fb_type].height = fb_base_info->crtc_plane[crtc_id].height;
		fb_base_info->info[fb_type].width = fb_base_info->crtc_plane[crtc_id].width;
	}
	return ret;
}
#endif

static int GB02FUNC303(struct drm_device *dev, struct drm_file *file_priv,
			 struct GB02STR87 *args, struct GB02STR162 *stage)
{

#if (KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE)
	int ret = 0;
	u32 *handles;
	struct drm_gem_object **objs, *obj;
	int i;
#endif
	if (!args->bo_handle_count)
		return 0;

	stage->bo_count = args->bo_handle_count;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	stage->implicit_fences = kvmalloc_array(stage->bo_count,
				  sizeof(struct dma_fence *),
				  GFP_KERNEL | __GFP_ZERO);
	if (!stage->implicit_fences)
		return -ENOMEM;
#endif

#if (KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE)
	objs = kvmalloc_array(stage->bo_count, sizeof(struct drm_gem_object *),
		GFP_KERNEL | __GFP_ZERO);
	if (!objs)
		return -ENOMEM;

	handles = kvmalloc_array(stage->bo_count, sizeof(u32), GFP_KERNEL);
	if (!handles) {
		ret = -ENOMEM;
		kvfree(objs);
		return ret;
	}

	if (copy_from_user(handles, (void __user *)(uintptr_t)args->bo_handles, stage->bo_count * sizeof(u32))) {
		ret = -EFAULT;
		DRM_DEBUG("Failed to copy in GEM handles\n");
		kvfree(objs);
		kvfree(handles);
		return ret;
	}

	spin_lock(&file_priv->table_lock);

	for (i = 0; i < stage->bo_count; i++) {
		/* Check if we currently have a reference on the object */
		obj = idr_find(&file_priv->object_idr, handles[i]);
		if (!obj) {
			gb_printf(KERN_ERR,"[%s] cant find handle %d\n", __func__, handles[i]);
			ret = -ENOENT;
			break;
		}
		drm_gem_object_get(obj);
		objs[i] = obj;
	}
	spin_unlock(&file_priv->table_lock);
	stage->bos = objs;

	if (handles) {
		kvfree(handles);
	}

	return ret;
#else
	return drm_gem_objects_lookup(file_priv,
				      (void __user *)(uintptr_t)args->bo_handles,
				      stage->bo_count, &stage->bos);
#endif
}

static int GB02FUNC315(struct drm_device *dev, struct drm_file *file_priv,
		           struct GB02STR87 *args, struct GB02STR162 *stage)
{
	u32 *handles = NULL;
	int ret = 0;
	int i;

	stage->in_fence_count = args->in_sync_count;
	if (!stage->in_fence_count)
		return 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	stage->in_fences = kvmalloc_array(stage->in_fence_count,
					sizeof(struct dma_fence *),
					GFP_KERNEL | __GFP_ZERO);
	if (!stage->in_fences) {
		DRM_DEBUG("Failed to allocate stage in fences\n");
		return -ENOMEM;
	}
#endif

	handles = kvmalloc_array(stage->in_fence_count, sizeof(u32), GFP_KERNEL);
	if (!handles) {
		DRM_DEBUG("Failed to allocate incoming syncobj handles\n");
		kvfree(stage->in_fences);
		return -ENOMEM;
	}

	if (copy_from_user(handles,
			   (void __user *)(uintptr_t)args->in_syncs,
			   stage->in_fence_count * sizeof(u32))) {
		ret = -EFAULT;
		DRM_DEBUG("Failed to copy in syncobj handles\n");
		goto fail_res;
	}

	for (i = 0; i < stage->in_fence_count; i++) {

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#if KERNEL_VERSION (5,0,0) < LINUX_VERSION_CODE
		ret = drm_syncobj_find_fence(file_priv, handles[i],
		 0, 0, &stage->in_fences[i]);
#else
		ret = drm_syncobj_find_fence(file_priv, handles[i],
		 			&stage->in_fences[i]);
#endif
		if (ret) {
			gb_printf(KERN_ERR, "%s %d find usr handles[%d]:%d err,ret:%d\n",
				__func__, __LINE__, i, handles[i], ret);
			goto fail_fence;
		}
#endif

#if KERNEL_VERSION (6, 6, 0) < LINUX_VERSION_CODE
		ret = drm_sched_job_add_syncobj_dependency(&stage->base, file_priv,
							   handles[i], 0);
		if (ret) {
			gb_printf(KERN_ERR, "%s %d find usr handles[%d]:%d err,ret:%d\n",
				__func__, __LINE__, i, handles[i], ret);
			goto fail_fence;
		}
#endif
	}

	kvfree(handles);
	return 0;

fail_fence:
	while (--i >= 0)
		dma_fence_put(stage->in_fences[i]);
fail_res:
	kvfree(stage->in_fences);
	kvfree(handles);
	return ret;
}

static int GB02FUNC329(struct drm_device *dev, void *data,
		struct drm_file *file)
{
	struct GB02STR39 *gbdev = dev->dev_private;
	struct GB02STR87 *args = (struct GB02STR87 *)data;
	struct drm_syncobj *sync_out = NULL;
	struct GB02STR162 *stage;
	int ret = 0, slot;
	int i;
	char time_buf[GB02MAC1960];

	gb_printf(KERN_DEBUG, "%s:%d----handle =0x%llx--\n",
		__func__, __LINE__, (long long)args->bo_handles);

	if (!args->sc && !args->bo_handle_count) {
		ret = GB02FUNC1718(file, args->bo_handles);
		return ret;
	}

	if (!args->sc)
		return -EINVAL;

	if (args->slot_req && args->slot_req != GB02MAC920)
		return -EINVAL;

	if (args->out_sync > 0) {
		sync_out = drm_syncobj_find(file, args->out_sync);
		if (!sync_out)
			return -ENODEV;
	}

	stage = kvzalloc(sizeof(*stage), GFP_KERNEL);
	if (!stage) {
		ret = -ENOMEM;
		goto fail_out_sync;
	}

	kref_init(&stage->refcount);

	if (gb_time_debug) {
		GB02FUNC1230(time_buf);
		pr_info("[%s,pid:%d] %s %s --stage:%llx\n",
			current->comm,current->pid,
			__func__, time_buf, (long long)stage);
	}

	stage->gbdev = gbdev;
	stage->sc = args->sc;
	stage->slot_req = args->slot_req;
	stage->reset_time = 0;
	stage->flush_id = GB02FUNC869(gbdev);
	stage->file_priv = file->driver_priv;
	stage->user_pid = current->pid;

	slot = GB02FUNC1248(stage);
	if (gb_time_debug) {
		GB02FUNC1230(time_buf);
		pr_info("[%s,pid:%d] %s %s --stage:%llx slot:%d\n",
			current->comm,current->pid,
			__func__, time_buf, (long long)stage, slot);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	ret = drm_sched_job_init(&stage->base, &stage->file_priv->sched_entity[slot], NULL);
	if (ret) {
		gb_printf(KERN_ERR, "%s: drm_sched_job_init ret %d\n",
				__func__, ret);
		goto out_put_job;
	}
#endif

	ret = GB02FUNC315(dev, file, args, stage);
	if (ret) {
		gb_printf(KERN_ERR, "%s: GB02FUNC315 failed %d\n", __func__, ret);
		goto fail_out_sync;
	}

	ret = GB02FUNC303(dev, file, args, stage);
	if (ret) {
		gb_printf(KERN_ERR, "%s: GB02FUNC303 failed %d\n", __func__, ret);
		goto fail_in_fence;
	}

	ret = GB02FUNC1279(stage);
	if (ret) {
		gb_printf(KERN_ERR, "%s: GB02FUNC1279 failed %d\n", __func__, ret);
		goto fail_in_fence;
	}
	gb_printf(KERN_DEBUG, "%s: submit stage %pK\n", __func__, stage);

	if (sync_out) {
		drm_syncobj_replace_fence(sync_out, stage->render_done_fence);
		drm_syncobj_put(sync_out);
	}

	if (kref_read(&stage->refcount) >= 1)
		GB02FUNC1294(stage);

	return ret;

fail_in_fence:
	if (stage->bos) {
		for (i = 0; i < stage->bo_count; i++)
			if (stage->bos[i]) 
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
				drm_gem_object_put_unlocked(stage->bos[i]);
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
				drm_gem_object_put(stage->bos[i]);
		#else
				drm_gem_object_unreference_unlocked(stage->bos[i]);
		#endif
		kvfree(stage->bos);
	}

	if (stage->implicit_fences)
		kvfree(stage->implicit_fences);

	if (stage->in_fences) {
		for (i = 0; i < stage->in_fence_count; i++)
			dma_fence_put(stage->in_fences[i]);
		kvfree(stage->in_fences);
	}

fail_out_sync:
	if (sync_out)
		drm_syncobj_put(sync_out);

out_put_job:
	if (stage) {
		if (kref_read(&stage->refcount) > 1)
			GB02FUNC1294(stage);
		kfree(stage);
	}

	return ret;
}

static int GB02FUNC353(struct drm_device *dev, void *data,
		            struct drm_file *file_priv)
{
	long ret = 0;
	struct GB02STR88 *args = data;
	struct drm_gem_object *gem_obj;
	struct GB02STR59 *gb_bo;
	unsigned long timeout = nsecs_to_jiffies(args->timeout_ns);
	struct GB02STR39 *gbdev = dev->dev_private;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2;
#else
	struct timespec64 t1, t2;
#endif
	long time_use1;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif

	if (!args->handle && args->pad) {
		ret = GB02FUNC1719(file_priv, args->pad);
		return ret;
	}

	if (args->pad)
		return -EINVAL;
	gem_obj = drm_gem_object_lookup(file_priv, args->handle);
	if (!gem_obj)
		return -ENOENT;
	gb_bo = GB02FUNC212(gem_obj);
	gb_printf(KERN_DEBUG, "%s:%d--ip domain =%x--\n",
		__func__, __LINE__, gb_bo->domain);

	if (atomic_read(&gbdev->reset.pending))
		return -EBUSY;

#if KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE
	gb_printf(KERN_DEBUG, "%s: %pK wait bo %pK timeout %ld\n", __func__,
			current, gpu_mmbo_to_gb_bo(GB02FUNC218(gem_obj)), timeout);
#else
#endif

#if (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)
	/* ret = reservation_object_wait_timeout_rcu(gb_bo->resv, true, true, timeout);
	 * // for UOS 1050: 4.19 kernel */
	ret = reservation_object_wait_timeout_rcu(gb_bo->gb_base.ttm_bo.bo.resv, true, true, timeout);
#elif (KERNEL_VERSION(5, 14, 0) > LINUX_VERSION_CODE) && (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE)
	ret = dma_resv_wait_timeout_rcu(gem_obj->resv, true, true, timeout);
#else
#if (KERNEL_VERSION(6, 6, 0) <= LINUX_VERSION_CODE)
	ret = dma_resv_wait_timeout(gem_obj->resv, DMA_RESV_USAGE_READ, true, timeout);
#else
	ret = dma_resv_wait_timeout(gem_obj->resv, true, true, timeout);
#endif
#endif
	/* ret == 0 means not signaled,
	 * ret > 0 means signaled
	 * ret < 0 means interrupted before timeout
	 * */
	if (!ret)
		ret = timeout ? -ETIMEDOUT : -EBUSY;

	gb_printf(KERN_DEBUG, "%s: %pK wait bo %pK ok timeout %ld ret %ld\n", __func__, current,
				gpu_mmbo_to_gb_bo(GB02FUNC218(gem_obj)), timeout, ret);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
		drm_gem_object_put(gem_obj);
#else
		drm_gem_object_put_unlocked(gem_obj);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec) / 1000;
	if (gb_time_debug) {
		pr_info("[%s,pid:%d] %s:cost time: %ld\n",
			current->comm,current->pid,
			__func__, time_use1);
	}

	return ret;
}

static int GB02FUNC368(struct drm_device *dev, void *data,
		      struct drm_file *file_priv)
{
	struct GB02STR95 *args = data;
	struct drm_gem_object *gem_obj;
	struct GB02STR59 *gb_bo;
	int ret = 0;

	if (args->flags != 0) {
		DRM_INFO("unknown mmap_bo flags: %d\n", args->flags);
		gb_printf(KERN_ERR, "%s unknown mmap_bo flags: %d\n",
			__func__, args->flags);
		return -EINVAL;
	}

	gem_obj = drm_gem_object_lookup(file_priv, args->handle);
	if (!gem_obj) {
		DRM_DEBUG("Failed to look up GEM BO %d\n", args->handle);
		gb_printf(KERN_ERR, "%s Failed to look up GEM BO %d\n",
			__func__, args->handle);
		return -ENOENT;
	}

#ifdef GB02MAC286
	/* Don't allow mmapping of heap objects, as heap pages aren't alloced yet */
	if (GB02FUNC212(gem_obj)->is_heap_growable)
		return -EINVAL;
#endif

	gb_bo = GB02FUNC212(gem_obj);

#ifdef GB02MAC426
	args->offset = GB02FUNC1394(&gb_bo->gb_base.ttm_bo);
#else
	args->offset = drm_vma_node_offset_addr(&gb_bo->gb_base.ttm_bo.bo.base.vma_node);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put_unlocked(gem_obj);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	drm_gem_object_put(gem_obj);
#else
	drm_gem_object_unreference_unlocked(gem_obj);
#endif

	return ret;
}

/*Resolve encoding import display data, and return bo vpaddr*/
#define GB02MAC533 1

static int GB02FUNC379(struct drm_device *dev, void *data,
			    struct drm_file *file_priv)
{
	struct GB02STR101 *args = data;
	struct drm_gem_object *gem_obj;
	struct GB02STR59 *bo;
	gem_obj = drm_gem_object_lookup(file_priv, args->handle);
	if (!gem_obj) {
		DRM_DEBUG("Failed to look up GEM BO %d\n", args->handle);
		return -ENOENT;
	}
	bo = GB02FUNC212(gem_obj);

	if (GB02MAC533 == args->pad) {
		args->offset = bo->base.iopaddr;
	} else {
		args->offset = bo->base.iovaddr;
	}
	gb_printf(KERN_INFO, "args->offset: 0x%llx %s\n", args->offset, __func__);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	drm_gem_object_put_unlocked(gem_obj);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	drm_gem_object_put(gem_obj);
#else
	drm_gem_object_unreference_unlocked(gem_obj);
#endif

	return 0;
}

static int GB02FUNC383(struct drm_device *dev, void *data,
			    struct drm_file *file_priv)
{
	return GB02FUNC379(dev, data, file_priv);
}

static int GB02FUNC387(struct drm_device *dev, void *data,
			    struct drm_file *file_priv)
{
	__u64 real_cpu_offset;
	__u64 vaddr;
	int mode;
	struct rb_root *gb_rb_root;
	struct GB02STR60 *gb_rb_data = NULL;
	struct GB02STR104 *args = (struct GB02STR104 *)data;
	struct GB02STR47 *gb_priv = file_priv->driver_priv;

	if (!gb_rb_tree) {
		gb_printf(KERN_ERR, "%s gb rb tree is not enabled\n",
			__func__);
		return -EINVAL;
	}

	if (args->gpu_va && !args->cpu_va) {
		mode = GB02MAC1130;
		vaddr = args->gpu_va;
	} else if (!args->gpu_va && args->cpu_va) {
		mode = GB02MAC1131;
		vaddr = args->cpu_va;
	} else {
		gb_printf(KERN_ERR, "%s args error,gpu_va:0x%llx cpu_va:0x%llx\n",
			__func__, (long long)args->gpu_va, (long long)args->cpu_va);
		return -ENOMEM;
	}

	gb_rb_root = GB02FUNC209(gb_priv, mode);
	if (!gb_rb_root) {
		gb_printf(KERN_ERR, "%s gb_bo_tree is null, mode:%d, drm_file:0x%llx\n",
			__func__, mode, (long long)file_priv);
		return -ENOMEM;
	}

	gb_rb_data = GB02FUNC241(gb_rb_root, vaddr, mode);
	if (gb_rb_data == NULL) {
		gb_printf(KERN_ERR, "%s Get cpu_va is null, gpu_va:0x%llx\n",
			__func__, (unsigned long long)args->gpu_va);
		return -EINVAL;
	}

	if (mode) {
		real_cpu_offset = args->cpu_va -
			gb_rb_data->gb_gem_bo->vm_info.vm_start;
		args->gpu_va = gb_rb_data->gb_gem_bo->vm_info.iovaddr +
			real_cpu_offset;
	} else {
		real_cpu_offset = args->gpu_va -
			gb_rb_data->gb_gem_bo->vm_info.iovaddr;
		args->cpu_va = gb_rb_data->gb_gem_bo->vm_info.vm_start +
			real_cpu_offset;
	}
	args->cpu_va_start = gb_rb_data->gb_gem_bo->vm_info.vm_start;
	args->cpu_va_end = gb_rb_data->gb_gem_bo->vm_info.vm_end;

#ifdef GB_KMD_DBG
	args->gpu_pa = gb_rb_data->gb_gem_bo->base.iopaddr + real_cpu_offset;
#endif

	return 0;

}

static int GB02FUNC395(struct drm_device *dev, void *data,
			    struct drm_file *file_priv)
{
	struct GB02STR113 *timems = (struct GB02STR113 *)data;
	if (timems->timems <= 0)
		return -EINVAL;
	GB02FUNC1349(timems->timems);
	return 0;
}
static int GB02FUNC397(struct GB02STR69 *bar, bool wc)
{
	if (bar->len) {
		request_mem_region(bar->base, bar->len, "gb");

		if (wc)
			bar->mmio = ioremap_wc(bar->base, bar->len);
		else
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
			bar->mmio = ioremap_nocache(bar->base, bar->len);
#else
			bar->mmio = ioremap(bar->base, bar->len);
#endif

		if (!bar->mmio)
			return -EFAULT;
	} else {
		bar->mmio = NULL;
	}
	gb_printf(KERN_INFO, "%s-%d: bar->iomap 0x%llx, wc = %d\n",
	 __func__, __LINE__, (long long)bar->mmio, wc);
	return 0;
}

u32 GB02FUNC403(void)
{
	struct GB02STR70 *gb_pcie = GB02FUNC518();
	u32 gpu_freq = GB02FUNC693();

	if (GENBU_02 == GB02FUNC460(gb_pcie->GB02STR153))
		return gpu_freq ;
	else
		return 0;
}

void GB02FUNC406(struct GB02STR47 *gb_priv)
{
	u32 prfcnt_config;

	if (!gb_priv) {
		gb_printf(KERN_ERR, "%s err gb_priv NULL\n", __func__);
		return;
	}

	/* Configure perf cnt*/
	prfcnt_config = GB02FUNC935() << GB02MAC1831;
	//reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1412),
	//		prfcnt_config | GB02MAC1833 | (1 << 8));
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1412),
			prfcnt_config | GB02MAC1833);
	reg_write(gb_priv->gbdev, GB02MAC1339(PRFCNT_BASE_LO),
			GB02MAC1226 & 0xFFFFFFFF);
	reg_write(gb_priv->gbdev, GB02MAC1339(PRFCNT_BASE_HI),
			GB02MAC1226 >> 32);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1413),
			0xffffffff);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1421),
			0xffffffff);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1416),
			0xffffffff);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1423),
			0xffffffff);
	if (performance2_set) {
		reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1412),
				prfcnt_config | GB02MAC1834 | (1 << 8));
	} else {
		reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1412),
				prfcnt_config | GB02MAC1834);
	}
}

void GB02FUNC416(struct GB02STR47 *gb_priv)
{
	if (!gb_priv) {
		gb_printf(KERN_ERR, "%s err gb_priv NULL\n", __func__);
		return;
	}

	/* Configure perf cnt*/
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1412),
			GB02MAC1833);
	reg_write(gb_priv->gbdev, GB02MAC1339(PRFCNT_BASE_LO), 0);
	reg_write(gb_priv->gbdev, GB02MAC1339(PRFCNT_BASE_HI), 0);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1413), 0);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1421), 0);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1416), 0);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1423), 0);
	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1403),
		GPU_COMMAND_PRFCNT_CLEAR);
	mdelay(10);
}

void GB02FUNC420(struct GB02STR47 *gb_priv)
{
	if (!gb_priv) {
		gb_printf(KERN_ERR, "%s err gb_priv NULL\n", __func__);
		return;
	}

	reg_write(gb_priv->gbdev, GB02MAC1339(GB02MAC1403),
			GPU_COMMAND_PRFCNT_SAMPLE);
	/* wait gpu write perf data to vram */
	mdelay(100);
}

int gl_perf_dump_cnt;
void GB02FUNC425(struct GB02STR47 *gb_priv)
{
	u32 l2_dbg_cmd[4] = {0xdeb005e0, 0xdeb005a0, 0xdeb00560, 0xdeb00520};
	u32 i, j, offset;
	u64 reg_val;

	if (!gb_priv) {
		pr_info("%s err gb_priv NULL\n", __func__);
		return;
	}

	pr_info("========l2 perf cnt %d dump begin========\n",
			gl_perf_dump_cnt);
	for (i = 0; i < 4; i++) {
		offset = 0x100;
		for (j = 0; j < 64; j++) {
			reg_write(gb_priv->gbdev, GB02MAC1339(0xfe0),
				offset);
			reg_write(gb_priv->gbdev, GB02MAC1339(0x54),
				l2_dbg_cmd[i]);
			udelay(20);
			reg_val = reg_read(gb_priv->gbdev, GB02MAC1339(0xfe0)) |
				((u64)reg_read(gb_priv->gbdev, GB02MAC1339(0xfe4)) << 32);
			pr_info("l2_%d 0x%x:%llu\n",
				i, offset, reg_val);
			offset += 4;
		}
	}
	pr_info("========l2 perf cnt %d dump end========\n",
			gl_perf_dump_cnt);
}

struct GB02STR47* GB02FUNC429(void)
{
	/* TODO: get any GB02STR47 in used and lock it*/
	return GB02FUNC91();
}

u32 perf_time = 100;
u32 GB02FUNC430(void __iomem *gb_perf_base)
{
	u32 gpu_ut, gpu_freq;
	u32* gpu_active = (u32*)(gb_perf_base + GB02MAC1417);

	gpu_freq = GB02FUNC403();
	if (!gpu_freq) {
		gb_printf(KERN_ERR, "%s get gpu ut err, gpu_active:0x%x, gpu_freq:0x%x\n",
			__func__, *gpu_active, gpu_freq);
		return 0xffffffff;
	}

	gpu_ut = ((u64)*gpu_active * 100) / (gpu_freq / (1000 / perf_time));
	return gpu_ut;
}

void __iomem *GB02FUNC437(struct GB02STR47 *gb_priv)
{
	u32 ddr_bar_id;

	ddr_bar_id = GB02FUNC468(
			gb_priv->gbdev->gb_pcie->GB02STR153);
	return (void __iomem*)gb_priv->gbdev->gb_pcie->pci_bars[ddr_bar_id].mmio +
			GB02MAC504;
}

int GB02FUNC439(u32 *gpu_ut)
{
	struct GB02STR47 *gb_priv = NULL;
	void __iomem *gb_perf_base = NULL;
	gb_priv = GB02FUNC429();
	*gpu_ut = 0;
	if (gb_priv == NULL)
		/* if no gb_priv is in used, then no proc use gpu */
		return 0;

	gb_perf_base = GB02FUNC437(gb_priv);
	/* TODO: get gb_priv lock */
	mutex_lock(&gb_priv->perfcnt_lock);
	memset_io(gb_perf_base, 0, GB02MAC1224);
	GB02FUNC406(gb_priv);
	/* sleep for perf increase cnt */
	msleep(perf_time);
	GB02FUNC420(gb_priv);
	/* sleep for gpu write perf data to vram */
	msleep(50);
	*gpu_ut = GB02FUNC430(gb_perf_base);
	if (*gpu_ut > 100)
		*gpu_ut = 100;
	GB02FUNC416(gb_priv);
	mutex_unlock(&gb_priv->perfcnt_lock);
	return 0;
}

void GB02FUNC443(void)
{
	int i, gap;
	u32 *dump_kaddr, *dump_kaddr_base;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec starttime, endtime;
#else
	struct timespec64 starttime, endtime;
#endif
	struct GB02STR47 *gb_perf_priv = GB02FUNC91();

	pr_info("========perf cnt dump start to sample now========\n");
	dump_kaddr_base = (u32 *)GB02FUNC437(gb_perf_priv);
	memset_io(dump_kaddr_base, 0, GB02MAC1224);
	GB02FUNC406(GB02FUNC91());

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&starttime);
	msleep(performance_ms);
	getnstimeofday(&endtime);
#else
	ktime_get_real_ts64(&starttime);
	msleep(performance_ms);
	ktime_get_real_ts64(&endtime);
#endif

	GB02FUNC420(gb_perf_priv);
	gl_perf_dump_cnt++;
	gap = (endtime.tv_sec - starttime.tv_sec) * 1000000 +
		(endtime.tv_nsec - starttime.tv_nsec) / 1000;
	pr_info("========perf cnt %d dump %d time data========\n",
			gl_perf_dump_cnt, gap);
	for (i = 0; i < GB02MAC1224; i += 16) {
		dump_kaddr = dump_kaddr_base + (i / 4);
		pr_info("0x2000%04x: %08x %08x %08x %08x\n", i,
					*dump_kaddr, *(dump_kaddr + 1),
					*(dump_kaddr + 2), *(dump_kaddr + 3));
	}
	pr_info("========perf cnt %d dump end=========\n",
			gl_perf_dump_cnt);

	GB02FUNC425(gb_perf_priv);
	GB02FUNC416(gb_perf_priv);

	/* just run once, set to false */
	perf_cnt = false;

}

int GB02FUNC449(void)
{
	if (strncmp(current->comm, "Xorg", 4) &&
		strncmp(current->comm, "systemd", 7) &&
		strncmp(current->comm, "kworker", 7) &&
		strncmp(current->comm, "modprobe", 8) &&
		strncmp(current->comm, "plymouthd", 9) &&
		strncmp(current->comm, "insmod", 6))
		return 1;
	else
		return 0;
}

static int GB02FUNC451(struct drm_device *dev, struct drm_file *file)
{
#ifdef GB02MAC295
	int ret;
	struct GB02STR39 *gbdev = dev->dev_private;
	struct GB02STR47 *gb_priv;

	gb_priv = kzalloc(sizeof(*gb_priv), GFP_KERNEL);
	if (!gb_priv)
		return -ENOMEM;

	gb_priv->gbdev = gbdev;
	gb_priv->drm_file = file;
	gb_priv->pcb = current;
	gb_priv->list_node.drm_file = file;
	gb_priv->list_node.cpu_root_node = RB_ROOT;
	gb_priv->list_node.gpu_root_node = RB_ROOT;
	gb_priv->list_node.gb_priv = gb_priv;
	file->driver_priv = gb_priv;

	ret = GB02FUNC1363(gb_priv);
	if (ret) {
		gb_printf(KERN_ERR, "%s GB02FUNC1363 err ! %d\n",
			__func__, ret);
		goto err_stage;
	}
/*
#ifdef GB02MAC483
	ret = GB02FUNC1753(dev, file, &gb_priv->dec_priv,
		       &gb_priv->dec1_priv, &gb_priv->enc_priv);
	if (ret) {
		gb_printf(KERN_ERR, "%s GB02FUNC1753 err ! %d\n",
			__func__, ret);
		goto err_stage;
	}
#endif
*/
	/* rb_list node must add at last */
	mutex_lock(&gbdev->rb_mutex);
	list_add(&gb_priv->list_node.list, &gb_list_head);
	mutex_unlock(&gbdev->rb_mutex);

	gb_printf(KERN_INFO, "%s invoke ok! drm_file:0x%llx\n",
		__func__, (long long)file);

	return 0;

err_stage:
	return ret;
#else
	return 0;
#endif
}

static void
gb_postclose(struct drm_device *dev, struct drm_file *file)
{
#ifdef GB02MAC295
	struct GB02STR47 *gb_priv = file->driver_priv;
	gb_printf(KERN_INFO, "%s close drm_file:0x%llx\n",
		__func__, (unsigned long long)file);

	GB02FUNC1744(dev, file);
	GB02FUNC1365(gb_priv);

	mutex_lock(&gb_priv->gbdev->rb_mutex);
	GB02FUNC203(gb_priv);
	mutex_unlock(&gb_priv->gbdev->rb_mutex);

	kfree(gb_priv);
	gb_priv = NULL;
#endif
}

static int
gb_ioctl_get_gpu_info(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct gpu_info *gb_info = GB02FUNC314();
	struct gpu_info *args = data;

	GB02FUNC1495(TTM_PL_VRAM);
	//GB02FUNC1701();

	memcpy(args, gb_info, sizeof(struct gpu_info));

	return 0;
}

static const struct drm_ioctl_desc gb_drm_driver_ioctls[] = {
	DRM_IOCTL_DEF_DRV(GB_SUBMIT, GB02FUNC329, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_CREATE_BO, GB02FUNC288, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_WAIT_BO, GB02FUNC353, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_MMAP_BO, GB02FUNC368, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_GET_PARAM, GB02FUNC198, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_GET_BO_OFFSET, GB02FUNC379, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_GET_BO_ADDR_MAPPING,
			GB02FUNC387, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_GET_GPU_INFO,
			gb_ioctl_get_gpu_info, DRM_RENDER_ALLOW),

	DRM_IOCTL_DEF_DRV(GB_VPU_VCMD_OPEN,
			GB02FUNC1726, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_VPU_VCMD_CLOSE,
			GB02FUNC1728, DRM_RENDER_ALLOW),
/*
	DRM_IOCTL_DEF_DRV(GB_VPU_VCMDBUF_FREE,
	GB02FUNC1736, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_VPU_VCMDBUF_RESERVE,
		GB02FUNC1733, DRM_RENDER_ALLOW),
*/
	DRM_IOCTL_DEF_DRV(GB_VPU_VCMDBUF_RESERVE,
		GB02FUNC1733, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_VPU_CMD_BUF_FREE,
		GB02FUNC1740, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_VPU_REG_OPS,
			GB02FUNC1729, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_DMA_TRANS_TO_FB,
			GB02FUNC1885, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_DMA_TRANS_TO_RAM,
			GB02FUNC1890, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_V2VDMA_TRANS,
			GB02FUNC1894, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_HDMA_OFFSET_TRANS,
			GB02FUNC1891, DRM_RENDER_ALLOW),
#ifdef GB_ALLSCREEN
	DRM_IOCTL_DEF_DRV(GB_GET_FB,
			GB02FUNC1892, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_CREATE_FB,
			GB02FUNC297, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_SWITCH_FB,
			GB02FUNC1893, DRM_RENDER_ALLOW),
#endif
	DRM_IOCTL_DEF_DRV(GB_SET_RENDER_SIZE,
			GB02FUNC1884, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_GET_BO_VADDR,
			GB02FUNC383, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_SET_SCHED_TIME_MS,
			GB02FUNC395, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(GB_INFINITY_ATOMIC,
			GB02FUNC1390, 0),
};

int GB02FUNC482(struct inode *inode, struct file *filp)
{
	struct drm_file *file_priv = filp->private_data;
	struct GB02STR47 *gb_priv = file_priv->driver_priv;
	GB02FUNC1346(gb_priv);
	return drm_release(inode, filp);
}

static int GB02FUNC484(struct file *filp, fl_owner_t id)
{
	int i;
	long timeout = msecs_to_jiffies(job_timeout_ms);
	struct drm_file *file_priv = filp->private_data;
	struct GB02STR47 *gb_priv = file_priv->driver_priv;

	for (i = 0; i < GB02MAC288; i++)
		timeout = drm_sched_entity_flush(&gb_priv->sched_entity[i], timeout);

	return timeout >= 0 ? 0 : timeout;
}

static const struct file_operations gb_drm_driver_fops = {
        .owner = THIS_MODULE,
        .open = drm_open,
        .flush = GB02FUNC484,
        .release = GB02FUNC482,
        .unlocked_ioctl = drm_ioctl,
        .mmap = GB02FUNC728,
        .poll = drm_poll,
        .read = drm_read,
        .compat_ioctl = drm_compat_ioctl,
        .llseek = noop_llseek,
};

vm_fault_t GB02FUNC490(struct vm_fault *vmf)
{
#if 0
	struct vm_area_struct *vma = vmf->vma;
	struct drm_gem_object *obj = vma->vm_private_data;
	struct GB02STR59 *bo = to_gb_bo(obj);
#endif
	return VM_FAULT_SIGBUS;
}

const struct vm_operations_struct gb_vm_ops = {
        .fault = GB02FUNC490,
        .open = drm_gem_vm_open,
        .close = drm_gem_vm_close,
};

static struct drm_driver gb_drm_driver = {
	.driver_features	= DRIVER_RENDER | DRIVER_GEM | DRIVER_SYNCOBJ
#ifdef GB02MAC481
	| DRIVER_MODESET
#endif
//#ifdef CONFIG_GBDC_ATOMIC_FEATURE
	| DRIVER_ATOMIC
//#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	| DRIVER_PRIME
#endif
	,
	.load = GB02FUNC1185,
	.unload = gbdc_unload,
#if !(defined SYS_CENTOS7_COMPILE_ENV || defined SYS_CENTOS7_9_2009) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
	.set_busid = drm_pci_set_busid,
#endif
	.open			= GB02FUNC451,
	.postclose		= gb_postclose,
	.lastclose		= drm_fb_helper_lastclose,
	.ioctls			= gb_drm_driver_ioctls,
	.num_ioctls		= ARRAY_SIZE(gb_drm_driver_ioctls),
	.fops			= &gb_drm_driver_fops,

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	.gem_free_object 	= GB02FUNC688,
	.gem_vm_ops             = &gb_vm_ops,
	.gem_open_object        = GB02FUNC715,
	.gem_close_object       = GB02FUNC723,
	.dumb_destroy = drm_gem_dumb_destroy,
#endif
	.dumb_create = GB02FUNC1779,
	.dumb_map_offset = GB02FUNC1783,
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 3, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	.prime_handle_to_fd = GB02FUNC1022,
	.prime_fd_to_handle = GB02FUNC1084,
	.gem_prime_export = GB02FUNC1060,
	.gem_prime_import = GB02FUNC1071,
	.gem_prime_get_sg_table = drm_gem_shmem_get_sg_table,
	.gem_prime_vmap = drm_gem_shmem_vmap,
	.gem_prime_vunmap = drm_gem_shmem_vunmap,
	.gem_prime_import_sg_table = drm_gem_shmem_prime_import_sg_table,
#else
	.prime_handle_to_fd = GB02FUNC1022,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_import = drm_gem_prime_import,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 3, 0)
	.gem_prime_export = drm_gem_prime_export,
	.gem_prime_res_obj = gb_gem_prime_res_obj,
#endif
#endif
	.name			= DRIVER_NAME,
	.desc			= DRIVER_DESC,
	.date			= DRIVER_DATE,
	.major			= GB02MAC447,
	.minor			= GB02MAC450,
	.patchlevel		= GB02MAC453,
};

static struct GB02STR67 genbu_glb_asic_dev_info[GENBU_TYPE_MAX] = {
	{GENBU_FPGA_DEV_INFO},
	{GENBU_01_DEV_INFO},
	{GENBU_02_DEV_INFO},
};

static struct GB02STR67 *GB02FUNC495(enum genbu_asic_type gb_type)
{
	if (gb_type >= GENBU_TYPE_MAX) {
		gb_printf(KERN_ERR, "%s fail!gbtype:%d\n", __func__, gb_type);
		return NULL;
	}

	return &(genbu_glb_asic_dev_info[gb_type]);
}

static int GB02FUNC497(struct pci_dev *pdev,
		struct gpu_info *devinfo, struct GB02STR39 *gbdev,
		struct GB02STR67 *dtype_info)
{
	int vram_id = 0;
	char *devmodel = "GenBu";
	char *devname = "GenBu02";
	char *devproduct = "Sietium";
	char *drv_info	= GB_PCI_NAME;

	strncpy(devinfo->devmodel, devmodel, min(strlen(devmodel),
			 sizeof(devinfo->devmodel) - 1));
	strncpy(devinfo->devname, devname, min(strlen(devname),
			 sizeof(devinfo->devname) - 1));
	strncpy(devinfo->product, devproduct, min(strlen(devproduct),
			 sizeof(devinfo->product) - 1));

	vram_id = GB02FUNC468(dtype_info);
	devinfo->vram = gbdev->gb_pcie->pci_bars[vram_id].len>>20;
	devinfo->clk = 33;
	devinfo->bitwidth = 64;

	if (pdev->bus->cur_bus_speed == PCIE_SPEED_8_0GT)
		devinfo->bandwidth = 8;
	else if (pdev->bus->cur_bus_speed == PCIE_SPEED_5_0GT)
		devinfo->bandwidth = 5;
	else if (pdev->bus->cur_bus_speed == PCIE_SPEED_2_5GT)
		devinfo->bandwidth = 2;

	strncpy(devinfo->driver_info, drv_info, min(strlen(drv_info),
			 sizeof(devinfo->driver_info) - 1));

	if (GB02FUNC460(dtype_info) == GENBU_01) {
		devinfo->vender_id = GB02MAC874;
		devinfo->device_id = GB02MAC875;
	} else if (GB02FUNC460(dtype_info) == GENBU_FPGA) {
		devinfo->vender_id = GB02MAC876;
		devinfo->device_id = GB02MAC877;
	} else if (GB02FUNC460(dtype_info) == GENBU_02) {
		devinfo->vender_id = GB02MAC878;
		devinfo->device_id = GB02MAC879;
	}

	sprintf(devinfo->bus_info, "pci@%4d:%2d.%d", pdev->bus->number,
		((pdev->devfn)>>3) & 0x1f, (pdev->devfn) & 0x7);

	gbdev->gpu_device_info = devinfo;

	return 0;
}

irqreturn_t GB02FUNC502(int irq, void *data)
{
	return GB02FUNC877(irq, data);
}
#define GB02MAC802
#if 1
enum gb_board_type GB02FUNC503(struct GB02STR70 *gpi)
{
	unsigned int val, ret;
	if (gpi->board_id >= 0x0 && gpi->board_id <= 0x1f)
		val = gpi->board_id;
	else {
		iowrite32(0x60000000, gpi->pci_bars[4].mmio + 0xd0c);
		val = ioread32(gpi->pci_bars[0].mmio + 0x210);
		gpi->board_id = val;
	}

	/* gb_printf(KERN_ERR, "%s %d board_id:%u\n", __func__, __LINE__, val); */
	switch (val) {
	case PCIE_LPDDR4_VAL:
		ret = PCIE_LPDDR4;
		break;
	case PCIE_CQ2040_P21_VAL:
	case PCIE_C0_200_VAL:
		ret = PCIE_C0_200;
		break;
	case FULL_LPDDR4_VAL:
		ret = PCIE_FULL_LPDDR4;
		break;
	case PCIE_M6FL8G_LPDDR4_VAL:
		ret = PCIE_M6FL8G_LPDDR4;
		break;
	case PCIE_HIE1LP4_LPDDR4_VAL:
	case PCIE_HIE1LP42_LPDDR4_VAL:
		ret = PCIE_HIE1LP4_LPDDR4;
		break;
	case PCIE_M4HL8G_LPDDR4_VAL:
		ret = PCIE_M4HL8G_LPDDR4;
		break;
	case CQ2040_MXM_M60_VAL:
		ret = GB02MAC1363;
		break;
	default:
		ret = PCIE_FULL_FUNC_DDR4;
		break;
	}
	return ret;

}

bool GB02FUNC507(int connector_id)
{
	bool is_vga = false;

	if ((PCIE_LPDDR4 == GB02FUNC503(GB02FUNC518()) ||
			PCIE_C0_200 == GB02FUNC503(GB02FUNC518()) ||
			GB02MAC1363 == GB02FUNC503(GB02FUNC518())) &&
		connector_id == 3)
		is_vga = true;
	else if (PCIE_FULL_LPDDR4 == GB02FUNC503(GB02FUNC518()) &&
                connector_id == 4)
                is_vga = true;

	return is_vga;
}
static void GB02FUNC511(struct GB02STR70 *gpi, int HDMI_reset_flag)
{
	int type = GB02FUNC503(gpi);

	iowrite32(0x60000000, gpi->pci_bars[4].mmio + 0xd0c);
	if ((HDMI_reset_flag == HIBERNATE_SLEEP) ||
		(HDMI_reset_flag == SYSTEM_WORKING)) {
		if (type == PCIE_LPDDR4 || type == PCIE_C0_200)
			HDMI_power_reset(gpi, 2);
		else if (type == PCIE_FULL_LPDDR4)
			HDMI_power_reset(gpi, 5);
		else if (type == PCIE_M6FL8G_LPDDR4) {
			HDMI_power_reset(gpi, 1);
			HDMI_power_reset(gpi, 3);
			HDMI_power_reset(gpi, 4);
			HDMI_power_reset(gpi, 5);
		} else if (type == PCIE_HIE1LP4_LPDDR4) {
			HDMI_power_reset(gpi, 0);
			HDMI_power_reset(gpi, 2);
			HDMI_power_reset(gpi, 3);
			HDMI_power_reset(gpi, 5);
		} else if (type == PCIE_M4HL8G_LPDDR4) {
			HDMI_power_reset(gpi, 0);
			HDMI_power_reset(gpi, 2);
			HDMI_power_reset(gpi, 3);
			HDMI_power_reset(gpi, 5);
		}
	}
}
static void GB02FUNC517(struct GB02STR70 *gpi, int HDMI_reset_flag)
{
	iowrite32(0x60000000, gpi->pci_bars[4].mmio + 0xd0c);
	/* DC read QOS 7 */
	iowrite32(0x1c0000, gpi->pci_bars[0].mmio + 0x1b4);
}
#endif

struct GB02STR70 *GB02FUNC518(void)
{
	return gl_gbdev->gb_pcie;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
int gb_drm_get_pci_dev(struct pci_dev *pdev, const struct pci_device_id *ent,
		struct drm_driver *kms_driver)
{
	struct drm_device *dev;
	int ret;

	dev = drm_dev_alloc(kms_driver, &pdev->dev);
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	ret = pci_enable_device(pdev);
	if (ret)
		goto err_free;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0))
	dev->pdev = pdev;
#endif
	pci_set_drvdata(pdev, dev);
	ret = drm_dev_register(dev, ent->driver_data);
	if (ret)
		goto err_agp;

	return 0;

err_agp:
	pci_disable_device(pdev);
err_free:
	drm_dev_put(dev);
	return ret;
}
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static int gb_kick_out_firmware_fb(struct pci_dev *pdev, struct drm_driver *driver)
#else
static int gb_kick_out_firmware_fb(struct pci_dev *pdev,int ddr_bar_id)
#endif
{
#if defined(CONFIG_X86) || LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	bool primary = false;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	struct apertures_struct *ap;

	ap = alloc_apertures(1);
	if (!ap)
		return -ENOMEM;

	ap->ranges[0].base = pci_resource_start(pdev, ddr_bar_id);
	ap->ranges[0].size = pci_resource_len(pdev, ddr_bar_id);
#endif

#ifdef CONFIG_X86
	primary = pdev->resource[PCI_ROM_RESOURCE].flags & IORESOURCE_ROM_SHADOW;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	drm_aperture_remove_framebuffers(driver);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	drm_aperture_remove_framebuffers(primary, driver);
#else
	drm_fb_helper_remove_conflicting_framebuffers(ap, "gbdrmfb", primary);
	kfree(ap);
#endif


	return 0;
}
static int GB02FUNC525(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct GB02STR39 *gbdev;
	struct pci_device_id kms_pci_id;
	int err, i;
	enum genbu_asic_type gb_type = (enum genbu_asic_type)(ent->driver_data);
	struct GB02STR67 *GB02STR153 = GB02FUNC495(gb_type);
	struct gpu_info *info_handle;
#ifdef	GB02MAC292
	struct GB02STR83 apcie_info;
#endif
	int ret,ddr_bar_id;
	enum gb_board_type board_type;

	/* set the default log level, 0:quite mode,
	 * 3:only print KERN_ERR, 7:print all log */
	atomic_set(&log_en, 3);
	ddr_bar_id = GB02FUNC468(GB02STR153);

/* Get rid of things like offb */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = gb_kick_out_firmware_fb(pdev,	&gb_drm_driver);
#else
	ret = gb_kick_out_firmware_fb(pdev, ddr_bar_id);
#endif
	gb_printf(KERN_INFO, "%s,kick out fb ,ret %d\n",__func__, ret);

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "%s Fatal error during PCI device init:%d\n",
			__func__, err);
		return err;
	}

	gbdev = devm_kzalloc(&pdev->dev, sizeof(*gbdev), GFP_KERNEL);
	if (!gbdev)
		return -ENOMEM;

	gbdev->gb_pcie =
	 devm_kzalloc(&pdev->dev, sizeof(*(gbdev->gb_pcie)), GFP_KERNEL);
	if (!gbdev->gb_pcie)
		return -ENOMEM;

	gbdev->dev = &pdev->dev;
	gbdev->gb_pcie->pdev = pdev;
	gbdev->gb_pcie->gbdev = gbdev;
	gl_gbdev = gbdev;
	gbdev->gb_pcie->GB02STR153 = GB02STR153;

	for (i = 0; i < GB02FUNC469(GB02STR153); i++) {
		bool wc;

		if(GB02FUNC468(GB02STR153) == i)
			GB02FUNC186(gbdev->gb_pcie->pdev);

		gbdev->gb_pcie->pci_bars[i].base = pci_resource_start(pdev, i);
		gbdev->gb_pcie->pci_bars[i].len = pci_resource_len(pdev, i);

		if (gbdev->gb_pcie->pci_bars[i].len == 0) {
			gb_printf(KERN_INFO, "%s i: %d\n", __func__, i);
			continue;
		}

		wc = (GB02FUNC468(GB02STR153) == i) ? true : false;
		if (GB02FUNC397(&gbdev->gb_pcie->pci_bars[i], wc)) {
			dev_err(&pdev->dev, "Fatal error during PCI bar ioremap\n");
			return -EFAULT;
		}

		if (GB02FUNC468(GB02STR153) == i)
			ddr_va = gbdev->gb_pcie->pci_bars[i].mmio;

		dev_info(&pdev->dev, "GB PCI BAR[%d] base 0x%llx, iomap 0x%llx, size 0x%llx\n",
			i, gbdev->gb_pcie->pci_bars[i].base,
			(long long)gbdev->gb_pcie->pci_bars[i].mmio,
			gbdev->gb_pcie->pci_bars[i].len);
	}

#ifdef GB02MAC294
	GB02FUNC175(gbdev);
#endif
	gbdev->gb_pcie->board_id = -1;
	GB02FUNC517(gbdev->gb_pcie, SYSTEM_WORKING);
	HDMI_power_reset_all(gbdev->gb_pcie);

#ifdef	GB02MAC292
	i = GB02FUNC468(GB02STR153);
	apcie_info.ddrwr_va = ddr_va = gbdev->gb_pcie->pci_bars[i].mmio;
	apcie_info.ddrwr_pa = gBaseDDRHw = gbdev->gb_pcie->pci_bars[i].base;
	apcie_info.ddrwr_len = gBaseDDRLen = gbdev->gb_pcie->pci_bars[i].len;
	i = GB02FUNC479(GB02STR153);
	apcie_info.hdwr_va = hd_va = gbdev->gb_pcie->pci_bars[i].mmio;
	apcie_info.hdwr_pa = gBaseHdwr = gbdev->gb_pcie->pci_bars[i].base;
	apcie_info.hdwr_len = gBaseLen = gbdev->gb_pcie->pci_bars[i].len;
	i = GB02FUNC477(GB02STR153);
	apcie_info.base_switch = (u64)gbdev->gb_pcie->pci_bars[i].mmio;
	i = GB02FUNC474(GB02STR153);
	apcie_info.pll_va = (u64)gbdev->gb_pcie->pci_bars[i].mmio;
	GB02FUNC508();
	GB02FUNC478(&apcie_info);
	gb_printf(KERN_INFO, "-------&&&&&&&&&&&&&&------ddr va =%pK--hd va=%pK--\n",
	 ddr_va, hd_va);
#endif

	pci_set_master(pdev);
	info_handle = GB02FUNC314();
	GB02FUNC497(pdev, info_handle, gbdev, GB02STR153);
	dev_info(gbdev->dev, "VRAM: %lluM \n", (unsigned long long)(info_handle->vram));
	/* It's needed to enable msi, or the interrupt handler
	 * can't be triggerred.
	 */
#ifndef GB02MAC287
	if (pci_enable_msi(pdev))
		dev_err(&pdev->dev, "Don't support MSI");
#endif
	gb_printf(KERN_INFO, "-------irq init----\n");
	err = GB02FUNC352(gbdev->gb_pcie);
	if (err) {
		dev_err(&pdev->dev, "GB02FUNC352 failed!\n");
		return err;
	}

	err = GB02FUNC364(gbdev->gb_pcie);
	if (err) {
		dev_err(&pdev->dev, "GB02FUNC364 failed!\n");
		return err;
	}
	GB02FUNC84(gbdev);

	if (gb_type == GENBU_02)
		GB02FUNC426(gbdev->gb_pcie);

#ifdef GB02MAC481
	board_type = GB02FUNC503(gbdev->gb_pcie);
	for (i = 0; i < GB02MAC658; i++) {
		bool is_edp = false;
		if (GB02FUNC1902(board_type, i))
			continue;

		if ((GB02MAC1363 == board_type) && ((0 == i) || (1 == i)))
			is_edp = true;
		err |= GB02FUNC1005(i, gbdev->gb_pcie, is_edp);
		if (err) {
			gb_printf(KERN_ERR, "%s %d dp init %d error:%d!\n",
				__func__, __LINE__, i, err);
		}
	}
#endif

#ifdef GB02MAC295
	err = GB02FUNC1703(gbdev->gb_pcie);
	if (err) {
		gb_printf(KERN_ERR, "%s %d hdma init error:%d!\n",
			__func__, __LINE__, err);
	}

	err = GB02FUNC107(gbdev, GB02STR153);
	if (err) {
		if (err != -EPROBE_DEFER)
			dev_err(&pdev->dev, "Fatal error during GPU init\n");
		goto err_out0;
	}
#endif

	atomic_set(&gbdev->reset.pending, GB_RST_IDLE);
	pm_runtime_set_active(gbdev->dev);
	pm_runtime_mark_last_busy(gbdev->dev);
	pm_runtime_enable(gbdev->dev);
	pm_runtime_set_autosuspend_delay(gbdev->dev, 50);
	pm_runtime_use_autosuspend(gbdev->dev);
	kms_pci_id.driver_data = (unsigned long)gbdev;

	INIT_LIST_HEAD(&gb_list_head);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0))
	err = drm_get_pci_dev(pdev, &kms_pci_id,
			&gb_drm_driver);
#else
	err = gb_drm_get_pci_dev(pdev, &kms_pci_id,
			&gb_drm_driver);
#endif

	if (err) {
		dev_err(&pdev->dev, "%s %d:drm_get_pci_dev failed!:%d\n",
			__func__, __LINE__, err);
		goto err_out1;
	}
#if 0
	ret |= GB02FUNC1742(gb_pcie_priv);
#endif
#ifdef GB02MAC292
	GB02FUNC486(&pdev->dev);
	GB02FUNC240();
#endif
	err |= GB02FUNC1856(gbdev->gb_pcie);
	if (err) {
		gb_printf(KERN_ERR, "%s %d v2vdma init error:%d!\n",
			__func__, __LINE__, err);
	}
	/* MCU firmware upgrade init */
	err = GB02FUNC670(gbdev, GB02STR153);
	if (err)
		dev_err(&pdev->dev, "firmware upgrade init failed:%d\n", err);

#ifdef GB02MAC511
	GB02FUNC296(gbdev);
#endif
	err = GB02FUNC317(info_handle);
	if (err) {
		dev_err(&pdev->dev, "GB02FUNC317 failed!\n");
		return err;
	}

	err = GB02FUNC334(gbdev);
	if (err) {
		dev_err(&pdev->dev, "GB02FUNC334 failed!\n");
		return err;
	}

	GB02FUNC42(gbdev);
	return err;
err_out1:
	pm_runtime_disable(gbdev->dev);
#ifdef GB02MAC295
	GB02FUNC128(gbdev);
#endif

#ifdef GB02MAC294
	GB02FUNC177(gbdev);
#endif
err_out0:
	return err;
}

static void GB02FUNC555(struct pci_dev *pdev)
{
	struct GB02STR39 *gbdev = NULL;

	gbdev = gl_gbdev;
	HDMI_power_down(gbdev->gb_pcie);

	gb_printf(KERN_INFO, "%s-%d: GB02FUNC555\n",
	 __func__, __LINE__ );
}

static void GB02FUNC558(struct pci_dev *pdev)
{
	struct drm_device *ddev = pci_get_drvdata(pdev);
	struct GB02STR39 *gbdev = NULL;
	enum genbu_asic_type gb_type;
	int i;

	if (ddev == NULL) {
		gb_printf(KERN_ERR, "%s %d:Error: drm_device is NULL\n",
			__func__, __LINE__);
		/* return; */
	}

	GB02FUNC322();
	/* gbdev = (struct GB02STR39 *)ddev->dev_private; */
	gbdev = gl_gbdev;
	if (gbdev == NULL) {
		gb_printf(KERN_ERR, "%s %d:Error: GB02STR39 is NULL\n",
			__func__, __LINE__);
		return;
	}

	GB02FUNC336(gbdev);
	GB02FUNC48(gbdev);

	pm_runtime_get_sync(gbdev->dev);
#ifdef GB02MAC294
	GB02FUNC177(gbdev);
#endif
#ifdef GB02MAC295
	GB02FUNC128(gbdev);
#endif
	pm_runtime_put_sync_suspend(gbdev->dev);
	pm_runtime_disable(gbdev->dev);

	for (i = 0; i < GB02FUNC469(
		gbdev->gb_pcie->GB02STR153); i++) {
		if (gbdev->gb_pcie->pci_bars[i].mmio)
			iounmap(gbdev->gb_pcie->pci_bars[i].mmio);

		release_mem_region(gbdev->gb_pcie->pci_bars[i].base,
		 gbdev->gb_pcie->pci_bars[i].len);
	}

	gb_type = GB02FUNC460(gbdev->gb_pcie->GB02STR153);
	if (gb_type == GENBU_02)
		GB02FUNC423(gbdev->gb_pcie);
	/* GB02FUNC370(gb_pcie_priv);
	 * GB02FUNC359(gb_pcie_priv); */
	GB02FUNC370(gbdev->gb_pcie);
	GB02FUNC359(gbdev->gb_pcie);

	pci_disable_msi(pdev);
	pci_clear_master(pdev);
	pci_disable_device(pdev);

	drm_put_dev(ddev);
}

static struct pci_device_id gb_ids[] = {
	{
		PCI_DEVICE(GB02MAC876,
			   GB02MAC877),
		.class = 0,
		.class_mask = 0,
		.driver_data = GENBU_FPGA,
	},
	{ PCI_DEVICE(GB02MAC874, GB02MAC875),
		.class = (PCI_CLASS_DISPLAY_VGA << 8),
		.class_mask = 0xffff00,
		.driver_data = GENBU_01,
	},
	{
		PCI_DEVICE(GB02MAC878, GB02MAC879),
		.class = 0,
		.class_mask = 0,
		.driver_data = GENBU_02,
	},
	{}

};

static int GB02FUNC567(struct device *dev)
{
        gb_printf(KERN_INFO, "--enter -GB pm prepare--- \n");
        return 0;
}

static int GB02FUNC568(struct drm_device *drm_dev)
{
	struct GB02STR39 *gbdev = (struct GB02STR39 *)drm_dev->dev_private;
	struct GB02STR70 *gb_pcie = gbdev->gb_pcie;
	struct dptx *dptx_info = NULL;
	int i = 0;
	enum gb_board_type board_type = GB02FUNC503(gb_pcie);

	mdelay(100);
	for (i = 0; i < GB02MAC658; i++) {
		if (GB02FUNC1902(board_type, i) || !gb_pcie->dptx[i])
			continue;
		GB02FUNC1015(i, gb_pcie, false);
		dptx_info = gb_pcie->dptx[i];
		dptx_info->edid_getted = false;
		dptx_info->link.trained = false;
		cancel_delayed_work_sync(&dptx_info->hotplug_work);
	}

	return 0;

}

static int GB02FUNC570(struct drm_device *drmdev)
{
	struct drm_crtc *crtc = NULL;
	u64 left_up_fb = U64_MAX;
	unsigned long cur_fb_offset = 0;
	u64 save_fb_size = 0;
	int ret = 0;
	struct drm_plane_state *state;
	struct gbdc_crtc *gbdc_crtc;
	//dump_func_begin;

	drm_for_each_crtc(crtc, drmdev) {
		gbdc_crtc = GB02FUNC1573(crtc);

		if (gbdc_crtc && crtc->enabled) {
			struct drm_framebuffer *fb;

			if (crtc->primary && crtc->primary->state) {
				state = crtc->primary->state;
				fb = state->fb;
				if (fb) {
					gb_printf(KERN_INFO, "crtc id %d,fb id %d\n",
							gbdc_crtc->crtc_id, fb->base.id);
					gb_printf(KERN_INFO, "cur_fb_offset 0x%lx\n",
							gbdc_crtc->cplanes[DC_PLANE_GRAPHIC].cur_fb_offset);
					save_fb_size = fb->width * fb->height * 4;
					cur_fb_offset = gbdc_crtc->cplanes[DC_PLANE_GRAPHIC].cur_fb_offset;
					if (left_up_fb > cur_fb_offset)
						left_up_fb = cur_fb_offset;
					}
			}

		}

	}

	gb_printf(KERN_INFO, "left_up_fb 0x%llx, FB SIZE 0x%llx\n", left_up_fb, save_fb_size);
	ret = GB02FUNC294(left_up_fb, save_fb_size, GB_BLOCK_FB);
	//dump_func_end;

	return ret;
}
static int __maybe_unused GB02FUNC575(struct drm_device *drm_dev)
{
	int ret = 0;
	int cursor_save_index = GB_BLOCK_CURSOR_FIRST;
	struct drm_crtc *crtc;
	u64 cursor_size = 0;

	drm_for_each_crtc(crtc, drm_dev) {
		struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

		if (crtc->enabled && gbdc_crtc->cursor_addr) {
			cursor_size = gbdc_crtc->cursor_height * gbdc_crtc->cursor_width * 4;

			gb_printf(KERN_INFO, "cursor_size 0x%llx,cursor_addr 0x%llx\n",
						cursor_size, gbdc_crtc->cursor_addr);
			gb_printf(KERN_INFO, "%s : gbcrtc id %d\n", __func__, gbdc_crtc->crtc_id);
			gbdc_crtc->cursor_resume = 0;
			ret = GB02FUNC294(gbdc_crtc->cursor_addr, cursor_size,
							cursor_save_index);
			if (!ret)
				gb_printf(KERN_INFO, "cursor register block ok\n");
			WARN_ON((cursor_save_index++) > GB_BLOCK_MAX);

		}
	} //for each
	return ret;
}
static int __maybe_unused GB02FUNC577(struct drm_device *drm_dev)
{
	struct drm_crtc *crtc;
	int ret = 0;

	drm_for_each_crtc(crtc, drm_dev) {
		struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);

		gb_printf(KERN_INFO, "crtc id %d, cursor_bo %p\n", gbdc_crtc->crtc_id,
				gbdc_crtc->cursor_bo);
		if (gbdc_crtc && gbdc_crtc->cursor_bo)
			gbdc_crtc->cursor_resume = 1;

	} //for each
	return ret;
}
static int GB02FUNC578(struct drm_device *drm_dev)
{
	int ret = 0;

	//ret = GB02FUNC577(drm_dev);
	gb_printf(KERN_INFO, "%s,ret =%d\n", __func__, ret);
	return ret;
}

static int GB02FUNC580(struct drm_device *drm_dev)
{
	int ret = 0;

	ret = GB02FUNC570(drm_dev);
	//ret |= GB02FUNC575(drm_dev);

	gb_printf(KERN_INFO, "%s ret=%d\n", __func__, ret);

	return ret;
}

static int GB02FUNC582(struct drm_device *drm_dev)
{
	int i = 0;
	int err = 0;
	struct GB02STR39 *gbdev = (struct GB02STR39 *)drm_dev->dev_private;
	struct GB02STR70 *gb_pcie = gbdev->gb_pcie;
	enum gb_board_type board_type = GB02FUNC503(gb_pcie);

	for (i = 0; i < GB02MAC658; i++) {
		if (GB02FUNC1902(board_type, i) || !gb_pcie->dptx[i])
			continue;

		err = GB02FUNC1016(i, gbdev->gb_pcie, false);
		if (err)
			gb_printf(KERN_ERR, "%s, %d dp init %d error:%d!\n",
				__func__, __LINE__, i, err);
	}

	return err;
}

static void GB02FUNC584(struct GB02STR39 *gbdev,
	enum system_working_status sleep_status)
{
	gbdev->sleep_status = sleep_status;
}

static int GB02FUNC585(struct GB02STR39 *gbdev)
{
	return gbdev->sleep_status;
}

static void GB02FUNC587(void)
{
	GB02FUNC966();
	GB02FUNC1404();
}

static void GB02FUNC588(struct drm_device *dev, struct GB02STR39 *gbdev)
{
	GB02FUNC969();
	GB02FUNC1405();
}

int GB02FUNC590(struct drm_device *dev, bool resume)
{
	int ret = 0;
	struct GB02STR39 *gbdev = (struct GB02STR39 *)dev->dev_private;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
	gb_pci_restore_rebar_state(dev->pdev);
#endif

	if (resume) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
		pci_set_power_state(dev->pdev, PCI_D0);
		pci_restore_state(dev->pdev);
		ret = pci_enable_device(dev->pdev);
		pci_set_master(dev->pdev);
#else
		pci_set_power_state(to_pci_dev(dev->dev), PCI_D0);
		pci_restore_state(to_pci_dev(dev->dev));
		ret = pci_enable_device(to_pci_dev(dev->dev));
		pci_set_master(to_pci_dev(dev->dev));
#endif
		GB02FUNC608(gbdev->gb_pcie->pci_bars[0].mmio, gbdev->gb_pcie->pci_bars[4].mmio, &gbdev->version);
		if (ret) {
			gb_printf(KERN_ERR, "pci_enable_device failed\n");
		}
	}

	GB02FUNC412();
	GB02FUNC517(gbdev->gb_pcie, GB02FUNC585(gbdev));
	GB02FUNC511(gbdev->gb_pcie, GB02FUNC585(gbdev));
	GB02FUNC582(dev);
	GB02FUNC285();
	GB02FUNC578(dev);
	//drm_helper_resume_force_mode(dev);
	ret = drm_mode_config_helper_resume(dev);
	GB02FUNC587();
	GB02FUNC1607();
	GB02FUNC136(gbdev);
	gbdev->mmu_mode->mmu_controll(gbdev, GB02MAC1274);
	GB02FUNC1343(gbdev);
	GB02FUNC185(gbdev);
	return ret;
}

int GB02FUNC594(struct drm_device *dev, bool suspend)
{
	struct GB02STR39 *gbdev = (struct GB02STR39 *)dev->dev_private;
	int ret = 0;

	GB02FUNC1352(gbdev);

	if (!GB02FUNC1368(gbdev)) {
		gb_printf(KERN_ERR, "%s stage is not idle\n", __func__);
		return -EBUSY;
	}

	GB02FUNC607(gbdev->gb_pcie->pci_bars[0].mmio, &gbdev->version);
	GB02FUNC584(gbdev, suspend);

	GB02FUNC178(gbdev);
	GB02FUNC580(dev);
	GB02FUNC257(dev, gbdev);
	GB02FUNC568(dev);
	GB02FUNC588(dev, gbdev);
	GB02FUNC1610();
	gb_printf(KERN_INFO, "drm_mode_config_helper_suspend begin\n");
	ret = drm_mode_config_helper_suspend(dev);
	gb_printf(KERN_INFO, "ret %d\n", ret);
	if (ret)
		return ret;
	gb_printf(KERN_INFO, "drm_mode_config_helper_suspend done\n");
	GB02FUNC860(gbdev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
	pci_save_state(dev->pdev);
#else
	pci_save_state(to_pci_dev(dev->dev));
#endif
	if (suspend) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
		pci_disable_device(dev->pdev);
		gb_printf(KERN_INFO, "disable pci device\n");
#else
		pci_disable_device(to_pci_dev(dev->dev));
#endif
	}
	gb_printf(KERN_INFO, "suspend done\n");
	return 0;
}

static int GB02FUNC598(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	return GB02FUNC594(drm_dev, true);
}

static int GB02FUNC599(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	return GB02FUNC590(drm_dev, true);
}

static int GB02FUNC600(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	gb_printf(KERN_INFO, "--enter -gb freeze--- \n");

	return GB02FUNC594(drm_dev, false);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static int gb_pmops_thaw(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	gb_printf(KERN_INFO, "--enter -gb thaw--- \n");

	return GB02FUNC590(drm_dev, false);
}
#endif

static int GB02FUNC602(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	gb_printf(KERN_INFO, "--enter -gb restore--- \n");

	return GB02FUNC590(drm_dev, false);
}

static const struct dev_pm_ops gb_pmops = {
        .prepare = GB02FUNC567,
        .suspend = GB02FUNC598,
        .resume = GB02FUNC599,
        .freeze = GB02FUNC600,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
        .thaw = gb_pmops_thaw,
#endif
        .restore = GB02FUNC602,
};

static struct pci_driver gb_driver = {
	.name		= "gb",
	.id_table	= gb_ids,
	.probe		= GB02FUNC525,
	.remove		= GB02FUNC558,
	.shutdown	= GB02FUNC555,
	.driver.pm  	= &gb_pmops,
};

static int __init GB02FUNC605(void)
{
	int err;

	GB02FUNC1();

	err = pci_register_driver(&gb_driver);
	if (err) {
		gb_printf(KERN_ERR, "failed to register the PCI driver\n");
		return err;
	}

	gb_printf(KERN_INFO, "PCI driver registered\n");
	err = GB02FUNC165();
	if (err)
		gb_printf(KERN_ERR, "easy shell init failed");
	return 0;
}

static void GB02FUNC606(void)
{
	pci_unregister_driver(&gb_driver);

	gb_printf(KERN_INFO, "PCI driver unregistered\n");
}

module_init(GB02FUNC605);
module_exit(GB02FUNC606);
MODULE_AUTHOR("GB Project Developers");
MODULE_DESCRIPTION("GB DRM Driver");
MODULE_LICENSE("GPL v2");
MODULE_DEVICE_TABLE(pci, gb_ids);
