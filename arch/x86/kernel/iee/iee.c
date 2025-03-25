// SPDX-License-Identifier: GPL-2.0
#include <asm/pgtable.h>
#include <linux/stdarg.h>
#include <asm/iee.h>
#include <asm/page.h>
#include <asm/pgtable_types.h>
#include <linux/sched.h>
#include <linux/memory.h>
#include <linux/pgtable.h>
#include <linux/cred.h>
#include <linux/key.h>
#include <asm/percpu.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/mm.h>
#include <asm/iee-si.h>
#include <asm/desc_defs.h>
#include <asm/pgtable_areas.h>
#include <linux/hugetlb.h>
#include <asm/iee-def.h>
#ifdef CONFIG_IEE_SELINUX_P
#include <asm/iee-selinuxp.h>
#endif

bool __iee_si_data iee_si_enabled = false;

typedef void (*iee_func)(void);
iee_func iee_funcs[] = {
	(iee_func)_iee_memcpy,
	(iee_func)_iee_memset,
	(iee_func)_iee_set_freeptr,
	(iee_func)_iee_split_huge_pmd,
	(iee_func)_iee_set_token_pgd,
	(iee_func)_iee_init_token,
	(iee_func)_iee_invalidate_token,
	(iee_func)_iee_validate_token,
	(iee_func)_iee_unset_token,
	(iee_func)_iee_set_token,
	(iee_func)_iee_test_and_clear_bit,
	(iee_func)_iee_set_sensitive_pte,
	(iee_func)_iee_unset_sensitive_pte,
#ifdef CONFIG_PTP
	(iee_func)_iee_set_pte,
	(iee_func)_iee_set_pmd,
	(iee_func)_iee_set_pud,
	(iee_func)_iee_set_p4d,
	(iee_func)_iee_set_pgd,
#endif
#ifdef CONFIG_IEE_SELINUX_P
	(iee_func)_iee_set_selinux_status_pg,
	(iee_func)_iee_set_selinux_enforcing,
	(iee_func)_iee_mark_selinux_initialized,
	(iee_func)_iee_set_sel_policy_cap,
	(iee_func)_iee_sel_rcu_assign_policy,
#endif
#ifdef CONFIG_CREDP
	(iee_func)_iee_copy_cred,
	(iee_func)_iee_set_cred_uid,
	(iee_func)_iee_set_cred_gid,
	(iee_func)_iee_set_cred_suid,
	(iee_func)_iee_set_cred_sgid,
	(iee_func)_iee_set_cred_euid,
	(iee_func)_iee_set_cred_egid,
	(iee_func)_iee_set_cred_fsuid,
	(iee_func)_iee_set_cred_fsgid,
	(iee_func)_iee_set_cred_user,
	(iee_func)_iee_set_cred_user_ns,
	(iee_func)_iee_set_cred_group_info,
	(iee_func)_iee_set_cred_securebits,
	(iee_func)_iee_set_cred_cap_inheritable,
	(iee_func)_iee_set_cred_cap_permitted,
	(iee_func)_iee_set_cred_cap_effective,
	(iee_func)_iee_set_cred_cap_bset,
	(iee_func)_iee_set_cred_cap_ambient,
	(iee_func)_iee_set_cred_jit_keyring,
	(iee_func)_iee_set_cred_session_keyring,
	(iee_func)_iee_set_cred_process_keyring,
	(iee_func)_iee_set_cred_thread_keyring,
	(iee_func)_iee_set_cred_request_key_auth,
	(iee_func)_iee_set_cred_non_rcu,
	(iee_func)_iee_set_cred_atomic_set_usage,
	(iee_func)_iee_set_cred_atomic_op_usage,
	(iee_func)_iee_set_cred_security,
	(iee_func)_iee_set_cred_rcu,
	(iee_func)_iee_set_cred_ucounts,
#endif
#ifdef CONFIG_KEYP
	(iee_func)_iee_set_key_union,
	(iee_func)_iee_set_key_struct,
	(iee_func)_iee_set_key_payload,
	(iee_func)_iee_set_key_usage,
	(iee_func)_iee_set_key_serial,
	(iee_func)_iee_set_key_watchers,
	(iee_func)_iee_set_key_user,
	(iee_func)_iee_set_key_security,
	(iee_func)_iee_set_key_expiry,
	(iee_func)_iee_set_key_revoked_at,
	(iee_func)_iee_set_key_last_used_at,
	(iee_func)_iee_set_key_uid,
	(iee_func)_iee_set_key_gid,
	(iee_func)_iee_set_key_perm,
	(iee_func)_iee_set_key_quotalen,
	(iee_func)_iee_set_key_datalen,
	(iee_func)_iee_set_key_state,
	(iee_func)_iee_set_key_magic,
	(iee_func)_iee_set_key_flags,
	(iee_func)_iee_set_key_index_key,
	(iee_func)_iee_set_key_hash,
	(iee_func)_iee_set_key_len_desc,
	(iee_func)_iee_set_key_type,
	(iee_func)_iee_set_key_domain_tag,
	(iee_func)_iee_set_key_description,
	(iee_func)_iee_set_key_restrict_link,
	(iee_func)_iee_set_key_flag_bit,
#endif
	NULL
};

