#include "kernel.h"
#include "pagealloc.h"
#include "pmp.h"
#include "proc.h"
#include "riscv.h"
#include "timer.h"

paged_mem_t paged_memory;

void init_paged_memory(void* paged_mem_end, int do_page_report) {
    regsize_t unclaimed_start = (regsize_t)&stack_top;
    paged_memory.lock = 0;
    // up-align at page size to avoid the last page being incomplete:
    regsize_t mem = PAGEROUNDUP(unclaimed_start);
    /*
    mem &= ~(PAGE_SIZE - 1);
    if (unclaimed_start > mem) {
        mem += PAGE_SIZE;
    }
    */
    regsize_t paged_mem_start = mem;
    paged_memory.unclaimed_start = unclaimed_start;
    paged_memory.unclaimed_end = paged_mem_start;
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
    init_vpt();
    if (do_page_report) {
        kprintf("paged memory: start=%p, end=%p, npages=%d\n",
                paged_mem_start, paged_mem_end, paged_memory.num_pages);
    }
}

// defined in kernel.ld:
extern void* RAM_START;
extern void* user_code_start;
extern void* rodata_start;
extern void* data_start;
extern void* _end;

void init_vpt() {
#if !HAS_S_MODE
    return;
#endif
    void *pagetable = kalloc("init_vpt: pagetable", -1);
    if (!pagetable) {
        // TODO: panic
        return;
    }
    paged_memory.kpagetable = pagetable;
    // void *l2 = kalloc("init_vpt: l2", -1);
    // if (!l2) {
    //     release_page(pagetable);
    //     // TODO: panic
    //     return;
    // }
    clear_vpt(pagetable);
    // clear_vpt(l2);

    // regsize_t *pte = (regsize_t*)pagetable;
    // regsize_t *l2pte = (regsize_t*)l2;

    // map level-2 pagetable at 0:
    // pte[0] = (PHYS_TO_PPN(l2pte) << 10) | PERM_NONLEAF;

    // map all of RAM as kernel-executable:
    // map_superpage(pte, &RAM_START, PERM_KCODE);
    void *pa = &RAM_START;
    void *end = &user_code_start;
    while (pa != end) {
        // map_page(proc->uvptl3, pa, PERM_UCODE);
        map_page_sv39(pagetable, pa, (regsize_t)pa, PERM_KCODE, -1);
        pa += PAGE_SIZE;
    }
    // map_page_sv39(pagetable, (void*)CLINT0_BASE_ADDRESS, CLINT0_BASE_ADDRESS, PERM_KDATA, -1);
    map_page_sv39(pagetable, (void*)PAGEROUND(MTIME), PAGEROUND(MTIME), PERM_KDATA, -1);
    map_page_sv39(pagetable, (void*)MTIMECMP_BASE, MTIMECMP_BASE, PERM_KDATA, -1);
    map_page_sv39(pagetable, (void*)PLIC_THRESHOLD, PLIC_THRESHOLD, PERM_KDATA, -1);
    map_page_sv39(pagetable, (void*)UART_BASE, UART_BASE, PERM_KDATA, -1);
    //TODO: map stack
    pa = &rodata_start;
    end = &data_start;
    while (pa != end) {
        map_page_sv39(pagetable, pa, (regsize_t)pa, PERM_KRODATA, -1);
        pa += PAGE_SIZE;
    }
    pa = (void*)PAGEROUND(&data_start);
    end = (void*)PAGEROUNDUP(&_end);
    while (pa != end) {
        map_page_sv39(pagetable, pa, (regsize_t)pa, PERM_KDATA, -1);
        pa += PAGE_SIZE;
    }

    // pre-map all pages in kernel space:
    for (int i = 0; i < MAX_PAGES; i++) {
        page_t *p = &paged_memory.pages[i];
        map_page_sv39(pagetable, p->ptr, (regsize_t)p->ptr, PERM_KDATA, -1);
    }

    // map all special-purpose mamory addresses as kernel-read-writable:
    // map_l2page(l2pte, CLINT0_BASE_ADDRESS, PERM_KDATA);
    // map_l2page(l2pte, PLIC_THRESHOLD, PERM_KDATA);
    // map_l2page(l2pte, UART_BASE, PERM_KDATA);

    // TODO:
    // add page fault handler to kill a user process doing nasty things
    regsize_t satp = MAKE_SATP(pagetable);
    trap_frame.ksatp = satp;
    set_satp(satp);
}

