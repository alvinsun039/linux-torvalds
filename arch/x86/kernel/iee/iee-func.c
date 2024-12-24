// SPDX-License-Identifier: GPL-2.0
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <asm/tlb.h>
#include <asm/tlbflush.h>
#include <asm/pgalloc.h>
#include <linux/iee-func.h>
#include <asm/iee-access.h>
#include <asm/pgalloc.h>
#include <asm/set_memory.h>

static inline void iee_set_sensitive_pte(pte_t *lm_ptep, int order, int use_block_pmd)
{
	int i;

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

static inline void iee_unset_sensitive_pte(pte_t *lm_ptep, int order, int use_block_pmd)
{
	int i;

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

static void do_split_huge_pmd(pmd_t *pmdp)
{
	pte_t *pgtable = pte_alloc_one_kernel(&init_mm);
	int i;
	struct page *page = pmd_page(*pmdp);
	pte_t *ptep = (pte_t *)((unsigned long)pgtable);

	for (i = 0; i < PMD_SIZE / PAGE_SIZE; i++, ptep++) {
		pte_t entry;
		pgprot_t pgprot = pmd_pgprot(*pmdp);

		entry = mk_pte(page + i, pgprot);
		WRITE_ONCE(*ptep, entry);
	}
	spinlock_t *ptl = pmd_lock(&init_mm, pmdp);

	if (pmd_leaf(READ_ONCE(*pmdp))) {
		/* avoid two users to split the same huge pmd */
		smp_wmb();
		pmd_populate_kernel(&init_mm, pmdp, pgtable);
		pgtable = NULL;
	}
	spin_unlock(ptl);
	if (pgtable)
		pte_free_kernel(&init_mm, pgtable);
}

// Input is the lm vaddr of sensitive data.
void set_iee_page(unsigned long addr, int order)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp = pud_offset(p4dp, addr);
	pmd_t *pmdp = pmd_offset(pudp, addr);
	int use_block_pmd = 0;

	if (pmd_leaf(*pmdp) && order < 9)
		do_split_huge_pmd(pmdp);
	else if (pmd_leaf(*pmdp))
		use_block_pmd = 1;

	pte_t *lm_ptep;

	if (use_block_pmd)
		lm_ptep = (pte_t *)pmdp;
	else
		lm_ptep = pte_offset_kernel(pmdp, addr);

	iee_set_sensitive_pte(lm_ptep, order, use_block_pmd);
	flush_tlb_kernel_range(addr, addr+PAGE_SIZE*(1 << order));
}

// Input is the lm vaddr of sensitive data.
void unset_iee_page(unsigned long addr, int order)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp = pud_offset(p4dp, addr);
	pmd_t *pmdp = pmd_offset(pudp, addr);
	pte_t *lm_ptep;
	int use_block_pmd = 0;
	// Use Block Descriptor.
	if (pmd_leaf(*pmdp)) {
		use_block_pmd = 1;
		lm_ptep = (pte_t *)pmdp;
	} else
		lm_ptep = pte_offset_kernel(pmdp, addr);

	iee_unset_sensitive_pte(lm_ptep, order, use_block_pmd);
	flush_tlb_kernel_range(addr, addr+PAGE_SIZE*(1 << order));
}

void iee_set_logical_mem_ro(unsigned long addr)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp = pud_offset(p4dp, addr);
	pmd_t *pmdp = pmd_offset(pudp, addr);

	if (pmd_leaf(*pmdp))
		do_split_huge_pmd(pmdp);

	pte_t *ptep = pte_offset_kernel(pmdp, addr);
	pte_t pte = READ_ONCE(*ptep);

	pte = __pte((pte_val(pte) & (~__RW) & (~___D)));
	set_pte(ptep, pte);
	flush_tlb_kernel_range(addr, addr+PAGE_SIZE);
}

void set_iee_page_valid(unsigned long addr) {}

void set_iee_page_invalid(unsigned long addr) {}

void iee_set_token_page_valid(void *token, void *token_page, unsigned int order)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, (unsigned long)token);
	p4d_t *p4dp = p4d_offset(pgdp, (unsigned long)token);
	pud_t *pudp = pud_offset(p4dp, (unsigned long)token);
	pmd_t *token_pmdp = pmd_offset(pudp, (unsigned long)token);
	pte_t *token_ptep;

	pgdp = pgd_offset_pgd(pgdir, (unsigned long)token_page);
	p4dp = p4d_offset(pgdp, (unsigned long)token_page);
	pudp = pud_offset(p4dp, (unsigned long)token_page);
	pmd_t *token_page_pmdp = pmd_offset(pudp, (unsigned long)token_page);
	pte_t *token_page_ptep;
	int use_block_pmd = 0;

	if (pmd_leaf(*token_pmdp) && order < 9) {
		do_split_huge_pmd(token_pmdp);
		do_split_huge_pmd(token_page_pmdp);
	} else if (pmd_leaf(*token_pmdp)) {
		use_block_pmd = 1;
	}

	if (use_block_pmd) {
		token_ptep = (pte_t *)token_pmdp;
		token_page_ptep = (pte_t *)token_page_pmdp;
	} else {
		token_ptep = pte_offset_kernel(token_pmdp, (unsigned long)token);
		token_page_ptep = pte_offset_kernel(token_page_pmdp, (unsigned long)token_page);
	}

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

			pte = __pte(((pte_val(pte) & ~PTE_PFN_MASK))
				| (__phys_to_pfn(__pa(token_page) + i * PAGE_SIZE) << PAGE_SHIFT));
			WRITE_ONCE(*token_ptep, pte);
			pte = READ_ONCE(*token_page_ptep);
			pte = __pte((pte_val(pte) & ~__RW) & ~___D);
			WRITE_ONCE(*token_page_ptep, pte);
			token_ptep++;
			token_page_ptep++;
		}
	}
	flush_tlb_kernel_range((unsigned long)token, (unsigned long)(token + (PAGE_SIZE * (1 << order))));
	flush_tlb_kernel_range((unsigned long)token_page, (unsigned long)(token_page + (PAGE_SIZE * (1 << order))));
}

