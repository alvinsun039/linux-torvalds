#include "txgbe_e56.h"
#include "txgbe_hw.h"

#include <linux/sort.h>

#define EPHY_RREG(REG)                             \
	do {                                       \
		rdata = 0;                         \
		rdata = rd32_ephy(hw, REG##_ADDR); \
	} while (0)

#define EPHY_WREG(REG)                                  \
	do {                                            \
		txgbe_wr32_ephy(hw, REG##_ADDR, rdata); \
	} while (0)

#define EPCS_RREG(REG)                                   \
	do {                                             \
		rdata = 0;                               \
		rdata = txgbe_rd32_epcs(hw, REG##_ADDR); \
	} while (0)

#define EPCS_WREG(REG)                                  \
	do {                                            \
		txgbe_wr32_epcs(hw, REG##_ADDR, rdata); \
	} while (0)

#define txgbe_e56_ephy_config(reg, field, val) \
	do {                                   \
		EPHY_RREG(reg);                \
		EPHY_XFLD(reg, field) = (val); \
		EPHY_WREG(reg);                \
	} while (0)

#define txgbe_e56_epcs_config(reg, field, val) \
	do {                                   \
		EPCS_RREG(reg);                \
		EPCS_XFLD(reg, field) = (val); \
		EPCS_WREG(reg);                \
	} while (0)

void SetFields(unsigned int *pSrcData, unsigned int bitHigh,
	       unsigned int bitLow, unsigned int setValue)
{
	int i;

	//-- Single bit field handling
	if (bitHigh == bitLow) {
		if (setValue == 0) { //clear single bit
			*pSrcData &= ~(1 << bitLow);
		} else { //set single bit
			*pSrcData |= (1 << bitLow);
		}
	} else {
		//first, clear the bit fields
		for (i = bitLow; i <= bitHigh; i++) {
			*pSrcData &= ~(1 << i); //clear single bit
		}

		//second, or the bit fields with set value
		*pSrcData |= (setValue << bitLow);
	}
}

u32 E56phyTxFfeCfg(struct txgbe_hw *hw, u32 speed)
{
	u32 addr;

	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		addr = 0x141c;
		txgbe_wr32_ephy(hw, addr, S10G_TX_FFE_CFG_MAIN);

		addr = 0x1420;
		txgbe_wr32_ephy(hw, addr, S10G_TX_FFE_CFG_PRE1);

		addr = 0x1424;
		txgbe_wr32_ephy(hw, addr, S10G_TX_FFE_CFG_PRE2);

		addr = 0x1428;
		txgbe_wr32_ephy(hw, addr, S10G_TX_FFE_CFG_POST);
	} else if (speed == TXGBE_LINK_SPEED_25GB_FULL) {
		addr = 0x141c;
		txgbe_wr32_ephy(hw, addr, S25G_TX_FFE_CFG_MAIN);

		addr = 0x1420;
		txgbe_wr32_ephy(hw, addr, S25G_TX_FFE_CFG_PRE1);

		addr = 0x1424;
		txgbe_wr32_ephy(hw, addr, S25G_TX_FFE_CFG_PRE2);

		addr = 0x1428;
		txgbe_wr32_ephy(hw, addr, S25G_TX_FFE_CFG_POST);
	}

	return 0;
}

u32 txgbe_e56_get_temp(struct txgbe_hw *hw, int *pTempData)
{
	int data_code, temp_data, temp_fraction;
	u32 addr, rdata, wdata;
	u32 timer = 0;

	addr = 0x10338;
	wdata = 0x0001;
	wr32(hw, addr, wdata);

	while (1) {
		rdata = rd32(hw, 0x1033c);
		if ((rdata >> 12) != 0)
			break;
		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait 0x1033c Timeout!!!\n");
			return -1;
		}
	}

	data_code = rdata & 0xFFF;
	temp_data = 419400 + 2205 * (data_code * 1000 / 4094 - 500);

	//Change double Temperature to int
	*pTempData = temp_data/10000;
	temp_fraction = temp_data - (*pTempData * 10000);
	if (temp_fraction >= 5000) {
		*pTempData += 1;
	}

	return 0;
}

u32 txgbe_e56_cfg_25g(struct txgbe_hw *hw)
{
	u32 addr;
	u32 rdata = 0;

	addr = E56PHY_CMS_PIN_OVRDVAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CMS_PIN_OVRDVAL_0_INT_PLL0_TX_SIGNAL_TYPE_I,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_PIN_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CMS_PIN_OVRDEN_0_OVRD_EN_PLL0_TX_SIGNAL_TYPE_I,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_ANA_OVRDVAL_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_CMS_ANA_OVRDVAL_2_ANA_LCPLL_HF_VCO_SWING_CTRL_I, 0xf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_VCO_SWING_CTRL_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_ANA_OVRDVAL_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 23, 0, 0x260000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_CMS_ANA_OVRDEN_1_OVRD_EN_ANA_LCPLL_HF_TEST_IN_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_TXS_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_TXS_CFG_1_ADAPTATION_WAIT_CNT_X256, 0xf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_WKUP_CNT_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_WKUP_CNTLDO_WKUP_CNT_X32, 0xff);
	SetFields(&rdata, E56PHY_TXS_WKUP_CNTDCC_WKUP_CNT_X32, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_PIN_OVRDVAL_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 27, 24, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_PIN_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_PIN_OVRDEN_0_OVRD_EN_TX0_EFUSE_BITS_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_ANA_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_ANA_OVRDVAL_1_ANA_TEST_DAC_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_ANA_OVRDEN_0_OVRD_EN_ANA_TEST_DAC_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	E56phyTxFfeCfg(hw, TXGBE_LINK_SPEED_25GB_FULL);

	addr = E56PHY_RXS_RXS_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_RXS_CFG_0_DSER_DATA_SEL, 0x0);
	SetFields(&rdata, E56PHY_RXS_RXS_CFG_0_TRAIN_CLK_GATE_BYPASS_EN,
		  0x1fff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_1_PREDIV1, 0x700);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_1_TARGET_CNT1, 0x2418);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_OSC_RANGE_SEL1, 0x1);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_VCO_CODE_INIT, 0x7fb);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_OSC_CURRENT_BOOST_EN1,
		  0x0);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_BBCDR_CURRENT_BOOST1, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_SDM_WIDTH, 0x3);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_PRELOCK,
		  0xf);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_POSTLOCK,
		  0x3);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_POSTLOCK,
		  0xa);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_PRELOCK,
		  0xf);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BBCDR_RDY_CNT, 0x3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_PRELOCK, 0x7);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_POSTLOCK,
		  0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_INTL_CONFIG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_INTL_CONFIG_0_ADC_INTL2SLICE_DELAY1,
		  0x3333);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_INTL_CONFIG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_INTL_CONFIG_2_INTERLEAVER_HBW_DISABLE1,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_TXFFE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_LTH, 0x56);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_UTH, 0x6a);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_TXFFE_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_LTH, 0x1f8);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_UTH, 0xf0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_TXFFE_TRAINING_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_LTH, 0x100);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_UTH, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_TXFFE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_LTH, 0x4);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_UTH, 0x37);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_TXFFE_TRAIN_MOD_TYPE,
		  0x38);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G__RXS0_FOM_18__ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56G__RXS0_FOM_18__DFE_COEFFL_HINT__MSB,
		  E56G__RXS0_FOM_18__DFE_COEFFL_HINT__LSB, 0x0);
	SetFields(&rdata, E56G__RXS0_FOM_18__DFE_COEFFH_HINT__MSB,
		  E56G__RXS0_FOM_18__DFE_COEFFH_HINT__LSB, 0x90);
	SetFields(&rdata, E56G__RXS0_FOM_18__DFE_COEFF_HINT_LOAD__MSB,
		  E56G__RXS0_FOM_18__DFE_COEFF_HINT_LOAD__LSB, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_VGA_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_0_VGA_TARGET, 0x34);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_VGA_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT0, 0xa);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT0, 0xa);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT123, 0xa);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT123, 0xa);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT0, 0x9);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT123, 0x9);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_1_LFEQ_LUT, 0x1ffffea);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P1,
		  S25G_PHY_RX_CTLE_TAP_FRACP1);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P2,
		  S25G_PHY_RX_CTLE_TAP_FRACP2);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P3,
		  S25G_PHY_RX_CTLE_TAP_FRACP3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P1,
		  S25G_PHY_RX_CTLE_TAPWT_WEIGHT1);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P2,
		  S25G_PHY_RX_CTLE_TAPWT_WEIGHT2);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P3,
		  S25G_PHY_RX_CTLE_TAPWT_WEIGHT3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_SLICE_DATA_AVG_CNT,
		  0x3);
	SetFields(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_DATA_AVG_CNT, 0x3);
	SetFields(&rdata,
		  E56PHY_RXS_OFFSET_N_GAIN_CAL_0_FE_OFFSET_DAC_CLK_CNT_X8, 0xc);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_1_SAMP_ADAPT_CFG, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_FFE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_FFE_TRAINING_0_FFE_TAP_EN, 0xf9ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_IDLE_DETECT_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MAX, 0xa);
	SetFields(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MIN, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	txgbe_e56_ephy_config(E56G__RXS3_ANA_OVRDVAL_11, ana_test_adc_clkgen_i,
			      0x0);
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_2,
			      ovrd_en_ana_test_adc_clkgen_i, 0x0);

	addr = E56PHY_RXS_ANA_OVRDVAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDVAL_0_ANA_EN_RTERM_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_TRIM_RTERM_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 4, 0, 0x0);
	SetFields(&rdata, 14, 13, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_BBCDR_VCOFILT_BYP_I, 0x1);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_TEST_BBCDR_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 2, 0, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDVAL_17_ANA_VGA2_BOOST_CSTM_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_ANABS_CONFIG_I,
		  0x1);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_VGA2_BOOST_CSTM_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 13, 13, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 13, 13, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_EYE_SCAN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_EYE_SCAN_1_EYE_SCAN_REF_TIMER, 0x400);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_RINGO_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 21, 12, 0x366);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_3_CTRL_FSM_TIMEOUT_X64K, 0x80);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_ON_PERIOD_X64K, 0x18);
	SetFields(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_PERIOD_X512K, 0x3e);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_5_USE_RECENT_MARKER_OFFSET, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_0_CONT_ON_ADC_OFST_CAL_ERR, 0x1);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_0_DO_RX_ADC_OFST_CAL, 0x3);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_0_RX_ERR_ACTION_EN, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST0_WAIT_CNT_X4096, 0xff);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST1_WAIT_CNT_X4096, 0xff);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST2_WAIT_CNT_X4096, 0xff);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST3_WAIT_CNT_X4096, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST4_WAIT_CNT_X4096, 0x1);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST5_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST6_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST7_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST8_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST9_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST10_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST11_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST12_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST13_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST14_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST15_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_7_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST4_EN, 0x4bf);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST5_EN, 0xc4bf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_8_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_8_TRAIN_ST7_EN, 0x47ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_12_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_12_TRAIN_ST15_EN, 0x67ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_13_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST0_DONE_EN, 0x8001);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST1_DONE_EN, 0x8002);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_14_TRAIN_ST3_DONE_EN, 0x8008);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_15_TRAIN_ST4_DONE_EN, 0x8004);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_17_TRAIN_ST8_DONE_EN, 0x20c0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_18_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_18_TRAIN_ST10_DONE_EN, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_29_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_29_TRAIN_ST15_DC_EN, 0x3f6d);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_33_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN0_RATE_SEL, 0x8000);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN1_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_34_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN2_RATE_SEL, 0x8000);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN3_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_KRT_TFSM_CFG_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X1000K,
		  0x49);
	SetFields(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X8000K,
		  0x37);
	SetFields(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_HOLDOFF_TIMER_X256K,
		  0x2f);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_FETX_FFE_TRAIN_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_FETX_FFE_TRAIN_CFG_0_KRT_FETX_INIT_FFE_CFG_2, 0x2);
	txgbe_wr32_ephy(hw, addr, rdata);

	return 0;
}

