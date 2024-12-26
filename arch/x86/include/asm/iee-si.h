/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_SI_H
#define _LINUX_IEE_SI_H
#define __iee_si_code   __section(".iee.si_text")
#define __iee_si_base   __section(".iee.si_base")
#define __iee_si_data   __section(".iee.si_data")

extern bool iee_pgt_jar_init;
extern bool iee_init_done;
extern unsigned long __iee_si_text_start[];
extern unsigned long __iee_si_text_end[];
extern unsigned long __iee_si_data_start[];
extern unsigned long __iee_si_data_end[];
extern void iee_rwx_gate(int flag, ...);

extern const unsigned long cr4_pinned_mask;
extern struct static_key_false cr_pinning;
extern unsigned long cr4_pinned_bits;
// Handler function for sensitive inst
u64 iee_si_handler(int flag, ...);

#define IEE_SI_TEST     0
#define IEE_WRITE_CR0   1
#define IEE_WRITE_CR3   2
#define IEE_WRITE_CR4   3
#define IEE_LOAD_IDT    4
#endif
