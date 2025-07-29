#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/fdtable.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/ttm/ttm_bo_driver.h>
#else
#include <drm/ttm/ttm_bo.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#include <linux/vmalloc.h>
#endif

#include "gb_mem_tool.h"
#include "common/gb_common.h"
#include "common/gb_bo.h"
#include "gpu/gb_mmu.h"
#include "gpu/gb_stage.h"
#include "ip/gb_edma.h"
#include "gpu/gb_regs.h"
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem.h>

#ifdef GB02MAC511
#define DEFAUL_PATH "/home/sietium/gb_vram_dump.dmp"
#define GB02MAC333 200 * 1024 * 1024
extern bool gb_time_debug;
/*get vram base addr*/
struct gb_vram_blk_info
{
	phys_addr_t vram_start;
	u32 size;
	u32 tmp_size;
	u64 *gb_data;
};

struct gb_mem_dump
{
	struct drm_device *dev;
	struct GB02STR39 *gb_dev;
	void __iomem *vram_base_addr;
	// user to dump vram to file
	u64 max_buffer;
	struct mutex file_lock;
	struct mutex gb_block_dump_lock;
	// use to save key vram to ram
	struct gb_vram_blk_info gb_key_block[GB_BLOCK_MAX];
	struct gb_vram_blk_info *gb_vram_blk_list; //use to save all of bo phy addr
	struct gb_vram_blk_info *gbvpu_vram_blk_list; //use to save all of bo phy addr
	int count;
	int pagetotal;
	u64 *cpu_va;
	struct page **cpage;
	phys_addr_t *gpu_phys;
	atomic_t bo_count;
	atomic_t vbo_count;
};

static struct gb_mem_dump gb_dump_cb;
/*dump vram*/
static inline unsigned int GB02FUNC180(u64 offset)
{
	return *(u32 *)((u64)gb_dump_cb.vram_base_addr + offset);
}
/*
int GB02FUNC184(u64 addr)
{
	u64 iova;
	u64 pgd;
	u64 index_mask;
	struct GB02STR39 *gbdev;
	struct GB02STR59 *bo = NULL;
	gbdev = gb_dump_cb.gb_dev;
	bo = GB02FUNC949(gbdev, addr);
	if (!bo) {
		gb_printf(KERN_ERR, "%s: can't found bo by addr 0x%llx\n",
			__func__, addr);
		return -ENOENT;
	}
	index_mask = 0x1ff;
	pgd = gb_dev->mmu_mode->pgd;
	iova = bo->base.iovaddr;
	return 0;
}
*/
void GB02FUNC188(unsigned int size, u64 start_offset, u32 *buff, int debug)
{
	int i;
	for (i = 0; i < size / 16; i++) {
		if (debug) {
			pr_info("0x%llx: %08x %08x %08x %08x\n", start_offset,
			                    GB02FUNC180(start_offset),
			                    GB02FUNC180(start_offset + 4),
			                    GB02FUNC180(start_offset + 8),
			                    GB02FUNC180(start_offset + 12));
		}
		if (buff) {
			buff[i * 4] = GB02FUNC180(start_offset);
			buff[i * 4 + 1] = GB02FUNC180(start_offset + 4);
			buff[i * 4 + 2] = GB02FUNC180(start_offset + 8);
			buff[i * 4 + 3] = GB02FUNC180(start_offset + 12);
		}
		start_offset += 16;
	}
}

static char* GB02FUNC190(unsigned long type)
{
	switch(type) {
		case 1:
			return "GPU_JOB_NULL";
		case 2:
		        return "GPU_JOB_SET_VAULE";
		case 3:
		        return "GPU_JOB_CACHE_FLUSH";
		case 4:
		        return "GPU_JOB_COMPUTE";
		case 5:
		        return "GPU_JOB_VERTEX";
		case 6:
		        return "GPU_JOB_GEOMETRY";
		case 7:
		        return "GPU_JOB_TILER";
		case 9:
		        return "GPU_JOB_FRAGMENT";
		case 10:
		        return "GPU_JOB_TYPE_INDEXED_VERTEX_SHADER_JOB";
		default:
			return "NULL";
	}
}

