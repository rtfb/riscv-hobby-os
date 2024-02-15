#include "vm.h"

#if !CONFIG_MMU

void* make_kernel_page_table(page_t *pages, int num_pages) {}
void clear_page_table(void *page) {}
void init_user_page_table(void *pagetable, uint32_t pid, void *kpagetable) {}
void free_page_table(regsize_t *pt) {}
void map_page_sv39(regsize_t *pagetable, void *phys_addr, regsize_t virt_addr, int perm, uint32_t pid) {}
void map_range(void *pagetable, void *pa_start, void *pa_end, void *va_start, int perm, uint32_t pid) {}
void map_range_id(void *pagetable, void *pa_start, void *pa_end, int perm) {}
void map_page_id(void *pagetable, void *pa, int perm, int pid) {}
void copy_page_table(regsize_t *dst, regsize_t *src, uint32_t pid) {}
void copy_kernel_pagemap(regsize_t *upt, regsize_t *kpt) {}
regsize_t* find_next_level_page_table(regsize_t *pagetable) {}
void* va2pa(regsize_t *pagetable, void *va) { return va; }

#endif // if !CONFIG_MMU
