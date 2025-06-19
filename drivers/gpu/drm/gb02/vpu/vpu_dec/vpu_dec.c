// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * jessy 20221019 Sietium
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
/* obviously, for kmalloc */
#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>
#include <linux/moduleparam.h>
/* request_irq(), free_irq() */
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
/* needed for virt_to_phys() */
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <drm/drm_gem.h>
#include "common/gb_pcie_info.h"
#include "gpu/gb_device.h"
#include "vpu/vpu_comm/vpu_list.h"
#include "vpu/vpu_comm/vpu_vcmd_registers.h"
#include "vpu_vcmd.h"
#include "vpu_subsys.h"
#include "vpu/vpu_comm/vpu_afe.h"
#include "vpu_dec.h"
#include "vpu/vpu_comm/vpu_comm.h"
#include "vpu/vpu_comm/vpu_dwl_defs.h"
#include "vpu_mm.h"
#include "vpu/vpu_comm/gb_vpu.h"
#include "vpu/vpu_dec/vpu_dec_irq.h"
#include "common/gb_kernel_ver.h"
//#include "vpu/vpu_mm/gb_vpu_ttm.h"
unsigned long multicorebase_actual[GB02MAC12];

unsigned long multicorebase[GB02MAC12] = {
	GB02MAC220,
	GB02MAC221,
	0,
	0
};
int irq[GB02MAC12] = {
	GB02MAC224,
	GB02MAC225,
	-1,
	-1
};
unsigned int iosize[GB02MAC12] = {
	GB02MAC222,
	GB02MAC223,
	0,
	0
};
atomic_t irq_rx = ATOMIC_INIT(0);
atomic_t irq_tx = ATOMIC_INIT(0);

/* spinlock_t owner_lock = SPIN_LOCK_UNLOCKED; */
DEFINE_SPINLOCK(owner_lock);
DECLARE_WAIT_QUEUE_HEAD(dec_wait_queue);
DECLARE_WAIT_QUEUE_HEAD(pp_wait_queue);
DECLARE_WAIT_QUEUE_HEAD(hw_queue);

hantrodec_t hantrodec_data; /* dynamic allocation? */
struct GB02STR31 *dec_private_data;

unsigned long *GB02FUNC409(void)
{
	return multicorebase;
}

int *GB02FUNC411(void)
{
	return irq;
}

unsigned int *GB02FUNC414(void)
{
	return iosize;
}

static char *CoreTypeStr(enum core_type ct)
{
	switch (ct) {
		GB02MAC234(HW_VC8000D);
		GB02MAC234(HW_VC8000DJ);
		GB02MAC234(HW_BIGOCEAN);
		GB02MAC234(HW_VCMD);
		GB02MAC234(HW_MMU);
		GB02MAC234(HW_MMU_WR);
		GB02MAC234(HW_DEC400);
		GB02MAC234(HW_L2CACHE);
		GB02MAC234(HW_SHAPER);
		GB02MAC234(HW_AXIFE);
		GB02MAC234(HW_AFBC);
	default:
		return "Invalid core type";
		break;
	}
}

static int CheckHwId(hantrodec_t *dev)
{
	int hwid;
	int i, j;
	int DecHwId[] = {0x6731, 0x6732, 0xB16D, 0x8001};
	size_t num_hw = sizeof(DecHwId) / sizeof(*DecHwId);
	int found = 0;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	for (i = 0; i < dev->cores; i++) {
		for (j = 0; j < HW_CORE_MAX; j++) {
			if ((j == HW_VC8000D || j == HW_BIGOCEAN
			 || j == HW_VC8000DJ)
			  && dev->hwregs[i][j] != NULL) {
				hwid = readl(dev->hwregs[i][j]);
				gb_printf(KERN_INFO, "%s: core %d:%d HW ID=0x%08x [%s]\n",
				 __func__, i, j, hwid, CoreTypeStr(j));
				hwid = (hwid >> 16) & 0xFFFF;
				while (num_hw--) {
					if (hwid == DecHwId[num_hw]) {
						gb_printf(KERN_INFO, "%s: HW found 0x%16lx\n",
						 __func__, dec_data->vpu_subsys[i].base_addr
						  + dec_data->vpu_subsys[i].submodule_offset[j]);
						found++;
						dev->hw_id[i][j] = hwid;
						break;
					}
				}
				if (!found) {
					gb_printf(KERN_ERR, "hantrodec: Unknown HW found at 0x%16lx\n",
					multicorebase_actual[i]);
					return 0;
				}
				found = 0;
				num_hw = sizeof(DecHwId) / sizeof(*DecHwId);
			}
		}
	}
	return 1;
}

static void ReleaseIO(void)
{
	int i, j;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	for (i = 0; i < hantrodec_data.cores; i++) {
		for (j = 0; j < HW_CORE_MAX; j++) {
			if (hantrodec_data.hwregs[i][j]) {
				iounmap((void *) hantrodec_data.hwregs[i][j]);
				release_mem_region(dec_data->vpu_subsys[i].base_addr
				 + dec_data->vpu_subsys[i].submodule_offset[j],
				  dec_data->vpu_subsys[i].submodule_iosize[j]);
				hantrodec_data.hwregs[i][j] = 0;
			}
		}
	}
}

static void GB02FUNC433(int i, int j)
{
	int hwid;
	u32  axife_config;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();
	hwid = ioread32((void *)(hantrodec_data.hwregs[i][j]
	 + GB02MAC461));
	axife_config = ioread32((void *)(hantrodec_data.hwregs[i][j]));
	dec_data->GB02STR25[i].axi_rd_chn_num = axife_config & 0x7F;
	dec_data->GB02STR25[i].axi_wr_chn_num = (axife_config >> 7) & 0x7F;
	dec_data->GB02STR25[i].axi_rd_burst_length =
	 (axife_config >> 14) & 0x1F;
	dec_data->GB02STR25[i].axi_wr_burst_length =
	 (axife_config >> 22) & 0x1F;
	dec_data->GB02STR25[i].fe_mode = 0;
	if (hwid == 0x1F66)
		dec_data->GB02STR25[i].fe_mode = 1;
}

void GB02FUNC438(int i, int j, struct GB02STR31 *dec_data)
{
	dec_data->GB02STR24[i][j].nbr_mask_regs = GB02MAC230;
	dec_data->GB02STR24[i][j].num_mode = GB02MAC231;
	dec_data->GB02STR24[i][j].mask_reg_offset = GB02MAC232;
	dec_data->GB02STR24[i][j].mask_bits_per_reg =
						GB02MAC233;
	dec_data->GB02STR24[i][j].page_sel_addr =
		dec_data->GB02STR24[i][j].mask_reg_offset +
		dec_data->GB02STR24[i][j].nbr_mask_regs * 4;
}

