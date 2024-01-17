/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NETNS_NFTABLES_H_
#define _NETNS_NFTABLES_H_
#include <linux/ky_kabi.h>

struct netns_nftables {
	u8			gencursor;

	KY_KABI_RESERVE(1)
	KY_KABI_RESERVE(2)
};

#endif
