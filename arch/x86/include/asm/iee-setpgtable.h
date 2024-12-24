/* SPDX-License-Identifier: GPL-2.0 */
#include <asm/pgtable_types.h>
extern void iee_set_pte(int flag, pte_t *ptep, pte_t pte);
extern void iee_set_pmd(int flag, pmd_t *pmdp, pmd_t pmd);
extern void iee_set_pud(int flag, pud_t *pudp, pud_t pud);
extern void iee_set_p4d(int flag, p4d_t *p4dp, p4d_t p4d);
extern void iee_set_pgd(int flag, pgd_t *pgdp, pgd_t pgd);
