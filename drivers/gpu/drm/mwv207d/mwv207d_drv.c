/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/fb.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_print.h>
#include <drm/drm_gem.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_aperture.h>
#include <drm/drm_atomic_helper.h>

#include "mwv207d_drv.h"
#include "mwv207d_drm.h"
#include "mwv207d_gem.h"
#include "mwv207d_bo.h"
#include "mwv207d_vm.h"
#include "mwv207d_ctx.h"
#include "mwv207d_submit.h"
#include "mwv207d_sched.h"
#include "mwv207d_irq.h"
#include "mwv207d_db.h"
#include "mwv207d_info.h"
#include "dc/mwv207d_kms.h"
#include "dc/mwv207d_vkms.h"
#include "mwv207d_debugfs.h"
#include "dc/mwv207d_audio.h"

#define DRIVER_VERSION  __stringify(DRIVER_MAJOR) "." __stringify(DRIVER_MINOR) "." __stringify(DRIVER_PATCHLEVEL)

static int selftest;
module_param(selftest, int, 0644);
MODULE_PARM_DESC(selftest, "run selftest when startup");

static int mwv207d_skip_thaw_kms = 1;
module_param(mwv207d_skip_thaw_kms, int, 0644);
MODULE_PARM_DESC(mwv207d_skip_thaw_kms, "skip_thaw_kms when s4");

static int isr_poll;
module_param(isr_poll, int, 0444);
MODULE_PARM_DESC(isr_poll, "choose polling over interrupt");

static int renderonly;
module_param(renderonly, int, 0444);
MODULE_PARM_DESC(renderonly, "no kms register if set 1");

static int pgtable_segment_size = 64;
module_param(pgtable_segment_size, int, 0444);
MODULE_PARM_DESC(pgtable_segment_size,
		"size of page table segment in MB");

static const struct file_operations mwv207d_driver_fops = {
	.owner		= THIS_MODULE,
	.open		= drm_open,
	.mmap		= drm_gem_mmap,
	.unlocked_ioctl	= drm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= drm_compat_ioctl,
#endif
	.poll		= drm_poll,
	.read		= drm_read,
	.llseek		= no_llseek,
	.release	= drm_release,
};

static const struct drm_ioctl_desc mwv207d_ioctls_drm[] = {
	DRM_IOCTL_DEF_DRV(MWV207D_INFO, mwv207d_db_ioctl,
			  DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207D_GEM_CREATE, mwv207d_gem_create_ioctl,
			  DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207D_GEM_MMAP, mwv207d_gem_mmap_ioctl,
			  DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207D_GEM_VA, mwv207d_gem_va_ioctl,
			  DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207D_GEM_WAIT, mwv207d_gem_wait_ioctl,
			  DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207D_GEM_METADATA, mwv207d_gem_metadata_ioctl,
			  DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207D_CTX, mwv207d_ctx_ioctl,
			  DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207D_SUBMIT, mwv207d_submit_ioctl,
			  DRM_AUTH|DRM_RENDER_ALLOW),
};

static int mwv207d_driver_open(struct drm_device *dev,
			       struct drm_file *file_priv)
{
	struct mwv207d_device *mdev = drm_to_mdev(dev);
	struct mwv207d_fpriv *fpriv;
	int ret;

	fpriv = kzalloc(sizeof(*fpriv), GFP_KERNEL);
	if (!fpriv)
		return -ENOMEM;

	fpriv->vm = mwv207d_vm_create(mdev);
	if (IS_ERR(fpriv->vm)) {
		ret = PTR_ERR(fpriv->vm);
		goto free;
	}

	ret = mwv207d_pipe_attach_vm(mdev, fpriv->vm);
	if (ret)
		goto put_vm;

	mwv207d_ctx_mgr_init(dev, &fpriv->ctx_mgr);

	file_priv->driver_priv = fpriv;

	return 0;
put_vm:
	mwv207d_vm_put(fpriv->vm);
free:
	kfree(fpriv);
	return ret;
}

static void mwv207d_driver_postclose(struct drm_device *dev,
				     struct drm_file *file_priv)
{
	struct mwv207d_fpriv *fpriv = to_fpriv(file_priv);

	mwv207d_ctx_mgr_fini(dev, &fpriv->ctx_mgr);
	mwv207d_pipe_detach_vm(drm_to_mdev(dev), fpriv->vm);
	mwv207d_vm_put(fpriv->vm);
	kfree(fpriv);
}

