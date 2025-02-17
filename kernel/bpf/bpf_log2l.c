// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file contains the implementation of the bpf_log2l function, which
 * computes the floor of the base-2 logarithm of a positive long integer. It
 * is exposed as a BPF kernel function (kfunc) to be used by various BPF
 * program types.
 *
 * Copyright (C) 2025 KylinSoft Corporation
 * Author: Jackie Liu <liuyun01@kylinos.cn>
 *
 */
#include <linux/bpf.h>
#include <linux/btf_ids.h>
#include <linux/log2.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>  /* Provides THIS_MODULE */

__bpf_kfunc_start_defs();

/**
 * bpf_log2l - Compute the floor of the base-2 logarithm of a positive long.
 * @v: A positive long integer.
 *
 * This function returns the floor of log2(v) for positive values.
 * It is exposed as a BPF kernel function (kfunc) so that various BPF
 * program types can call it.
 */
__bpf_kfunc unsigned int bpf_log2l(unsigned long v)
{
	return ilog2(roundup_pow_of_two(v));
}

__bpf_kfunc_end_defs();

/*
 * Build the BTF kfunc ID set for bpf_log2l. The BTF_KFUNCS_* macros
 * ensure that the BTF information for this function is recorded.
 */
BTF_SET8_START(bpf_log2l_ids)
BTF_ID_FLAGS(func, bpf_log2l)
BTF_SET8_END(bpf_log2l_ids)

/*
 * Define the kfunc set structure. The owner field is set to THIS_MODULE.
 * When built into the kernel, this symbol is effectively permanent.
 */
static const struct btf_kfunc_id_set log2l_kfunc_set = {
	.owner = THIS_MODULE,
	.set   = &bpf_log2l_ids,
};

/**
 * bpf_log2l_kfunc_init - Register bpf_log2l for all BPF program types.
 *
 * This function registers the bpf_log2l kfunc for a list of BPF program
 * types. This ensures that bpf_log2l is available in all contexts where
 * BPF programs run.
 *
 * Return: 0 on success, or a negative error code if registration fails.
 */
static int __init bpf_log2l_kfunc_init(void)
{
	/* Array of BPF program types to register the kfunc for.
	 * Extend this list for new program types.
	 */
	int prog_types[] = {
		BPF_PROG_TYPE_SOCKET_FILTER,
		BPF_PROG_TYPE_TRACING,
		BPF_PROG_TYPE_SYSCALL,
		BPF_PROG_TYPE_LSM,
		BPF_PROG_TYPE_STRUCT_OPS,
		BPF_PROG_TYPE_UNSPEC,
	};

	for (int i = 0; i < ARRAY_SIZE(prog_types); i++) {
		int err = register_btf_kfunc_id_set(prog_types[i],
						    &log2l_kfunc_set);
		if (err) {
			pr_err("Failed to register bpf_log2l for prog type %d: %d\n",
			       prog_types[i], err);
			return err;
		}
	}

	return 0;
}

late_initcall(bpf_log2l_kfunc_init);
