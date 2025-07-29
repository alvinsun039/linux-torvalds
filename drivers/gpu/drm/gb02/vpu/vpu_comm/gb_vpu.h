#ifndef	__GB_VPU_H__
#define	__GB_VPU_H__
#include "vpu_list.h"
#include "common/gb_pcie_info.h"
#include "gpu/gb_ip_offset.h"
//#include "gpu/gb_device.h"

#ifdef HANTROVCMD_ENABLE_IP_SUPPORT
#define GB02MAC2        32
#define GB02MAC4                  64
#else
#define GB02MAC4                  27
#endif
#define GB02MAC9 4
#define GB02MAC12 GB02MAC9

#define GB02MAC17 82
#define GB02MAC20 3

enum vbo_type {
	VBO_OUTPP = 0,
	VBO_OUT,
	VBO_OUTPP_MID,
};
struct GB02STR2 {
	int vpu_part_bo_num[GB02MAC20];
	u64 vpu_bo_priv_addr[GB02MAC20][GB02MAC17];
	struct list_head vbo_list;
	struct drm_file *file_priv;
	struct GB02STR2 *vbo_priv;
};

struct GB02STR3 {
	uint64_t offset;
	void __iomem *vir_buff;
	uint64_t flp_offset;
	struct drm_gem_object *obj;
};
enum core_type {
	/* Decoder */
	HW_VC8000D = 0,
	HW_VC8000DJ,
	HW_BIGOCEAN,
	HW_VCMD,
	HW_MMU,
	HW_MMU_WR,
	HW_DEC400,
	HW_L2CACHE,
	HW_SHAPER,
	/* Encoder*/
	/* Auxiliary IPs */
	HW_AXIFE,
	HW_AFBC,
	HW_CORE_MAX
};
struct GB02STR5 {
	unsigned long vcmd_base_addr;
	u32 vcmd_iosize;
	int vcmd_irq;
	/*input vc8000e=0,IM=1,vc8000d=2,jpege=3,jpegd=4*/
	u32 sub_module_type;
	u16 submodule_main_addr;
	u16 submodule_dec400_addr;
	u16 submodule_L2Cache_addr;
	u16 submodule_MMU_addr[2];
	u16 submodule_axife_addr[2];
};

struct GB02STR7 {
	struct GB02STR5 enc_vcmd_core_cfg;
	u32 core_id;
	u32 sw_cmdbuf_rdy_num;
	spinlock_t *spinlock;
	wait_queue_head_t  *wait_queue;
	wait_queue_head_t  *wait_abort_queue;
	bi_list list_manager;

	volatile u8 *hwregs; /* IO mem base */
	u32 reg_mirror[GB02MAC4];
	u32 duration_without_int;
	volatile u8 working_state;
	u64 total_exe_time;
	u16 status_cmdbuf_id;
	u32 hw_version_id; /*megvii 0x43421001, later 0x43421102*/
	u32 *vcmd_reg_mem_virtual_address;
	size_t vcmd_reg_mem_bus_address;
	unsigned int mmu_vcmd_reg_mem_bus_address;
	u32  vcmd_reg_mem_size;
};

typedef struct {
	char *buffer;
	volatile unsigned int iosize[GB02MAC12];
	/* mapped address to different HW cores regs*/
	volatile u8 *hwregs[GB02MAC12][HW_CORE_MAX];
	/* mapped address to different HW cores regs*/
	volatile u8 *apbfilter_hwregs[GB02MAC12][HW_CORE_MAX];
	volatile int irq[GB02MAC12];
	int hw_id[GB02MAC12][HW_CORE_MAX];
	int client_type[GB02MAC12];
	int cores;
	struct fasync_struct *async_queue_dec;
	struct fasync_struct *async_queue_pp;
} hantrodec_t;

#define GB02MAC58		30
struct GB02STR10 {
	bool			fw_header_present;
	struct GB02STR201	*vcpu_bo;
	/*virtual addr*/
	void			*cpu_addr;
	/*physical addr*/
	uint64_t		gpu_addr;
	uint32_t		max_handles;
	atomic_t		handles[GB02MAC58];
	struct drm_file		*filp[GB02MAC58];
	uint32_t		img_size[GB02MAC58];
	struct delayed_work	idle_work;
};

struct GB02STR11 {
	struct GB02STR201	*vcpu_bo;
	uint64_t		gpu_addr;
	void			*cpu_addr;
	uint32_t		fw_version;
	uint32_t		fb_version;
	atomic_t		handles[GB02MAC58];
	struct drm_file		*filp[GB02MAC58];
	uint32_t		img_size[GB02MAC58];
	struct delayed_work	idle_work;
	uint32_t		keyselect;
};

irqreturn_t GB02FUNC947(int irq);
irqreturn_t GB02FUNC1391(int irq);
irqreturn_t GB02FUNC1649(int irq);
int GB02FUNC22(hantrodec_t *gbdec_data, void *pci_bars,
	struct GB02STR67 *dev_info, struct GB02STR3 *vcmd_buff,
	struct GB02STR3 *vcmd_reg, struct GB02STR126 vpu_offset);

int GB02FUNC26(hantrodec_t *gbdec_data, void *pci_bars,
	      struct GB02STR67 *dev_info,
	      struct GB02STR3 *vcmd_buff, struct GB02STR3 *vcmd_reg,
	      struct GB02STR126 vpu_offset);

int GB02FUNC28(struct GB02STR7 *gbenc_data, void *pci_bars,
	struct GB02STR67 *dev_info, struct GB02STR3 *vcmd_pool,
	struct GB02STR3 *vcmd_reg, struct GB02STR126 vpu_offset);
#endif