static struct drm_driver mwv207d_drm_driver = {
	.driver_features	= DRIVER_MODESET | DRIVER_ATOMIC |
				  DRIVER_GEM | DRIVER_RENDER |
				  DRIVER_SYNCOBJ | DRIVER_SYNCOBJ_TIMELINE,
	.fops			= &mwv207d_driver_fops,
	.open                   = mwv207d_driver_open,
	.postclose              = mwv207d_driver_postclose,
	.dumb_create		= mwv207d_gem_dumb_create,
	.dumb_map_offset	= drm_gem_dumb_map_offset,
	.gem_prime_import_sg_table = mwv207d_gem_prime_import_sg_table,
	.prime_handle_to_fd     = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle     = drm_gem_prime_fd_to_handle,
	.ioctls                 = mwv207d_ioctls_drm,
	.num_ioctls             = ARRAY_SIZE(mwv207d_ioctls_drm),
	.name			= DRIVER_NAME,
	.desc			= DRIVER_DESC,
	.date			= DRIVER_DATE,
	.major			= DRIVER_MAJOR,
	.minor			= DRIVER_MINOR,
	.patchlevel             = DRIVER_PATCHLEVEL,
};

static int mwv207d_display_init(struct mwv207d_device *mdev)
{
	if (mdev->renderonly)
		return 0;
	if (mdev->hw.is_pf)
		return mwv207d_kms_init(mdev);
	return mwv207d_vkms_init(mdev);
}

static void mwv207d_display_fini(struct mwv207d_device *mdev)
{
	if (mdev->renderonly)
		return;
	if (mdev->hw.is_pf)
		mwv207d_kms_fini(mdev);
	else
		mwv207d_vkms_fini(mdev);
}

static int mwv207d_is_region_available(struct pci_dev *pdev, int bar)
{
	int ret;

	ret = pci_request_region(pdev, bar, "mwv207ddrmfb");
	if (ret == 0)
		pci_release_region(pdev, bar);

	return !!(ret == 0);
}

static int mwv207d_sync_firmwarefb_removal(struct pci_dev *pdev)
{
	int i;

	for (i = 0; i < 10; i++) {
		if (i > 0)
			msleep(i * 16);

		if (mwv207d_is_region_available(pdev, 0))
			return 0;
	}

	pci_err(pdev, "failed to remove firwarefb in %d ms", i * (i - 1) * 8);

	return -EBUSY;
}

static int mwv207d_pci_probe(struct pci_dev *pdev,
			     const struct pci_device_id *ent)
{
	struct mwv207d_device *mdev;
	int ret;

	dev_info(&pdev->dev, "%s <v%s %s>",
		 DRIVER_DESC, DRIVER_VERSION, DRIVER_DATE);

	if (!!renderonly) {
		dev_info(&pdev->dev, "renderonly mode, no kms register");
		mwv207d_drm_driver.driver_features &= ~DRIVER_MODESET;
	}

	ret = drm_aperture_remove_conflicting_pci_framebuffers(pdev,
			&mwv207d_drm_driver);
	if (ret)
		return ret;
	mdev = devm_drm_dev_alloc(&pdev->dev, &mwv207d_drm_driver,
				  struct mwv207d_device, base);
	if (!mdev)
		return -ENOMEM;

	mdev->dev = &pdev->dev;
	mdev->isr_poll = !!isr_poll;
	mdev->renderonly = !!renderonly;
	mdev->pgtable_segment_size = max_t(u64, pgtable_segment_size, 8) << 20;

	ret = dma_set_mask_and_coherent(mdev->dev, DMA_BIT_MASK(36));
	if (ret)
		return ret;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;
	pci_set_drvdata(pdev, mdev);
	pci_set_master(pdev);

	ret = mwv207d_sync_firmwarefb_removal(pdev);
	if (ret)
		goto clean_master;

	ret = mwv207d_db_init(mdev);
	if (ret)
		goto clean_master;
	ret = mwv207d_hw_init(mdev);
	if (ret)
		goto clean_db;
	ret = mwv207d_mm_init(mdev);
	if (ret)
		goto clean_hw;
	ret = mwv207d_display_init(mdev);
	if (ret)
		goto clean_mm;
	ret = mwv207d_sched_init(mdev);
	if (ret)
		goto clean_display;
	ret = mwv207d_kctx_init(mdev);
	if (ret)
		goto clean_sched;
	ret = mwv207d_sysfs_init(mdev);
	if (ret)
		goto clean_kctx;

	mwv207d_db_sort(mdev);

	if (selftest) {
		ret = mwv207d_test(mdev);
		if (ret)
			goto clean_sysfs;
	}

	ret = drm_dev_register(&mdev->base, 0);
	if (ret)
		goto clean_sysfs;
	ret = mwv207d_fbdev_init(mdev);
	if (ret)
		goto clean_drm;
	mwv207d_debugfs_init(mdev);

	return 0;

clean_drm:
	drm_dev_unregister(&mdev->base);
clean_sysfs:
	mwv207d_sysfs_fini(mdev);
clean_kctx:
	mwv207d_kctx_fini(mdev);
clean_sched:
	mwv207d_sched_fini(mdev);
clean_display:
	mwv207d_display_fini(mdev);
clean_mm:
	mwv207d_mm_fini(mdev);
clean_hw:
	mwv207d_hw_fini(mdev);
clean_db:
	mwv207d_db_fini(mdev);
clean_master:
	pci_clear_master(pdev);
	pci_set_drvdata(pdev, NULL);
	pr_err("mwv207d: failed to probe device, ret = %d", ret);

	return ret;
}

