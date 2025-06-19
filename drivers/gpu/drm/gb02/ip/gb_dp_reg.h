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

#ifndef __DPTX_REG_H__
#define __DPTX_REG_H__

/* Constants */
#define GB02MAC2044		0
#define GB02MAC2045		1
#define GB02MAC2046		2

/* Controller Version Registers */
#define GB02MAC2047			0x00
#define GB02MAC2048			0x31303061

#define GB02MAC2049			0x04

#define GB02MAC2050				0x08
#define GB02MAC2051               0x9001
#define GB02MAC2052		0x16c3

/* DPTX Configuration Register */
#define GB02MAC2054			0x100

/* CoreControl Register */
#define GB02MAC2057			0x200
#define GB02MAC2059			0x204

/* MST */
#define GB02MAC2061(n)	(0x210 + (n) * 4)

#define GB02MAC2064 14

/* Video Registers. N=0-3 */
#define GB02MAC2066(n)		(0x300  + 0x10000 * (n))
#define GB02MAC2068(n)	(0x304  + 0x10000 * (n))
#define GB02MAC2070(n)	(0x308  + 0x10000 * (n))
#define GB02MAC2072(n)	(0x30C  + 0x10000 * (n))
#define GB02MAC2074(n)		(0x310  + 0x10000 * (n))
#define GB02MAC2076(n)		(0x314  + 0x10000 * (n))
#define GB02MAC2078(n)		(0x318  + 0x10000 * (n))
#define GB02MAC2079(n)		(0x31c  + 0x10000 * (n))
#define GB02MAC2080(n)		(0x320  + 0x10000 * (n))
#define GB02MAC2081(n)		(0x324  + 0x10000 * (n))
#define GB02MAC2082(n)		(0x328  + 0x10000 * (n))
#define GB02MAC2083(n)		(0x32C  + 0x10000 * (n))
#define GB02MAC2084(n)		(0x3804 + 0x10000 * (n))
#define GB02MAC2085(n)		(0x3808 + 0x10000 * (n))
#define GB02MAC2086(n)		(0x380C + 0x10000 * (n))
#define GB02MAC2087(n)		(0x3810 + 0x10000 * (n))
#define GB02MAC2088(n)		(0x3814 + 0x10000 * (n))
#define GB02MAC2089(n)		(0x3800 + 0x10000 * (n))
#define GB02MAC2091	0x330
#define GB02MAC2093 16
#define GB02MAC2094 1
/* Audio Registers */
#define GB02MAC2095		0x400
#define GB02MAC2097			0x3904
#define GB02MAC2098			0x3908
#define GB02MAC2099			0x390C
#define GB02MAC2100			0x3910
#define GB02MAC2101			0x3914
#define GB02MAC2102			0x3918

/* SecondaryDataPacket Registers */
#define GB02MAC2103		0x500
#define GB02MAC2104        0x504
#define GB02MAC2105			0x508
#define GB02MAC2106			0x600

/* PHY Layer Control Registers */
#define GB02MAC2107			0xA00
#define GB02MAC2108			0xA04
#define GB02MAC2109			0xA08
#define GB02MAC2110			0xA0C
#define GB02MAC2111			0xA10
#define GB02MAC2112		0xC00
#define GB02MAC2113		0xC04
#define GB02MAC2114		0xC08

/* AUX Channel Interface Registers */
#define GB02MAC2115			0xB00
#define GB02MAC2116			0xB04
#define GB02MAC2117			0xB08
#define GB02MAC2118			0xB0C
#define GB02MAC2119			0xB10
#define GB02MAC2120			0xB14
#define GB02MAC2121		0xb40
#define GB02MAC2123		0xb44
#define GB02MAC2124	0xb48

/* Interrupt Registers */
#define GB02MAC2126			0xD00
#define GB02MAC2127			0xD04
#define GB02MAC2128			0xD08
#define GB02MAC2129			0xD0C

/* HDCP Registers */
#define GB02MAC2130		0xE00
#define GB02MAC2131		0xE10

