#ifndef _TXGBE_E56_BP_H_
#define _TXGBE_E56_BP_H_

int txgbe_e56_set_link_to_kr(struct txgbe_adapter *adapter,
			     unsigned char byLinkMode, unsigned int bypassCtle);
void txgbe_e56_bp_watchdog_event(struct txgbe_adapter *adapter);
void txgbe_e65_bp_down_event(struct txgbe_adapter *adapter);

#endif
