#ifndef GB02MAC652
#define GB02MAC652

#include "common/gb_common.h"
#include "common/gb-peripherals-common.h"

#define		GB02MAC656		6
#define		GB02MAC658	6
#define		GB02MAC660		4
#define		GB02MAC662		2
#define		GB02MAC664	2
#define		GB02MAC666		4
#define		GB02MAC668		1
#define		GB02MAC670	1
#define		GB02MAC672		1

#define		GB02MAC675	(0x1000000 + 0x3fc00 + 4 * 8)
#define		GB02MAC677	32

struct GB02STR67 {
	enum genbu_asic_type gb_type;
	//unsigned long vram_size;
	unsigned long dc_base_offset;
	unsigned int crtc_step;
	int gpu_reg_bar_id;
	int dc_reg_bar_id;
	int ddr_bar_id;
	int mcu_peri_bar_id;
	int peri_base_bar_id;
	int ipc_bar_id;
	int pci_bar_cnt;
	int crtc_num;
	int connector_num;
	int irq_num;
};

struct GB02STR68 {
     unsigned int hdmi_firmware_maj;
     unsigned int hdmi_firmware_min;
     unsigned int vga_firmware_maj;
     unsigned int vga_firmware_min;
     unsigned int version_info;
};

struct GB02STR69 {
	phys_addr_t base;
	void __iomem *mmio;
	resource_size_t len;
};

struct GB02STR70 {
	struct pci_dev *pdev;
	struct GB02STR69 pci_bars[GB02MAC507];
	int irq_vector_num;
	int msi_enabled;
	int irqs[GB02MAC509];
	struct GB02STR67 *GB02STR153;
	struct GB02STR62 edma_para;
	void *vdec0_info;
	void *vdec1_info;
	void *venc_info;
	void *ao_info;
	struct GB02STR39 *gbdev;
	struct dptx* dptx[GB02MAC508];
	int board_id;
	struct GB02STR68 f_info;
};

#define GENBU_FPGA_DEV_INFO \
	.gb_type = GENBU_FPGA, \
	.crtc_step = 0x100000, \
	.ddr_bar_id = 0, \
	.ipc_bar_id = -1, \
	.pci_bar_cnt = 5, \
	.gpu_reg_bar_id = 2, \
	.dc_reg_bar_id = 2, \
	.dc_base_offset = 0x100000, \
	.connector_num = GB02MAC670, \
	.crtc_num = GB02MAC668, \
	.irq_num = 16, \

#define GENBU_01_DEV_INFO \
	.gb_type = GENBU_01, \
	.crtc_step = 0x20000, \
	.ddr_bar_id = 0, \
	.ipc_bar_id = 3, \
	.pci_bar_cnt = 4, \
	.gpu_reg_bar_id = 2, \
	.dc_reg_bar_id = 2, \
	.dc_base_offset = 0x100000, \
	.connector_num = GB02MAC664, \
	.crtc_num = GB02MAC662, \
	.irq_num = 1, \

#define GENBU_02_DEV_INFO \
	.gb_type = GENBU_02, \
	.crtc_step = 0x20000, \
	.ddr_bar_id = 2, \
	.ipc_bar_id = -1, \
	.pci_bar_cnt = 5, \
	.mcu_peri_bar_id = 0, \
	.gpu_reg_bar_id = 1, \
	.dc_reg_bar_id = 1, \
	.peri_base_bar_id = 4, \
	.dc_base_offset = 0x0, \
	.connector_num = GB02MAC658, \
	.crtc_num = GB02MAC656, \
	.irq_num = 8, \

enum genbu_asic_type GB02FUNC460(struct GB02STR67 *GB02STR153);
u64 GB02FUNC462(void);
u64 GB02FUNC464(void);
int GB02FUNC465(struct GB02STR67 *GB02STR153);
int GB02FUNC468(struct GB02STR67 *GB02STR153);
int GB02FUNC469(struct GB02STR67 *GB02STR153);
int GB02FUNC472(struct GB02STR67 *GB02STR153);
int GB02FUNC474(struct GB02STR67 *GB02STR153);
int GB02FUNC477(struct GB02STR67 *GB02STR153);
u64 GB02FUNC471(struct GB02STR67 *GB02STR153);
int GB02FUNC479(struct GB02STR67 *GB02STR153);
struct GB02STR70 *GB02FUNC518(void);
int GB02FUNC480(struct GB02STR67 *GB02STR153);
u64 GB02FUNC456(void);
#endif
