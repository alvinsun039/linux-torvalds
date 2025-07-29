#include <linux/pci.h>
#include <linux/miscdevice.h>
#include "gb_common.h"
#include "gb_pcie_info.h"
#include "gb_irq.h"
#include "xt.h"
#include "audio/gb02_fpga_v2vdma.h"
#include "audio/local.h"
#include "audio/i2s_platform.h"
#include "vpu/vpu_comm/gb_vpu.h"
#include "vpu/vpu_mm/gb_vpu_ttm.h"
#include "vpu/vpu_dec/vpu_vcmd.h"
#include "vpu/vpu_enc/vpu_vc8000E_vcmd.h"
#ifdef GB_IRQ_DBG
#define gb_irq_debug pr_err
#else
#define gb_irq_debug(format, arg...) \
	do {} while (0)
#endif

void __iomem *gb_comp_irq_reg_bar_base;
static void GB02FUNC348(u32 offset, u32 value)
{
	iowrite32(value, gb_comp_irq_reg_bar_base + offset);
	gb_irq_debug("write gb_irq_debug reg:0x%x:0x%x\n", offset, value);
	udelay(50);
}

static u32 GB02FUNC350(u32 offset)
{
	u32 val = 0;

	val = ioread32(gb_comp_irq_reg_bar_base + offset);
	gb_irq_debug("read gb_irq_debug reg:0x%x:0x%x\n", offset, val);

	return (u64)val;
}

int GB02FUNC352(struct GB02STR70 *pcie_info)
{
	int ret = 0;

	if (!pci_msi_enabled())
		gb_printf(KERN_ERR, "%s:%d:MSI disabled.\n", __func__, __LINE__);

	//ret = pci_enable_msi(pcie_info->pdev);
	if (!ret) {
		gb_printf(KERN_INFO, "GB using MSI, irq: %d\n.\n",
			pcie_info->pdev->irq);
		pcie_info->msi_enabled = 1;
#ifdef GB02MAC287
		pcie_info->irq_vector_num =
		 pci_alloc_irq_vectors(pcie_info->pdev, 1,
		  GB02FUNC472(pcie_info->GB02STR153), PCI_IRQ_MSIX);
#else
		pcie_info->irq_vector_num =
		 GB02FUNC472(pcie_info->GB02STR153);
#endif
	} else {
		gb_printf(KERN_ERR, "GB doesn't support MSI\n");
		pcie_info->irq_vector_num =
		 GB02FUNC472(pcie_info->GB02STR153);
		pcie_info->msi_enabled = 0;
	}

	gb_printf(KERN_INFO, "%s:%d:MSI vector num:%d.\n",
	 __func__, __LINE__, pcie_info->irq_vector_num);
	gb_printf(KERN_INFO, "%s successfully!\n", __func__);

	return 0;
}

int GB02FUNC359(struct GB02STR70 *pcie_info)
{
	if (pcie_info->msi_enabled) {
		pci_disable_msi(pcie_info->pdev);
		pcie_info->msi_enabled = 0;
	}

	return 0;
}

int GB02FUNC364(struct GB02STR70 *pcie_info)
{
	int i;

	if (pcie_info->msi_enabled) {
		for (i = 0; i < pcie_info->irq_vector_num; i++) {
#ifdef GB02MAC287
			pcie_info->irqs[i] = pci_irq_vector(pcie_info->pdev, i);
#else
			pcie_info->irqs[i] = pcie_info->pdev->irq;
#endif
			gb_printf(KERN_INFO, "GB IRQ%d id: %d\n", i, pcie_info->irqs[i]);
		}
	} else {
		for (i = 0; i < pcie_info->irq_vector_num; i++) {
			pcie_info->irqs[i] = pcie_info->pdev->irq;
			gb_printf(KERN_INFO, "GB IRQ%d id: %d\n", i, pcie_info->irqs[i]);
		}
	}

	return 0;
	}

int GB02FUNC370(struct GB02STR70 *pcie_info)
{
	return 0;
}

