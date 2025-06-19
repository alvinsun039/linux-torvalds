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

#ifndef _GB_UK_H_
#define _GB_UK_H_

#include <uapi/drm/drm.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define GB02MAC874           0x16c3
#define GB02MAC875           0xabcd
#define GB02MAC876	0x10ee
#define GB02MAC877	0x8018

#define GB02MAC878           0x8510
#define GB02MAC879           0x0201

#define GB02MAC882			0x00
#define GB02MAC883			0x01
#define GB02MAC884		0x02
#define GB02MAC885			0x03
#define GB02MAC886		0x04
#define GB02MAC887		0x05
#define GB02MAC888	0x06

#define GB02MAC889            0x07
#define GB02MAC890              0x08
#define GB02MAC891      0x09
#define GB02MAC892         0x0a

#define GB02MAC893          0x0b
#define GB02MAC894         0x0c
#define GB02MAC895             0x0d
#define GB02MAC896        0x0e

#define GB02MAC897                   0x20
#define GB02MAC898           0x21

#define GB02MAC899                0x23
#define GB02MAC900                0x24

#define GB02MAC901             0x25

#define GB02MAC902          0x30

#define GB02MAC903	        	0x31
#define GB02MAC904		0x32
#define GB02MAC905	        0x33

#define GB02MAC906	        0x34

#define DRM_IOCTL_GB_SUBMIT \
	DRM_IOW(DRM_COMMAND_BASE + GB02MAC882, struct GB02STR87)
#define DRM_IOCTL_GB_WAIT_BO \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC883, struct GB02STR88)
#define DRM_IOCTL_GB_CREATE_BO \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC884, struct GB02STR89)
#define DRM_IOCTL_GB_MMAP_BO \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC885, struct GB02STR95)
#define DRM_IOCTL_GB_GET_PARAM \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC886, struct GB02STR100)
#define DRM_IOCTL_GB_GET_BO_OFFSET \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC887, struct GB02STR101)
#define DRM_IOCTL_GB_GET_BO_ADDR_MAPPING \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC888, struct GB02STR104)
	
#define DRM_IOCTL_GB_VPU_VCMD_OPEN \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC889, struct GB02STR97)
#define DRM_IOCTL_GB_VPU_VCMD_CLOSE \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC905, struct GB02STR97)

#define DRM_IOCTL_GB_VPU_REG_OPS \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC890, struct GB02STR93)
#define DRM_IOCTL_GB_VPU_VCMDBUF_RESERVE \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC891, struct drm_gb_cmdbuf_resv)
#define DRM_IOCTL_GB_VPU_CMD_BUF_FREE \
		DRM_IOWR(DRM_COMMAND_BASE + GB02MAC892, struct drm_gb_cmdbuf_resv)

#define DRM_IOCTL_GB_DMA_TRANS_TO_FB \
        DRM_IOWR(DRM_COMMAND_BASE + GB02MAC893, struct GB02STR105)
#define DRM_IOCTL_GB_DMA_TRANS_TO_RAM \
        DRM_IOWR(DRM_COMMAND_BASE + GB02MAC894, struct GB02STR106)
#define DRM_IOCTL_GB_V2VDMA_TRANS \
        DRM_IOWR(DRM_COMMAND_BASE + GB02MAC895, struct GB02STR108)  
#define DRM_IOCTL_GB_HDMA_OFFSET_TRANS \
        DRM_IOWR(DRM_COMMAND_BASE + GB02MAC896, struct GB02STR107)

#define DRM_IOCTL_GB_GET_FB \
        DRM_IOWR(DRM_COMMAND_BASE + GB02MAC897, struct GB02STR109)

#define DRM_IOCTL_GB_GET_FULLSCREEN \
        DRM_IOWR(DRM_COMMAND_BASE + GB02MAC898, struct GB02STR110)

#define DRM_IOCTL_GB_CREATE_FB \
        DRM_IOWR(DRM_COMMAND_BASE + GB02MAC899, struct GB02STR90)

#define DRM_IOCTL_GB_SWITCH_FB \
        DRM_IOWR(DRM_COMMAND_BASE + GB02MAC900, struct GB02STR111)  

#define DRM_IOCTL_GB_GET_GPU_INFO \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC901, struct gpu_info)

#define DRM_IOCTL_GB_SET_RENDER_SIZE \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC902, struct GB02STR112)

