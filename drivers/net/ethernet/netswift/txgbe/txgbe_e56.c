#include "txgbe_e56.h"
#include "txgbe_hw.h"

#define EPHY_RREG(REG)                             \
	do {                                       \
		rdata = 0;                         \
		rdata = rd32_ephy(hw, REG##_ADDR); \
	} while (0)

#define EPHY_WREG(REG)                                                         \
	do {                                                                   \
		txgbe_wr32_ephy(hw, rdata) printk("Write A: 0x%x,  D: 0x%x\n", \
						  REG##_ADDR, rdata);          \
	} while (0)

#define EPCS_RREG(REG)                                   \
	do {                                             \
		rdata = 0;                               \
		rdata = txgbe_rd32_epcs(hw, REG##_ADDR); \
	} while (0)

#define EPCS_WREG(REG)                                                         \
	do {                                                                   \
		txgbe_wr32_epcs(hw, rdata) printk("Write A: 0x%x,  D: 0x%x\n", \
						  REG##_ADDR, rdata);          \
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

u32 E56phyTxFfeCfg(struct txgbe_hw *hw)
{
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

	E56phyTxFfeCfg(hw);

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
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_VCO_CODE_INIT, 0x7fb);
	SetFields(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_OSC_RANGE_SEL1, 0x1);
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
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P1, 0x18);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P2, 0x0);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P3, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P1, 0x1);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P2, 0x0);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P3, 0x0);
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
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_8_TRAIN_ST7_EN, 0x47bf);
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
	SetFields(&rdata, E56PHY_FETX_FFE_TRAIN_CFG_0_KRT_FETX_INIT_FFE_CFG_3,
		  0x2);
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

	addr = E56G_RXS0_ANA_OVRDEN_1_ADDR;
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
	E56phyTxFfeCfg(hw);

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
	((E56G_RXS0_OSC_CAL_N_CDR_4 *)&rdata)->vco_code_init = 0x7ff;
	((E56G_RXS0_OSC_CAL_N_CDR_4 *)&rdata)->osc_range_sel0 = 0x2;
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
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P1, 0x18);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P2, 0x0);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P3, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P1, 0x1);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P2, 0x0);
	SetFields(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P3, 0x0);
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
	SetFields(&rdata, E56PHY_CTRL_FSM_CFG_8_TRAIN_ST7_EN, 0x47bf);
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
	SetFields(&rdata, E56PHY_FETX_FFE_TRAIN_CFG_0_KRT_FETX_INIT_FFE_CFG_3,
		  0x2);
	txgbe_wr32_ephy(hw, addr, rdata);

	return 0;
}

int E56phyRxsOscInitForTempTrackRange(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int addr, rdata, timer;
	int T = 40;
	int RX_COARSE_MID_TD, CMVAR_RANGE_H = 0, CMVAR_RANGE_L = 0;
	int OFFSET_CENTRE_RANGE_H, OFFSET_CENTRE_RANGE_L;
	int osc_freq_err_occur;

	txgbe_e56_get_temp(hw, &T);

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
			break;
			return -1;
		}
	}

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

	rdata = 0;
	addr = E56PHY_PMD_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);

	rdata &= 0xfff0ffff;
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
			break;
			return -1;
		}
	}

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
			break;
			return -1;
		} //if (timer++ > PHYINIT_TIMEOUT) {
	}

	rdata = 0;
	addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	OFFSET_CENTRE_RANGE_L = (rdata >> 4) & 0xf;
	//if(osc_freq_err_occur) {
	if (OFFSET_CENTRE_RANGE_L > RX_COARSE_MID_TD) {
		OFFSET_CENTRE_RANGE_L =
			OFFSET_CENTRE_RANGE_L - RX_COARSE_MID_TD;
	} else {
		OFFSET_CENTRE_RANGE_L =
			RX_COARSE_MID_TD - OFFSET_CENTRE_RANGE_L;
	}

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
			break;
			return -1;
		} //if (timer++ > PHYINIT_TIMEOUT) {
	}

	if (OFFSET_CENTRE_RANGE_L > OFFSET_CENTRE_RANGE_H) {
		rdata = 0x0000;
		addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR;
		rdata = rd32_ephy(hw, addr);
		SetFields(&rdata, 1, 0, CMVAR_RANGE_H);
		txgbe_wr32_ephy(hw, addr, rdata);
	}

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

	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_RXS0_OVRDEN_0_OVRD_EN_RXS0_RX0_SAMP_CAL_DONE_O,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0;
	addr = E56PHY_PMD_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	SetFields(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x1);
	addr = E56PHY_PMD_CFG_0_ADDR;
	txgbe_wr32_ephy(hw, addr, rdata);

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
	while ((rdata & 0x1F) != E56PHY_RX_RDY_ST) {
		rdata = rd32_ephy(hw, addr);
		udelay(500);

		if (timer++ > PHYINIT_TIMEOUT) {
			printk("ERROR: Wait CTRL_FSM_RX_STAT[0]::ctrl_fsm_rx0_st[5:0] = RX_RDY_ST Timeout!!!\n");
			break;
			return -1;
		}
	}

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

u32 txgbe_e56_cfg_25g_temp(struct txgbe_hw *hw)
{
	u32 status;
	u32 value;
	int temp;

	status = txgbe_e56_get_temp(hw, &temp);
	if (status)
		temp = DEFAULT_TEMP;

	if (temp < DEFAULT_TEMP) {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN0);
		SetFields(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN0, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL2);
		SetFields(&value, 20, 16, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL2, value);
	} else if (temp > HIGH_TEMP) {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN0);
		SetFields(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN0, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL2);
		SetFields(&value, 20, 16, 0x3);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL2, value);
	} else {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		SetFields(&value, 4, 4, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN0, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL4);
		SetFields(&value, 24, 24, 0x1);
		SetFields(&value, 31, 29, 0x4);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL4, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL5);
		SetFields(&value, 1, 0, 0x0);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL5, value);
	}

	return 0;
}

u32 txgbe_e56_cfg_10g_temp(struct txgbe_hw *hw)
{
	u32 status;
	u32 value;
	int temp;

	status = txgbe_e56_get_temp(hw, &temp);
	if (status)
		temp = DEFAULT_TEMP;

	if (temp < DEFAULT_TEMP) {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		SetFields(&value, 12, 12, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN0, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL7);
		SetFields(&value, 8, 4, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL7, value);
	} else if (temp > HIGH_TEMP) {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		SetFields(&value, 12, 12, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL7);
		SetFields(&value, 8, 4, 0x3);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL7, value);
	} else {
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

		txgbe_e56_cfg_25g_temp(hw);
		txgbe_e56_cfg_25g(hw);

		value = rd32_ephy(hw, PMD_CFG0);
		SetFields(&value, 21, 20, 0x3);
		SetFields(&value, 19, 12, 0x1); //TX_EN set
		SetFields(&value, 8, 8, 0x0);
		SetFields(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, PMD_CFG0, value);

		E56phyRxsCalibAdaptSeq(hw, speed);
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

		txgbe_e56_cfg_10g_temp(hw);
		txgbe_e56_cfg_10g(hw);

		value = rd32_ephy(hw, PMD_CFG0);
		SetFields(&value, 21, 20, 0x3);
		SetFields(&value, 19, 12, 0x1); //TX_EN set
		SetFields(&value, 8, 8, 0x0);
		SetFields(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, PMD_CFG0, value);

		E56phyRxsCalibAdaptSeq(hw, speed);
	}

	return 0;
}