void init_user_vpt(process_t *proc) {
#if !HAS_S_MODE
    return;
#endif
    clear_vpt(proc->uvpt);
    // clear_vpt(proc->uvptl2);
    // clear_vpt(proc->uvptl3);

    // map all of RAM as kernel-executable. Yes, in user page table as well.
    // That's because the kernel sets satp while still executing its own code,
    // and if it weren't mapped as executable, it would page fault. It's safe to
    // do because the userland will not have access to anything mapped for
    // supervisor access anyway.
    // map_superpage(proc->uvpt, &RAM_START, PERM_KCODE);
    /*
    void *pa = &RAM_START;
    for (int i = 0; i < 16; i++) {
        // map_page(proc->uvptl3, pa, PERM_UCODE);
        map_page_sv39(proc->uvpt, pa, (regsize_t)pa, PERM_KCODE, proc->pid);
        pa += PAGE_SIZE;
    }
    */
    copy_kernel_pagemap(proc->uvpt, paged_memory.kpagetable);
    // Ox64: 0x50200000 => ppn2=0x1, ppn1=0x81
    // D1: 0x40200000 => ppn2=0x1, ppn1=0x1

    // set the level-2 and level-3 page tables:
    // proc->uvpt[0] = PHYS_TO_PTE(proc->uvptl2) | PERM_NONLEAF;
    // proc->uvptl2[0] = PHYS_TO_PTE(proc->uvptl3) | PERM_NONLEAF;

    // now map all userland code as user-executable:
    void *user_code_end = &rodata_start;
    void *pa = &user_code_start;
    while (pa != user_code_end) {
        // map_page(proc->uvptl3, pa, PERM_UCODE);
        map_page_sv39(proc->uvpt, pa, UVA(pa), PERM_UCODE, proc->pid);
        pa += PAGE_SIZE;
    }
}

void map_page_sv39(regsize_t *pagetable, void *phys_addr, regsize_t virt_addr, int perm, uint32_t pid) {
    int nonleaf = PERM_NONLEAF;
    // if (perm & PTE_U) {
    //     nonleaf |= PTE_U;
    // }
    for (int level = 2; level >= 0; level--) {
        int vpn_n = VPN(virt_addr, level);
        regsize_t pte = pagetable[vpn_n];
        if (pte == 0 && level > 0) {
            regsize_t *pagetable_next = kalloc("pagetable", pid);
            if (pagetable_next == 0) {
                // TODO: panic OOM
                return;
            }
            clear_vpt(pagetable_next);
            pagetable[vpn_n] = PHYS_TO_PTE(pagetable_next) | nonleaf;
            pagetable = pagetable_next;
            continue;
        }
        if (level > 0) {
            pagetable = PTE_TO_PHYS(pte);
            continue;
        }
        // if (IS_VALID(pte)) {
        //     // TODO: panic - should never remap?
        //     return;
        // }
        pagetable[vpn_n] = PHYS_TO_PTE(phys_addr) | perm | PTE_V;
        break;
    }
}

void copy_kernel_pagemap(regsize_t *upt, regsize_t *kpt) {
    for (int i = 0; i < 512; i++) {
        regsize_t pte = kpt[i];
        // if (pte != 0 && ((pte & PTE_U) == 0)) {
        if (pte != 0 && !IS_USER(pte)) {
            upt[i] = pte;
        }
    }
}

/*
// map_page adds a physical page mapping to a given page table with given
// permissions.
void map_page(regsize_t *pagetable, void *phys_addr, int perm) {
    pagetable[PPN0(phys_addr)] = PHYS_TO_PTE(phys_addr) | perm | PTE_V;
}

void map_l2page(regsize_t *pagetable, regsize_t phys_addr, int perm) {
    pagetable[PPN1(phys_addr)] = PHYS_TO_PTE2(phys_addr) | perm | PTE_V;
}

void map_superpage(regsize_t *pagetable, void *phys_addr, int perm) {
    pagetable[PPN2(phys_addr)] = PHYS_TO_PTE(phys_addr) | perm | PTE_V;
}
*/