void iee_set_token_page_invalid(void *token, void *__unused, unsigned long order)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, (unsigned long)token);
	p4d_t *p4dp = p4d_offset(pgdp, (unsigned long)token);
	pud_t *pudp = pud_offset(p4dp, (unsigned long)token);
	pmd_t *token_pmdp = pmd_offset(pudp, (unsigned long)token);
	pte_t *token_ptep;
	void *token_page;
	int use_block_pmd = 0;

	if (pmd_leaf(*token_pmdp)) {
		use_block_pmd = 1;
		token_ptep = (pte_t *)token_pmdp;
		token_page = page_address(pmd_page(*token_pmdp));
	} else {
		token_ptep = pte_offset_kernel(token_pmdp, (unsigned long)token);
		token_page = page_address(pte_page(*token_ptep));
	}
	pgdp = pgd_offset_pgd(pgdir, (unsigned long)token_page);
	p4dp = p4d_offset(pgdp, (unsigned long)token_page);
	pudp = pud_offset(p4dp, (unsigned long)token_page);
	pmd_t *token_page_pmdp = pmd_offset(pudp, (unsigned long)token_page);
	pte_t *token_page_ptep;

	if (use_block_pmd)
		token_page_ptep = (pte_t *)token_page_pmdp;
	else
		token_page_ptep = pte_offset_kernel(token_page_pmdp, (unsigned long)token);

	if (use_block_pmd) {
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
	free_pages((unsigned long)token_page, order);
	flush_tlb_kernel_range((unsigned long)token, (unsigned long)(token + (PAGE_SIZE * (1 << order))));
	flush_tlb_kernel_range((unsigned long)token_page, (unsigned long)(token_page + (PAGE_SIZE * (1 << order))));
}

void __init iee_set_kernel_upage(unsigned long addr)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	pgd_t pgd = READ_ONCE(*pgdp);

	pgd = __pgd((pgd_val(pgd) | _USR) & ~___G);
	set_pgd(pgdp, pgd);

	p4d_t *p4dp = p4d_offset(pgdp, addr);
	p4d_t p4d = READ_ONCE(*p4dp);

	p4d = __p4d((p4d_val(p4d) | _USR) & ~___G);
	set_p4d(p4dp, p4d);

	pud_t *pudp = pud_offset(p4dp, addr);

	if (pud_leaf(*pudp))
		panic("Huge pud page set upage!\n");

	pud_t pud = READ_ONCE(*pudp);

	pud = __pud((pud_val(pud) | _USR) & ~___G);
	set_pud(pudp, pud);

	pmd_t *pmdp = pmd_offset(pudp, addr);

	if (pmd_leaf(*pmdp))
		do_split_huge_pmd(pmdp);

	pmd_t pmd = READ_ONCE(*pmdp);

	pmd = __pmd((pmd_val(pmd) | _USR) & ~___G);
	set_pmd(pmdp, pmd);

	pte_t *ptep = pte_offset_kernel(pmdp, addr);
	pte_t pte = READ_ONCE(*ptep);

	pte = __pte((pte_val(pte) | _USR) & ~___G);
	set_pte(ptep, pte);
	flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
}

void set_iee_stack_page(unsigned long addr, int order)
{
	set_iee_page(addr, order);
}

void unset_iee_stack_page(unsigned long addr, int order)
{
	unset_iee_page(addr, order);
}

void __init iee_rest_init(void)
{
	// Prepare data for iee rwx gate
	unsigned long addr;
	/* Map .iee.text as U RWX pages */
	addr = (unsigned long)__iee_si_text_start;
	for (; addr < (unsigned long)__iee_si_text_end; addr += PAGE_SIZE) {
		iee_set_kernel_upage((unsigned long)addr);
		iee_set_kernel_upage((unsigned long)__va(__pa(addr)));
	}
	iee_init_done = true;
	/* Map .iee.data as RO pages */
	set_memory_ro((unsigned long)__iee_si_data_start,
	((unsigned long)__iee_si_data_end - (unsigned long)__iee_si_data_start) / PAGE_SIZE);
	// All initialization is done. Do some simple tests.
	pr_err("IEE: testing iee_exec_entry si_test...");
	iee_rwx_gate(IEE_SI_TEST);
	pr_err("IEE: testing iee_exec_entry si_test...");
}
