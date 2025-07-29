/*
 * Copyright (c) 2016 Synopsys, Inc.
 *
 * Synopsys DP TX Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "gb_dp_dptx.h"

/*
 * Core Access Layer
 *
 * Provides low-level register access to the DPTX core.
 */

/**
 * GB02FUNC1050() - Enables interrupts
 * @dptx: The dptx struct
 * @bits: The interrupts to enable
 *
 * This function enables (unmasks) all interrupts in the INTERRUPT
 * register specified by @bits.
 */
static void GB02FUNC1050(struct dptx *dptx, u32 bits)
{
	u32 ien;

	dptx_dbg(dptx, "%s:\n", __func__);
	ien = dptx_readl(dptx, GB02MAC2127);
	ien |= bits;
	dptx_writel(dptx, GB02MAC2127, ien);
	//dptx_writel(dptx, 0x60000, 1);
}

/**
 * GB02FUNC1053() - Disables interrupts
 * @dptx: The dptx struct
 * @bits: The interrupts to disable
 *
 * This function disables (masks) all interrupts in the INTERRUPT
 * register specified by @bits.
 */
static void GB02FUNC1053(struct dptx *dptx, u32 bits)
{
	u32 ien;

	dptx_dbg(dptx, "%s:\n", __func__);
	ien = dptx_readl(dptx, GB02MAC2127);
	ien &= ~bits;
	dptx_writel(dptx, GB02MAC2127, ien);
}

/**
 * GB02FUNC1055() - Enables top-level interrupts
 * @dptx: The dptx struct
 *
 * Enables (unmasks) all top-level interrupts.
 */
void GB02FUNC1055(struct dptx *dptx)
{
	GB02FUNC1050(dptx, GB02MAC2240 &
		~(GB02MAC2224 | GB02MAC2226 |
			GB02MAC2239));
}

/**
 * GB02FUNC1057() - Disables top-level interrupts
 * @dptx: The dptx struct
 *
 * Disables (masks) all top-level interrupts.
 */
void GB02FUNC1057(struct dptx *dptx)
{
	GB02FUNC1053(dptx, GB02MAC2240);
}

/**
 * GB02FUNC1059() - Performs a core soft reset
 * @dptx: The dptx struct
 * @bits: The components to reset
 *
 * Resets specified parts of the core by writing @bits into the core
 * soft reset control register and clearing them 10-20 microseconds
 * later.
 */
void GB02FUNC1059(struct dptx *dptx, u32 bits)
{
	u32 rst;

	bits &= (GB02MAC2171);

	/* Set reset bits */
	rst = dptx_readl(dptx, GB02MAC2059);
	rst |= bits;
	dptx_writel(dptx, GB02MAC2059, rst);

	usleep_range(10000, 20000);

	/* Clear reset bits */
	rst = dptx_readl(dptx, GB02MAC2059);
	rst &= ~bits;
	dptx_writel(dptx, GB02MAC2059, rst);
}

/**
 * GB02FUNC1061() - Reset all core modules
 * @dptx: The dptx struct
 */
void GB02FUNC1061(struct dptx *dptx)
{
	GB02FUNC1059(dptx, GB02MAC2171);
}

void GB02FUNC1063(struct dptx *dptx)
{
	GB02FUNC1059(dptx, GB02MAC2166);
}

/**
 * GB02FUNC1064() - Initializes the DP TX PHY module
 * @dptx: The dptx struct
 *
 * Initializes the PHY layer of the core. This needs to be called
 * whenever the PHY layer is reset.
 */
void GB02FUNC1064(struct dptx *dptx)
{
	u32 phyifctrl;

	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	phyifctrl &= ~GB02MAC2199;
	dptx_writel(dptx, GB02MAC2107, phyifctrl);
}


/**
* GB02FUNC1068() - Returns true, if sink is enabled ssc
* @dptx: The dptx struct
*
*/
bool GB02FUNC1068(struct dptx *dptx)
{
	u8 byte;

	GB02FUNC1151(dptx, DP_MAX_DOWNSPREAD, &byte);

	return byte & 1;
}