int GB02FUNC373(struct GB02STR70 *pcie_info,
	enum gb_irq_ip_type ip_type, int ip_irq_index)
{
#ifndef GB02MAC287
	return 0;
#else
	if (pcie_info->GB02STR153->gb_type == GENBU_FPGA) {
		switch (ip_type) {
		case irq_type_v2vdma:
			return 0;
		case irq_type_audio:
			if (ip_irq_index >= AUDIO_IRQ_TOTAL)
				return -1;
			return ip_irq_index + 1;
		case irq_type_dec0:
			return 3;
		case irq_type_dec1:
			return 9;
		case irq_type_enc:
			return 4;
		case irq_type_dc:
			if (ip_irq_index >= DC_IRQ_TOTAL)
				return -1;
			return ip_irq_index + 4;
		case irq_type_gpu:
			if (ip_irq_index >= GPU_IRQ_TOTAL)
				return -1;
			return ip_irq_index + 6;
		default:
			return -1;
		}
	} else if (pcie_info->GB02STR153->gb_type == GENBU_01) {
		switch (ip_type) {
		case irq_type_gpu:
			if (ip_irq_index >= GPU_IRQ_TOTAL)
				return -1;

			if (ip_irq_index == GPU_EVENT_IRQ)
				return ip_irq_index + 2;
			return ip_irq_index + 1;
		case irq_type_hdmac:
			return 4;
		case irq_type_comp:
			return 0;
		default:
			return -1;
		}
	} else if (pcie_info->GB02STR153->gb_type == GENBU_02) {
		switch (ip_type) {
		case irq_type_gpu:
			if (ip_irq_index >= GPU_IRQ_TOTAL)
				return -1;

			if (ip_irq_index == GPU_EVENT_IRQ)
				return ip_irq_index + 2;
			return ip_irq_index + 1;
		case irq_type_hdmac:
			return 4;
		case irq_type_comp:
			return 0;
		default:
			return -1;
		}
	}
#endif
	return 0;
}

static irqreturn_t GB02FUNC388(int irq, void *data)
{
	struct GB02STR70 *pcie_info = (struct GB02STR70 *)data;
	unsigned long irq_final_status, irq_status, irq_raw_status;
	int i;
	irqreturn_t ret = IRQ_HANDLED;

	irq_final_status = (unsigned long)GB02FUNC350(GB02MAC562) |
	 ((unsigned long)GB02FUNC350(GB02MAC563) << 32);

	if (!irq_final_status) {
		irq_raw_status = (unsigned long)GB02FUNC350(GB02MAC555) |
			((unsigned long)GB02FUNC350(GB02MAC556) << 32);
		irq_status = (unsigned long)GB02FUNC350(GB02MAC558) |
			((unsigned long)GB02FUNC350(GB02MAC559) << 32);
		gb_irq_debug("%s %d no irq! raw_status:0x%lx, status:0x%lx\n",
				__func__, __LINE__, irq_raw_status, irq_status);
		return IRQ_NONE;
	}
restart:
	for_each_set_bit(i, &irq_final_status, gb_irq_bit_total) {
		switch (i) {
		case v2vdma_intr:
			(void)GB02FUNC1854(irq,
					(void *)pcie_info);
			break;
		case hdmac_interupt:
			(void)GB02FUNC1662(irq,
					(void *)pcie_info->edma_para.dma_ctrl);
			break;
		case audio_dma_intr:
			(void)GB02FUNC561(irq, NULL);
			break;
		case i2s0_intr:
			break;
		case i2s1_intr:
			break;
		case i2s2_intr:
			break;
		case i2s3_intr:
			break;
		case i2s4_intr:
			break;
		case i2s5_intr:
			break;
		case dec0_INTR:
			GB02FUNC947(irq);
			break;
		case dec1_INTR:
			GB02FUNC1391(irq);
			break;
		case enc0_INTR:
			GB02FUNC1649(irq);
			break;
		case enc1_INTR:
			break;
		case enc2_INTR:
			break;
		case gpu_IRQMMU:
			ret = GB02FUNC502(irq,
				(void *)(pcie_info->gbdev));
			break;
		case gpu_IRQJOB:
			ret = GB02FUNC502(irq,
				(void *)(pcie_info->gbdev));
			break;
		case gpu_IRQGPU:
			ret = GB02FUNC502(irq,
				(void *)(pcie_info->gbdev));
			break;
		case gpu_IRQEVENT:
		case dc0_irqde:
			ret = GB02FUNC1754(irq, (void *)(pcie_info->gbdev), 0);
			break;
		case dc0_irqse:
		case dc1_irqde:
			ret = GB02FUNC1754(irq, (void *)(pcie_info->gbdev), 1);
			break;
		case dc1_irqse:
		case dc2_irqde:
			ret = GB02FUNC1754(irq, (void *)(pcie_info->gbdev), 2);
			break;
		case dc2_irqse:
		case dc3_irqde:
			ret = GB02FUNC1754(irq, (void *)(pcie_info->gbdev), 3);
			break;
		case dc3_irqse:
		case dc4_irqde:
			ret = GB02FUNC1754(irq, (void *)(pcie_info->gbdev), 4);
			break;
		case dc4_irqse:
		case dc5_irqde:
			ret = GB02FUNC1754(irq, (void *)(pcie_info->gbdev), 5);
			break;
		case dc5_irqse:
		case dp0_intr:
			ret = GB02FUNC1511(irq, pcie_info->dptx[0]);
			break;
		case dp1_intr:
			ret = GB02FUNC1511(irq, pcie_info->dptx[1]);
			break;
		case dp2_intr:
			ret = GB02FUNC1511(irq, pcie_info->dptx[2]);
			break;
		case dp3_intr:
			ret = GB02FUNC1511(irq, pcie_info->dptx[3]);
			break;
		case dp4_intr:
			ret = GB02FUNC1511(irq, pcie_info->dptx[4]);
			break;
		case dp5_intr:
			ret = GB02FUNC1511(irq, pcie_info->dptx[5]);
			break;
		case ddr0_int_o:
		case ddr1_int_o:
		case ddr2_int_o:
		case ddr3_int_o:
		case sen_volt_int_nomal:
		case sen_volt_int_fatal:
		case sen_temp_int_nomal:
		case sen_temp_int_fatal:
		default:
			gb_irq_debug("%s %d err irq! %d\n",
				__func__, __LINE__, i);
			break;
		}
	}

	irq_final_status = (unsigned long)GB02FUNC350(GB02MAC562) |
	 ((unsigned long)GB02FUNC350(GB02MAC563) << 32);

	if (irq_final_status & GENMASK_ULL(gb_irq_bit_total - 1, 0))
		goto restart;

	return ret;
}

