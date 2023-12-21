#include "pagealloc.h"
#include "kernel.h"

paged_mem_t paged_memory;

void init_paged_memory(void* paged_mem_end, int do_page_report) {
    regsize_t unclaimed_start = (regsize_t)&stack_top;
    paged_memory.lock = 0;
    regsize_t mem = unclaimed_start;
    // up-align at page size to avoid the last page being incomplete:
    mem &= ~(PAGE_SIZE - 1);
    if (unclaimed_start > mem) {
        mem += PAGE_SIZE;
    }
    regsize_t paged_mem_start = mem;
    paged_memory.unclaimed_start = unclaimed_start;
    paged_memory.unclaimed_end = paged_mem_start;
    int i = 0;
    while (mem < (regsize_t)paged_mem_end && i < MAX_PAGES) {
        paged_memory.pages[i].ptr = (void*)mem;
        paged_memory.pages[i].flags = PAGE_FREE;
        mem += PAGE_SIZE;
        i++;
    }
    init_pmp_config(&paged_memory.pmp_config, (void*)paged_mem_start);
    apply_pmp_config(&paged_memory.pmp_config);
    paged_memory.num_pages = i;
    if (do_page_report) {
        kprintf("paged memory: start=%p, end=%p, npages=%d\n",
                paged_mem_start, paged_mem_end, paged_memory.num_pages);
    }
}

void* allocate_page() {
    acquire(&paged_memory.lock);
    for (int i = 0; i < paged_memory.num_pages; i++) {
        page_t* page = &paged_memory.pages[i];
        if (page->flags == PAGE_FREE) {
            page->flags = PAGE_ALLOCATED;
            release(&paged_memory.lock);
            return page->ptr;
        }
    }
    release(&paged_memory.lock);
    return 0;
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

uint32_t isolate_page(void *page) {
    int pmp_index = find_free_pmp_slot(&paged_memory.pmp_config);
    if (pmp_index == -1) {
        return -1;
    }
    // set_page_pmp_perms(&paged_memory.pmp_config, page, pmp_index, 0);
    set_page_pmp_perms2(&paged_memory.pmp_config, page, pmp_index, 0);
    // fence_i();
    return pmp_index;
}

uint32_t unisolate_page(int pmp_index) {
    reset_page_pmp_perms(&paged_memory.pmp_config, pmp_index);
    fence_i();
    return 0;
}