/**
 * GB02FUNC1069() - Move phy to P3 state and programs SSC
 * @dptx: The dptx struct
 *
 * Enables SSC should be called during hot plug.
 *
 */
int GB02FUNC1069(struct dptx *dptx, bool sink_ssc)
{
	u32 phyifctrl;
        u8  retval;

	/* Enable 4 lanes, before programming SSC */
	GB02FUNC1087(dptx, 4);

	// Move PHY to P3 to program SSC
	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	phyifctrl |= (3 << GB02MAC2198);// move phy to P3 state
	dptx_writel(dptx, GB02MAC2107, phyifctrl);

	retval = GB02FUNC1095(dptx, GB02MAC1379);
	if (retval) {
		dptx_err(dptx, "Timed out waiting for PHY BUSY\n");
	        return retval;
	}

	phyifctrl = dptx_readl(dptx, GB02MAC2107);
        if(dptx->ssc_en && sink_ssc)
	        phyifctrl &= ~GB02MAC2197;
	else
		phyifctrl |= GB02MAC2197;

        dptx_writel(dptx, GB02MAC2107, phyifctrl);

	retval = GB02FUNC1095(dptx, GB02MAC1379);
	if (retval) {
	        dptx_err(dptx, "Timed out waiting for PHY BUSY\n");
	        return retval;
	 }

	return 0;
}

/**
 * GB02FUNC1072() - Check value of GB02MAC2050 register
 * @dptx: The dptx struct
 *
 * Returns True if DPTX core correctly identifyed.
 */
bool GB02FUNC1072(struct dptx *dptx)
{
	u32 dptx_id;

	dptx_id = dptx_readl(dptx, GB02MAC2050);
	if (dptx_id != ((GB02MAC2051 << GB02MAC2144) |
			GB02MAC2052))
		return false;

	return true;
}

/**
* GB02FUNC1076() - Enables SSC based on automation request,
*		      if DPTX controller enables ssc
* @dptx: The dptx struct
*
*/
void GB02FUNC1076(struct dptx *dptx)
{
	bool sink_ssc = GB02FUNC1068(dptx);
	if(sink_ssc)
		dev_dbg(dptx->dev, "%s: SSC enable on the sink side\n", __func__);
	else
		dev_dbg(dptx->dev, "%s: SSC disabled on the sink side\n", __func__);
	GB02FUNC1069(dptx, sink_ssc);
}

/* zfl:文档第3步，获取dptx配置参数 */
void GB02FUNC1078(struct dptx *dptx)
{
	u32 reg;

	reg = dptx_readl(dptx, GB02MAC2054);

	/* Num MST streams */
	dptx->streams = (reg & DPTX_CONFIG1_NUM_STREAMS_MASK) >>
		GB02MAC2148;

	/* Combo PHY     zfl?这参数干啥的，代码里也没用*/
	dptx->hwparams.gen2phy = !!(reg & GB02MAC2156);

	/* DSC   zfl:显示视频压缩技术*/
	dptx->hwparams.dsc = !!(reg & GB02MAC2154);

	/* Multi pixel mode 	zfl?这个是？*/
	switch ((reg & DPTX_CONFIG1_MP_MODE_MASK) >> GB02MAC2150) {
	default:
	case GB02MAC2151:
		dptx->hwparams.multipixel = GB02MAC2044;
		break;
	case GB02MAC2152:
		dptx->hwparams.multipixel = GB02MAC2045;
		break;
	case GB02MAC2153:
		dptx->hwparams.multipixel = GB02MAC2046;
		break;
	}
}

/**
 * GB02FUNC1079() - Initializes the DP TX core
 * @dptx: The dptx struct
 *
 * Initialize the DP TX core and put it in a known state.
 */
