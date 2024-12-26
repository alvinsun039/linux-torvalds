// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/iee-func.h>
#include <asm/stack-slab.h>
#include <linux/mm_types.h>
#include <asm/io.h>
#include "slab.h"

struct iee_free_slab_work {
	struct work_struct work;
	struct kmem_cache *s;
	struct slab *slab;
};

void iee_free_task_struct_slab(struct work_struct *work)
{
	struct iee_free_slab_work *iee_free_slab_work = container_of(work, struct iee_free_slab_work, work);
	struct kmem_cache *s = iee_free_slab_work->s;
	struct slab *slab = iee_free_slab_work->slab;
	struct folio *folio = slab_folio(slab);
	int order = folio_order(folio);
	// Free stack and tmp page.
	int i;
	void *start = fixup_red_left(s, page_address(folio_page(folio, 0)));
	void *obj;
	void *iee_stack;
	void *tmp_page;
	void *token;

	for (i = 0; i < iee_get_oo_objects(s); i++) {
		obj = start + s->random_seq[i];
		iee_stack = (void *)iee_read_token_stack((struct task_struct *)obj);
		if (iee_stack)
			free_iee_stack((void *)(iee_stack - PAGE_SIZE * 4));

		tmp_page = iee_read_tmp_page((struct task_struct *)obj);
		free_pages((unsigned long)tmp_page, 0);
	}
	// Free token.
	token = (void *)__phys_to_iee(page_to_phys(folio_page(folio, 0)));
	iee_set_token_page_invalid(token, NULL, order);
	__free_pages(&folio->page, order);
	kfree(iee_free_slab_work);
}

#ifdef CONFIG_CREDP
void iee_free_cred_slab(struct work_struct *work)
{
	struct iee_free_slab_work *iee_free_slab_work = container_of(work, struct iee_free_slab_work, work);
	struct slab *slab = iee_free_slab_work->slab;
	struct folio *folio = slab_folio(slab);
	int order = folio_order(folio);

	unset_iee_page((unsigned long)page_address(folio_page(slab_folio(slab), 0)), order);
	__free_pages(&folio->page, order);
	kfree(iee_free_slab_work);
}
#endif

void iee_free_slab(struct kmem_cache *s, struct slab *slab, void (*do_free_slab)(struct work_struct *work))
{
	struct iee_free_slab_work *iee_free_slab_work = kmalloc(sizeof(struct iee_free_slab_work), GFP_ATOMIC);

	iee_free_slab_work->s = s;
	iee_free_slab_work->slab = slab;
	INIT_WORK(&iee_free_slab_work->work, do_free_slab);
	schedule_work(&iee_free_slab_work->work);
}