void GB02FUNC440(int i, int j, struct GB02STR31 *dec_data)
{
	dec_data->GB02STR24[i][j].nbr_mask_regs = GB02MAC226;
	dec_data->GB02STR24[i][j].num_mode = GB02MAC227;
	dec_data->GB02STR24[i][j].mask_reg_offset =
					GB02MAC228;
	dec_data->GB02STR24[i][j].mask_bits_per_reg =
					GB02MAC229;
	dec_data->GB02STR24[i][j].page_sel_addr =
			dec_data->GB02STR24[i][j].mask_reg_offset +
			dec_data->GB02STR24[i][j].nbr_mask_regs * 4;
}

static int GB02FUNC444(int i, int j)
{
	int hwid;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();
	if (hantrodec_data.hwregs[i][j] == NULL) {
		gb_printf(KERN_ERR, "hantrodec: failed to ioremap HW %d regs\n", j);
#if 0
		release_mem_region(dec_data->vpu_subsys[i].base_addr
			+ dec_data->vpu_subsys[i].submodule_offset[j],
			dec_data->vpu_subsys[i].submodule_iosize[j]);
#endif
		return -1;
	}
	if (dec_data->vpu_subsys[i].has_apbfilter[j]) {
		dec_data->GB02STR24[i][j].has_apbfilter = 1;
		hwid = ioread32((void *)(hantrodec_data.hwregs[i][HW_VC8000D]));
		gb_printf(KERN_INFO, "%s-%d: vc800d %d, hwid=%d, hwregsaddr=%pK\n",
		 __func__, __LINE__, HW_VC8000D, hwid,
		  hantrodec_data.hwregs[i][HW_VC8000D]);

		if (IS_BIGOCEAN(hwid & 0xFFFF)) {
			if (j == HW_AXIFE)
				GB02FUNC438(i, j, dec_data);
		} else {
			hwid = ioread32((void *)
			(hantrodec_data.hwregs[i][HW_VC8000D]
				+ GB02MAC461));
			if (hwid == 0x1F58) {
				if (j == HW_VC8000D)
					GB02FUNC440(i, j,
					 dec_data);
				if (j == HW_AXIFE)
					GB02FUNC438(i, j,
						dec_data);
			} else if (hwid == 0x1F59) {
				if (j == HW_AXIFE)
					GB02FUNC438(i, j,
						dec_data);
			} else
				gb_printf(KERN_INFO, "dec: APBFILTER can read\n");
		}
		hantrodec_data.apbfilter_hwregs[i][j] =
			hantrodec_data.hwregs[i][j] +
			dec_data->GB02STR24[i][j].mask_reg_offset;
	} else
		dec_data->GB02STR24[i][j].has_apbfilter = 0;

	if (j == HW_AXIFE)
		GB02FUNC433(i, j);
	return 0;
}
static int ReserveIO(void)
{
	int i, j, ret;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	memcpy(multicorebase_actual, multicorebase,
	 GB02MAC12 * sizeof(unsigned long));
	memcpy((unsigned int *)(hantrodec_data.iosize),
	 iosize, GB02MAC12 * sizeof(unsigned int));
	memcpy((unsigned int *)(hantrodec_data.irq),
	 irq, GB02MAC12 * sizeof(int));

	for (i = 0; i < GB02MAC9; i++) {
		if (!dec_data->vpu_subsys[i].base_addr)
			continue;

		for (j = 0; j < HW_CORE_MAX; j++) {
			if (dec_data->vpu_subsys[i].submodule_iosize[j]) {
				gb_printf(KERN_INFO, "%s: base=0x%16lx, iosize=%d\n",
				 __func__, dec_data->vpu_subsys[i].base_addr
				  + dec_data->vpu_subsys[i].submodule_offset[j],
				   dec_data->vpu_subsys[i].submodule_iosize[j]);

				hantrodec_data.hwregs[i][j] =
				(volatile u8 *)dec_data->dec_pcie.vpu_vaddr_base +
				dec_data->vpu_subsys[i].submodule_offset[j];

				dec_data->vpu_subsys[i].submodule_hwregs[j]
				 = hantrodec_data.hwregs[i][j];
				gb_printf(KERN_INFO, "%s-%d: base=%lx,off=%x,vaddr=%pK\n",
				 __func__, __LINE__,
				  dec_data->vpu_subsys[i].base_addr,
				   dec_data->vpu_subsys[i].submodule_offset[j],
				    hantrodec_data.hwregs[i][j]);
				ret = GB02FUNC444(i, j);
				if (ret != 0) {
					gb_printf(KERN_ERR, "%s: error dec_hwregs ret %d\n",
					 __func__, j);
					return -1;
				}
				dec_data->config.its_main_core_id[i] = -1;
				dec_data->config.its_aux_core_id[i] = -1;

				gb_printf(KERN_INFO, "%s: decHW %d reg[0] = 0x%08x [%s]\n",
				 __func__, j,
				  readl(hantrodec_data.hwregs[i][j]),
				   CoreTypeStr(j));
			} else
				hantrodec_data.hwregs[i][j] = NULL;
		}
		hantrodec_data.cores++;
	}

	/* check for correct HW */
	if (!CheckHwId(&hantrodec_data)) {
		ReleaseIO();
		return -1;
	}
	return 0;
}

#define GB02MAC621(cmd) case (cmd): return(#cmd + 3)

static char *IoctlCmdStr(unsigned int cmd)
{
	switch (cmd) {
	GB02MAC621(GBDEC_IOC_MC_CORES);
	GB02MAC621(GBDEC_IOCGHWOFFSET);
	GB02MAC621(GBDEC_IOCGHWIOSIZE);
	GB02MAC621(GBDEC_IOC_MC_OFFSETS);
	GB02MAC621(GBDEC_IOCS_DEC_PUSH_REG);
	GB02MAC621(GBDEC_IOCS_DEC_PULL_REG);
	GB02MAC621(GBDEC_IOCT_DEC_RELEASE);
	GB02MAC621(GBDEC_IOCG_CORE_WAIT);
	GB02MAC621(GBDEC_IOX_ASIC_ID);
	GB02MAC621(GBDEC_IOCG_CORE_ID);
	GB02MAC621(GBDEC_IOCS_DEC_WRITE_REG);
	GB02MAC621(GBDEC_IOCS_DEC_READ_REG);
	GB02MAC621(GBDEC_IOX_ASIC_BUILD_ID);
	GB02MAC621(GBDEC_IOX_SUBSYS);
	GB02MAC621(GBDEC_DEBUG_STATUS);

 	GB02MAC621(HANTRO_VCMD_IOCH_GET_HWINFO_FROM_VCMD);
  	//GB02MAC621(HANTRO_VCMD_IOCH_GET_CMDBUF_POOL_SIZE);
  	//GB02MAC621(HANTRO_VCMD_IOCH_SET_CMDBUF_POOL_BASE);
	/* AXI FE / APB filter */
	GB02MAC621(GBDEC_IOC_AXIFE_CONFIG);
	default:
		return "Invalid ioctl cmd";
	}
}

