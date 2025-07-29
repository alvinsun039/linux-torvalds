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
#include <linux/version.h>

#ifndef __DRM_DP_HELPER_ADDITIONS_H__
#define __DRM_DP_HELPER_ADDITIONS_H__

/*
 * The following aren't defined in kernel headers 
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
#define DP_LINK_BW_8_1				0x1e
#define DP_TRAINING_PATTERN_4			7
#define DP_TPS4_SUPPORTED			BIT(7)


#define DP_PSR2_WITH_Y_COORD_IS_SUPPORTED  3      /* eDP 1.4a */

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,13,0)
#define DP_DSC_SUPPORT                      0x060   /* DP 1.4 */
#define DP_DSC_DECOMPRESSION_IS_SUPPORTED  (1 << 0)

#define DP_DSC_REV                          0x061
#define DP_DSC_MAJOR_MASK                  (0xf << 0)
#define DP_DSC_MINOR_MASK                  (0xf << 4)
#define DP_DSC_MAJOR_SHIFT                 0
#define DP_DSC_MINOR_SHIFT                 4

#define DP_DSC_RC_BUF_BLK_SIZE              0x062
# define DP_DSC_RC_BUF_BLK_SIZE_1           0x0
# define DP_DSC_RC_BUF_BLK_SIZE_4           0x1
# define DP_DSC_RC_BUF_BLK_SIZE_16          0x2
# define DP_DSC_RC_BUF_BLK_SIZE_64          0x3

#define DP_DSC_RC_BUF_SIZE                  0x063

#define DP_DSC_SLICE_CAP_1                  0x064
# define DP_DSC_1_PER_DP_DSC_SINK           (1 << 0)
# define DP_DSC_2_PER_DP_DSC_SINK           (1 << 1)
# define DP_DSC_4_PER_DP_DSC_SINK           (1 << 3)
# define DP_DSC_6_PER_DP_DSC_SINK           (1 << 4)
# define DP_DSC_8_PER_DP_DSC_SINK           (1 << 5)
# define DP_DSC_10_PER_DP_DSC_SINK          (1 << 6)
# define DP_DSC_12_PER_DP_DSC_SINK          (1 << 7)

#define DP_DSC_LINE_BUF_BIT_DEPTH           0x065
# define DP_DSC_LINE_BUF_BIT_DEPTH_MASK     (0xf << 0)
# define DP_DSC_LINE_BUF_BIT_DEPTH_9        0x0
# define DP_DSC_LINE_BUF_BIT_DEPTH_10       0x1
# define DP_DSC_LINE_BUF_BIT_DEPTH_11       0x2
# define DP_DSC_LINE_BUF_BIT_DEPTH_12       0x3
# define DP_DSC_LINE_BUF_BIT_DEPTH_13       0x4
# define DP_DSC_LINE_BUF_BIT_DEPTH_14       0x5
# define DP_DSC_LINE_BUF_BIT_DEPTH_15       0x6
# define DP_DSC_LINE_BUF_BIT_DEPTH_16       0x7
# define DP_DSC_LINE_BUF_BIT_DEPTH_8        0x8

#define DP_DSC_BLK_PREDICTION_SUPPORT       0x066
#define DP_DSC_BLK_PREDICTION_IS_SUPPORTED (1 << 0)

#define DP_DSC_MAX_BITS_PER_PIXEL_LOW       0x067   /* eDP 1.4 */

#define DP_DSC_MAX_BITS_PER_PIXEL_HI        0x068   /* eDP 1.4 */

#define DP_DSC_DEC_COLOR_FORMAT_CAP         0x069
# define DP_DSC_RGB                         (1 << 0)
# define DP_DSC_YCbCr444                    (1 << 1)
# define DP_DSC_YCbCr422_Simple             (1 << 2)
# define DP_DSC_YCbCr422_Native             (1 << 3)
# define DP_DSC_YCbCr420_Native             (1 << 4)

#define DP_DSC_DEC_COLOR_DEPTH_CAP          0x06A
# define DP_DSC_8_BPC                       (1 << 1)
# define DP_DSC_10_BPC                      (1 << 2)
# define DP_DSC_12_BPC                      (1 << 3)

