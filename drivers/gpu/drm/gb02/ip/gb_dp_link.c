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
#include "gb_dp.h"
#include "common/gb_common.h"

static int GB02FUNC1519(struct dptx *dptx)
{
	return GB02FUNC1154(dptx, DP_LANE0_1_STATUS,
					 dptx->link.status,
					 DP_LINK_STATUS_SIZE);
}

static int GB02FUNC1521(struct dptx *dptx, bool *out_done)
{
	int retval;
	u8 byte;
	u32 reg;

	dptx_dbg(dptx, "%s:\n", __func__);

	if (!out_done) {
		dptx_err(dptx, "out_done is NULL\n");
		return -EINVAL;
	}

	*out_done = false;

	retval = GB02FUNC1151(dptx, DP_TRAINING_AUX_RD_INTERVAL, &byte);
	if (retval)
		return retval;

	reg = min_t(u32, (byte & 0x7f), 4);
	reg *= 4000;
	if (!reg)
		reg = 400;

	udelay(reg);

	retval = GB02FUNC1519(dptx);
	if (retval)
		return retval;

	*out_done = drm_dp_clock_recovery_ok(dptx->link.status,
					     dptx->link.lanes);

	dptx_dbg(dptx, "%s: CR_DONE = %d\n", __func__, *out_done);

	return 0;
}

static int GB02FUNC1523(struct dptx *dptx,
				      bool *out_cr_done,
				      bool *out_ch_eq_done)
{
	int retval;
	bool done;

	dptx_dbg(dptx, "%s:\n", __func__);

	if (!out_cr_done || !out_ch_eq_done) {
		dptx_err(dptx, "out_cr_done or out_ch_eq_done is NULL\n");
		return -EINVAL;
	}

	retval = GB02FUNC1521(dptx, &done);
	if (retval)
		return retval;

	*out_cr_done = false;
	*out_ch_eq_done = false;

	if (!done)
		return 0;

	*out_cr_done = true;
	*out_ch_eq_done = drm_dp_channel_eq_ok(dptx->link.status,
					       dptx->link.lanes);

	dptx_dbg(dptx, "%s: CH_EQ_DONE = %d\n", __func__, *out_ch_eq_done);

	return 0;
}

void GB02FUNC1525(struct dptx *dptx)
{
	unsigned int i;

	for (i = 0; i < dptx->link.lanes; i++) {
		u8 pe;
		u8 vs;

		pe = dptx->link.preemp_level[i];
		vs = dptx->link.vswing_level[i];

		GB02FUNC1100(dptx, i, pe);
		GB02FUNC1114(dptx, i, vs);
		GB02FUNC1105(dptx, i, pe, vs);
	}
}

int GB02FUNC1526(struct dptx *dptx)
{
	int retval;
	unsigned int i;
	u8 bytes[4] = { 0xff, 0xff, 0xff, 0xff };

	for (i = 0; i < dptx->link.lanes; i++) {
		u8 byte = 0;

		byte |= ((dptx->link.vswing_level[i] <<
			  DP_TRAIN_VOLTAGE_SWING_SHIFT) &
			 DP_TRAIN_VOLTAGE_SWING_MASK);

		if (dptx->link.vswing_level[i] == 3)
			byte |= DP_TRAIN_MAX_SWING_REACHED;

		byte |= ((dptx->link.preemp_level[i] <<
			  DP_TRAIN_PRE_EMPHASIS_SHIFT) &
			 DP_TRAIN_PRE_EMPHASIS_MASK);

		if (dptx->link.preemp_level[i] == 2)
			byte |= DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;

		bytes[i] = byte;
	}

	retval = GB02FUNC1156(dptx, DP_TRAINING_LANE0_SET, bytes,
					  dptx->link.lanes);
	if (retval)
		return retval;

	return 0;
}

