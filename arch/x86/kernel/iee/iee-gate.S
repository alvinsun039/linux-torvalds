/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/percpu.h>
#include <asm/processor-flags.h>
#include <asm/asm.h>

#define X86_CR4_SMEP_SMAP (X86_CR4_SMEP | X86_CR4_SMAP)

/* scratch_reg would be changed,
caller should dertimine if scratch_reg should be saved and restored */
.macro DISABLE_WP scratch_reg:req
    /* Disable write protection*/
    movq %cr0, %\scratch_reg
    andq $(~X86_CR0_WP), %\scratch_reg
    movq %\scratch_reg, %cr0
.endm

.macro ENABLE_WP scratch_reg:req
    /* Enable write protection */
    movq %cr0, %\scratch_reg
1:
    orq $X86_CR0_WP, %\scratch_reg
    movq %\scratch_reg, %cr0
    testq $X86_CR0_WP, %\scratch_reg
    je 1b
.endm

SYM_FUNC_START(iee_rw_gate)
    /* save Interrupt flag */
    pushf
    /* close irq*/
    cli

    pushq %r12

    DISABLE_WP r12

    /* switch to iee stack */
    movq PER_CPU_VAR(pcpu_hot + X86_current_task), %r12 /* r12 -> task_struct */
    addq iee_offset(%rip), %r12
    movq %rsp, kernel_from_token_offset(%r12)
    movq iee_from_token_offset(%r12), %rsp

    /* call iee func */
    leaq iee_funcs(%rip), %rax
    call *(%rax, %rdi, 8)

    /* switch to kernel stack */
    movq kernel_from_token_offset(%r12), %rsp

    ENABLE_WP r12

    popq %r12

    /* restore irq*/
    popf

    lfence   /* Serializing instruction before the ret */
    jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(iee_rw_gate)
#if defined(CONFIG_CREDP) || defined(CONFIG_KEYP) || defined(CONFIG_PTP)
EXPORT_SYMBOL(iee_rw_gate)
#endif

SYM_FUNC_START(iee_read_token_stack)
    /* save Interrupt flag */
    pushf
    /* close irq*/
    cli

    push %r12

    DISABLE_WP r12

    addq iee_offset(%rip), %rdi
    movq iee_from_token_offset(%rdi), %rax

    ENABLE_WP r12

    pop %r12

    /* restore irq*/
    popf

    lfence   /* Serializing instruction before the ret */
    jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(iee_read_token_stack)

SYM_FUNC_START(iee_read_tmp_page)
    /* save Interrupt flag */
    pushf
    /* close irq*/
    cli

    push %r12

    DISABLE_WP r12

    addq iee_offset(%rip), %rdi
    movq tmp_page_from_token_offset(%rdi), %rax

    ENABLE_WP r12

    pop %r12

    /* restore irq*/
    popf
    lfence   /* Serializing instruction before the ret */
    jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(iee_read_tmp_page)

SYM_FUNC_START(iee_read_freeptr)
    /* save Interrupt flag */
    pushf
    /* close irq*/
    cli

    push %r12

    DISABLE_WP r12

    addq iee_offset(%rip), %rdi
    movq (%rdi), %rax

    ENABLE_WP r12

    pop %r12

    /* restore irq*/
    popf
    lfence   /* Serializing instruction before the ret */
    jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(iee_read_freeptr)

SYM_FUNC_START(iee_rwx_gate_inner)
    pushq %r12

    /* save Interrupt flag*/
    pushfq
    /* close irq */
    cli

    /* set SMEP=0 to enable supervisor-mode exec user-mode insn */
    movq %cr4, %rax  /* rax -> cr4 */
    andq $(~X86_CR4_SMEP), %rax
    movq %rax, %cr4

    DISABLE_WP r12

    movq %rsp, %r12

    /* If iee hasn't been initialized, skip stack switch. */
    cmpb $0, iee_init_done(%rip)
    jz 2f

    /* switch to iee stack */
    movq PER_CPU_VAR(pcpu_hot + X86_current_task), %rax /* rax -> task_struct */
    addq iee_offset(%rip), %rax
    movq %rsp, kernel_from_token_offset(%rax)
    movq iee_from_token_offset(%rax), %rsp

