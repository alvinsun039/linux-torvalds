
#ifndef __GB02_DP_H__
#define __GB02_DP_H__

#include <linux/types.h>
#include "gb_dp_dptx.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_dp_helper.h>
#else
#include <drm/display/drm_dp.h>
#endif

#define GB02MAC1564  0x00000
#define GB02MAC1566  0x10000
#define GB02MAC1568   0x20000
#define GB02MAC1570  0x30000
#define GB02MAC1572   0x40000
#define GB02MAC1574  0x50000
#define GB02MAC1575   0x60000

#define GB02MAC1577              (0x0<<28)
#define GB02MAC1578             (0x1<<28)
#define GB02MAC1579       (0x2<<28)
#define GB02MAC1581              (0x4<<28)
#define GB02MAC1583              (0x5<<28)
#define GB02MAC1584   (0x6<<28)
#define GB02MAC1586           (0x8<<28)
#define GB02MAC1587           (0x9<<28)
#define GB02MAC1588		28

#define GB02MAC1590			0x0
#define GB02MAC1592			0x1

#define GB02MAC1593	0x040A
#define GB02MAC1594	0x040B
#define GB02MAC1595		0x1
#define GB02MAC1596		0x2

#ifndef fallthrough
#define fallthrough do {} while (0)
#endif

typedef union {
	uint32_t val;
	struct {
		uint32_t en:1;
		uint32_t bypass:1;
		uint32_t dsmen:1;
		uint32_t refdiv:6;
		uint32_t postdiv2:3;
		uint32_t postdiv1:3;
		uint32_t fbdiv:12;
		uint32_t extdiv:4;
		uint32_t extdiven:1;
	} data;
} tDCDP_PLLP_CFG0;

typedef union {
	uint32_t val;
	struct {
		uint32_t frac:24;
		uint32_t update:1;
		uint32_t rsvd:7;
	} data;
} tDCDP_PLLP_CFG1;

typedef union {
	uint32_t val;
	struct {
		uint32_t en:1;
		uint32_t bypass:1;
		uint32_t dsmen:1;
		uint32_t refdiv:6;
		uint32_t postdiv2:3;
		uint32_t postdiv1:3;
		uint32_t fbdiv:12;
		uint32_t rsvd:5;
	} data;
} tDCDP_PLLM_CFG0;

typedef union {
	uint32_t val;
	struct {
		uint32_t frac:24;
		uint32_t update:1;
		uint32_t rsvd:7;
	} data;
} tDCDP_PLLM_CFG1;

typedef union {
	uint32_t val;
	struct {
		uint32_t en:1;
		uint32_t bypass:1;
		uint32_t refdiv:6;
		uint32_t postdiv2:3;
		uint32_t postdiv1:3;
		uint32_t fbdiv:12;
		uint32_t pllbw:1;
		uint32_t rsvd:5;
	} data;
} tDCDP_PLLD_CFG0;

typedef union {
	uint32_t val;
	struct {
		uint32_t frac:24;
		uint32_t update:1;
		uint32_t rsvd:7;
	} data;
} tDCDP_PLLD_CFG1;

typedef union {
	uint32_t val;
	struct {
		uint32_t plld_locked:1;
		uint32_t pllm_locked:1;
		uint32_t pllp_locked:1;
		uint32_t rsvd:29;
	} data;
} tDCDP_PLL_STAT;

enum DPTX_AUDIO_STATUS {
	DPTX_AUDIO_START_PLAY = 0,
	DPTX_AUDIO_EDID_CONFIGURE,
	DPTX_AUDIO_MAX_VAL,
};

extern int GB02FUNC982(const struct drm_display_mode *mode,
	int dp_index);
extern bool GB02FUNC1500(int dp_index);
extern int GB02FUNC1001(int dp_index,
	struct drm_device *drm);
extern int GB02FUNC873(int dp_index);
extern int GB02FUNC879(int dp_index);
extern void GB02FUNC1505(struct work_struct *work);
extern int GB02FUNC988(struct dptx *dptx_info);
extern unsigned char *GB02FUNC1504(int dp_index);
extern bool GB02FUNC1503(int dp_index);
extern ssize_t GB02FUNC1030(struct drm_dp_aux *aux,
                                  struct drm_dp_aux_msg *msg);
void GB02FUNC1013(struct dptx *dptx_info);
int GB02FUNC1440(struct dptx *dptx, int set_info);
int GB02FUNC1041(struct GB02STR70 *gb_pcie);

#endif