int GB02FUNC1079(struct dptx *dptx, bool is_edp)
{
	char str[15];
	u32 version;
	u32 hpd_ien;
	u32 reg;

	/* Reset the core */
	GB02FUNC1061(dptx);

	reg = dptx_readl(dptx, GB02MAC2057);
	if (is_edp) {
		/* enable edp */
		reg |= (1 << 27);
	}
	/* disable fec */
	reg &= (~(1 << 26));
	/* reset SCALE DOWN */
	reg &= (~(0x01FF << 16));
	/* disable fast train */
	reg &= (~(0x0001 << 2));
	/* ENABLE_SCRAMBLE */
	reg &= (~(0x0001 << 0));
	//CCTL
	dptx_writel(dptx, GB02MAC2057, reg);
#if 0
	/* CFG PHY POWERDOWN */
	reg = dptx_readl(dptx, GB02MAC2107);
	reg |= (3 << GB02MAC2198);// move phy to P3 state
	dptx_writel(dptx, GB02MAC2107, reg);
	ret = GB02FUNC1095(dptx, GB02MAC1379);
	if (ret) {
		dptx_err(dptx, "%s %d Timed out waiting for PHY BUSY\n",
			__func__, __LINE__);
		return ret;
	}

	/* CFG PHY POWERON */
	reg = dptx_readl(dptx, GB02MAC2107);
	reg &= (~DPTX_PHYIF_CTRL_LANE_PWRDOWN_MASK);// P0
	dptx_writel(dptx, GB02MAC2107, reg);
#endif
	/* Reset the core */
//	GB02FUNC1061(dptx);
#if 0
	/* Reset the phy */
	GB02FUNC1063(dptx);
	ret = GB02FUNC1095(dptx, GB02MAC1379);
	if (ret) {
		dptx_err(dptx, "%s %d Timed out waiting for PHY BUSY\n",
			__func__, __LINE__);
		return ret;
	}
#endif
	/* Check the core version */
	memset(str, 0, sizeof(str));
	version = dptx_readl(dptx, GB02MAC2047);
	str[0] = (version >> 24) & 0xff;
	str[1] = '.';
	str[2] = (version >> 16) & 0xff;
	str[3] = (version >> 8) & 0xff;
	str[4] = version & 0xff;

	version = dptx_readl(dptx, GB02MAC2049);
	str[5] = '-';
	str[6] = (version >> 24) & 0xff;
	str[7] = (version >> 16) & 0xff;
	str[8] = (version >> 8) & 0xff;
	str[9] = version & 0xff;

	dptx_dbg(dptx, "Core version: %s\n", str);
	dptx->version = version;
	GB02FUNC1064(dptx);

	/* Enable all HPD interrupts */
	//hpd_ien = dptx_readl(dptx, GB02MAC2129);
	hpd_ien = (GB02MAC2248 | GB02MAC2249 |
		    GB02MAC2250);

	dptx_writel(dptx, GB02MAC2129, hpd_ien);

#ifdef DP_HDCP_ENABLE
	/* Mask interrupt related to HDCP22 GPIO output status */
	reg = dptx_readl(dptx, GB02MAC2131);
	reg |= GB02MAC2345;
	dptx_writel(dptx, GB02MAC2131, reg);
#endif

	/* Enable all top-level interrupts */
	GB02FUNC1055(dptx);
#if 0
	/* AUX_250US_CNT_LIMIT */
	dptx_writel(dptx, GB02MAC2121, 0x000000fa);
	/* AUX_2000US_CNT_LIMIT */
	dptx_writel(dptx, GB02MAC2123, 0x000007d0);
	/* AUX_100000US_CNT_LIMIT */
	dptx_writel(dptx, GB02MAC2124, 0x000186a0);
#endif
	return 0;
}

/**
 * GB02FUNC1085() - Deinitialize the core
 * @dptx: The dptx struct
 *
 * Disable the core in preparation for module shutdown.
 */
int GB02FUNC1085(struct dptx *dptx)
{
	GB02FUNC1057(dptx);
	GB02FUNC1061(dptx);
	return 0;
}