/* I2C APB Registers */
#define GB02MAC2132			0xc006c
#define GB02MAC2133			0xc0000
#define GB02MAC2134			0xc0004
#define GB02MAC2135		0xc0010
#define GB02MAC2136	BIT(4)

/* RAM Registers */

#define GB02MAC2137(n)		(0x381C + 0x10000 * (n))
#define GB02MAC2138(n)	(0x3820 + 0x10000 * (n))
#define GB02MAC2139(n)	(0x3824 + 0x10000 * (n))

#define GB02MAC2140	12
#define DPTX_VG_RAM_ADDR_START_MASK	GENMASK(12, 0)
#define GB02MAC2141	0
#define GB02MAC2142	BIT(0)
#define GB02MAC2143	0
#define DPTX_VG_WRT_RAM_DATA_MASK	GENMASK(0, 7)

/* Register Bitfields */
#define GB02MAC2144		16
#define DPTX_ID_DEVICE_ID_MASK		GENMASK(31, 16)
#define GB02MAC2146		0
#define DPTX_ID_VENDOR_ID_MASK		GENMASK(15, 0)

#define GB02MAC2148	16
#define DPTX_CONFIG1_NUM_STREAMS_MASK	GENMASK(18, 16)
#define GB02MAC2150	19
#define DPTX_CONFIG1_MP_MODE_MASK	GENMASK(21, 19)
#define GB02MAC2151	1
#define GB02MAC2152	2
#define GB02MAC2153	4
#define GB02MAC2154		BIT(22)
#define GB02MAC2155	25
#define DPTX_CONFIG1_NUM_DSC_ENC_MASK	GENMASK(28, 25)
#define GB02MAC2156		BIT(29)

#define GB02MAC2157		BIT(1)
#define GB02MAC2158		BIT(2)
#define GB02MAC2159		BIT(3)
#define GB02MAC2160	BIT(25)
#define GB02MAC2161		BIT(26)
#define GB02MAC2162	BIT(28)
#define GB02MAC2163	BIT(29)
#define GB02MAC2164	BIT(16)

#define GB02MAC2165	BIT(0)
#define GB02MAC2166		BIT(1)
#define GB02MAC2167		BIT(2)
#define GB02MAC2168	BIT(3)
#define GB02MAC2169		BIT(4)
#define GB02MAC2170(n)	BIT(5 + n)
#define GB02MAC2171 (GB02MAC2165 |		\
			    GB02MAC2167 |		\
			    GB02MAC2168 |	\
			    GB02MAC2169)

#define GB02MAC2172	0
#define DPTX_PHYIF_CTRL_TPS_SEL_MASK	GENMASK(3, 0)
#define GB02MAC2173	0
#define GB02MAC2174		1
#define GB02MAC2175		2
#define GB02MAC2176		3
#define GB02MAC2177		4
#define GB02MAC2178	5
#define GB02MAC2179	6
#define GB02MAC2180	7
#define GB02MAC2181	8
#define GB02MAC2182	9
#define GB02MAC2183	4
#define DPTX_PHYIF_CTRL_RATE_MASK	GENMASK(5, 4)
#define GB02MAC2184	6
#define DPTX_PHYIF_CTRL_LANES_MASK	GENMASK(7, 6)
#define GB02MAC2185	0x0	//1.62G
#define GB02MAC2186	0x1	//2.7G
#define GB02MAC2187	0x2 //5.4G
#define GB02MAC2188	0x3	//8.1G
#define GB02MAC2189	0x4
#define GB02MAC2190	0x5
#define GB02MAC2191	0x6
#define GB02MAC2192	0x7
#define GB02MAC2193	(GB02MAC2192 - GB02MAC2188 - 1)
#define GB02MAC2194	8
#define DPTX_PHYIF_CTRL_XMIT_EN_MASK	GENMASK(11, 8)
#define GB02MAC2195(lane)	BIT(8 + lane)
#define GB02MAC2196(lane)	BIT(12 + lane)
#define GB02MAC2197		BIT(16)
#define GB02MAC2198 17
#define DPTX_PHYIF_CTRL_LANE_PWRDOWN_MASK GENMASK(20, 17)
#define GB02MAC2199		BIT(25)

