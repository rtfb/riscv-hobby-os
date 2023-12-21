#ifndef _PAGEALLOC_H_
#define _PAGEALLOC_H_

#include "spinlock.h"
#include "pmp.h"
#include "riscv.h"

#define PAGE_FREE           0
#define PAGE_ALLOCATED      1

// Describes a single page of memory. Has a pointer to the actual piece of
// memory and flags with the status.
typedef struct page_s {
    void *ptr;
    uint32_t flags;
} page_t;

// Contains all pages. Lock should be acquired to modify anything in this
// struct.
typedef struct paged_mem_s {
    spinlock lock;
    page_t pages[MAX_PAGES];
    uint32_t num_pages;

    pmp_config_t pmp_config;

    // the region of unclaimed memory between stack_top and the first page
    regsize_t unclaimed_start;
    regsize_t unclaimed_end;
} paged_mem_t;

// defined in pagealloc.c
extern paged_mem_t paged_memory;

void init_paged_memory(void* paged_mem_end, int do_page_report);
void* allocate_page();
void release_page(void *ptr);
uint32_t count_free_pages();
void copy_page(void* dst, void* src);
uint32_t isolate_page(void *page);
uint32_t unisolate_page(int pmp_index);

#endif // ifndef _PAGEALLOC_H_