u32 txgbe_e56_cfg_10g(struct txgbe_hw *hw)
{
	u32 addr;
	u32 rdata = 0;

	addr = E56G_CMS_ANA_OVRDVAL_7_ADDR;
	rdata = rd32_ephy(hw, addr);
	((E56G_CMS_ANA_OVRDVAL_7 *)&rdata)->ana_lcpll_lf_vco_swing_ctrl_i = 0xf;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G_CMS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	((E56G_CMS_ANA_OVRDEN_1 *)&rdata)
		->ovrd_en_ana_lcpll_lf_vco_swing_ctrl_i = 0x1;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G_CMS_ANA_OVRDVAL_9_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 23, 0, 0x260000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G__RXS0_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	((E56G_CMS_ANA_OVRDEN_1 *)&rdata)->ovrd_en_ana_lcpll_lf_test_in_i = 0x1;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_TXS_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_TXS_CFG_1_ADAPTATION_WAIT_CNT_X256, 0xf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_WKUP_CNT_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_WKUP_CNTLDO_WKUP_CNT_X32, 0xff);
	SetFields(&rdata, E56PHY_TXS_WKUP_CNTDCC_WKUP_CNT_X32, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_PIN_OVRDVAL_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 19, 16, 0x6);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_PIN_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_PIN_OVRDEN_0_OVRD_EN_TX0_EFUSE_BITS_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_ANA_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_ANA_OVRDVAL_1_ANA_TEST_DAC_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_TXS_ANA_OVRDEN_0_OVRD_EN_ANA_TEST_DAC_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//Setting TX FFE
	E56phyTxFfeCfg(hw, TXGBE_LINK_SPEED_10GB_FULL);

	addr = E56PHY_RXS_RXS_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_RXS_CFG_0_DSER_DATA_SEL, 0x0);
	SetFields(&rdata, E56PHY_RXS_RXS_CFG_0_TRAIN_CLK_GATE_BYPASS_EN,
		  0x1fff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	((E56G_RXS0_OSC_CAL_N_CDR_0 *)&rdata)->prediv0 = 0xfa0;
	((E56G_RXS0_OSC_CAL_N_CDR_0 *)&rdata)->target_cnt0 = 0x203a;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	((E56G_RXS0_OSC_CAL_N_CDR_4 *)&rdata)->osc_range_sel0 = 0x2;
	((E56G_RXS0_OSC_CAL_N_CDR_4 *)&rdata)->vco_code_init = 0x7ff;
	((E56G_RXS0_OSC_CAL_N_CDR_4 *)&rdata)->osc_current_boost_en0 = 0x1;
	((E56G_RXS0_OSC_CAL_N_CDR_4 *)&rdata)->bbcdr_current_boost0 = 0x0;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_SDM_WIDTH, 0x3);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_PRELOCK,
		  0xf);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_POSTLOCK,
		  0xf);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_POSTLOCK,
		  0xc);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_PRELOCK,
		  0xf);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BBCDR_RDY_CNT, 0x3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_PRELOCK, 0x7);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_POSTLOCK,
		  0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_INTL_CONFIG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	((E56G_RXS0_INTL_CONFIG_0 *)&rdata)->adc_intl2slice_delay0 = 0x5555;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_INTL_CONFIG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	((E56G_RXS0_INTL_CONFIG_2 *)&rdata)->interleaver_hbw_disable0 = 0x1;
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_TXFFE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_LTH, 0x56);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_UTH, 0x6a);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_TXFFE_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_LTH, 0x1e8);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_UTH, 0x78);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_TXFFE_TRAINING_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_LTH, 0x100);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_UTH, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_TXFFE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_LTH, 0x4);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_UTH, 0x37);
	SetFields(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_TXFFE_TRAIN_MOD_TYPE,
		  0x38);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_VGA_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_0_VGA_TARGET, 0x34);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_VGA_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT0, 0xa);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT0, 0xa);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT123, 0xa);
	SetFields(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT123, 0xa);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT0, 0x9);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT123, 0x9);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_1_LFEQ_LUT, 0x1ffffea);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P1,
		  S10G_PHY_RX_CTLE_TAP_FRACP1);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P2,
		  S10G_PHY_RX_CTLE_TAP_FRACP2);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P3,
		  S10G_PHY_RX_CTLE_TAP_FRACP3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P1,
		  S10G_PHY_RX_CTLE_TAPWT_WEIGHT1);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P2,
		  S10G_PHY_RX_CTLE_TAPWT_WEIGHT2);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P3,
		  S10G_PHY_RX_CTLE_TAPWT_WEIGHT3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_SLICE_DATA_AVG_CNT,
		  0x3);
	SetFields(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_DATA_AVG_CNT, 0x3);
	SetFields(&rdata,
		  E56PHY_RXS_OFFSET_N_GAIN_CAL_0_FE_OFFSET_DAC_CLK_CNT_X8, 0xc);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_1_SAMP_ADAPT_CFG, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_FFE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_FFE_TRAINING_0_FFE_TAP_EN, 0xf9ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_IDLE_DETECT_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MAX, 0xa);
	SetFields(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MIN, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	txgbe_e56_ephy_config(E56G__RXS3_ANA_OVRDVAL_11, ana_test_adc_clkgen_i,
			      0x0);
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_2,
			      ovrd_en_ana_test_adc_clkgen_i, 0x0);

	addr = E56PHY_RXS_ANA_OVRDVAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDVAL_0_ANA_EN_RTERM_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_TRIM_RTERM_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 4, 0, 0x6);
	SetFields(&rdata, 14, 13, 0x2);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_BBCDR_VCOFILT_BYP_I, 0x1);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_TEST_BBCDR_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 2, 0, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDVAL_17_ANA_VGA2_BOOST_CSTM_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_ANABS_CONFIG_I,
		  0x1);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_VGA2_BOOST_CSTM_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 13, 13, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 13, 13, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_EYE_SCAN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_EYE_SCAN_1_EYE_SCAN_REF_TIMER, 0x400);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_RINGO_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, 9, 4, 0x366);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_3_CTRL_FSM_TIMEOUT_X64K, 0x80);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_ON_PERIOD_X64K, 0x18);
	SetFields(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_PERIOD_X512K, 0x3e);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_5_USE_RECENT_MARKER_OFFSET, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_0_CONT_ON_ADC_OFST_CAL_ERR, 0x1);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_0_DO_RX_ADC_OFST_CAL, 0x3);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_0_RX_ERR_ACTION_EN, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST0_WAIT_CNT_X4096, 0xff);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST1_WAIT_CNT_X4096, 0xff);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST2_WAIT_CNT_X4096, 0xff);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST3_WAIT_CNT_X4096, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST4_WAIT_CNT_X4096, 0x1);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST5_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST6_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST7_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST8_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST9_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST10_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST11_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST12_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST13_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST14_WAIT_CNT_X4096, 0x4);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST15_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_7_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST4_EN, 0x4bf);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST5_EN, 0xc4bf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_8_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_8_TRAIN_ST7_EN, 0x47ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_12_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_12_TRAIN_ST15_EN, 0x67ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_13_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST0_DONE_EN, 0x8001);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST1_DONE_EN, 0x8002);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_14_TRAIN_ST3_DONE_EN, 0x8008);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_15_TRAIN_ST4_DONE_EN, 0x8004);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_17_TRAIN_ST8_DONE_EN, 0x20c0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_18_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_18_TRAIN_ST10_DONE_EN, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_29_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_29_TRAIN_ST15_DC_EN, 0x3f6d);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_33_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN0_RATE_SEL, 0x8000);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN1_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_34_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN2_RATE_SEL, 0x8000);
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN3_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_KRT_TFSM_CFG_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X1000K,
		  0x49);
	SetFields(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X8000K,
		  0x37);
	SetFields(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_HOLDOFF_TIMER_X256K,
		  0x2f);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_FETX_FFE_TRAIN_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_FETX_FFE_TRAIN_CFG_0_KRT_FETX_INIT_FFE_CFG_2, 0x2);
	txgbe_wr32_ephy(hw, addr, rdata);

	return 0;
}