#define GB02MAC2200(lane)	(6 * lane)
#define DPTX_PHY_TX_EQ_PREEMP_MASK(lane)	GENMASK(6 * lane + 1, 6 * lane)
#define GB02MAC2201(lane)	(6 * lane + 2)
#define DPTX_PHY_TX_EQ_VSWING_MASK(lane)	GENMASK(6 * lane + 3, \
							6 * lane + 2)

#define GB02MAC2202	0
#define DPTX_AUX_CMD_REQ_LEN_MASK	GENMASK(3, 0)
#define GB02MAC2203	BIT(4)
#define GB02MAC2204		8
#define DPTX_AUX_CMD_ADDR_MASK		GENMASK(27, 8)
#define GB02MAC2205		28
#define DPTX_AUX_CMD_TYPE_MASK		GENMASK(31, 28)
#define GB02MAC2206		0x0
#define GB02MAC2207		0x1
#define GB02MAC2208		0x2
#define GB02MAC2209		0x4
#define GB02MAC2210	0x8

#define GB02MAC2211	0
#define DPTX_AUX_STS_STATUS_MASK	GENMASK(3, 0)
#define GB02MAC2212		0x0
#define GB02MAC2213	0x1
#define GB02MAC2214	0x2
#define GB02MAC2215	0x4
#define GB02MAC2216	0x8
#define GB02MAC2217		8
#define DPTX_AUX_STS_AUXM_MASK		GENMASK(15, 8)
#define GB02MAC2218	BIT(16)
#define GB02MAC2219		BIT(17)
#define GB02MAC2220		BIT(18)
#define GB02MAC2221	19
#define DPTX_AUX_STS_BYTES_READ_MASK	GENMASK(23, 19)
#define GB02MAC2222		BIT(24)

#define GB02MAC2223			BIT(0)
#define GB02MAC2224		BIT(1)
#define GB02MAC2225			BIT(2)
#define GB02MAC2226	BIT(3)
#define GB02MAC2227			BIT(4)
#define GB02MAC2228	BIT(5)
#define GB02MAC2229	BIT(6)
#define GB02MAC2230		BIT(7)
#define GB02MAC2231	(GB02MAC2223 |			\
				 GB02MAC2224 |			\
				 GB02MAC2225 |			\
				 GB02MAC2226 |		\
				 GB02MAC2227 |			\
				 GB02MAC2228 |	\
				 GB02MAC2229 |	\
				 GB02MAC2230)
#define GB02MAC2232			BIT(0)
#define GB02MAC2233		BIT(1)
#define GB02MAC2234			BIT(2)
#define GB02MAC2235	BIT(3)
#define GB02MAC2236			BIT(4)
#define GB02MAC2237	BIT(5)
#define GB02MAC2238	BIT(6)
#define GB02MAC2239			BIT(7)
#define GB02MAC2240	(GB02MAC2232 |			\
				 GB02MAC2233 |		\
				 GB02MAC2234 |		\
				 GB02MAC2235 |	\
				 GB02MAC2236 |			\
				 /* GB02MAC2237 |*/ \
				 /* GB02MAC2238 |*/ \
				 GB02MAC2239)

#define GB02MAC2241			BIT(0)
#define GB02MAC2242		BIT(1)
#define GB02MAC2243		BIT(2)
#define GB02MAC2244	BIT(3)
#define GB02MAC2245		BIT(8)
#define GB02MAC2246		4
#define DPTX_HPDSTS_STATE_MASK		GENMASK(6, 4)
#define GB02MAC2247		16
#define DPTX_HPDSTS_TIMER_MASK		GENMASK(31, 16)

#define GB02MAC2248		GB02MAC2241
#define GB02MAC2249	GB02MAC2242
#define GB02MAC2250	GB02MAC2243
#define GB02MAC2251	GB02MAC2244

#define GB02MAC2252	BIT(5)

