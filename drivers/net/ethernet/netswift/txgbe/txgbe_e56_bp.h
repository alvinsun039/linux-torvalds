#ifndef _TXGBE_E56_BP_H_
#define _TXGBE_E56_BP_H_

#define TXGBE_E56_AN_TXDIS	BIT(3)
#define TXGBE_E56_AN_PG_RCV	BIT(2)
#define TXGBE_E56_AN_INC_LINK	BIT(1)
#define TXGBE_E56_AN_INT_CMPLT	BIT(0)

#define TXGBE_10G_FEC_REQ	BIT(15)
#define TXGBE_10G_FEC_ABL	BIT(14)
#define TXGBE_25G_BASE_FEC_REQ	BIT(13)
#define TXGBE_25G_RS_FEC_REQ	BIT(12)

int txgbe_e56_set_link_to_kr(struct txgbe_adapter *adapter, unsigned char byLinkMode, unsigned int bypassCtle);
void txgbe_e56_bp_watchdog_event(struct txgbe_adapter *adapter);
void txgbe_e65_bp_down_event(struct txgbe_adapter *adapter);

#endif
