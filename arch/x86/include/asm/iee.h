/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_H
#define _LINUX_IEE_H
#include <asm/pgtable_types.h>
#include <linux/sched.h>

extern unsigned long iee_offset;
extern unsigned long IEE_OFFSET;
#define __iee_code		__section(".iee.text")
#define __iee_header  __section(".iee.text.header")
void __iee_code _iee_memcpy(unsigned long __unused, void *dst, void *src, size_t n);
void __iee_code _iee_memset(unsigned long __unused, void *ptr, int data, size_t n);
void __iee_code _iee_set_freeptr(unsigned long __unused, void **pptr, void *ptr);
void __iee_code _iee_split_huge_pmd(unsigned long __unused,
	pmd_t *pmdp, pte_t *pgtable);
void __iee_code _iee_set_token_pgd(unsigned long __unused,
	struct task_struct *tsk, pgd_t *pgd);
void __iee_code _iee_init_token(unsigned long __unused,
	struct task_struct *tsk, void *iee_stack, void *tmp_page);
void __iee_code _iee_invalidate_token(unsigned long __unused,
	struct task_struct *tsk);
void __iee_code _iee_validate_token(unsigned long __unused,
	struct task_struct *tsk);
void __iee_code _iee_unset_token(unsigned long __unused, pte_t *token_ptep,
	pte_t *token_page_ptep, void *token, void *token_page, unsigned long order);
void __iee_code _iee_set_token(unsigned long __unused,
	pte_t *token_ptep, pte_t *new_ptep, void *new, unsigned long order, int use_block_pmd);
unsigned long __iee_code _iee_test_and_clear_bit(unsigned long __unused,
	long nr, volatile unsigned long *addr);
void __iee_code _iee_set_sensitive_pte(unsigned long __unused,
	pte_t *lm_ptep, int order, int use_block_pmd);
void __iee_code _iee_unset_sensitive_pte(unsigned long __unused,
	pte_t *lm_ptep, int order, int use_block_pmd);
#ifdef CONFIG_PTP
void __iee_code _iee_set_pte(unsigned long __unused,
	pte_t *ptep, pte_t pte);
void __iee_code _iee_set_pmd(unsigned long __unused,
	pmd_t *pmdp, pmd_t pmd);
void __iee_code _iee_set_pud(unsigned long __unused,
	pud_t *pudp, pud_t pud);
void __iee_code _iee_set_p4d(unsigned long __unused,
	p4d_t *p4dp, p4d_t p4d);
void __iee_code _iee_set_pgd(unsigned long __unused,
	pgd_t *pgdp, pgd_t pgd);