void __iee_code _iee_memcpy(unsigned long __unused, void *dst, void *src, size_t n)
{
	char *_dst, *_src;

	_dst = (char *)(__phys_to_iee(__pa(dst)));
	_src = (char *)src;

	while (n--)
		*_dst++ = *_src++;
}

void __iee_code _iee_memset(unsigned long __unused, void *ptr, int data, size_t n)
{
	char *_ptr;

	_ptr = (char *)(__phys_to_iee(__pa(ptr)));

	while (n--)
		*_ptr++ = data;
}

void __iee_code _iee_set_freeptr(unsigned long __unused, void **pptr, void *ptr)
{
	pptr = (void **)(__phys_to_iee(__pa(pptr)));
	*pptr = ptr;
}

void __iee_code _iee_split_huge_pmd(unsigned long __unused, pmd_t *pmdp, pte_t *pgtable)
{
	int i;
	struct page *page = pmd_page(*pmdp);
	pte_t *ptep = (pte_t *)(__phys_to_iee(__pa(pgtable)));

	for (i = 0; i < PMD_SIZE / PAGE_SIZE; i++, ptep++) {
		pte_t entry;
		pgprot_t pgprot = pmd_pgprot(*pmdp);

		entry = mk_pte(page + i, pgprot);
		WRITE_ONCE(*ptep, entry);
	}
}

void __iee_code _iee_set_token_pgd(unsigned long __unused, struct task_struct *tsk, pgd_t *pgd)
{
	struct task_token *token;

	token = (struct task_token *)(__phys_to_iee(__pa(tsk)));
	token->pgd = pgd;
}

void __iee_code _iee_init_token(unsigned long __unused, struct task_struct *tsk, void *iee_stack, void *tmp_page)
{
	struct task_token *token;

	token = (struct task_token *)(__phys_to_iee(__pa(tsk)));
	token->iee_stack = iee_stack;
	token->tmp_page = tmp_page;
}

void __iee_code _iee_invalidate_token(unsigned long __unused, struct task_struct *tsk)
{
	struct task_token *token = (struct task_token *)(__phys_to_iee(__pa(tsk)));

	token->pgd = NULL;
	token->valid = false;
	token->kernel_stack = NULL;
}

void __iee_code _iee_validate_token(unsigned long __unused, struct task_struct *tsk)
{
	struct task_token *token = (struct task_token *)(__phys_to_iee(__pa(tsk)));

	token->valid = true;
}

void __iee_code _iee_unset_token(unsigned long __unused,
	pte_t *token_ptep, pte_t *token_page_ptep,
	void *token, void *token_page, unsigned long order)
{
	token_ptep = (pte_t *)(__phys_to_iee(__pa(token_ptep)));
	token_page_ptep = (pte_t *)(__phys_to_iee(__pa(token_page_ptep)));
	if (order == 0) {
		pmd_t *pmdp = (pmd_t *)token_page_ptep;
		pmd_t pmd = READ_ONCE(*pmdp);

		pmd = __pmd(pmd_val(pmd) | ___D | __RW);
		WRITE_ONCE(*pmdp, pmd);
		pmdp = (pmd_t *)token_ptep;
		pmd = READ_ONCE(*pmdp);
		pmd = __pmd((pmd_val(pmd) & ~PTE_PFN_MASK)
			| (__phys_to_pfn(__iee_pa(token)) << PAGE_SHIFT));
		WRITE_ONCE(*pmdp, pmd);
	} else {
		for (int i = 0; i < (0x1 << order); i++) {
			pte_t pte = READ_ONCE(*token_ptep);

			pte = __pte((pte_val(pte) & ~PTE_PFN_MASK)
				| (__phys_to_pfn(__iee_pa(token) + i * PAGE_SIZE) << PAGE_SHIFT));

			WRITE_ONCE(*token_ptep, pte);
			pte = READ_ONCE(*token_page_ptep);
			pte = __pte(pte_val(pte) | ___D | __RW);
			WRITE_ONCE(*token_page_ptep, pte);
			token_ptep++;
			token_page_ptep++;
		}
	}
}