/*
 * PHYIF core access functions
 */

unsigned int GB02FUNC1086(struct dptx *dptx)
{
	u32 phyifctrl;
	u32 val;

	dptx_dbg(dptx, "%s:\n", __func__);

	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	val = (phyifctrl & DPTX_PHYIF_CTRL_LANES_MASK) >>
		GB02MAC2184;

	return (1 << val);
}

void GB02FUNC1087(struct dptx *dptx, unsigned int lanes)
{
	u32 phyifctrl;
	u32 val;

	dptx_dbg(dptx, "%s: lanes=%d\n", __func__, lanes);

	switch (lanes) {
	case 1:
		val = 0;
		break;
	case 2:
		val = 1;
		break;
	case 4:
		val = 2;
		break;
	default:
		dptx_err(dptx, "Invalid number of lanes %d\n", lanes);
		return;
	}

	phyifctrl = 0;
	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	phyifctrl &= ~DPTX_PHYIF_CTRL_LANES_MASK;
	phyifctrl |= (val << GB02MAC2184);
	dptx_writel(dptx, GB02MAC2107, phyifctrl);
}

void GB02FUNC1090(struct dptx *dptx, unsigned int rate)
{
	u32 phyifctrl;

	dptx_dbg(dptx, "%s: rate=%d\n", __func__, rate);

	phyifctrl = dptx_readl(dptx, GB02MAC2107);
#ifdef CONFIG_GB02_CTS
	switch (rate) {
	case GB02MAC2185:

		dptx_writel(dptx, 0xc14, 0x00000001);
		dptx_writel(dptx, 0xc18, 0x00500051);
		dptx_writel(dptx, 0xc1c, 0x001e600a);
		dptx_writel(dptx, 0xc20, 0x00000000);
	//	dptx_writel(dptx, 0xc3c, 0x00000006);
                dptx_writel(dptx, 0xc3c, 0x00000000);
	//	dptx_writel(dptx, 0xc40, 0x01010014);
                dptx_writel(dptx, 0xc40, 0x0101002f);
		dptx_writel(dptx, 0xc44, 0x00000777);
                break;
	case GB02MAC2186:

                dptx_writel(dptx, 0xc14, 0x00000001);
		dptx_writel(dptx, 0xc18, 0x00400051);
		dptx_writel(dptx, 0xc1c, 0x0018a01e);
		dptx_writel(dptx, 0xc20, 0x00000000);
		dptx_writel(dptx, 0xc3c, 0x00000006);
                dptx_writel(dptx, 0xc3c, 0x00000000);
	//	dptx_writel(dptx, 0xc40, 0x01010014);
                dptx_writel(dptx, 0xc40, 0x0101002f);
		dptx_writel(dptx, 0xc44, 0x00000777);

		break;
	case GB02MAC2187:

                dptx_writel(dptx, 0xc14, 0x00000001);
		dptx_writel(dptx, 0xc18, 0x00400051);
		dptx_writel(dptx, 0xc1c, 0x0018a01e);
		dptx_writel(dptx, 0xc20, 0x00000000);
		dptx_writel(dptx, 0xc3c, 0x00000006);
                dptx_writel(dptx, 0xc3c, 0x00000000);
	//	dptx_writel(dptx, 0xc40, 0x01010014);
                dptx_writel(dptx, 0xc40, 0x0101002f);
		dptx_writel(dptx, 0xc44, 0x00000777);

		break;
	case GB02MAC2188:

		dptx_writel(dptx, 0xc14, 0x00000001);
		dptx_writel(dptx, 0xc18, 0x00100051);
		dptx_writel(dptx, 0xc1c, 0x0015c045);
		dptx_writel(dptx, 0xc20, 0x00000000);
		dptx_writel(dptx, 0xc3c, 0x00000006);
                dptx_writel(dptx, 0xc3c, 0x00000000);
	//	dptx_writel(dptx, 0xc40, 0x01010014);
                dptx_writel(dptx, 0xc40, 0x0101002f);
		dptx_writel(dptx, 0xc44, 0x00000777);
		break;
	default:
		dptx_err(dptx, "Invalid PHY rate %d\n", rate);
                dptx_writel(dptx, 0xc14, 0x00000001);
		dptx_writel(dptx, 0xc18, 0x00500051);
		dptx_writel(dptx, 0xc1c, 0x001e600a);
		dptx_writel(dptx, 0xc20, 0x00000000);
		dptx_writel(dptx, 0xc3c, 0x00000006);
                dptx_writel(dptx, 0xc3c, 0x00000000);
	//	dptx_writel(dptx, 0xc40, 0x01010014);
		dptx_writel(dptx, 0xc40, 0x0101002f);
		dptx_writel(dptx, 0xc44, 0x00000777);
		break;
	}
#endif
	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	phyifctrl &= ~DPTX_PHYIF_CTRL_RATE_MASK;
	phyifctrl |= rate << GB02MAC2183;
	dptx_writel(dptx, GB02MAC2107, phyifctrl);
}

