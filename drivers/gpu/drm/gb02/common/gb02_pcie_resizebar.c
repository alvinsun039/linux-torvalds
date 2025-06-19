// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/bitfield.h>
#include <linux/kthread.h>
#include <linux/idr.h>
#include <linux/version.h>
#include "common/gb02_pcie_resizebar.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#else
#include <drm/drm_device.h>
#endif
#include "common/gb_common.h"
#include "gb_device.h"
#include <linux/slab.h>

static inline int GB02FUNC155(u64 bytes)
{
	bytes = roundup_pow_of_two(bytes);

	/*  Converts the logarithm to a number representing the size of bar. 
	For example, if bytes is 2^21 (2 MiB) and the logarithm is 21, 
	subtract 20 to get 1, indicating that the size exponent is 1 (corresponding to 2 MiB). */

	return max(ilog2(bytes), 20) - 20;
}

static int GB02FUNC157(struct pci_dev *pdev, int bar)
{
	unsigned int pos, nbars, i;
	u32 ctrl;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_REBAR);
	if (!pos)
		return -ENOTSUPP;

	pci_read_config_dword(pdev, pos + GB02MAC384, &ctrl);
	nbars = (ctrl & GB02MAC390) >>
			PCI_REBAR_CTRL_NBAR_SHIFT;

	for (i = 0; i < nbars; i++, pos += 8) {
		int bar_idx;

		pci_read_config_dword(pdev, pos + GB02MAC384, &ctrl);
		bar_idx = ctrl & GB02MAC387;
		if (bar_idx == bar)
			return pos;
	}

	return -ENOENT;
}

static u32 GB02FUNC162(struct pci_dev *pdev, int bar)
{
	int pos;
	u32 cap;

	pos = GB02FUNC157(pdev, bar);
	if (pos < 0)
		return 0;

	pci_read_config_dword(pdev, pos + GB02MAC378, &cap);

	/* 	GB02MAC381 do not start from bit 0, 
	move 4 bits to the right and align these to the least significant bits*/
	return (cap & GB02MAC381) >> 4;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)

void gb_pci_restore_rebar_state(struct pci_dev *pdev)
{
	unsigned int pos, nbars, i;
	u32 ctrl;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_REBAR);
	if (!pos)
		return;

	pci_read_config_dword(pdev, pos + GB02MAC384, &ctrl);
	nbars = (ctrl & GB02MAC390) >>
			GB02MAC393;

	for (i = 0; i < nbars; i++, pos += 8) {
		struct resource *res;
		int bar_idx, size;

		pci_read_config_dword(pdev, pos + GB02MAC384, &ctrl);
		bar_idx = ctrl & GB02MAC387;
		res = pdev->resource + bar_idx;
		size = ilog2(resource_size(res)) - 20;
		ctrl &= ~GB02MAC396;
		ctrl |= size << GB02MAC399;
		pci_write_config_dword(pdev, pos + GB02MAC384, ctrl);
	}
}

static void gb_free_list(struct list_head *head)
{
	struct gb_pci_dev_resource *dev_res, *tmp;

	list_for_each_entry_safe(dev_res, tmp, head, list) {
		list_del(&dev_res->list);
		kfree(dev_res);
	}
}
static int gb_add_to_list(struct list_head *head, struct pci_dev *dev,
			struct resource *res, resource_size_t add_size,
			resource_size_t min_align)
{
	struct gb_pci_dev_resource *tmp;

	tmp = kzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	tmp->res = res;
	tmp->dev = dev;
	tmp->start = res->start;
	tmp->end = res->end;
	tmp->flags = res->flags;
	tmp->add_size = add_size;
	tmp->min_align = min_align;

	list_add(&tmp->list, head);
	return 0;
}

static inline u64 gb_pci_rebar_size_to_bytes(int size)
{
	/*For example,if size =0 return：2^20=1MB;	else if size=13,return：2^33 =8GB*/
	return 1ULL << (size + 20); 
}