void GB02FUNC193(struct GB02STR162 *stage)
{
	u64 size;
	int job_flag = 1;
	u64 fault_pointer, base_addr, next_job_addr = 0;
	char *job_type;
	u64 v_offset, offset;
	u32 exception_status;
	void __iomem *cpu_va;
	struct GB02STR59 *bo;
	struct gb_heap_ttm_bo *gb_heap_bo;
	genbu_job_header *gb_job_head;

	// parse stage->sc to phy addr need GB02FUNC949
	bo = GB02FUNC949(stage->gbdev, stage->sc); // get bo by sc

	if (!bo) {
		gb_printf(KERN_ERR, "[%s] get bo by sc failed\n", __func__);
		goto end;
	}

	base_addr = bo->base.iovaddr; // get bo base vaddr
	v_offset = stage->sc - base_addr; // get sc offset
	gb_job_head = kvzalloc(sizeof(genbu_job_header), GFP_KERNEL);

	do {
		offset = v_offset + bo->base.pages[0]; // real sc phy addr
		cpu_va = (void __iomem *)((u64)gb_dump_cb.vram_base_addr + offset); // phy addr -> cpu_va
		memcpy_fromio(gb_job_head, cpu_va, sizeof(genbu_job_header)); // copy job head da

		/* Analysis job data*/
		next_job_addr = gb_job_head->next; // get next job gpu_va
		fault_pointer = gb_job_head->fault_pointer; // get current job fault status
		v_offset = next_job_addr - base_addr; // get job addr offset
		exception_status = gb_job_head->exception_status; //exception_status

		if (job_flag) {
			job_type = GB02FUNC190(gb_job_head->type);
			gb_info(stage->gbdev->dev,
				"[%20s] fault_pointer:0x%016llx next_job_addr: 0x%016llx "
				"exception_status:0x%u\n"
				"index: %u\n"
				"relax_dep1: %u,relax_dep2: %u\n"
				"dep1      : %u,      dep2: %u\n"
				"slot:0x%u sc:0x%llx user_pid:0x%llx\n",
				job_type, fault_pointer, next_job_addr, exception_status,
				gb_job_head->index,
				gb_job_head->relax_dependency_1, gb_job_head->relax_dependency_2,
				gb_job_head->dependency_1, gb_job_head->dependency_2,
				stage->slot_req, stage->sc, stage->user_pid);
			if ((next_job_addr & 0xFFFFFFFF) == stage_read(stage->gbdev, GB02MAC1506(!stage->slot_req)))
				job_flag = 0;
		}
	} while (next_job_addr && (0 == fault_pointer));

	kvfree(gb_job_head);

	if (!fault_pointer) {
		gb_printf(KERN_ERR, "[%s] not get fault addr\n", __func__);
		goto end;
	}

	bo = GB02FUNC949(stage->gbdev, fault_pointer);
	if (!bo) {
		gb_printf(KERN_ERR, "[%s] get bo by fault addr failed\n", __func__);
		goto end;
	}

	gb_printf(KERN_ERR, "[%s] fault_addr 0x%llx\n"
			"bo 0x%llx bo start: 0x%llx, end 0x%llx "
			"pa 0x%llx, size %llx\n",
			__func__, fault_pointer, (u64)bo, bo->base.node.start,
			(bo->base.node.start + bo->base.node.size) << PAGE_SHIFT,
			bo->base.pages[0], bo->base.node.size);

	if (bo->is_heap_growable) {
        	list_for_each_entry(gb_heap_bo, &bo->gb_base.ttm_bo_root, ttm_bo_list) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
			size = gb_heap_bo->ttm_bo.bo.base.size;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
			size = gb_heap_bo->ttm_bo.bo.resource->num_pages * PAGE_SIZE;
#else
			size = gb_heap_bo->ttm_bo.bo.mem.num_pages * PAGE_SIZE;
#endif
			pr_info("[%s] sub bo%llx sub bo gpuva 0x%llx end 0x%llx, nr_pages %d phy 0x%llx\n",
                    __func__, (u64)gb_heap_bo, gb_heap_bo->gb_va_start,
					gb_heap_bo->gb_va_start + gb_heap_bo->grow_nr_pages * GB02MAC311,
					gb_heap_bo->grow_nr_pages, gb_heap_bo->pages[0]);
			GB02FUNC188(size, gb_heap_bo->pages[0], NULL, gb_time_debug);
		}
	} else {
		size = bo->base.nr_pages * PAGE_SIZE;
		GB02FUNC188(size, bo->base.pages[0], NULL, gb_time_debug);
	}