void __iee_code _iee_set_token(unsigned long __unused,
	pte_t *token_ptep, pte_t *token_page_ptep, void *token_page, unsigned long order, int use_block_pmd)
{
	token_ptep = (pte_t *)(__phys_to_iee(__pa(token_ptep)));
	token_page_ptep = (pte_t *)(__phys_to_iee(__pa(token_page_ptep)));
	if (use_block_pmd) {
		pmd_t *pmdp = (pmd_t *)token_page_ptep;
		pmd_t pmd = READ_ONCE(*pmdp);

		pmd = __pmd((pmd_val(pmd) & ~__RW) & ~___D);
		WRITE_ONCE(*pmdp, pmd);
		pmdp = (pmd_t *)token_ptep;
		pmd = READ_ONCE(*pmdp);
		pmd = __pmd(((pmd_val(pmd) & ~PTE_PFN_MASK))
			| (__phys_to_pfn(__pa(token_page)) << PAGE_SHIFT));
		WRITE_ONCE(*pmdp, pmd);
	} else {
		for (int i = 0; i < (0x1 << order); i++) {
			pte_t pte = READ_ONCE(*token_ptep);

			pte = __pte(((pte_val(pte) & ~PTE_PFN_MASK)) |
				(__phys_to_pfn(__pa(token_page) + i * PAGE_SIZE) << PAGE_SHIFT));
			WRITE_ONCE(*token_ptep, pte);
			pte = READ_ONCE(*token_page_ptep);
			pte = __pte((pte_val(pte) & ~__RW) & ~___D);
			WRITE_ONCE(*token_page_ptep, pte);
			token_ptep++;
			token_page_ptep++;
		}
	}
}

unsigned long __iee_code _iee_test_and_clear_bit(unsigned long __unused, long nr, volatile unsigned long *addr)
{
	unsigned long *iee_addr = (unsigned long *)__phys_to_iee(__pa(addr));

	kcsan_mb();
	instrument_atomic_read_write(iee_addr + BIT_WORD(nr), sizeof(long));
	return arch_test_and_clear_bit(nr, iee_addr);
}

void __iee_code _iee_set_sensitive_pte(unsigned long __unused, pte_t *lm_ptep, int order, int use_block_pmd)
{
	int i;

	lm_ptep = (pte_t *)(__phys_to_iee(__pa(lm_ptep)));
	if (use_block_pmd) {
		pmd_t pmd = __pmd(pte_val(READ_ONCE(*lm_ptep)));

		pmd = __pmd((pmd_val(pmd) & (~__RW) & (~___D)));
		WRITE_ONCE(*lm_ptep, __pte(pmd_val(pmd)));
	} else {
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = READ_ONCE(*lm_ptep);

			pte = __pte((pte_val(pte) & (~__RW) & (~___D)));
			WRITE_ONCE(*lm_ptep, pte);
			lm_ptep++;
		}
	}
}

void __iee_code _iee_unset_sensitive_pte(unsigned long __unused, pte_t *lm_ptep, int order, int use_block_pmd)
{
	int i;

	lm_ptep = (pte_t *)(__phys_to_iee(__pa(lm_ptep)));
	if (use_block_pmd) {
		pmd_t pmd = __pmd(pte_val(READ_ONCE(*lm_ptep)));

		pmd = __pmd((pmd_val(pmd) | __RW | ___D));
		WRITE_ONCE(*lm_ptep, __pte(pmd_val(pmd)));
	} else {
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = READ_ONCE(*lm_ptep);

			pte = __pte(pte_val(pte) | __RW | ___D);
			WRITE_ONCE(*lm_ptep, pte);
			lm_ptep++;
		}
	}
}

#ifdef CONFIG_PTP
void __iee_code _iee_set_pte(unsigned long __unused, pte_t *ptep, pte_t pte)
{
	WRITE_ONCE(*(pte_t *)(__phys_to_iee(__pa(ptep))), pte);
}

