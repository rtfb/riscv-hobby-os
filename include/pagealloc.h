#ifndef _PAGEALLOC_H_
#define _PAGEALLOC_H_

#include "spinlock.h"

#define PAGE_SIZE           512 // bytes

// Let's hardcode it for now. Make it small enough to fit in HiFive1 (32 pages
// * 512 bytes = 16k RAM).
#define MAX_PAGES           32

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
} paged_mem_t;

void init_paged_memory(void* paged_mem_end);
void* allocate_page();
void release_page(void *ptr);
void copy_page(void* dst, void* src);

#endif // ifndef _PAGEALLOC_H_
