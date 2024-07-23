/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_LIVEPATCH_SHADOW_H_
#define _LINUX_LIVEPATCH_SHADOW_H_

typedef int (*klp_shadow_ctor_t)(void *obj,
				 void *shadow_data,
				 void *ctor_data);
typedef void (*klp_shadow_dtor_t)(void *obj, void *shadow_data);

void *klp_shadow_get(void *obj, unsigned long id);
void *klp_shadow_alloc(void *obj, unsigned long id,
		       size_t size, gfp_t gfp_flags,
		       klp_shadow_ctor_t ctor, void *ctor_data);
void *klp_shadow_get_or_alloc(void *obj, unsigned long id,
			      size_t size, gfp_t gfp_flags,
			      klp_shadow_ctor_t ctor, void *ctor_data);
void klp_shadow_free(void *obj, unsigned long id, klp_shadow_dtor_t dtor);
void klp_shadow_free_all(unsigned long id, klp_shadow_dtor_t dtor);

#endif /* _LINUX_LIVEPATCH_SHADOW_H_ */