#define DRM_IOCTL_GB_GET_BO_VADDR \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC903, struct GB02STR102)
#define DRM_IOCTL_GB_SET_SCHED_TIME_MS \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC904, struct GB02STR113)

#define DRM_IOCTL_GB_INFINITY_ATOMIC \
	DRM_IOWR(DRM_COMMAND_BASE + GB02MAC906, struct drm_mode_atomic)

/* Use 'k' as magic number */
#define GB02MAC918  'k'

#define GBDEC_PP_INSTANCE       _IO(GB02MAC918, 1)
#define GBDEC_HW_PERFORMANCE    _IO(GBODEC_IOC_MAGIC, 2)
#define GBDEC_IOCGHWOFFSET _IOR(GB02MAC918, 3, unsigned long *)
#define GBDEC_IOCGHWIOSIZE\
		_IOR(GB02MAC918, 4, struct GB02STR26 *)

#define GBDEC_IOC_MC_OFFSETS\
	_IOR(GB02MAC918, 7, unsigned long *)
#define GBDEC_IOC_MC_CORES  _IOR(GB02MAC918, 8, unsigned int *)

#define GBDEC_IOCS_DEC_PUSH_REG\
	_IOW(GB02MAC918, 9, struct GB02STR27 *)
#define GBDEC_IOCS_PP_PUSH_REG\
	_IOW(GB02MAC918, 10, struct GB02STR27 *)

#define GBDEC_IOCH_DEC_RESERVE   _IO(GB02MAC918, 11)
#define GBDEC_IOCT_DEC_RELEASE   _IO(GB02MAC918, 12)
#define GBDEC_IOCQ_PP_RESERVE    _IO(GB02MAC918, 13)
#define GBDEC_IOCT_PP_RELEASE    _IO(GB02MAC918, 14)


#define GBDEC_IOCG_CORE_WAIT \
		_IOR(GB02MAC918, 19, int *)
#define GBDEC_IOX_ASIC_ID \
		_IOWR(GB02MAC918, 20, struct GB02STR29 *)
#define GBDEC_IOCG_CORE_ID \
		_IOR(GB02MAC918, 21, unsigned long)


#define GBDEC_IOCS_DEC_WRITE_REG \
	_IOW(GB02MAC918, 22, struct GB02STR27 *)
#define GBDEC_IOCS_DEC_READ_REG \
	_IOWR(GB02MAC918, 23, struct GB02STR27 *)
#define GBDEC_IOX_ASIC_BUILD_ID \
	_IOWR(GB02MAC918, 24, u32 *)
#define GBDEC_IOX_SUBSYS \
	_IOWR(GB02MAC918, 25, struct GB02STR30 *)

#define GBDEC_DEBUG_STATUS   _IO(GB02MAC918, 29)

#define GBDEC_IOCS_DEC_WRITE_APBFILTER_REG \
	_IOW(GB02MAC918, 30, struct GB02STR27 *)

#define GBDEC_IOC_AXIFE_CONFIG \
	_IOR(GB02MAC918, 32, struct GB02STR25 *)

#define GBDEC_IOCS_DEC_PULL_REG \
	_IOWR(GB02MAC918, 17, struct GB02STR27 *)
#define GBDEC_IOCS_PP_PULL_REG \
	_IOWR(GB02MAC918, 18, struct GB02STR27 *)


#define GB02MAC919 32
#define GB02MAC920 (1 << 0)

#define GB02MAC921 (1 << 0)
#define GB02MAC922  (1 << 1)
#define GB02MAC923 (1 << 2)

/*nomal vram=0; vcmd buff = 1; vcmd status = 2; */
enum gbdec_bo_flag {
	DEC_BO_NORMAL,
	DEC_BO_CMD_BUF,
	DEC_BO_CMD_STATUS,
	DEC_BO_CMD_REG
};

struct GB02STR87 {
	__u64 sc;

	__u64 in_syncs;
	__u32 in_sync_count;

	__u32 out_sync;

	__u64 bo_handles;
	__u32 bo_handle_count;

	__u32 slot_req;

};

struct GB02STR88 {
	__u32 handle;
	__u32 pad;
	__s64 timeout_ns; /* absolute */
};

#define GB02MAC924    (1<<0)
#define GB02MAC925      (1<<1)
#define GB02MAC926        (1<<2)
#define GB02MAC927    (1<<3)
#define GB02MAC928 (1<<6)