static void mwv207d_pci_remove(struct pci_dev *pdev)
{
	struct mwv207d_device *mdev = pci_get_drvdata(pdev);

	drm_dev_unregister(&mdev->base);
	mwv207d_sysfs_fini(mdev);
	mwv207d_kctx_fini(mdev);
	mwv207d_sched_fini(mdev);
	mwv207d_display_fini(mdev);
	mwv207d_mm_fini(mdev);
	mwv207d_hw_fini(mdev);
	mwv207d_db_fini(mdev);
	pci_clear_master(pdev);
	pci_set_drvdata(pdev, NULL);
}

static int mwv207d_pmops_suspend(struct device *dev)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	struct pci_dev *pdev = to_pci_dev(dev);
	int ret;

	dev_info(dev, "suspend entry");
	BUG_ON(!mdev->hw.is_pf);

	ret = mwv207d_kms_suspend(mdev);
	if (ret) {
		dev_err(dev, "failed to suspend kms: %d", ret);
		return ret;
	}

	mwv207d_audio_suspend(mdev);

	ret = mwv207d_mm_suspend(mdev);
	if (ret) {
		dev_err(dev, "failed to suspend mm: %d", ret);
		goto resume_audio;
	}

	ret = mwv207d_sched_suspend(mdev);
	if (ret) {
		dev_err(dev, "failed to suspend sched: %d", ret);
		goto resume_mm;
	}

	mwv207d_irq_suspend(mdev);

	mwv207d_hw_suspend(mdev);

	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_set_power_state(pdev, PCI_D3hot);
	dev_info(dev, "suspend exit");

	return 0;
resume_mm:
	mwv207d_mm_resume(mdev);
resume_audio:
	mwv207d_audio_resume(mdev);
	(void)mwv207d_kms_resume(mdev);
	return ret;
}

static int mwv207d_pmops_resume(struct device *dev)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);
	struct pci_dev *pdev = to_pci_dev(dev);
	int ret;

	dev_info(dev, "resume entry");
	BUG_ON(!mdev->hw.is_pf);

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	ret = pci_enable_device(pdev);
	if (ret) {
		dev_err(dev, "failed to enable pci device: %d", ret);
		return ret;
	}
	pci_set_master(pdev);

	if (mdev->hw.is_pf)
		wait_for(mdev_read(mdev, 0x34C120) &
			 0x4,
			 10 * 1000);

	mwv207d_hw_resume(mdev);
	mwv207d_irq_resume(mdev);
	mwv207d_mm_resume(mdev);
	mwv207d_sched_resume(mdev);
	mwv207d_audio_resume(mdev);

	ret = mwv207d_kms_resume(mdev);
	if (ret) {
		dev_err(dev, "failed to resume kms: %d", ret);
		return ret;
	}

	dev_info(dev, "resume exit");
	return 0;
}

static int mwv207d_pmops_thaw(struct device *dev)
{
	struct mwv207d_device *mdev = dev_get_drvdata(dev);

	mdev->skip_thaw_kms = mwv207d_skip_thaw_kms;
	return mwv207d_pmops_resume(dev);
}

static const struct dev_pm_ops mwv207d_pm_ops = {
	.suspend = mwv207d_pmops_suspend,
	.resume = mwv207d_pmops_resume,
	.freeze = mwv207d_pmops_suspend,
	.thaw = mwv207d_pmops_thaw,
	.poweroff = mwv207d_pmops_suspend,
	.restore = mwv207d_pmops_resume,
};

static const struct pci_device_id pciidlist[] = {
	{ 0x0731, 0x1100, 0x0731, 0x1101, 0, 0, 0 },
	{ 0x0731, 0x1100, 0x0731, 0x1102, 0, 0, 0 },
	{ 0x0731, 0x1100, 0x0731, 0x1103, 0, 0, 0 },
	{ 0x0731, 0x1100, 0x0731, 0x1104, 0, 0, 0 },
	{ 0x0731, 0x1100, 0x0731, 0x1106, 0, 0, 0 },
	{ 0x0731, 0x1100, 0x0731, 0x1107, 0, 0, 0 },
	{ 0x0731, 0x1100, 0x0731, 0x1108, 0, 0, 0 },
	{ 0x0731, 0x1109, 0x0731, 0x110F, 0, 0, 0 },
	{ 0x0731, 0xF011, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ 0x0731, 0xFF11, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ 0x0731, 0xF111, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ 0 },
};

MODULE_DEVICE_TABLE(pci, pciidlist);
static struct pci_driver mwv207d_pci_driver = {
	.name       = DRIVER_NAME,
	.id_table   = pciidlist,
	.probe      = mwv207d_pci_probe,
	.remove     = mwv207d_pci_remove,
	.driver.pm  = &mwv207d_pm_ops,
};

module_pci_driver(mwv207d_pci_driver);

MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
MODULE_DESCRIPTION(DRIVER_DESC);