int E56phyRxsOscInitForTempTrackRange(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int addr, rdata, timer;
	int T = 40;
	int RX_COARSE_MID_TD, CMVAR_RANGE_H = 0, CMVAR_RANGE_L = 0;
	int OFFSET_CENTRE_RANGE_H, OFFSET_CENTRE_RANGE_L, RANGE_FINAL;
	int osc_freq_err_occur;

	//1. Read the temperature T just before RXS is enabled.
	txgbe_e56_get_temp(hw, &T);

	//2. Define software variable RX_COARSE_MID_TD (RX Coarse Code mid value dependent upon temperature)
	if (T < -5) {
		RX_COARSE_MID_TD = 10;
	} else if (T < 30) {
		RX_COARSE_MID_TD = 9;
	} else if (T < 65) {
		RX_COARSE_MID_TD = 8;
	} else if (T < 100) {
		RX_COARSE_MID_TD = 7;
	} else {
		RX_COARSE_MID_TD = 6;
	}

	//Set CMVAR_RANGE_H/L based on the link speed mode
	if (speed == TXGBE_LINK_SPEED_10GB_FULL) { //10G mode
		CMVAR_RANGE_H = S10G_CMVAR_RANGE_H;
		CMVAR_RANGE_L = S10G_CMVAR_RANGE_L;
	} else if (speed == TXGBE_LINK_SPEED_25GB_FULL) { //25G mode
		CMVAR_RANGE_H = S25G_CMVAR_RANGE_H;
		CMVAR_RANGE_L = S25G_CMVAR_RANGE_L;
	}

	// TBD select all lane
	//3. Program ALIAS::RXS::RANGE_SEL = CMVAR::RANGE_H
	// RXS0_ANA_OVRDVAL[5]
	// ana_bbcdr_osc_range_sel_i[1:0]
	rdata = 0x0000;
	addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDVAL_5_ANA_BBCDR_OSC_RANGE_SEL_I,
		  CMVAR_RANGE_H);
	txgbe_wr32_ephy(hw, addr, rdata);

	// RXS0_ANA_OVRDEN[0]
	// [29] ovrd_en_ana_bbcdr_osc_range_sel_i
	rdata = 0x0000;
	addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_BBCDR_OSC_RANGE_SEL_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//4. Do SEQ::RX_ENABLE to enable RXS, and let it stop after oscillator calibration.
	//This needs to be done by blocking the RX power-up fsm at the state following the oscillator calibration state.
	//Follow below steps to do the same before SEQ::RX_ENABLE.
	//a. ALIAS::PDIG::CTRL_FSM_RX_ST can be stopped at RX_SAMP_CAL_ST which is the state
	//after RX_OSC_CAL_ST by configuring ALIAS::RXS::SAMP_CAL_DONE=0b0

	// RXS0_OVRDVAL[0]
	// [22] rxs0_rx0_samp_cal_done_o
	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDVAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_0_RXS0_RX0_SAMP_CAL_DONE_O, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	// RXS0_OVRDEN[0]
	// [27] ovrd_en_rxs0_rx0_samp_cal_done_o
	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_0_OVRD_EN_RXS0_RX0_SAMP_CAL_DONE_O,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//Do SEQ::RX_ENABLE to enable RXS
	rdata = 0;
	addr = E56PHY_PMD_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);

	SetFields(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//b. Poll ALIAS::PDIG::CTRL_FSM_RX_ST and confirm its value is RX_SAMP_CAL_ST
	// poll CTRL_FSM_RX_ST
	rdata = 0;
	timer = 0;
	osc_freq_err_occur = 0;
	while ((rdata & 0x3f) != 0x9) { //Bit[5:0]!= 0x9
		udelay(500);
		// INTR[0]
		// [11:8] intr_rx_osc_freq_err
		rdata = 0;
		addr = E56PHY_INTR_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		// TBD is always osc_freq_err occur?
		if ((rdata & 0x100) == 0x100) {
			osc_freq_err_occur = 1;
			break;
		}
		rdata = 0;
		addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
		rdata = rd32_ephy(hw, addr);

		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
			break;
			return -1;
		}
	}

	//5/6.Define software variable as OFFSET_CENTRE_RANGE_H = ALIAS::RXS::COARSE
	//- RX_COARSE_MID_TD. Clear the INTR.
	rdata = 0;
	addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	OFFSET_CENTRE_RANGE_H = (rdata >> 4) & 0xf;
	if (OFFSET_CENTRE_RANGE_H > RX_COARSE_MID_TD) {
		OFFSET_CENTRE_RANGE_H =
			OFFSET_CENTRE_RANGE_H - RX_COARSE_MID_TD;
	} else {
		OFFSET_CENTRE_RANGE_H =
			RX_COARSE_MID_TD - OFFSET_CENTRE_RANGE_H;
	}

	//7. Do SEQ::RX_DISABLE to disable RXS. Poll ALIAS::PDIG::CTRL_FSM_RX_ST and confirm
	//its value is POWERDN_ST

	rdata = 0;
	addr = E56PHY_PMD_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	timer = 0;
	while (1) {
		udelay(500);
		rdata = 0;
		addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		if ((rdata & 0x3f) == 0x21) {
			break;
		}
		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
			break;
			return -1;
		}
	}

	//8. Since RX power-up fsm is stopped in RX_SAMP_CAL_ST, it is possible the timeout interrupt is set.
	//Clear the same by clearing ALIAS::PDIG::INTR_CTRL_FSM_RX_ERR.
	//Also clear ALIAS::PDIG::INTR_RX_OSC_FREQ_ERR which could also be set.
	udelay(500);
	rdata = 0;
	addr = E56PHY_INTR_0_ADDR;
	rdata = rd32_ephy(hw, addr);

	udelay(500);
	addr = E56PHY_INTR_0_ADDR;
	txgbe_wr32_ephy(hw, addr, rdata);

	udelay(500);
	rdata = 0;
	addr = E56PHY_INTR_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	// next round

	//9. Program ALIAS::RXS::RANGE_SEL = CMVAR::RANGE_L
	// RXS0_ANA_OVRDVAL[5]
	// ana_bbcdr_osc_range_sel_i[1:0]
	rdata = 0x0000;
	addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDVAL_5_ANA_BBCDR_OSC_RANGE_SEL_I,
		  CMVAR_RANGE_L);
	txgbe_wr32_ephy(hw, addr, rdata);

	// RXS0_ANA_OVRDEN[0]
	// [29] ovrd_en_ana_bbcdr_osc_range_sel_i
	rdata = 0x0000;
	addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_BBCDR_OSC_RANGE_SEL_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//10. Do SEQ::RX_ENABLE to enable RXS, and let it stop after oscillator calibration.
	// RXS0_OVRDVAL[0]
	// [22] rxs0_rx0_samp_cal_done_o
	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDVAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_0_RXS0_RX0_SAMP_CAL_DONE_O, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	// RXS0_OVRDEN[0]
	// [27] ovrd_en_rxs0_rx0_samp_cal_done_o
	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_0_OVRD_EN_RXS0_RX0_SAMP_CAL_DONE_O,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0;
	addr = E56PHY_PMD_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);

	SetFields(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	// poll CTRL_FSM_RX_ST
	timer = 0;
	osc_freq_err_occur = 0;
	while ((rdata & 0x3f) != 0x9) { //Bit[5:0]!= 0x9
		udelay(500);
		// INTR[0]
		// [11:8] intr_rx_osc_freq_err
		rdata = 0;
		addr = E56PHY_INTR_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		// TBD is always osc_freq_err occur?
		if ((rdata & 0x100) == 0x100) {
			osc_freq_err_occur = 1;
			break;
		}
		rdata = 0;
		addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
			break;
			return -1;
		} //if (timer++ > PHYINIT_TIMEOUT) {
	}

	//11/12.Define software variable as OFFSET_CENTRE_RANGE_L = ALIAS::RXS::COARSE -
	//RX_COARSE_MID_TD. Clear the INTR.
	rdata = 0;
	addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	OFFSET_CENTRE_RANGE_L = (rdata >> 4) & 0xf;
	if (OFFSET_CENTRE_RANGE_L > RX_COARSE_MID_TD) {
		OFFSET_CENTRE_RANGE_L =
			OFFSET_CENTRE_RANGE_L - RX_COARSE_MID_TD;
	} else {
		OFFSET_CENTRE_RANGE_L =
			RX_COARSE_MID_TD - OFFSET_CENTRE_RANGE_L;
	}

	//13. Perform below calculation in software. Goal is to pick range value which is closer to RX_COARSE_MID_TD
	if (OFFSET_CENTRE_RANGE_L < OFFSET_CENTRE_RANGE_H) {
		RANGE_FINAL = CMVAR_RANGE_L;
	} else {
		RANGE_FINAL = CMVAR_RANGE_H;
	}

	//14. Do SEQ::RX_DISABLE to disable RXS. Poll ALIAS::PDIG::CTRL_FSM_RX_ST
	//and confirm its value is POWERDN_ST
	rdata = 0;
	addr = E56PHY_PMD_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x0);
	addr = E56PHY_PMD_CFG_0_ADDR;
	txgbe_wr32_ephy(hw, addr, rdata);

	timer = 0;
	while (1) {
		udelay(500);
		rdata = 0;
		addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		if ((rdata & 0x3f) == 0x21) {
			break;
		}
		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
			break;
			return -1;
		} //if (timer++ > PHYINIT_TIMEOUT) {
	}

	//15. Since RX power-up fsm is stopped in RX_SAMP_CAL_ST,
	//it is possible the timeout interrupt is set. Clear the same by clearing
	//ALIAS::PDIG::INTR_CTRL_FSM_RX_ERR. Also clear ALIAS::PDIG::INTR_RX_OSC_FREQ_ERR
	//which could also be set.
	udelay(500);
	rdata = 0;
	addr = E56PHY_INTR_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	udelay(500);
	txgbe_wr32_ephy(hw, addr, rdata);

	udelay(500);
	rdata = 0;
	addr = E56PHY_INTR_0_ADDR;
	rdata = rd32_ephy(hw, addr);

	//16. Program ALIAS::RXS::RANGE_SEL = RANGE_FINAL
	rdata = 0x0000;
	addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_ANA_OVRDVAL_5_ANA_BBCDR_OSC_RANGE_SEL_I,
		  RANGE_FINAL);
	txgbe_wr32_ephy(hw, addr, rdata);

	//17. Program following before enabling RXS. Purpose is to disable power-up FSM control on ADC offset adaptation
	//Note: this step will be done in 2.3.3 RXS calibration and adaptation sequence

	//18. After this SEQ::RX_ENABLE can be done at any time. Note to ensure that ALIAS::RXS::RANGE_SEL = RANGE_FINAL configuration is retained.
	//Rmove the OVRDEN on rxs0_rx0_samp_cal_done_o

	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_0_OVRD_EN_RXS0_RX0_SAMP_CAL_DONE_O,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	//Do SEQ::RX_ENABLE
	rdata = 0;
	addr = E56PHY_PMD_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x1);
	addr = E56PHY_PMD_CFG_0_ADDR;
	txgbe_wr32_ephy(hw, addr, rdata);

	return status;
}

