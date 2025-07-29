#ifndef	__GBDC_IRQ_H__
#define	__GBDC_IRQ_H__
#include <linux/interrupt.h>
int GB02FUNC1737(struct GB02STR39 *gb_dev, int crtc_id, u32 height1, u32 height2);
irqreturn_t GB02FUNC1754(int irq, void *dev, int crtc_id);
int GB02FUNC1759(int crtc_id);
int GB02FUNC1741(struct GB02STR155 *gb_dev, struct drm_crtc *crtc, bool enable);
int GB02FUNC1743(struct GB02STR155 *gb_dev, struct drm_crtc *crtc, bool enable);
void GB02FUNC1747(unsigned long data);
#endif
