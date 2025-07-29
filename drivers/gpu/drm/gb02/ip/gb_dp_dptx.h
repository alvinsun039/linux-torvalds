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

#ifndef __DPTX_DRIVER_H__
#define __DPTX_DRIVER_H__

#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_dp_helper.h>
#include <drm/drm_dp_mst_helper.h>
#else
#include <drm/display/drm_dp.h>
#include <drm/display/drm_dp_mst_helper.h>
#endif
#include <drm/drm_fixed.h>
//#include "drm_dp_helper_additions.h"

//#define GB02_PAL_DP
#define GB02MAC1325		0x100
#define GB02MAC1326		0x10
#define GB02MAC1327	0x9
#define GB02MAC1328	(9 * 4)
//#define DPTX_TYPE_C
#define GB02MAC1329

//DSC definitions coming from RC files of DSC C model
#define GB02MAC1330			8192
#define GB02MAC1332			6144
#define GB02MAC1333			512
#define GB02MAC1335		3
#define GB02MAC1338		5
#define GB02MAC1341		3
#define GB02MAC1344	9
#define GB02MAC1346		24
#define GB02MAC1349		GB02MAC1346
#define GB02MAC1351			3
#define	GB02MAC1353			12
#define GB02MAC1355			6
#define GB02MAC1357		11
#define GB02MAC1359		11
#define GB02MAC1362		3
#define GB02MAC1365		3
#define GB02MAC1368	3
#define GB02MAC1370		11

struct GB02STR141 {
	int minQP	:5;
	int maxQP	:5;
	int offset	:6;
};


// Rounding up to the nearest multiple of a number
#define ROUND_UP_TO_NEAREST(numToRound, mult) \
	((((numToRound+(mult)-1) / (mult)) * (mult)))


#include "gb_dp_avgen.h"
#include "gb_dp_reg.h"
#include "gb_dp_dbg.h"

struct dptx;

/* The max rate and lanes supported by the core */
#define GB02MAC1378 GB02MAC2186
#define GB02MAC1379 4

/* The default rate and lanes to use for link training */
#define GB02MAC1382 GB02MAC1378
#define GB02MAC1384 GB02MAC1379

/**
 * struct GB02STR143 - The link state.
 * @status: Holds the sink status register values.
 * @trained: True if the link is successfully trained.
 * @rate: The current rate that the link is trained at.
 * @lanes: The current number of lanes that the link is trained at.
 * @preemp_level: The pre-emphasis level used for each lane.
 * @vswing_level: The vswing level used for each lane.
 */
struct GB02STR143 {
	u8 status[DP_LINK_STATUS_SIZE];
	bool trained;
	u8 rate;
	u8 lanes;
	u8 preemp_level[4];
	u8 vswing_level[4];
};

enum established_timings {
	DMT_640x480_60hz,
	DMT_800x600_60hz,
	DMT_1024x768_60hz,
	NONE
};

/**
 * struct GB02STR144 - The aux state
 * @sts: The AUXSTS register contents.
 * @data: The AUX data register contents.
 * @event: Indicates an AUX event ocurred.
 * @abort: Indicates that the AUX transfer should be aborted.
 */
struct GB02STR144 {
	u32 sts;
	u32 data[4];
	atomic_t abort;
};

struct GB02STR145 {
	u8 HB0;
	u8 HB1;
	u8 HB2;
	u8 HB3;
} __packed;

struct GB02STR147 {
	u8 en;
	u32 payload[9];
	u8 blanking;
	u8 cont;
} __packed;

#define GB02MAC1397	0x331c1169
#define GB02MAC1400	10

struct GB02STR148 {
	u32 lsb;
	u32 msb;
};

struct GB02STR149 {
	u32 lsb;
	u32 msb;
};

struct GB02STR150 {
	struct GB02STR148 aksv;
	struct GB02STR149 dpk[40];
	u32 enc_key;
	u32 crc32;
	u8 auth_fail_count;
	int hdcp13_is_en;
	int hdcp22_is_en;
};