static int gb_pci_reassign_bridge_resources(struct pci_dev *bridge, unsigned long type)
{
	struct gb_pci_dev_resource *dev_res;
	struct pci_dev *next;
	LIST_HEAD(saved);
	LIST_HEAD(failed);
	unsigned int i;
	int ret;

	/* Walk to the root hub, releasing bridge BARs when possible */
	next = bridge;
	do {
		bridge = next;
		for (i = PCI_BRIDGE_RESOURCES; i < PCI_BRIDGE_RESOURCE_END; i++) {
			struct resource *res = &bridge->resource[i];

			if ((res->flags ^ type) & (IORESOURCE_IO | IORESOURCE_MEM | 
 							IORESOURCE_PREFETCH |IORESOURCE_MEM_64))
				continue;

			/* Ignore BARs which are still in use */
			if (res->child)
				continue;

			ret = gb_add_to_list(&saved, bridge, res, 0, 0);
			if (ret)
				goto cleanup;

 			dev_info(&bridge->dev, "BAR %d: releasing %pR\n", i,res);

			if (res->parent)
				release_resource(res);
			res->start = 0;
			res->end = 0;
			break;
		}
		if (i == PCI_BRIDGE_RESOURCE_END)
			break;

		next = bridge->bus ? bridge->bus->self : NULL;
	} while (next);

	if (list_empty(&saved))
		return -ENOENT;

	/* Use pci_assign_unassigned_bridge_resources to assign resources */
	pci_assign_unassigned_bridge_resources(bridge);

	list_for_each_entry(dev_res, &saved, list) {
		/* Skip the bridge we just assigned resources for */
		if (bridge == dev_res->dev)
			continue;

		bridge = dev_res->dev;
		// pci_setup_bridge(bridge->subordinate);
	}

	gb_free_list(&saved);

	/* Enable all bridges */
	ret = pci_reenable_device(bridge);
	if (ret)
		dev_err(&bridge->dev, "Error reenabling bridge (%d)\n", ret);
	pci_set_master(bridge);

	return 0;

cleanup:
	/* Restore size and flags */
	list_for_each_entry(dev_res, &failed, list) {
		struct resource *res = dev_res->res;
		res->start = dev_res->start;
		res->end = dev_res->end;
		res->flags = dev_res->flags;
	}
	gb_free_list(&failed);

	/* Revert to the old configuration */
	list_for_each_entry(dev_res, &saved, list) {
		struct resource *res = dev_res->res;
		bridge = dev_res->dev;
		i = res - bridge->resource;
		res->start = dev_res->start;
		res->end = dev_res->end;
		res->flags = dev_res->flags;
		pci_claim_resource(bridge, i);
		//pci_setup_bridge(bridge->subordinate);
	}
	gb_free_list(&saved);

	return ret;
}

static int gb_pci_rebar_get_current_size(struct pci_dev *pdev, int bar)
{
	int pos;
	u32 ctrl;

	pos = GB02FUNC157(pdev, bar);
	if (pos < 0)
		return pos;

	pci_read_config_dword(pdev, pos + GB02MAC384, &ctrl);
	return (ctrl & GB02MAC396) >> GB02MAC399;
}

static int gb_pci_rebar_set_size(struct pci_dev *pdev, int bar, int size)
{
	int pos;
	u32 ctrl;

	pos = GB02FUNC157(pdev, bar);
	if (pos < 0)
		return pos;

	pci_read_config_dword(pdev, pos + GB02MAC384, &ctrl);
	ctrl &= ~GB02MAC396;
	ctrl |= size << GB02MAC399;
	pci_write_config_dword(pdev, pos + GB02MAC384, ctrl);
	return 0;
}

static int gb_pci_resize_resource(struct pci_dev *dev, int resno, int size)
{
	struct resource *res = dev->resource + resno;
	int old,ret;
	u32 sizes;
	u16 cmd;

	/* Make sure the resource isn't assigned before resizing it. */

	if (!(res->flags & IORESOURCE_UNSET))
		return -EBUSY;

	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	if (cmd & PCI_COMMAND_MEMORY)
		return -EBUSY;

	sizes = GB02FUNC162(dev, resno);
	if (!sizes)
		return -ENOTSUPP;

	if (!(sizes & BIT(size)))
		return -EINVAL;

	old = gb_pci_rebar_get_current_size(dev, resno);
	if (old < 0)
		return old;
		
	ret = gb_pci_rebar_set_size(dev, resno, size);
	if (ret)
		return ret;

	res->end = res->start + gb_pci_rebar_size_to_bytes(size) - 1;

	/* Check if the new config works by trying to assign everything. */
	ret = gb_pci_reassign_bridge_resources(dev->bus->self, res->flags);

	if (ret)
		goto error_resize;

	return 0;

error_resize:
	gb_pci_rebar_set_size(dev, resno, old);
	res->end = res->start + gb_pci_rebar_size_to_bytes(old) - 1;
	return ret;
}