int GB02FUNC1530(struct dptx *dptx, int *out_changed)
{
	int retval;
	unsigned int lanes;
	unsigned int i;
	u8 byte;
	u8 adj[4] = { 0, };
	int changed = false;

	lanes = dptx->link.lanes;

	switch (lanes) {
	case 4:
		retval = GB02FUNC1151(dptx, DP_ADJUST_REQUEST_LANE2_3, &byte);
		if (retval)
			return retval;

		adj[2] = byte & 0x0f;
		adj[3] = (byte & 0xf0) >> 4;
		fallthrough;
		// fall through
	case 2:
		// fall through
		fallthrough;
	case 1:
		retval = GB02FUNC1151(dptx, DP_ADJUST_REQUEST_LANE0_1, &byte);
		if (retval)
			return retval;

		adj[0] = byte & 0x0f;
		adj[1] = (byte & 0xf0) >> 4;
		break;
	default:
		dptx_err(dptx, "Invalid number of lanes %d\n", lanes);
		return -EINVAL;
	}

	/* Save the drive settings */
	for (i = 0; i < lanes; i++) {
		u8 vs = adj[i] & 0x3;
		u8 pe = (adj[i] & 0xc) >> 2;

		if (dptx->link.vswing_level[i] != vs)
			changed = true;

		dptx->link.vswing_level[i] = vs;
		dptx->link.preemp_level[i] = pe;
	}

	GB02FUNC1525(dptx);

	retval = GB02FUNC1526(dptx);
	if (retval)
		return retval;

	if (out_changed)
		*out_changed = changed;

	return 0;
}

int GB02FUNC1149(struct dptx *dptx,
				   u8 rate,
				   u8 lanes)
{
	u8 sink_max_rate;
	u8 sink_max_lanes;

	dptx_dbg(dptx, "%s: lanes=%d, rate=%d\n", __func__, lanes, rate);

	if (dptx->is_edp) {
		if ((rate < GB02MAC2189) || (rate > GB02MAC2192)) {
			gb_printf(KERN_WARNING, "Invalid rate %d\n", rate);
			rate = GB02MAC2192;
		}
	} else {
		if (rate > GB02MAC2188) {
			dptx_err(dptx, "Invalid rate %d\n", rate);
			rate = GB02MAC2185;
		}
	}

	if (!lanes || (lanes == 3) || (lanes > 4)) {
		dptx_err(dptx, "Invalid lanes %d\n", lanes);
		lanes = 1;
	}

	/* Initialize link parameters */
	memset(dptx->link.preemp_level, 0, sizeof(u8) * 4);
	memset(dptx->link.vswing_level, 0, sizeof(u8) * 4);
	memset(dptx->link.status, 0, DP_LINK_STATUS_SIZE);

	sink_max_lanes = drm_dp_max_lane_count(dptx->rx_caps);

	if (lanes > sink_max_lanes)
		lanes = sink_max_lanes;

	sink_max_rate = dptx->rx_caps[DP_MAX_LINK_RATE];
	sink_max_rate = GB02FUNC1126(sink_max_rate);

	if (rate > sink_max_rate)
		rate = sink_max_rate;

	dptx->link.lanes = lanes;
	dptx->link.rate = rate;
	dptx->link.trained = false;

	return 0;
}

int GB02FUNC1535(struct dptx *dptx, u8 pattern)
{
	int retval;

	retval = GB02FUNC1153(dptx, DP_TRAINING_PATTERN_SET, pattern);
	if (retval)
		return retval;

	return 0;
}

