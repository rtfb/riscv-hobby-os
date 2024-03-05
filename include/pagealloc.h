#ifndef _PAGEALLOC_H_
#define _PAGEALLOC_H_

#include "spinlock.h"

// Let's hardcode it for now. Make it small enough to fit in HiFive1 (32 pages
// * 512 bytes = 16k RAM). On machines with MMU we need more since a lot of
// pages get consumed for pagetable bookkeeping.
#if CONFIG_MMU
#define MAX_PAGES           48
#else
#define MAX_PAGES           32
#endif

#define PAGE_FREE           0
#define PAGE_ALLOCATED      1
#define PAGE_USERMEM        2

#define PAGE_ROUND_DOWN(p)  ((regsize_t)(p) & ~(PAGE_SIZE-1))
#define PAGE_ROUND_UP(p)    (PAGE_ROUND_DOWN(p) + PAGE_SIZE)

// Describes a single page of memory. Has a pointer to the actual piece of
// memory and flags with the status.
typedef struct page_s {
    void *ptr;
    uint32_t flags;
    char const *site;   // allocation site
    uint32_t pid;       // if the page is associated with a user process, this holds the process pid
} page_t;

// Contains all pages. Lock should be acquired to modify anything in this
// struct.
typedef struct paged_mem_s {
#if CONFIG_MMU
    regsize_t ksatp;
    void *kpagetable;
#endif

    spinlock lock;
    page_t pages[MAX_PAGES];
    uint32_t num_pages;

    // the region of unclaimed memory between stack_top_addr and the first page
    regsize_t unclaimed_start;
    regsize_t unclaimed_end;
} paged_mem_t;

// defined in pagealloc.c
extern paged_mem_t paged_memory;

void init_paged_memory(void* paged_mem_end);
void do_page_report(void* paged_mem_end);
void* allocate_page(char const *site, uint32_t pid, uint32_t flags);
void* kalloc(char const *site, uint32_t pid);
void release_page(void *ptr);
uint32_t count_free_pages();
void copy_page(void* dst, void* src);

#endif // ifndef _PAGEALLOC_H_