static void gb_pci_release_resource(struct pci_dev *dev, int resno)
{
	struct resource *res = dev->resource + resno;

	dev_info(&dev->dev, "BAR %d: releasing %pR\n", resno, res);
	release_resource(res);
	res->end = resource_size(res) - 1;
	res->start = 0;
	res->flags |= IORESOURCE_UNSET;
}
#endif

void GB02FUNC174(struct pci_dev *pdev, unsigned long vram_size,int current_size,int requested_size)
{
	int r;
	unsigned long sizes;

	if (pci_resource_len(pdev, GB02MAC724)){
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
		gb_pci_release_resource(pdev, GB02MAC724);
#else
		pci_release_resource(pdev, GB02MAC724);
#endif
	}

	// Check if BAR2 has PCIe rebar capabilities
	sizes = GB02FUNC162(pdev, GB02MAC724); 
	if (sizes == 0)
		return;				/* ReBAR not available. Nothing to do. */
resize:

	if (requested_size <= current_size)
		return;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
	r = gb_pci_resize_resource(pdev, GB02MAC724, requested_size); 
#else
	r = pci_resize_resource(pdev, GB02MAC724, requested_size); 
#endif

	if (r != 0) {
		requested_size -= 1;
		goto resize;
	} else if (r == 0) {
		dev_info(&pdev->dev, "GB PCI BAR%d resized to %dM\n", GB02MAC724, 1 << requested_size);
	} else if (r == -ENOSPC) {
		gb_printf(KERN_ERR, "No address space to allocate resized BAR2.\n");
	} else if (r) {
		gb_printf(KERN_ERR, "BAR resizing failed with error.r:%d\n",r);
	}

	/* Re-attempt assignment of PCIe resources */
	pci_assign_unassigned_bus_resources(pdev->bus); 

	if (pci_resource_flags(pdev, GB02MAC724) & IORESOURCE_UNSET) {
		if (requested_size != current_size)	{
			/* Try to get the BAR back with the original size */
			requested_size = current_size;
			goto resize;
		}
		/* Something went horribly wrong and the kernel didn't manage to re-allocate BAR2.
			This is unlikely (because we had space before), but can happen. */
		gb_printf(KERN_ERR, "FATAL: Failed to re-allocate BAR2.\n");
	}
}

static int GB02FUNC179(struct pci_dev *pdev)
{
	int value;
	pci_read_config_dword(pdev, 0x1c, &value);
	if (pci_resource_flags(pdev, GB02MAC724) & (IORESOURCE_MEM | IORESOURCE_MEM_64)) {
			if (pci_resource_start(pdev, GB02MAC724) >= 0x100000000ULL && (value != 0x0))
				return 1;
	}
	return 0;
}

void GB02FUNC186(struct pci_dev *pdev)
{
	u32 pci_cmd;
	unsigned long vram_size;
	int ret,current_size,board_vram_size,requested_size,pf_bar_cap_size;
	vram_size = GB02FUNC456();
	current_size = GB02FUNC155(pci_resource_len(pdev, GB02MAC724)); 
	board_vram_size = GB02FUNC155(vram_size);
	pf_bar_cap_size = fls(GB02FUNC162(pdev, GB02MAC724)) - 1;
	requested_size = min(board_vram_size,pf_bar_cap_size);
	if (requested_size <= current_size)
		return;

	ret = GB02FUNC179(pdev);
	if (ret == 0) {
		gb_printf(KERN_ERR, "The platform is not enable above4G \n");
		//return;
	}

	/* First disable PCI memory decoding references */
	pci_read_config_dword(pdev, PCI_COMMAND, &pci_cmd);
	pci_write_config_dword(pdev, PCI_COMMAND,
						   pci_cmd & ~PCI_COMMAND_MEMORY);

	GB02FUNC174(pdev, vram_size,current_size,requested_size);

	pci_assign_unassigned_bus_resources(pdev->bus);
	pci_write_config_dword(pdev, PCI_COMMAND, pci_cmd);
}