long DecFlushRegs(hantrodec_t *dev, struct GB02STR27 *core)
{
	long ret = 0, i;
	u32 id = core->id;
	u32 type = core->type;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	gb_printf(KERN_INFO, "%s: start,id = %d, type:%d, size:%d, reg_id:%d\n", __func__,
		core->id, core->type, core->size, core->reg_id);

	if (type == HW_VC8000D
		&& !dec_data->vpu_subsys[id].submodule_hwregs[type])
		type = HW_VC8000DJ;
	if (dev->client_type[id] == GB02MAC211)
		type = HW_BIGOCEAN;

	if (id >= GB02MAC9 || !dec_data->vpu_subsys[id].base_addr
		|| core->type >= HW_CORE_MAX
		|| !dec_data->vpu_subsys[id].submodule_hwregs[type])
		return -EINVAL;

	gb_printf(KERN_INFO, "%s: submodule_iosize = %d\n", __func__,
		dec_data->vpu_subsys[id].submodule_iosize[type]);

	ret = copy_from_user(dec_data->dec_regs[id], core->regs,
		dec_data->vpu_subsys[id].submodule_iosize[type]);
	if (ret) {
		gb_printf(KERN_ERR, "%s:copy_from_user failed, returned %li\n",
		 __func__, ret);
		return -EFAULT;
	}

	if (type == HW_VC8000D || type == HW_BIGOCEAN || type == HW_VC8000DJ) {
		/* write all regs but the status reg[1] to hardware */
		if (dec_data->reg_access_opt) {
			for (i = 3;
			 i < dec_data->vpu_subsys[id].submodule_iosize[type]
			  / 4; i++) {
				/* check whether register value is updated. */
				if (dec_data->dec_regs[id][i] !=
				 dec_data->shadow_dec_regs[id][i]) {
					iowrite32(dec_data->dec_regs[id][i],
					 (void *)(dev->hwregs[id][type]
					  + i * 4));
					dec_data->shadow_dec_regs[id][i] =
					 dec_data->dec_regs[id][i];
				}
			}
		} else
			for (i = 3;
			 i < dec_data->vpu_subsys[id].submodule_iosize[type]
			  / 4; i++)
				iowrite32(dec_data->dec_regs[id][i],
				 (void *)(dev->hwregs[id][type] +
				  i * 4));

		iowrite32(dec_data->dec_regs[id][2],
		 (void *)(dev->hwregs[id][type] + 8));
		dec_data->shadow_dec_regs[id][2] = dec_data->dec_regs[id][2];

		iowrite32(dec_data->dec_regs[id][1],
		 (void *)(dev->hwregs[id][type] + 4));
		dec_data->shadow_dec_regs[id][1] = dec_data->dec_regs[id][1];

		gb_printf(KERN_INFO, "%s: flushed registers core id = %d\n", __func__, id);
	} else {
		for (i = 0;
		 i < dec_data->vpu_subsys[id].submodule_iosize[type] / 4; i++)
			iowrite32(dec_data->dec_regs[id][i],
				(void *)(dev->hwregs[id][type] + i * 4));
	}
	return 0;
}

long DecWriteRegs(hantrodec_t *dev, struct GB02STR27 *core)
{
	long ret = 0;
	u32 i = core->reg_id;
	u32 id = core->id;
	u32 type = core->type;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	gb_printf(KERN_INFO, "%s: start,id = %d, type:%d-[%s], size:%d, reg_id:%d\n",
	 __func__, core->id, core->type, CoreTypeStr(core->type),
	  core->size, core->reg_id);

	if (type == HW_VC8000D &&
	 !dec_data->vpu_subsys[id].submodule_hwregs[type])
		type = HW_VC8000DJ;
	if (dev->client_type[id] == GB02MAC211)
		type = HW_BIGOCEAN;

	if (id >= GB02MAC9 || !dec_data->vpu_subsys[id].base_addr
	 || (core->size & 0x3) || type >= HW_CORE_MAX ||
	  !dec_data->vpu_subsys[id].submodule_hwregs[type]
	   || core->reg_id * 4 + core->size >
	    dec_data->vpu_subsys[id].submodule_iosize[type])
		return -EINVAL;

	ret = copy_from_user(dec_data->dec_regs[id], core->regs, core->size);
	if (ret) {
		gb_printf(KERN_ERR, "%s:copy_from_user failed, returned %li\n",
		 __func__, ret);
		return -EFAULT;
	}

	for (i = core->reg_id; i < core->reg_id + core->size/4; i++) {
		gb_printf(KERN_INFO, "%s: write %08x to reg[%d] core %d\n", __func__,
		 dec_data->dec_regs[id][i-core->reg_id], i, id);
		iowrite32(dec_data->dec_regs[id][i-core->reg_id],
		 (void *)(dev->hwregs[id][type] + i * 4));
		if (type == HW_VC8000D)
			dec_data->shadow_dec_regs[id][i] =
			 dec_data->dec_regs[id][i-core->reg_id];
	}
	return 0;
}

long DecWriteApbFilterRegs(hantrodec_t *dev, struct GB02STR27 *core)
{
	long ret = 0;
	u32 i = core->reg_id;
	u32 id = core->id;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	gb_printf(KERN_INFO, "%s: start,id:%d, type:%d-[%s], size:%d, reg_id:%d\n", __func__,
	 core->id, core->type, CoreTypeStr(core->type),
	  core->size, core->reg_id);

	if (id >= GB02MAC9 || !dec_data->vpu_subsys[id].base_addr
	 || core->type >= HW_CORE_MAX
	  || !dec_data->vpu_subsys[id].submodule_hwregs[core->type]
	   || (core->size & 0x3) || core->reg_id * 4 + core->size >
	    dec_data->vpu_subsys[id].submodule_iosize[core->type] + 4)
		return -EINVAL;

	ret = copy_from_user(dec_data->apbfilter_regs[id],
	 core->regs, core->size);
	if (ret) {
		gb_printf(KERN_ERR, "%s:copy_from_user failed, returned %li\n",
		 __func__, ret);
		return -EFAULT;
	}

	for (i = core->reg_id; i < core->reg_id + core->size / 4; i++) {
		gb_printf(KERN_INFO, "hantrodec: write %08x to reg[%d] core %d\n",
			dec_data->dec_regs[id][i-core->reg_id], i, id);
		iowrite32(dec_data->apbfilter_regs[id][i-core->reg_id],
			(void *)(dev->apbfilter_hwregs[id][core->type]
			 + i * 4));
	}
	return 0;
}

