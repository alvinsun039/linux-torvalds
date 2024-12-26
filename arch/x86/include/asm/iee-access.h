/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_ACCESS_H
#define _LINUX_IEE_ACCESS_H

#include <asm/iee-def.h>

extern unsigned long long iee_rw_gate(int flag, ...);

static inline void iee_memcpy(void *dst, const void *src, size_t n)
{
	iee_rw_gate(IEE_OP_MEMCPY, dst, src, n);
}

static inline void iee_memset(void *ptr, int data, size_t n)
{
	iee_rw_gate(IEE_OP_MEMSET, ptr, data, n);
}

static inline void iee_set_freeptr(void **pptr, void *ptr)
{
	iee_rw_gate(IEE_OP_SET_FREEPTR, pptr, ptr);
}

static inline void iee_split_huge_pmd(pmd_t *pmdp, pte_t *pgtable)
{
	iee_rw_gate(IEE_OP_SPLIT_HUGE_PMD, pmdp, pgtable);
}

static inline unsigned long iee_test_and_clear_bit(long nr, volatile unsigned long *addr)
{
	return iee_rw_gate(IEE_OP_TEST_CLEAR_BIT, nr, addr);
}

#endif