int GB02FUNC1536(struct dptx *dptx)
{
	int retval;
	u32 cctl;
	u8 byte;

	/* Initialize PHY */
#if 0
// Move PHY to P3 to program SSC -----------------------------------------------
	phyifctrl = dptx_readl(dptx, GB02MAC2107);

	phyifctrl |= (3 << GB02MAC2198);// move phy to P3 state

	dptx_writel(dptx, GB02MAC2107, phyifctrl);


	retval = GB02FUNC1095(dptx, GB02MAC1379);
        if (retval) {
		dptx_err(dptx, "Timed out waiting for PHY BUSY\n");
 		return retval;
	}

// -----------------------------------------------------------------------------
#endif

	GB02FUNC1087(dptx, dptx->link.lanes);
	GB02FUNC1090(dptx, dptx->link.rate);
#if 0
//--------------------------------------------- move the phy to p0
	phyifctrl = dptx_readl(dptx, GB02MAC2107);

	phyifctrl &= ~DPTX_PHYIF_CTRL_LANE_PWRDOWN_MASK;// P0

	dptx_writel(dptx, GB02MAC2107, phyifctrl);
// -------------------------------------------------------

	/* Wait for PHY busy */
	retval = GB02FUNC1095(dptx, GB02MAC1379);
	if (retval) {
		dptx_err(dptx, "Timed out waiting for PHY BUSY\n");
		return retval;
	}
#endif

	/* Set PHY_TX_EQ */
	GB02FUNC1525(dptx);

	GB02FUNC1118(dptx, GB02MAC2173);
	retval = GB02FUNC1535(dptx,
						DP_TRAINING_PATTERN_DISABLE);
	if (retval)
		return retval;

	GB02FUNC1121(dptx, dptx->link.lanes, true);

	retval = GB02FUNC1125(dptx->link.rate);
	if (retval < 0)
		return retval;

	byte = retval;
	retval = GB02FUNC1153(dptx, DP_LINK_BW_SET, byte);
	if (retval)
		return retval;

	byte = dptx->link.lanes;
	cctl = dptx_readl(dptx, GB02MAC2057);

	if (drm_dp_enhanced_frame_cap(dptx->rx_caps)) {
		byte |= DP_ENHANCED_FRAME_CAP;
		cctl |= GB02MAC2157;
	} else {
		cctl &= ~GB02MAC2157;
	}

	dptx_writel(dptx, GB02MAC2057, cctl);

	retval = GB02FUNC1153(dptx, DP_LANE_COUNT_SET, byte);
	if (retval)
		return retval;

	/* C10 PHY ... check if SSC is enabled and program DPCD*/
	if(dptx->ssc_en && GB02FUNC1068(dptx))
		byte = DP_SPREAD_AMP_0_5;
	else
		byte = 0;

	retval = GB02FUNC1153(dptx, DP_DOWNSPREAD_CTRL, byte);
	if (retval)
		return retval;
	msleep(150);
	byte = 1;
	retval = GB02FUNC1153(dptx, DP_MAIN_LINK_CHANNEL_CODING_SET, byte);
	if (retval)
		return retval;

	msleep(50);
	return 0;
}

int GB02FUNC1546(struct dptx *dptx, bool ch_eq)
{
	int i;
	int retval;
	int changed = 0;
	bool done = false;
#ifdef GB02_PAL_DP
	u8 byte = 7;

	GB02FUNC1153(dptx, 0x106, byte);
	GB02FUNC1153(dptx, 0x105, byte);
	GB02FUNC1153(dptx, 0x104, byte);
	GB02FUNC1153(dptx, 0x103, byte);
#endif
	dptx_dbg(dptx, "%s:\n", __func__);

	retval = GB02FUNC1521(dptx, &done);
	if (retval)
		return retval;

	if (done)
		return 0;
#ifdef GB02_PAL_DP
	else
		return -1;
#endif
	/* Try each adjustment setting 5 times */
	for (i = 0; i < 5; i++) {
		retval = GB02FUNC1530(dptx, &changed);
		if (retval)
			return retval;

		/* Reset iteration count if we changed settings */
		//if (changed)
		//	i = 0;

		retval = GB02FUNC1521(dptx, &done);
		if (retval)
			return retval;

		if (done)
			return 0;

		/* TODO check for all lanes? */
		/* Failed and reached the maximum voltage swing */
		if (dptx->link.vswing_level[0] == 3)
			return -EPROTO;
	}

	return -EPROTO;
}

int GB02FUNC1550(struct dptx *dptx)
{
	int retval;

	dptx_dbg(dptx, "%s:\n", __func__);

	GB02FUNC1118(dptx, GB02MAC2174);

	retval = GB02FUNC1535(dptx,
						DP_TRAINING_PATTERN_1 | 0x20);
	if (retval)
		return retval;

	return GB02FUNC1546(dptx, false);
}

