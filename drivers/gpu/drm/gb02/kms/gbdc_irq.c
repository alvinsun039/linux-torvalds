#include <linux/miscdevice.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drmP.h>
#else
#include <drm/drm_vblank.h>
#endif
#include "common/gb_common.h"
#include "common/gb_pcie_info.h"
#include "kms/device/gbdc_regs.h"
#include "kms/device/reg_ops.h"
#include "gbdc_mode.h"
#include "common/gb_common.h"
#include "gb_pcie_map.h"
#include "gbdc_fbdev.h"
#include "gbdc_pm.h"
#include "gbdc_mm.h"
#include "gbdc_connector.h"
#include "device/gb_dev_res.h"
#include "gb_kms.h"
#include "device/gbdc_device.h"
#include "gbdc_planes.h"
#include "gbdc_crtc.h"
#include "gbdc_encoder.h"
#include "gbdc_mode.h"
#include "gbdc_display.h"
#include "gbdc_drv.h"
#include "ip/gb_edma.h"
#include "ip/gb_v2vdma.h"
#include "gpu/gb_device.h"
#include "gpu/gb_ttm.h"
#include "gpu/gb_mmu.h"
#include "gpu/gb_regs.h"
#include "gbdc_irq.h"
#include "gbdc_infinity.h"

#define GB02MAC2603 6
static atomic_t irq_done[GB02MAC2603 + (GB02MAC2603/2)];

static void GB02FUNC1735(struct GB02STR39 *gb_dev, int crtc_id, void __iomem **dc_base, void __iomem **de_base)
{
	struct GB02STR249 *dc_config;
	struct GB02STR155 *dc_dev = gb_dev->gbdc_dev;
	dc_config = &dc_dev->pcie_info.dc_config[crtc_id];
	*dc_base = dc_config->dc_base;
	*de_base = dc_config->de_base;
}

int GB02FUNC1737(struct GB02STR39 *gb_dev, int crtc_id, u32 height1, u32 height2)
{
	void __iomem *dc_base;
	void __iomem *de_base;
	u32 value = 0;
	atomic_set(&irq_done[crtc_id], 1);
	GB02FUNC1735(gb_dev, crtc_id, &dc_base, &de_base);
	// enable vsync irq
	value = GB02FUNC730(de_base, GB02MAC582);
	value |= GB02MAC566;
	GB02FUNC733(de_base, GB02MAC582, value);
	return 0;
}

static int GB02FUNC1738(void __iomem *de_base, bool enable, u16 line)
{
	GB02FUNC733(de_base, GB02MAC583, GB02MAC566);
	if (enable) {
		GB02FUNC734(de_base, GB02MAC582,
					GB02MAC566);
	} else {
		GB02FUNC735(de_base, GB02MAC582,
					//GB02MAC566);
					0);
	}

	return 0;
}

int GB02FUNC1741(struct GB02STR155 *gb_dev, struct drm_crtc *crtc, bool enable)
{
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);
	struct drm_display_mode *mode;
	int crtc_id = gbdc_crtc->crtc_id;
	void __iomem *de_base = gb_dev->pcie_info.dc_config[crtc_id].de_base;

	gb_printf(KERN_INFO, "%s:enable=%d,crtcid%d\n", __func__, enable, crtc_id);
	if (crtc->state)
		mode = &crtc->state->mode;
	else
		mode = &crtc->hwmode;

	GB02FUNC1738(de_base, enable, mode->vdisplay);

	return 0;
}
int GB02FUNC1743(struct GB02STR155 *gb_dev, struct drm_crtc *crtc, bool enable)
{
	struct gbdc_crtc *gbdc_crtc = GB02FUNC1573(crtc);//phycrtc
	struct drm_display_mode *mode;
	int crtc_id = gbdc_crtc->crtc_id;
	void __iomem *de_base = gb_dev->pcie_info.dc_config[crtc_id].de_base;

	gb_printf(KERN_INFO, "%s: enable=%d,crtcid %d\n", __func__, enable,crtc_id);
	mode = &gbdc_crtc->adjusted_mode;

	dump_mode_more(&gbdc_crtc->adjusted_mode, false);

	GB02FUNC733(de_base, GB02MAC583, GB02MAC566);
	if (enable) {
		GB02FUNC733(de_base, GB02MAC581, GB02MAC566);
		GB02FUNC734(de_base, GB02MAC582,
					GB02MAC566);
	} else {
		GB02FUNC733(de_base, GB02MAC581, 0);
		GB02FUNC735(de_base, GB02MAC582,
					GB02MAC566);
	}
	GB02FUNC733(de_base, GB02MAC643, mode->vdisplay);


	return 0;
}

