/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_FUNC_H
#define _LINUX_IEE_FUNC_H
#include <linux/sched.h>
extern void *alloc_low_pages(unsigned int num);
#define HUGE_PMD_ORDER 9
#define TASK_ORDER 4
extern unsigned long init_iee_stack_begin[];
extern unsigned long init_iee_stack_end[];
extern void *init_token_page_vaddr;

extern void set_iee_page_valid(unsigned long addr);
extern void set_iee_page_invalid(unsigned long addr);
extern void iee_set_logical_mem_ro(unsigned long addr);
extern void set_iee_page(unsigned long addr, int order);
extern void unset_iee_page(unsigned long addr, int order);
extern void iee_mark_all_lm_pgtable_ro(void);
extern void iee_set_token_page_valid(void *token, void *token_page, unsigned int order);
extern void iee_set_token_page_invalid(void *token, void *token_page, unsigned long order);
extern void iee_free_token(struct task_struct *tsk);
extern unsigned long iee_read_token_stack(struct task_struct *tsk);
extern void *iee_read_tmp_page(struct task_struct *tsk);
extern void *iee_read_pgd(struct task_struct *tsk);
extern void iee_set_kernel_upage(unsigned long addr);
extern void iee_rest_init(void);
extern void set_iee_stack_page(unsigned long addr, int order);
extern void unset_iee_stack_page(unsigned long addr, int order);
extern void *iee_read_freeptr(unsigned long ptr);
extern void iee_free_slab(struct kmem_cache *s, struct slab *slab, void (*do_free_slab)(struct work_struct *work));

extern void iee_free_task_struct_slab(struct work_struct *work);
extern void iee_free_cred_slab(struct work_struct *work);
#endif  /* _LINUX_IEE_FUNC_H */