int E56phySetRxsUfineLeMax(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int rdata;
	unsigned int ULTRAFINE_CODE;

	unsigned int CMVAR_UFINE_MAX = 0;

	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		CMVAR_UFINE_MAX = S10G_CMVAR_UFINE_MAX;
	} else if (speed == TXGBE_LINK_SPEED_25GB_FULL) {
		CMVAR_UFINE_MAX = S25G_CMVAR_UFINE_MAX;
	}

	//a. Assign software defined variables as below �C
	//ii. ULTRAFINE_CODE = ALIAS::RXS::ULTRAFINE
	EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
	ULTRAFINE_CODE =
		EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_ultrafine_i);
	//Set ovrd_en=1 to overide ASIC value
	EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
	EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1, ovrd_en_ana_bbcdr_ultrafine_i) = 1;
	EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);

	//b. Perform the below logic sequence �C
	while (ULTRAFINE_CODE > CMVAR_UFINE_MAX) {
		ULTRAFINE_CODE = ULTRAFINE_CODE - 1;
		txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDVAL_5,
				      ana_bbcdr_ultrafine_i, ULTRAFINE_CODE);
		// Wait until 1milliseconds or greater
		msleep(10);
	}

	return status;
}

//--------------------------------------------------------------
//compare function for qsort()
//--------------------------------------------------------------
int compare(const void *a, const void *b)
{
	const int *num1 = (const int *)a;
	const int *num2 = (const int *)b;

	if (*num1 < *num2) {
		return -1;
	} else if (*num1 > *num2) {
		return 1;
	} else {
		return 0;
	}
}

