/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PCIE_RESIZEBAR_H
#define __PCIE_RESIZEBAR_H

/* Resizable BARs */
#define  GB02MAC378		4	/* capability register */
#define  GB02MAC381		0x00FFFFF0  /* supported BAR sizes */
#define  GB02MAC384		8	/* control register */
#define  GB02MAC387		0x00000007  /* BAR index */
#define  GB02MAC390	0x000000E0  /* # of resizable BARs */
#define  GB02MAC393	5	    /* shift for # of BARs */
#define  GB02MAC396	0x00001F00  /* BAR size */
#define  GB02MAC399	8	    /* shift for BAR size */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
struct gb_pci_dev_resource {
	struct list_head list;
	struct resource *res;
	struct pci_dev *dev;
	resource_size_t start;
	resource_size_t end;      
	resource_size_t add_size;
	resource_size_t min_align;
	unsigned long flags;
};
void gb_pci_restore_rebar_state(struct pci_dev *pdev);
#endif

void GB02FUNC186(struct pci_dev *pdev);
#endif