void clear_vpt(void *page) {
    for (regsize_t *pte = (regsize_t*)page; pte != page+PAGE_SIZE; pte++) {
        *pte = 0;
    }
}

/*
void free_page_table_r(regsize_t *pt, int level) {
    if (level == 2) {
        release_page(PTE_TO_PHYS(*pt));
        return;
    }
    for (regsize_t *pte = pt; pte != pt+PAGE_SIZE; pte++) {
        if (IS_NONLEAF(*pte) && IS_USER(*pte)) {
            free_page_table_r(PTE_TO_PHYS(*pte), level+1);
        }
    }
}

void free_page_table(regsize_t *pt) {
    free_page_table_r(pt, 0);
    release_page(pt);
}
*/

void free_page_table(regsize_t *pt) {
    void *root = pt;
    for (regsize_t *pte = pt; pte != pt+512; pte++) {
        if (IS_NONLEAF(*pte) && IS_USER(*pte)) {
            void *l2 = PTE_TO_PHYS(*pte);
            for (regsize_t *pte = l2; pte != l2+PAGE_SIZE; pte++) {
                if (IS_NONLEAF(*pte) && IS_USER(*pte)) {
                    void *l3 = PTE_TO_PHYS(*pte);
                    release_page(l3);
                    break;
                }
            }
            release_page(l2);
            break;
        }
    }
    release_page(root);
}

void copy_page_table2(regsize_t *dst, regsize_t *src, uint32_t pid) {
    int nonleaf = PERM_NONLEAF;
    // if (perm & PTE_U) {
    //     nonleaf |= PTE_U;
    // }
    copy_page(dst, src); // copy root table
    regsize_t *l2 = find_next_level_page_table(src);
    if (l2 == 0) {
        return;
    }
    regsize_t *l2dst = kalloc("pagetable", pid);
    if (l2dst == 0) {
        // TODO: panic OOM
        return;
    }
    copy_page(l2dst, l2);
    dst[VPN((regsize_t)l2dst, 2)] = PHYS_TO_PTE(l2dst) | nonleaf;
    regsize_t *l3 = find_next_level_page_table(l2);
    if (l3 == 0) {
        return;
    }
    regsize_t *l3dst = kalloc("pagetable", pid);
    if (l3dst == 0) {
        // TODO: panic OOM
        return;
    }
    copy_page(l3dst, l3);
    l2dst[VPN((regsize_t)l3dst, 1)] = PHYS_TO_PTE(l3dst) | nonleaf;
}

regsize_t* find_next_level_page_table(regsize_t *pagetable) {
    regsize_t *end = pagetable + PAGE_SIZE/sizeof(regsize_t);
    for (regsize_t *pte = pagetable; pte != end; pte++) {
        if (IS_NONLEAF(*pte) && IS_USER(*pte)) {
            return PTE_TO_PHYS(*pte);
        }
    }
    return 0;
}

void copy_page_table(regsize_t *dst, regsize_t *src, uint32_t pid) {
    int nonleaf = PERM_NONLEAF;
    // if (perm & PTE_U) {
    //     nonleaf |= PTE_U;
    // }
    copy_page(dst, src); // copy root table
    int level = 1;
    regsize_t *end = src + 512;
    for (regsize_t *pte = src; pte != end; pte++, level--) {
        if (IS_NONLEAF(*pte)) {
            regsize_t *pagetable_next = kalloc("pagetable", pid);
            if (pagetable_next == 0) {
                // TODO: panic OOM
                return;
            }
            src = PTE_TO_PHYS(*pte);
            pte = src;
            end = src + 512;
            copy_page(pagetable_next, src); // copy level'th table
            dst[VPN((regsize_t)pagetable_next, level)] = PHYS_TO_PTE(pagetable_next) | nonleaf;
            if (level == 0) {
                break;
            }
            dst = pagetable_next;
            continue;
        }
    }
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