/**
 * struct dptx - The representation of the DP TX core
 * @mutex: dptx mutex
 * @base: Base address of the registers
 * @irq: IRQ number
 * @version: Contents of the IP_VERSION register
 * @max_rate: The maximum rate that the controller supports
 * @max_lanes: The maximum lane count that the controller supports
 * @dev: The struct device
 * @root: The debugfs root
 * @regset: The debugfs regset
 * @vparams: The video params to use
 * @aparams: The audio params to use
 * @hparams: The HDCP params to use
 * @waitq: The waitq
 * @shutdown: Signals that the driver is shutting down and that all
 *            operations should be aborted.
 * @c_connect: Signals that a HOT_PLUG or HOT_UNPLUG has occurred.
 * @sink_request: Signals the a HPD_IRQ has occurred.
 * @rx_caps: The sink's receiver capabilities.
 * @edid: The sink's EDID.
 * @sdp_list: The array of SDP elements
 * @aux: AUX channel state for performing an AUX transfer.
 * @link: The current link state.
 * @multipixel: Controls multipixel configuration. 0-Single, 1-Dual, 2-Quad.
 */
typedef struct dptx {
	struct mutex mutex; /* generic mutex for dptx */

	struct {
		u8 multipixel;
		u8 streams;
		bool gen2phy;
		bool dsc;
	} hwparams;

	void __iomem *base;
	int irq;

	u32 version;
	int index;
	u8 max_rate;
	u8 max_lanes;
	bool ycbcr420;
	u8 streams;

	bool mst;
	int active_mst_links;

	struct drm_dp_mst_topology_mgr mst_mgr;  //sahakyan

	bool cr_fail;// harutk need to remove

	u8 multipixel;
	u8 bstatus;

	bool ssc_en;
	bool fec;
	bool dsc;

	int vcp_id;
	u16 pbn;
	bool need_rad;
	bool logic_port;
	u8 rad_port;
	u8 port[2];

	bool dummy_dtds_present;
	enum established_timings selected_est_timing;

	struct device *dev;
	struct dentry *root;
	struct debugfs_regset32 *regset;

	struct GB02STR124 vparams;
	struct GB02STR122 aparams;
	struct GB02STR123 audio_desc;
	struct GB02STR150 hparams;

	wait_queue_head_t waitq;

	atomic_t shutdown;
	atomic_t c_connect;
	atomic_t sink_request;

	u8 rx_caps[GB02MAC1325];

	u8 *edid;
	u8 *edid_second;
#define GB02MAC1428 128

	struct GB02STR147 sdp_list[GB02MAC1326];
	struct GB02STR144 aux;
	struct GB02STR143 link;
	struct drm_device *drm_dev;
	struct delayed_work hotplug_work;
	bool is_edp;
	bool edid_getted;
}dptx_s;

/*
 * Core interface functions
 */
int GB02FUNC1079(struct dptx *dptx, bool is_edp);
void GB02FUNC1078(struct dptx *dptx);
bool GB02FUNC1072(struct dptx *dptx);
void GB02FUNC1064(struct dptx *dptx);
int GB02FUNC1069(struct dptx *dptx, bool sink_ssc);
bool GB02FUNC1068(struct dptx *dptx);
void GB02FUNC1076(struct dptx *dptx);

int GB02FUNC1085(struct dptx *dptx);
void GB02FUNC1059(struct dptx *dptx, u32 bits);
void GB02FUNC1061(struct dptx *dptx);
void GB02FUNC1063(struct dptx *dptx);

irqreturn_t dptx_irq(int irq, void *dev);
irqreturn_t dptx_threaded_irq(int irq, void *dev);

void GB02FUNC1055(struct dptx *dp);
void GB02FUNC1057(struct dptx *dp);
void GB02FUNC659(struct dptx *dptx);

/*
 * PHY IF Control
 */
