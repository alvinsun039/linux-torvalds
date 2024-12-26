/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_STACK_SLAB_H
#define _LINUX_STACK_SLAB_H

extern void __init iee_stack_init(void);
extern void *get_iee_stack(void);
extern void free_iee_stack(void *obj);

#endif