#define GB02MAC930     (1<<8)
#define GB02MAC932     (1<<9)
#define GB02MAC934       (1<<10)
#define GB02MAC936    (1<<11)
#define GB02MAC938     (1<<12)
#define GB02MAC940      (1<<13)
#define GB02MAC942   (1<<30)

#define GB02MAC946 (GB02MAC934 | GB02MAC932)
#define GB02MAC949 (GB02MAC930 | GB02MAC946)
#define GB02MAC952 (GB02MAC936 | GB02MAC938 | GB02MAC940)
#define GB02MAC955 (GB02MAC930 | GB02MAC934 | GB02MAC952)
#define GB02MAC957 (GB02MAC949 | GB02MAC952)

#define GB02MAC960 (GB02MAC924 | GB02MAC925 | GB02MAC926 | GB02MAC927 | GB02MAC928 | GB02MAC957 | GB02MAC942)

#define	GB02MAC964	11
#define	GB02MAC966	12
#define	GB02MAC968	21

enum ip_type {
	GB_IP_DEC = 1,
	GB_IP_ENC,
	GB_IP_DEC1,
};

enum vpu_module {
	VC8000E = 0,
	VC8000D = 2,
	JPEGD = 4,
};
struct drm_gb_cmdbuf_resv
{
/*input ;executing_time=encoded_image_size*(rdoLevel+data1)*(rdoq+1);*/
	__u64 executing_time;
/*input input vc8000e=0,IM=1,vc8000d=2, jpege=3, jpegd=4*/
	__u16 module_type;
/*input, reserve is not used; link and run is input.*/
	__u16 cmdbuf_size;
/*input,normal=0, high/live=1*/
	__u16 priority;
/*output, it is unique in driver.*/
	__u16 cmdbuf_id;
/*just used for polling.*/
	__u16 core_id;
	__u32 handle_count;
	__u64 handles;
	__u32 cmdbuf_offset;
	__u32 status_offset;
};


struct GB02STR89 {
	__u32 size;
	__u32 flags;

	__u32 handle;
	__u32 domain;

	/*physical addr for vpu*/
	__u64 offset_va;
	__u64 offset_pa;

};
struct GB02STR90 {
	struct GB02STR89 bo;
	int fb_type;
};

struct GB02STR91 {
	__u32 ip_type;
/*nomal vram=1; vcmd buff = 2; vcmd status = 3;*/
	__u32 flags;
	__u64 offset;
};

struct GB02STR92 {
/*dec=1; enc = 2;*/
	__u32 ip_type;

	__u32 handle;
};

struct GB02STR93 {
/*dec=1; enc = 2;*/
	__u32 ip_type;
	__u32 cmd;
	void  *priv;

	__u32 handle;
};

struct GB02STR95 {
	__u32 handle;
	__u32 flags;

	__u64 offset;
};

struct GB02STR97 {
        /*dec=1; enc = 2;*/
	__u32 ip_type;
	__u32 size;
	__u32 flags;

	__u32 handle;
	__u32 domain;

	/*gpu virtual addr*/
	__u64 offset_va;
	/*gpu physical addr*/
	__u64 offset_pa;
};