int E56phyRxRdSecondCode(struct txgbe_hw *hw, int *SECOND_CODE)
{
	int status = 0, i, N, median;
	unsigned int rdata;
	int arraySize, RXS_BBCDR_SECOND_ORDER_ST[5];

	//Set ovrd_en=0 to read ASIC value
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_1,
			      ovrd_en_ana_bbcdr_int_cstm_i, 0);

	//As status update from RXS hardware is asynchronous to read status of SECOND_ORDER, follow sequence mentioned below.
	N = 5;
	for (i = 0; i < N; i = i + 1) {
		//set RXS_BBCDR_SECOND_ORDER_ST[i] = RXS::ANA_OVRDVAL[5]::ana_bbcdr_int_cstm_i[4:0]
		EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
		RXS_BBCDR_SECOND_ORDER_ST[i] = EPHY_XFLD(
			E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_int_cstm_i);
		udelay(100);
	}

	//sort array RXS_BBCDR_SECOND_ORDER_ST[i]
	arraySize = sizeof(RXS_BBCDR_SECOND_ORDER_ST) /
		    sizeof(RXS_BBCDR_SECOND_ORDER_ST[0]);
	sort(RXS_BBCDR_SECOND_ORDER_ST, arraySize, sizeof(int), compare, NULL);

	median = ((N + 1) / 2) - 1;
	*SECOND_CODE = RXS_BBCDR_SECOND_ORDER_ST[median];

	return status;
}

