#ifndef __GB_MEM_TOOL_H
#define __GB_MEM_TOOL_H
#include "gpu/gb_device.h"
#include "kms/gbdc_crtc.h"


enum gb_block_index {
	GB_BLOCK_MMU,
	GB_BLOCK_FB,
	GB_BLOCK_PRIV,
	GB_BLOCK_CURSOR_FIRST, //3
	GB_BLOCK_MAX = 6
};

typedef struct genbu_job_header {
   unsigned long exception_status : 32;
   unsigned long first_incomplete_task : 32;
   unsigned long fault_pointer : 64;
   unsigned long is_64b                : 1;
   unsigned long type                  : 7;
   unsigned long barrier               : 1;
   unsigned long invalidate_cache      : 1;
   unsigned long reserved_1            : 1;
   unsigned long suppress_prefetch     : 1;
   unsigned long enable_texture_mapper : 1;
   unsigned long reserved_2            : 1;
   unsigned long relax_dependency_1    : 1;
   unsigned long relax_dependency_2    : 1;
   unsigned long index                 : 16;
   unsigned long dependency_1 : 16;
   unsigned long dependency_2 : 16;
   unsigned long next : 64;
} genbu_job_header;

void GB02FUNC193(struct GB02STR162 *stage);

#define GB02MAC511
#ifdef GB02MAC511
int GB02FUNC202(char *path, unsigned int size, u32 *buff);
int GB02FUNC206(char *path, unsigned int size, u64 start_offset,
    int debug);
int GB02FUNC211(unsigned long long offset, int size, int debug);
int GB02FUNC294(u64 gb_addr, u64 size, int block_index);
void GB02FUNC308(u64 fb_addr, u64 size);
int GB02FUNC285(void);
int GB02FUNC257(struct drm_device *dev, struct GB02STR39 *gbdev);
int GB02FUNC296(struct GB02STR39 *gb_dev);
int GB02FUNC213(struct gbdc_crtc *gbdc_crtc, int debug);
void GB02FUNC188(unsigned int size, u64 start_offset, u32 *buff, int debug);
#else
static inline void GB02FUNC308(u64 fb_addr, u64 size) {}
static inline int GB02FUNC206(char *path, unsigned int size,
    u64 start_offset, int debug)
{
    return 0;
}

static inline int GB02FUNC211(unsigned long long offset, int size, int debug)
{
    return 0;
}

static inline int
GB02FUNC294(u64 gb_addr, u64 size, int block_index)
{
    return 0;
}

static inline int GB02FUNC285(void)
{
    return 0;
}

int GB02FUNC257(struct drm_device *dev, struct GB02STR39 *gbdev)
{
    return 0;
}

static inline int GB02FUNC296(struct GB02STR39 *gb_dev)
{
    return 0;
}
#endif
#endif