void __iee_code _iee_set_pmd(unsigned long __unused, pmd_t *pmdp, pmd_t pmd)
{
	WRITE_ONCE(*(pmd_t *)(__phys_to_iee(__pa(pmdp))), pmd);
}

void __iee_code _iee_set_pud(unsigned long __unused, pud_t *pudp, pud_t pud)
{
	WRITE_ONCE(*(pud_t *)(__phys_to_iee(__pa(pudp))), pud);
}

void __iee_code _iee_set_p4d(unsigned long __unused, p4d_t *p4dp, p4d_t p4d)
{
	WRITE_ONCE(*(p4d_t *)(__phys_to_iee(__pa(p4dp))), p4d);
}

void __iee_code _iee_set_pgd(unsigned long __unused, pgd_t *pgdp, pgd_t pgd)
{
	WRITE_ONCE(*(pgd_t *)(__phys_to_iee(__pa(pgdp))), pgd);
}
#endif

#ifdef CONFIG_IEE_SELINUX_P
void __iee_code _iee_set_selinux_status_pg(unsigned long __unused, struct page *new_page)
{
	struct page **iee_addr = (struct page **)__phys_to_iee(__pa_symbol(&(selinux_state.status_page)));
	*iee_addr = new_page;
}

void __iee_code _iee_set_selinux_enforcing(unsigned long __unused, bool value)
{
	*(bool *)__phys_to_iee(__pa_symbol(&(selinux_state.enforcing))) = value;
}

void __iee_code _iee_mark_selinux_initialized(unsigned long __unused)
{
	/* synchronize selinux status*/
	smp_store_release(((bool *)__phys_to_iee(__pa_symbol(&(selinux_state.initialized)))), true);
}

void __iee_code _iee_set_sel_policy_cap(unsigned long __unused, unsigned int idx, int cap)
{
	*(bool *)__phys_to_iee(__pa_symbol(&(selinux_state.policycap[idx]))) = cap;
}

/*
 * Please make sure param iee_new_policy is from policy_jar memcache.
 * Need to free new_policy after calling this func as it's only used to
 * trans data from kernel.
 */
void __iee_code _iee_sel_rcu_assign_policy(unsigned long __unused, struct selinux_policy *new_policy,
					struct selinux_policy *iee_new_policy)
{
	struct selinux_policy *iee_addr = (struct selinux_policy *)(__phys_to_iee(__pa(iee_new_policy)));

	memcpy(iee_addr, new_policy, sizeof(struct selinux_policy));
	rcu_assign_pointer(*((struct selinux_policy **)__phys_to_iee(__pa_symbol(&(selinux_state.policy)))),
			iee_new_policy);
}
#endif

#ifdef CONFIG_CREDP
static struct cred *iee_cred(unsigned long __unused, struct cred *cred)
{
	if (cred == &init_cred)
		cred = (struct cred *)__phys_to_iee(__pa_symbol(cred));
	else
		cred = (struct cred *)(__phys_to_iee(__pa(cred)));
	return cred;
}

void __iee_code _iee_set_cred_rcu(unsigned long __unused, struct cred *cred, struct rcu_head *rcu)
{
	cred = iee_cred(__unused, cred);
	*((struct rcu_head **)(&(cred->rcu.func))) = rcu;
}

void __iee_code _iee_set_cred_security(unsigned long __unused, struct cred *cred, void *security)
{
	cred = iee_cred(__unused, cred);
	cred->security = security;
}

unsigned long __iee_code _iee_set_cred_atomic_op_usage(unsigned long __unused, struct cred *cred, int flag, int nr)
{
	cred = iee_cred(__unused, cred);
	switch (flag) {
	case AT_ADD: {
		atomic_long_add(nr, &cred->usage);
		return 0;
	}
	case AT_INC_NOT_ZERO: {
		return atomic_long_inc_not_zero(&cred->usage);
	}
	case AT_SUB_AND_TEST: {
		return atomic_long_sub_and_test(nr, &cred->usage);
	}
	}
	return 0;
}

void __iee_code _iee_set_cred_atomic_set_usage(unsigned long __unused, struct cred *cred, int i)
{
	cred = iee_cred(__unused, cred);
	atomic_long_set(&cred->usage, i);
}