//--------------------------------------------------------------
//2.3.4 RXS post CDR lock temperature tracking sequence
//
//Below sequence must be run before the temperature drifts by >5degC after the CDR locks for the first time or after the
//ious time this sequence was run. It is recommended to call this sequence periodically (eg: once every 100ms) or trigger
// sequence if the temperature drifts by >=5degC. Temperature must be read from an on-die temperature sensor.
//--------------------------------------------------------------
int E56phyRxsPostCdrLockTempTrackSeq(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int rdata;
	int SECOND_CODE;
	int COARSE_CODE;
	int FINE_CODE;
	int ULTRAFINE_CODE;

	int CMVAR_SEC_LOW_TH;
	int CMVAR_UFINE_MAX = 0;
	int CMVAR_FINE_MAX;
	int CMVAR_UFINE_UMAX_WRAP = 0;
	int CMVAR_COARSE_MAX;
	int CMVAR_UFINE_FMAX_WRAP = 0;
	int CMVAR_FINE_FMAX_WRAP = 0;
	int CMVAR_SEC_HIGH_TH;
	int CMVAR_UFINE_MIN;
	int CMVAR_FINE_MIN;
	int CMVAR_UFINE_UMIN_WRAP;
	int CMVAR_COARSE_MIN;
	int CMVAR_UFINE_FMIN_WRAP;
	int CMVAR_FINE_FMIN_WRAP;

	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		CMVAR_SEC_LOW_TH = S10G_CMVAR_SEC_LOW_TH;
		CMVAR_UFINE_MAX = S10G_CMVAR_UFINE_MAX;
		CMVAR_FINE_MAX = S10G_CMVAR_FINE_MAX;
		CMVAR_UFINE_UMAX_WRAP = S10G_CMVAR_UFINE_UMAX_WRAP;
		CMVAR_COARSE_MAX = S10G_CMVAR_COARSE_MAX;
		CMVAR_UFINE_FMAX_WRAP = S10G_CMVAR_UFINE_FMAX_WRAP;
		CMVAR_FINE_FMAX_WRAP = S10G_CMVAR_FINE_FMAX_WRAP;
		CMVAR_SEC_HIGH_TH = S10G_CMVAR_SEC_HIGH_TH;
		CMVAR_UFINE_MIN = S10G_CMVAR_UFINE_MIN;
		CMVAR_FINE_MIN = S10G_CMVAR_FINE_MIN;
		CMVAR_UFINE_UMIN_WRAP = S10G_CMVAR_UFINE_UMIN_WRAP;
		CMVAR_COARSE_MIN = S10G_CMVAR_COARSE_MIN;
		CMVAR_UFINE_FMIN_WRAP = S10G_CMVAR_UFINE_FMIN_WRAP;
		CMVAR_FINE_FMIN_WRAP = S10G_CMVAR_FINE_FMIN_WRAP;
	} else if (speed == TXGBE_LINK_SPEED_25GB_FULL) {
		CMVAR_SEC_LOW_TH = S25G_CMVAR_SEC_LOW_TH;
		CMVAR_UFINE_MAX = S25G_CMVAR_UFINE_MAX;
		CMVAR_FINE_MAX = S25G_CMVAR_FINE_MAX;
		CMVAR_UFINE_UMAX_WRAP = S25G_CMVAR_UFINE_UMAX_WRAP;
		CMVAR_COARSE_MAX = S25G_CMVAR_COARSE_MAX;
		CMVAR_UFINE_FMAX_WRAP = S25G_CMVAR_UFINE_FMAX_WRAP;
		CMVAR_FINE_FMAX_WRAP = S25G_CMVAR_FINE_FMAX_WRAP;
		CMVAR_SEC_HIGH_TH = S25G_CMVAR_SEC_HIGH_TH;
		CMVAR_UFINE_MIN = S25G_CMVAR_UFINE_MIN;
		CMVAR_FINE_MIN = S25G_CMVAR_FINE_MIN;
		CMVAR_UFINE_UMIN_WRAP = S25G_CMVAR_UFINE_UMIN_WRAP;
		CMVAR_COARSE_MIN = S25G_CMVAR_COARSE_MIN;
		CMVAR_UFINE_FMIN_WRAP = S25G_CMVAR_UFINE_FMIN_WRAP;
		CMVAR_FINE_FMIN_WRAP = S25G_CMVAR_FINE_FMIN_WRAP;
	}

	//Assign software defined variables as below �C
	//a. SECOND_CODE = ALIAS::RXS::SECOND_ORDER
	status |= E56phyRxRdSecondCode(hw, &SECOND_CODE);

	//b. COARSE_CODE = ALIAS::RXS::COARSE
	//c. FINE_CODE = ALIAS::RXS::FINE
	//d. ULTRAFINE_CODE = ALIAS::RXS::ULTRAFINE
	EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
	COARSE_CODE = EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_coarse_i);
	FINE_CODE = EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i);
	ULTRAFINE_CODE =
		EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_ultrafine_i);

	if (SECOND_CODE <= CMVAR_SEC_LOW_TH) {
		if (ULTRAFINE_CODE < CMVAR_UFINE_MAX) {
			txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDVAL_5,
					      ana_bbcdr_ultrafine_i,
					      ULTRAFINE_CODE + 1);
			//Set ovrd_en=1 to overide ASIC value
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else if (FINE_CODE < CMVAR_FINE_MAX) {
			EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) =
				CMVAR_UFINE_UMAX_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i) =
				FINE_CODE + 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDVAL_5);
			//Note: All two of above code updates should be written in a single register write
			//Set ovrd_en=1 to overide ASIC value
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_fine_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else if (COARSE_CODE < CMVAR_COARSE_MAX) {
			EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) =
				CMVAR_UFINE_FMAX_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i) =
				CMVAR_FINE_FMAX_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_coarse_i) = COARSE_CODE + 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDVAL_5);
			//Note: All three of above code updates should be written in a single register write
			//Set ovrd_en=1 to overide ASIC value
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_coarse_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_fine_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else {
			printk("ERROR: (SECOND_CODE <= CMVAR_SEC_LOW_TH) temperature tracking occurs Error condition\n");
		}
	} else if (SECOND_CODE >= CMVAR_SEC_HIGH_TH) {
		if (ULTRAFINE_CODE > CMVAR_UFINE_MIN) {
			txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDVAL_5,
					      ana_bbcdr_ultrafine_i,
					      ULTRAFINE_CODE - 1);
			//Set ovrd_en=1 to overide ASIC value
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else if (FINE_CODE > CMVAR_FINE_MIN) {
			EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) =
				CMVAR_UFINE_UMIN_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i) =
				FINE_CODE - 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDVAL_5);
			//Note: All two of above code updates should be written in a single register write
			//Set ovrd_en=1 to overide ASIC value
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_fine_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else if (COARSE_CODE > CMVAR_COARSE_MIN) {
			EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) =
				CMVAR_UFINE_FMIN_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i) =
				CMVAR_FINE_FMIN_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_coarse_i) = COARSE_CODE - 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDVAL_5);
			//Note: All three of above code updates should be written in a single register write
			//Set ovrd_en=1 to overide ASIC value
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_coarse_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_fine_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else {
			printk("ERROR: (SECOND_CODE >= CMVAR_SEC_HIGH_TH) temperature tracking occurs Error condition\n");
		}
	}

	return status;
}

