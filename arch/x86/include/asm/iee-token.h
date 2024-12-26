/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_TOKEN_H
#define _LINUX_IEE_TOKEN_H

#include <asm/iee-def.h>
#include <asm/pgtable_types.h>

extern unsigned long long iee_rw_gate(int flag, ...);
struct task_token;
struct task_struct;
struct mm_struct;

static inline void iee_set_token_pgd(struct task_struct *tsk, pgd_t *pgd)
{
	iee_rw_gate(IEE_OP_SET_TOKEN_PGD, tsk, pgd);
}

static inline void iee_init_token(struct task_struct *tsk, void *iee_stack, void *tmp_page)
{
	iee_rw_gate(IEE_OP_INIT_TOKEN, tsk, iee_stack, tmp_page);
}

static inline void iee_invalidate_token(struct task_struct *tsk)
{
	iee_rw_gate(IEE_OP_INVALIDATE_TOKEN, tsk);
}

static inline void iee_validate_token(struct task_struct *tsk)
{
	iee_rw_gate(IEE_OP_VALIDATE_TOKEN, tsk);
}

#endif