void __iee_code _iee_set_cred_non_rcu(unsigned long __unused, struct cred *cred, int non_rcu)
{
	cred = iee_cred(__unused, cred);
	cred->non_rcu = non_rcu;
}

void __iee_code _iee_set_cred_session_keyring(unsigned long __unused, struct cred *cred, struct key *session_keyring)
{
	cred = iee_cred(__unused, cred);
	cred->session_keyring = session_keyring;
}

void __iee_code _iee_set_cred_process_keyring(unsigned long __unused, struct cred *cred, struct key *process_keyring)
{
	cred = iee_cred(__unused, cred);
	cred->process_keyring = process_keyring;
}

void __iee_code _iee_set_cred_thread_keyring(unsigned long __unused, struct cred *cred, struct key *thread_keyring)
{
	cred = iee_cred(__unused, cred);
	cred->thread_keyring = thread_keyring;
}

void __iee_code _iee_set_cred_request_key_auth(unsigned long __unused, struct cred *cred, struct key *request_key_auth)
{
	cred = iee_cred(__unused, cred);
	cred->request_key_auth = request_key_auth;
}

void __iee_code _iee_set_cred_jit_keyring(unsigned long __unused, struct cred *cred, unsigned char jit_keyring)
{
	cred = iee_cred(__unused, cred);
	cred->jit_keyring = jit_keyring;
}

void __iee_code _iee_set_cred_cap_inheritable(unsigned long __unused, struct cred *cred, kernel_cap_t cap_inheritable)
{
	cred = iee_cred(__unused, cred);
	cred->cap_inheritable = cap_inheritable;
}

void __iee_code _iee_set_cred_cap_permitted(unsigned long __unused, struct cred *cred, kernel_cap_t cap_permitted)
{
	cred = iee_cred(__unused, cred);
	cred->cap_permitted = cap_permitted;
}

void __iee_code _iee_set_cred_cap_effective(unsigned long __unused, struct cred *cred, kernel_cap_t cap_effective)
{
	cred = iee_cred(__unused, cred);
	cred->cap_effective = cap_effective;
}

void __iee_code _iee_set_cred_cap_bset(unsigned long __unused, struct cred *cred, kernel_cap_t cap_bset)
{
	cred = iee_cred(__unused, cred);
	cred->cap_bset = cap_bset;
}

void __iee_code _iee_set_cred_cap_ambient(unsigned long __unused, struct cred *cred, kernel_cap_t cap_ambient)
{
	cred = iee_cred(__unused, cred);
	cred->cap_ambient = cap_ambient;
}

void __iee_code _iee_set_cred_securebits(unsigned long __unused, struct cred *cred, unsigned int securebits)
{
	cred = iee_cred(__unused, cred);
	cred->securebits = securebits;
}

void __iee_code _iee_set_cred_group_info(unsigned long __unused, struct cred *cred, struct group_info *group_info)
{
	cred = iee_cred(__unused, cred);
	cred->group_info = group_info;
}

void __iee_code _iee_set_cred_ucounts(unsigned long __unused, struct cred *cred, struct ucounts *ucounts)
{
	cred = iee_cred(__unused, cred);
	cred->ucounts = ucounts;
}

void __iee_code _iee_set_cred_user_ns(unsigned long __unused, struct cred *cred, struct user_namespace *user_ns)
{
	cred = iee_cred(__unused, cred);
	cred->user_ns = user_ns;
}

void __iee_code _iee_set_cred_user(unsigned long __unused, struct cred *cred, struct user_struct *user)
{
	cred = iee_cred(__unused, cred);
	cred->user = user;
}

void __iee_code _iee_set_cred_fsgid(unsigned long __unused, struct cred *cred, kgid_t fsgid)
{
	cred = iee_cred(__unused, cred);
	cred->fsgid = fsgid;
}

void __iee_code _iee_set_cred_fsuid(unsigned long __unused, struct cred *cred, kuid_t fsuid)
{
	cred = iee_cred(__unused, cred);
	cred->fsuid = fsuid;
}

void __iee_code _iee_set_cred_egid(unsigned long __unused, struct cred *cred, kgid_t egid)
{
	cred = iee_cred(__unused, cred);
	cred->egid = egid;
}

void __iee_code _iee_set_cred_euid(unsigned long __unused, struct cred *cred, kuid_t euid)
{
	cred = iee_cred(__unused, cred);
	cred->euid = euid;
}