long DecRefreshRegs(hantrodec_t *dev, struct GB02STR27 *core)
{
	long ret, i;
	u32 id = core->id;
	u32 type = core->type;
	u16 submodule_iosize;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();
	submodule_iosize = dec_data->vpu_subsys[id].submodule_iosize[type];
	gb_printf(KERN_INFO, "%s: start,id:%d, type:%d-[%s], size:%d, reg_id:%d\n",
	 __func__, core->id, core->type, CoreTypeStr(core->type),
	  core->size, core->reg_id);

	if (type == HW_VC8000D &&
	 !dec_data->vpu_subsys[id].submodule_hwregs[type])
		type = HW_VC8000DJ;
	if (dev->client_type[id] == GB02MAC211)
		type = HW_BIGOCEAN;

	if (id >= GB02MAC9 || !dec_data->vpu_subsys[id].base_addr
		|| type >= HW_CORE_MAX
		|| !dec_data->vpu_subsys[id].submodule_hwregs[type])
		return -EINVAL;

	gb_printf(KERN_INFO, "%s: submodule_iosize = %d\n", __func__, submodule_iosize);

	if (!dec_data->reg_access_opt) {
		for (i = 0; i < submodule_iosize / 4; i++)
			dec_data->dec_regs[id][i] =
			 ioread32((void *)(dev->hwregs[id][type] + i * 4));
	} else {
		// only need to read swreg1,62(?),63,168,169
		#define REFRESH_REG(idx) i = (idx);\
			       	dec_data->shadow_dec_regs[id][i] =\
			       	dec_data->dec_regs[id][i] =\
			       	ioread32((void *)(dev->hwregs[id][type] + i * 4))
		REFRESH_REG(0);
		REFRESH_REG(1);
		REFRESH_REG(62);
		REFRESH_REG(63);
		REFRESH_REG(168);
		REFRESH_REG(169);
		#undef REFRESH_REG
	}

	ret = copy_to_user(core->regs, dec_data->dec_regs[id],
	 submodule_iosize);
	if (ret) {
		gb_printf(KERN_ERR, "%s:copy_to_user failed, returned %li\n", __func__, ret);
		return -EFAULT;
	}
	return 0;
}

long DecReadRegs(hantrodec_t *dev, struct GB02STR27 *core)
{
	long ret;
	u32 id = core->id;
	u32 i = core->reg_id;
	u32 type = core->type;
	u16 submodule_iosize;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();
	submodule_iosize = dec_data->vpu_subsys[id].submodule_iosize[type];
	gb_printf(KERN_INFO, "%s: start,id:%d, type:%d, size:%d, reg_id:%d\n", __func__,
		core->id, core->type, core->size, core->reg_id);

	if (type == HW_VC8000D
		&& !dec_data->vpu_subsys[id].submodule_hwregs[type])
		type = HW_VC8000DJ;
	if (dev->client_type[id] == GB02MAC211)
		type = HW_BIGOCEAN;

	if (id >= GB02MAC9 || !dec_data->vpu_subsys[id].base_addr
		|| type >= HW_CORE_MAX
		|| !dec_data->vpu_subsys[id].submodule_hwregs[type]
		|| (core->size & 0x3)
		|| core->reg_id * 4 + core->size > submodule_iosize)
		return -EINVAL;

	/* read specific registers from hardware */
	for (i = core->reg_id; i < core->reg_id + core->size/4; i++) {
		dec_data->dec_regs[id][i-core->reg_id] =
			ioread32((void *)(dev->hwregs[id][type] + i * 4));
		gb_printf(KERN_INFO, "%s: read %08x from reg[%d] core %d\n", __func__,
			dec_data->dec_regs[id][i-core->reg_id], i, id);
		if (type == HW_VC8000D)
			dec_data->shadow_dec_regs[id][i] =
			 dec_data->dec_regs[id][i];
	}

	/* put registers to user space*/
	ret = copy_to_user(core->regs, dec_data->dec_regs[id], core->size);
	if (ret) {
		gb_printf(KERN_ERR, "%s:copy_to_user failed, returned %li\n", __func__, ret);
		return -EFAULT;
	}
	return 0;
}

static int CoreHasFormat(const u32 *cfg, int core, u32 format)
{
	return (cfg[core] & (1 << format)) ? 1 : 0;
}

int GetDecCore(long core, hantrodec_t *dev,
	struct drm_file *filp, unsigned long format)
{
	int success = 0;
	unsigned long flags;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	spin_lock_irqsave(&owner_lock, flags);
	if (CoreHasFormat(dec_data->config.cfg, core, format)
		&& dec_data->dec_owner[core] == NULL) {
		dec_data->dec_owner[core] = filp;
		success = 1;

		if (dec_data->config.its_aux_core_id[core] >= 0
			&& !CoreHasFormat(dec_data->config.cfg,
				dec_data->config.its_aux_core_id[core], format))
			dec_data->config.cfg[dec_data->config.its_aux_core_id[core]]
			 = 0;
		else if (dec_data->config.its_main_core_id[core] >= 0)
			dec_data->config.cfg[dec_data->config.its_main_core_id[core]]
			 = dec_data->config.cfg[core];
	}
	spin_unlock_irqrestore(&owner_lock, flags);

	return success;
}

int GetDecCoreAny(long *core, hantrodec_t *dev,
	struct drm_file *filp, unsigned long format)
{
	int success = 0;
	long c;

	*core = -1;
	for (c = 0; c < dev->cores; c++) {
		/* a free core that has format */
		if (GetDecCore(c, dev, filp, format)) {
			success = 1;
			*core = c;
			break;
		}
	}
	return success;
}