#define GB02MAC2253		6
#define DPTX_AG_CONFIG1_WORD_WIDTH_MASK			GENMASK(9, 6)
#define GB02MAC2254			14
#define GB02MAC2255			BIT(14)
#define GB02MAC2256			24
#define DPTX_AG_CONFIG3_CH_NUMCL0_MASK			GENMASK(27, 24)
#define GB02MAC2257			28
#define DPTX_AG_CONFIG3_CH_NUMCR0_MASK			GENMASK(31, 28)
#define GB02MAC2258			0
#define DPTX_AG_CONFIG4_SAMP_FREQ_MASK			GENMASK(3, 0)
#define GB02MAC2259		8
#define DPTX_AG_CONFIG4_WORD_LENGTH_MASK		GENMASK(11, 8)
#define GB02MAC2260		12
#define DPTX_AG_CONFIG4_ORIG_SAMP_FREQ_MASK		GENMASK(15, 12)
#define GB02MAC2261			0
#define GB02MAC2262			BIT(0)
#define GB02MAC2263			12
#define DPTX_AUD_CONFIG1_NCH_MASK			GENMASK(14, 12)
#define GB02MAC2264		5
#define DPTX_AUD_CONFIG1_DATA_WIDTH_MASK		GENMASK(9, 5)
#define GB02MAC2265		1
#define DPTX_AUD_CONFIG1_DATA_EN_IN_MASK		GENMASK(4, 1)
#define GB02MAC2266			24
#define DPTX_AUD_CONFIG1_ATS_VER_MASK			GENMASK(29, 24)

#define GB02MAC2267                16
#define DPTX_VSAMPLE_CTRL_VMAP_BPC_MASK                 GENMASK(20, 16)
#define GB02MAC2268		21
#define DPTX_VSAMPLE_CTRL_MULTI_PIXEL_MASK		GENMASK(22, 21)
#define GB02MAC2269			BIT(23)
#define GB02MAC2270			GB02MAC2046
#define GB02MAC2271			GB02MAC2045
#define GB02MAC2272			GB02MAC2044
#define GB02MAC2273                      29
#define DPTX_VIDEO_VMSA2_BPC_MASK                       GENMASK(31, 29)
#define GB02MAC2274                      25
#define DPTX_VIDEO_VMSA2_COL_MASK                       GENMASK(28, 25)
#define GB02MAC2275                  31
#define GB02MAC2276           30 // ignore MSA
#define DPTX_VIDEO_VMSA3_PIX_ENC_MASK                   GENMASK(31, 30)
#define GB02MAC2277			BIT(0)
#define GB02MAC2278			BIT(1)
#define GB02MAC2279			BIT(2)
#define GB02MAC2280			BIT(0)
#define GB02MAC2281			BIT(1)
#define GB02MAC2282			2
#define GB02MAC2283			16
#define GB02MAC2284			16
#define GB02MAC2285			0
#define GB02MAC2286			0
#define GB02MAC2287				16
#define GB02MAC2288			0
#define GB02MAC2289				16
#define GB02MAC2290			0
#define GB02MAC2291			16
#define GB02MAC2292			0
#define DPTX_VIDEO_CONFIG5_TU_MASK			GENMASK(6, 0)
#define GB02MAC2293            14
#define DPTX_VIDEO_CONFIG5_TU_FRAC_MASK_MST             GENMASK(19, 14)
#define GB02MAC2294            16
#define DPTX_VIDEO_CONFIG5_TU_FRAC_MASK_SST             GENMASK(19, 16)
#define GB02MAC2295		7
#define DPTX_VIDEO_CONFIG5_INIT_THRESHOLD_MASK		GENMASK(13, 7)

#define GB02MAC2296			12
#define DPTX_VG_CONFIG1_BPC_MASK			GENMASK(14, 12)
#define GB02MAC2297			17
#define DPTX_VG_CONFIG1_PATTERN_MASK			GENMASK(18, 17)
#define GB02MAC2298		19
#define DPTX_VG_CONFIG1_MULTI_PIXEL_MASK		GENMASK(20, 19)
#define GB02MAC2299			GB02MAC2046
#define GB02MAC2300			GB02MAC2045
#define GB02MAC2301			GB02MAC2044
#define GB02MAC2302			BIT(0)
#define GB02MAC2303			BIT(1)
#define GB02MAC2305			BIT(2)
#define GB02MAC2306				BIT(3)
#define GB02MAC2307			BIT(5)
#define GB02MAC2308			BIT(6)
#define GB02MAC2309		BIT(7)
#define GB02MAC2310			BIT(15)
#define GB02MAC2311			12
#define GB02MAC2312			0
#define GB02MAC2313			16