enum drm_gb_param {
	DRM_GB_PARAM_GPU_PROD_ID,
	DRM_GB_PARAM_GPU_REVISION,
	DRM_GB_PARAM_SHADER_PRESENT,
	DRM_GB_PARAM_TILER_PRESENT,
	DRM_GB_PARAM_L2_PRESENT,
	DRM_GB_PARAM_STACK_PRESENT,
	DRM_GB_PARAM_AS_PRESENT,
	DRM_GB_PARAM_SS_PRESENT,
	DRM_GB_PARAM_L2_FEATURES,
	DRM_GB_PARAM_CORE_FEATURES,
	DRM_GB_PARAM_TILER_FEATURES,
	DRM_GB_PARAM_MEM_FEATURES,
	DRM_GB_PARAM_MMU_FEATURES,
	DRM_GB_PARAM_THREAD_FEATURES,
	DRM_GB_PARAM_MAX_THREADS,
	DRM_GB_PARAM_THREAD_MAX_WORKGROUP_SZ,
	DRM_GB_PARAM_THREAD_MAX_BARRIER_SZ,
	DRM_GB_PARAM_COHERENCY_FEATURES,
	DRM_GB_PARAM_TEXTURE_FEATURES0,
	DRM_GB_PARAM_TEXTURE_FEATURES1,
	DRM_GB_PARAM_TEXTURE_FEATURES2,
	DRM_GB_PARAM_TEXTURE_FEATURES3,
	DRM_GB_PARAM_SS_FEATURES0,
	DRM_GB_PARAM_SS_FEATURES1,
	DRM_GB_PARAM_SS_FEATURES2,
	DRM_GB_PARAM_SS_FEATURES3,
	DRM_GB_PARAM_SS_FEATURES4,
	DRM_GB_PARAM_SS_FEATURES5,
	DRM_GB_PARAM_SS_FEATURES6,
	DRM_GB_PARAM_SS_FEATURES7,
	DRM_GB_PARAM_SS_FEATURES8,
	DRM_GB_PARAM_SS_FEATURES9,
	DRM_GB_PARAM_SS_FEATURES10,
	DRM_GB_PARAM_SS_FEATURES11,
	DRM_GB_PARAM_SS_FEATURES12,
	DRM_GB_PARAM_SS_FEATURES13,
	DRM_GB_PARAM_SS_FEATURES14,
	DRM_GB_PARAM_SS_FEATURES15,
	DRM_GB_PARAM_NR_CORE_GROUPS,
	DRM_GB_PARAM_GROW_HEAP_MEMORY,
	DRM_GB_PARAM_VPU_BO_SAVE_FLAG,
};

struct GB02STR100 {
	__u32 param;
	__u32 pad;
	__u64 value;
};

struct GB02STR101 {
	__u32 handle;
	__u32 pad;
	__u64 offset;
};

struct GB02STR102 {
	__u32 handle;
	__u32 pad;
	__u64 vaddr;
};

struct GB02STR104 {
	__u64 gpu_va;
#ifdef GB_KMD_DBG
	__u64 gpu_pa;
#endif
	__u64 cpu_va;
	__u64 cpu_va_start;
	__u64 cpu_va_end;
};

enum gb_dma_data_direction {
	/* RAM TO VRAM */
	GB_DMA_TO_DEVICE = 0,
	/* VRAM TO RAM */
	GB_DMA_FROM_DEVICE = 1,
	GB_DMA_NONE,
};

struct GB02STR105 {
	/* fb handle, user set */
	__u32 handle;
	/* window width of which for dma trans, user set */
	__u32 width;
	/* window height of which for dma trans, user set */
	__u32 height;
	/* current plane width, ex:1920*1080p is 1920, user set */
	__u32 plane_width;
	/* number of ll entry, user don't care */
	__u32 num_dma_entry;
	/* ram va, user set, must be 4K align */
	__u64 cpu_va;
	/* vram va base, user set */
	__u64 gpu_va_base;
	/* vram va, user set */
	__u64 gpu_va;
	/* ll table's addr for dma trans, user don't care */
	__u64 ll_table_addr;
	/* dma dir, user set */
	enum gb_dma_data_direction dma_direction;
};

struct GB02STR106 {
	/* fb handle, user set */
	__u32 handle;
	/* ram va, user set, must be 4K align */
	__u64 cpu_va;
	/* dma trans size */
	__u64 size;
	/* dma dir, user set */
	enum gb_dma_data_direction dma_direction;
};

struct GB02STR107 {
	/* fb handle, user set */
	__u32 handle;
	/* ram va, user set, must be 4K align */
	__u64 cpu_va;
	/* dma trans size */
	__u64 size;
	/* dma trans offset */
	__u64 offset;
	/* dma dir, user set */
	enum gb_dma_data_direction dma_direction;
};

struct GB02STR108 {
	__u32 src_handle;
	__u32 dst_handle;
	__u64 src_offset;
	__u64 dst_offset;
	size_t size;
};

struct GB02STR109 {
	__u64 height;
	__u64 width;
	__u64 size;
};

struct GB02STR110 {
	int flag;
	__u64 gpu_va;
};

struct GB02STR111 {
	int switch_fb;
};

struct GB02STR112 {
	__u32 size;
};

struct GB02STR113 {
	__u32 timems;
};

#if defined(__cplusplus)
}
#endif

#endif /* _GB_UK_H_ */
