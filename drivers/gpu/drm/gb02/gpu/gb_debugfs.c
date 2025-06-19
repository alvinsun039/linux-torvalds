#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/sysfs.h>
#include "gb_debugfs.h"
#include "gb_stage.h"
#include "gb_device.h"
#include "gb_gpu.h"

bool gb_time_debug = false;
bool gb_rb_tree = false;
bool gb_mmu_mmap = false;
int job_timeout_ms = 10000;
bool perf_cnt = false;
bool gb_gpu_dma = true;
bool l2_cache_power = true;
bool performance2_set = false;
int performance_ms = 4000;
bool dump_err_vram = true;
int gb_pg_dump_state = 0;
bool gb_current_vram_state = true;


static struct class *genbu02_class;

static char *gb_file_status[] = {
	"disabled",
	"enabled",
};

static char *gb_class_name_arry[] = {
	"time_debug_state",
	"rb_tree_state",
	"mmu_map_state",
	"job_timeout",
	"perf_cnt_state",
	"gpu_dma_state",
	"l2_cache_state",
	"perf2_set_state",
	"perf_time_ms",
	"gb_err_vram_buff",
	"gb_pg_dump",
	"gb_current_vram",
};

static size_t GB02FUNC6(struct class *class,
			struct class_attribute *arttr, char *buf)
{
	if (!strcmp(arttr->attr.name,
			gb_class_name_arry[time_debug_state])) {
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[gb_time_debug]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[rb_tree_state])) {
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[gb_rb_tree]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[mmu_map_state])) {
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[gb_mmu_mmap]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[job_timeout])) {
		return scnprintf(buf, PAGE_SIZE, "%d\n", job_timeout_ms);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[perf_cnt_state])) {
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[perf_cnt]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[gpu_dma_state])) {
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[gb_gpu_dma]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[l2_cache_state])) {
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[l2_cache_power]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[perf2_set_state])) {
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[performance2_set]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[perf_time_ms])) {
		return scnprintf(buf, PAGE_SIZE, "%d\n", performance_ms);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[gb_err_vram_buff])) {
		GB02FUNC1312();
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[dump_err_vram]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[gb_current_vram])) {
		return scnprintf(buf, PAGE_SIZE, "%s\n", gb_file_status[gb_current_vram_state]);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[gb_pg_dump])) {
		return scnprintf(buf, PAGE_SIZE, "%d\n", gb_pg_dump_state);
	}
	gb_printf(KERN_INFO, "%s name:%s state:%s",
		__func__, arttr->attr.name, buf);
	return 0;
}

static size_t GB02FUNC16(struct class *class,
			struct class_attribute *arttr, char *buf, size_t size)
{

	if (!strcmp(arttr->attr.name,
			gb_class_name_arry[time_debug_state])) {
		if (!strcmp(buf, "enabled\n")) {
			gb_time_debug = true;
		} else if (!strcmp(buf, "disabled\n")) {
			gb_time_debug = false;
		}
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[rb_tree_state])) {
		if (!strcmp(buf, "enabled\n")) {
			gb_rb_tree = true;
		} else if (!strcmp(buf, "disabled\n")) {
			gb_rb_tree = false;
		}
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[mmu_map_state])) {
		if (!strcmp(buf, "enabled\n")) {
			gb_mmu_mmap = true;
		} else if (!strcmp(buf, "disabled\n")) {
			gb_mmu_mmap = false;
		}
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[job_timeout])) {
		int timeout_ms;
		sscanf(buf, "%i", &timeout_ms);
		job_timeout_ms = timeout_ms;
		GB02FUNC1349(job_timeout_ms);
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[perf_cnt_state])) {
		if (!strcmp(buf, "enabled\n")) {
			perf_cnt = true;
			GB02FUNC443();
		} else if (!strcmp(buf, "disabled\n")) {
			perf_cnt = false;
		}
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[gpu_dma_state])) {
		if (!strcmp(buf, "enabled\n")) {
			gb_gpu_dma = true;
		} else if (!strcmp(buf, "disabled\n")) {
			gb_gpu_dma = false;
		}
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[l2_cache_state])) {
		if (!strcmp(buf, "enabled\n")) {
			l2_cache_power = true;
			GB02FUNC854();
		} else if (!strcmp(buf, "disabled\n")) {
			l2_cache_power = false;
			GB02FUNC855();
		}
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[perf2_set_state])) {
		if (!strcmp(buf, "enabled\n")) {
			performance2_set = true;
		} else if (!strcmp(buf, "disabled\n")) {
			performance2_set = false;
		}
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[perf_time_ms])) {
		int timeout_ms;
		sscanf(buf, "%i", &timeout_ms);
		performance_ms = timeout_ms;
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[gb_err_vram_buff])) {
		/* vram do not to write */
		dump_err_vram = true;
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[gb_pg_dump])) {
		/* vram do not to write */
		int debug_state;
                sscanf(buf, "%i", &debug_state);
                gb_pg_dump_state = debug_state;
	} else if (!strcmp(arttr->attr.name,
					gb_class_name_arry[gb_current_vram])) {
		u64 sieze;
		sscanf(buf, "%llu", &sieze);
		GB02FUNC1232(sieze);
	}

	gb_printf(KERN_INFO, "%s name:%s change state:%s",
		__func__, arttr->attr.name, buf);
	return size;
}

static struct class_attribute gb_class_file_arry[] = {
	__ATTR(time_debug_state, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(rb_tree_state, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(mmu_map_state, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(job_timeout, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(perf_cnt_state, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(gpu_dma_state, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(l2_cache_state, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(perf2_set_state, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(perf_time_ms, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(gb_err_vram_buff, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(gb_pg_dump, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR(gb_current_vram, 0664,
		(void *)GB02FUNC6, (void *)GB02FUNC16),
	__ATTR_NULL,
};

int GB02FUNC42(struct GB02STR39 *gbdev)
{
	int result = 0;
	int i;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	genbu02_class = class_create(THIS_MODULE, "Genbu02");
#else
	genbu02_class = class_create("Genbu02");
#endif
	if (IS_ERR(genbu02_class)) {
		result = PTR_ERR(genbu02_class);
		goto out;

	}

	for (i = 0; i < GB_CLASS_NAME_MAX; i++) {
		result = class_create_file(genbu02_class,
					&gb_class_file_arry[i]);
		if (result)
			gb_printf(KERN_ERR, "%s create file %s faild\n",
				__func__,
				gb_class_file_arry[i].attr.name);
	}

	return 0;
out:
	return result;
}

void GB02FUNC48(struct GB02STR39 *gbdev)
{
	int i;

	for (i = 0; i < GB_CLASS_NAME_MAX; i++)
		class_remove_file(genbu02_class, &gb_class_file_arry[i]);

	class_destroy(genbu02_class);
}