int E56phyRxsCalibAdaptSeq(struct txgbe_hw *hw, u32 speed)
{
	int status = 0, i;
	u32 addr, timer;
	u32 rdata = 0x0;

	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_EN_I,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_EN_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_DONE_O,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_DONE_O,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//2. Follow sequence described in 2.3.2 RXS Osc Initialization for temperature tracking range here. RXS would be enabled at the end of this sequence. For the case when PAM4 KR training is not enabled (including PAM4 mode without KR training), wait until ALIAS::PDIG::CTRL_FSM_RX_ST would return RX_TRAIN_15_ST (RX_RDY_ST).
	E56phyRxsOscInitForTempTrackRange(hw, speed);

	addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
	timer = 0;
	rdata = 0;
	while (EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx0_st) !=
	       E56PHY_RX_RDY_ST) {
		rdata = rd32_ephy(hw, addr);
		udelay(500);
		EPHY_RREG(E56G__PMD_CTRL_FSM_RX_STAT_0);
		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait CTRL_FSM_RX_STAT[0]::ctrl_fsm_rx0_st[5:0] = RX_RDY_ST Timeout!!!\n");
			break;
		}
	}

	//RXS ADC adaptation sequence
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_CDR_EN_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_CDR_EN_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//b. Wait for 0.5us or greater
	udelay(100);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_CDR_EN_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_CDR_EN_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0;
	timer = 0;
	while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_cdr_rdy_o) != 1) {
		EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
		udelay(500);

		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait RXS0_OVRDVAL[1]::rxs0_rx0_cdr_rdy_o =1 Timeout!!!\n");
			break;
		}
	}

	//4. Disable VGA and CTLE training so that they don't interfere with ADC calibration
	//a. Set ALIAS::RXS::VGA_TRAIN_EN = 0b0
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_VGA_TRAIN_EN_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_VGA_TRAIN_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//b. Set ALIAS::RXS::CTLE_TRAIN_EN = 0b0
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_CTLE_TRAIN_EN_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_CTLE_TRAIN_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//5. Perform ADC interleaver calibration
	//a. Remove the OVERRIDE on ALIAS::RXS::ADC_INTL_CAL_DONE
	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata,
		  E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_DONE_O,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	timer = 0;
	while (((rdata >>
		 E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_DONE_O_LSB) &
		1) != 1) {
		rdata = rd32_ephy(hw, addr);
		udelay(1000);

		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait  E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_DONE_O_LSB Timeout!!!\n");
			break;
		}
	}

	//6. Perform ADC offset adaptation and ADC gain adaptation, repeat them a few times and after that keep it disabled.
	for (i = 0; i < 16; i++) {
		//a. ALIAS::RXS::ADC_OFST_ADAPT_EN = 0b1
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
		rdata = rd32_ephy(hw, addr);
		SetFields(&rdata,
			  E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		//b. Wait for 1ms or greater
		rdata = 0;
		timer = 0;
		while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
				 rxs0_rx0_adc_ofst_adapt_done_o) != 1) {
			EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
			udelay(500);
			if (timer++ > PHYINIT_TIMEOUT) {
				printk("ERROR: Wait RXS0_OVRDVAL[1]::rxs0_rx0_adc_ofst_adapt_done_o =1 Timeout!!!\n");
				break;
			}
		}

		//c. ALIAS::RXS::ADC_OFST_ADAPT_EN = 0b0
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
		rdata = rd32_ephy(hw, addr);
		SetFields(&rdata,
			  E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		//d. ALIAS::RXS::ADC_GAIN_ADAPT_EN = 0b1
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
		rdata = rd32_ephy(hw, addr);
		SetFields(&rdata,
			  E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		//e. Wait for 1ms or greater
		rdata = 0;
		timer = 0;
		while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
				 rxs0_rx0_adc_gain_adapt_done_o) != 1) {
			EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
			udelay(500);

			if (timer++ > PHYINIT_TIMEOUT) {
				printk("ERROR: Wait E56G__PMD_RXS0_OVRDVAL_1[1]::rxs0_rx0_adc_gain_adapt_done_o =1 Timeout!!!\n");
				break;
			}
		}

		//f. ALIAS::RXS::ADC_GAIN_ADAPT_EN = 0b0
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
		rdata = rd32_ephy(hw, addr);
		SetFields(&rdata,
			  E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);
	}
	//g. Repeat #a to #f total 16 times

	//7. Perform ADC interleaver adaptation for 10ms or greater, and after that disable it
	//a. ALIAS::RXS::ADC_INTL_ADAPT_EN = 0b1
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);
	//b. Wait for 10ms or greater
	msleep(10);

	//c. ALIAS::RXS::ADC_INTL_ADAPT_EN = 0b0
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	//8. Now re-enable VGA and CTLE trainings, so that it continues to adapt tracking changes in temperature or voltage
	//a. Remove the OVERRIDE on ALIAS::RXS::VGA_TRAIN_EN
	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_VGA_TRAIN_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//b. Remove the OVERRIDE on ALIAS::RXS::CTLE_TRAIN_EN
	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_CTLE_TRAIN_EN_I,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	return status;
}

u32 txgbe_e56_cfg_temp(struct txgbe_hw *hw)
{
	struct txgbe_adapter *adapter = hw->back;
	u32 status;
	u32 value;
	int temp;

	status = txgbe_e56_get_temp(hw, &temp);
	if (status)
		temp = DEFAULT_TEMP;

	adapter->amlite_temp = temp;
	if (temp < DEFAULT_TEMP) {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN0);
		SetFields(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN0, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL2);
		SetFields(&value, 20, 16, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL2, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		SetFields(&value, 12, 12, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL7);
		SetFields(&value, 8, 4, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL7, value);
	} else if (temp > HIGH_TEMP) {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN0);
		SetFields(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN0, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL2);
		SetFields(&value, 20, 16, 0x3);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL2, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		SetFields(&value, 12, 12, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL7);
		SetFields(&value, 8, 4, 0x3);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL7, value);
	} else {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		SetFields(&value, 4, 4, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL4);
		SetFields(&value, 24, 24, 0x1);
		SetFields(&value, 31, 29, 0x4);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL4, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL5);
		SetFields(&value, 1, 0, 0x0);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL5, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		SetFields(&value, 23, 23, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL9);
		SetFields(&value, 24, 24, 0x1);
		SetFields(&value, 31, 29, 0x4);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL9, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL10);
		SetFields(&value, 1, 0, 0x0);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL10, value);
	}

	return 0;
}