#endif
#ifdef CONFIG_CREDP
#include <linux/cred.h>
extern struct cred init_cred;
void __iee_code _iee_set_cred_uid(unsigned long __unused, struct cred *cred, kuid_t uid);
void __iee_code _iee_set_cred_gid(unsigned long __unused, struct cred *cred, kgid_t gid);
void __iee_code _iee_copy_cred(unsigned long __unused, struct cred *old, struct cred *new);
void __iee_code _iee_set_cred_suid(unsigned long __unused, struct cred *cred, kuid_t suid);
void __iee_code _iee_set_cred_sgid(unsigned long __unused, struct cred *cred, kgid_t sgid);
void __iee_code _iee_set_cred_euid(unsigned long __unused, struct cred *cred, kuid_t euid);
void __iee_code _iee_set_cred_egid(unsigned long __unused, struct cred *cred, kgid_t egid);
void __iee_code _iee_set_cred_fsuid(unsigned long __unused, struct cred *cred, kuid_t fsuid);
void __iee_code _iee_set_cred_fsgid(unsigned long __unused, struct cred *cred, kgid_t fsgid);
void __iee_code _iee_set_cred_user(unsigned long __unused, struct cred *cred, struct user_struct *user);
void __iee_code _iee_set_cred_user_ns(unsigned long __unused, struct cred *cred, struct user_namespace *user_ns);
void __iee_code _iee_set_cred_group_info(unsigned long __unused, struct cred *cred, struct group_info *group_info);
void __iee_code _iee_set_cred_securebits(unsigned long __unused, struct cred *cred, unsigned int securebits);
void __iee_code _iee_set_cred_cap_inheritable(unsigned long __unused, struct cred *cred, kernel_cap_t cap_inheritable);
void __iee_code _iee_set_cred_cap_permitted(unsigned long __unused, struct cred *cred, kernel_cap_t cap_permitted);
void __iee_code _iee_set_cred_cap_effective(unsigned long __unused, struct cred *cred, kernel_cap_t cap_effective);
void __iee_code _iee_set_cred_cap_bset(unsigned long __unused, struct cred *cred, kernel_cap_t cap_bset);
void __iee_code _iee_set_cred_cap_ambient(unsigned long __unused, struct cred *cred, kernel_cap_t cap_ambient);
void __iee_code _iee_set_cred_jit_keyring(unsigned long __unused, struct cred *cred, unsigned char jit_keyring);
void __iee_code _iee_set_cred_session_keyring(unsigned long __unused, struct cred *cred, struct key *session_keyring);
void __iee_code _iee_set_cred_process_keyring(unsigned long __unused, struct cred *cred, struct key *process_keyring);
void __iee_code _iee_set_cred_thread_keyring(unsigned long __unused, struct cred *cred, struct key *thread_keyring);
void __iee_code _iee_set_cred_request_key_auth(unsigned long __unused, struct cred *cred, struct key *request_key_auth);
void __iee_code _iee_set_cred_non_rcu(unsigned long __unused, struct cred *cred, int non_rcu);
void __iee_code _iee_set_cred_atomic_set_usage(unsigned long __unused, struct cred *cred, int i);
unsigned long __iee_code _iee_set_cred_atomic_op_usage(unsigned long __unused, struct cred *cred, int flag, int nr);
void __iee_code _iee_set_cred_security(unsigned long __unused, struct cred *cred, void *security);
void __iee_code _iee_set_cred_rcu(unsigned long __unused, struct cred *cred, struct rcu_head *rcu);
void __iee_code _iee_set_cred_ucounts(unsigned long __unused, struct cred *cred, struct ucounts *ucounts);
#endif
#ifdef CONFIG_KEYP
#include<linux/key.h>
struct watch_list;
void __iee_code _iee_set_key_union(unsigned long __unused, struct key *key, struct key_union *key_union);
void __iee_code _iee_set_key_struct(unsigned long __unused, struct key *key, struct key_struct *key_struct);
void __iee_code _iee_set_key_payload(unsigned long __unused, struct key *key, union key_payload *key_payload);
unsigned long __iee_code _iee_set_key_usage(unsigned long __unused, struct key *key, int n, int flag);
void __iee_code _iee_set_key_serial(unsigned long __unused, struct key *key, key_serial_t serial);
void __iee_code _iee_set_key_watchers(unsigned long __unused, struct key *key, struct watch_list *watchers);
void __iee_code _iee_set_key_user(unsigned long __unused, struct key *key, struct key_user *user);
void __iee_code _iee_set_key_security(unsigned long __unused, struct key *key, void *security);
void __iee_code _iee_set_key_expiry(unsigned long __unused, struct key *key, time64_t expiry);
void __iee_code _iee_set_key_revoked_at(unsigned long __unused, struct key *key, time64_t revoked_at);
void __iee_code _iee_set_key_last_used_at(unsigned long __unused, struct key *key, time64_t last_used_at);
void __iee_code _iee_set_key_uid(unsigned long __unused, struct key *key, kuid_t uid);
void __iee_code _iee_set_key_gid(unsigned long __unused, struct key *key, kgid_t gid);
void __iee_code _iee_set_key_perm(unsigned long __unused, struct key *key, key_perm_t perm);
void __iee_code _iee_set_key_quotalen(unsigned long __unused, struct key *key, unsigned short quotalen);
void __iee_code _iee_set_key_datalen(unsigned long __unused, struct key *key, unsigned short datalen);
void __iee_code _iee_set_key_state(unsigned long __unused, struct key *key, short state);
void __iee_code _iee_set_key_magic(unsigned long __unused, struct key *key, unsigned int magic);
void __iee_code _iee_set_key_flags(unsigned long __unused, struct key *key, unsigned long flags);
void __iee_code _iee_set_key_index_key(unsigned long __unused, struct key *key, struct keyring_index_key *index_key);
void __iee_code _iee_set_key_hash(unsigned long __unused, struct key *key, unsigned long hash);
void __iee_code _iee_set_key_len_desc(unsigned long __unused, struct key *key, unsigned long len_desc);
void __iee_code _iee_set_key_type(unsigned long __unused, struct key *key, struct key_type *type);
void __iee_code _iee_set_key_domain_tag(unsigned long __unused, struct key *key, struct key_tag *domain_tag);
void __iee_code _iee_set_key_description(unsigned long __unused, struct key *key, char *description);
void __iee_code _iee_set_key_restrict_link(unsigned long __unused, struct key *key,
									struct key_restriction *restrict_link);
unsigned long __iee_code _iee_set_key_flag_bit(unsigned long __unused, struct key *key, long nr, int flag);
#endif
#endif
