/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_SELINUX_P_H
#define _LINUX_IEE_SELINUX_P_H

#include <asm/iee.h>
#include <linux/mutex.h>
#include "security.h"
#include "ss/services.h"
void __iee_code _iee_set_selinux_status_pg(unsigned long __unused, struct page *new_page);
void __iee_code _iee_set_selinux_enforcing(unsigned long __unused, bool value);
void __iee_code _iee_mark_selinux_initialized(unsigned long __unused);
void __iee_code _iee_set_sel_policy_cap(unsigned long __unused, unsigned int idx, int cap);
void __iee_code _iee_sel_rcu_assign_policy(unsigned long __unused,
	struct selinux_policy *new_policy, struct selinux_policy *iee_new_policy);

extern unsigned long long iee_rw_gate(int flag, ...);
static inline struct mutex *iee_get_selinux_policy_lock(void)
{
	return (struct mutex *)(selinux_state.policy_mutex.owner.counter);
}

static inline struct mutex *iee_get_selinux_status_lock(void)
{
	return (struct mutex *)(selinux_state.status_lock.owner.counter);
}

/* APIs for modifying selinux_state */
extern void iee_set_selinux_status_pg(struct page *new_page);
extern void iee_set_sel_policy_cap(unsigned int idx, int cap);
extern void iee_sel_rcu_assign_policy(struct selinux_policy *new_policy,
		struct selinux_policy *iee_new_policy);

extern struct kmem_cache *policy_jar;

#endif