void __iee_code _iee_set_cred_sgid(unsigned long __unused, struct cred *cred, kgid_t sgid)
{
	cred = iee_cred(__unused, cred);
	cred->sgid = sgid;
}

void __iee_code _iee_set_cred_suid(unsigned long __unused, struct cred *cred, kuid_t suid)
{
	cred = iee_cred(__unused, cred);
	cred->suid = suid;
}

void __iee_code _iee_copy_cred(unsigned long __unused, struct cred *old, struct cred *new)
{
	if (new == &init_cred)
		panic("copy_cred for init_cred: %lx\n", (unsigned long)new);

	struct rcu_head *rcu = (struct rcu_head *)(new->rcu.func);
	struct cred *_new = (struct cred *)__phys_to_iee(__pa(new));

	_iee_memcpy(__unused, new, old, sizeof(struct cred));
	*(struct rcu_head **)(&(_new->rcu.func)) = rcu;
	*(struct rcu_head *)(_new->rcu.func) = *(struct rcu_head *)(old->rcu.func);
}

void __iee_code _iee_set_cred_gid(unsigned long __unused, struct cred *cred, kgid_t gid)
{
	cred = iee_cred(__unused, cred);
	cred->gid = gid;
}

void __iee_code _iee_set_cred_uid(unsigned long __unused, struct cred *cred, kuid_t uid)
{
	cred = iee_cred(__unused, cred);
	cred->uid = uid;
}
#endif

#ifdef CONFIG_KEYP
unsigned long __iee_code _iee_set_key_flag_bit(unsigned long __unused, struct key *key,
				      long nr, int flag)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	switch (flag) {
	case SET_BIT_OP: {
		set_bit(nr, &key->flags);
		break;
	}
	case TEST_AND_CLEAR_BIT: {
		return test_and_clear_bit(nr, &key->flags);
	}
	case TEST_AND_SET_BIT: {
		return test_and_set_bit(nr, &key->flags);
	}
	}
	return 0;
}

void __iee_code _iee_set_key_restrict_link(unsigned long __unused,
					   struct key *key,
					   struct key_restriction *restrict_link)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->restrict_link = restrict_link;
}

void __iee_code _iee_set_key_magic(unsigned long __unused, struct key *key,
				   unsigned int magic)
{
#ifdef KEY_DEBUGGING
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->magic = magic;
#endif
}

void __iee_code _iee_set_key_flags(unsigned long __unused, struct key *key,
				   unsigned long flags)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->flags = flags;
}

void __iee_code _iee_set_key_index_key(unsigned long __unused,
					   struct key *key,
					   struct keyring_index_key *index_key)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->index_key = *index_key;
}

void __iee_code _iee_set_key_hash(unsigned long __unused, struct key *key,
				  unsigned long hash)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->hash = hash;
}

void __iee_code _iee_set_key_len_desc(unsigned long __unused, struct key *key,
				      unsigned long len_desc)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->len_desc = len_desc;
}

void __iee_code _iee_set_key_type(unsigned long __unused, struct key *key,
				  struct key_type *type)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->type = type;
}

void __iee_code _iee_set_key_domain_tag(unsigned long __unused,
					struct key *key,
					struct key_tag *domain_tag)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->domain_tag = domain_tag;
}

void __iee_code _iee_set_key_description(unsigned long __unused,
					 struct key *key, char *description)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->description = description;
}

void __iee_code _iee_set_key_uid(unsigned long __unused, struct key *key,
				 kuid_t uid)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->uid = uid;
}

void __iee_code _iee_set_key_gid(unsigned long __unused, struct key *key,
				 kgid_t gid)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->gid = gid;
}

void __iee_code _iee_set_key_perm(unsigned long __unused, struct key *key,
				  key_perm_t perm)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->perm = perm;
}

void __iee_code _iee_set_key_quotalen(unsigned long __unused, struct key *key,
				      unsigned short quotalen)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->quotalen = quotalen;
}

void __iee_code _iee_set_key_datalen(unsigned long __unused, struct key *key,
				     unsigned short datalen)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->datalen = datalen;
}

void __iee_code _iee_set_key_state(unsigned long __unused, struct key *key,
				   short state)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	WRITE_ONCE(key->state, state);
}

void __iee_code _iee_set_key_user(unsigned long __unused, struct key *key,
				  struct key_user *user)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->user = user;
}

void __iee_code _iee_set_key_security(unsigned long __unused, struct key *key,
				      void *security)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->security = security;
}