#define DP_DSC_PEAK_THROUGHPUT              0x06B
#define DP_DSC_THROUGHPUT_MODE_0_MASK      (0xf << 0)
#define DP_DSC_THROUGHPUT_MODE_0_SHIFT     0
#define DP_DSC_THROUGHPUT_MODE_0_340       (1 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_400       (2 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_450       (3 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_500       (4 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_550       (5 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_600       (6 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_650       (7 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_700       (8 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_750       (9 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_800       (10 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_850       (11 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_900       (12 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_950       (13 << 0)
#define DP_DSC_THROUGHPUT_MODE_0_1000      (14 << 0)
#define DP_DSC_THROUGHPUT_MODE_1_MASK      (0xf << 4)
#define DP_DSC_THROUGHPUT_MODE_1_SHIFT     4
#define DP_DSC_THROUGHPUT_MODE_1_340       (1 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_400       (2 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_450       (3 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_500       (4 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_550       (5 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_600       (6 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_650       (7 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_700       (8 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_750       (9 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_800       (10 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_850       (11 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_900       (12 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_950       (13 << 4)
#define DP_DSC_THROUGHPUT_MODE_1_1000      (14 << 4)

#define DP_DSC_MAX_SLICE_WIDTH              0x06C

#define DP_DSC_SLICE_CAP_2                  0x06D
#define DP_DSC_16_PER_DP_DSC_SINK          (1 << 0)
#define DP_DSC_20_PER_DP_DSC_SINK          (1 << 1)
#define DP_DSC_24_PER_DP_DSC_SINK          (1 << 2)

#define DP_DSC_BITS_PER_PIXEL_INC           0x06F
#define DP_DSC_BITS_PER_PIXEL_1_16         0x0
#define DP_DSC_BITS_PER_PIXEL_1_8          0x1
#define DP_DSC_BITS_PER_PIXEL_1_4          0x2
#define DP_DSC_BITS_PER_PIXEL_1_2          0x3
#define DP_DSC_BITS_PER_PIXEL_1            0x4

#define DP_PSR_SUPPORT                      0x070   /* XXX 1.2? */
#define DP_PSR_IS_SUPPORTED                1
#define DP_PSR2_IS_SUPPORTED              2       /* eDP 1.4 */

#define DP_PSR_CAPS                         0x071   /* XXX 1.2? */
#define DP_PSR_NO_TRAIN_ON_EXIT            1
#define DP_PSR_SETUP_TIME_330              (0 << 1)
#define DP_PSR_SETUP_TIME_275              (1 << 1)
#define DP_PSR_SETUP_TIME_220              (2 << 1)
#define DP_PSR_SETUP_TIME_165              (3 << 1)
#define DP_PSR_SETUP_TIME_110              (4 << 1)
#define DP_PSR_SETUP_TIME_55               (5 << 1)
#define DP_PSR_SETUP_TIME_0                (6 << 1)
#define DP_PSR_NO_TRAIN_ON_EXIT            1
#define DP_PSR_SETUP_TIME_330              (0 << 1)
#define DP_PSR_SETUP_TIME_275              (1 << 1)
#define DP_PSR_SETUP_TIME_220              (2 << 1)
#define DP_PSR_SETUP_TIME_165              (3 << 1)
#define DP_PSR_SETUP_TIME_110              (4 << 1)
#define DP_PSR_SETUP_TIME_55               (5 << 1)
#define DP_PSR_SETUP_TIME_0                (6 << 1)
#define DP_PSR_SETUP_TIME_MASK             (7 << 1)
#define DP_PSR_SETUP_TIME_SHIFT            1
#define DP_PSR2_SU_Y_COORDINATE_REQUIRED   (1 << 4)  /* eDP 1.4a */
#define DP_PSR2_SU_GRANULARITY_REQUIRED    (1 << 5)  /* eDP 1.4b */
#endif