long GB02FUNC516(hantrodec_t *dev, struct drm_file *filp,
	unsigned long format)
{
	long core = -1;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	/* reserve a core */
	if (down_interruptible(&dec_data->dec_core_sem))
		return -ERESTARTSYS;

	/* lock a core that has specific format*/
	if (wait_event_interruptible(hw_queue,
		GetDecCoreAny(&core, dev, filp, format) != 0))
		return -ERESTARTSYS;

	dev->client_type[core] = format;
	return core;
}

void ReleaseDecoder(hantrodec_t *dev, long core)
{
	u32 status;
	unsigned long flags;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	gb_printf(KERN_INFO, "%s: start Release Decoder core = %ld\n", __func__, core);

	if (dev->client_type[core] == GB02MAC211)
		status = ioread32((void *)(dev->hwregs[core][HW_BIGOCEAN]
			+ GB02MAC413));
	else
		status = ioread32((void *)(dev->hwregs[core][HW_VC8000D]
			+ GB02MAC409));

	/* make sure HW is disabled */
	if (status & GB02MAC440) {
		gb_printf(KERN_INFO, "%s: DEC[%li] still enabled -> reset\n",
		 __func__, core);

		/* abort decoder */
		status |= GB02MAC442 | GB02MAC443;
		iowrite32(status, (void *)(dev->hwregs[core][HW_VC8000D]
		 + GB02MAC409));
	}

	spin_lock_irqsave(&owner_lock, flags);

	/* If aux core released, revert main core's config back */
	if (dec_data->config.its_main_core_id[core] >= 0)
		dec_data->config.cfg[dec_data->config.its_main_core_id[core]] =
		dec_data->config.cfg_backup[dec_data->config.its_main_core_id[core]];

	/* If main core released, revert aux core's config back */
	if (dec_data->config.its_aux_core_id[core] >= 0)
		dec_data->config.cfg[dec_data->config.its_aux_core_id[core]] =
		dec_data->config.cfg_backup[dec_data->config.its_aux_core_id[core]];

	dec_data->dec_owner[core] = NULL;
	spin_unlock_irqrestore(&owner_lock, flags);
	up(&dec_data->dec_core_sem);

	wake_up_interruptible_all(&hw_queue);
}

static int CheckDecIrq(hantrodec_t *dev, int id)
{
	unsigned long flags;
	int rdy = 0;
	const u32 irq_mask = (1 << id);
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	spin_lock_irqsave(&owner_lock, flags);

	if (dec_data->dec_irq & irq_mask) {
		/* reset the wait condition(s) */
		dec_data->dec_irq &= ~irq_mask;
		rdy = 1;
	}

	spin_unlock_irqrestore(&owner_lock, flags);
	return rdy;
}

long WaitDecReadyAndRefreshRegs(hantrodec_t *dev, struct GB02STR27 *core)
{
	u32 id = core->id;
	long ret;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	gb_printf(KERN_INFO, "%s: start DEC id = %d\n", __func__, id);

	ret = wait_event_interruptible(dec_wait_queue, CheckDecIrq(dev, id));
	if (ret) {
		gb_printf(KERN_ERR, "%s: error interrupted DEC id = %d\n", __func__, id);
		return -ERESTARTSYS;
	}
	atomic_inc(dec_data->irq_tx);
	/* refresh registers */
	return DecRefreshRegs(dev, core);
}

static int GB02FUNC529(hantrodec_t *dev,
		const struct drm_file *filp, int *id)
{
	unsigned long flags;
	int rdy = 0, n = 0;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	do {
		u32 irq_mask = (1 << n);

		spin_lock_irqsave(&owner_lock, flags);
		if (dec_data->dec_irq & irq_mask) {
			if (dec_data->dec_owner[n] == filp) {
				/* we have an IRQ for our client */
				/* reset the wait condition(s) */
				dec_data->dec_irq &= ~irq_mask;

				/* signal ready core no. for our client */
				*id = n;
				rdy = 1;
				spin_unlock_irqrestore(&owner_lock, flags);
				break;
			} else if (dec_data->dec_owner[n] == NULL) {
				/* zombie IRQ */
				gb_printf(KERN_INFO, "%s: IRQ on core[%d], but no owner\n",
				 __func__, n);

				/* reset the wait condition(s) */
				dec_data->dec_irq &= ~irq_mask;
			}
		}

		spin_unlock_irqrestore(&owner_lock, flags);
		n++;
	} while (n < dev->cores);
	return rdy;
}

long GB02FUNC535(hantrodec_t *dev, const struct drm_file *filp, int *id)
{
	long ret;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	gb_printf(KERN_INFO, "%s: WaitCore start DEC *id = %d\n", __func__, *id);
	ret = wait_event_interruptible(dec_wait_queue,
	 GB02FUNC529(dev, filp, id));
	if	(ret) {
		gb_printf(KERN_ERR, "%s:error id: %d interrupt ret:0x%lx\n",
		 __func__, *id, ret);
		return -ERESTARTSYS;
	}
	atomic_inc(dec_data->irq_tx);
	return 0;
}

int GB02FUNC538(hantrodec_t *dev, struct drm_file *filp,
		unsigned long format)
{
	long c;
	unsigned long flags;
	int core_id = -1;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	for (c = 0; c < dev->cores; c++) {
		/* a core that has format */
		spin_lock_irqsave(&owner_lock, flags);
		if (CoreHasFormat(dec_data->config.cfg, c, format)) {
			core_id = c;
			spin_unlock_irqrestore(&owner_lock, flags);
			break;
		}
		spin_unlock_irqrestore(&owner_lock, flags);
	}
	return core_id;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18))
irqreturn_t hantrodec_isr(int irq, void *dev_id, struct pt_regs *regs)
#else
irqreturn_t hantrodec_isr(int irq, void *dev_id)
#endif
{
	unsigned long flags;
	unsigned int handled = 0;
	int i;
	u8 *hwregs;
	hantrodec_t *dev = (hantrodec_t *) dev_id;
	u32 irq_status_dec;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	spin_lock_irqsave(&owner_lock, flags);

	for (i = 0; i < dev->cores; i++) {
		volatile u8 *hwregs = dev->hwregs[i][HW_VC8000D];

		/* interrupt status register read */
		irq_status_dec = ioread32((void *)(hwregs
		 + GB02MAC409));

		if (irq_status_dec & GB02MAC444) {
			/* clear dec IRQ */
			irq_status_dec &= (~GB02MAC444);
			iowrite32(irq_status_dec, (void *)(hwregs
				+ GB02MAC409));
			gb_printf(KERN_INFO, "%s: decoder IRQ received! core %d\n",
			 __func__, i);

			atomic_inc(dec_data->irq_rx);
			dec_data->dec_irq |= (1 << i);
			wake_up_interruptible_all(&dec_wait_queue);
			handled++;
		}
	}

	spin_unlock_irqrestore(&owner_lock, flags);

	if (!handled)
		gb_printf(KERN_ERR, "%s: IRQ received, but not hantrodec's!\n", __func__);

	(void)hwregs;
	return IRQ_RETVAL(handled);
}

