#include "kernel.h"
#include "pagealloc.h"
#include "pmp.h"
#include "riscv.h"
#include "timer.h"

paged_mem_t paged_memory;

void init_paged_memory(void* paged_mem_end, int do_page_report) {
    regsize_t unclaimed_start = (regsize_t)&stack_top;
    paged_memory.lock = 0;
    // up-align at page size to avoid the last page being incomplete:
    regsize_t mem = PAGEROUNDUP(unclaimed_start);
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
    kprintf("init_paged_memory1\n");
    kprintf("init_paged_memory2\n");
    if (do_page_report) {
        kprintf("paged memory: start=%p, end=%p, npages=%d\n",
                paged_mem_start, paged_mem_end, paged_memory.num_pages);
    }
    kprintf("init_paged_memory3\n");
}

// defined in kernel.ld:
extern void* RAM_START;
extern void* user_code_start;
extern void* rodata_start;
extern void* data_start;
extern void* _end;

void dump_pt(regsize_t *pt) {
    for (int i = 0; i < 32; i += 2) {
        kprintf("%d: %p    %p\n", i, pt[i], pt[i+1]);
    }
    pt = PTE_TO_PHYS(pt[1]);
    kprintf("L2: %p\n", pt);
    // int base = 128;
    int base = 0;
    for (int i = base; i < base+32; i += 2) {
        kprintf("%d: %p    %p\n", i, pt[i], pt[i+1]);
    }
    pt = PTE_TO_PHYS(pt[base+1]);
    kprintf("L3: %p\n", pt);
    for (int i = 0; i < 32; i += 2) {
        kprintf("%d: %p    %p\n", i, pt[i], pt[i+1]);
    }
}

void dump_page(void *page) {
    regsize_t *p;
    for (int i = 0; i < 512; i += 2) {
        kprintf("%d: %p    %p\n", i, p[i], p[i+1]);
    }
}

void dump_regs() {
    register regsize_t reg asm ("a0");
    asm volatile ("mv a0, ra" : "=r"(reg));
    kprintf("ra = %p\n", reg);
    asm volatile ("mv a0, sp" : "=r"(reg));
    kprintf("sp = %p\n", reg);
    asm volatile ("mv a0, s0" : "=r"(reg));
    kprintf("s0 = %p\n", reg);
}

void init_vpt() {
    register regsize_t reg1 asm ("a0");
    asm volatile ("mv a0, s0" : "=r"(reg1));
    kprintf("s0 = %p\n", reg1);
#if !HAS_S_MODE
    return;
#endif
    void *pagetable = kalloc("init_vpt: pagetable", -1);
    if (!pagetable) {
        // TODO: panic
        return;
    }
    kprintf("init_vpt: pagetable=%p\n", pagetable);
    paged_memory.kpagetable = pagetable;
    clear_vpt(pagetable);

    kprintf("1 RAM_START=%p, user_code_start=%p\n", &RAM_START, &user_code_start);
    // map kernel code as kernel-executable:
    map_range_id(pagetable, &RAM_START, &user_code_start, PERM_KCODE);
    // Ox64: 0x50200000 => ppn2=0x1, ppn1=0x81
    // D1: 0x40200000 => ppn2=0x1, ppn1=0x1

    kprintf("2\n");
    // map rodata:
    map_range_id(pagetable, &rodata_start, &data_start, PERM_KRODATA);

    kprintf("3\n");
    // map data segment:
    void *start = (void*)PAGEROUND(&data_start);
    void *end = (void*)PAGEROUNDUP(&_end);
    map_range_id(pagetable, start, end, PERM_KDATA);

    kprintf("4\n");
    // pre-map all pages in kernel space:
    for (int i = 0; i < MAX_PAGES; i++) {
        page_t *p = &paged_memory.pages[i];
        map_page_sv39(pagetable, p->ptr, (regsize_t)p->ptr, PERM_KDATA, -1);
    }

    kprintf("5\n");
    // map all special-purpose memory addresses as kernel-read-writable:
    map_page_sv39(pagetable, (void*)PAGEROUND(MTIME), PAGEROUND(MTIME), PERM_KDATA, -1);
    kprintf("6\n");
    map_page_sv39(pagetable, (void*)MTIMECMP_BASE, MTIMECMP_BASE, PERM_KDATA, -1);
    kprintf("7\n");
    map_page_sv39(pagetable, (void*)PLIC_THRESHOLD, PLIC_THRESHOLD, PERM_KDATA, -1);
    kprintf("8\n");
    map_page_sv39(pagetable, (void*)UART_BASE, UART_BASE, PERM_KDATA, -1);

    // dump_pt(pagetable);

    // regsize_t ubsatp = get_satp();
    // uint32_t satph = ubsatp >> 32;
    // uint32_t satpl = ubsatp & 0xffffffff;
    // kprintf("UBOOT SATP satph=%x, satpl=%x\n", satph, satpl);
    // dump_pt((regsize_t*)UNMAKE_SATP(ubsatp));
    register regsize_t reg2 asm ("a0");
    asm volatile ("mv a0, sp" : "=r"(reg2));
    regsize_t *goodsp = (regsize_t*)PAGEROUND(reg2);
    for (int i = 0; i < 512; i += 2) {
        kprintf("%d: %p    %p\n", i, goodsp[i], goodsp[i+1]);
    }

    // TODO:
    // add page fault handler to kill a user process doing nasty things
    regsize_t satp = MAKE_SATP(pagetable);
    trap_frame.ksatp = satp;
    asm volatile ("mv a0, s0" : "=r"(reg1));
    kprintf("s0 = %p\n", reg1);
    // set_satp(satp);
    regsize_t invalidate = SMCIR_INVALIDALL;
    asm volatile (
        "csrw " THEAD_SMCIR_CSR ", %0"
        :
        : "r"(invalidate)
    );
    asm volatile(".word 0x1b0000b");  // sync.is
    kprintf("THEAD_SMCIR_CSR done\n");
    asm volatile (
        "sfence.vma zero, zero  \n\
         csrw satp, %0          \n\
         sfence.vma zero, zero"
        :                 // no output
        : "r"(satp)      // input in value
    );

    asm volatile ("mv a0, s0" : "=r"(reg1));
    kprintf("s0 = %p\n", reg1);
    kprintf("/set_satp\n");
    asm volatile ("mv a0, s0" : "=r"(reg1));
    kprintf("s0 = %p\n", reg1);
    kprintf("post1\n");
    kprintf("post2\n");
    kprintf("post3\n");
    kprintf("post4\n");
    kprintf("post5\n");
    kprintf("post6\n");
    kprintf("post7\n");
    register regsize_t reg asm ("a0");
    asm volatile ("mv a0, ra" : "=r"(reg));
    kprintf("ra = %p\n", reg);
    asm volatile ("mv a0, sp" : "=r"(reg));
    regsize_t *sp = (regsize_t*)PAGEROUND(reg);
    kprintf("sp = %p\n", reg);
    asm volatile ("mv a0, s0" : "=r"(reg));
    kprintf("s0 = %p\n", reg);
    kprintf("----\n");
    asm volatile ("ld a0, 56(sp)" : "=r"(reg));
    kprintf("ra = %p\n", reg);
    asm volatile ("ld a0, 48(sp)" : "=r"(reg));
    kprintf("s0 = %p\n", reg);
    for (int i = 0; i < 512; i += 2) {
        kprintf("%d: %p    %p\n", i, sp[i], sp[i+1]);
    }
}

