#include "kernel.h"
#include "pagealloc.h"
#include "pmp.h"
#include "riscv.h"
#include "vm.h"

paged_mem_t paged_memory;

void init_paged_memory(void* paged_mem_end) {
    paged_memory.lock = 0;
    // up-align to page size to make all pages naturally aligned:
    regsize_t mem = PAGE_ROUND_UP(&stack_top);
    paged_memory.unclaimed_start = (regsize_t)&stack_top;
    paged_memory.unclaimed_end = mem;
    int i = 0;
    while (mem < (regsize_t)paged_mem_end && i < MAX_PAGES) {
        page_t *p = &paged_memory.pages[i];
        p->ptr = (void*)mem;
        p->flags = PAGE_FREE;
        p->site = 0;
        p->pid = -1;
        mem += PAGE_SIZE;
        i++;
    }
    paged_memory.num_pages = i;
#if CONFIG_MMU
    void *pagetable = make_kernel_page_table(paged_memory.pages, i);
    paged_memory.kpagetable = pagetable;
    regsize_t satp = MAKE_SATP(pagetable);
    paged_memory.ksatp = satp;
    set_satp(satp);
#endif
}

void do_page_report(void* mem_end) {
    regsize_t mem_start = paged_memory.unclaimed_end;
    kprintf("paged memory: start=%p, end=%p, npages=%d\n",
        mem_start, mem_end, paged_memory.num_pages);
}

void* allocate_page(char const *site, uint32_t pid, uint32_t flags) {
    acquire(&paged_memory.lock);
    for (int i = 0; i < paged_memory.num_pages; i++) {
        page_t* page = &paged_memory.pages[i];
        if (page->flags == PAGE_FREE) {
            page->flags = flags | PAGE_ALLOCATED;
            page->site = site;
            page->pid = pid;
            release(&paged_memory.lock);
            return page->ptr;
        }
    }
    release(&paged_memory.lock);
    return 0;
}

// kalloc is a convenience shorthand for allocating a page for kernel needs.
void *kalloc(char const *site, uint32_t pid) {
    return allocate_page(site, pid, 0);
}

void release_page(void *ptr) {
    acquire(&paged_memory.lock);
    for (int i = 0; i < paged_memory.num_pages; i++) {
        page_t* page = &paged_memory.pages[i];
        if (page->ptr == ptr) {
            if (page->flags == PAGE_FREE) {
                release(&paged_memory.lock);
                // TODO: panic here: release of an unallocated page
                return;
            }
            page->flags = PAGE_FREE;
            page->site = 0;
            page->pid = -1;
            release(&paged_memory.lock);
            return;
        }
    }
    release(&paged_memory.lock);
    // TODO: panic here: can't find such page
}

uint32_t count_free_pages() {
    uint32_t num = 0;
    for (int i = 0; i < paged_memory.num_pages; i++) {
        page_t* page = &paged_memory.pages[i];
        if (page->flags == PAGE_FREE) {
            num++;
        }
    }
    return num;
}

void copy_page(void* dst, void* src) {
    regsize_t* pdst = (regsize_t*)dst;
    regsize_t* psrc = (regsize_t*)src;
    for (int i = 0; i < PAGE_SIZE/sizeof(regsize_t); i++) {
        *pdst++ = *psrc++;
    }
}