static void __maybe_unused GB02FUNC1746(struct gbdc_crtc *gbdc_crtc)
{
	static long last_time;
	long current_time = 0;
	int internel_time;
	struct timespec64 time_tv;
	struct GB02STR155 *gb_dev = GB02FUNC85(gbdc_crtc->base.dev->dev_private);
	int crtc_id;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);

	ktime_get_real_ts64(&time_tv);

	current_time = time_tv.tv_sec*1000 + time_tv.tv_nsec/1000000;

	internel_time = current_time - last_time;
	if (internel_time > 20)
		gb_printf(KERN_INFO, "%s irq period = %d ms\n", __func__, internel_time);
	last_time = current_time;

	crtc_id = GB02FUNC1904(gb_type, gbdc_crtc->crtc_id);
	drm_handle_vblank(gb_dev->drm_dev, crtc_id);
	spin_lock(&gb_dev->drm_dev->event_lock);
	if (gbdc_crtc->event) {
		drm_crtc_send_vblank_event(&gbdc_crtc->base, gbdc_crtc->event);
		drm_crtc_vblank_put(&gbdc_crtc->base);
		gbdc_crtc->event = NULL;
		//printk("!!!!!!!!!!!!!!!!!!!!!!!1drmA_crtc_send_vblank_event\n");
	}
	spin_unlock(&gb_dev->drm_dev->event_lock);
	if (test_and_clear_bit(GBDC_PENDING_FB_UNREF, &gbdc_crtc->pending))
		drm_flip_work_commit(&gbdc_crtc->gbdc_fb_unref_work, system_unbound_wq);
}
void GB02FUNC1747(unsigned long data)
{
	struct gbdc_crtc *gbdc_crtc = (struct gbdc_crtc *)data;

	static long last_time1;
	long current_time = 0;
	int internel_time = 0;
	struct timespec64 time_tv;
	struct timespec64 time2_tv;
	long cost_time = 0;
	struct GB02STR155 *gb_dev = GB02FUNC85(gbdc_crtc->base.dev->dev_private);
	int crtc_id = 0;
	//struct GB02STR70 *pcie_info = GB02FUNC518();
	//enum gb_board_type gb_type = GB02FUNC503(pcie_info);

	ktime_get_real_ts64(&time_tv);

	current_time = time_tv.tv_sec*1000 + time_tv.tv_nsec/1000000;

	internel_time = current_time - last_time1;
	if (internel_time > 20 && internel_time < 100) {
		gb_printf(KERN_INFO, "%s:crtc%d period = %dms\n", __func__, gbdc_crtc->crtc_id, internel_time);
	}
	last_time1 = current_time;

	crtc_id = drm_crtc_index(&gbdc_crtc->base);

	spin_lock(&gb_dev->drm_dev->event_lock);
	if (gbdc_crtc->event) {
		drm_crtc_send_vblank_event(&gbdc_crtc->base, gbdc_crtc->event);
		drm_crtc_vblank_put(&gbdc_crtc->base);
		gbdc_crtc->event = NULL;
	}
	spin_unlock(&gb_dev->drm_dev->event_lock);
	drm_handle_vblank(gb_dev->drm_dev, crtc_id);
	if (test_and_clear_bit(GBDC_PENDING_FB_UNREF, &gbdc_crtc->pending))
		drm_flip_work_commit(&gbdc_crtc->gbdc_fb_unref_work, system_unbound_wq);
	ktime_get_real_ts64(&time2_tv);

	cost_time = (time2_tv.tv_sec - time_tv.tv_sec) * 1000000 +
			(time2_tv.tv_nsec - time_tv.tv_nsec)/1000;
	if (cost_time > 1000)
		gb_printf(KERN_INFO, "%s:crtc%d,cost time:%ldus\n", __func__, crtc_id, cost_time);

}
static __maybe_unused struct gbdc_crtc *GB02FUNC1749(struct gbdc_crtc *phy_crtc)
{
	struct gbdc_crtc *entry,*temp;
	struct gbdc_crtc *virt_crtc = NULL;

	list_for_each_entry_safe(entry, temp, &phy_crtc->head, head) {
		if(entry->virt && entry->crtc_id > 5) {
			virt_crtc = entry;
			break;
		}
	}
	if(virt_crtc)
		pr_debug("%s: virt_crtcid %d\n",__func__,virt_crtc->crtc_id);
	else
		pr_warn("%s: no virt crtc\n",__func__);
	return virt_crtc;

}
struct gbdc_crtc *GB02FUNC1750(struct drm_device *dev,int crtc_id)
{
	struct drm_crtc *crtc;
	struct gbdc_crtc *gbdc_crtc;
	struct drm_vblank_crtc *vblank;
	bool virt_need_vsync = false;