void __iee_code _iee_set_key_expiry(unsigned long __unused, struct key *key,
				    time64_t expiry)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->expiry = expiry;
}

void __iee_code _iee_set_key_revoked_at(unsigned long __unused,
					struct key *key, time64_t revoked_at)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->revoked_at = revoked_at;
}

void __iee_code _iee_set_key_last_used_at(unsigned long __unused,
					  struct key *key,
					  time64_t last_used_at)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->last_used_at = last_used_at;
}

unsigned long __iee_code _iee_set_key_usage(unsigned long __unused, struct key *key,
				   int n, int flag)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	switch (flag) {
	case REFCOUNT_INC: {
		refcount_inc(&key->usage);
		break;
	}
	case REFCOUNT_SET: {
		refcount_set(&key->usage, n);
		break;
	}
	case REFCOUNT_DEC_AND_TEST: {
		return refcount_dec_and_test(&key->usage);
	}
	case REFCOUNT_INC_NOT_ZERO: {
		return refcount_inc_not_zero(&key->usage);
	}
	}
	return 0;
}

void __iee_code _iee_set_key_serial(unsigned long __unused, struct key *key,
				    key_serial_t serial)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->serial = serial;
}

void __iee_code _iee_set_key_watchers(unsigned long __unused, struct key *key, struct watch_list *watchers)
{
#ifdef CONFIG_KEY_NOTIFICATIONS
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->watchers = watchers;
#endif
}

void __iee_code _iee_set_key_union(unsigned long __unused, struct key *key,
				   struct key_union *key_union)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->graveyard_link.next = (struct list_head *)key_union;
}

void __iee_code _iee_set_key_struct(unsigned long __unused, struct key *key,
				    struct key_struct *key_struct)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->name_link.prev = (struct list_head *)key_struct;
}

void __iee_code _iee_set_key_payload(unsigned long __unused, struct key *key,
				     union key_payload *key_payload)
{
	key = (struct key *)(__phys_to_iee(__pa(key)));
	key->name_link.next = (struct list_head *)key_payload;
}
#endif

/* iee si */
bool iee_pgt_jar_init __iee_si_data;
bool iee_init_done __iee_si_data;

u64 __iee_si_code notrace iee_si_handler(int flag, ...)
{
	va_list pArgs;
	u64 val;

	va_start(pArgs, flag);
	switch (flag) {
	case IEE_SI_TEST:
		break;
	case IEE_WRITE_CR0: {
		val = va_arg(pArgs, u64);
		unsigned long bits_missing = 0;
set_register_cr0:
		asm volatile("mov %0,%%cr0" : "+r"(val) : : "memory");
		if (static_branch_likely(&cr_pinning)) {
			if (unlikely((val & X86_CR0_WP) != X86_CR0_WP)) {
				bits_missing = X86_CR0_WP;
				val |= bits_missing;
				goto set_register_cr0;
			}
			/* Warn after we've set the missing bits. */
			WARN_ONCE(bits_missing, "CR0 WP bit went missing!?\n");
		}
		break;
	}
	case IEE_WRITE_CR3: {
		val = va_arg(pArgs, u64);
		asm volatile("mov %0,%%cr3" : : "r"(val) : "memory");
		break;
	}
	case IEE_WRITE_CR4: {
		val = va_arg(pArgs, u64);
		val &= ~(X86_CR4_SMEP);
		unsigned long bits_changed = 0;
set_register_cr4:
		asm volatile("mov %0,%%cr4" : "+r" (val) : : "memory");
		if (static_branch_likely(&cr_pinning)) {
			if (unlikely((val & cr4_pinned_mask) != cr4_pinned_bits)) {
				bits_changed = (val & cr4_pinned_mask) ^ cr4_pinned_bits;
				val = (val & ~cr4_pinned_mask) | cr4_pinned_bits;
				goto set_register_cr4;
			}
			/* Warn after we've corrected the changed bits. */
			WARN_ONCE(bits_changed, "pinned CR4 bits changed: 0x%lx!?\n",
				bits_changed);
		}
		break;
	}
	case IEE_LOAD_IDT: {
		const struct desc_ptr *new_val = va_arg(pArgs, const struct desc_ptr*);
		unsigned long new_addr = new_val->address;

		asm volatile("lidt %0"::"m" (*new_val));
		break;
	}
	}
	va_end(pArgs);
	return 0;
}
