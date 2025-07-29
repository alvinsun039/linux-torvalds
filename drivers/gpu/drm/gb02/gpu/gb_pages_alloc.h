#ifndef GB_PAGE_ALLOCS_H_
#define GB02MAC1324

int GB02FUNC1111(struct GB02STR39 *gbdev, size_t nr_pages, phys_addr_t *pages);
void GB02FUNC1119(struct GB02STR125 *GB02STR125, struct GB02STR39 *gbdev);

struct GB02STR125 *GB02FUNC1123(struct GB02STR39 *gbdev);
void GB02FUNC1130(struct GB02STR125 *GB02STR125, struct GB02STR39 *gbdev);

int GB02FUNC1132(struct GB02STR39 *gbdev, size_t nr_pages, phys_addr_t *pages);
void GB02FUNC1133(struct GB02STR39 *gbdev, size_t nr_pages, phys_addr_t *pages);

struct GB02STR125 *GB02FUNC1104(struct GB02STR39 *gbdev, phys_addr_t phys);
phys_addr_t GB02FUNC1102(struct GB02STR125 *p);

u64 *GB02FUNC1107(struct GB02STR125 *p);

#endif