unsigned int GB02FUNC1093(struct dptx *dptx)
{
	u32 phyifctrl;
	u32 rate;

	dptx_dbg(dptx, "%s:\n", __func__);

	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	rate = (phyifctrl & DPTX_PHYIF_CTRL_RATE_MASK) >>
		GB02MAC2183;

	return rate;
}

int GB02FUNC1095(struct dptx *dptx, unsigned int lanes)
{
	unsigned int count;
	u32 phyifctrl;
	u32 mask = 0;

	dptx_dbg(dptx, "%s: lanes=%d\n", __func__, lanes);

	switch (lanes) {
	case 4:
		mask |= GB02MAC2196(3);
		mask |= GB02MAC2196(2);
		mask |= GB02MAC2196(1);
		mask |= GB02MAC2196(0);
		break;
	case 2:
		mask |= GB02MAC2196(1);
		mask |= GB02MAC2196(0);
		break;
	case 1:
		mask |= GB02MAC2196(0);
		break;
	default:
		dptx_err(dptx, "Invalid number of lanes %d\n", lanes);
		break;
	}

	count = 0;

	while (1) {
		phyifctrl = dptx_readl(dptx, GB02MAC2107);

		if (!(phyifctrl & mask))
			break;

		count++;
		if (count > 1000) {
			dptx_warn(dptx, "%s: PHY BUSY timed out\n", __func__);
			return -EBUSY;
		}

		udelay(5);
	}

	return 0;
}