end:
	return;
}

int GB02FUNC202(char *path, unsigned int size, u32 *buff)
{
	struct file *file = NULL;
	int ret = 0;

	if (NULL == path)
		path = DEFAUL_PATH;

	mutex_lock(&gb_dump_cb.file_lock);
	file = filp_open(path, O_RDWR | O_CREAT, 0666);
	if (file) {
		ret = kernel_write(file, buff, size, &file->f_pos);
		filp_close(file, NULL);
	} else
		ret = -1;

	mutex_unlock(&gb_dump_cb.file_lock);

	return ret;
}

int GB02FUNC206(char *path, unsigned int size, u64 start_offset,
	int debug)
{
	u32 *buff;
	int ret = 0;

	if (size > GB02MAC333)
		size = GB02MAC333;

	size = round_down(size, 16);
	buff = kvmalloc(size, GFP_KERNEL);

	if (!buff)
		return -ENOMEM;

	memset(buff, 0, size);
	GB02FUNC188(size, start_offset, buff, debug);
	ret = GB02FUNC202(path, size, buff);
	kvfree(buff);
	return ret;
}

int GB02FUNC211(unsigned long long offset, int size, int debug)

{
	GB02FUNC206("/home/sietium/gb_dump_mem.dmp", size, offset, 1);
	return 0;
}
int GB02FUNC213(struct gbdc_crtc *gbdc_crtc, int debug)
{
	char dump_file[100] = {0};
	u64 size, offset;
	struct drm_crtc *crtc = NULL;

	if (!gbdc_crtc)
		return 0;

	crtc = &gbdc_crtc->base;

	if (!crtc)
		return  0;

	if (!strcmp(current->comm, "plymouthd") && crtc->enabled) {
		struct drm_framebuffer *fb = NULL;

		sprintf(dump_file, "/home/sietium/crtc%d.bin", gbdc_crtc->crtc_id);
		offset = gbdc_crtc->cplanes[DC_PLANE_GRAPHIC].cur_fb_offset;
		pr_info("%s, offset %llx\n", __func__, offset);

		if (crtc->primary && crtc->primary->fb)
			fb = crtc->primary->fb;

		if (fb)	{
			size = fb->width * fb->height * 4;
			pr_info("%s, size 0x%llx\n", __func__, size);
			GB02FUNC206(dump_file, size, offset, debug);
		}
	}

	return 0;
}

static int GB02FUNC223(struct drm_device *dev, struct GB02STR39 *gbdev,
	u64 src_addr, u64 dst_addr, int size,int write)
{
	phys_addr_t *gpu_phys = NULL;
	struct page **user_pages = NULL;
	struct page *tmp_pages = NULL;
	unsigned long num_dma_pages = (size - 1) / PAGE_SIZE + 1;
	struct GB02STR188 dma_info = {0};
	int ret, i, count = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	struct timespec t1, t2, t3, t4, t5;
#else
	struct timespec64 t1, t2, t3, t4, t5;
#endif

#ifdef GB_TIME_DEBUG
	long time_use1, time_use2, time_use3, time_use4, time_use5;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t1);
#else
	ktime_get_real_ts64(&t1);
#endif

	mdelay(100);

	if (dst_addr % PAGE_SIZE) {
		gb_printf(KERN_ERR, "%s %d dma addr invalid!cva:0x%llx\n",
			__func__, __LINE__, dst_addr);
		return -1;
	}

	if (size >= 32 * GB) {
		gb_printf(KERN_ERR, "%s %d dma size 0x%x larger than 32GB!\n",
			__func__, __LINE__, size);
		return -1;
	}
	mdelay(100);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t2);
#else
	ktime_get_real_ts64(&t2);
