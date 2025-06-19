#ifndef __GB_TTM_H__
#define __GB_TTM_H__
#include <generated/uapi/linux/version.h>
#include "kms/gb_pcie_map.h"

#define	GB02MAC2038 (0x100000000ULL >> PAGE_SHIFT)
#define GB02MAC2039 1

/* ttm */
struct GB02STR175 {
/*#ifndef NO_DRM_GLOBAL_REF*/
#if (!(defined CONFIG_CENTOS || defined SYS_CENTOS7_9_2009) && \
	defined SYS_CENTOS7_COMPILE_ENV) \
	|| (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0) && \
	!defined CONFIG_CENTOS_OS)
	struct drm_global_reference mem_global_ref;
	struct ttm_bo_global_ref bo_global_ref;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	struct ttm_bo_device bdev;
#else
	struct ttm_device bdev;
#endif
	bool initialized;
};

struct GB02STR176 {
	struct GB02STR175	ttm;
};
struct GB02STR177 {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	struct ttm_resource_manager manager;
#endif
	struct drm_mm mm;
	spinlock_t lock;
};
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static inline struct GB02STR177 *
to_range_manager(struct ttm_resource_manager *man)
{
	return container_of(man, struct GB02STR177, manager);
}
#endif

int GB02FUNC1516(struct file *filp, struct vm_area_struct *vma);
int GB02FUNC1502(struct drm_device *dev, size_t size, int align,
			  int init_domain, uint32_t flags,
			  bool kernel, struct GB02STR50 *ttm_bo);
void GB02FUNC1508(struct GB02STR50 *ttm_bo);

u64 GB02FUNC1484(struct GB02STR50 *bo);
u64 GB02FUNC1394(struct GB02STR50 *bo);
int gb_ttm_get_bo_offset(struct GB02STR50 *bo);

int GB02FUNC1518(struct GB02STR50 *ttm_bo, bool no_intr, int domain);
void GB02FUNC1522(struct GB02STR50 *ttm_bo);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
int GB02FUNC1485(struct GB02STR50 *bo, u32 pl_flag, u64 *gpu_addr);
#else
int GB02FUNC1485(struct GB02STR50 *bo, u32 mem_type, u64 *gpu_addr);
#endif
int GB02FUNC1487(struct GB02STR50 *bo);
int GB02FUNC1480(struct drm_device *drm_dev,
		struct GB02STR253 *vram_config, struct GB02STR175 *ttm);
void GB02FUNC1482(struct GB02STR176	*gb_mm);
int GB02FUNC1513(struct GB02STR59 *bo, struct GB02STR39 *gbdev,
	struct GB02STR47 *gb_priv, u64 addr, int clear_intr, int as);
extern int GB02FUNC1495(int mem_type);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
struct ttm_resource *
genbu_ttm_resource_manager_first(struct ttm_resource_manager *man,
			   struct ttm_resource_cursor *cursor);
struct ttm_resource *
genbu_ttm_resource_manager_next(struct ttm_resource_manager *man,
			  struct ttm_resource_cursor *cursor,
			  struct ttm_resource *res);

#define genbu_ttm_resource_manager_for_each_res(man, cursor, res)		\
	for (res = genbu_ttm_resource_manager_first(man, cursor); res;	\
	     res = genbu_ttm_resource_manager_next(man, cursor, res))
#endif

#endif
