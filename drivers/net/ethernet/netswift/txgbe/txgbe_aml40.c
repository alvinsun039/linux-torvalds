#include "txgbe_type.h"
#include "txgbe_hw.h"
#include "txgbe_aml40.h"
#include "txgbe_e56.h"
#include "txgbe_e56_bp.h"
#include "txgbe_phy.h"

#include "txgbe.h"

/**
 *  txgbe_get_media_type_aml40 - Get media type
 *  @hw: pointer to hardware structure
 *
 *  Returns the media type (fiber, copper, backplane)
 **/
enum txgbe_media_type txgbe_get_media_type_aml40(struct txgbe_hw *hw)
{
	return txgbe_media_type_fiber_qsfp;
}

/**
 *  txgbe_setup_mac_link_aml - Set MAC link speed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: true when waiting for completion is needed
 *
 *  Set the link speed in the AUTOC register and restarts link.
 **/
static s32 txgbe_setup_mac_link_aml40(struct txgbe_hw *hw,
			       u32 speed,
			       bool autoneg_wait_to_complete)
{
	bool autoneg = false;
	s32 status = 0;
	u32 link_capabilities = TXGBE_LINK_SPEED_UNKNOWN;
	struct txgbe_adapter *adapter = hw->back;
	u32 link_speed = TXGBE_LINK_SPEED_UNKNOWN;
	bool link_up = false;
	s32 ret_status = 0;
	int i = 0, j = 0;
	u32 value = 0;

	/* Check to see if speed passed in is supported. */
	status = TCALL(hw, mac.ops.get_link_capabilities,
			       &link_capabilities, &autoneg);
	if (status)
		goto out;

	speed &= link_capabilities;

	if (speed == TXGBE_LINK_SPEED_UNKNOWN) {
		status = TXGBE_ERR_LINK_SETUP;
		goto out;
	}

	if (!(hw->dac_sfp)) {
		status = TCALL(hw, mac.ops.check_link,
				&link_speed, &link_up, false);

		if (status != 0)
			goto out;

		if ((link_speed == speed) && link_up && adapter->phy_tx_ready &&
			(adapter->fec_link_mode & adapter->cur_fec_link)) {
				goto out;
		}
	}

	if (!adapter->phy_tx_ready) {
		mutex_lock(&adapter->e56_lock);
		ret_status = txgbe_set_link_to_amlite(hw, speed);
		mutex_unlock(&adapter->e56_lock);
	} else {
		mutex_lock(&adapter->e56_lock);
		/* this ret_status for workaorund not return to upper*/
		ret_status = txgbe_e56_reconfig_rx(hw, speed);
		mutex_unlock(&adapter->e56_lock);
	}

	if (ret_status == TXGBE_ERR_PHY_INIT_NOT_DONE)
		goto out;

	do {
		if (speed != TXGBE_LINK_SPEED_40GB_FULL)
			goto out;

		if (!(adapter->fec_link_mode & BIT(j)) &&
			!((adapter->fec_link_mode == TXGBE_PHY_FEC_AUTO) && (j == 3))) {
			j += 1;
			continue;
		}

		/*revert to rs fec mode if all fec mode cannot link when auto try*/
		if ((adapter->fec_link_mode == TXGBE_PHY_FEC_AUTO) && (j == 3))
			adapter->cur_fec_link = TXGBE_PHY_FEC_RS;
		else
			adapter->cur_fec_link = adapter->fec_link_mode & BIT(j);

		/*if in fec auto mode, try another fec mode after no link in 1s*/
		/* for lr sfp, enable KR-FEC to link up with mellonax and intel */
		mutex_lock(&adapter->e56_lock);
		if (adapter->cur_fec_link  & TXGBE_PHY_FEC_RS) {
			//disable BASER FEC
			value = txgbe_rd32_epcs(hw, SR_PMA_KR_FEC_CTRL);
			SetFields(&value, 0, 0, 0);
			txgbe_wr32_epcs(hw, SR_PMA_KR_FEC_CTRL, value);

			//enable RS FEC
			txgbe_wr32_epcs(hw, 0x180a3, 0x68c1);
			txgbe_wr32_epcs(hw, 0x180a4, 0x3321);
			txgbe_wr32_epcs(hw, 0x180a5, 0x973e);
			txgbe_wr32_epcs(hw, 0x180a6, 0xccde);

			txgbe_wr32_epcs(hw, 0x38018, 1024);
			value = txgbe_rd32_epcs(hw, 0x100c8);
			SetFields(&value, 2, 2, 1);
			txgbe_wr32_epcs(hw, 0x100c8, value);
		} else if (adapter->cur_fec_link & TXGBE_PHY_FEC_BASER) {
			//disable RS FEC
			txgbe_wr32_epcs(hw, 0x180a3, 0x7690);
			txgbe_wr32_epcs(hw, 0x180a4, 0x3347);
			txgbe_wr32_epcs(hw, 0x180a5, 0x896f);
			txgbe_wr32_epcs(hw, 0x180a6, 0xccb8);
			txgbe_wr32_epcs(hw, 0x38018, 0x3fff);
			value = txgbe_rd32_epcs(hw, 0x100c8);
			SetFields(&value, 2, 2, 0);
			txgbe_wr32_epcs(hw, 0x100c8, value);

			//enable BASER FEC
			value = txgbe_rd32_epcs(hw, SR_PMA_KR_FEC_CTRL);
			SetFields(&value, 0, 0, 1);
			txgbe_wr32_epcs(hw, SR_PMA_KR_FEC_CTRL, value);
		} else {
			//disable RS FEC
			txgbe_wr32_epcs(hw, 0x180a3, 0x7690);
			txgbe_wr32_epcs(hw, 0x180a4, 0x3347);
			txgbe_wr32_epcs(hw, 0x180a5, 0x896f);
			txgbe_wr32_epcs(hw, 0x180a6, 0xccb8);
			txgbe_wr32_epcs(hw, 0x38018, 0x3fff);
			value = txgbe_rd32_epcs(hw, 0x100c8);
			SetFields(&value, 2, 2, 0);
			txgbe_wr32_epcs(hw, 0x100c8, value);

			//disable BASER FEC
			value = txgbe_rd32_epcs(hw, SR_PMA_KR_FEC_CTRL);
			SetFields(&value, 0, 0, 0);
			txgbe_wr32_epcs(hw, SR_PMA_KR_FEC_CTRL, value);
		}
		mutex_unlock(&adapter->e56_lock);

		for (i = 0; i < 4; i++) {
			msleep(250);
			TCALL(hw, mac.ops.check_link,
			&link_speed, &link_up, false);
			if (link_up)
				goto out;
		}
		j += 1;
	} while (j < 4);

out:
	return status;
}