void GB02FUNC1087(struct dptx *dptx, unsigned int num);
unsigned int GB02FUNC1086(struct dptx *dptx);
void GB02FUNC1090(struct dptx *dptx, unsigned int rate);
unsigned int GB02FUNC1093(struct dptx *dptx);
int GB02FUNC1095(struct dptx *dptx, unsigned int lanes);
void GB02FUNC1105(struct dptx *dptx,
			       unsigned int lane,
			       unsigned int pre,
			       unsigned int vsw);
void GB02FUNC1100(struct dptx *dptx,
	unsigned int lane, unsigned int level);
void GB02FUNC1114(struct dptx *dptx,
	unsigned int lane, unsigned int level);
void GB02FUNC1118(struct dptx *dptx,
			  unsigned int pattern);
void GB02FUNC1121(struct dptx *dptx, unsigned int lane, bool enable);

int GB02FUNC1125(unsigned int rate);
u8 GB02FUNC1126(unsigned int bw);

int GB02FUNC1562(struct dptx *dptx);
int GB02FUNC1536(struct dptx *dptx);
int GB02FUNC1565(struct dptx *dptx);
int GB02FUNC1149(struct dptx *dptx,
				   u8 rate,
				   u8 lanes);
/*
 * AUX Channel
 */

#define GB02MAC1447 2000

int GB02FUNC372(struct dptx *dptx,
	unsigned int device_addr, u8 *bytes, u32 len);

int GB02FUNC375(struct dptx *dptx,
	unsigned int device_addr);

int GB02FUNC376(struct dptx *dptx,
	unsigned int device_addr, u8 *bytes, u32 len);

int GB02FUNC382(struct dptx *dptx, u32 addr, u8 *byte);
int GB02FUNC385(struct dptx *dptx, u32 addr, u8 byte);

int GB02FUNC378(struct dptx *dptx,
				unsigned int reg_addr,
				u8 *bytes, u32 len);

int GB02FUNC380(struct dptx *dptx,
			       unsigned int reg_addr,
			       u8 *bytes, u32 len);

#ifndef GB02MAC1323
static inline int GB02FUNC1151(struct dptx *dptx, u32 addr, u8 *byte)
{
	return GB02FUNC382(dptx, addr, byte);
}

static inline int GB02FUNC1153(struct dptx *dptx, u32 addr, u8 byte)
{
	return GB02FUNC385(dptx, addr, byte);
}

static inline int GB02FUNC1154(struct dptx *dptx,
					    unsigned int reg_addr,
					    u8 *bytes, u32 len)
{
	return GB02FUNC378(dptx, reg_addr, bytes, len);
}

static inline int GB02FUNC1156(struct dptx *dptx,
					   unsigned int reg_addr,
					   u8 *bytes, u32 len)
{
	return GB02FUNC380(dptx, reg_addr, bytes, len);
}

#else
#define GB02FUNC1151(_dptx, _addr, _byteptr) ({		\
	int _ret = GB02FUNC382(_dptx, _addr, _byteptr);	\
	dptx_dbg(dptx, "%s: DPCD Read %s(0x%03x) = 0x%02x\n",	\
		 __func__, #_addr, _addr, *(_byteptr));		\
	_ret;							\
})

#define GB02FUNC1153(_dptx, _addr, _byte) ({			\
	int _ret;						\
	dptx_dbg(dptx, "%s: DPCD Write %s(0x%03x) = 0x%02x\n",	\
		 __func__, #_addr, _addr, _byte);		\
	_ret = GB02FUNC385(_dptx, _addr, _byte);		\
	_ret;							\
})

char *GB02FUNC1129(u8 *bytes, unsigned int n);

#define GB02FUNC1154(_dptx, _addr, _b, _len) ({	\
	int _ret;						\
	char *_str;						\
	_ret = GB02FUNC378(_dptx,		\
					   _addr, _b, _len);	\
	_str = GB02FUNC1129(_b, _len);				\
	dptx_dbg(dptx, "%s: Read %llu bytes from %s(0x%02x) = [ %s ]\n", \
		 __func__, (u64)_len, #_addr, _addr, _str);		\
	_ret;							\
})

