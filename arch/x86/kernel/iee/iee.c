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