#endif
	user_pages = kvmalloc(sizeof(struct page *) * num_dma_pages,
			GFP_KERNEL | __GFP_ZERO);
	mdelay(100);
	for (i = 0; i < num_dma_pages; i++)
	{
		tmp_pages = vmalloc_to_page((void *)(dst_addr + i * PAGE_SIZE));
		if (tmp_pages) {
			user_pages[count] = tmp_pages;
			count++;
		} else {
			dev_err(gbdev->dev, "%s no more cpu mem for suspend", __func__);
			return -1;
		}
	}
	mdelay(100);
	if (count <= 0) {
		kfree(user_pages);
		user_pages = NULL;
		gb_printf(KERN_ERR, "%s %d get user %lu cpu pages failed!\n",
			__func__, __LINE__, num_dma_pages);
		return -1;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t3);
#else
	ktime_get_real_ts64(&t3);
#endif
	num_dma_pages = count * GB02MAC317;
	mdelay(100);
	gpu_phys = kvzalloc(num_dma_pages * sizeof(phys_addr_t*), GFP_KERNEL);
	for (i = 0; i < num_dma_pages; i++)
		gpu_phys[i] = src_addr + i * GB02MAC311;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t4);
#else
	ktime_get_real_ts64(&t4);
#endif
	num_dma_pages = count;

	dma_info.cpu_pages = user_pages;
	dma_info.gpu_phys = gpu_phys;
	dma_info.num_dma_pages = num_dma_pages;
	dma_info.dma_dir = write;
	dma_info.last_page_size = size % PAGE_SIZE;
	mdelay(100);
	ret = GB02FUNC1688(dev->dev, &dma_info, gbdev->gb_pcie);
	if (ret) {
		gb_printf(KERN_ERR, "%s %d dma failed! cpu va:0x%llx,size:0x%x,dir:%d\n",
			__func__, __LINE__, dst_addr,
			size, write);
		ret = -1;
		goto release_pages;
	}
	mdelay(100);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	getnstimeofday(&t5);
#else
	ktime_get_real_ts64(&t5);
#endif

#ifdef GB_TIME_DEBUG
	time_use1 = (t2.tv_sec - t1.tv_sec) * 1000000 +
		(t2.tv_nsec - t1.tv_nsec)/1000;
	time_use2 = (t3.tv_sec - t2.tv_sec) * 1000000 +
		(t3.tv_nsec - t2.tv_nsec)/1000;
	time_use3 = (t4.tv_sec - t3.tv_sec) * 1000000 +
		(t4.tv_nsec - t3.tv_nsec)/1000;
	time_use4 = (t5.tv_sec - t4.tv_sec) * 1000000 +
		(t5.tv_nsec - t4.tv_nsec)/1000;
	time_use5 = (t5.tv_sec - t1.tv_sec) * 1000000 +
                (t1.tv_nsec - t1.tv_nsec)/1000;
	gb_printf(KERN_INFO, "%s user page: %ld gpu page: %ld drm_start:%ld,\
		total :%ld\n",__func__, time_use2, time_use3, time_use4, time_use5);

#endif
release_pages :
	kvfree(user_pages);
	kvfree(gpu_phys);
	return ret;
}

#define GB02MAC510   ((64 * 1024 * 1024) / PAGE_SIZE)

int GB02FUNC242(struct drm_device *dev,
			struct GB02STR39 *gbdev,
			struct page **page,
			phys_addr_t *phys,
			int numpages,
			int write)
{
	int ret = 0;
	int i;
	int cnt = numpages / GB02MAC510;
	struct GB02STR188 dma_info = {0};

	for (i = 0; i < cnt; i++) {
		dma_info.cpu_pages = &page[i * GB02MAC510];
		dma_info.gpu_phys = &phys[i * GB02MAC317 * GB02MAC510];
		dma_info.num_dma_pages = GB02MAC510;
		numpages -= GB02MAC510;
		dma_info.dma_dir = write;
		dma_info.last_page_size = 0;
		ret = GB02FUNC1688(dev->dev, &dma_info, gbdev->gb_pcie);
		if (ret) {
			gb_printf(KERN_ERR, "%s edma translate failed!\n", __func__);
			return ret;
		}
	}

	if (numpages > 0) {
		dma_info.cpu_pages = &page[i * GB02MAC510];
		dma_info.gpu_phys = &phys[i * GB02MAC317 * GB02MAC510];
		dma_info.num_dma_pages = numpages;
		dma_info.dma_dir = write;
		dma_info.last_page_size = 0;
		ret = GB02FUNC1688(dev->dev, &dma_info, gbdev->gb_pcie);
		if (ret) {
			gb_printf(KERN_ERR, "%s edma translate failed!\n", __func__);
			return ret;
		}
	}

	return 0;
}

