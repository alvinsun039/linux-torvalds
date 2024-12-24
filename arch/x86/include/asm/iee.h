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
	pte_t *token_page_ptep, void *token, void *token_page, unsigned long order, int use_block_pmd);
void __iee_code _iee_set_token(unsigned long __unused,
	pte_t *token_ptep, pte_t *new_ptep, void *new, unsigned long order, int use_block_pmd);
unsigned long __iee_code _iee_test_and_clear_bit(unsigned long __unused,
	long nr, volatile unsigned long *addr);
void __iee_code _iee_set_sensitive_pte(unsigned long __unused,
	pte_t *lm_ptep, int order, int use_block_pmd);
void __iee_code _iee_unset_sensitive_pte(unsigned long __unused,
	pte_t *lm_ptep, int order, int use_block_pmd);
#endif