void init_user_vpt(void *pagetable, uint32_t pid) {
#if !HAS_S_MODE
    return;
#endif
    clear_vpt(pagetable);

    // Copy non-leaf pointers from the kernel pagetable to user. That's because
    // the kernel sets satp while still executing its own code, and if it
    // weren't mapped as executable, it would page fault. It's safe to do
    // because the userland will not have access to anything mapped for
    // supervisor access anyway.
    // regsize_t *kpt = (regsize_t*)0x40212000;
    copy_kernel_pagemap(pagetable, paged_memory.kpagetable);
    // regsize_t *pt = pagetable;
    // pt[1] = 0x10084c01;

    // now map all userland code as user-executable:
    void *start = &user_code_start;
    void *end = &rodata_start;
    map_range(pagetable, start, end, (void*)UVA(start), PERM_UCODE, pid);
    // dump_pt(pagetable);
}

void map_page_sv39(regsize_t *pagetable, void *phys_addr, regsize_t virt_addr, int perm, uint32_t pid) {
    kprintf("map_page_sv39: pagetable=%p, phys_addr=%p, virt_addr=%x\n", pagetable, phys_addr, virt_addr);
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
            pagetable[vpn_n] = PHYS_TO_PTE(pagetable_next) | PERM_NONLEAF;
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
        pagetable[vpn_n] = PHYS_TO_PTE(phys_addr) | perm | PTE_V | PTE_A | PTE_D;
        break;
    }
}

void map_range(void *pagetable, void *pa_start, void *pa_end, void *va_start, int perm, uint32_t pid) {
    void *page_pa = pa_start;
    regsize_t va = (regsize_t)va_start;
    while (page_pa != pa_end) {
        map_page_sv39(pagetable, page_pa, va, perm, -1);
        page_pa += PAGE_SIZE;
        va += PAGE_SIZE;
    }
}

void map_range_id(void *pagetable, void *pa_start, void *pa_end, int perm) {
    map_range(pagetable, pa_start, pa_end, pa_start, perm, -1);
}

void copy_kernel_pagemap(regsize_t *upt, regsize_t *kpt) {
    kprintf("copy_kernel_pagemap: upt=%p, kpt=%p\n", upt, kpt);
    for (int i = 0; i < PAGE_SIZE/sizeof(regsize_t); i++) {
        kprintf("copy_kernel_pagemap: kpt[%d] = %p\n", i, *kpt);
        if ((*kpt & 0xf) == PERM_NONLEAF) {
            upt[i] = *kpt;
        }
        kpt++;
    }
}

void clear_vpt(void *page) {
    for (regsize_t *pte = (regsize_t*)page; pte != page+PAGE_SIZE; pte++) {
        *pte = 0;
    }
}

void free_page_table_r(regsize_t *pt, int level) {
    regsize_t *end = pt + PAGE_SIZE/sizeof(regsize_t);
    for (regsize_t *pte = pt; pte != end; pte++) {
        if (IS_NONLEAF(*pte) && IS_USER(*pte)) {
            if (level == 1) {
                release_page(PTE_TO_PHYS(*pte));
                continue;
            }
            free_page_table_r(PTE_TO_PHYS(*pte), level+1);
        }
    }
    if (level == 1) {
        release_page(PTE_TO_PHYS(*pt));
    }
}

void free_page_table(regsize_t *pt) {
    free_page_table_r(pt, 0);
    release_page(pt);
}

void copy_page_table(regsize_t *dst, regsize_t *src, uint32_t pid) {
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
    dst[VPN((regsize_t)l2dst, 2)] = PHYS_TO_PTE(l2dst) | PERM_NONLEAF;
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
    l2dst[VPN((regsize_t)l3dst, 1)] = PHYS_TO_PTE(l3dst) | PERM_NONLEAF;
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