#define GB02FUNC1156(_dptx, _addr, _b, _len) ({	\
	int _ret;						\
	char *_str = GB02FUNC1129(_b, _len);			\
	dptx_dbg(dptx, "%s: Writing %llu bytes to %s(0x%02x) = [ %s ]\n", \
		 __func__, (u64)_len, #_addr, _addr, _str);		\
	_ret = GB02FUNC380(_dptx, _addr, _b, _len); \
	_ret;							\
})

#endif

/*
 * Link training
 */
int GB02FUNC1567(struct dptx *dptx, u8 rate, u8 lanes);
int GB02FUNC1585(struct dptx *dptx);

/*
 * Register read and write functions
 */
static inline u32 GB02FUNC1159(struct dptx *dp, char const *func,
	int line, u32 offset)
{
	u32 data = readl(dp->base + offset);
#ifdef DPTX_DEBUG_REG
	dptx_dbg(dp, "%s:%d: READ: base:0x%pK, addr=0x%05x data=0x%08x\n",
			func, line, dp->base, offset, data);
#endif

	return data;
}
static inline u32 GB02FUNC1162(struct dptx *dp, char const *func,
	int line, u32 offset)
{
	u32 data = readl(dp->base + 0x20000 + offset);
#ifdef DPTX_DEBUG_REG
	dptx_dbg(dp, "%s:%d: dpphyREAD: base:0x%pK, addr=0x%05x data=0x%08x\n",
			func, line, dp->base, offset, data);
#endif

	return data;
}


static inline void GB02FUNC1164(struct dptx *dp, char const *func, int line,
	u32 offset, u32 data)
{
#ifdef DPTX_DEBUG_REG
	u32 rdata = 0;
	dptx_dbg(dp, "%s:%d: WRITE: base:0x%pK, addr=0x%05x data=0x%08x\n",
			func, line, dp->base, offset, data);
#endif
	writel(data, dp->base + offset);
#ifdef DPTX_DEBUG_REG
	rdata = GB02FUNC1159(dp, func, line, offset);
	if (rdata != data)
		gb_printf(KERN_ERR, "%s-%d:ERR,base:0x%p,addr:0x%x,data:0x%x,rdata:0x%x\n",
		 func, line, dp->base, offset, data, rdata);
#endif
}
static inline void GB02FUNC1165(struct dptx *dp, char const *func, int line,
	u32 offset, u32 data)
{
#ifdef DPTX_DEBUG_REG
	dptx_dbg(dp, "%s:%d: ~~~~~~~dpphy WRITE: base:0x%pK, addr=0x%05x data=0x%08x\n",
			func, line, dp->base + 0x20000, offset, data);
#endif
	writel(data, dp->base + 0x20000 + offset);
}

#define dptx_readl(_dptx, _offset) ({ \
	GB02FUNC1159(_dptx, __func__, __LINE__, _offset); \
})
#define dpphy_readl(_dptx, _offset) ({ \
	GB02FUNC1162(_dptx, __func__, __LINE__, _offset); \
})

#define dptx_writel(_dptx, _offset, _data) do { \
	GB02FUNC1164(_dptx, __func__, __LINE__, _offset, _data); \
} while (0)
#define dpphy_writel(_dptx, _offset, _data) do { \
	GB02FUNC1165(_dptx, __func__, __LINE__, _offset, _data); \
} while (0)

/*
 * Wait functions
 */
#define dptx_wait(_dptx, _cond, _timeout)				\
	({								\
		int __retval;						\
		__retval = wait_event_interruptible_timeout(		\
			_dptx->waitq,					\
			((_cond) || (atomic_read(&_dptx->shutdown))),	\
			msecs_to_jiffies(_timeout));			\
		if (atomic_read(&_dptx->shutdown)) {			\
			__retval = -ESHUTDOWN;				\
		}							\
		else if (!__retval) {					\
			__retval = -ETIMEDOUT;				\
		}							\
		__retval;						\
	})
