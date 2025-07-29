---
title: GenBu KMD based DRM 软件设计说明书
keywords: GenBu, Gallium
author: alvin@sietium.com
date: 2021-09-10
CJKmainfont: AR PL KaitiM GB
CJKoptions: AutoFakeBold
linkcolors: true
linkcolor: blue
subparagraph: true
toc: true
toc-depth: 4
margin-left: 1in 
margin-right: 1in 
margin-top: 1.5in
margin-bottom: 1in 
header-includes:
    - \usepackage[margins=raggedright]{floatrow}
    - \usepackage{titlesec}
    - \newcommand{\sectionbreak}{\clearpage}
    - \usepackage{enumitem}
    - \usepackage{amsfonts}
    - \setlist[itemize,1]{label=$\bullet$}
    - \setlist[itemize,2]{label=$\diamond$}
    - \let\oldparagraph\paragraph
    - \renewcommand{\paragraph}[1]{\oldparagraph{#1}\mbox{}}
    - \let\oldsubparagraph\subparagraph
    - \renewcommand{\subparagraph}[1]{\oldsubparagraph{#1}\mbox{}}
---

# 缩略词
| 缩写                                        | 解释                                        |
|:--------------------------------------------|:--------------------------------------------|
| BO                                          | Buffer Object                                   |
| DRI                                          | Direct Rendering Infrastructure             |
| DRM                                        | Direct Rendering Manager                    |
| GB                                           | GenBu                                       |
| GEM                                         | Graphics Execution Manager                  |
| SC                                            | Stage Chain                                   |
| SD                                            | Stage Descriptor                              |
| KMD                                         | Kernel Mode Driver                          |
| Sync                                        | Synchronization                             |
| KMS                                         | Kernel Mode Setting                     |

# DRM驱动框架模型
-   DRM<Direct Rendering Manager>是Linux目前主流的图形处理器驱动框架，刚开始主要负责显示部分，是代替传统的FB架构，DRM更能适应当前日益更新的图形处理器GPU。比如FB原生不支持多层合成，不支持VSYNC，不支持DMA-BUF，不支持异步更新，不支持fence机制等等，而这些功能DRM原生都支持。后来发展越来越快，不尽管理FB，先提供了一套通用接口管理GPU渲染任务。当前主流的GPU都有对应的DRM显卡驱动，而且对于AMD显卡，它的基于DRM的开源驱动正成为自己的主流驱动。
- DRM驱动将图形处理和显示控制分开，显示控制功能集中在KMS<Kernel Mode Setting>模块下。

## DRM公共接口
DRM的每个功能接口都对应一个feature开关，所有的feature都在drm_driver_feature这个枚举变量中：
```c
/**
 * enum drm_driver_feature - feature flags
 *
 * See &drm_driver.driver_features, drm_device.driver_features and
 * drm_core_check_feature().
 */
enum drm_driver_feature {
        /**
         * @DRIVER_GEM:
         *
         * Driver use the GEM memory manager. This should be set for all modern
         * drivers.
         */
        DRIVER_GEM                      = BIT(0),
        /**
         * @DRIVER_MODESET:
         *
         * Driver supports mode setting interfaces (KMS).
         */
        DRIVER_MODESET                  = BIT(1),
        /**
         * @DRIVER_RENDER:
         *
         * Driver supports dedicated render nodes. See also the :ref:`section on
         * render nodes <drm_render_node>` for details.
         */
        DRIVER_RENDER                   = BIT(3),
        /**
         * @DRIVER_ATOMIC:
         *
         * Driver supports the full atomic modesetting userspace API. Drivers
         * which only use atomic internally, but do not the support the full
         * userspace API (e.g. not all properties converted to atomic, or
         * multi-plane updates are not guaranteed to be tear-free) should not
         * set this flag.
         */
        DRIVER_ATOMIC                   = BIT(4),
        /**
         * @DRIVER_SYNCOBJ:
         *
         * Driver supports &drm_syncobj for explicit synchronization of command
         * submission.
         */
        DRIVER_SYNCOBJ                  = BIT(5),
        /**
         * @DRIVER_SYNCOBJ_TIMELINE:
         *
         * Driver supports the timeline flavor of &drm_syncobj for explicit
         * synchronization of command submission.
		 */
        DRIVER_SYNCOBJ_TIMELINE         = BIT(6),

        /* IMPORTANT: Below are all the legacy flags, add new ones above. */

        /**
         * @DRIVER_USE_AGP:
         *
         * Set up DRM AGP support, see drm_agp_init(), the DRM core will manage
         * AGP resources. New drivers don't need this.
         */
        DRIVER_USE_AGP                  = BIT(25),
        /**
         * @DRIVER_LEGACY:
         *
         * Denote a legacy driver using shadow attach. Do not use.
         */
        DRIVER_LEGACY                   = BIT(26),
        /**
         * @DRIVER_PCI_DMA:
         *
         * Driver is capable of PCI DMA, mapping of PCI DMA buffers to userspace
         * will be enabled. Only for legacy drivers. Do not use.
         */
        DRIVER_PCI_DMA                  = BIT(27),
        /**
         * @DRIVER_SG:
         *
         * Driver can perform scatter/gather DMA, allocation and mapping of
         * scatter/gather buffers will be enabled. Only for legacy drivers. Do
         * not use.
         */
        DRIVER_SG                       = BIT(28),

        /**
         * @DRIVER_HAVE_DMA:
         *
         * Driver supports DMA, the userspace DMA API will be supported. Only
         * for legacy drivers. Do not use.
         */
        DRIVER_HAVE_DMA                 = BIT(29),
        /**
         * @DRIVER_HAVE_IRQ:
         *
         * Legacy irq support. Only for legacy drivers. Do not use.
         *
         * New drivers can either use the drm_irq_install() and
         * drm_irq_uninstall() helper functions, or roll their own irq support
         * code by calling request_irq() directly.
         */
        DRIVER_HAVE_IRQ                 = BIT(30),
        /**
         * @DRIVER_KMS_LEGACY_CONTEXT:
         *
         * Used only by nouveau for backwards compatibility with existing
         * userspace.  Do not use.
         */
        DRIVER_KMS_LEGACY_CONTEXT       = BIT(31),
};

```
每一个drm驱动都要实例化一个 drm_driver 结构体，其中 .driver_features 成员就用于配置各种feature的接口。在实现GB DRM驱动时打开了DRIVER_RENDER、DRIVER_GEM和DRIVER_SYNCOBJ三个功能：

```c
static struct drm_driver gb_drm_driver = { 
        .driver_features        = DRIVER_RENDER | DRIVER_GEM | DRIVER_SYNCOBJ,
        .open                   = gb_open,
        .postclose              = gb_postclose,
        .ioctls                 = gb_drm_driver_ioctls,
        .num_ioctls             = ARRAY_SIZE(gb_drm_driver_ioctls),
        .fops                   = &gb_drm_driver_fops,

        .gem_create_object      = gb_gem_create_object,
        .gem_free_object_unlocked = gb_gem_free_object,
        .gem_vm_ops             = &gb_vm_ops,
        .gem_open_object        = gb_gem_open,
        .gem_close_object       = gb_gem_close,

        .prime_handle_to_fd = drm_gem_prime_handle_to_fd,
        .prime_fd_to_handle = drm_gem_prime_fd_to_handle,
        .name = DRIVER_NAME,
        .desc = DRIVER_DESC,
        .date = DRIVER_DATE,
        .major = DRIVER_MAJOR,
        .minor = DRIVER_MINOR,
        .patchlevel = DRIVER_PATCHLEVEL,
};
```
 - DRIVER_RENDER
 打开DRIVER_RENDER后，驱动会生成 /dev/dri/renderDXXX 文件

 - DRIVER_GEM
只有打开DRIVER_GEM feature，GEM提供的接口才能使用，如下面三个接口就受DRIVER_GEM 控制。
```c
        DRM_IOCTL_DEF(DRM_IOCTL_GEM_CLOSE, drm_gem_close_ioctl, DRM_RENDER_ALLOW),
        DRM_IOCTL_DEF(DRM_IOCTL_GEM_FLINK, drm_gem_flink_ioctl, DRM_AUTH),
        DRM_IOCTL_DEF(DRM_IOCTL_GEM_OPEN, drm_gem_open_ioctl, DRM_AUTH),

```

 - DRIVER_SYNOBJ
该feature控制着drm syncobj 功能，打开后能使用drm syncobj 供drm驱动进行同步操作，如下面的ioctl受该feature的控制。
```c
        DRM_IOCTL_DEF(DRM_IOCTL_SYNCOBJ_CREATE, drm_syncobj_create_ioctl,
                      DRM_RENDER_ALLOW), 
        DRM_IOCTL_DEF(DRM_IOCTL_SYNCOBJ_DESTROY, drm_syncobj_destroy_ioctl,
                      DRM_RENDER_ALLOW),
        DRM_IOCTL_DEF(DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, drm_syncobj_handle_to_fd_ioctl,
                      DRM_RENDER_ALLOW), 
        DRM_IOCTL_DEF(DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, drm_syncobj_fd_to_handle_ioctl,
         ...
```

## KMS
要在DRM驱动中使用KMS对应的接口，必须设置drm_driver.driver_features中包含DRIVER_MODESET。
本文不对KMS详细介绍，这里只重点介绍几个KMS的概念。

### DRM Framebuffer
 - 它是一块内存区域，可以理解为一块画布，驱动和应用层都能访问它。绘制前需要将它格式化，设定绘制的色彩模式（例如RGB24，YUV 等）和画布的大小（分辨率）。

### CRTC
阴极摄像管上下文。这个看名字很很难懂，但简单的来说他就是显示输出的上下文，可以理解为扫描仪。CRTC对内连接 Framebuffer 地址，对外连接 Encoder，会扫描 Framebuffer 上的内容，叠加上 Planes 的内容，最后传给Encoder。

### Planes
简单理解就是画布。它和 Framebuffer 一样是内存地址。它的作用是干什么呢？打个比方，在电脑上，一边打字聊微信一边看电影，这里对立出来两个概念，打字是文字交互，是小范围更新的 Graphics 模式；看电影是全幅高速更新的 Video 模式，这两种模式将显卡的使用拉上了两个极端。

这时Planes就发挥了很好的作用，它给 Video 刷新提供了高速通道，使 Video 单独为一个图层，可以叠加在 Graphic 上或之下，并具有缩放等功能。

Planes 是可以有多个的，相当于图层叠加，因此扫描仪（CRTC）扫描的图像实际上往往是 Framebuffer 和 Planes 的组合（Blending）。

### Encoder
编码器。它的作用就是将内存的 pixel 像素编码（转换）为显示器所需要的信号。简单理解就是，如果需要将画面显示到不同的设备（Display Device）上，需要将画面转化为不同的电信号，例如 DVID、VGA、YPbPr、CVBS、Mipi、eDP 等。

Encoder 和 CRTC 之间的交互就是我们所说的 ModeSetting，其中包含了前面提到的色彩模式、还有时序（Timing）等。

### Connector
连接器。它常常对应于物理连接器 (例如 VGA, DVI, FPD-Link, HDMI, DisplayPort, S-Video等) ，它不是指物理线，在 DRM中，Connector 是一个抽象的数据结构，代表连接的显示设备，从Connector中可以得到当前物理连接的输出设备相关的信息 ，例如，连接状态，EDID数据，DPMS状态、支持的视频模式等。


# 显存管理 GEM
 - 基于DRM框架的存储管理目前都采用GEM接口，GEM（Graphics Execution Manager）是Linux DRM中用于完成memory 管理的内核基础设施。
- GEM作为一种内存管理方式，并未覆盖各种在userspace和kernel使用情况（use cases）。
 - GEM提供了一组标准的内存相关的操作给userspace，以及一组辅助函数给kernel drivers， kernel  drivers还需要实现一些硬件相关的私有操作函数。
 - GEM所管理的memory具体类型、属性是不可知的，我们并不知道它所管理的buffer对象包含了什么。如果要获知GEM所管理的buffer对象的具体内容和使用目的，需要kernel drivers自己实现一组私有的ioctl来获取对应的信息。

 - GEM对userspace的接口基本一致，在drm driver注册时需配置drm_driver.driver_features中包含DRIVER_GEM feature。一般用户态程序就可以使用GEM提供的公共接口使用GEM BO。

根据硬件及体系结构的不同，GEM的后端有多种实现方式，总共有以下几种，Linux内核同时也提供了不同种类的辅助函数：

 * TTM（Translation Table Manager）
底层的实现方式基于TTM实现，详细内容参见内网wiki TTM介绍
* VRAM
一种独立显卡形式的实现，底层实现方式还是基于TTM机制
```c
struct drm_gem_vram_object {
  struct ttm_buffer_object bo;
  struct dma_buf_map map;
  unsigned int vmap_use_count;
  struct ttm_placement placement;
  struct ttm_place placements[2];
};
```
 * shmem (Share Memory) 是基于内核shmem实现，和系统内存的分配、映射、回收等一样，适用于显存也来自于主存的GPU，一般的SoC设备使用此种方式管理显存。
```c
struct drm_gem_shmem_object {
  struct drm_gem_object base;
  struct mutex pages_lock;
  struct page **pages;
  unsigned int pages_use_count;
  int madv;
  struct list_head madv_list;
  unsigned int pages_mark_dirty_on_put    : 1;
  unsigned int pages_mark_accessed_on_put : 1;
  struct sg_table *sgt;
  struct mutex vmap_lock;
  void *vaddr;
  unsigned int vmap_use_count;
  bool map_wc;
};
```
内核还为其设置好了 GEM buffer object的辅助函数
```c
#define DEFINE_DRM_GEM_SHMEM_FOPS(name) \
        static const struct file_operations name = {\
                .owner          = THIS_MODULE,\
                .open           = drm_open,\
                .release        = drm_release,\
                .unlocked_ioctl = drm_ioctl,\
                .compat_ioctl   = drm_compat_ioctl,\
                .poll           = drm_poll,\
                .read           = drm_read,\
                .llseek         = noop_llseek,\
                .mmap           = drm_gem_shmem_mmap, \
        }
```
 * GEM CMA (Contiguous Memory Allocator) 
该实现方式基于内核启动时预留一段内存池，主要用于分配大块的、连续的物理内存方式。
```c
struct drm_gem_cma_object {
  struct drm_gem_object base;
  dma_addr_t paddr;
  struct sg_table *sgt;
  void *vaddr;
  bool map_noncoherent;
};

#define DRM_GEM_CMA_VMAP_DRIVER_OPS \
        .gem_create_object      = drm_cma_gem_create_object_default_funcs, \
        .dumb_create            = drm_gem_cma_dumb_create, \
        .prime_handle_to_fd     = drm_gem_prime_handle_to_fd, \
        .prime_fd_to_handle     = drm_gem_prime_fd_to_handle, \
        .gem_prime_import_sg_table = drm_gem_cma_prime_import_sg_table_vmap, \
        .gem_prime_mmap         = drm_gem_prime_mmap

```

由于GenBu01显卡在显存管理上没有设计Gart机制，针对使用与独立显卡的TTM管理方式，存在改动基础内核代码的风险，因此没做过多的研究。

根据上述几种GEM后端存储的实现方式，当前GenBu01显卡的显存形式更接近CMA方式，同时shmem的操作更灵活，KMD老的实现方式是基于same va的页式管理，和shmem的 页式虚拟地址映射类似，因此GenBu01的GEM bo实现设计参考了基于CMA和shmem两种机制的部分内容。

下文我们就来介绍GenBu01的实现方式。

## GenBu01 GEM BO实现

Gem buffer object是一块buffer的描述，GenBu01有1GB的显存，其中一部分用于显示控制模块使用的Frame buffer，剩下的用于显存数据存放和页表内容存放。底层的显存管理仍然选择kmd中的页式管理，由于GPU中的MMU识别的页式大小为4KB，因此对整个1G空间（除去FB）进行了4KB的页式管理
```c
/* 显存的页式管理单元 */
struct gb_page {
        u64 gpu_phy;
        struct list_head lru;
        volatile u32 pfn;
        struct gb_device *gbdev;
};
```
对外提供的存储管理接口如下：

```c
/* 页式分配函数，每次可以分配多个页面，每个页面不保证连续 */
int gb_alloc_pages(struct gb_device *gbdev, size_t nr_pages, phys_addr_t *pages);
/* 页式释放函数 */
void gb_free_pages(struct gb_page *gb_page, struct gb_device *gbdev);

/* 分配用于存放mmu页表的函数，和gb_alloc_pages基本一致，该函数的产生主要解决一些体系结构的页面大小不是4KB的情况 */
struct gb_page *gb_alloc_mmu_page(struct gb_device *gbdev);
/* mmu页的释放函数 */
void gb_free_mmu_page(struct gb_page *gb_page, struct gb_device *gbdev);

/* 显存分配函数的封装 */
int gb_vram_alloc_pages(struct gb_device *gbdev, size_t nr_pages, phys_addr_t *pages);
/* 显存释放函数 */
void gb_vram_free_pages(struct gb_device *gbdev, size_t nr_pages, phys_addr_t *pages);

/* 映射当前显存页，可用于CPU在内核态访问该页内容 */
u64 *gb_kmap(struct gb_page *p);

/* 通过一个页的物理地址获取这个页的结构体，也就是该页的描述符 */
struct gb_page *gb_phys_to_page(struct gb_device *gbdev, phys_addr_t phys);
/* 通过一个页式结构获取该页对应的显存物理地址 */
phys_addr_t gb_page_to_phys(struct gb_page *p);

```
### GenBu01 BO的实现与用法
为了更方便的使用GEM提供的基础BO drm_gem_object，GenBu01驱动中设计了两个数据来进一步修饰：
```c
struct drm_gem_gbmem_object {
        struct drm_gem_object base;
        phys_addr_t *pages;  // physical address
        int nr_pages;
        phys_addr_t *heap_pages;
        int heap_nr_pages;
        u64 heap_vpfn;
        struct mutex pages_lock;

        struct sg_table *sgt;
        u64 iovaddr;
};

struct gb_gem_object {
        struct drm_gem_gbmem_object base;

        struct gb_device *gbdev;
        struct gb_file_priv *private_data;
        struct gb_mmu *mmu;
        struct drm_mm_node node;
        bool is_mapped          :1;
        bool noexec             :1;
        bool is_heap            :1;
};

```
 - 数据结构drm_gem_gbmem_object 参考了 drm_gem_cma_object 和 drm_gem_shmem_object 这两个系统原生的数据结构。该结构主要用来描述显存信息。
 - 数据结构 gb_gem_object 是 drm_gem_gbmem_object的二次封装，包含了设备信息，方便bo在任何时候可获取设备访问接口。

####  BO Create
主要功能函数为：
```c
static struct genbu_bo *
genbu_bo_alloc(struct genbu_device *dev, size_t size,
                  uint32_t flags, const char *label)

```
该函数为调用CREATE_BO ioctl，内核态的实现函数为
```c
static int gb_ioctl_create_bo(struct drm_device *dev, void *data,
                struct drm_file *file)
```
gb_ioctl_create_bo函数与用户态的接口数据都保存在 drm_gb_create_bo结构体中 

```c
/**
 * 此drmIoctl参数用来向内核申请显存块BO, 但此时的BO还未mmap，所以不能被CPU访问
 *
 */
struct drm_gb_create_bo {
    /** 申请的BO的大小，以字节计, 为了减少内存碎片，实际上申请的大小都是页对齐的 */  
        __u32 size;
    /** BO的flags */
        __u32 flags;
        /** 由内核返回的GEM handle */
        __u32 handle;
    /** 填充0，对齐该结构体 */  
        __u32 pad;
        /** 由内核返回的这个BO的GPU VA, 由UMD填入Stage描述符相应的地址字段 */  
        __u64 offset;
};
```
gb_ioctl_create_bo 会创建对应的drm_gem_bo, 该bo会逐层被 drm_gem_gbmem_object 和 gb_mem_object 包含。

一般情况，在创建bo的时候会调用 gb_vram_alloc_pages 函数为BO分配实际的显存空间，但
用户态程序有些bo不用被用户程序访问，这类bo会设置drm_gb_create_bo.flags为GB_BO_HEAP，表明该BO是一段HEAP区域，GPU只有在渲染过程中才会访问这段区域，针对此BO，不会调用gb_vram_alloc_pages分配实际的物理显存。

gb_ioctl_create_bo 在执行过程中会通过调用 drm_driver中的gem_open_object的钩子函数，该函数对应的实现为gb_gem_open, 在执行该函数时，bo的分配已经完成，此时通过drm_mm_insert_node_generic 完成 drm_mm_node的初始化，该node的区间为该bo在drm mm中获取的虚拟地址空间，该空间和vma虚拟地址不同。而node->start作为该bo的设备虚拟地址。通过调用gb_gpu_mmap 完成设备虚拟地址到显存物理地址的映射，此时，该区域就可以被GPU访问。

#### BO Map
 - 首先用户态mesa程序通过genbu_bo_mmap实现，该函数对应 MMAP_BO ioctl 接口函数实现，该接口的主要实现函数为 gb_ioctl_mmap_bo，该函数主要返回给用户一个映射偏移地址 offset, 供mmap函数实现。当前该offset 暂时没有赋予实际意义，后面可作为用户态与内核态交互的媒介。

 - 其次，mmap系统调用对应的内核态实现函数为 gb_mmap 通过 remap_pfn_range 函数实现了CPU访问 bo对应的物理显存的映射（值得注意的是，如果bo有HEAP标签，将不能调用mmap，因为用户不用访问这块区间）

#### BO Free
 - BO的释放在用户态程序 genbu_bo_free函数中实现，该函数会调用 BO_FREE ioctl 来释放bo，对应的内核态的钩子函数为gb_gem_close, 该函数首先会通过调用 gb_vram_free_pages 释放实际分配到的物理显存，接下来通过gb_gpu_munmap函数完成对应显存到虚拟地址的映射关系的释放。

# GPU任务调度
- GenBu01 GPU硬件有三个作业槽 slot0、slot1、slot2，驱动可将每一个作业放入具体的某个作业槽中，启动该作业槽工作，GPU就能执行该作业的渲染任务，而且这三个作业槽可同时工作。
基于GenBu GPU硬件的这一特点，驱动将硬件中的slot与DRM框架中的gpu scheduler进行了一一映射。

首先我们简单了解下drm框架中的GPU scheduler原理：

 - 当前的GPU功能越来强大，但是在强大都需要接收CPU发来的渲染命令，为了更好的与CPU配合，GPU给CPU提供了下发命令流（command stream）接口，这些命令流用于控制GPU硬件，指挥GPU进行特定的渲染任务。
 - linux内核中的GPU scheduler正是用于GPU命令流的调度，这部分代码原本是从AMD GPU driver中独立出来的，在内核版本<待更新>中开始作为DRM框架中的GPU公共调度器。
 - GPU scheduler为用户程序提供了entities，可用于用户程序向其提交stages(GB中的job我们称之为stage job，表示流水线阶段作业)，这些stages先被加入到software queue上，然后再经调度器调度到hardware（GPU）上。
 - GPU上一个命令流通道对应一个GPU scheduler，在GenBu01 GPU上对应一个作业槽 stage slot.
 - GPU scheduler调度策略有两个层级，第一层是按优先级调度，第二层是同等优先级下先入队的先调度，即FIFO模式。GPU scheduler通过回调函数的方法实现不同硬件的jobs提交。
 - 在jobs被提交到硬件之前GPU scheduler提供了依赖项检查特性，只有当jobs的所有依赖项全部可用时，才会被提交到硬件上。
 - GPU上一个命令流通道对应一个gpu scheduler，一个gpu scheduler上包含多个run queue，这些run queue代表了不同的优先级。
 - 当有一个新的job需要提交到GPU上时，先被提交到entities上(这里的entity和进程调度实体类似，是调度器的调度单元)，被提交的entity通过负载均衡算法，确定该entity最终会被调度到的gpu scheduler，并把entity加入到选中的gpu scheduler的run Queue的列表中，等待被调度。

下面我们详细看gpu scheduler中的几个关键数据结构和函数：

## gpu scheduler 初始化函数 drm_sched_init()
```c
int drm_sched_init(struct drm_gpu_scheduler *sched,
                   const struct drm_sched_backend_ops *ops,
                   unsigned hw_submission,
                   unsigned hang_limit,
                   long timeout,
                   const char *name)
```
该函数用于初始化 gpu scheduler，其中:

 * sched: 代表调度器实例
 * ops: 代表调度时的回调函数
 * hw_submission: 代表该调度器命令通道上可同时提交的作业次数
 * hang_limit: 代表
 * timeout: 代表作业多少个时间周期后，任务将被超时处理，并调用ops中的超时回调处理函数
 * name: 代表调度器名，调试时使用

## 调度器的回调函数
```c
struct drm_sched_backend_ops {
        struct dma_fence *(*dependency)(struct drm_sched_job *sched_job,
                                        struct drm_sched_entity *s_entity);
        struct dma_fence *(*run_job)(struct drm_sched_job *sched_job);
        void (*timedout_job)(struct drm_sched_job *sched_job);
        void (*free_job)(struct drm_sched_job *sched_job);
};
```
 * dependency：当一个job被视作下一个调度对象时，会调用该接口。如果该job存在依赖项，需返回一个dma_fence指针，GPU scheduler会在这个返回的dma_fence的callback list中添加唤醒操作，一旦该fence被signal，就能再次唤醒GPU scheduler。如果不存在任何依赖项，则返回NULL。
 * run_job：一旦job的所有依赖项变得可用后，会调用该接口。这个接口主要是实现GPU HW相关的命令提交。该接口成功把命令提交到GPU上后，返回一个dma_fence，gpu scheduler会向这个dma_fence的callback list中添加finish fence唤醒操作，而这个dma_fence一般会在GPU处理完毕该job后被signal。
 * timedout_job：当一个提交到GPU执行时间过长时，该接口会被调用，以触发GPU执行恢复的处理流程。
 * free_job：用做当stage job被处理完毕后的相关资源释放工作。
 
## gpu调度实体初始化函数 drm_sched_entity_init()
```c
int drm_sched_entity_init(struct drm_sched_entity *entity,
                          struct drm_sched_rq **rq_list,
                          unsigned int num_rq_list,
                          atomic_t *guilty)
```

 * entity: 调度实体实例
 * rq_list: 运行队列链表，链接着调度器中可被提交的作业
 * num_rq_list: 运行队列个数
 * guilty: 出错时的计数器

## 作业初始化函數 drm_sched_job_init()
```c
int drm_sched_job_init(struct drm_sched_job *job,
                       struct drm_sched_entity *entity,
                       void *owner)
```
 * job: 调度的任务集，简称作业
 * entity：指定job会被提交到该调度实体
 * owner：调试时使用

本函数会为该job初始化两个dma_fence：scheduled 和finished，当scheduled fence被signaled，表明该job要被发送到GPU上，当finished fence被signaled，表明该job已在gpu上处理完毕。
所以通过这两个fence可以告知外界job当前的状态。

## 提交job drm_sched_entity_push_job
```c
void drm_sched_entity_push_job(struct drm_sched_job *sched_job,
                               struct drm_sched_entity *entity)
```
当一个job被drm_sched_job_init()初始化后，就可以通过函数 drm_sched_entity_push_job()提交到entity的job_queue上了。
如果entity是首次被提交job到其上的job_queue上，该entity会被加入到gpu scheduler的run queue上，并唤醒gpu scheduler上的调度线程。
当调度器调度当前job时，会调用 drm_sched_backend_ops中的 run_job 回调函数.

## GenBu01中的调度实现
  本章详细介绍GenBu01 GPU的调度如何使用 drm gpu scheduler调度器
  
### 基于GenBu01 stage slot的调度器结构体
```c
struct gb_queue_state {
	/* drm 调度器 */
        struct drm_gpu_scheduler sched;

	/* fence 上下文 */
        u64 fence_context;
	/* 每个context 提交的顺序 */
        u64 emit_seqno;
};

struct gb_stage_slot {
	/* 每一个slot 对应一个drm调度器实例 */
        struct gb_queue_state queue[NUM_STAGE_SLOTS];
        spinlock_t stage_lock;
};

```

/* GPU在设备初始化时会初始化NUM_STAGE_SLOTS个 drm_gpu_scheduler 实例 */
```c
for (j = 0; j < NUM_STAGE_SLOTS; j++) {
	drm_sched_init(&ss->queue[j].sched, &gb_sched_ops, 1, 0, msecs_to_jiffies(STAGE_TIMEOUT_MS), "genbu");
}
```

### 用户态的stage作业提交
```c
/**
 * 此drmIoctl参数用来向GPU提交Stage描述符，并请求KMD让GPU执行提交的Stage描述符
 *
 */
struct drm_gb_submit {

	/** 向GPU提交的Stage描述符的首地址，GPU VA */
	__u64 sc;

	/** in_sync变量的用户态地址，由内核调用copy_from_user使用, 它们被用来完成explicit同步 */
	__u64 in_syncs;

	/** in_sync的个数，内核对应dma_fence的个数，目前至多只需要1个 */
	__u32 in_sync_count;

	/** genbu_context->syncobj, 用于调试 */
	__u32 out_sync;

	/** 存放BO handles的用户态地址，由内核调用copy_from_user使用, 它们被用来完成implicit同步 */
	__u64 bo_handles;

	/** 上面bo_handles的个数，实际内存大小乘以4, bo_handle类型是u32 */
	__u32 bo_handle_count;

	/* 使用哪个slot的标记信息 */
	__u32 slot_req;
};
```
 - 该结构体通过 gb_ioctl_submit ioctl来实现，drm_gb_submit 结构体在该函数中被转换为gb_stage结构体，下文会详细介绍gb_stage结构体。

 - gb_stage 结构体，该结构体不仅包含了用户态作业描述符信息，还封装了drm gpu scheduler中的调度作业 drm_sched_job结构体
```c
struct gb_stage {
        /* drm gpu scheduler的作业 */
        struct drm_sched_job base;
	/* 该结构体引用的次数 */
        struct kref refcount;
	/* gb 设备的指针，方便访问设备资源 */
        struct gb_device *gbdev;
	/* 基于文件的私有数据，可被看做进程上下文context */
        struct gb_file_priv *file_priv;

	/* 用户传入的fence，用与stage的依赖检查 */
        struct dma_fence **in_fences;
	/* fence的个数 */
        u32 in_fence_count;

	/* 接受任务完成中断信号的fence，接收到可认为本作业已正常完成 */
        struct dma_fence *done_fence;

	/* 作业描述符的首地址，是GPU读取作业数据的起始地址 */
        __u64 sc;
	/* 使用哪个slot的标记信息 */
        __u32 slot_req;
        __u32 flush_id;

	/* 来自于BO中的隐式fence，用于用户态和内核态的同步 */
        struct dma_fence **implicit_fences;
	/* 本stage job中涉及的所有gem bo */
        struct drm_gem_object **bos;
	/* bo的数量 */
        u32 bo_count;

	/* 用于drm-sched 中作业完成时的同步fence */
        struct dma_fence *render_done_fence;
};

```

首先在open函数中会调用drm_sched_entity_init，为三个stage slots分别初始化三个drm_sched_entity实例。
当前为了更好的将GB中的slot和drm scheduler对应起来，设置了一个调度器中的优先级，并且一次只有一个运行队列能加入到调度实体中，这样达到了 “一个drm 调度器“ “一个运行队列“ “一个调度体” “一个stage slot”的一一对应关系。核心代码如下:

```c
gb_stage_open() {
	...
	for (i = 0; i < NUM_STAGE_SLOTS; i++) {
		/* GB 中的调度优先级目前只有一个，后面根据需求在优化基于优先级的调度策略 */
    		rq = &ss->queue[i].sched.sched_rq[DRM_SCHED_PRIORITY_NORMAL];
		/* 用上面的运行队列初始化一个调度实体，目前设置一个调度实体只能有一个运行队列 */
		drm_sched_entity_init(&gb_priv->sched_entity[i], &rq, 1, NULL);
	}
	...
}
```

函数drm_sched_entity_init()是在open()函数中被调用，这样当多个应用程序调用各自open()函数后，driver会为每个应用程序创建各自的entity。
如上文所述，entity是stage job的提交点，gpu上的一个命令流通道对应一个gpu scheduler，当有多个应用程序同时向同一个gpu命令流通道提交stage job时，stage job先被加入到对应的entity上，再等待gpu scheduler的统一调度。

用户态程序通过ioctl系统调用调用gb_ioctl_submit，该函数会通过调用drm_sched_job_init()函数对 gb_stage中的 drm_sched_job成员即base变量进行初始化，并将该stage job加入到上面已经初始化好的调度实体entity中，接下来在调用 drm_sched_entity_push_job 函数将stage job提交到 gpu scheduler中，等待gpu scheduler的统一调度，主要代码如下：

```c
/* 部分代码 */
int gb_stage_push(struct gb_stage *stage) {
	...
	/* 获取已经初始化好的对应的slot的调度体 */
	struct drm_sched_entity *entity = &stage->file_priv->sched_entity[slot];
	...
	/* 将stage job绑定到调度体中 */
	drm_sched_job_init(&stage->base, entity, NULL);
	/* 提交调度体到系统调度队列中，等待调度, 被调度上时会调run_job回调函数 */
	drm_sched_entity_push_job(&stage->base, entity);
	...
}
```

gpu scheduler在调度过程中主要会通过注册到调度器中的回调函数来执行操作，回调函数如下：
```c
static const struct drm_sched_backend_ops gb_sched_ops = { 
        .dependency = gb_stage_dependency, // 会检查作业的依赖关系
        .run_job = gb_stage_run,           // 作业被调度时的主要操作
        .timedout_job = gb_stage_timedout, // 当作业执行超时时的处理函数
        .free_job = gb_stage_free          // 结束时的释放操作
};
```
## 同步
KMD调度任务同步通过DRM提供的Syncobj和硬件中断实现。syncobj的功能需要drm_driver.drm_driver_feature包含 DRIVER_SYNCOBJ 即可。

# GenBu01硬件资源管理
对于内核驱动而言，硬件资源按存储可划分为显存资源、寄存器资源和中断资源。按模块可划分为了三个主要的硬件模块：PCIe接口模块、GPU模块、STAGE模块和MMU模块。

## 硬件初始化
#### PCIe接口初始化


## 中断子系统
GPU、STAGE、MMU三个硬件模块都各自有自己的中断源，分别为GPU中断、STAGE中断和MMU中断。

### GPU中断

### STAGE中断

### MMU中断

# 参考资料
 - https://www.cnblogs.com/yaongtime/p/14418357.html
 - https://blog.csdn.net/hexiaolong2009/article/details/83720940