#define GB02MAC1656                     0x222
#define GB02MAC1658			0x223
#define GB02MAC1660                     0x224
#define GB02MAC1662			0x225
#define GB02MAC1664			0x226
#define GB02MAC1666			0x227
#define GB02MAC1668			0x228
#define GB02MAC1669			0x229
#define GB02MAC1672		0x22A
#define GB02MAC1675		0x22B
#define GB02MAC1678		0x22C
#define GB02MAC1680		0x22D
#define GB02MAC1683			0x22E
#define GB02MAC1686			0x22F
#define GB02MAC1689			0x230
#define GB02MAC1692			0x231
#define GB02MAC1695			0x248
#define GB02MAC1697			0x0
#define GB02MAC1700		0x1
#define GB02MAC1703	0x2
#define GB02MAC1706		0x3

#define GB02MAC1709		0x250
#define GB02MAC1712		0x251
#define GB02MAC1715		0x252
#define GB02MAC1718		0x253
#define GB02MAC1721		0x254
#define GB02MAC1723		0x255
#define GB02MAC1725		0x256
#define GB02MAC1726		0x257
#define GB02MAC1728		0x258
#define GB02MAC1730		0x259

#define DP_TEST_PHY_PATTERN_SEL_MASK		GENMASK(2, 0)
#define GB02MAC1733		0x0
#define GB02MAC1735			0x1
#define GB02MAC1737		0x2
#define GB02MAC1739		0x3
#define GB02MAC1741		0x4
#define GB02MAC1743		0x5
#define GB02MAC1745		0x6
#define GB02MAC1747	0x7

#define GB02MAC1749				0x232


#if LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0)
#define DP_TEST_COLOR_FORMAT_MASK		GENMASK(2, 1)
#define DP_TEST_BIT_DEPTH_MASK                  GENMASK(7, 5)
#define DP_TEST_BIT_DEPTH_SHIFT			5
#define DP_TEST_BIT_DEPTH_6			0x0
#define DP_TEST_BIT_DEPTH_8			0x1
#define DP_TEST_BIT_DEPTH_10			0x2
#define DP_TEST_BIT_DEPTH_12			0x3
#define DP_TEST_BIT_DEPTH_16			0x4
#endif

#define GB02MAC1752             3
#define GB02MAC1754		BIT(3)
#define GB02MAC1756		4
#define GB02MAC1758		BIT(4)

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0)
#define DP_TEST_DYNAMIC_RANGE_CEA               0x1
#endif
#define GB02MAC1763	        0x0
#define GB02MAC1766           0x2
#define GB02MAC1769		0x4
#define GB02MAC1772		0x0
#define GB02MAC1775		0x1

#define	GB02MAC1779			0x271
#define DP_TEST_AUDIO_SAMPLING_RATE_MASK	GENMASK(3, 0)
#define GB02MAC1782		4
#define DP_TEST_AUDIO_CH_COUNT_MASK		GENMASK(7, 4)

#define GB02MAC1785		0x0
#define GB02MAC1787	0x1
#define GB02MAC1789		0x2
#define GB02MAC1791	0x3
#define GB02MAC1793		0x4
#define GB02MAC1795	0x5
#define GB02MAC1797		0x6

#define GB02MAC1800			0x0
#define GB02MAC1802			0x1
#define GB02MAC1803			0x2
#define GB02MAC1805			0x3
#define GB02MAC1807			0x4
#define GB02MAC1809			0x5
#define GB02MAC1810			0x6
#define GB02MAC1812			0x7

#define GB02MAC1813                         0x04    /* eDP 1.4a */
#define GB02MAC1815                         0x05    /* eDP 1.4b */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
static inline bool
drm_dp_tps4_supported(const u8 dpcd[DP_RECEIVER_CAP_SIZE])
{
	return dpcd[DP_DPCD_REV] >= 0x14 &&
		dpcd[DP_MAX_DOWNSPREAD] & DP_TPS4_SUPPORTED;
}

#define DP_FEC_CONFIGURATION			0x120
#define DP_FEC_READY				(1 << 0)
#endif

#define GB02MAC1819 BIT(7)

#endif