long GB02FUNC549(struct drm_file *filp,
	unsigned int cmd, void *arg)
{
	int err = 0;
	long tmp;
	struct GB02STR31 *dec_data = NULL;

	dec_data = GB02FUNC589();

	gb_printf(KERN_INFO, "%s:ioctl cmd: 0x%08x [%s]\n", __func__, cmd, IoctlCmdStr(cmd));

	if (_IOC_TYPE(cmd) != GB02MAC918 &&
	_IOC_TYPE(cmd) != GB02MAC236)
		return -ENOTTY;
	if ((_IOC_TYPE(cmd) == GB02MAC918 &&
	_IOC_NR(cmd) > GB02MAC919) ||
	(_IOC_TYPE(cmd) == GB02MAC236 &&
	_IOC_NR(cmd) > GB02MAC237))
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !gb_access_ok(VERIFY_WRITE, (void *) arg, _IOC_SIZE(cmd));
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !gb_access_ok(VERIFY_READ, (void *) arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {
		case GBDEC_IOCGHWOFFSET: {
			__u32 id;

			__get_user(id, (__u32 *)arg);
			if (id >= hantrodec_data.cores)
				return -EFAULT;
			__put_user(multicorebase_actual[id],
			 (unsigned long *) arg);
		break;
	}
	case GBDEC_IOCGHWIOSIZE: {
		struct GB02STR26 core;
		unsigned long ret;

		/* get registers from user space*/
		tmp = copy_from_user(&core, (void *)arg,
		 sizeof(struct GB02STR26));
		if (tmp) {
			gb_printf(KERN_ERR, "%s:copy_from_user error, return %li\n",
			 __func__, tmp);
			return -EFAULT;
		}

		if (core.id >= GB02MAC9 /*hantrodec_data.cores*/)
			return -EFAULT;

		if (core.type == HW_SHAPER) {
			u32 asic_id;
			/* Shaper is configured with l2cache. */
			if (dec_data->vpu_subsys[core.id].submodule_hwregs[HW_L2CACHE]) {
				asic_id =
				 ioread32((void *)dec_data->vpu_subsys[core.id].submodule_hwregs[HW_L2CACHE]);
				switch ((asic_id >> 16) & 0x3) {
				case 1: /* cache only */
					core.size = 0; break;
				case 0: /* cache + shaper */
				case 2: /* shaper only*/
					core.size =
					 dec_data->vpu_subsys[core.id].submodule_iosize[HW_L2CACHE];
					break;
				default:
					return -EFAULT;
				}
			} else
				core.size = 0;
		} else {
			core.size =
			 dec_data->vpu_subsys[core.id].submodule_iosize[core.type];
			if (core.type == HW_VC8000D && !core.size &&
			 dec_data->vpu_subsys[core.id].submodule_hwregs[HW_VC8000DJ]) {
				core.size =
				 dec_data->vpu_subsys[core.id].submodule_iosize[HW_VC8000DJ];
			}
		}
		ret = copy_to_user((u32 *) arg, &core,
		 sizeof(struct GB02STR26));

		return ret;
	}
	case GBDEC_IOC_MC_OFFSETS: {
		tmp = copy_to_user((unsigned long *) arg,
			multicorebase_actual, sizeof(multicorebase_actual));
		if (err) {
			gb_printf(KERN_ERR, "%s:copy_to_user error, return %li\n",
			 __func__, tmp);
			return -EFAULT;
		}
		break;
	}
	case GBDEC_IOC_MC_CORES:
		__put_user(hantrodec_data.cores, (unsigned int *) arg);
		gb_printf(KERN_ERR, "%s: put_user cores=%d\n", __func__,
		 hantrodec_data.cores);
		break;
	case GBDEC_IOCS_DEC_PUSH_REG: {
		struct GB02STR27 core;

		/* get registers from user space*/
		tmp = copy_from_user(&core, (void *)arg,
		 sizeof(struct GB02STR27));
		if (tmp) {
			gb_printf(KERN_ERR, "%s:copy_from_user error, return %li\n",
			 __func__, tmp);
			return -EFAULT;
		}

		return DecFlushRegs(&hantrodec_data, &core);
	}
	case GBDEC_IOCS_DEC_WRITE_REG: {
		struct GB02STR27 core;

		/* get registers from user space*/
		tmp = copy_from_user(&core, (void *)arg,
		 sizeof(struct GB02STR27));
		if (tmp) {
			gb_printf(KERN_ERR, "copy_from_user failed, returned %li\n", tmp);
			return -EFAULT;
		}

		return DecWriteRegs(&hantrodec_data, &core);
	}
	case GBDEC_IOCS_DEC_WRITE_APBFILTER_REG: {
	struct GB02STR27 core;
		/* get registers from user space*/
		tmp = copy_from_user(&core, (void *)arg,
		 sizeof(struct GB02STR27));
		if (tmp) {
			gb_printf(KERN_ERR, "copy_from_user error, returned %ld\n", tmp);
			return -EFAULT;
		}

		return DecWriteApbFilterRegs(&hantrodec_data, &core);
	}
	case GBDEC_IOCS_DEC_PULL_REG: {
		struct GB02STR27 core;

		/* get registers from user space*/
		tmp = copy_from_user(&core, (void *)arg,
		 sizeof(struct GB02STR27));
		if (tmp) {
			gb_printf(KERN_ERR, "%s:copy_from_user error, tmp = %ld\n",
			 __func__, tmp);
			return -EFAULT;
		}

		return DecRefreshRegs(&hantrodec_data, &core);
	}
	case GBDEC_IOCS_DEC_READ_REG: {
		struct GB02STR27 core;

		/* get registers from user space*/
		tmp = copy_from_user(&core, (void *)arg,
		 sizeof(struct GB02STR27));
		if (tmp) {
			gb_printf(KERN_ERR, "%s:copy_from_user error, return %ld\n",
			 __func__, tmp);
			return -EFAULT;
		}

		return DecReadRegs(&hantrodec_data, &core);
	}
	case GBDEC_IOCH_DEC_RESERVE: {
		u32 format = 0;

		__get_user(format, (unsigned long *)arg);
		gb_printf(KERN_INFO, "%s: Reserve DEC core, format = %i\n",
		 __func__, format);
		return GB02FUNC516(&hantrodec_data, filp, format);
	}
	case GBDEC_IOCT_DEC_RELEASE: {
		u32 core = 0;

		__get_user(core, (unsigned long *)arg);
		if (core >= hantrodec_data.cores
			|| dec_data->dec_owner[core] != filp) {
			gb_printf(KERN_ERR, "%s:bogus DEC release, core = %i\n",
			 __func__, core);
			return -EFAULT;
		}
		gb_printf(KERN_INFO, "%s: Release DEC, core = %i\n", __func__, core);

		ReleaseDecoder(&hantrodec_data, core);
		break;
	}
	case GBDEC_IOCG_CORE_WAIT: {
		int id;

		tmp = GB02FUNC535(&hantrodec_data, filp, &id);
		__put_user(id, (int *) arg);
		return tmp;
	}
	case GBDEC_IOX_ASIC_ID: {
		struct GB02STR29 core;
		unsigned long ret;

		/* get registers from user space*/
		tmp = copy_from_user(&core, (void *)arg,
		 sizeof(struct GB02STR29));
		if (tmp) {
			gb_printf(KERN_ERR, "%s:copy_from_user error, return %li\n",
			 __func__, tmp);
			return -EFAULT;
		}
		if (core.id >= GB02MAC9 /*hantrodec_data.cores*/
			|| ((core.type == HW_VC8000D
			|| core.type == HW_VC8000DJ)
			&& !dec_data->vpu_subsys[core.id].submodule_iosize[core.type
			 == HW_VC8000D]
			&& !dec_data->vpu_subsys[core.id].submodule_iosize[core.type
			 == HW_VC8000DJ])
			|| ((core.type != HW_VC8000D
			 && core.type != HW_VC8000DJ)
			 && !dec_data->vpu_subsys[core.id].submodule_iosize[core.type])) {
			return -EFAULT;
		}

		core.size =
		 dec_data->vpu_subsys[core.id].submodule_iosize[core.type];
		if (dec_data->vpu_subsys[core.id].submodule_hwregs[core.type])
			core.asic_id = ioread32((void *)
			hantrodec_data.hwregs[core.id][core.type]);
		else if (core.type == HW_VC8000D
			&& hantrodec_data.hwregs[core.id][HW_VC8000DJ])
			core.asic_id = ioread32((void *)
			hantrodec_data.hwregs[core.id][HW_VC8000DJ]);
		else
			core.asic_id = 0;
		ret = copy_to_user((u32 *) arg, &core,
		 sizeof(struct GB02STR29));

		return ret;
	}
	case GBDEC_IOCG_CORE_ID: {
		u32 format = 0;

		__get_user(format, (unsigned long *)arg);

		gb_printf(KERN_INFO, "%s: Get DEC Core_id, format = %i\n", __func__, format);
		return GB02FUNC538(&hantrodec_data, filp, format);
	}
	case GBDEC_IOX_ASIC_BUILD_ID: {
		u32 id, hw_id;

		__get_user(id, (u32 *)arg);

		if (id >= hantrodec_data.cores)
			return -EFAULT;
		if (hantrodec_data.hwregs[id][HW_VC8000D]
			|| hantrodec_data.hwregs[id][HW_VC8000DJ]) {
			volatile u8 *hwregs;

			/* VC8000D first if it exists, otherwise VC8000DJ. */
			if (hantrodec_data.hwregs[id][HW_VC8000D])
				hwregs = hantrodec_data.hwregs[id][HW_VC8000D];
			else
				hwregs = hantrodec_data.hwregs[id][HW_VC8000DJ];
			hw_id = ioread32((void *)hwregs);
			if (IS_G1(hw_id >> 16) || IS_G2(hw_id >> 16)
				|| (IS_VC8000D(hw_id >> 16)
				&& ((hw_id & 0xFFFF) == 0x6010)))
				__put_user(hw_id, (u32 *) arg);
			else {
				hw_id = ioread32((void *)
				(hwregs + GB02MAC461));
				__put_user(hw_id, (u32 *) arg);
			}
		} else if (hantrodec_data.hwregs[id][HW_BIGOCEAN]) {
			hw_id = ioread32((void *)
			(hantrodec_data.hwregs[id][HW_BIGOCEAN]));
			if (IS_BIGOCEAN(hw_id >> 16))
				__put_user(hw_id, (u32 *) arg);
			else
				return -EFAULT;
		}
		return 0;
	}
	case GBDEC_DEBUG_STATUS: {

		gb_printf(KERN_INFO, "DEC_DEBUG_STATUS, dec_irq = 0x%08x, pp_irq = 0x%08x\n",
			dec_data->dec_irq, dec_data->pp_irq);
		gb_printf(KERN_INFO, "DEC_DEBUG_STATUS, IRQs received/sent2user = %d/%d\n",
			atomic_read(dec_data->irq_rx),
			 atomic_read(dec_data->irq_tx));

		for (tmp = 0; tmp < hantrodec_data.cores; tmp++) {
			gb_printf(KERN_INFO, "hantrodec: dec_core[%li] %s\n", tmp,
				dec_data->dec_owner[tmp] == NULL ?
				 "FREE" : "RESERVED");
			gb_printf(KERN_INFO, "hantrodec: pp_core[%li] %s\n", tmp,
				dec_data->pp_owner[tmp] == NULL ?
				 "FREE" : "RESERVED");
		}
		return 0;
	}
	case GBDEC_IOX_SUBSYS: {
		struct GB02STR30 subsys = {0};
		unsigned long ret;
		/* TODO(min): check all the subsys */
		subsys.subsys_vcmd_num = 1;
		subsys.subsys_num = subsys.subsys_vcmd_num;
		ret = copy_to_user((u32 *) arg, &subsys,
		 sizeof(struct GB02STR30));
		return ret;
	}
	case GBDEC_IOC_AXIFE_CONFIG: {
		struct GB02STR25 tmp_axife;
		unsigned long ret;

		/* get registers from user space*/
		tmp = copy_from_user(&tmp_axife, (void *)arg,
		 sizeof(struct GB02STR25));
		if (tmp) {
			gb_printf(KERN_ERR, "AXIFE_CONFIG copy_from_user error, return %li\n",
			 tmp);
			return -EFAULT;
		}

		if (tmp_axife.id >= GB02MAC9)
			return -EFAULT;

		dec_data->GB02STR25[tmp_axife.id].id = tmp_axife.id;
		memcpy(&tmp_axife, &(dec_data->GB02STR25[tmp_axife.id]),
			sizeof(struct GB02STR25));
		ret = copy_to_user((u32 *) arg, &tmp_axife,
		 sizeof(struct GB02STR25));
		return ret;
	}
	default: {
		if (_IOC_TYPE(cmd) == GB02MAC236)
			return (GB02FUNC987(filp, cmd, arg));
		return -ENOTTY;
	}
	}

	return 0;
}

struct GB02STR31 *GB02FUNC589(void)
{
	return dec_private_data;
}

void GB02FUNC591(struct GB02STR31 *dec_data,
			   struct GB02STR69 *pci_bars,
			   struct GB02STR67 *dev_info,
			   struct GB02STR126 vpu_offset)
{
	int i = 0;

	i = GB02FUNC465(dev_info);
	dec_data->dec_pcie.vpu_paddr_base = pci_bars[i].base +
						vpu_offset.dec0_offset_base;
	dec_data->dec_pcie.vpu_vaddr_base = pci_bars[i].mmio +
						vpu_offset.dec0_offset_base;
	i = GB02FUNC468(dev_info);
	dec_data->dec_pcie.vram_vaddr_base = pci_bars[i].mmio;
	dec_data->dec_pcie.vram_paddr_base = pci_bars[i].base;

	dec_data->irq_rx = &irq_rx;
	dec_data->irq_tx = &irq_tx;
	dec_data->dec_irq = 0;
	dec_data->pp_irq = 0;
	dec_data->reg_access_opt = 0;
}

static void GB02FUNC593(struct GB02STR23 *subsys,
	struct GB02STR20 dec_pcie)
{
	int i;

	for (i = 0; i < GB02MAC9; i++) {
		if (subsys[i].base_addr == 0x1000)
			subsys[i].base_addr = dec_pcie.vpu_paddr_base;
		else {
			subsys[i].base_addr += dec_pcie.vpu_paddr_base;
			multicorebase[i] += dec_pcie.vpu_paddr_base;
		}

		gb_printf(KERN_INFO, "%s-%d: vpu subsys %d base addr =%lx\n",
		 __func__, __LINE__, i, subsys[i].base_addr);
	}
}
int GB02FUNC22(hantrodec_t *gbdec_data, void *pci_bars,
	struct GB02STR67 *dev_info, struct GB02STR3 *vcmd_pool,
	struct GB02STR3 *vcmd_reg, struct GB02STR126 vpu_offset)
{
	int result = 0, i;
	unsigned int vcmd = 0;
	unsigned long base_port = -1;
	int total_vcmd_core_num = 0;
	struct GB02STR31 *dec_data;
	u64 dec_paddr;

	dec_data =
	 (struct GB02STR31 *)vmalloc(sizeof(struct GB02STR31));
	if (dec_data == NULL) {
		gb_printf(KERN_ERR, "%s: struct GB02STR31 vmalloc error\n",
		 __func__);
		return -1;
	}
	gbdec_data = &hantrodec_data;

	GB02FUNC591(dec_data, (struct GB02STR69 *)pci_bars,
	 dev_info, vpu_offset);
	dec_private_data = dec_data;
	dec_paddr = dec_data->dec_pcie.vpu_paddr_base;
	CheckSubsysCoreArray(dec_data->vpu_subsys, &vcmd, &total_vcmd_core_num);
	GB02FUNC593(dec_data->vpu_subsys, dec_data->dec_pcie);

	if (base_port != -1) {
		multicorebase[0] = base_port;
		multicorebase[0] += GB02MAC220;
		dec_data->vpu_subsys[0].base_addr = base_port;
		gb_printf(KERN_INFO, "hantrodec: Init single core at %lu IRQ = %d\n",
			multicorebase[0], irq[0]);
	} else {
		gb_printf(KERN_INFO, "hantrodec: Init multi core[0] at %lu\n"
	       "                      core[1] at %lu\n"
	       "                      core[2] at %lu\n"
	       "                      core[3] at %lu\n"
	       "           IRQ_0 = %d\n"
	       "           IRQ_1 = %d\n",
	       multicorebase[0], multicorebase[1],
	       multicorebase[2], multicorebase[3],
	       irq[0], irq[1]);
	}
	hantrodec_data.cores = 0;
	hantrodec_data.iosize[0] = GB02MAC222;
	hantrodec_data.irq[0] = irq[0];
	hantrodec_data.iosize[1] = GB02MAC223;
	hantrodec_data.irq[1] = irq[1];

	for (i = 0; i < GB02MAC12; i++) {
		int j;

		for (j = 0; j < HW_CORE_MAX; j++)
			hantrodec_data.hwregs[i][j] = 0;
	}
	hantrodec_data.async_queue_dec = NULL;
	hantrodec_data.async_queue_pp = NULL;

	result = ReserveIO();
	if (result < 0)
		goto err;

	for (i = 0; i < hantrodec_data.cores; i++)
		AXIFEEnable(hantrodec_data.hwregs[i][HW_AXIFE]);

	if (vcmd) {
		result = GB02FUNC955(dec_data->dec_pcie,
		 total_vcmd_core_num, vcmd_pool, vcmd_reg, dec_paddr);
		if (result)
			goto err;
	}

	gb_printf(KERN_INFO, "%s: module init finish\n", __func__);
	return 0;
err:
	ReleaseIO();
	vfree(dec_data);
	gb_printf(KERN_ERR, "hantrodec: module not inserted\n");
	return result;
}

/*
 * Function name   : GB02FUNC603
 * Description     : clean up
 * Return type     : int
*/
void GB02FUNC603(int total_vcmd_core_num)
{
	hantrodec_t *dev = &hantrodec_data;
	int i;
	volatile u8 *mmu_hwregs[GB02MAC9][2];
	int has_mmu = 0;

	for (i = 0; i < GB02MAC9; i++) {
		mmu_hwregs[i][0] = dev->hwregs[i][HW_MMU];
		mmu_hwregs[i][1] = dev->hwregs[i][HW_MMU_WR];
		if (dev->hwregs[i][HW_DEC400]) {
			/* disable dec400 when rmmod driver. */
			iowrite32(0x00810002,
				(void *)(dev->hwregs[i][HW_DEC400] + 0x800));
		}
		if (dev->hwregs[i][HW_MMU])
			has_mmu = 1;
	}

	GB02FUNC976(total_vcmd_core_num);
	ReleaseIO();
	vfree(GB02FUNC589());
	gb_printf(KERN_INFO, "%s: module removed finish\n", __func__);
}