/**
 *  txgbe_get_link_capabilities_aml40 - Determines link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @autoneg: true when autoneg or autotry is enabled
 *
 *  Determines the link capabilities by reading the AUTOC register.
 **/
static s32 txgbe_get_link_capabilities_aml40(struct txgbe_hw *hw,
				      u32 *speed,
				      bool *autoneg)
{
	s32 status = 0;

	if (hw->phy.sfp_type == txgbe_sfp_type_40g_core0 ||
		hw->phy.sfp_type == txgbe_sfp_type_40g_core1) {
		*speed = TXGBE_LINK_SPEED_40GB_FULL;
		*autoneg = false;
	} else {
		*speed = TXGBE_LINK_SPEED_40GB_FULL;
		*autoneg = true;
	}

	return status;
}

/**
 *  txgbe_check_mac_link_aml40 - Determine link and speed status
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @link_up: true when link is up
 *  @link_up_wait_to_complete: bool used to wait for link up or not
 *
 *  Reads the links register to determine if link is up and the current speed
 **/
static s32 txgbe_check_mac_link_aml40(struct txgbe_hw *hw, u32 *speed,
				bool *link_up, bool link_up_wait_to_complete)
{
	u32 links_reg = 0;
	u32 i;

	if (link_up_wait_to_complete) {
		for (i = 0; i < TXGBE_LINK_UP_TIME; i++) {
			links_reg = rd32(hw,
						TXGBE_CFG_PORT_ST);
			if (links_reg & TXGBE_CFG_PORT_ST_LINK_UP) {
				*link_up = true;
				break;
			} else {
				*link_up = false;
			}
			msleep(100);
		}
	} else {
		links_reg = rd32(hw, TXGBE_CFG_PORT_ST);
		if (links_reg & TXGBE_CFG_PORT_ST_LINK_UP)
			*link_up = true;
		else
			*link_up = false;
	}

	if (*link_up) {
		if ((links_reg & TXGBE_CFG_PORT_ST_AML_LINK_40G) ==
				TXGBE_CFG_PORT_ST_AML_LINK_40G)
			*speed = TXGBE_LINK_SPEED_40GB_FULL;
	} else {
		*speed = TXGBE_LINK_SPEED_UNKNOWN;
	}

	return 0;
}