int GB02FUNC1553(struct dptx *dptx)
{
	int retval;
	bool cr_done;
	bool ch_eq_done;
	unsigned int pattern = 0;
	unsigned int i;
	u8 dp_pattern = 0;

	dptx_dbg(dptx, "%s:\n", __func__);

	//switch (dptx->max_rate) {
	switch (dptx->link.rate) {
	case GB02MAC2188:
		if (drm_dp_tps4_supported(dptx->rx_caps)) {
			pattern = GB02MAC2177;
			dp_pattern = DP_TRAINING_PATTERN_4;
			break;
		}
		fallthrough;
		/* Fall through */
	case GB02MAC2187:
	case GB02MAC2192:
	/* review R324用哪个??? */
	case GB02MAC2191:
		if (drm_dp_tps3_supported(dptx->rx_caps)) {
			pattern = GB02MAC2176;
			dp_pattern = DP_TRAINING_PATTERN_3;
			break;
		}
		fallthrough;
		/* Fall through */
	case GB02MAC2185:
	case GB02MAC2186:
	case GB02MAC2190:
	case GB02MAC2189:
		pattern = GB02MAC2175;
		dp_pattern = DP_TRAINING_PATTERN_2;
		break;
	default:
		dptx_err(dptx, "Invalid rate %d\n", dptx->link.rate);
		return -EINVAL;
	}

	GB02FUNC1118(dptx, pattern);

	/* TODO this needs to be different for other versions of
	 * DPRX
	 */
	if (dp_pattern != DP_TRAINING_PATTERN_4) {
		retval = GB02FUNC1535(dptx,
						dp_pattern | 0x20);
	} else {
		retval = GB02FUNC1535(dptx,
						dp_pattern);

		dptx_dbg(dptx, "%s:  Enabling scrambling for TPS4\n",
			 __func__);
	}
	if (retval)
		return retval;

	for (i = 0; i < 6; i++) {
		retval = GB02FUNC1523(dptx,
						    &cr_done,
						    &ch_eq_done);

		if (retval)
			return retval;

                dptx->cr_fail = false;

		if (!cr_done) {
                        dptx->cr_fail = true;
			return -EPROTO;
                }

		if (ch_eq_done)
			return 0;

		retval = GB02FUNC1530(dptx, NULL);
		if (retval)
			return retval;
	}

	return -EPROTO;
}

int GB02FUNC1562(struct dptx *dptx)
{
	unsigned int rate = dptx->link.rate;

	switch (rate) {
	case GB02MAC2185:
		return -EPROTO;
	case GB02MAC2186:
		rate = GB02MAC2185;
		break;
	case GB02MAC2187:
		rate = GB02MAC2186;
		break;
	case GB02MAC2188:
		rate = GB02MAC2187;
		break;
	case GB02MAC2189:
		return -EPROTO;
	case GB02MAC2190:
		rate = GB02MAC2189;
		break;
	case GB02MAC2191:
		rate = GB02MAC2190;
		break;
	case GB02MAC2192:
		rate = GB02MAC2191;
		break;
	}

	dptx_dbg(dptx, "%s: Reducing rate from %d to %d\n",
		 __func__, dptx->link.rate, rate);
	dptx->link.rate = rate;
	return 0;
}

int GB02FUNC1565(struct dptx *dptx)
{
	unsigned int lanes;

	switch (dptx->link.lanes) {
	case 4:
		lanes = 2;
		break;
	case 2:
		lanes = 1;
		break;
	case 1:
	default:
		return -EPROTO;
	}

	dptx_dbg(dptx, "%s: Reducing lanes from %d to %d\n",
		 __func__, dptx->link.lanes, lanes);
	dptx->link.lanes = lanes;
    dptx->link.rate  = dptx->max_rate;
	return 0;
}

