// SPDX-License-Identifier: GPL-2.0
#include <asm/iee-selinuxp.h>
#include <asm/iee-def.h>
#include "security.h"
#include "ss/services.h"

#ifdef CONFIG_IEE_SELINUX_P
inline void iee_set_selinux_status_pg(struct page *new_page)
{
	iee_rw_gate(IEE_SEL_SET_STATUS_PG, new_page);
}

inline void enforcing_set(bool value)
{
	iee_rw_gate(IEE_SEL_SET_ENFORCING, value);
}

inline void selinux_mark_initialized(void)
{
	iee_rw_gate(IEE_SEL_SET_INITIALIZED);
}

inline void iee_set_sel_policy_cap(unsigned int idx, int cap)
{
	iee_rw_gate(IEE_SEL_SET_POLICY_CAP, idx, cap);
}

/*
 * Please make sure param iee_new_policy is from policy_jar memcache.
 * Need to free new_policy after calling this func as it's only used to
 * trans data from kernel.
 */
inline void iee_sel_rcu_assign_policy(struct selinux_policy *new_policy,
				      struct selinux_policy *iee_new_policy)
{
	iee_rw_gate(IEE_SEL_RCU_ASSIGN_POLICY, new_policy, iee_new_policy);
}
#endif