#define GB02MAC2314			BIT(0)
#define GB02MAC2315			BIT(1)
#define GB02MAC2316			BIT(2)
#define GB02MAC2317				1
#define GB02MAC2318				1
#define GB02MAC2319                              3
#define GB02MAC2320				9
#define GB02MAC2321				7
#define GB02MAC2322				7
#define GB02MAC2323				0xF
#define GB02MAC2324				0xF
#define GB02MAC2325					BIT(15)

#define GB02MAC2326				0x3614
#define GB02MAC2327                                   0xE00
#define GB02MAC2328                            0x3618
#define GB02MAC2329				0x3630
#define GB02MAC2330				0x3620
#define GB02MAC2331				0x3624
#define GB02MAC2332				0x361C
#define GB02MAC2333				0xE0C
#define GB02MAC2334				0xE08
#define GB02MAC2335					0xE04
#define GB02MAC2336                              0x3628
#define GB02MAC2337                       0x362c

#define GB02MAC2338			7
#define GB02MAC2339			BIT(7)
#define GB02MAC2340			BIT(0)
#define GB02MAC2341			BIT(3)
#define GB02MAC2342				BIT(5)
#define GB02MAC2343				BIT(6)
#define GB02MAC2344				BIT(7)
#define GB02MAC2345				BIT(8)
#define DPTX_REVOC_LIST_MASK				GENMASK(31, 24)
#define GB02MAC2346				23
#define DPTX_REVOC_SIZE_MASK				GENMASK(23, 8)
#define GB02MAC2347			0
#define GB02MAC2348			BIT(0)
#define GB02MAC2349			0
#define DPTX_IDPK_DATA_INDEX_MASK			GENMASK(5, 0)
#define GB02MAC2350			6
#define GB02MAC2351			BIT(6)
#define GB02MAC2352				BIT(2)
#define GB02MAC2353					BIT(6)
#define GB02MAC2354				BIT(1)

#define GB02MAC2355					0x0230
#define DPTX_DSC_NUM_ENC_MSK			GENMASK(3,0)
#define GB02MAC2356					0x0234
#define GB02MAC2357		BIT(18)
#define GB02MAC2358		18

#define DPTX_DSC_ENC_STREAM_SEL(i)				GENMASK((2 * i + 1),(2 * i))
#define GB02MAC2359(i)		((i)*2)
#define GB02MAC2360		22

// Stream should be [1 .. 4],  SST = 1
#define DPTX_DSC_PPS(stream, i)			((0x3a00 + (stream -1) * 0x10000) + (i * 0x4))
#define GB02MAC2361					1

// PPS SDPs
#define GB02MAC2362				0
#define GB02MAC2363				4
#define GB02MAC2364			24
#define GB02MAC2365			5
#define DPTX_DSC_BPP_HIGH_MASK				GENMASK(9, 8)
#define DPTX_DSC_BPP_LOW_MASK				GENMASK(7, 0)
#define DPTX_DSC_BPP_LOW_MASK1				GENMASK(15, 8)
#define GB02MAC2366					28

#define GB02MAC2367					0x104
#define GB02MAC2368				16
#define DSC_MAX_NUM_LINES_MASK				GENMASK(31, 16)

#define GB02MAC2369					0x334
#define GB02MAC2370			0
#define GB02MAC2371			5
#define GB02MAC2372	16
#define DPTX_DSC_LSTEER_XMIT_DELAY_MASK		GENMASK(31, 16)
#define DPTX_DSC_LSTEER_FRAC_SHIFT_MASK		GENMASK(9, 5)
#define DPTX_DSC_LSTEER_INT_SHIFT_MASK		GENMASK(4, 0)

#define GB02MAC2373 8 // For Interop testing only RGB 8bpp 8bpc
#define GB02MAC2374 8 // For Interop testing only RGB 8bpp 8bpc

#endif