2:  call iee_si_handler

    /* switch to kernel stack. If iee hasn't been initialized, skip switch*/
    movq %r12, %rsp

    ENABLE_WP r12

    /* set SMEP=1 to disable supervisor-mode exec user-mode insn */
    movq %cr4, %rax  /* rax -> cr4 */
1:  orq $X86_CR4_SMEP_SMAP, %rax
    movq %rax, %cr4
    andq $(X86_CR4_SMEP_SMAP), %rax
    cmpq $(X86_CR4_SMEP_SMAP), %rax
    jnz 1

    /* restore irq*/
    popfq

    popq %r12
    jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(iee_rwx_gate_inner)

SYM_FUNC_START(iee_rwx_gate)
	cmpb $0, iee_si_enabled(%rip)
	jz disabled
	call iee_rwx_gate_inner
	jmp __x86_return_thunk  /* ret */
disabled:
	call iee_si_handler
	jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(iee_rwx_gate)

#ifdef CONFIG_PTP
SYM_FUNC_START(iee_set_xchg)
.L_set_xchg:
    xchg    %rsi, (%rdi)
    mov     %rsi, %rax
    jmp __x86_return_thunk  /* ret */
.E_set_xchg:
    /* save RFLAGS, close irq */
    pushfq
    cli

    push %r12

    DISABLE_WP r12

    addq iee_offset(%rip), %rdi
    xchg    %rsi, (%rdi)
    mov     %rsi, %rax

    ENABLE_WP r12

    popq %r12

    /* restore RFLAGS*/
    popfq
    jmp __x86_return_thunk  /* ret */
    _ASM_EXTABLE(.L_set_xchg, .E_set_xchg)
SYM_FUNC_END(iee_set_xchg)

SYM_FUNC_START(iee_set_try_cmpxchg)
    mov %rsi, %rax
.L_set_try_cmpxchg:
    lock cmpxchgq %rdx, (%rdi)
    jmp __x86_return_thunk  /* ret */
.E_set_try_cmpxchg:
    /* save RFLAGS, close irq */
    pushfq
    cli

    pushq %r12

    DISABLE_WP r12

    addq iee_offset(%rip), %rdi
    lock cmpxchgq %rdx, (%rdi)

    ENABLE_WP r12

    popq %r12
    /* restore RFLAGS*/
    popfq
    jmp __x86_return_thunk  /* ret */
    _ASM_EXTABLE(.L_set_try_cmpxchg, .E_set_try_cmpxchg)
SYM_FUNC_END(iee_set_try_cmpxchg)

.macro IEE_SET_PGTABLE level
.L_set_\level:
    movq %rdx, (%rsi)
    jmp __x86_return_thunk
.E_set_\level:
    pushfq
    cli

    DISABLE_WP rdi

    addq iee_offset(%rip), %rsi
    movq %rdx, (%rsi)

    ENABLE_WP rdi

    popfq
    jmp __x86_return_thunk
    _ASM_EXTABLE(.L_set_\level, .E_set_\level)
.endm

SYM_FUNC_START(iee_set_pte)
    IEE_SET_PGTABLE pte
SYM_FUNC_END(iee_set_pte)
EXPORT_SYMBOL(iee_set_pte)

SYM_FUNC_START(iee_set_pmd)
    IEE_SET_PGTABLE pmd
SYM_FUNC_END(iee_set_pmd)
EXPORT_SYMBOL(iee_set_pmd)

SYM_FUNC_START(iee_set_pud)
    IEE_SET_PGTABLE pud
SYM_FUNC_END(iee_set_pud)
EXPORT_SYMBOL(iee_set_pud)

SYM_FUNC_START(iee_set_p4d)
    IEE_SET_PGTABLE p4d
SYM_FUNC_END(iee_set_p4d)
EXPORT_SYMBOL(iee_set_p4d)

SYM_FUNC_START(iee_set_pgd)
    IEE_SET_PGTABLE pgd
SYM_FUNC_END(iee_set_pgd)
EXPORT_SYMBOL(iee_set_pgd)
#endif