static void txgbe_init_mac_link_ops_aml40(struct txgbe_hw *hw)
{
	struct txgbe_mac_info *mac = &hw->mac;

	mac->ops.disable_tx_laser =
			txgbe_disable_tx_laser_multispeed_fiber;
	mac->ops.enable_tx_laser =
			txgbe_enable_tx_laser_multispeed_fiber;
	mac->ops.flap_tx_laser = txgbe_flap_tx_laser_multispeed_fiber;

	mac->ops.setup_link = txgbe_setup_mac_link_aml40;
	mac->ops.set_rate_select_speed = txgbe_set_hard_rate_select_speed;
}

static s32 txgbe_setup_sfp_modules_aml40(struct txgbe_hw *hw)
{
	s32 ret_val = 0;

	DEBUGFUNC("txgbe_setup_sfp_modules_aml40");

	if (hw->phy.sfp_type != txgbe_sfp_type_unknown) {
		txgbe_init_mac_link_ops_aml40(hw);

		hw->phy.ops.reset = NULL;
	}

	return ret_val;
}
#if 0 //fix compile warnning, unused now
/**
 *  txgbe_init_phy_ops - PHY/SFP specific init
 *  @hw: pointer to hardware structure
 *
 *  Initialize any function pointers that were not able to be
 *  set during init_shared_code because the PHY/SFP type was
 *  not known.  Perform the SFP init if necessary.
 *
 **/
s32 txgbe_init_phy_ops_aml40(struct txgbe_hw *hw)
{
	struct txgbe_adapter *adapter = hw->back;
	s32 ret_val = 0;

	txgbe_init_i2c(hw);
	wr32(hw, 0x11220, 0xF);

	mutex_init(&adapter->e56_lock);
	/* Identify the PHY or SFP module */
	ret_val = TCALL(hw, phy.ops.identify);
	if (ret_val == TXGBE_ERR_SFP_NOT_SUPPORTED)
		goto init_phy_ops_out;

	/* Setup function pointers based on detected SFP module and speeds */
	txgbe_init_mac_link_ops_aml40(hw);
	if (hw->phy.sfp_type != txgbe_sfp_type_unknown)
		hw->phy.ops.reset = NULL;

init_phy_ops_out:
	return ret_val;
}
#endif
s32 txgbe_init_ops_aml40(struct txgbe_hw *hw)
{
	struct txgbe_mac_info *mac = &hw->mac;
	s32 ret_val = 0;

	ret_val = txgbe_init_ops_generic(hw);

	/* MAC */
	mac->ops.get_media_type = txgbe_get_media_type_aml40;
	mac->ops.setup_sfp = txgbe_setup_sfp_modules_aml40;

	/* LINK */
	mac->ops.check_link = txgbe_check_mac_link_aml40;
	mac->ops.setup_link = txgbe_setup_mac_link_aml40;
	mac->ops.get_link_capabilities = txgbe_get_link_capabilities_aml40;

	return ret_val;
}