void GB02FUNC412(void)
{
	u64 irq_en = (~DISABLE_IRQ);
	u64 irq_mask = DISABLE_IRQ;

	gb_irq_debug("%s %d irq_en:0x%llx, irq_mask:0x%llx\n",
		__func__, __LINE__, irq_en, irq_mask);
	gb_printf(KERN_INFO, "-------%s %d--irq en=%llx--mask=%llx---\n",
	 __func__, __LINE__, irq_en, irq_mask);
	msleep(100);
	GB02FUNC348(GB02MAC549, irq_en & (u64)0xffffffff);
	GB02FUNC348(GB02MAC550, irq_en >> 32);
	GB02FUNC348(GB02MAC551, irq_mask & (u64)0xffffffff);
	GB02FUNC348(GB02MAC552, irq_mask >> 32);
	gb_printf(KERN_INFO, "-------%s %d----\n", __func__, __LINE__);
	msleep(100);
}

static int GB02FUNC417(struct GB02STR70 *pcie_info)
{
	int ret;
	int irq_index = GB02FUNC373(pcie_info, irq_type_comp, 0);

	if ((irq_index < 0) || (irq_index >= GB02MAC509)) {
		gb_printf(KERN_ERR, "%s %d get irq index failed!%d\n",
			__func__, __LINE__, irq_index);
		return -1;
	}
	gb_printf(KERN_INFO, "%s-%d: pcie_info->irqs[%d] = %d\n",
		__func__, __LINE__, irq_index, pcie_info->irqs[irq_index]);
	ret = devm_request_irq(&pcie_info->pdev->dev,
				pcie_info->irqs[irq_index], GB02FUNC388,
				IRQF_SHARED,
				dev_name(&pcie_info->pdev->dev),
				pcie_info);

	if (ret)
		gb_printf(KERN_ERR, "%s %d request irq err, ret %d\n",
			__func__, __LINE__, ret);

	return ret;
}

void GB02FUNC423(struct GB02STR70 *pcie_info)
{
	int irq_index = GB02FUNC373(pcie_info, irq_type_comp, 0);

	if ((irq_index < 0) || (irq_index >= GB02MAC509)) {
		gb_printf(KERN_ERR, "%s %d get irq index failed!%d\n",
			__func__, __LINE__, irq_index);
		return;
	}

	devm_free_irq(&pcie_info->pdev->dev,
		pcie_info->irqs[irq_index],
		pcie_info);
}

int GB02FUNC426(struct GB02STR70 *pcie_info)
{
	int reg_bar_id = GB02FUNC465(pcie_info->GB02STR153);

	gb_comp_irq_reg_bar_base = pcie_info->pci_bars[reg_bar_id].mmio;
	GB02FUNC412();

	return GB02FUNC417(pcie_info);
}