int GB02FUNC1567(struct dptx *dptx, u8 rate, u8 lanes)
{
	int retval,retval1;
	u8 byte;
	u32 hpd_sts;

	retval = GB02FUNC1149(dptx, rate, lanes);
	if (retval) {
		dev_err(NULL, "%s:%d--link trainint init fail\n",
			__func__, __LINE__);
		goto fail;
	}

again:
	dptx_dbg(dptx, "%s: Starting link training\n", __func__);
	retval = GB02FUNC1536(dptx);
	if (retval) {
		dev_err(NULL, "%s:%d--link trainint start fail---\n",
				__func__, __LINE__);
		goto fail;
	}

	retval = GB02FUNC1550(dptx);
	if (retval) {
		if (retval == -EPROTO) {
			if (GB02FUNC1562(dptx)) {
				/* TODO If CR_DONE bits for some lanes
				 * are set, we should reduce lanes to
				 * those lanes.
				 */
				if (GB02FUNC1565(dptx)) {
					dev_err(NULL, "%s:%d--link reduce lanes fail\n",
						__func__, __LINE__);
					retval = -EPROTO;
					goto fail;
				}
				else {
					/*
					 * Check clock recovery status (LANE0_CR_DONE bit) in LANE0_1_STATUS DPCD register
					 * and fail training, if clock recovery failed for Lane 0
					 */
					if (!(dptx->link.status[0] & 1)) {
						dev_err(NULL, "%s:%d---link reduce lanes status fail\n",
							__func__, __LINE__);
						goto fail;
					}
				}
			}

			GB02FUNC1149(dptx,
						dptx->link.rate,
						dptx->link.lanes);
			goto again;
		} else {
				dev_err(NULL, "%s:%d--link cr fail---\n",
					__func__, __LINE__);
        			goto fail;
		}
	}

	retval = GB02FUNC1553(dptx);
	if (retval) {
		if (retval == -EPROTO) {
                   if(!dptx->cr_fail) {
                     if(dptx->link.lanes == 1) {
                      if(GB02FUNC1562(dptx))  {
				dev_err(NULL, "%s:%d---link reduce rate fail--\n",
					__func__, __LINE__);
                     	      goto fail;
                      }
                      dptx->link.lanes = dptx->max_lanes;
                     } else {
                     GB02FUNC1565(dptx);
                    }
                   } else {
			if (GB02FUNC1562(dptx)) {
				if (GB02FUNC1565(dptx)) {
					retval = -EPROTO;
					dev_err(NULL, "%s:%d----link reduce lane fail---\n",
						__func__, __LINE__);
					goto fail;
				}
			}
                   }

			GB02FUNC1149(dptx,
						dptx->link.rate,
						dptx->link.lanes);
			goto again;
		} else {
			dev_err(NULL, "%s:%d--link ch eq fail---\n",
				__func__, __LINE__);
			goto fail;
		}
	}

	GB02FUNC1118(dptx, GB02MAC2173);

	retval = GB02FUNC1535(dptx,
						DP_TRAINING_PATTERN_DISABLE);
	if (retval) {
		dev_err(NULL, "%s:%d---link training pattern set fail\n",
			__func__, __LINE__);
		goto fail;
	}

	GB02FUNC789(dptx, 0);
	//GB02FUNC788(dptx, 0);
	GB02FUNC1121(dptx, dptx->link.lanes, true);
	dptx->link.trained = true;

	/* Branch device detection */
	retval = GB02FUNC1151(dptx, DP_SINK_COUNT, &byte);
	if (retval)
		return retval;

	retval = GB02FUNC1151(dptx, 0x2002, &byte);
	if (retval)
		return retval;

	GB02FUNC699(dptx, 0);
	dptx_dbg(dptx, "Link training succeeded rate=%d lanes=%d\n",
		 dptx->link.rate, dptx->link.lanes);

	return 0;

fail:
      hpd_sts = dptx_readl(dptx, GB02MAC2128);

      if(hpd_sts & GB02MAC2245)
      {
         GB02FUNC1118(dptx, GB02MAC2173);
         retval1 = GB02FUNC1535(dptx, DP_TRAINING_PATTERN_DISABLE);
	if (retval1)
		return retval1;

	dptx_err(dptx, "Link training failed %d\n", retval);

      } else {
	dptx_err(dptx, "Link training failed  as sink is disconnected %d\n", retval);

      }
	return retval;
}

int GB02FUNC1585(struct dptx *dptx)
{
	int retval;
	u8 bytes[2];

	retval = GB02FUNC1154(dptx, DP_SINK_COUNT,
					   bytes, 2);
	if (retval)
		return retval;

	retval = GB02FUNC1519(dptx);
	if (retval)
		return retval;
#if 0
	byte = dptx->link.status[DP_LANE_ALIGN_STATUS_UPDATED -
				 DP_LANE0_1_STATUS];

	if (!(byte & DP_LINK_STATUS_UPDATED))
		return 0;
#endif
	/* Check if need to retrain link */
	if (dptx->link.trained &&
		(!drm_dp_channel_eq_ok(dptx->link.status, dptx->link.lanes) ||
	     !drm_dp_clock_recovery_ok(dptx->link.status, dptx->link.lanes))) {
		dptx_dbg(dptx, "%s: Retraining link\n", __func__);
		return GB02FUNC1567(dptx,
			dptx->max_rate, dptx->max_lanes);
	}

	return 0;
}