static void GB02FUNC252(struct GB02STR59 *gb_bo,
	struct GB02STR39 *gbdev)
{
	int i, j;
	struct GB02STR2 *tmp = NULL;

	if (gb_bo->domain != GB02MAC2678)
		return;
	list_for_each_entry(tmp, &gbdev->vpu_bo_list_head, vbo_list) {
		for (i = 0; i < GB02MAC20; i++) {
			for (j = 0; j < GB02MAC17; j++) {
				if (gb_bo->vbo_save_flag & GB02MAC946 &&
					(u64)gb_bo == tmp->vpu_bo_priv_addr[i][j])
						gb_bo->vbo_save_flag = 0;
			}
		}
	}
}

// need hold ddr lock before call backup/restore func
int GB02FUNC257(struct drm_device *dev, struct GB02STR39 *gbdev)
{
	int i = 0;
	int j = 0;
	int count = 0;
	int lru_cnt = 0;
	int cpupagecount = 0;
	int ret = 0;
	int pagetotal = 0;
	int numpages = 0;
	int phypagecnt = 0;
	u64 src_addr;
	u64 *dst_addr;
	u64 *base_addr;
	int vbo_nsave_page_num = 0;
	struct GB02STR59 *gb_bo;
	struct page *temp_pages;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	struct ttm_buffer_object *tbo;
#endif
	struct GB02STR50 *gb_tbo;
	struct gb_vram_blk_info *gb_block;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	struct ttm_bo_device *tdev;
	struct ttm_mem_type_manager *man;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	struct ttm_bo_global *glob = &ttm_bo_glob;
	struct ttm_bo_device *tdev;
	struct ttm_resource_manager *man;
#else
	struct ttm_device *tdev;
	struct ttm_resource_manager *man;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	struct ttm_resource_cursor cursor;
	struct ttm_resource *res;
#endif
	gb_block = gb_dump_cb.gb_key_block;
	base_addr = gb_dump_cb.vram_base_addr;
	for (i = 0; i < GB_BLOCK_MAX; i++) {
		if (gb_block[i].size > 0) {
			src_addr = gb_block[i].vram_start;
			dst_addr = vmalloc(gb_block[i].size);
			gb_block[i].tmp_size = gb_block[i].size;
			gb_printf(KERN_INFO, "[%s]bo src %llx dst %llx size %x \
				base_addr 0x%llx offset 0x%llx\n", __func__,(u64)src_addr,
				(u64)dst_addr, gb_block[i].size, (u64)base_addr,
				(u64)gb_block[i].vram_start);
			GB02FUNC223(dev, gbdev, src_addr, (u64)dst_addr,
				gb_block[i].size, 1);
			gb_block[i].gb_data = dst_addr;
		}
	}

	tdev = &gb_dump_cb.gb_dev->gb_mm.ttm.bdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	man = &tdev->man[TTM_PL_VRAM];
#else
	man = tdev->man_drv[TTM_PL_VRAM];
#endif
	if (list_empty(&man->lru[0]))
		return ret;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	list_for_each_entry(tbo, &man->lru[0], lru) {
		gb_tbo = ttm_ob_to_ttm_bo(tbo);
#else
	genbu_ttm_resource_manager_for_each_res(man, &cursor, res) {
		gb_tbo = ttm_ob_to_ttm_bo(res->bo);
#endif    		
		if (!gb_tbo->sub_bo_flag) {
			gb_bo = GB02FUNC205(gb_tbo);
			GB02FUNC252(gb_bo, gbdev);
			if (gb_bo->vbo_save_flag & GB02MAC949)
				vbo_nsave_page_num += 1;
		}
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	count = atomic_read(&tdev->glob->bo_count);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	count = atomic_read(&glob->bo_count);
#else
	count = atomic_read(&ttm_glob.bo_count);
#endif
	count = count - vbo_nsave_page_num;
	pr_info("[%s] bo count %d\n", __func__, count);
	gb_block = kvmalloc(count * sizeof(struct gb_vram_blk_info), GFP_KERNEL);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	list_for_each_entry(tbo, &man->lru[0], lru) {
		gb_tbo = ttm_ob_to_ttm_bo(tbo);
#else
	genbu_ttm_resource_manager_for_each_res(man, &cursor, res) {
		gb_tbo = ttm_ob_to_ttm_bo(res->bo);
#endif    		
		if (!gb_tbo->sub_bo_flag) {
			gb_bo = GB02FUNC205(gb_tbo);
			if (gb_bo->vbo_save_flag & GB02MAC949)
				continue;
			if (gb_bo->cmdbuf_type == GB02MAC968)
				continue;
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
		gb_block[lru_cnt].vram_start = gb_tbo->bo.offset;
		gb_block[lru_cnt].size = gb_tbo->bo.mem.num_pages * PAGE_SIZE;
		pagetotal += gb_tbo->bo.mem.num_pages;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0) 
		gb_block[lru_cnt].vram_start = gb_tbo->offset;
		gb_block[lru_cnt].size = gb_tbo->bo.num_pages * PAGE_SIZE;
		pagetotal += gb_tbo->bo.num_pages;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0) 
		gb_block[lru_cnt].vram_start = gb_tbo->offset;
		gb_block[lru_cnt].size = PFN_UP(gb_tbo->bo.base.size) * PAGE_SIZE;
		pagetotal += PFN_UP(gb_tbo->bo.base.size);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0) 
		gb_block[lru_cnt].vram_start = gb_tbo->offset;
		gb_block[lru_cnt].size = gb_tbo->bo.resource->num_pages * PAGE_SIZE;
		pagetotal += gb_tbo->bo.resource->num_pages;
#endif
		lru_cnt++;
	}

	gb_dump_cb.pagetotal = pagetotal;
	dst_addr = vmalloc(pagetotal * PAGE_SIZE);
	if (!dst_addr) {
		gb_printf(KERN_ERR, "[%s] malloc cpuva Fail!\n", __func__);
		return -1;
	}
	gb_dump_cb.cpu_va = dst_addr;
	gb_dump_cb.cpage = kvmalloc(sizeof(struct page *) * pagetotal, GFP_KERNEL | __GFP_ZERO);
	if (!dst_addr) {
		gb_printf(KERN_ERR, "[%s] malloc cpuva Fail!\n", __func__);
		return -1;
	}

	for (i = 0; i < pagetotal; i++) {
		temp_pages = vmalloc_to_page((void *)dst_addr + i * PAGE_SIZE);
		if (temp_pages) {
			gb_dump_cb.cpage[cpupagecount] = temp_pages;
			cpupagecount++;
		}
	}

	if (cpupagecount <= 0) {
		gb_printf(KERN_ERR, "[%s] cpu page num invalild!\n", __func__);
		return -1;
	}

	gb_dump_cb.gpu_phys = kvmalloc(pagetotal * GB02MAC317 * sizeof(phys_addr_t), GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < lru_cnt; i++) {
		src_addr = gb_block[i].vram_start;
		numpages = gb_block[i].size / GB02MAC311;
		for (j = 0; j < numpages; j++) {
			gb_dump_cb.gpu_phys[phypagecnt] = src_addr + j * GB02MAC311;
			phypagecnt++;
		}
	}
	
	ret = GB02FUNC242(dev, gbdev, gb_dump_cb.cpage, gb_dump_cb.gpu_phys, pagetotal, 1);
	if (ret)
		gb_printf(KERN_ERR, "[%s] dma translate failed!\n", __func__);

	gb_printf(KERN_INFO, "[%s] count %d", __func__, i);
	gb_dump_cb.gb_vram_blk_list = gb_block; // free after restore success
	gb_dump_cb.count = i;
	gb_dump_cb.dev = dev;
	gb_dump_cb.gb_dev = gbdev;
	return ret;
}

int GB02FUNC285(void)
{
	int i, count;
	u64 *src_addr;
	u64 *dst_addr;
	u64 *base_addr;
	struct gb_vram_blk_info *gb_block;
	gb_block = gb_dump_cb.gb_key_block;
	base_addr = gb_dump_cb.vram_base_addr;
	for (i = 0; i < GB_BLOCK_MAX; i++) {
		dst_addr = (u64*)((u64)base_addr + gb_block[i].vram_start);
		src_addr = gb_block[i].gb_data;
		if (src_addr && gb_block[i].tmp_size > 0) {
			gb_printf(KERN_INFO, "[%s]bo src %llx dst %llx size %x base_addr 0x%llx\
				offset 0x%llx\n", __func__, (u64)src_addr, (u64)dst_addr,
				gb_block[i].size, (u64)base_addr, (u64)gb_block[i].vram_start);
			memcpy_toio(dst_addr, src_addr, gb_block[i].tmp_size);
			kvfree(src_addr);
			gb_block[i].tmp_size = 0;
		}
	}
	count = gb_dump_cb.count;
	gb_block = gb_dump_cb.gb_vram_blk_list;
	gb_printf(KERN_INFO, "[%s] count %d", __func__, count);

	if (count <= 0 || gb_block == NULL)
	       return 0;

	GB02FUNC242(gb_dump_cb.dev, gb_dump_cb.gb_dev, gb_dump_cb.cpage, 
		gb_dump_cb.gpu_phys, gb_dump_cb.pagetotal, 0);
	kvfree(gb_block);
	vfree(gb_dump_cb.cpu_va);	
	kvfree(gb_dump_cb.cpage);
	kvfree(gb_dump_cb.gpu_phys);
	gb_dump_cb.gb_vram_blk_list = NULL;
	gb_dump_cb.count = 0;
	gb_printf(KERN_INFO, "[%s] restore vram data done\n", __func__);
	return 0;
}


int GB02FUNC293(int flag)
{
	if (flag)
		GB02FUNC257(gb_dump_cb.gb_dev->ddev, gb_dump_cb.gb_dev);
	else
		GB02FUNC285();
	return 0;
}
int GB02FUNC294(u64 gb_addr, u64 size, int block_index)
{
	struct gb_vram_blk_info *gb_key;
	gb_key = gb_dump_cb.gb_key_block;
	if (block_index >= GB_BLOCK_MAX) {
		gb_printf(KERN_INFO, "[%s] register dump block %d defeat\n",
			__func__, block_index);
		return -1;
	}
	gb_key[block_index].vram_start = gb_addr;
	gb_key[block_index].size = size;
	gb_printf(KERN_INFO, "[%s] Block %d addr %llx size:%llx cpu va %llx\n",
		__func__, block_index, gb_addr, size,
		(u64)gb_key[block_index].gb_data);
	return 0;
}

int GB02FUNC296(struct GB02STR39 *gb_dev)
{
	int ddr_bar_id;
	struct gb_vram_blk_info *gb_key;
	memset(&gb_dump_cb, 0, sizeof(struct gb_mem_dump));

	gb_dump_cb.gb_dev = gb_dev;
	gb_key = gb_dump_cb.gb_key_block;
	ddr_bar_id = GB02FUNC468(gb_dev->gb_pcie->GB02STR153);
	gb_dump_cb.vram_base_addr = gb_dev->gb_pcie->pci_bars[ddr_bar_id].mmio;
	gb_dump_cb.max_buffer = GB02MAC333;
	mutex_init(&gb_dump_cb.file_lock);
	mutex_init(&gb_dump_cb.gb_block_dump_lock);

	gb_key[GB_BLOCK_MMU].vram_start = GB02MAC505;
	gb_key[GB_BLOCK_MMU].size = GB02MAC493;
	atomic_set(&gb_dump_cb.bo_count, 0);
	atomic_set(&gb_dump_cb.vbo_count, 0);
	return 0;
}
#endif