//Reference setting code for SFP mode
int txgbe_set_link_to_amlite(struct txgbe_hw *hw, u32 speed)
{
	u32 value = 0;

	SetFields(&value, SFP1_TX_FAULT, 1);
	SetFields(&value, SFP1_TX_DISABLE, 1);
	SetFields(&value, SFP1_RS1, 1);
	SetFields(&value, SFP1_RS0, 1);
	wr32(hw, TXGBE_GPIO_DDR, value);

	SetFields(&value, SFP1_TX_FAULT, 0);
	SetFields(&value, SFP1_TX_DISABLE, 0);
	SetFields(&value, SFP1_RS1, 1);
	SetFields(&value, SFP1_RS0, 1);
	wr32(hw, TXGBE_GPIO_DR, value);

	/////////////////////////// XLGPCS REGS Start
	value = txgbe_rd32_epcs(hw, VR_PCS_DIG_CTRL1);
	value |= 0x8000;
	txgbe_wr32_epcs(hw, VR_PCS_DIG_CTRL1, value);

	udelay(1000);
	value = txgbe_rd32_epcs(hw, VR_PCS_DIG_CTRL1);
	if ((value & 0x8000)) {
		return -1;
	}

	value = txgbe_rd32_epcs(hw, SR_AN_CTRL);
	SetFields(&value, 12, 12, 0);
	txgbe_wr32_epcs(hw, SR_AN_CTRL, value);

	if (speed == TXGBE_LINK_SPEED_25GB_FULL) {
		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL1);
		SetFields(&value, 5, 2, 5);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL1, value);

		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL2);
		SetFields(&value, 3, 0, 7);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL2, value);

		value = txgbe_rd32_epcs(hw, SR_PMA_CTRL2);
		SetFields(&value, 6, 0, 0x39);
		txgbe_wr32_epcs(hw, SR_PMA_CTRL2, value);

		value = rd32_ephy(hw, ANA_OVRDVAL0);
		SetFields(&value, 29, 29, 0x1);
		SetFields(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL0, value);

		value = rd32_ephy(hw, ANA_OVRDVAL5);
		SetFields(&value, 24, 24, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL5, value);

		value = rd32_ephy(hw, ANA_OVRDEN0);
		SetFields(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN0, value);

		value = rd32_ephy(hw, ANA_OVRDEN1);
		SetFields(&value, 30, 30, 0x1);
		SetFields(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN1, value);

		value = rd32_ephy(hw, PLL0_CFG0);
		SetFields(&value, 25, 24, 0x1);
		SetFields(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL0_CFG0, value);

		value = rd32_ephy(hw, PLL0_CFG2);
		SetFields(&value, 12, 8, 0x4);
		txgbe_wr32_ephy(hw, PLL0_CFG2, value);

		value = rd32_ephy(hw, PLL1_CFG0);
		SetFields(&value, 25, 24, 0x1);
		SetFields(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL1_CFG0, value);

		value = rd32_ephy(hw, PLL1_CFG2);
		SetFields(&value, 12, 8, 0x8);
		txgbe_wr32_ephy(hw, PLL1_CFG2, value);

		value = rd32_ephy(hw, PLL0_DIV_CFG0);
		SetFields(&value, 18, 8, 0x294);
		SetFields(&value, 4, 0, 0x8);
		txgbe_wr32_ephy(hw, PLL0_DIV_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG0);
		SetFields(&value, 30, 28, 0x7);
		SetFields(&value, 26, 24, 0x5);
		SetFields(&value, 18, 16, 0x3);
		SetFields(&value, 14, 12, 0x5);
		SetFields(&value, 10, 8, 0x5);
		txgbe_wr32_ephy(hw, DATAPATH_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG1);
		SetFields(&value, 26, 24, 0x5);
		SetFields(&value, 10, 8, 0x5);
		SetFields(&value, 18, 16, 0x3);
		SetFields(&value, 2, 0, 0x3);
		txgbe_wr32_ephy(hw, DATAPATH_CFG1, value);

		value = rd32_ephy(hw, AN_CFG1);
		SetFields(&value, 4, 0, 0x9);
		txgbe_wr32_ephy(hw, AN_CFG1, value);

		txgbe_e56_cfg_temp(hw);
		txgbe_e56_cfg_25g(hw);

		value = rd32_ephy(hw, PMD_CFG0);
		SetFields(&value, 21, 20, 0x3);
		SetFields(&value, 19, 12, 0x1); //TX_EN set
		SetFields(&value, 8, 8, 0x0);
		SetFields(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, PMD_CFG0, value);
	}

	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL1);
		SetFields(&value, 5, 2, 0);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL1, value);

		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL2);
		SetFields(&value, 3, 0, 0);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL2, value);

		value = txgbe_rd32_epcs(hw, SR_PMA_CTRL2);
		SetFields(&value, 6, 0, 0xb);
		txgbe_wr32_epcs(hw, SR_PMA_CTRL2, value);

		value = rd32_ephy(hw, ANA_OVRDVAL0);
		SetFields(&value, 29, 29, 0x1);
		SetFields(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL0, value);

		value = rd32_ephy(hw, ANA_OVRDVAL5);
		SetFields(&value, 24, 24, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL5, value);

		value = rd32_ephy(hw, ANA_OVRDEN0);
		SetFields(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN0, value);

		value = rd32_ephy(hw, ANA_OVRDEN1);
		SetFields(&value, 30, 30, 0x1);
		SetFields(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN1, value);

		value = rd32_ephy(hw, PLL0_CFG0);
		SetFields(&value, 25, 24, 0x1);
		SetFields(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL0_CFG0, value);

		value = rd32_ephy(hw, PLL0_CFG2);
		SetFields(&value, 12, 8, 0x4);
		txgbe_wr32_ephy(hw, PLL0_CFG2, value);

		value = rd32_ephy(hw, PLL1_CFG0);
		SetFields(&value, 25, 24, 0x1);
		SetFields(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL1_CFG0, value);

		value = rd32_ephy(hw, PLL1_CFG2);
		SetFields(&value, 12, 8, 0x8);
		txgbe_wr32_ephy(hw, PLL1_CFG2, value);

		value = rd32_ephy(hw, PLL0_DIV_CFG0);
		SetFields(&value, 18, 8, 0x294);
		SetFields(&value, 4, 0, 0x8);
		txgbe_wr32_ephy(hw, PLL0_DIV_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG0);
		SetFields(&value, 30, 28, 0x7);
		SetFields(&value, 26, 24, 0x5);
		SetFields(&value, 18, 16, 0x5);
		SetFields(&value, 14, 12, 0x5);
		SetFields(&value, 10, 8, 0x5);
		txgbe_wr32_ephy(hw, DATAPATH_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG1);
		SetFields(&value, 26, 24, 0x5);
		SetFields(&value, 10, 8, 0x5);
		SetFields(&value, 18, 16, 0x5);
		SetFields(&value, 2, 0, 0x5);
		txgbe_wr32_ephy(hw, DATAPATH_CFG1, value);

		value = rd32_ephy(hw, AN_CFG1);
		SetFields(&value, 4, 0, 0x2);
		txgbe_wr32_ephy(hw, AN_CFG1, value);

		txgbe_e56_cfg_temp(hw);
		txgbe_e56_cfg_10g(hw);

		value = rd32_ephy(hw, PMD_CFG0);
		SetFields(&value, 21, 20, 0x3);
		SetFields(&value, 19, 12, 0x1); //TX_EN set
		SetFields(&value, 8, 8, 0x0);
		SetFields(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, PMD_CFG0, value);
	}

	E56phyRxsCalibAdaptSeq(hw, speed);

	//Step 2 of 2.3.4
	E56phySetRxsUfineLeMax(hw, speed);

	//2.3.4 RXS post CDR lock temperature tracking sequence
	E56phyRxsPostCdrLockTempTrackSeq(hw, speed);

	return 0;
}
