// SPDX-License-Identifier: GPL-2.0
#include <linux/slab.h>
#include <linux/mm.h>

struct kmem_cache *iee_stack_jar;

void __init iee_stack_init(void)
{
	iee_stack_jar = kmem_cache_create("iee_stack_jar", (PAGE_SIZE << 2),
					  (PAGE_SIZE << 2), SLAB_PANIC, NULL);
}

void *get_iee_stack(void)
{
	return __phys_to_iee(__pa(kmem_cache_alloc(iee_stack_jar, GFP_KERNEL)));
}

void free_iee_stack(void *obj)
{
	kmem_cache_free(iee_stack_jar, __va(__iee_pa(obj)));
}
