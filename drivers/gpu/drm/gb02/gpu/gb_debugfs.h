#ifndef	__GB_DEBUGFS_H__
#define	__GB_DEBUGFS_H__
#include "gb_device.h"
#include "common/gb_common.h"

enum {
	time_debug_state = 0,
	rb_tree_state,
	mmu_map_state,
	job_timeout,
	perf_cnt_state,
	gpu_dma_state,
	l2_cache_state,
	perf2_set_state,
	perf_time_ms,
	gb_err_vram_buff,
	gb_pg_dump,
	gb_current_vram,
	GB_CLASS_NAME_MAX,
};

int GB02FUNC42(struct GB02STR39 *gbdev);
void GB02FUNC48(struct GB02STR39 *gbdev);

#endif