	drm_for_each_crtc(crtc, dev) {
		gbdc_crtc = GB02FUNC1573(crtc);
		if (!gbdc_crtc->virt)
			continue;
		vblank = GB02FUNC1011(crtc);
		if (vblank->enabled && gbdc_crtc->vblank_id == crtc_id) {
			gbdc_crtc->virt_need_vsync = true;
			virt_need_vsync = true;
			break;
		}
	}
	if (!virt_need_vsync)
		return NULL;
	else
		return gbdc_crtc;

}
irqreturn_t GB02FUNC1754(int irq, void *dev, int crtc_id)
{
	u32 irq_status;
	void __iomem *dc_base;
	void __iomem *de_base;
	irqreturn_t retval = IRQ_NONE;
	struct GB02STR39 *gb_dev = dev;
	static long last_time0;
	long current_time = 0;
	int interval_time;
	struct timespec64 time_tv;
	int virt_id;
	struct gbdc_crtc *virt_crtc;
	struct GB02STR245 *kms_info = &gb_dev->gbdc_dev->kms_info;
	ktime_get_real_ts64(&time_tv);

	current_time = time_tv.tv_sec*1000 + time_tv.tv_nsec/1000000;

	interval_time = current_time - last_time0;
	if (interval_time > 20 && interval_time < 100)
		gb_printf(KERN_INFO, "%s:crtc%d,period=%dms\n", __func__, crtc_id, interval_time);
	last_time0 = current_time;

	GB02FUNC1735(gb_dev, crtc_id, &dc_base, &de_base);
	irq_status = GB02FUNC730(de_base, GB02MAC580);
	if (irq_status & GB02MAC566) {
		atomic_set(&irq_done[crtc_id], 0);
		GB02FUNC733(de_base, GB02MAC583, irq_status);
		if (kms_info->crtcs[crtc_id]) {
			if(GB02FUNC1691(gb_dev->ddev, crtc_id, NULL, true)) {
				pr_debug("%s: crtcid %d is virt crtc\n",__func__,crtc_id);
				virt_id = GB02FUNC1692(gb_dev->ddev, crtc_id, true);
				if(virt_id != -1) {
					if (crtc_id == kms_info->crtcs[virt_id]->vblank_id)
						tasklet_schedule(&kms_info->crtcs[virt_id]->task_vblank);
				//else
					tasklet_schedule(&kms_info->crtcs[crtc_id]->task_vblank);
				}
			}
			else {
				tasklet_schedule(&kms_info->crtcs[crtc_id]->task_vblank);
				if ((virt_crtc = GB02FUNC1750(gb_dev->ddev, crtc_id)) != NULL) {
					//pr_info("%s: virtcrtc need vsync vblank",__func__);
					tasklet_schedule(&kms_info->crtcs[virt_crtc->crtc_id]->task_vblank);
				}
			}

		}
		retval = IRQ_HANDLED;
	} else {
		atomic_set(&irq_done[crtc_id], 0);
		GB02FUNC733(de_base, GB02MAC583, irq_status);
	}

	return retval;
}

__attribute((unused)) int gbdc_disable_vsync_irq(struct GB02STR39 *gb_dev, int crtc_id)
{
	void __iomem *dc_base;
	void __iomem *de_base;
	GB02FUNC1735(gb_dev, crtc_id, &dc_base, &de_base);
	GB02FUNC733(de_base, GB02MAC582, 0x0);

	return 0;
}

__attribute((unused)) int gbdc_enable_vsync_irq(struct GB02STR39 *gb_dev, int crtc_id)
{
	void __iomem *dc_base;
	void __iomem *de_base;
	GB02FUNC1735(gb_dev, crtc_id, &dc_base, &de_base);
	GB02FUNC733(de_base, GB02MAC582, GB02MAC566);

	return 0;
}

int GB02FUNC1759(int crtc_id)
{
	while (atomic_read(&irq_done[crtc_id]));
	atomic_set(&irq_done[crtc_id], 1);
	return 0;
}