#define dptx_link_wait(_dptx, _cond, _timeout)				\
	({								\
		int __retval;						\
		__retval = wait_event_interruptible_timeout(		\
			_dptx->waitq,					\
			(_cond),	\
			msecs_to_jiffies(_timeout));			\
		__retval;						\
	})

void dptx_notify(struct dptx *dptx);
void dptx_notify_shutdown(struct dptx *dptx);

/* Type C */
void GB02FUNC1402(struct dptx *dptx);

/*
 * Debugfs
 */
void dptx_debugfs_init(struct dptx *dptx);
void dptx_debugfs_exit(struct dptx *dptx);

void GB02FUNC431(struct dptx *dptx, struct GB02STR147 *data);
int dptx_configure_hdcp13(struct dptx *dptx);
void dptx_en_dis_hdcp13(struct dptx *dptx, u8 enable);
void dptx_init_hdcp_keys(struct dptx *dptx);

int dptx_en_hdcp22(struct dptx *dptx);
void dptx_en_dis_hdcp22(struct dptx *dptx, u8 enable);

int GB02FUNC1483(struct dptx *dptx, u8 enable);
int GB02FUNC1479(struct dptx *dptx, u8 enable);
int GB02FUNC1481(struct dptx *dptx, u8 enable);

struct dptx *dptx_get_handle(void);

int GB02FUNC362(struct dptx *dptx,
		      bool rw,
		      bool i2c,
		      u32 addr,
		      u8 *bytes,
		      unsigned int len);
int GB02FUNC372(struct dptx *dptx,
			     u32 device_addr,
			     u8 *bytes,
			     u32 len);
int GB02FUNC376(struct dptx *dptx,
			    u32 device_addr,
			    u8 *bytes,
			    u32 len);
int GB02FUNC340(struct dptx *dptx,
			bool rw,
			bool i2c,
			bool mot,
			bool addr_only,
			u32 addr,
			u8 *bytes,
			unsigned int len);
static inline int GB02FUNC1171(struct dptx *dptx, u32 frequency, int stream)
{
	return 0;
};
int GB02FUNC1172(struct dptx *dptx, u32 audio_clock_freq,
	u16 oversample_factor);
/*
 * HDCP2.2
 */
void dptx_write_hdcp22_test_keys(struct dptx *dptx);

//#define DPTX_COMBO_PHY
// To enable audio during hotplug
void GB02FUNC399(struct dptx *dptx);

void GB02FUNC402(struct dptx *dptx);
void GB02FUNC1525(struct dptx *dptx);
int GB02FUNC1550(struct dptx *dptx);
int GB02FUNC1553(struct dptx *dptx);
/* EDID Audio Data Block */
#define GB02MAC1521		1
#define GB02MAC1523		2
#define EDID_TAG_MASK		GENMASK(7, 5)
#define GB02MAC1525		5
#define EDID_SIZE_MASK		GENMASK(4, 0)
#define GB02MAC1527		0

/* Established timing blocks */
#define GB02MAC1528	BIT(0)
#define GB02MAC1529	BIT(1)
#define GB02MAC1531	BIT(2)
#define GB02MAC1533	BIT(3)
#define GB02MAC1534	BIT(4)
#define GB02MAC1535	BIT(5)
#define GB02MAC1537	BIT(6)
#define GB02MAC1539	BIT(7)

#define GB02MAC1541	BIT(0)
#define GB02MAC1543	BIT(1)
#define GB02MAC1545	BIT(2)
#define GB02MAC1547	BIT(3)
#define GB02MAC1548	BIT(4)
#define GB02MAC1550	BIT(5)
#define GB02MAC1552	BIT(6)
#define GB02MAC1554	BIT(7)

#define GB02MAC1557	BIT(7)

#endif
