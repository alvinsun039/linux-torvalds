#ifndef __GB_VPU_OBJECT_H__
#define __GB_VPU_OBJECT_H__

void GB02FUNC1757(struct ttm_buffer_object *tbo);
int GB02FUNC1758(struct GB02STR56 *bo, void **ptr);
void GB02FUNC1760(struct GB02STR201 *bo);
/**
 * radeon_bo_reserve - reserve bo
 * @bo:		bo structure
 * @no_intr:	don't return -ERESTARTSYS on pending signal
 *
 * Returns:
 * -ERESTARTSYS: A wait for the buffer to become unreserved was interrupted by
 * a signal. Release all buffer reservations and return to user-space.
 */
static inline int GB02FUNC1762(struct ttm_buffer_object	 *tbo, bool no_intr)
{
	int r;

	r = ttm_bo_reserve(tbo, !no_intr, false, NULL);
	if (unlikely(r != 0)) {
		if (r != -ERESTARTSYS)
			gb_printf(KERN_ERR, "%s-%d: %p reserve failed\n",
			 __func__, __LINE__, tbo);
		return r;
	}
	return 0;
}

static inline void GB02FUNC1763(struct ttm_buffer_object	 *tbo)
{
	ttm_bo_unreserve(tbo);
}
#define dec_bo_to_gb_bo(dec_bo) \
	container_of(dec_bo, struct GB02STR59, dec_base)

#endif
