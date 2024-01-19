#ifndef _PAGEALLOC_H_
#define _PAGEALLOC_H_

#include "proc.h"
#include "spinlock.h"

// Let's hardcode it for now. Make it small enough to fit in HiFive1 (32 pages
// * 512 bytes = 16k RAM).
#define MAX_PAGES           32

#define PAGE_FREE           0
#define PAGE_ALLOCATED      1
#define PAGE_USERMEM        2

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
    spinlock lock;
    page_t pages[MAX_PAGES];
    uint32_t num_pages;

    // the region of unclaimed memory between stack_top and the first page
    regsize_t unclaimed_start;
    regsize_t unclaimed_end;
} paged_mem_t;

// defined in pagealloc.c
extern paged_mem_t paged_memory;

void init_paged_memory(void* paged_mem_end, int do_page_report);
void init_vpt();
void clear_vpt(void *page);
void init_user_vpt(process_t *proc);
void free_page_table(regsize_t *pt);
void map_page(regsize_t *pagetable, void *phys_addr, int perm);
void map_l2page(regsize_t *pagetable, regsize_t phys_addr, int perm);
void map_superpage(regsize_t *pagetable, void *phys_addr, int perm);
void map_page_sv39(regsize_t *pagetable, void *phys_addr, regsize_t virt_addr, int perm, uint32_t pid);
void* allocate_page(char const *site, uint32_t pid, uint32_t flags);
void* kalloc(char const *site, uint32_t pid);
void release_page(void *ptr);
uint32_t count_free_pages();
void copy_page(void* dst, void* src);
void copy_page_table(regsize_t *dst, regsize_t *src, uint32_t pid);
void copy_page_table2(regsize_t *dst, regsize_t *src, uint32_t pid);
regsize_t* find_next_level_page_table(regsize_t *pagetable);

#endif // ifndef _PAGEALLOC_H_