void GB02FUNC1100(struct dptx *dptx,
			       unsigned int lane,
			       unsigned int level)
{
	u32 phytxeq;

	dptx_dbg(dptx, "%s: lane=%d, level=0x%x\n", __func__, lane, level);

	if (lane > 3) {
		dptx_err(dptx, "Invalid lane %d", lane);
		return;
	}

	if (level > 3) {
		dptx_err(dptx, "Invalid pre-emphasis level %d, using 3", level);
		level = 3;
	}

	phytxeq = dptx_readl(dptx, GB02MAC2108);
	phytxeq &= ~DPTX_PHY_TX_EQ_PREEMP_MASK(lane);
	phytxeq |= (level << GB02MAC2200(lane)) &
		DPTX_PHY_TX_EQ_PREEMP_MASK(lane);

	dptx_writel(dptx, GB02MAC2108, phytxeq);
}
void GB02FUNC1105(struct dptx *dptx,
			       unsigned int lane,
			       unsigned int pre,
			       unsigned int vsw)
{
	u32 eq_main;
	u32 eq_post;
	u32 vboost_lvl;

	eq_main = 0x209000; //0x9002
	eq_post = 0x309000; //0x9003
	vboost_lvl = 0x30000c;
	printk(KERN_INFO"%s-%d: lane=%d, pre=0x%x vsw=0x%x\n",
			__func__, __LINE__, lane, pre, vsw);

	if (lane > 3) {
		dptx_err(dptx, "Invalid lane %d", lane);
		return;
	}

	if (pre > 3) {
		dptx_err(dptx, "Invalid pre-emphasis level %d, using 3", pre);
		pre = 3;
	}

        switch (lane) {
	     case 0:
		     eq_main = 0x201000;
		     eq_post = 0x301000;
		     break;
             case 1:
		     eq_main = 0x201100;
		     eq_post = 0x301100;
		     break;
             case 2:
		     eq_main = 0x201200;
		     eq_post = 0x301200;
		     break;
	     case 3:
		     eq_main = 0x201300;
		     eq_post = 0x301300;
		     break;
             default:
	             break;
          }
	switch (pre){
	case 0:
		switch (vsw) {
		case 0:
			dptx_writel(dptx, 0x20000+eq_main, 0x99F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2040);
	                dptx_writel(dptx, 0x20000+vboost_lvl, 0x0240);
                      //  dptx_writel(dptx, 0x20000+vboost_lvl, 0x0280);
			break;
		case 1:
			dptx_writel(dptx, 0x20000+eq_main, 0xA5F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2040);
			dptx_writel(dptx, 0x20000+vboost_lvl, 0x0240);
			break;
		case 2:
			dptx_writel(dptx, 0x20000+eq_main, 0xB1F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2040);
			dptx_writel(dptx, 0x20000+vboost_lvl, 0x0240);
			break;
		case 3:
			dptx_writel(dptx, 0x20000+eq_main, 0xB1F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2040);
			dptx_writel(dptx, 0x20000+vboost_lvl, 0x03C0);
			break;
		}
		break;
	case 1:
		switch (vsw) {
		case 0:
			dptx_writel(dptx, 0x20000+eq_main, 0x9FF8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2640);
			dptx_writel(dptx, 0x20000+vboost_lvl,0x0240);
			break;
		case 1:
			dptx_writel(dptx, 0x20000+eq_main, 0xA9F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2840);
			dptx_writel(dptx, 0x20000+vboost_lvl,0x0280);

			break;
		case 2:
			dptx_writel(dptx, 0x20000+eq_main, 0xA9F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2840);
			dptx_writel(dptx, 0x20000+vboost_lvl,0x03C0);
			break;
		}
		break;
	case 2:
		switch (vsw) {
		case 0:
			dptx_writel(dptx, 0x20000+eq_main, 0xA5F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2C40);
			dptx_writel(dptx, 0x20000+vboost_lvl, 0x0280);
			break;
		case 1:
			dptx_writel(dptx, 0x20000+eq_main, 0xA5F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x2C40);
			dptx_writel(dptx, 0x20000+vboost_lvl, 0x03C0);
			break;
		}
		break;
	case 3:
		switch (vsw) {
		case 0:
			dptx_writel(dptx, 0x20000+eq_main, 0xA1F8);
			dptx_writel(dptx, 0x20000+eq_post, 0x3040);
			dptx_writel(dptx, 0x20000+vboost_lvl,0x03C0);
			break;
		}
		break;
	default:
		break;
	}
#if 0
	gb_printf(KERN_INFO"%s-%d: eq_main = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0x20000+eq_main));
	gb_printf(KERN_INFO"%s-%d: eq_post = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0x20000+eq_post));
	gb_printf(KERN_INFO"%s-%d: dptx_c14 = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0xc14));
	gb_printf(KERN_INFO"%s-%d: dptx_c18 = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0xc18));
        gb_printf(KERN_INFO"%s-%d: dptx_c1c = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0xc1c));
	gb_printf(KERN_INFO"%s-%d: dptx_c20 = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0xc20));
        gb_printf(KERN_INFO"%s-%d: dptx_c3c = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0xc3c));
	gb_printf(KERN_INFO"%s-%d: dptx_c40 = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0xc40));
        gb_printf(KERN_INFO"%s-%d: dptx_c44 = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0xc44));
	gb_printf(KERN_INFO"%s-%d: dptx_a00 = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0xa00));
        gb_printf(KERN_INFO"%s-%d: dpphy_12004c = 0x%x\n", __func__, __LINE__,
			dptx_readl(dptx, 0x12004c));
#endif
}

void GB02FUNC1114(struct dptx *dptx,
			 unsigned int lane,
			 unsigned int level)
{
	u32 phytxeq;

	dptx_dbg(dptx, "%s: lane=%d, level=0x%x\n", __func__, lane, level);

	if (lane > 3) {
		dptx_err(dptx, "Invalid lane %d", lane);
		return;
	}

	if (level > 3) {
		dptx_err(dptx, "Invalid vswing level %d, using 3", level);
		level = 3;
	}

	phytxeq = dptx_readl(dptx, GB02MAC2108);
	phytxeq &= ~DPTX_PHY_TX_EQ_VSWING_MASK(lane);
	phytxeq |= (level << GB02MAC2201(lane)) &
		DPTX_PHY_TX_EQ_VSWING_MASK(lane);

	dptx_writel(dptx, GB02MAC2108, phytxeq);
}

void GB02FUNC1118(struct dptx *dptx,
			  unsigned int pattern)
{
	u32 phyifctrl = 0;

	dptx_dbg(dptx, "%s: Setting PHY pattern=0x%x\n", __func__, pattern);

	phyifctrl = dptx_readl(dptx, GB02MAC2107);
	phyifctrl &= ~DPTX_PHYIF_CTRL_TPS_SEL_MASK;
	phyifctrl |= ((pattern << GB02MAC2172) &
		      DPTX_PHYIF_CTRL_TPS_SEL_MASK);
	dptx_writel(dptx, GB02MAC2107, phyifctrl);
}

void GB02FUNC1121(struct dptx *dptx, unsigned int lanes, bool enable)
{
	u32 phyifctrl;
	u32 mask = 0;

	dptx_dbg(dptx, "%s: lanes=%d, enable=%d\n", __func__, lanes, enable);

	phyifctrl = dptx_readl(dptx, GB02MAC2107);

	switch (lanes) {
	case 4:
		mask |= GB02MAC2195(3);
		mask |= GB02MAC2195(2);
		mask |= GB02MAC2195(1);
		mask |= GB02MAC2195(0);
		break;
	case 2:
		mask |= GB02MAC2195(1);
		mask |= GB02MAC2195(0);
		break;
	case 1:
		mask |= GB02MAC2195(0);
		break;
	default:
		dptx_err(dptx, "Invalid number of lanes %d\n", lanes);
		break;
	}

	if (enable)
		phyifctrl |= mask;
	else
		phyifctrl &= ~mask;

	dptx_writel(dptx, GB02MAC2107, phyifctrl);
}

int GB02FUNC1125(unsigned int rate)
{
	switch (rate) {
	case GB02MAC2185:
		return DP_LINK_BW_1_62;
	case GB02MAC2186:
		return DP_LINK_BW_2_7;
	case GB02MAC2187:
		return DP_LINK_BW_5_4;
	case GB02MAC2188:
		return DP_LINK_BW_8_1;
	case GB02MAC2189:
	case GB02MAC2190:
	case GB02MAC2191:
	case GB02MAC2192:
		return DP_LINK_RATE_TABLE;
	default:
		dptx_err(dptx, "Invalid rate 0x%x\n", rate);
		return -EINVAL;
	}
}

u8 GB02FUNC1126(unsigned int bw)
{
	switch (bw) {
	case DP_LINK_BW_1_62:
		return GB02MAC2185;
	case DP_LINK_BW_2_7:
		return GB02MAC2186;
	case DP_LINK_BW_5_4:
		return GB02MAC2187;
	case DP_LINK_BW_8_1:
		return GB02MAC2188;
	default:
		dptx_err(dptx, "Invalid bw 0x%x\n", bw);
		return -EINVAL;
	}
}
